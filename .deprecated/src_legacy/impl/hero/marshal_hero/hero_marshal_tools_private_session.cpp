[[nodiscard]] std::string runtime_job_to_json(
    const cuwacunu::hero::runtime::runtime_job_record_t &record,
    const cuwacunu::hero::runtime::runtime_job_observation_t &observation) {
  const std::string observed_state =
      cuwacunu::hero::runtime::stable_state_for_observation(record,
                                                            observation);
  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(record.schema) << ","
      << "\"job_cursor\":" << json_quote(record.job_cursor) << ","
      << "\"job_kind\":" << json_quote(record.job_kind) << ","
      << "\"boot_id\":" << json_quote(record.boot_id) << ","
      << "\"state\":" << json_quote(record.state) << ","
      << "\"observed_state\":" << json_quote(observed_state) << ","
      << "\"state_detail\":" << json_quote(record.state_detail) << ","
      << "\"worker_binary\":" << json_quote(record.worker_binary) << ","
      << "\"worker_command\":" << json_quote(record.worker_command) << ","
      << "\"global_config_path\":" << json_quote(record.global_config_path)
      << ","
      << "\"source_campaign_dsl_path\":"
      << json_quote(record.source_campaign_dsl_path) << ","
      << "\"source_contract_dsl_path\":"
      << json_quote(record.source_contract_dsl_path) << ","
      << "\"source_wave_dsl_path\":" << json_quote(record.source_wave_dsl_path)
      << ","
      << "\"campaign_dsl_path\":" << json_quote(record.campaign_dsl_path) << ","
      << "\"contract_dsl_path\":" << json_quote(record.contract_dsl_path) << ","
      << "\"wave_dsl_path\":" << json_quote(record.wave_dsl_path) << ","
      << "\"binding_id\":" << json_quote(record.binding_id) << ","
      << "\"requested_device\":" << json_quote(record.requested_device) << ","
      << "\"resolved_device\":" << json_quote(record.resolved_device) << ","
      << "\"device_source_section\":"
      << json_quote(record.device_source_section) << ","
      << "\"device_contract_hash\":" << json_quote(record.device_contract_hash)
      << ","
      << "\"device_error\":" << json_quote(record.device_error) << ","
      << "\"cuda_required\":" << bool_json(record.cuda_required) << ","
      << "\"reset_runtime_state\":" << bool_json(record.reset_runtime_state)
      << ","
      << "\"stdout_path\":" << json_quote(record.stdout_path) << ","
      << "\"stderr_path\":" << json_quote(record.stderr_path) << ","
      << "\"trace_path\":" << json_quote(record.trace_path) << ","
      << "\"started_at_ms\":" << record.started_at_ms << ","
      << "\"updated_at_ms\":" << record.updated_at_ms << ","
      << "\"finished_at_ms\":"
      << (record.finished_at_ms.has_value()
              ? std::to_string(*record.finished_at_ms)
              : "null")
      << ",\"runner_pid\":"
      << (record.runner_pid.has_value() ? std::to_string(*record.runner_pid)
                                        : "null")
      << ",\"runner_start_ticks\":"
      << (record.runner_start_ticks.has_value()
              ? std::to_string(*record.runner_start_ticks)
              : "null")
      << ",\"target_pid\":"
      << (record.target_pid.has_value() ? std::to_string(*record.target_pid)
                                        : "null")
      << ",\"target_pgid\":"
      << (record.target_pgid.has_value() ? std::to_string(*record.target_pgid)
                                         : "null")
      << ",\"target_start_ticks\":"
      << (record.target_start_ticks.has_value()
              ? std::to_string(*record.target_start_ticks)
              : "null")
      << ",\"exit_code\":"
      << (record.exit_code.has_value() ? std::to_string(*record.exit_code)
                                       : "null")
      << ",\"term_signal\":"
      << (record.term_signal.has_value() ? std::to_string(*record.term_signal)
                                         : "null")
      << ",\"runner_alive\":" << bool_json(observation.runner_alive)
      << ",\"target_alive\":" << bool_json(observation.target_alive) << "}";
  return out.str();
}

[[nodiscard]] std::string runtime_job_summary_to_json(
    const cuwacunu::hero::runtime::runtime_job_record_t &record,
    const cuwacunu::hero::runtime::runtime_job_observation_t &observation) {
  const std::string observed_state =
      cuwacunu::hero::runtime::stable_state_for_observation(record,
                                                            observation);
  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(record.schema) << ","
      << "\"job_cursor\":" << json_quote(record.job_cursor) << ","
      << "\"job_kind\":" << json_quote(record.job_kind) << ","
      << "\"state\":" << json_quote(record.state) << ","
      << "\"observed_state\":" << json_quote(observed_state) << ","
      << "\"state_detail\":" << json_quote(record.state_detail) << ","
      << "\"binding_id\":" << json_quote(record.binding_id) << ","
      << "\"requested_device\":" << json_quote(record.requested_device) << ","
      << "\"resolved_device\":" << json_quote(record.resolved_device) << ","
      << "\"device_error\":" << json_quote(record.device_error) << ","
      << "\"stdout_path\":" << json_quote(record.stdout_path) << ","
      << "\"stderr_path\":" << json_quote(record.stderr_path) << ","
      << "\"trace_path\":" << json_quote(record.trace_path) << ","
      << "\"started_at_ms\":" << record.started_at_ms
      << ",\"updated_at_ms\":" << record.updated_at_ms << ",\"finished_at_ms\":"
      << (record.finished_at_ms.has_value()
              ? std::to_string(*record.finished_at_ms)
              : "null")
      << ",\"exit_code\":"
      << (record.exit_code.has_value() ? std::to_string(*record.exit_code)
                                       : "null")
      << ",\"term_signal\":"
      << (record.term_signal.has_value() ? std::to_string(*record.term_signal)
                                         : "null")
      << ",\"runner_alive\":" << bool_json(observation.runner_alive)
      << ",\"target_alive\":" << bool_json(observation.target_alive) << "}";
  return out.str();
}

