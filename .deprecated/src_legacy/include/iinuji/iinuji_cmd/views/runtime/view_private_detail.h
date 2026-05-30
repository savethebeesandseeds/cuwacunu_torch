struct runtime_pid_job_match_t {
  const cuwacunu::hero::runtime::runtime_job_record_t *job{nullptr};
  std::string role{};
};

inline std::optional<runtime_pid_job_match_t>
runtime_find_job_for_pid(const CmdState &st, std::uint64_t pid) {
  if (pid == 0)
    return std::nullopt;
  if (const auto target_it = st.runtime.target_job_index_by_pid.find(pid);
      target_it != st.runtime.target_job_index_by_pid.end() &&
      target_it->second < st.runtime.jobs.size()) {
    return runtime_pid_job_match_t{.job = &st.runtime.jobs[target_it->second],
                                   .role = "target"};
  }
  if (const auto runner_it = st.runtime.runner_job_index_by_pid.find(pid);
      runner_it != st.runtime.runner_job_index_by_pid.end() &&
      runner_it->second < st.runtime.jobs.size()) {
    return runtime_pid_job_match_t{.job = &st.runtime.jobs[runner_it->second],
                                   .role = "runner"};
  }
  return std::nullopt;
}

inline std::string runtime_job_display_label(
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  return job.binding_id.empty() ? trim_to_width(job.job_cursor, 32)
                                : trim_to_width(job.binding_id, 32);
}

inline bool runtime_selected_none_session(const CmdState &st) {
  return runtime_is_none_session_selection(st.runtime.selected_session_id);
}

inline bool runtime_selected_none_campaign(const CmdState &st) {
  return runtime_is_none_campaign_selection(
      st.runtime.selected_campaign_cursor);
}

inline std::string runtime_selected_device_label(const CmdState &st);

inline std::string runtime_inventory_summary_text(const CmdState &st) {
  std::ostringstream oss;
  oss << count_runtime_active_sessions(st) << "/" << st.runtime.sessions.size()
      << " marshals | " << count_runtime_active_campaigns(st) << "/"
      << st.runtime.campaigns.size() << " campaigns | "
      << count_runtime_active_jobs(st) << "/" << st.runtime.jobs.size()
      << " jobs";
  return oss.str();
}

inline std::string runtime_job_process_text(
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  std::vector<std::string> tokens{};
  if (job.runner_pid.has_value()) {
    tokens.push_back("runner=" + std::to_string(*job.runner_pid));
  }
  if (job.target_pid.has_value()) {
    tokens.push_back("target=" + std::to_string(*job.target_pid));
  }
  if (job.target_pgid.has_value()) {
    tokens.push_back("pgid=" + std::to_string(*job.target_pgid));
  }
  if (job.exit_code.has_value()) {
    tokens.push_back("exit=" + std::to_string(*job.exit_code));
  }
  if (job.term_signal.has_value()) {
    tokens.push_back("signal=" + std::to_string(*job.term_signal));
  }
  if (tokens.empty())
    return "<none>";
  std::ostringstream oss;
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    if (i > 0)
      oss << " | ";
    oss << tokens[i];
  }
  return oss.str();
}

inline std::string runtime_format_binary_kib(std::uint64_t kib) {
  std::ostringstream oss;
  if (kib >= 1024ull * 1024ull) {
    oss << std::fixed << std::setprecision(1)
        << (static_cast<double>(kib) / (1024.0 * 1024.0)) << " GiB";
    return oss.str();
  }
  if (kib >= 1024ull) {
    oss << std::fixed << std::setprecision(1)
        << (static_cast<double>(kib) / 1024.0) << " MiB";
    return oss.str();
  }
  oss << kib << " KiB";
  return oss.str();
}

inline std::string runtime_format_binary_mib(std::uint64_t mib) {
  std::ostringstream oss;
  if (mib >= 1024ull) {
    oss << std::fixed << std::setprecision(1)
        << (static_cast<double>(mib) / 1024.0) << " GiB";
    return oss.str();
  }
  oss << mib << " MiB";
  return oss.str();
}

inline std::string runtime_format_scalar(double value, int precision = 2) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(precision) << value;
  return oss.str();
}

inline std::string runtime_format_ratio_percent(std::uint64_t used,
                                                std::uint64_t total) {
  if (total == 0)
    return "?%";
  const auto pct = static_cast<int>(
      (100.0 * static_cast<double>(used)) / static_cast<double>(total) + 0.5);
  return std::to_string(std::clamp(pct, 0, 100)) + "%";
}

inline std::string runtime_selected_device_label(const CmdState &st) {
  const std::size_t selected = runtime_selected_device_index(st).value_or(0);
  if (selected == 0) {
    return "HOST cpu/ram";
  }
  const std::size_t gpu_index = selected - 1;
  if (gpu_index >= st.runtime.device.gpus.size())
    return "<gpu>";
  const auto &gpu = st.runtime.device.gpus[gpu_index];
  return "GPU" + (gpu.index.empty() ? std::string("?") : gpu.index) + " cuda";
}

inline std::size_t
runtime_gpu_compute_process_count(const CmdState &st,
                                  const RuntimeDeviceGpuSnapshot &gpu) {
  return static_cast<std::size_t>(std::count_if(
      st.runtime.device.gpu_processes.begin(),
      st.runtime.device.gpu_processes.end(),
      [&](const auto &proc) { return proc.gpu_uuid == gpu.uuid; }));
}

inline std::string runtime_gpu_label_for_uuid(const CmdState &st,
                                              std::string_view gpu_uuid) {
  for (const auto &gpu : st.runtime.device.gpus) {
    if (gpu.uuid != gpu_uuid)
      continue;
    return "gpu" + (gpu.index.empty() ? std::string("?") : gpu.index);
  }
  return gpu_uuid.empty() ? std::string("gpu?") : std::string(gpu_uuid);
}

inline std::vector<const RuntimeDeviceGpuProcessSnapshot *>
runtime_gpu_process_refs(const CmdState &st,
                         const RuntimeDeviceGpuSnapshot &gpu) {
  std::vector<const RuntimeDeviceGpuProcessSnapshot *> refs{};
  for (const auto &proc : st.runtime.device.gpu_processes) {
    if (proc.gpu_uuid != gpu.uuid)
      continue;
    refs.push_back(&proc);
  }
  return refs;
}

inline std::vector<const RuntimeDeviceGpuProcessSnapshot *>
runtime_gpu_process_refs_for_pid(const CmdState &st, std::uint64_t pid) {
  std::vector<const RuntimeDeviceGpuProcessSnapshot *> refs{};
  if (pid == 0)
    return refs;
  for (const auto &proc : st.runtime.device.gpu_processes) {
    if (proc.pid != pid)
      continue;
    refs.push_back(&proc);
  }
  return refs;
}

inline void append_runtime_device_bar_meta(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::string_view key, int pct, std::string suffix,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Info) {
  const int clamped_pct = std::clamp(pct, 0, 100);
  const auto bar_emphasis =
      emphasis == cuwacunu::iinuji::text_line_emphasis_t::Info
          ? runtime_percent_emphasis(clamped_pct)
          : emphasis;
  append_runtime_detail_segments(
      out, key,
      {
          cuwacunu::iinuji::styled_text_segment_t{
              .text = runtime_ascii_progress_bar(clamped_pct),
              .emphasis = bar_emphasis,
          },
          cuwacunu::iinuji::styled_text_segment_t{
              .text = " " + std::to_string(clamped_pct) + "%",
              .emphasis = bar_emphasis,
          },
          cuwacunu::iinuji::styled_text_segment_t{
              .text = "  " + std::move(suffix),
              .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Debug,
          },
      });
}