[[nodiscard]] std::string runtime_campaign_to_json(
    const cuwacunu::hero::runtime::runtime_campaign_record_t &record) {
  const bool runner_alive =
      record.runner_pid.has_value() &&
      cuwacunu::hero::runtime::process_identity_alive(
          *record.runner_pid, record.runner_start_ticks, record.boot_id);
  std::ostringstream run_bind_ids_json;
  run_bind_ids_json << "[";
  for (std::size_t i = 0; i < record.run_bind_ids.size(); ++i) {
    if (i != 0)
      run_bind_ids_json << ",";
    run_bind_ids_json << json_quote(record.run_bind_ids[i]);
  }
  run_bind_ids_json << "]";

  std::ostringstream job_cursors_json;
  job_cursors_json << "[";
  for (std::size_t i = 0; i < record.job_cursors.size(); ++i) {
    if (i != 0)
      job_cursors_json << ",";
    job_cursors_json << json_quote(record.job_cursors[i]);
  }
  job_cursors_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(record.schema) << ","
      << "\"campaign_cursor\":" << json_quote(record.campaign_cursor) << ","
      << "\"boot_id\":" << json_quote(record.boot_id) << ","
      << "\"state\":" << json_quote(record.state) << ","
      << "\"state_detail\":" << json_quote(record.state_detail) << ","
      << "\"marshal_session_id\":" << json_quote(record.marshal_session_id)
      << ","
      << "\"global_config_path\":" << json_quote(record.global_config_path)
      << ","
      << "\"source_campaign_dsl_path\":"
      << json_quote(record.source_campaign_dsl_path) << ","
      << "\"campaign_dsl_path\":" << json_quote(record.campaign_dsl_path) << ","
      << "\"reset_runtime_state\":" << bool_json(record.reset_runtime_state)
      << ","
      << "\"stdout_path\":" << json_quote(record.stdout_path) << ","
      << "\"stderr_path\":" << json_quote(record.stderr_path) << ","
      << "\"started_at_ms\":" << record.started_at_ms << ","
      << "\"updated_at_ms\":" << record.updated_at_ms << ","
      << "\"finished_at_ms\":"
      << (record.finished_at_ms.has_value()
              ? std::to_string(*record.finished_at_ms)
              : "null")
      << ",\"runner_pid\":"
      << (record.runner_pid.has_value() ? std::to_string(*record.runner_pid)
                                        : "null")
      << ",\"runner_start_ticks\":"
      << (record.runner_start_ticks.has_value()
              ? std::to_string(*record.runner_start_ticks)
              : "null")
      << ",\"current_run_index\":"
      << (record.current_run_index.has_value()
              ? std::to_string(*record.current_run_index)
              : "null")
      << ",\"runner_alive\":" << bool_json(runner_alive)
      << ",\"run_bind_ids\":" << run_bind_ids_json.str()
      << ",\"job_cursors\":" << job_cursors_json.str() << "}";
  return out.str();
}

[[nodiscard]] bool marshal_session_runner_process_alive(
    const cuwacunu::hero::marshal::marshal_session_record_t &record) {
  return record.runner_pid.has_value() &&
         cuwacunu::hero::runtime::process_identity_alive(
             *record.runner_pid, record.runner_start_ticks, record.boot_id);
}

[[nodiscard]] bool file_identity_matches_path(const std::filesystem::path &path,
                                              const struct stat &identity) {
  struct stat path_stat {};
  if (::stat(path.c_str(), &path_stat) != 0)
    return false;
  return path_stat.st_dev == identity.st_dev &&
         path_stat.st_ino == identity.st_ino;
}

[[nodiscard]] bool marshal_session_runner_matches_current_binary(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &record,
    bool *out_matches, std::string *out_warning) {
  if (out_warning)
    out_warning->clear();
  if (!out_matches) {
    if (out_warning)
      *out_warning = "runner-binary match output pointer is null";
    return false;
  }
  *out_matches = true;
  if (!marshal_session_runner_process_alive(record) || !record.runner_pid) {
    return true;
  }

  const std::filesystem::path current_binary =
      resolve_marshal_session_runner_binary(app);
  if (current_binary.empty()) {
    if (out_warning) {
      *out_warning = "could not resolve current marshal runner binary; leaving "
                     "existing runner in place";
    }
    return true;
  }

  const std::filesystem::path runner_exe =
      std::filesystem::path("/proc") /
      std::to_string(static_cast<unsigned long long>(*record.runner_pid)) /
      "exe";
  struct stat runner_stat {};
  if (::stat(runner_exe.c_str(), &runner_stat) != 0) {
    *out_matches = false;
    if (out_warning) {
      *out_warning =
          "marshal session runner executable identity could not be inspected "
          "for pid=" +
          std::to_string(static_cast<unsigned long long>(*record.runner_pid)) +
          "; replacing the runner defensively";
    }
    return true;
  }

  *out_matches = file_identity_matches_path(current_binary, runner_stat);
  if (!*out_matches && out_warning) {
    *out_warning =
        "marshal session runner pid=" +
        std::to_string(static_cast<unsigned long long>(*record.runner_pid)) +
        " is running an older marshal binary; replacing the runner";
  }
  return true;
}

[[nodiscard]] bool marshal_operator_message_has_text(
    const cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t
        &message) {
  return !trim_ascii(message.text).empty();
}

[[nodiscard]] bool marshal_session_has_pending_operator_message(
    const cuwacunu::hero::marshal::marshal_session_record_t &record) {
  return std::any_of(record.pending_operator_messages.begin(),
                     record.pending_operator_messages.end(),
                     [&](const auto &message) {
                       return marshal_operator_message_has_text(message);
                     });
}

[[nodiscard]] bool marshal_session_runner_should_be_attached(
    const cuwacunu::hero::marshal::marshal_session_record_t &record) {
  if (record.lifecycle == "bootstrapping")
    return true;
  if (record.lifecycle != "live")
    return false;
  if (marshal_session_has_pending_operator_message(record))
    return true;
  if (cuwacunu::hero::marshal::marshal_session_has_active_campaign(record))
    return true;
  if (cuwacunu::hero::marshal::marshal_session_work_blocked(record))
    return false;
  if (cuwacunu::hero::marshal::marshal_session_is_review_ready(record))
    return false;
  return trim_ascii(record.finish_reason) == "none";
}

[[nodiscard]] std::string marshal_session_to_json(
    const cuwacunu::hero::marshal::marshal_session_record_t &record) {
  const bool runner_alive = marshal_session_runner_process_alive(record);
  std::ostringstream pending_messages_json;
  pending_messages_json << "[";
  for (std::size_t i = 0; i < record.pending_operator_messages.size(); ++i) {
    if (i != 0)
      pending_messages_json << ",";
    const auto &message = record.pending_operator_messages[i];
    pending_messages_json
        << "{"
        << "\"message_id\":" << json_quote(message.message_id) << ","
        << "\"text\":" << json_quote(message.text) << ","
        << "\"delivery_mode\":" << json_quote(message.delivery_mode) << ","
        << "\"delivery_status\":" << json_quote(message.delivery_status)
        << ",\"thread_id_at_delivery\":"
        << json_quote(message.thread_id_at_delivery) << ","
        << "\"received_at_ms\":"
        << (message.received_at_ms.has_value()
                ? std::to_string(*message.received_at_ms)
                : "null")
        << ",\"delivered_at_ms\":"
        << (message.delivered_at_ms.has_value()
                ? std::to_string(*message.delivered_at_ms)
                : "null")
        << ",\"handled_at_ms\":"
        << (message.handled_at_ms.has_value()
                ? std::to_string(*message.handled_at_ms)
                : "null")
        << ",\"delivery_attempts\":" << message.delivery_attempts << ","
        << "\"last_error\":" << json_quote(message.last_error) << "}";
  }
  pending_messages_json << "]";
  std::ostringstream thread_lineage_json;
  thread_lineage_json << "[";
  for (std::size_t i = 0; i < record.thread_lineage.size(); ++i) {
    if (i != 0)
      thread_lineage_json << ",";
    thread_lineage_json << json_quote(record.thread_lineage[i]);
  }
  thread_lineage_json << "]";
  std::ostringstream campaign_cursors_json;
  campaign_cursors_json << "[";
  for (std::size_t i = 0; i < record.campaign_cursors.size(); ++i) {
    if (i != 0)
      campaign_cursors_json << ",";
    campaign_cursors_json << json_quote(record.campaign_cursors[i]);
  }
  campaign_cursors_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(record.schema) << ","
      << "\"marshal_session_id\":" << json_quote(record.marshal_session_id)
      << ","
      << "\"lifecycle\":" << json_quote(record.lifecycle) << ","
      << "\"status_detail\":" << json_quote(record.status_detail) << ","
      << "\"work_gate\":" << json_quote(record.work_gate) << ","
      << "\"finish_reason\":" << json_quote(record.finish_reason) << ","
      << "\"activity\":" << json_quote(record.activity) << ","
      << "\"objective_name\":" << json_quote(record.objective_name) << ","
      << "\"global_config_path\":" << json_quote(record.global_config_path)
      << ","
      << "\"boot_id\":" << json_quote(record.boot_id) << ","
      << "\"runner_pid\":"
      << (record.runner_pid.has_value() ? std::to_string(*record.runner_pid)
                                        : "null")
      << ",\"runner_start_ticks\":"
      << (record.runner_start_ticks.has_value()
              ? std::to_string(*record.runner_start_ticks)
              : "null")
      << ",\"runner_alive\":" << bool_json(runner_alive) << ","
      << "\"source_marshal_objective_dsl_path\":"
      << json_quote(record.source_marshal_objective_dsl_path) << ","
      << "\"source_campaign_dsl_path\":"
      << json_quote(record.source_campaign_dsl_path) << ","
      << "\"source_marshal_objective_md_path\":"
      << json_quote(record.source_marshal_objective_md_path) << ","
      << "\"source_marshal_guidance_md_path\":"
      << json_quote(record.source_marshal_guidance_md_path) << ","
      << "\"session_root\":" << json_quote(record.session_root) << ","
      << "\"objective_root\":" << json_quote(record.objective_root) << ","
      << "\"campaign_dsl_path\":" << json_quote(record.campaign_dsl_path) << ","
      << "\"marshal_objective_dsl_path\":"
      << json_quote(record.marshal_objective_dsl_path) << ","
      << "\"marshal_objective_md_path\":"
      << json_quote(record.marshal_objective_md_path) << ","
      << "\"marshal_guidance_md_path\":"
      << json_quote(record.marshal_guidance_md_path) << ","
      << "\"hero_marshal_dsl_path\":"
      << json_quote(record.hero_marshal_dsl_path) << ","
      << "\"config_policy_path\":" << json_quote(record.config_policy_path)
      << ","
      << "\"briefing_path\":" << json_quote(record.briefing_path) << ","
      << "\"memory_path\":" << json_quote(record.memory_path) << ","
      << "\"current_thread_id\":" << json_quote(record.current_thread_id) << ","
      << "\"codex_continuity\":" << json_quote(record.codex_continuity) << ","
      << "\"last_resume_error\":" << json_quote(record.last_resume_error) << ","
      << "\"thread_lineage\":" << thread_lineage_json.str() << ","
      << "\"resolved_marshal_codex_binary\":"
      << json_quote(record.resolved_marshal_codex_binary) << ","
      << "\"resolved_marshal_codex_model\":"
      << json_quote(record.resolved_marshal_codex_model) << ","
      << "\"resolved_marshal_codex_reasoning_effort\":"
      << json_quote(record.resolved_marshal_codex_reasoning_effort) << ","
      << "\"human_request_path\":" << json_quote(record.human_request_path)
      << ","
      << "\"codex_stdout_path\":" << json_quote(record.codex_stdout_path) << ","
      << "\"codex_stderr_path\":" << json_quote(record.codex_stderr_path) << ","
      << "\"governance_resolution_path\":"
      << json_quote(
             cuwacunu::hero::marshal::
                 marshal_session_human_governance_resolution_latest_path(
                     std::filesystem::path(record.session_root).parent_path(),
                     record.marshal_session_id)
                     .string())
      << ",\"governance_resolution_sig_path\":"
      << json_quote(
             cuwacunu::hero::marshal::
                 marshal_session_human_governance_resolution_latest_sig_path(
                     std::filesystem::path(record.session_root).parent_path(),
                     record.marshal_session_id)
                     .string())
      << ",\"clarification_answer_path\":"
      << json_quote(
             cuwacunu::hero::marshal::
                 marshal_session_human_clarification_answer_latest_path(
                     std::filesystem::path(record.session_root).parent_path(),
                     record.marshal_session_id)
                     .string())
      << ","
      << "\"events_path\":" << json_quote(record.events_path) << ","
      << "\"turns_path\":" << json_quote(record.turns_path) << ","
      << "\"started_at_ms\":" << record.started_at_ms << ","
      << "\"updated_at_ms\":" << record.updated_at_ms << ","
      << "\"finished_at_ms\":"
      << (record.finished_at_ms.has_value()
              ? std::to_string(*record.finished_at_ms)
              : "null")
      << ",\"checkpoint_count\":" << record.checkpoint_count
      << ",\"launch_count\":" << record.launch_count
      << ",\"max_campaign_launches\":" << record.max_campaign_launches
      << ",\"remaining_campaign_launches\":"
      << record.remaining_campaign_launches
      << ",\"authority_scope\":" << json_quote(record.authority_scope)
      << ",\"campaign_status\":" << json_quote(record.campaign_status) << ","
      << "\"campaign_cursor\":" << json_quote(record.campaign_cursor) << ","
      << "\"next_message_seq\":" << record.next_message_seq << ","
      << "\"pending_operator_messages\":" << pending_messages_json.str() << ","
      << "\"last_event_seq\":" << record.last_event_seq << ","
      << "\"last_codex_action\":" << json_quote(record.last_codex_action) << ","
      << "\"last_warning\":" << json_quote(record.last_warning) << ","
      << "\"last_warning_code\":" << json_quote(record.last_warning_code) << ","
      << "\"last_input_checkpoint_path\":"
      << json_quote(record.last_input_checkpoint_path) << ","
      << "\"last_intent_checkpoint_path\":"
      << json_quote(record.last_intent_checkpoint_path) << ","
      << "\"last_mutation_checkpoint_path\":"
      << json_quote(record.last_mutation_checkpoint_path) << ","
      << "\"campaign_cursors\":" << campaign_cursors_json.str() << "}";
  return out.str();
}