inline void append_runtime_selected_host_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const CmdState &st) {
  const auto &device = st.runtime.device;
  if (!device.ok && device.collected_at_ms == 0 && device.error.empty()) {
    append_runtime_detail_text(out, "device snapshot pending...",
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }
  append_runtime_detail_meta(out, "type", "host cpu/ram",
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_runtime_detail_meta(out, "host",
                             device.host_name.empty() ? std::string("<host>")
                                                      : device.host_name,
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Success);
  append_runtime_detail_meta(out, "cpu_count", std::to_string(device.cpu_count),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Accent);
  if (device.cpu_usage_pct >= 0) {
    append_runtime_device_bar_meta(out, "cpu", runtime_host_cpu_percent(device),
                                   "sampled from /proc/stat");
    if (device.cpu_iowait_pct >= 0) {
      append_runtime_device_bar_meta(
          out, "iowait", device.cpu_iowait_pct,
          "share of sampled cpu time spent waiting on io",
          cuwacunu::iinuji::text_line_emphasis_t::Debug);
    }
  } else {
    append_runtime_detail_meta(out, "cpu", "awaiting second sample",
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
  }
  append_runtime_detail_meta(out, "loadavg",
                             runtime_format_scalar(device.load1) + " / " +
                                 runtime_format_scalar(device.load5) + " / " +
                                 runtime_format_scalar(device.load15),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Debug);
  if (device.mem_total_kib != 0) {
    const std::uint64_t used_kib =
        device.mem_total_kib > device.mem_available_kib
            ? device.mem_total_kib - device.mem_available_kib
            : 0;
    append_runtime_device_bar_meta(
        out, "ram", runtime_host_memory_percent(device),
        runtime_format_binary_kib(used_kib) + " / " +
            runtime_format_binary_kib(device.mem_total_kib));
  }
  append_runtime_detail_meta(
      out, "observed_gpus",
      std::to_string(device.gpus.size()) + " queried cuda device" +
          (device.gpus.size() == 1 ? "" : "s"),
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      device.gpus.empty() ? cuwacunu::iinuji::text_line_emphasis_t::Debug
                          : cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_runtime_detail_meta(out, "runtime", runtime_inventory_summary_text(st),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Debug);
  append_runtime_device_updated_detail(out, st);
  if (!device.gpu_error.empty()) {
    append_runtime_detail_meta(out, "gpu_probe", device.gpu_error,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  if (!device.error.empty()) {
    append_runtime_detail_meta(out, "device_error", device.error,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
}

inline void append_runtime_selected_gpu_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    const RuntimeDeviceGpuSnapshot &gpu) {
  const auto proc_refs = runtime_gpu_process_refs(st, gpu);
  append_runtime_detail_meta(
      out, "type",
      "gpu" + (gpu.index.empty() ? std::string("?") : gpu.index) + " cuda",
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_runtime_detail_meta(out, "name",
                             gpu.name.empty() ? std::string("<gpu>") : gpu.name,
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Success);
  append_runtime_device_bar_meta(
      out, "core", std::max(0, gpu.util_gpu_pct),
      "compute proc " +
          std::to_string(runtime_gpu_compute_process_count(st, gpu)));
  const int mem_pct = runtime_gpu_memory_percent(gpu);
  append_runtime_device_bar_meta(
      out, "memory", mem_pct,
      runtime_format_binary_mib(gpu.memory_used_mib) + " / " +
          runtime_format_binary_mib(gpu.memory_total_mib));
  if (gpu.temperature_c >= 0) {
    append_runtime_detail_meta(out, "temperature",
                               std::to_string(gpu.temperature_c) + " °C",
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  if (!gpu.uuid.empty()) {
    append_runtime_detail_meta(out, "uuid", gpu.uuid,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
  }
  append_runtime_detail_meta(
      out, "processes", std::to_string(proc_refs.size()) + " attached",
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      proc_refs.empty() ? cuwacunu::iinuji::text_line_emphasis_t::Debug
                        : cuwacunu::iinuji::text_line_emphasis_t::Accent);
  for (const auto *proc : proc_refs) {
    if (proc == nullptr)
      continue;
    const auto job_match = runtime_find_job_for_pid(st, proc->pid);
    const auto proc_state = runtime_probe_pid_resources(proc->pid);
    std::ostringstream oss;
    oss << "pid " << proc->pid;
    if (job_match.has_value() && job_match->job != nullptr) {
      oss << "  " << job_match->role << " "
          << runtime_job_display_label(*job_match->job);
    } else if (!proc_state.name.empty()) {
      oss << "  " << proc_state.name;
    }
    if (proc_state.rss_kib != 0) {
      oss << "  rss " << runtime_format_binary_kib(proc_state.rss_kib);
    }
    if (proc->used_memory_mib != 0) {
      oss << "  vram " << runtime_format_binary_mib(proc->used_memory_mib);
    }
    append_runtime_detail_meta(
        out, "proc", oss.str(), cuwacunu::iinuji::text_line_emphasis_t::Info,
        job_match.has_value() ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                              : cuwacunu::iinuji::text_line_emphasis_t::Debug);
  }
  append_runtime_device_updated_detail(out, st);
  if (!st.runtime.device.error.empty()) {
    append_runtime_detail_meta(out, "device_error", st.runtime.device.error,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
}

inline void append_runtime_selected_device_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const CmdState &st) {
  const std::size_t selected = runtime_selected_device_index(st).value_or(0);
  if (selected == 0) {
    append_runtime_selected_host_detail(out, st);
    return;
  }
  const std::size_t gpu_index = selected - 1;
  if (gpu_index >= st.runtime.device.gpus.size()) {
    append_runtime_detail_text(out, "unavailable",
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }
  append_runtime_selected_gpu_detail(out, st,
                                     st.runtime.device.gpus[gpu_index]);
}

inline void append_runtime_none_session_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const CmdState &st) {
  append_runtime_detail_meta(out, "state", "orphan branch",
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_runtime_detail_meta(out, "marshal_session_id",
                             runtime_none_branch_label(),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_runtime_detail_meta(
      out, "campaigns",
      std::to_string(runtime_orphan_campaign_indices(st).size()),
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_runtime_detail_meta(
      out, "jobs", std::to_string(runtime_orphan_job_indices(st).size()),
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  if (st.runtime.campaign.has_value()) {
    append_runtime_detail_meta(
        out, "selected_campaign",
        runtime_campaign_ref_text(st.runtime.campaign->campaign_cursor),
        cuwacunu::iinuji::text_line_emphasis_t::Info,
        runtime_campaign_row_emphasis(*st.runtime.campaign));
  } else if (runtime_selected_none_campaign(st)) {
    append_runtime_detail_meta(out, "selected_campaign",
                               runtime_none_branch_label(),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  if (st.runtime.job.has_value()) {
    append_runtime_detail_meta(out, "selected_job",
                               runtime_job_display_label(*st.runtime.job),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               runtime_job_row_emphasis(*st.runtime.job));
  }
  append_runtime_note_block(
      out,
      "This synthetic marshal branch groups campaigns without a linked "
      "marshal and jobs whose lineage does not resolve back to a marshal.",
      "Branch Note", cuwacunu::iinuji::text_line_emphasis_t::Accent,
      cuwacunu::iinuji::text_line_emphasis_t::Debug);
}

inline std::string runtime_report_preview_text(std::string text,
                                               std::size_t max_lines = 36) {
  text = trim_copy(std::move(text));
  if (text.empty())
    return {};

  const std::size_t supporting_pos = text.find("\n## Supporting Files");
  if (supporting_pos != std::string::npos) {
    text = trim_copy(text.substr(0, supporting_pos));
  }

  std::ostringstream out;
  std::size_t line_count = 0;
  for (const auto &line : split_lines_keep_empty(text)) {
    if (line_count >= max_lines) {
      out << "\n...";
      break;
    }
    if (line_count != 0)
      out << '\n';
    out << line;
    ++line_count;
  }
  return trim_copy(out.str());
}

inline void append_runtime_session_report_preview(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  const std::string report_path = runtime_session_summary_path(st, session);
  append_runtime_detail_meta(out, "report_path", runtime_path_text(report_path),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_path_emphasis(report_path));
  if (report_path.empty())
    return;

  std::string report_text{};
  std::string ignored_error{};
  if (!read_text_file_safe(report_path, &report_text, &ignored_error)) {
    append_runtime_detail_meta(out, "report_preview",
                               ignored_error.empty()
                                   ? std::string("<unavailable>")
                                   : ignored_error,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }
  append_runtime_note_block(
      out, runtime_report_preview_text(std::move(report_text)),
      "Report of Affairs", cuwacunu::iinuji::text_line_emphasis_t::Accent,
      cuwacunu::iinuji::text_line_emphasis_t::None);
}

inline void append_runtime_session_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    bool primary) {
  append_runtime_detail_section(
      out, primary ? "Selected Marshal" : "Linked Marshal",
      primary ? cuwacunu::iinuji::text_line_emphasis_t::Accent
              : cuwacunu::iinuji::text_line_emphasis_t::Info);
  if (runtime_selected_none_session(st)) {
    append_runtime_none_session_detail(out, st);
    return;
  }
  if (!st.runtime.session.has_value()) {
    append_runtime_detail_text(out, "unavailable",
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }
  const auto &session = *st.runtime.session;
  append_runtime_detail_meta(out, "state", runtime_session_state_text(session),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_session_row_emphasis(st, session));
  if (cuwacunu::hero::marshal::is_marshal_session_summary_state(session)) {
    const bool pending_review =
        operator_session_has_pending_review(st, session.marshal_session_id);
    append_runtime_detail_meta(
        out, "report",
        pending_review ? std::string("review pending")
                       : std::string("acknowledged"),
        cuwacunu::iinuji::text_line_emphasis_t::Info,
        pending_review ? cuwacunu::iinuji::text_line_emphasis_t::Warning
                       : cuwacunu::iinuji::text_line_emphasis_t::Info);
    append_runtime_session_report_preview(out, st, session);
  }
  append_runtime_detail_meta(out, "objective",
                             session.objective_name.empty()
                                 ? std::string("<unnamed objective>")
                                 : session.objective_name);
  append_runtime_detail_meta(out, "marshal_session_id",
                             session.marshal_session_id);
  if (!session.campaign_cursor.empty()) {
    append_runtime_detail_meta(
        out, "active_campaign",
        runtime_campaign_ref_text(session.campaign_cursor));
  }
  append_runtime_detail_meta(out, "campaigns",
                             std::to_string(count_runtime_campaigns_for_session(
                                 st, session.marshal_session_id)));
  append_runtime_detail_meta(out, "jobs",
                             std::to_string(count_runtime_jobs_for_session(
                                 st, session.marshal_session_id)));
  append_runtime_detail_meta(out, "started_at",
                             format_time_marker_ms(session.started_at_ms));
  append_runtime_detail_meta(out, "updated_at",
                             format_time_marker_ms(session.updated_at_ms));
  if (session.finished_at_ms.has_value()) {
    append_runtime_detail_meta(
        out, "finished_at",
        format_optional_time_marker_ms(session.finished_at_ms));
  }
  if (session.work_gate == "operator_pause") {
    append_runtime_detail_meta(out, "work_gate", session.work_gate,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  if (!session.finish_reason.empty() && session.finish_reason != "none") {
    append_runtime_detail_meta(out, "finish_reason", session.finish_reason,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  if (!session.last_warning.empty()) {
    append_runtime_detail_meta(out, "last_warning", session.last_warning,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  if (!session.status_detail.empty()) {
    append_runtime_note_block(out, session.status_detail, "Marshal Note",
                              cuwacunu::iinuji::text_line_emphasis_t::Accent,
                              cuwacunu::iinuji::text_line_emphasis_t::None);
  }
}

inline void append_runtime_none_campaign_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const CmdState &st) {
  append_runtime_detail_meta(out, "state", "orphan branch",
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_runtime_detail_meta(out, "campaign_cursor",
                             runtime_none_branch_label(),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_runtime_detail_meta(out, "parent_marshal", runtime_none_branch_label(),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_runtime_detail_meta(
      out, "job_count", std::to_string(runtime_orphan_job_indices(st).size()),
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  if (st.runtime.job.has_value()) {
    append_runtime_detail_meta(out, "selected_job",
                               runtime_job_display_label(*st.runtime.job),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               runtime_job_row_emphasis(*st.runtime.job));
  }
  append_runtime_note_block(
      out,
      "This synthetic campaign branch groups jobs that do not resolve to a "
      "runtime campaign.",
      "Branch Note", cuwacunu::iinuji::text_line_emphasis_t::Accent,
      cuwacunu::iinuji::text_line_emphasis_t::Debug);
}

inline void append_runtime_campaign_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    bool primary) {
  append_runtime_detail_section(
      out, primary ? "Selected Campaign" : "Linked Campaign",
      primary ? cuwacunu::iinuji::text_line_emphasis_t::Accent
              : cuwacunu::iinuji::text_line_emphasis_t::Info);
  if (runtime_selected_none_campaign(st)) {
    append_runtime_none_campaign_detail(out, st);
    return;
  }
  if (!st.runtime.campaign.has_value()) {
    append_runtime_detail_text(out, "unavailable",
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }
  const auto &campaign = *st.runtime.campaign;
  append_runtime_detail_meta(out, "state", campaign.state,
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_campaign_row_emphasis(campaign));
  append_runtime_detail_meta(out, "campaign_cursor", campaign.campaign_cursor);
  const std::string parent_session_id =
      runtime_campaign_parent_session_id(st, campaign);
  append_runtime_detail_meta(out, "parent_marshal",
                             runtime_session_ref_text(parent_session_id));
  append_runtime_detail_meta(out, "current_run",
                             campaign.current_run_index.has_value()
                                 ? std::to_string(*campaign.current_run_index)
                                 : std::string("<none>"));
  append_runtime_detail_meta(out, "bind_count",
                             std::to_string(campaign.run_bind_ids.size()));
  append_runtime_detail_meta(out, "job_count",
                             std::to_string(count_runtime_jobs_for_campaign(
                                 st, campaign.campaign_cursor)));
  if (campaign.runner_pid.has_value()) {
    append_runtime_detail_meta(out, "runner_pid",
                               std::to_string(*campaign.runner_pid));
  }
  append_runtime_detail_meta(out, "started_at",
                             format_time_marker_ms(campaign.started_at_ms));
  append_runtime_detail_meta(out, "updated_at",
                             format_time_marker_ms(campaign.updated_at_ms));
  if (campaign.finished_at_ms.has_value()) {
    append_runtime_detail_meta(
        out, "finished_at",
        format_optional_time_marker_ms(campaign.finished_at_ms));
  }
  if (!campaign.state_detail.empty()) {
    append_runtime_note_block(out, campaign.state_detail, "Campaign Note",
                              cuwacunu::iinuji::text_line_emphasis_t::Info,
                              cuwacunu::iinuji::text_line_emphasis_t::None);
  }
}

inline cuwacunu::iinuji::text_line_emphasis_t runtime_job_device_emphasis(
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  if (job.resolved_device.empty()) {
    return cuwacunu::iinuji::text_line_emphasis_t::Debug;
  }
  if (job.cuda_required &&
      job.resolved_device.find("cpu") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::MutedError;
  }
  if (job.resolved_device.find("cuda") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Debug;
}

inline void append_runtime_job_gpu_attachment_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    std::string_view label, const std::optional<std::uint64_t> &pid) {
  if (!pid.has_value())
    return;
  const auto refs = runtime_gpu_process_refs_for_pid(st, *pid);
  for (const auto *ref : refs) {
    if (ref == nullptr)
      continue;
    std::ostringstream oss;
    oss << label << " " << runtime_gpu_label_for_uuid(st, ref->gpu_uuid);
    if (ref->used_memory_mib != 0) {
      oss << " " << runtime_format_binary_mib(ref->used_memory_mib);
    }
    append_runtime_detail_meta(out, "gpu_attach", oss.str(),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Accent);
  }
}

inline void append_runtime_job_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    bool primary) {
  append_runtime_detail_section(
      out, primary ? "Selected Job" : "Linked Job",
      primary ? cuwacunu::iinuji::text_line_emphasis_t::Accent
              : cuwacunu::iinuji::text_line_emphasis_t::Info);
  if (!st.runtime.job.has_value()) {
    append_runtime_detail_text(out, "unavailable",
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }
  const auto &job = *st.runtime.job;
  append_runtime_detail_meta(out, "state", job.state,
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_job_row_emphasis(job));
  append_runtime_detail_meta(out, "binding_id",
                             job.binding_id.empty() ? std::string("<none>")
                                                    : job.binding_id);
  append_runtime_detail_meta(out, "job_cursor", job.job_cursor);
  const std::string parent_campaign =
      runtime_job_parent_campaign_cursor(st, job);
  const std::string parent_session = runtime_job_parent_session_id(st, job);
  append_runtime_detail_meta(out, "parent_campaign",
                             runtime_campaign_ref_text(parent_campaign));
  append_runtime_detail_meta(out, "parent_marshal",
                             runtime_session_ref_text(parent_session));
  const std::string device_request = job.requested_device.empty()
                                         ? std::string("<default>")
                                         : job.requested_device;
  const std::string device_resolved = job.resolved_device.empty()
                                          ? std::string("<unknown>")
                                          : job.resolved_device;
  append_runtime_detail_meta(out, "device",
                             device_request + " -> " + device_resolved,
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_job_device_emphasis(job));
  if (!job.device_error.empty()) {
    append_runtime_detail_meta(out, "device_error", job.device_error,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  append_runtime_detail_meta(out, "process", runtime_job_process_text(job),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_job_row_emphasis(job));
  append_runtime_job_gpu_attachment_detail(out, st, "runner", job.runner_pid);
  append_runtime_job_gpu_attachment_detail(out, st, "target", job.target_pid);
  append_runtime_detail_meta(out, "started_at",
                             format_time_marker_ms(job.started_at_ms));
  append_runtime_detail_meta(out, "updated_at",
                             format_time_marker_ms(job.updated_at_ms));
  if (job.finished_at_ms.has_value()) {
    append_runtime_detail_meta(
        out, "finished_at", format_optional_time_marker_ms(job.finished_at_ms));
  }
  if (!job.state_detail.empty()) {
    append_runtime_note_block(out, job.state_detail, "Job Note",
                              cuwacunu::iinuji::text_line_emphasis_t::Info,
                              cuwacunu::iinuji::text_line_emphasis_t::None);
  }
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_manifest_primary_styled_lines(const CmdState &st,
                                           RuntimeRowKind primary_kind);

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_manifest_primary_styled_lines(const CmdState &st,
                                           RuntimeRowKind primary_kind) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out{};
  switch (primary_kind) {
  case RuntimeRowKind::Device:
    append_runtime_selected_device_detail(out, st);
    break;
  case RuntimeRowKind::Session:
    append_runtime_session_detail(out, st, true);
    break;
  case RuntimeRowKind::Campaign:
    append_runtime_campaign_detail(out, st, true);
    break;
  case RuntimeRowKind::Job:
    append_runtime_job_detail(out, st, true);
    break;
  }
  return out;
}

inline void append_runtime_artifact_section(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::string_view title, const std::string &path, std::string note,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Info) {
  append_runtime_detail_section(out, title, emphasis);
  append_runtime_detail_meta(out, "path", runtime_path_text(path),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_path_emphasis(path));
  append_runtime_detail_text(out, std::move(note),
                             cuwacunu::iinuji::text_line_emphasis_t::Debug);
}

inline void append_runtime_excerpt_section(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::string_view title, const std::string &path, const std::string &excerpt,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Info) {
  append_runtime_detail_section(out, title, emphasis);
  append_runtime_detail_meta(out, "path", runtime_path_text(path),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_path_emphasis(path));
  append_runtime_detail_text(out, runtime_excerpt_text(excerpt),
                             cuwacunu::iinuji::text_line_emphasis_t::Debug);
}

inline bool runtime_try_parse_json_object_line(const std::string &raw_line,
                                               cmd_json_value_t *out) {
  if (out == nullptr)
    return false;
  *out = cmd_json_value_t{};
  try {
    *out = cuwacunu::piaabo::JsonParser(raw_line).parse();
  } catch (...) {
    return false;
  }
  return out->type == cmd_json_type_t::OBJECT;
}

inline std::string runtime_json_first_string(const cmd_json_value_t *object,
                                             std::string_view key_a,
                                             std::string_view key_b = {},
                                             std::string_view key_c = {},
                                             std::string_view key_d = {},
                                             std::string_view key_e = {}) {
  const std::string_view keys[] = {key_a, key_b, key_c, key_d, key_e};
  for (const auto key : keys) {
    if (key.empty())
      continue;
    const std::string value = cmd_json_string(cmd_json_field(object, key));
    if (!value.empty())
      return value;
  }
  return {};
}

inline std::string runtime_compact_log_text(std::string text,
                                            std::size_t limit = 116) {
  text = trim_copy(text);
  if (text.empty())
    return "<empty>";
  return trim_to_width(text, limit);
}

inline cuwacunu::iinuji::text_line_emphasis_t
runtime_marshal_event_emphasis(std::string_view event_name) {
  return runtime_marshaled_event_emphasis_from_name(event_name);
}

inline std::string
runtime_event_viewer_background(cuwacunu::iinuji::text_line_emphasis_t emphasis,
                                bool body_line, bool active) {
  switch (emphasis) {
  case cuwacunu::iinuji::text_line_emphasis_t::Fatal:
  case cuwacunu::iinuji::text_line_emphasis_t::Error:
  case cuwacunu::iinuji::text_line_emphasis_t::MutedError:
    if (active)
      return body_line ? "#181015" : "#22131a";
    return body_line ? "#120c10" : "#171015";
  case cuwacunu::iinuji::text_line_emphasis_t::Warning:
  case cuwacunu::iinuji::text_line_emphasis_t::MutedWarning:
    if (active)
      return body_line ? "#17140f" : "#201a12";
    return body_line ? "#12100c" : "#17140e";
  case cuwacunu::iinuji::text_line_emphasis_t::Success:
    if (active)
      return body_line ? "#111712" : "#152017";
    return body_line ? "#0d120e" : "#101711";
  case cuwacunu::iinuji::text_line_emphasis_t::Accent:
    if (active)
      return body_line ? "#14131a" : "#1a1722";
    return body_line ? "#100f16" : "#14131c";
  case cuwacunu::iinuji::text_line_emphasis_t::Info:
  case cuwacunu::iinuji::text_line_emphasis_t::Debug:
  case cuwacunu::iinuji::text_line_emphasis_t::None:
    if (active)
      return body_line ? "#131722" : "#171c29";
    return body_line ? "#10131c" : "#131722";
  }
  return active ? (body_line ? "#131722" : "#171c29")
                : (body_line ? "#10131c" : "#131722");
}

inline cuwacunu::iinuji::text_line_emphasis_t
runtime_event_metadata_emphasis(std::string_view key, std::string_view value) {
  const std::string lowered_key = to_lower_copy(std::string(key));
  if (lowered_key == "payload.text" ||
      (lowered_key.rfind("payload.", 0) == 0 &&
       lowered_key.find(".text") != std::string::npos)) {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  const std::string merged = lowered_key + " " + std::string(value);
  if (merged.find("fail") != std::string::npos ||
      merged.find("error") != std::string::npos ||
      merged.find("terminate") != std::string::npos ||
      merged.find("deny") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::MutedError;
  }
  if (merged.find("pause") != std::string::npos ||
      merged.find("warn") != std::string::npos ||
      merged.find("park") != std::string::npos ||
      merged.find("block") != std::string::npos ||
      merged.find("retry") != std::string::npos ||
      merged.find("stderr") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (merged.find("checkpoint") != std::string::npos ||
      merged.find("launch") != std::string::npos ||
      merged.find("campaign") != std::string::npos ||
      merged.find("tool") != std::string::npos ||
      merged.find("stream") != std::string::npos ||
      merged.find("line") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  }
  if (merged.find("complete") != std::string::npos ||
      merged.find("deliver") != std::string::npos ||
      merged.find("success") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Debug;
}

inline std::string
runtime_structured_viewer_subject(RuntimeLogViewerKind kind) {
  switch (kind) {
  case RuntimeLogViewerKind::MarshalEvents:
    return "timeline";
  case RuntimeLogViewerKind::MarshalCodexStdout:
    return "codex stdout";
  case RuntimeLogViewerKind::MarshalCodexStderr:
    return "codex stderr";
  case RuntimeLogViewerKind::JobStdout:
  case RuntimeLogViewerKind::JobStderr:
  case RuntimeLogViewerKind::CampaignStdout:
  case RuntimeLogViewerKind::CampaignStderr:
  case RuntimeLogViewerKind::JobTrace:
  case RuntimeLogViewerKind::ArtifactFile:
  case RuntimeLogViewerKind::None:
    break;
  }
  return runtime_log_viewer_kind_label(kind);
}

inline std::string
runtime_structured_viewer_empty_text(RuntimeLogViewerKind kind) {
  switch (kind) {
  case RuntimeLogViewerKind::MarshalEvents:
    return "No marshal events recorded yet.";
  case RuntimeLogViewerKind::MarshalCodexStdout:
    return "No codex stdout entries recorded yet.";
  case RuntimeLogViewerKind::MarshalCodexStderr:
    return "No codex stderr entries recorded yet.";
  case RuntimeLogViewerKind::JobStdout:
  case RuntimeLogViewerKind::JobStderr:
  case RuntimeLogViewerKind::CampaignStdout:
  case RuntimeLogViewerKind::CampaignStderr:
  case RuntimeLogViewerKind::JobTrace:
  case RuntimeLogViewerKind::ArtifactFile:
  case RuntimeLogViewerKind::None:
    break;
  }
  return "No structured log entries recorded yet.";
}

inline RuntimeEventViewerPanel
make_runtime_event_viewer_panel(const CmdState &st) {
  RuntimeEventViewerPanel panel{};
  auto &out = panel.lines;
  const auto &entries = st.runtime.marshal_event_viewer.entries;
  const RuntimeLogViewerKind kind = st.runtime.log_viewer_kind;
  const bool marshal_payload_view = kind == RuntimeLogViewerKind::MarshalEvents;
  const std::string count_label =
      std::to_string(entries.size()) +
      (kind == RuntimeLogViewerKind::MarshalEvents ? " events" : " entries");

  push_runtime_segments(
      out,
      {{.text = runtime_structured_viewer_subject(kind) + " ",
        .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info},
       {.text = count_label,
        .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Accent},
       {.text =
            " | live=" +
            std::string(st.runtime.log_viewer_live_follow ? "follow" : "hold"),
        .emphasis = st.runtime.log_viewer_live_follow
                        ? cuwacunu::iinuji::text_line_emphasis_t::Success
                        : cuwacunu::iinuji::text_line_emphasis_t::Warning}},
      cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_runtime_segments(
      out,
      {{.text = "path ",
        .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info},
       {.text = runtime_path_text(st.runtime.log_viewer_path),
        .emphasis = runtime_path_emphasis(st.runtime.log_viewer_path)}},
      cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_runtime_line(out, {});

  if (entries.empty()) {
    push_runtime_line(out, runtime_structured_viewer_empty_text(kind),
                      cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return panel;
  }

  const std::size_t selected = std::min(
      st.runtime.marshal_event_viewer.selected_entry, entries.size() - 1);

  for (std::size_t i = 0; i < entries.size(); ++i) {
    const auto &entry = entries[i];
    const bool active = (i == selected);
    const std::size_t event_start_line = out.size();
    const std::string header_bg =
        runtime_event_viewer_background(entry.emphasis, false, active);
    const std::string body_bg =
        runtime_event_viewer_background(entry.emphasis, true, active);

    std::vector<cuwacunu::iinuji::styled_text_segment_t> header{};
    header.push_back(
        {.text = std::string(active ? "> " : "  "),
         .emphasis = active ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                            : cuwacunu::iinuji::text_line_emphasis_t::None});
    header.push_back(
        {.text = "[" + std::to_string(i + 1) + "] ",
         .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Debug});
    header.push_back(
        {.text = "[" + entry.event_name + "] ",
         .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None});
    if (entry.timestamp_ms != 0) {
      header.push_back(
          {.text = format_age_since_ms_minutes(entry.timestamp_ms),
           .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info});
      header.push_back(
          {.text = " | " + format_time_marker_ms(entry.timestamp_ms),
           .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Debug});
    }
    push_runtime_segments_with_background(
        out, std::move(header), header_bg,
        cuwacunu::iinuji::text_line_emphasis_t::None);

    if (marshal_payload_view && !entry.payload.empty()) {
      for (const auto &payload : entry.payload) {
        std::vector<cuwacunu::iinuji::styled_text_segment_t> row{};
        row.push_back(
            {.text = "    ",
             .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None});
        row.push_back(
            {.text = payload.first + " = ",
             .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info});
        row.push_back({.text = payload.second,
                       .emphasis = runtime_event_metadata_emphasis(
                           payload.first, payload.second)});
        push_runtime_segments_with_background(
            out, std::move(row), body_bg,
            cuwacunu::iinuji::text_line_emphasis_t::Info);
      }
    } else {
      push_runtime_line_with_background(
          out,
          std::string("    ") +
              (entry.summary.empty() ? std::string("<empty>") : entry.summary),
          body_bg, cuwacunu::iinuji::text_line_emphasis_t::None);
    }

    if (!marshal_payload_view && !entry.metadata.empty()) {
      std::vector<cuwacunu::iinuji::styled_text_segment_t> meta{};
      meta.push_back(
          {.text = "    ",
           .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None});
      for (std::size_t j = 0; j < entry.metadata.size(); ++j) {
        if (j != 0) {
          meta.push_back(
              {.text = " | ",
               .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info});
        }
        meta.push_back(
            {.text = entry.metadata[j].first + "=",
             .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info});
        meta.push_back(
            {.text = entry.metadata[j].second,
             .emphasis = runtime_event_metadata_emphasis(
                 entry.metadata[j].first, entry.metadata[j].second)});
      }
      push_runtime_segments_with_background(
          out, std::move(meta), body_bg,
          cuwacunu::iinuji::text_line_emphasis_t::Info);
    }

    push_runtime_line_with_background(
        out, "", body_bg, cuwacunu::iinuji::text_line_emphasis_t::None);
    if (active) {
      panel.selected_line = event_start_line;
      panel.selected_line_end = out.empty() ? event_start_line : out.size() - 1;
    }
  }
  return panel;
}

inline void append_runtime_marshaled_event_lines(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const std::vector<std::string> &lines) {
  for (const auto &raw_line : lines) {
    const auto entry = runtime_build_marshaled_event_entry(raw_line);
    std::vector<cuwacunu::iinuji::styled_text_segment_t> segments{};
    segments.push_back(
        {.text = "  ",
         .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None});
    if (entry.timestamp_ms != 0) {
      segments.push_back(
          {.text = "[" + format_age_since_ms_minutes(entry.timestamp_ms) + "] ",
           .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Debug});
    }
    segments.push_back(
        {.text = "[" + entry.event_name + "] ",
         .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None});
    segments.push_back(
        {.text = runtime_compact_log_text(entry.summary),
         .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None});
    push_runtime_segments_with_background(
        out, std::move(segments),
        runtime_event_viewer_background(entry.emphasis, true, false),
        cuwacunu::iinuji::text_line_emphasis_t::None);
  }
}

inline void append_runtime_codex_stream_lines(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const std::vector<std::string> &lines, std::string_view fallback_kind) {
  for (const auto &raw_line : lines) {
    const auto entry =
        runtime_build_codex_stream_entry(raw_line, fallback_kind);

    std::vector<cuwacunu::iinuji::styled_text_segment_t> segments{};
    segments.push_back(
        {.text = "  ",
         .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None});
    segments.push_back(
        {.text = "[" + entry.event_name + "] ",
         .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None});
    segments.push_back(
        {.text = runtime_compact_log_text(entry.summary),
         .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None});
    push_runtime_segments_with_background(
        out, std::move(segments),
        runtime_event_viewer_background(entry.emphasis, true, false),
        cuwacunu::iinuji::text_line_emphasis_t::None);
  }
}

inline void append_runtime_session_log_section(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string_view title, const std::string &path, std::size_t tail_lines,
    std::string_view missing_label, std::string_view fallback_kind,
    cuwacunu::iinuji::text_line_emphasis_t section_emphasis,
    bool parse_as_events) {
  append_runtime_detail_section(out, title, section_emphasis);
  append_runtime_detail_meta(out, "path", runtime_path_text(path),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_path_emphasis(path));

  if (path.empty()) {
    append_runtime_detail_text(out,
                               std::string(missing_label) +
                                   " path is not recorded for this session.",
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }

  const auto lines = runtime_tail_text_file_lines(path, tail_lines);
  if (lines.empty()) {
    const bool live = runtime_session_is_active(session);
    append_runtime_detail_text(
        out,
        live ? std::string("waiting for ") + std::string(missing_label) + "..."
             : std::string(missing_label) + " unavailable.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }

  if (parse_as_events) {
    append_runtime_marshaled_event_lines(out, lines);
  } else {
    append_runtime_codex_stream_lines(out, lines, fallback_kind);
  }
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_log_styled_lines(const CmdState &st, RuntimeRowKind primary_kind) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out{};
  if (primary_kind == RuntimeRowKind::Device) {
    append_runtime_selected_device_detail(out, st);
    push_runtime_line(out, {});
    append_runtime_detail_section(out, "Device Logs",
                                  cuwacunu::iinuji::text_line_emphasis_t::Info);
    append_runtime_detail_text(
        out,
        "device lane has no log artifact; stay on manifest for live meters",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return out;
  }
  if (primary_kind == RuntimeRowKind::Session &&
      runtime_selected_none_session(st)) {
    append_runtime_detail_section(
        out, "Orphan Marshal Branch",
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
    append_runtime_detail_text(
        out,
        "This synthetic marshal row has no marshal-owned logs. Orphan "
        "campaign and job logs are shown below when a child is selected.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    push_runtime_line(out, {});
  }
  if (primary_kind == RuntimeRowKind::Session &&
      st.runtime.session.has_value()) {
    append_runtime_session_log_section(
        out, *st.runtime.session, "Marshal Event Timeline",
        st.runtime.session->events_path, 8, "marshal events", "event",
        cuwacunu::iinuji::text_line_emphasis_t::Info, true);
    push_runtime_line(out, {});
    append_runtime_session_log_section(
        out, *st.runtime.session, "Codex Stdout",
        st.runtime.session->codex_stdout_path, 5, "codex stdout", "stdout",
        cuwacunu::iinuji::text_line_emphasis_t::Accent, false);
    push_runtime_line(out, {});
    append_runtime_session_log_section(
        out, *st.runtime.session, "Codex Stderr",
        st.runtime.session->codex_stderr_path, 5, "codex stderr", "stderr",
        cuwacunu::iinuji::text_line_emphasis_t::Warning, false);
    push_runtime_line(out, {});
  }
  if (primary_kind == RuntimeRowKind::Campaign &&
      runtime_selected_none_campaign(st)) {
    append_runtime_detail_section(
        out, "Orphan Campaign Branch",
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
    append_runtime_detail_text(
        out,
        "This synthetic campaign row has no campaign-owned logs. Orphan job "
        "logs are shown below when a child is selected.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    push_runtime_line(out, {});
  }
  if (st.runtime.campaign.has_value()) {
    append_runtime_artifact_section(
        out, "Campaign stdout", st.runtime.campaign->stdout_path,
        "campaign log tailing remains owned by Runtime Hero",
        cuwacunu::iinuji::text_line_emphasis_t::Info);
    push_runtime_line(out, {});
    append_runtime_artifact_section(
        out, "Campaign stderr", st.runtime.campaign->stderr_path,
        "campaign log tailing remains owned by Runtime Hero",
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
    push_runtime_line(out, {});
  }
  if (st.runtime.job.has_value()) {
    append_runtime_excerpt_section(
        out, "Job stdout", st.runtime.job->stdout_path,
        st.runtime.job_stdout_excerpt,
        cuwacunu::iinuji::text_line_emphasis_t::Info);
    push_runtime_line(out, {});
    append_runtime_excerpt_section(
        out, "Job stderr", st.runtime.job->stderr_path,
        st.runtime.job_stderr_excerpt,
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
    return out;
  }
  if (runtime_selected_none_campaign(st) || runtime_selected_none_session(st)) {
    append_runtime_detail_section(out, "Job Logs",
                                  cuwacunu::iinuji::text_line_emphasis_t::Info);
    append_runtime_detail_text(
        out, "Select an orphan job under <none> to inspect stdout and stderr.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return out;
  }
  append_runtime_detail_section(
      out, "Job stderr", cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_runtime_detail_text(out, "job log unavailable",
                             cuwacunu::iinuji::text_line_emphasis_t::Debug);
  return out;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_trace_styled_lines(const CmdState &st,
                                RuntimeRowKind primary_kind) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out{};
  if (primary_kind == RuntimeRowKind::Device) {
    append_runtime_selected_device_detail(out, st);
    push_runtime_line(out, {});
    append_runtime_detail_section(out, "Device Trace",
                                  cuwacunu::iinuji::text_line_emphasis_t::Info);
    append_runtime_detail_text(
        out,
        "device lane has no trace artifact; stay on manifest for live meters",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return out;
  }
  if (primary_kind == RuntimeRowKind::Session &&
      runtime_selected_none_session(st)) {
    append_runtime_detail_section(
        out, "Orphan Marshal Branch",
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
    append_runtime_detail_text(
        out,
        "This synthetic marshal row has no marshal checkpoint trace. Orphan "
        "job traces are shown below when a child is selected.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    push_runtime_line(out, {});
  }
  if (primary_kind == RuntimeRowKind::Session &&
      st.runtime.session.has_value()) {
    append_runtime_detail_section(out, "Marshal Checkpoints",
                                  cuwacunu::iinuji::text_line_emphasis_t::Info);
    append_runtime_detail_meta(
        out, "latest_intent_checkpoint",
        runtime_path_text(st.runtime.session->last_intent_checkpoint_path),
        cuwacunu::iinuji::text_line_emphasis_t::Info,
        runtime_path_emphasis(st.runtime.session->last_intent_checkpoint_path));
    append_runtime_detail_text(
        out, "marshal checkpoint tailing remains owned by Marshal Hero",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    push_runtime_line(out, {});
    append_runtime_detail_meta(
        out, "latest_mutation_checkpoint",
        runtime_path_text(st.runtime.session->last_mutation_checkpoint_path),
        cuwacunu::iinuji::text_line_emphasis_t::Info,
        runtime_path_emphasis(
            st.runtime.session->last_mutation_checkpoint_path));
    append_runtime_detail_text(
        out, "marshal checkpoint tailing remains owned by Marshal Hero",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    push_runtime_line(out, {});
  }
  if (primary_kind == RuntimeRowKind::Campaign &&
      runtime_selected_none_campaign(st)) {
    append_runtime_detail_section(
        out, "Orphan Campaign Branch",
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
    append_runtime_detail_text(
        out,
        "This synthetic campaign row has no campaign trace. Orphan job traces "
        "are shown below when a child is selected.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    push_runtime_line(out, {});
  }
  if (st.runtime.job.has_value()) {
    append_runtime_excerpt_section(
        out, "Job Trace", st.runtime.job->trace_path,
        st.runtime.job_trace_excerpt,
        cuwacunu::iinuji::text_line_emphasis_t::Info);
    return out;
  }
  if (runtime_selected_none_campaign(st) || runtime_selected_none_session(st)) {
    append_runtime_detail_section(out, "Job Trace",
                                  cuwacunu::iinuji::text_line_emphasis_t::Info);
    append_runtime_detail_text(
        out, "Select an orphan job under <none> to inspect trace output.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return out;
  }
  append_runtime_detail_section(out, "Job Trace",
                                cuwacunu::iinuji::text_line_emphasis_t::Info);
  append_runtime_detail_text(out, "job trace unavailable",
                             cuwacunu::iinuji::text_line_emphasis_t::Debug);
  return out;
}

inline bool runtime_show_device_history_panel(const CmdState &st) {
  return !runtime_log_viewer_is_open(st) &&
         runtime_primary_kind(st) == RuntimeRowKind::Device;
}

inline bool runtime_show_secondary_panel(const CmdState &st) {
  return runtime_show_device_history_panel(st);
}

inline std::string runtime_primary_panel_title(const CmdState &st) {
  if (runtime_log_viewer_is_open(st)) {
    if (!runtime_log_viewer_supports_follow(st)) {
      return " " + runtime_log_viewer_title(st) + " [viewer] ";
    }
    return " " + runtime_log_viewer_title(st) +
           (st.runtime.log_viewer_live_follow ? " [viewer][live] "
                                              : " [viewer][hold] ");
  }
  switch (runtime_primary_kind(st)) {
  case RuntimeRowKind::Device:
    return " details ";
  case RuntimeRowKind::Session:
    return " selected marshal ";
  case RuntimeRowKind::Campaign:
    return " selected campaign ";
  case RuntimeRowKind::Job:
    return " selected job ";
  }
  return " runtime detail ";
}

inline std::string runtime_secondary_panel_title(const CmdState &st) {
  if (runtime_show_device_history_panel(st)) {
    return " history ";
  }
  return {};
}

struct runtime_styled_box_metrics_t {
  int text_h{0};
  int total_rows{0};
  int max_scroll_y{0};
  int selected_row{0};
  int selected_row_end{0};
};

inline runtime_styled_box_metrics_t measure_runtime_styled_box(
    const std::vector<cuwacunu::iinuji::styled_text_line_t> &lines, int width,
    int height, bool wrap, std::optional<std::size_t> selected_line,
    std::optional<std::size_t> selected_line_end = std::nullopt) {
  runtime_styled_box_metrics_t out{};
  if (width <= 0 || height <= 0)
    return out;

  int reserve_v = 0;
  int reserve_h = 0;
  int text_w = width;
  int text_h = height;
  int total_rows = 0;
  int max_line_len = 0;

  auto measure_lines = [&](int current_width, int *out_total_rows,
                           int *out_max_line_len,
                           std::vector<int> *out_line_offsets) {
    const int safe_w = std::max(1, current_width);
    int rows = 0;
    int line_len = 0;
    if (out_line_offsets)
      out_line_offsets->assign(lines.size(), 0);
    for (std::size_t i = 0; i < lines.size(); ++i) {
      if (out_line_offsets)
        (*out_line_offsets)[i] = rows;
      if (cuwacunu::iinuji::ansi::has_esc(lines[i].text)) {
        cuwacunu::iinuji::ansi::style_t base{};
        const auto physical_lines = split_lines_keep_empty(lines[i].text);
        if (physical_lines.empty()) {
          ++rows;
          continue;
        }
        for (const auto &physical_line : physical_lines) {
          std::vector<cuwacunu::iinuji::ansi::row_t> wrapped_rows{};
          cuwacunu::iinuji::ansi::hard_wrap(physical_line, safe_w, base, 0,
                                            wrapped_rows);
          if (wrapped_rows.empty()) {
            ++rows;
            continue;
          }
          for (std::size_t j = 0; j < wrapped_rows.size(); ++j) {
            line_len = std::max(line_len, wrapped_rows[j].len);
            ++rows;
            if (!wrap)
              break;
          }
          if (!wrap)
            break;
        }
        continue;
      }
      const auto chunks = wrap ? wrap_text(lines[i].text, safe_w)
                               : split_lines_keep_empty(lines[i].text);
      if (chunks.empty()) {
        ++rows;
        continue;
      }
      for (std::size_t j = 0; j < chunks.size(); ++j) {
        line_len = std::max(line_len, static_cast<int>(chunks[j].size()));
        ++rows;
        if (!wrap)
          break;
      }
    }
    if (out_total_rows)
      *out_total_rows = rows;
    if (out_max_line_len)
      *out_max_line_len = line_len;
  };

  for (int it = 0; it < 3; ++it) {
    text_w = std::max(0, width - reserve_v);
    text_h = std::max(0, height - reserve_h);
    if (text_w <= 0 || text_h <= 0)
      return out;

    measure_lines(text_w, &total_rows, &max_line_len, nullptr);

    const bool need_h = (!wrap && max_line_len > text_w);
    const int reserve_h_new = need_h ? 1 : 0;
    const int text_h_if = std::max(0, height - reserve_h_new);
    const bool need_v = total_rows > text_h_if;
    const int reserve_v_new = need_v ? 1 : 0;
    if (reserve_h_new == reserve_h && reserve_v_new == reserve_v)
      break;
    reserve_h = reserve_h_new;
    reserve_v = reserve_v_new;
  }

  text_w = std::max(0, width - reserve_v);
  text_h = std::max(0, height - reserve_h);
  if (text_w <= 0 || text_h <= 0)
    return out;

  std::vector<int> line_offsets{};
  measure_lines(text_w, &total_rows, &max_line_len, &line_offsets);

  out.text_h = text_h;
  out.total_rows = total_rows;
  out.max_scroll_y = std::max(0, total_rows - text_h);
  if (selected_line.has_value() && *selected_line < line_offsets.size()) {
    out.selected_row = line_offsets[*selected_line];
    std::size_t end_index = selected_line_end.value_or(*selected_line);
    if (end_index < *selected_line)
      end_index = *selected_line;
    if (end_index >= line_offsets.size())
      end_index = line_offsets.size() - 1;
    if (end_index + 1 < line_offsets.size()) {
      out.selected_row_end = std::max(
          out.selected_row, std::max(0, line_offsets[end_index + 1] - 1));
    } else {
      out.selected_row_end = std::max(out.selected_row, total_rows - 1);
    }
  }
  return out;
}

inline const RuntimeDeviceGpuSnapshot *
runtime_find_gpu_in_snapshot(const RuntimeDeviceSnapshot &snapshot,
                             const RuntimeDeviceGpuSnapshot &current_gpu) {
  for (const auto &gpu : snapshot.gpus) {
    if (!current_gpu.uuid.empty() && gpu.uuid == current_gpu.uuid)
      return &gpu;
  }
  for (const auto &gpu : snapshot.gpus) {
    if (!current_gpu.index.empty() && gpu.index == current_gpu.index)
      return &gpu;
  }
  return nullptr;
}

inline std::vector<std::vector<std::pair<double, double>>>
make_runtime_device_history_series(const CmdState &st) {
  std::vector<std::vector<std::pair<double, double>>> series{};
  if (st.runtime.device_history.empty())
    return series;

  const std::size_t selected = runtime_selected_device_index(st).value_or(0);
  if (selected == 0) {
    std::vector<std::pair<double, double>> cpu{};
    std::vector<std::pair<double, double>> ram{};
    for (std::size_t i = 0; i < st.runtime.device_history.size(); ++i) {
      const auto &snapshot = st.runtime.device_history[i];
      const double x = static_cast<double>(i);
      cpu.push_back(
          {x, static_cast<double>(runtime_host_cpu_percent(snapshot))});
      if (snapshot.mem_total_kib != 0) {
        ram.push_back(
            {x, static_cast<double>(runtime_host_memory_percent(snapshot))});
      }
    }
    if (!cpu.empty())
      series.push_back(std::move(cpu));
    if (!ram.empty())
      series.push_back(std::move(ram));
    return series;
  }

  const std::size_t gpu_index = selected - 1;
  if (gpu_index >= st.runtime.device.gpus.size())
    return series;
  const auto &current_gpu = st.runtime.device.gpus[gpu_index];

  std::vector<std::pair<double, double>> core{};
  std::vector<std::pair<double, double>> memory{};
  for (std::size_t i = 0; i < st.runtime.device_history.size(); ++i) {
    const auto &snapshot = st.runtime.device_history[i];
    const auto *gpu = runtime_find_gpu_in_snapshot(snapshot, current_gpu);
    if (gpu == nullptr)
      continue;
    const double x = static_cast<double>(i);
    core.push_back({x, static_cast<double>(std::max(0, gpu->util_gpu_pct))});
    memory.push_back(
        {x, static_cast<double>(runtime_gpu_memory_percent(*gpu))});
  }
  if (!core.empty())
    series.push_back(std::move(core));
  if (!memory.empty())
    series.push_back(std::move(memory));
  return series;
}

inline std::vector<cuwacunu::iinuji::plot_series_cfg_t>
make_runtime_device_history_plot_cfg(const CmdState &st) {
  std::vector<cuwacunu::iinuji::plot_series_cfg_t> cfg{};
  if (runtime_selected_device_index(st).value_or(0) == 0) {
    cfg.push_back(cuwacunu::iinuji::plot_series_cfg_t{
        .color_fg = "#f0d060",
        .mode = cuwacunu::iinuji::plot_mode_t::Line,
    });
    cfg.push_back(cuwacunu::iinuji::plot_series_cfg_t{
        .color_fg = "#67c1ff",
        .mode = cuwacunu::iinuji::plot_mode_t::Line,
    });
    return cfg;
  }
  cfg.push_back(cuwacunu::iinuji::plot_series_cfg_t{
      .color_fg = "#7fe08a",
      .mode = cuwacunu::iinuji::plot_mode_t::Line,
  });
  cfg.push_back(cuwacunu::iinuji::plot_series_cfg_t{
      .color_fg = "#67c1ff",
      .mode = cuwacunu::iinuji::plot_mode_t::Line,
  });
  return cfg;
}

inline cuwacunu::iinuji::plotbox_opts_t
make_runtime_device_history_plot_opts() {
  cuwacunu::iinuji::plotbox_opts_t opts{};
  opts.draw_axes = false;
  opts.draw_grid = true;
  opts.baseline0 = false;
  opts.y_ticks = 4;
  opts.x_ticks = 0;
  opts.y_min = 0.0;
  opts.y_max = 100.0;
  opts.margin_left = 0;
  opts.margin_right = 0;
  opts.margin_top = 0;
  opts.margin_bot = 0;
  return opts;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_primary_styled_lines(const CmdState &st) {
  if (runtime_event_viewer_is_open(st)) {
    return make_runtime_event_viewer_panel(st).lines;
  }
  std::vector<cuwacunu::iinuji::styled_text_line_t> out{};
  const RuntimeRowKind primary_kind = runtime_primary_kind(st);
  std::vector<cuwacunu::iinuji::styled_text_line_t> body{};
  switch (st.runtime.detail_mode) {
  case RuntimeDetailMode::Manifest:
    body = make_runtime_manifest_primary_styled_lines(st, primary_kind);
    break;
  case RuntimeDetailMode::Log:
    body = make_runtime_log_styled_lines(st, primary_kind);
    break;
  case RuntimeDetailMode::Trace:
    body = make_runtime_trace_styled_lines(st, primary_kind);
    break;
  }
  if (body.empty())
    body = make_runtime_manifest_primary_styled_lines(st, primary_kind);
  out.insert(out.end(), body.begin(), body.end());
  return out;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_right_styled_lines(const CmdState &st) {
  return make_runtime_primary_styled_lines(st);
}

inline std::string make_runtime_right(const CmdState &st) {
  return runtime_styled_lines_to_text(make_runtime_right_styled_lines(st));
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