[[nodiscard]] bool append_marshal_session_event(
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string_view producer, std::string_view event,
    const std::string &payload_json, std::string *error, int /*payload_tag*/) {
  if (error)
    error->clear();
  if (!loop) {
    if (error)
      *error = "missing marshal session for event append";
    return false;
  }
  struct event_append_lock_guard_t {
    int fd{-1};

    ~event_append_lock_guard_t() {
      if (fd >= 0) {
        (void)::flock(fd, LOCK_UN);
        (void)::close(fd);
      }
    }
  } lock{};
  const std::filesystem::path lock_path = session_event_lock_path(*loop);
  lock.fd = ::open(lock_path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
  if (lock.fd < 0) {
    if (error)
      *error = "cannot open marshal event lock: " + lock_path.string();
    return false;
  }
  if (::flock(lock.fd, LOCK_EX) != 0) {
    if (error)
      *error = "cannot acquire marshal event lock: " + lock_path.string();
    return false;
  }

  std::uint64_t file_event_seq = 0;
  std::error_code exists_ec{};
  if (std::filesystem::exists(loop->events_path, exists_ec) && !exists_ec) {
    std::string events_text{};
    if (!cuwacunu::hero::runtime::read_text_file(loop->events_path,
                                                 &events_text, error)) {
      return false;
    }
    std::istringstream in{events_text};
    std::string line{};
    while (std::getline(in, line)) {
      const std::string trimmed = trim_ascii(line);
      if (trimmed.empty())
        continue;
      std::uint64_t seq = 0;
      if (extract_json_u64_field(trimmed, "seq", &seq) &&
          seq > file_event_seq) {
        file_event_seq = seq;
      }
    }
  } else if (exists_ec) {
    if (error) {
      *error = "cannot inspect marshal events file: " + loop->events_path;
    }
    return false;
  }

  const std::uint64_t event_seq =
      std::max(loop->last_event_seq, file_event_seq) + 1;
  loop->last_event_seq = event_seq;
  const std::string payload =
      "{\"event_id\":" +
      json_quote(loop->marshal_session_id + ":" + std::to_string(event_seq)) +
      ",\"session_id\":" + json_quote(loop->marshal_session_id) +
      ",\"seq\":" + std::to_string(event_seq) +
      ",\"at_ms\":" + std::to_string(cuwacunu::hero::runtime::now_ms_utc()) +
      ",\"producer\":" + json_quote(std::string(producer)) +
      ",\"type\":" + json_quote(event) + ",\"payload\":" + payload_json + "}\n";
  return cuwacunu::hero::runtime::append_text_file(loop->events_path, payload,
                                                   error);
}

[[nodiscard]] bool append_marshal_session_event(
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string_view producer, std::string_view event, std::string_view detail,
    std::string *error) {
  return append_marshal_session_event(
      loop, producer, event,
      "{\"detail\":" + json_quote(std::string(detail)) + "}", error, 0);
}

[[nodiscard]] bool append_marshal_session_event(
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string_view event, std::string_view detail, std::string *error) {
  return append_marshal_session_event(loop, "marshal", event, detail, error);
}

[[nodiscard]] bool append_marshal_session_event(
    cuwacunu::hero::marshal::marshal_session_record_t **loop,
    std::string_view producer, std::string_view event, std::string_view detail,
    std::string *error) {
  if (!loop)
    return append_marshal_session_event(
        static_cast<cuwacunu::hero::marshal::marshal_session_record_t *>(
            nullptr),
        producer, event, detail, error);
  return append_marshal_session_event(*loop, producer, event, detail, error);
}

[[nodiscard]] bool append_marshal_session_event(
    cuwacunu::hero::marshal::marshal_session_record_t **loop,
    std::string_view event, std::string_view detail, std::string *error) {
  if (!loop)
    return append_marshal_session_event(
        static_cast<cuwacunu::hero::marshal::marshal_session_record_t *>(
            nullptr),
        event, detail, error);
  return append_marshal_session_event(*loop, event, detail, error);
}

[[nodiscard]] bool marshal_session_message_id_exists(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::string_view candidate) {
  const std::string needle =
      "\"message_id\":" + json_quote(std::string(candidate));
  const auto pending_it = std::find_if(
      loop.pending_operator_messages.begin(),
      loop.pending_operator_messages.end(), [&](const auto &message) {
        return trim_ascii(message.message_id) == trim_ascii(candidate);
      });
  if (pending_it != loop.pending_operator_messages.end())
    return true;

  auto file_contains_needle = [&](std::string_view path_text) {
    const std::string path = trim_ascii(path_text);
    if (path.empty())
      return false;
    std::string text{};
    std::string ignored{};
    if (!cuwacunu::hero::runtime::read_text_file(path, &text, &ignored))
      return false;
    return text.find(needle) != std::string::npos;
  };

  if (file_contains_needle(loop.events_path))
    return true;
  if (file_contains_needle(loop.turns_path))
    return true;

  const std::filesystem::path events_path =
      cuwacunu::hero::marshal::marshal_session_events_path(
          app.defaults.marshal_root, loop.marshal_session_id);
  if (events_path.string() != trim_ascii(loop.events_path) &&
      file_contains_needle(events_path.string())) {
    return true;
  }

  const std::filesystem::path turns_path =
      cuwacunu::hero::marshal::marshal_session_turns_path(
          app.defaults.marshal_root, loop.marshal_session_id);
  if (turns_path.string() != trim_ascii(loop.turns_path) &&
      file_contains_needle(turns_path.string())) {
    return true;
  }
  return false;
}

[[nodiscard]] std::string make_operator_message_id(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop) {
  if (!loop)
    return "marshal.message.0";
  for (;;) {
    const std::uint64_t seq = ++loop->next_message_seq;
    const std::string candidate =
        loop->marshal_session_id + ".msg." +
        cuwacunu::hero::runtime::base36_lower_u64(seq);
    if (!marshal_session_message_id_exists(app, *loop, candidate))
      return candidate;
  }
}

[[nodiscard]] std::string marshal_operator_message_to_payload_json(
    const cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t
        &message,
    std::string_view note = {}, std::string_view reply_text = {},
    std::string_view warning = {}) {
  std::ostringstream out;
  std::string detail = trim_ascii(std::string(note));
  if (!trim_ascii(reply_text).empty()) {
    if (!detail.empty())
      detail += " | ";
    detail += "reply=" + trim_ascii(reply_text);
  }
  if (!trim_ascii(warning).empty()) {
    if (!detail.empty())
      detail += " | ";
    detail += "warning=" + trim_ascii(warning);
  }
  out << "{"
      << "\"message_id\":" << json_quote(message.message_id) << ","
      << "\"text\":" << json_quote(message.text) << ","
      << "\"delivery_mode\":" << json_quote(message.delivery_mode) << ","
      << "\"delivery_status\":" << json_quote(message.delivery_status) << ","
      << "\"thread_id_at_delivery\":"
      << json_quote(message.thread_id_at_delivery) << ","
      << "\"received_at_ms\":"
      << (message.received_at_ms.has_value()
              ? std::to_string(*message.received_at_ms)
              : "null")
      << ",\"delivered_at_ms\":"
      << (message.delivered_at_ms.has_value()
              ? std::to_string(*message.delivered_at_ms)
              : "null")
      << ",\"handled_at_ms\":"
      << (message.handled_at_ms.has_value()
              ? std::to_string(*message.handled_at_ms)
              : "null")
      << ",\"delivery_attempts\":" << message.delivery_attempts << ","
      << "\"last_error\":" << json_quote(message.last_error) << ","
      << "\"detail\":" << json_quote(detail) << ","
      << "\"reply_text\":" << json_quote(std::string(reply_text)) << ","
      << "\"warning\":" << json_quote(std::string(warning)) << ","
      << "\"note\":" << json_quote(std::string(note)) << "}";
  return out.str();
}

[[nodiscard]] bool append_operator_message_event(
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string_view producer, std::string_view event,
    const cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t
        &message,
    std::string_view note, std::string_view reply_text,
    std::string_view warning, std::string *error) {
  return append_marshal_session_event(loop, producer, event,
                                      marshal_operator_message_to_payload_json(
                                          message, note, reply_text, warning),
                                      error, 0);
}

[[nodiscard]] bool append_operator_message_turn(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    const cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t
        &message,
    std::string_view reply_text, std::string *error) {
  if (error)
    error->clear();
  if (trim_ascii(loop.turns_path).empty())
    return true;
  const std::uint64_t at_ms =
      message.handled_at_ms.value_or(cuwacunu::hero::runtime::now_ms_utc());
  const std::string turn_id =
      trim_ascii(message.message_id).empty()
          ? loop.marshal_session_id + ".turn." +
                cuwacunu::hero::runtime::base36_lower_u64(at_ms)
          : trim_ascii(message.message_id) + ".turn";
  const std::string payload =
      "{\"turn_id\":" + json_quote(turn_id) +
      ",\"session_id\":" + json_quote(loop.marshal_session_id) +
      ",\"message_id\":" + json_quote(message.message_id) +
      ",\"at_ms\":" + std::to_string(at_ms) +
      ",\"thread_id\":" + json_quote(message.thread_id_at_delivery) +
      ",\"operator_text\":" + json_quote(message.text) +
      ",\"marshal_reply\":" + json_quote(std::string(reply_text)) + "}\n";
  return cuwacunu::hero::runtime::append_text_file(loop.turns_path, payload,
                                                   error);
}

[[nodiscard]] bool
read_marshal_session(const app_context_t &app,
                     std::string_view marshal_session_id,
                     cuwacunu::hero::marshal::marshal_session_record_t *out,
                     std::string *error) {
  if (!cuwacunu::hero::marshal::read_marshal_session_record(
          app.defaults.marshal_root, marshal_session_id, out, error)) {
    return false;
  }
  if (out != nullptr) {
    out->schema = std::string(cuwacunu::hero::marshal::kMarshalSessionSchemaV6);
    cuwacunu::hero::marshal::canonicalize_marshal_session_record(out);
  }
  if (out != nullptr && trim_ascii(out->codex_stdout_path).empty()) {
    out->codex_stdout_path =
        cuwacunu::hero::marshal::marshal_session_codex_stdout_path(
            app.defaults.marshal_root, out->marshal_session_id)
            .string();
  }
  if (out != nullptr && trim_ascii(out->codex_stderr_path).empty()) {
    const std::filesystem::path stderr_path =
        cuwacunu::hero::marshal::marshal_session_codex_stderr_path(
            app.defaults.marshal_root, out->marshal_session_id);
    const std::filesystem::path compat_path =
        cuwacunu::hero::marshal::marshal_session_compat_codex_session_log_path(
            app.defaults.marshal_root, out->marshal_session_id);
    std::error_code ec{};
    if (std::filesystem::exists(stderr_path, ec) && !ec) {
      out->codex_stderr_path = stderr_path.string();
    } else {
      ec.clear();
      if (std::filesystem::exists(compat_path, ec) && !ec) {
        out->codex_stderr_path = compat_path.string();
      } else {
        out->codex_stderr_path = stderr_path.string();
      }
    }
  }
  return true;
}

[[nodiscard]] bool recover_marshal_codex_thread_id_from_logs(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop, bool *out_changed,
    std::string *error) {
  if (error)
    error->clear();
  if (out_changed)
    *out_changed = false;
  if (!loop)
    return true;
  if (!trim_ascii(loop->current_thread_id).empty())
    return true;
  if (loop->lifecycle != "live")
    return true;

  std::string stdout_text{};
  std::string stderr_text{};
  std::string ignored{};
  (void)cuwacunu::hero::runtime::read_text_file(loop->codex_stdout_path,
                                                &stdout_text, &ignored);
  ignored.clear();
  (void)cuwacunu::hero::runtime::read_text_file(loop->codex_stderr_path,
                                                &stderr_text, &ignored);
  const std::string recovered_thread_id =
      extract_current_thread_id_from_codex_logs(stdout_text, stderr_text);
  if (recovered_thread_id.empty())
    return true;

  loop->current_thread_id = recovered_thread_id;
  if (loop->thread_lineage.empty() ||
      loop->thread_lineage.back() != recovered_thread_id) {
    loop->thread_lineage.push_back(recovered_thread_id);
  }
  loop->codex_continuity = "attached";
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  std::string event_error{};
  std::ostringstream payload;
  payload << "{"
          << "\"thread_id\":" << json_quote(recovered_thread_id) << ","
          << "\"detail\":"
          << json_quote("recovered missing Codex thread id from persisted "
                        "Codex stdout/stderr logs")
          << ",\"stdout_log\":" << json_quote(loop->codex_stdout_path)
          << ",\"stderr_log\":" << json_quote(loop->codex_stderr_path) << "}";
  (void)append_marshal_session_event(loop, "codex", "codex.thread_recovered",
                                     payload.str(), &event_error, 0);
  if (!write_marshal_session(app, *loop, error))
    return false;
  if (out_changed)
    *out_changed = true;
  return true;
}

void clear_marshal_session_runner_identity(
    cuwacunu::hero::marshal::marshal_session_record_t *loop);
void clear_marshal_session_campaign_linkage(
    cuwacunu::hero::marshal::marshal_session_record_t *loop);

[[nodiscard]] bool write_marshal_session(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &record,
    std::string *error) {
  cuwacunu::hero::marshal::marshal_session_record_t persisted = record;
  persisted.schema =
      std::string(cuwacunu::hero::marshal::kMarshalSessionSchemaV6);
  cuwacunu::hero::marshal::canonicalize_marshal_session_record(&persisted);
  if (!marshal_session_runner_should_be_attached(persisted)) {
    clear_marshal_session_runner_identity(&persisted);
  }
  auto sync_human_summary_report =
      [&](const cuwacunu::hero::marshal::marshal_session_record_t &loop,
          std::string *out_error) -> bool {
    if (out_error)
      out_error->clear();
    const std::filesystem::path summary_path =
        cuwacunu::hero::marshal::marshal_session_human_summary_path(
            app.defaults.marshal_root, loop.marshal_session_id);
    const std::filesystem::path ack_path =
        cuwacunu::hero::marshal::marshal_session_human_summary_ack_latest_path(
            app.defaults.marshal_root, loop.marshal_session_id);
    const std::filesystem::path ack_sig_path = cuwacunu::hero::marshal::
        marshal_session_human_summary_ack_latest_sig_path(
            app.defaults.marshal_root, loop.marshal_session_id);

    if (!cuwacunu::hero::marshal::is_marshal_session_summary_state(loop)) {
      std::error_code ec{};
      (void)std::filesystem::remove(summary_path, ec);
      (void)std::filesystem::remove(ack_path, ec);
      (void)std::filesystem::remove(ack_sig_path, ec);
      return true;
    }

    std::error_code ec{};
    std::filesystem::create_directories(summary_path.parent_path(), ec);
    if (ec) {
      if (out_error) {
        *out_error = "cannot create human summary dir: " +
                     summary_path.parent_path().string();
      }
      return false;
    }

    const std::string last_campaign_cursor = loop.campaign_cursors.empty()
                                                 ? std::string()
                                                 : loop.campaign_cursors.back();
    const std::optional<std::uint64_t> elapsed_ms =
        marshal_session_elapsed_ms(loop);
    const bool review_ready =
        cuwacunu::hero::marshal::marshal_session_is_review_ready(loop);
    const std::string report_state =
        review_ready ? std::string("review-ready")
                     : (loop.lifecycle == "terminal" ? std::string("terminal")
                                                     : loop.lifecycle);
    const std::string outcome =
        trim_ascii(loop.status_detail).empty()
            ? std::string("No final outcome detail was recorded.")
            : trim_ascii(loop.status_detail);
    const std::string last_action = trim_ascii(loop.last_codex_action).empty()
                                        ? std::string("<unset>")
                                        : trim_ascii(loop.last_codex_action);
    std::ostringstream out;
    out << "# Report of Affairs\n\n"
        << "Objective: "
        << (trim_ascii(loop.objective_name).empty() ? std::string("<unnamed>")
                                                    : loop.objective_name)
        << "\n"
        << "Marshal ID: " << loop.marshal_session_id << "\n"
        << "State: " << report_state << "\n"
        << "Finish Reason: " << loop.finish_reason << "\n\n"
        << "## Outcome\n\n"
        << outcome << "\n\n"
        << "## Effort\n\n"
        << "- Wall Time: " << format_compact_duration_ms(elapsed_ms);
    if (elapsed_ms.has_value())
      out << " (" << *elapsed_ms << " ms)";
    out << "\n"
        << "- Checkpoints: " << loop.checkpoint_count << "\n"
        << "- Campaign Launches: "
        << format_effort_budget(loop.launch_count, loop.max_campaign_launches,
                                loop.remaining_campaign_launches)
        << "\n"
        << "- Last Codex Action: " << last_action << "\n";
    if (!trim_ascii(last_campaign_cursor).empty()) {
      out << "- Last Campaign Cursor: " << last_campaign_cursor << "\n";
    }
    out << "\n## Session State\n\n"
        << "- Lifecycle: " << loop.lifecycle << "\n"
        << "- Work Gate: " << loop.work_gate << "\n"
        << "- Activity: " << loop.activity << "\n"
        << "- Campaign Status: " << loop.campaign_status << "\n"
        << "- Started At Ms: " << loop.started_at_ms << "\n"
        << "- Finished At Ms: ";
    if (loop.finished_at_ms.has_value()) {
      out << *loop.finished_at_ms;
    } else {
      out << "<unset>";
    }
    out << "\n\n";
    if (!trim_ascii(loop.last_warning).empty()) {
      out << "## Warnings\n\n" << loop.last_warning << "\n\n";
    }
    out << "## Operator Next Step\n\n";
    if (review_ready) {
      out << "This report is actionable. Use Message in Human Hero, or "
             "hero.marshal.message_session, when you want to send more "
             "operator guidance after reviewing the outcome. Acknowledge with "
             "a review message "
             "only signs off this report and clears it from the Human "
             "inbox. Archive through Human Hero when you want to release "
             "objective ownership and stop this session from blocking a fresh "
             "launch.\n\n";
    } else {
      out << "This report is informational. Acknowledge it through Human Hero "
             "with a review message when you have reviewed the session "
             "outcome. That action only clears the report from the Human "
             "inbox; it does not reopen the session.\n\n";
    }
    out << "## Supporting Files\n\n"
        << "- This report: " << summary_path.string() << "\n"
        << "- Session memory (long form): " << loop.memory_path << "\n"
        << "- Session manifest: "
        << cuwacunu::hero::marshal::marshal_session_manifest_path(
               app.defaults.marshal_root, loop.marshal_session_id)
               .string()
        << "\n"
        << "- Latest input checkpoint: "
        << cuwacunu::hero::marshal::
               marshal_session_latest_input_checkpoint_path(
                   app.defaults.marshal_root, loop.marshal_session_id)
                   .string()
        << "\n"
        << "- Latest intent checkpoint: "
        << cuwacunu::hero::marshal::
               marshal_session_latest_intent_checkpoint_path(
                   app.defaults.marshal_root, loop.marshal_session_id)
                   .string()
        << "\n"
        << "- Codex stdout: " << loop.codex_stdout_path << "\n"
        << "- Codex stderr: " << loop.codex_stderr_path << "\n"
        << "- Events: " << loop.events_path << "\n";
    if (!trim_ascii(last_campaign_cursor).empty()) {
      out << "- Runtime campaign: " << last_campaign_cursor << "\n";
    }
    out << "- Summary ack target: " << ack_path.string() << "\n";

    return cuwacunu::hero::runtime::write_text_file_atomic(
        summary_path, out.str(), out_error);
  };

  if (!cuwacunu::hero::marshal::write_marshal_session_record(
          app.defaults.marshal_root, persisted, error)) {
    return false;
  }
  std::string summary_error{};
  if (!sync_human_summary_report(persisted, &summary_error)) {
    log_warn("[hero_marshal_mcp] failed to refresh Human Hero "
             "summary artifact: %s",
             summary_error.c_str());
  }
  std::string marker_error{};
  if (!cuwacunu::hero::marshal::sync_human_pending_request_count(
          app.defaults.marshal_root, &marker_error)) {
    log_warn("[hero_marshal_mcp] failed to refresh Human Hero "
             "pending marker: %s",
             marker_error.c_str());
  }
  marker_error.clear();
  if (!cuwacunu::hero::marshal::sync_human_pending_summary_count(
          app.defaults.marshal_root, &marker_error)) {
    log_warn("[hero_marshal_mcp] failed to refresh Human Hero "
             "finished marker: %s",
             marker_error.c_str());
  }
  return true;
}

[[nodiscard]] bool list_marshal_sessions(
    const app_context_t &app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out,
    std::string *error) {
  return cuwacunu::hero::marshal::scan_marshal_session_records(
      app.defaults.marshal_root, out, error);
}

[[nodiscard]] bool read_runtime_campaign_direct(
    const app_context_t &app, std::string_view campaign_cursor,
    cuwacunu::hero::runtime::runtime_campaign_record_t *out,
    std::string *error) {
  return cuwacunu::hero::runtime::read_runtime_campaign_record(
      campaigns_root_for_app(app), campaign_cursor, out, error);
}

[[nodiscard]] bool extract_runtime_campaign_from_tool_structured(
    const std::string &structured_json,
    cuwacunu::hero::runtime::runtime_campaign_record_t *out,
    std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "runtime campaign output pointer is null";
    return false;
  }
  *out = cuwacunu::hero::runtime::runtime_campaign_record_t{};

  std::string campaign_json{};
  if (!extract_json_object_field(structured_json, "campaign", &campaign_json)) {
    if (error)
      *error = "runtime tool structured output is missing campaign object";
    return false;
  }
  std::string schema{};
  if (!extract_json_string_field(campaign_json, "schema", &schema) ||
      trim_ascii(schema) !=
          std::string(cuwacunu::hero::runtime::kRuntimeCampaignSchemaV2)) {
    if (error)
      *error = "runtime tool campaign object has unexpected schema";
    return false;
  }
  if (!extract_json_string_field(campaign_json, "campaign_cursor",
                                 &out->campaign_cursor)) {
    if (error)
      *error = "runtime tool campaign object is missing campaign_cursor";
    return false;
  }
  (void)extract_json_string_field(campaign_json, "state", &out->state);
  (void)extract_json_string_field(campaign_json, "state_detail",
                                  &out->state_detail);
  (void)extract_json_string_field(campaign_json, "marshal_session_id",
                                  &out->marshal_session_id);
  std::uint64_t current_run_index = 0;
  if (extract_json_u64_field(campaign_json, "current_run_index",
                             &current_run_index)) {
    out->current_run_index = current_run_index;
  }
  return true;
}

[[nodiscard]] bool request_runtime_campaign_stop_for_marshal_action(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string_view campaign_cursor, bool force,
    cuwacunu::hero::runtime::runtime_campaign_record_t *out_campaign,
    std::string *out_warning, std::string *error) {
  if (error)
    error->clear();
  if (out_warning)
    out_warning->clear();
  if (!out_campaign) {
    if (error)
      *error = "runtime campaign stop output pointer is null";
    return false;
  }
  *out_campaign = cuwacunu::hero::runtime::runtime_campaign_record_t{};

  marshal_session_workspace_context_t workspace_context{};
  if (!load_marshal_session_workspace_context(app, session, &workspace_context,
                                              error)) {
    return false;
  }
  std::string stop_structured{};
  const std::string stop_args =
      "{\"campaign_cursor\":" + json_quote(trim_ascii(campaign_cursor)) +
      ",\"force\":" + bool_json(force) +
      ",\"suppress_marshal_session_update\":true}";
  if (!call_runtime_tool(app, workspace_context.runtime_hero_binary,
                         "hero.runtime.stop_campaign", stop_args,
                         &stop_structured, error)) {
    return false;
  }
  std::vector<std::string> warnings{};
  append_structured_warnings(stop_structured,
                             "runtime stop warning: ", &warnings);
  for (const std::string &warning : warnings) {
    append_warning_text(out_warning, warning);
  }
  return extract_runtime_campaign_from_tool_structured(stop_structured,
                                                       out_campaign, error);
}

[[nodiscard]] bool
read_runtime_job_direct(const app_context_t &app, std::string_view job_cursor,
                        cuwacunu::hero::runtime::runtime_job_record_t *out,
                        std::string *error) {
  return cuwacunu::hero::runtime::read_runtime_job_record(
      campaigns_root_for_app(app), job_cursor, out, error);
}

[[nodiscard]] std::filesystem::path runtime_job_trace_path_for_record(
    const app_context_t &app,
    const cuwacunu::hero::runtime::runtime_job_record_t &record) {
  if (!record.trace_path.empty())
    return std::filesystem::path(record.trace_path);
  return cuwacunu::hero::runtime::runtime_job_trace_path(
      campaigns_root_for_app(app), record.job_cursor);
}

[[nodiscard]] bool is_runtime_campaign_terminal_state(std::string_view state) {
  return state != "launching" && state != "running" && state != "stopping";
}

[[nodiscard]] bool is_marshal_session_terminal_state(std::string_view state) {
  return cuwacunu::hero::marshal::is_marshal_session_terminal_state(state);
}

[[nodiscard]] std::string observed_campaign_state(
    const cuwacunu::hero::runtime::runtime_campaign_record_t &record) {
  const bool runner_alive =
      record.runner_pid.has_value() &&
      cuwacunu::hero::runtime::process_identity_alive(
          *record.runner_pid, record.runner_start_ticks, record.boot_id);
  if (record.state == "launching" || record.state == "running" ||
      record.state == "stopping") {
    if (runner_alive) {
      return (record.state == "launching") ? "running" : record.state;
    }
    return "failed";
  }
  return record.state;
}

void clear_marshal_session_runner_identity(
    cuwacunu::hero::marshal::marshal_session_record_t *loop) {
  if (!loop)
    return;
  loop->boot_id.clear();
  loop->runner_pid.reset();
  loop->runner_start_ticks.reset();
}

void clear_marshal_session_campaign_linkage(
    cuwacunu::hero::marshal::marshal_session_record_t *loop) {
  if (!loop)
    return;
  loop->campaign_status = "none";
  loop->campaign_cursor.clear();
}

[[nodiscard]] bool session_runner_lock_is_held(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    bool *out_held, std::string *error) {
  if (error)
    error->clear();
  if (!out_held) {
    if (error)
      *error = "session runner lock held output pointer is null";
    return false;
  }
  *out_held = false;
  const std::filesystem::path lock_path = session_runner_lock_path(loop);
  const int fd = ::open(lock_path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
  if (fd < 0) {
    if (error)
      *error = "cannot open session runner lock: " + lock_path.string();
    return false;
  }
  if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
    const int err = errno;
    (void)::close(fd);
    if (err == EWOULDBLOCK || err == EAGAIN) {
      *out_held = true;
      return true;
    }
    if (error)
      *error = "cannot probe session runner lock: " + loop.marshal_session_id;
    return false;
  }
  (void)::flock(fd, LOCK_UN);
  (void)::close(fd);
  return true;
}

[[nodiscard]] bool marshal_session_runner_alive(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    bool *out_alive, std::string *error) {
  if (error)
    error->clear();
  if (!out_alive) {
    if (error)
      *error = "session runner alive output pointer is null";
    return false;
  }
  *out_alive = false;
  if (marshal_session_runner_process_alive(loop)) {
    *out_alive = true;
    return true;
  }
  return session_runner_lock_is_held(loop, out_alive, error);
}

[[nodiscard]] bool capture_process_identity_for_session(
    std::uint64_t pid_value,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string *error) {
  if (error)
    error->clear();
  if (!loop) {
    if (error)
      *error = "marshal session pointer is null";
    return false;
  }
  if (pid_value == 0) {
    if (error)
      *error = "marshal session runner pid is zero";
    return false;
  }
  loop->boot_id = cuwacunu::hero::runtime::current_boot_id();
  loop->runner_pid = pid_value;
  std::uint64_t start_ticks = 0;
  if (cuwacunu::hero::runtime::read_process_start_ticks(pid_value,
                                                        &start_ticks)) {
    loop->runner_start_ticks = start_ticks;
  } else {
    loop->runner_start_ticks.reset();
  }
  return true;
}

[[nodiscard]] bool terminate_marshal_session_runner_process(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::string *out_warning, std::string *error) {
  if (error)
    error->clear();
  if (out_warning)
    out_warning->clear();

  cancel_session_checkpoint_best_effort(loop, true, out_warning);
  if (!loop.runner_pid.has_value() ||
      !marshal_session_runner_process_alive(loop))
    return true;

  const pid_t runner_pid = static_cast<pid_t>(*loop.runner_pid);
  auto wait_until_dead = [&]() {
    constexpr int kPollCount = 20;
    for (int i = 0; i < kPollCount; ++i) {
      if (!marshal_session_runner_process_alive(loop))
        return true;
      ::usleep(50 * 1000);
    }
    return !marshal_session_runner_process_alive(loop);
  };

  if (::kill(runner_pid, SIGTERM) != 0 && errno != ESRCH) {
    if (error) {
      *error = "cannot terminate marshal session runner pid=" +
               std::to_string(static_cast<long long>(runner_pid));
    }
    return false;
  }
  if (wait_until_dead())
    return true;

  if (::kill(runner_pid, SIGKILL) != 0 && errno != ESRCH) {
    if (error) {
      *error = "cannot force-kill marshal session runner pid=" +
               std::to_string(static_cast<long long>(runner_pid));
    }
    return false;
  }
  if (wait_until_dead())
    return true;

  if (error) {
    *error = "marshal session runner pid=" +
             std::to_string(static_cast<long long>(runner_pid)) +
             " did not exit after termination";
  }
  return false;
}

[[nodiscard]] bool park_session_idle_after_interruption(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string_view detail, bool *out_changed, std::string *error) {
  if (error)
    error->clear();
  if (out_changed)
    *out_changed = false;
  if (!loop) {
    if (error)
      *error = "interrupt park session pointer is null";
    return false;
  }
  const std::string normalized_detail = trim_ascii(detail);
  const std::optional<std::uint64_t> finished_at =
      loop->finished_at_ms.has_value()
          ? loop->finished_at_ms
          : std::optional<std::uint64_t>(cuwacunu::hero::runtime::now_ms_utc());
  const bool needs_change =
      !cuwacunu::hero::marshal::marshal_session_is_review_ready(*loop) ||
      loop->status_detail != normalized_detail || loop->work_gate != "open" ||
      loop->finish_reason != "interrupted" ||
      !loop->finished_at_ms.has_value() ||
      trim_ascii(loop->campaign_cursor).size() != 0 ||
      !trim_ascii(loop->boot_id).empty() || loop->runner_pid.has_value() ||
      loop->runner_start_ticks.has_value();
  if (!needs_change)
    return true;

  loop->lifecycle = "live";
  loop->activity = "review";
  loop->status_detail = normalized_detail;
  loop->work_gate = "open";
  loop->finish_reason = "interrupted";
  loop->finished_at_ms = finished_at;
  loop->updated_at_ms = *loop->finished_at_ms;
  clear_marshal_session_campaign_linkage(loop);
  clear_marshal_session_runner_identity(loop);
  if (!write_marshal_session(app, *loop, error))
    return false;
  std::string ignored{};
  (void)append_marshal_session_event(&loop, "session.reconciled",
                                     loop->status_detail, &ignored);
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  (void)write_marshal_session(app, *loop, &ignored);
  if (out_changed)
    *out_changed = true;
  return true;
}

[[nodiscard]] bool reconcile_marshal_session(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop, bool *out_changed,
    std::string *error) {
  if (error)
    error->clear();
  if (out_changed)
    *out_changed = false;
  if (!loop) {
    if (error)
      *error = "marshal session pointer is null";
    return false;
  }

  bool continuity_recovered = false;
  if (!recover_marshal_codex_thread_id_from_logs(
          app, loop, &continuity_recovered, error)) {
    return false;
  }
  if (continuity_recovered && out_changed)
    *out_changed = true;

  bool runner_alive = false;
  if (!marshal_session_runner_alive(*loop, &runner_alive, error))
    return false;
  if (runner_alive)
    return true;

  {
    const std::string retained_campaign_cursor =
        trim_ascii(loop->campaign_cursor);
    const bool retained_work_blocked =
        loop->lifecycle == "live" && loop->work_gate != "open";
    const bool retained_review_ready =
        cuwacunu::hero::marshal::marshal_session_is_review_ready(*loop);
    if (!retained_campaign_cursor.empty() &&
        (retained_work_blocked || retained_review_ready ||
         is_marshal_session_terminal_state(loop->lifecycle))) {
      cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
      if (!read_runtime_campaign_direct(app, retained_campaign_cursor,
                                        &campaign, error)) {
        return false;
      }
      const std::string observed_state = observed_campaign_state(campaign);
      if (is_runtime_campaign_terminal_state(observed_state)) {
        clear_marshal_session_campaign_linkage(loop);
        const std::string detail_lower = lowercase_copy(loop->status_detail);
        if (retained_work_blocked &&
            detail_lower.find("pause_session requested by operator") == 0) {
          loop->status_detail = "pause_session requested by operator";
        } else if (is_marshal_session_terminal_state(loop->lifecycle) &&
                   detail_lower.find(
                       "terminate_session requested by operator") == 0) {
          loop->status_detail = "terminate_session requested by operator";
        }
        loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
        if (!write_marshal_session(app, *loop, error))
          return false;
        if (out_changed)
          *out_changed = true;
      }
    }
  }

  const bool review_ready =
      cuwacunu::hero::marshal::marshal_session_is_review_ready(*loop);
  const bool campaign_active =
      cuwacunu::hero::marshal::marshal_session_has_active_campaign(*loop);
  if (campaign_active) {
    const std::string campaign_cursor = trim_ascii(loop->campaign_cursor);
    if (campaign_cursor.empty()) {
      return park_session_idle_after_interruption(
          app, loop,
          "sudden interruption detected after the marshal runner disappeared "
          "while no active runtime campaign was recorded; session parked for "
          "review "
          "with recovery context so the operator can review runtime evidence "
          "and use message_session to resume",
          out_changed, error);
    }
    cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
    std::string campaign_error{};
    if (!read_runtime_campaign_direct(app, campaign_cursor, &campaign,
                                      &campaign_error)) {
      return park_session_idle_after_interruption(
          app, loop,
          "sudden interruption detected after the marshal runner disappeared "
          "while tracking runtime campaign " +
              campaign_cursor +
              "; runtime campaign manifest could not "
              "be read: " +
              campaign_error +
              "; session parked for review with recovery context so the "
              "operator can inspect runtime evidence and use message_session "
              "to resume",
          out_changed, error);
    }
    const std::string observed_state = observed_campaign_state(campaign);
    std::string observed_detail = trim_ascii(campaign.state_detail);
    if (observed_state != campaign.state && observed_state == "failed") {
      observed_detail = "campaign runner is no longer alive";
    }
    if (observed_detail.empty()) {
      if (observed_state == "failed") {
        observed_detail = "campaign runner is no longer alive";
      } else if (observed_state == "running") {
        observed_detail = "campaign runner alive";
      }
    }
    std::ostringstream detail;
    detail << "sudden interruption detected after runtime campaign "
           << campaign_cursor << " was left in observed state "
           << observed_state;
    if (!observed_detail.empty())
      detail << ": " << observed_detail;
    detail << "; session parked for review with recovery context so the "
              "operator can inspect runtime evidence and use message_session "
              "to resume";
    return park_session_idle_after_interruption(app, loop, detail.str(),
                                                out_changed, error);
  }

  if (loop->lifecycle == "live" && loop->work_gate == "open" && !review_ready) {
    std::ostringstream detail;
    detail << "sudden interruption detected while the marshal planning runner "
              "was active";
    const std::string checkpoint_path =
        trim_ascii(loop->last_input_checkpoint_path);
    if (!checkpoint_path.empty()) {
      detail << " near input checkpoint " << checkpoint_path;
    }
    detail << "; session parked for review with recovery context so the "
              "operator can review the interrupted checkpoint and use "
              "message_session to resume";
    return park_session_idle_after_interruption(app, loop, detail.str(),
                                                out_changed, error);
  }

  return true;
}

[[nodiscard]] bool read_marshal_session_reconciled(
    const app_context_t &app, std::string_view marshal_session_id,
    cuwacunu::hero::marshal::marshal_session_record_t *out, bool *out_changed,
    std::string *error) {
  if (error)
    error->clear();
  if (out_changed)
    *out_changed = false;
  if (!read_marshal_session(app, marshal_session_id, out, error))
    return false;
  return reconcile_marshal_session(app, out, out_changed, error);
}

[[nodiscard]] bool list_marshal_sessions_reconciled(
    const app_context_t &app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out,
    std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "missing destination for marshal session list";
    return false;
  }
  if (!list_marshal_sessions(app, out, error))
    return false;
  for (auto &session : *out) {
    bool ignored_changed = false;
    if (!reconcile_marshal_session(app, &session, &ignored_changed, error))
      return false;
  }
  return true;
}
