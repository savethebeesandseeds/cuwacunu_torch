[[nodiscard]] bool launch_runtime_campaign_for_decision(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    const marshal_checkpoint_decision_t &decision,
    std::string_view state_detail, std::string_view event_name,
    std::string *out_campaign_cursor, std::string *out_campaign_json,
    std::string *out_warning, std::string *error) {
  if (error)
    error->clear();
  if (out_campaign_cursor)
    out_campaign_cursor->clear();
  if (out_campaign_json)
    out_campaign_json->clear();
  if (out_warning)
    out_warning->clear();
  if (!loop) {
    if (error)
      *error = "marshal session pointer is null";
    return false;
  }

  std::string resolved_binding_id{};
  std::string launch_mode_warning{};
  if (decision.launch_mode == "binding") {
    resolved_binding_id = trim_ascii(decision.launch_binding_id);
  } else if (decision.launch_mode == "run_plan") {
    cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
    if (!decode_campaign_snapshot(app, loop->campaign_dsl_path, &instruction,
                                  error)) {
      return false;
    }
    marshal_run_plan_progress_t run_plan_progress{};
    if (!collect_marshal_run_plan_progress(app, *loop, instruction, nullptr,
                                           &run_plan_progress, error)) {
      return false;
    }
    if (!run_plan_progress.ordered_prefix_valid) {
      if (error) {
        *error =
            "launch.mode=run_plan cannot determine the next pending RUN step "
            "because prior successful launches no longer match the current "
            "default RUN sequence";
      }
      return false;
    }
    resolved_binding_id = trim_ascii(run_plan_progress.next_pending_bind_id);
    if (resolved_binding_id.empty()) {
      if (error) {
        *error = "launch.mode=run_plan found no pending RUN steps in the "
                 "current objective-local campaign";
      }
      return false;
    }
    launch_mode_warning = "run_plan launch decomposed to next pending bind: " +
                          resolved_binding_id;
  }

  std::string start_args =
      "{\"campaign_dsl_path\":" + json_quote(loop->campaign_dsl_path);
  start_args +=
      ",\"marshal_session_id\":" + json_quote(loop->marshal_session_id);
  if (!resolved_binding_id.empty()) {
    start_args += ",\"binding_id\":" + json_quote(resolved_binding_id);
  }
  if (decision.launch_reset_runtime_state) {
    start_args += ",\"reset_runtime_state\":true";
  }
  start_args += "}";

  marshal_session_workspace_context_t workspace_context{};
  if (!load_marshal_session_workspace_context(app, *loop, &workspace_context,
                                              error)) {
    return false;
  }
  std::string start_structured{};
  if (!call_runtime_tool(app, workspace_context.runtime_hero_binary,
                         "hero.runtime.start_campaign", start_args,
                         &start_structured, error)) {
    return false;
  }

  std::vector<std::string> launch_warnings{};
  append_structured_warnings(start_structured,
                             "runtime launch warning: ", &launch_warnings);
  std::string runtime_warning{};
  if (!launch_mode_warning.empty()) {
    append_warning_text(&runtime_warning, launch_mode_warning);
  }
  for (const std::string &item : launch_warnings) {
    append_warning_text(&runtime_warning, item);
  }

  std::string campaign_cursor{};
  if (!extract_json_string_field(start_structured, "campaign_cursor",
                                 &campaign_cursor) ||
      campaign_cursor.empty()) {
    if (error)
      *error = "Runtime Hero start_campaign did not return campaign_cursor";
    return false;
  }
  std::string campaign_json{};
  if (!extract_json_object_field(start_structured, "campaign",
                                 &campaign_json)) {
    if (error)
      *error = "Runtime Hero start_campaign did not return campaign object";
    return false;
  }

  loop->lifecycle = "live";
  loop->activity = "awaiting_campaign_fact";
  loop->campaign_status = "running";
  loop->status_detail = std::string(state_detail);
  if (!resolved_binding_id.empty()) {
    append_warning_text(&loop->status_detail,
                        "active bind=" + resolved_binding_id);
  }
  loop->work_gate = "open";
  loop->finish_reason = "none";
  loop->campaign_cursor = campaign_cursor;
  loop->campaign_cursors.push_back(campaign_cursor);
  ++loop->launch_count;
  if (loop->remaining_campaign_launches > 0) {
    --loop->remaining_campaign_launches;
  }
  for (auto &pending_message : loop->pending_operator_messages) {
    if (!marshal_operator_message_has_text(pending_message))
      continue;
    pending_message.delivered_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    pending_message.delivery_status = "handled";
    pending_message.handled_at_ms = pending_message.delivered_at_ms;
    std::string ignored{};
    (void)append_operator_message_event(
        loop, "marshal", "operator.message_delivered", pending_message,
        "consumed before campaign launch", "", "", &ignored);
    (void)append_operator_message_event(
        loop, "marshal", "operator.message_handled", pending_message,
        "consumed before campaign launch", "", "", &ignored);
  }
  loop->pending_operator_messages.clear();
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop->last_warning.clear();
  std::string bookkeeping_error{};
  if (!write_marshal_session(app, *loop, &bookkeeping_error)) {
    append_warning_text(&runtime_warning,
                        "post-launch session manifest degraded: " +
                            bookkeeping_error);
  } else if (!append_marshal_session_event(
                 &loop, "runtime", std::string(event_name), campaign_cursor,
                 &bookkeeping_error)) {
    append_warning_text(&runtime_warning,
                        "post-launch marshal event degraded: " +
                            bookkeeping_error);
  }
  if (!runtime_warning.empty()) {
    persist_marshal_session_warning_best_effort(app, loop, runtime_warning);
    if (out_warning)
      *out_warning = runtime_warning;
  }
  if (out_campaign_cursor)
    *out_campaign_cursor = std::move(campaign_cursor);
  if (out_campaign_json)
    *out_campaign_json = std::move(campaign_json);
  return true;
}

[[nodiscard]] bool run_marshal_checkpoint_with_codex(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::uint64_t checkpoint_index, marshal_checkpoint_decision_t *out_decision,
    std::string *out_warning, std::string *error) {
  if (error)
    error->clear();
  if (out_warning)
    out_warning->clear();
  if (!loop) {
    if (error)
      *error = "marshal session pointer is null";
    return false;
  }
  if (!out_decision) {
    if (error)
      *error = "marshal decision output pointer is null";
    return false;
  }
  scoped_temp_path_t decision_schema_path{};
  if (!write_temp_marshal_decision_schema(marshal_decision_schema_json(),
                                          &decision_schema_path, error)) {
    return false;
  }
  marshal_session_workspace_context_t workspace_context{};
  if (!load_marshal_session_workspace_context(app, *loop, &workspace_context,
                                              error)) {
    return false;
  }
  if (!refresh_marshal_session_hero_marshal_dsl(workspace_context, *loop,
                                                error)) {
    return false;
  }
  if (!refresh_marshal_session_config_policy_dsl(workspace_context, *loop,
                                                 error)) {
    return false;
  }
  bool recovered_thread_id = false;
  if (!recover_marshal_codex_thread_id_from_logs(app, loop,
                                                 &recovered_thread_id, error)) {
    return false;
  }
  (void)recovered_thread_id;

  const std::string prior_thread_id = trim_ascii(loop->current_thread_id);
  const bool had_prior_session = !prior_thread_id.empty();
  const std::filesystem::path resume_prompt_path =
      marshal_session_resume_prompt_path_for(app, *loop, checkpoint_index);
  if (had_prior_session &&
      !write_marshal_resume_prompt(*loop, checkpoint_index, resume_prompt_path,
                                   error)) {
    return false;
  }
  bool full_briefing_ready = false;
  auto ensure_full_briefing = [&]() -> bool {
    if (full_briefing_ready)
      return true;
    if (!rewrite_marshal_session_briefing(app, *loop, error))
      return false;
    full_briefing_ready = true;
    return true;
  };

  const std::filesystem::path decision_path =
      cuwacunu::hero::marshal::marshal_session_intent_checkpoint_path(
          app.defaults.marshal_root, loop->marshal_session_id,
          checkpoint_index);
  const std::string config_args = json_array_from_strings(
      {"--global-config", app.global_config_path.string(), "--config",
       loop->config_policy_path});
  const std::string runtime_args = json_array_from_strings(
      {"--global-config", app.global_config_path.string()});
  const std::string lattice_args = runtime_args;
  const std::string enabled_config_tools = json_array_from_strings(
      {"hero.config.default.list", "hero.config.default.read",
       "hero.config.default.create", "hero.config.default.replace",
       "hero.config.default.delete", "hero.config.temp.list",
       "hero.config.temp.read", "hero.config.temp.create",
       "hero.config.temp.replace", "hero.config.temp.delete",
       "hero.config.objective.list", "hero.config.objective.read",
       "hero.config.objective.create", "hero.config.objective.replace",
       "hero.config.objective.delete"});
  const std::string enabled_runtime_tools = json_array_from_strings(
      {"hero.runtime.get_campaign", "hero.runtime.get_job",
       "hero.runtime.list_jobs", "hero.runtime.tail_log",
       "hero.runtime.tail_trace"});
  const std::string enabled_lattice_tools = json_array_from_strings(
      {"hero.lattice.list_facts", "hero.lattice.get_fact",
       "hero.lattice.list_views", "hero.lattice.get_view"});

  auto append_common_codex_mcp_args = [&](std::vector<std::string> *argv) {
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-config.enabled=true");
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-config.command=" +
                    json_quote(workspace_context.config_hero_binary.string()));
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-config.args=" + config_args);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-config.enabled_tools=" +
                    enabled_config_tools);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-config.startup_timeout_sec=30");

    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-runtime.enabled=true");
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-runtime.command=" +
                    json_quote(workspace_context.runtime_hero_binary.string()));
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-runtime.args=" + runtime_args);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-runtime.enabled_tools=" +
                    enabled_runtime_tools);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-runtime.startup_timeout_sec=30");

    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.enabled=true");
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.command=" +
                    json_quote(workspace_context.lattice_hero_binary.string()));
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.args=" + lattice_args);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.enabled_tools=" +
                    enabled_lattice_tools);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.startup_timeout_sec=30");
  };

  auto load_decision_text = [&](std::string *out_text,
                                std::string *out_err) -> bool {
    if (!out_text) {
      if (out_err)
        *out_err = "decision output pointer is null";
      return false;
    }
    out_text->clear();
    return cuwacunu::hero::runtime::read_text_file(decision_path, out_text,
                                                   out_err);
  };

  auto run_codex_attempt = [&](bool resume_mode, std::string *out_decision_text,
                               std::string *out_session_id,
                               std::string *out_attempt_error) -> bool {
    if (out_attempt_error)
      out_attempt_error->clear();
    if (out_decision_text)
      out_decision_text->clear();
    if (out_session_id)
      out_session_id->clear();

    std::vector<std::string> argv{};
    argv.reserve(64);
    const std::filesystem::path persisted_codex_binary_path =
        resolve_marshal_codex_binary_path(workspace_context.repo_root,
                                          loop->resolved_marshal_codex_binary);
    const std::string persisted_codex_binary =
        trim_ascii(persisted_codex_binary_path.string());
    const std::string persisted_codex_model =
        trim_ascii(loop->resolved_marshal_codex_model);
    const std::string persisted_codex_reasoning_effort =
        trim_ascii(loop->resolved_marshal_codex_reasoning_effort);
    if (persisted_codex_binary.empty()) {
      if (out_attempt_error) {
        *out_attempt_error =
            "marshal session is missing resolved_marshal_codex_binary";
      }
      return false;
    }
    if (!path_is_executable(persisted_codex_binary_path)) {
      if (out_attempt_error) {
        *out_attempt_error =
            "resolved_marshal_codex_binary is not executable: " +
            persisted_codex_binary;
      }
      return false;
    }
    loop->resolved_marshal_codex_binary = persisted_codex_binary;
    if (persisted_codex_model.empty()) {
      if (out_attempt_error) {
        *out_attempt_error =
            "marshal session is missing resolved_marshal_codex_model";
      }
      return false;
    }
    if (persisted_codex_reasoning_effort.empty()) {
      if (out_attempt_error) {
        *out_attempt_error = "marshal session is missing "
                             "resolved_marshal_codex_reasoning_effort";
      }
      return false;
    }
    argv.push_back(persisted_codex_binary);
    argv.push_back("exec");
    argv.push_back("--json");
    if (resume_mode) {
      argv.push_back("resume");
    } else {
      argv.push_back("-s");
      argv.push_back("read-only");
      argv.push_back("--color");
      argv.push_back("never");
    }
    argv.push_back("-m");
    argv.push_back(persisted_codex_model);
    argv.push_back("-c");
    argv.push_back("model_reasoning_effort=" +
                   json_quote(persisted_codex_reasoning_effort));
    append_common_codex_mcp_args(&argv);
    if (!resume_mode) {
      argv.push_back("--output-schema");
      argv.push_back(decision_schema_path.path.string());
    }
    argv.push_back("-o");
    argv.push_back(decision_path.string());
    if (resume_mode) {
      argv.push_back(loop->current_thread_id);
    }
    argv.push_back("-");

    int exit_code = -1;
    std::string invoke_error{};
    loop->codex_stdout_path =
        cuwacunu::hero::marshal::marshal_session_codex_stdout_path(
            app.defaults.marshal_root, loop->marshal_session_id)
            .string();
    loop->codex_stderr_path =
        cuwacunu::hero::marshal::marshal_session_codex_stderr_path(
            app.defaults.marshal_root, loop->marshal_session_id)
            .string();
    const std::filesystem::path stdout_path(loop->codex_stdout_path);
    const std::filesystem::path stderr_jsonl_path(loop->codex_stderr_path);
    const std::filesystem::path stderr_capture_path =
        stderr_jsonl_path.parent_path() /
        (stderr_jsonl_path.filename().string() + ".tmp");
    const std::filesystem::path checkpoint_pid_path =
        marshal_session_checkpoint_pid_path(*loop);
    remove_file_noexcept(stderr_capture_path);
    remove_file_noexcept(marshal_session_compat_codex_session_log_path(*loop));

    const std::filesystem::path stdin_path =
        resume_mode ? resume_prompt_path
                    : std::filesystem::path(loop->briefing_path);
    const bool invoked = run_command_with_stdio_and_timeout(
        argv, stdin_path, stdout_path, stderr_capture_path,
        &workspace_context.repo_root, nullptr,
        workspace_context.marshal_codex_timeout_sec, &checkpoint_pid_path,
        &exit_code, &invoke_error);

    std::string stderr_text{};
    std::string stderr_finalize_error{};
    if (!cuwacunu::hero::runtime::read_text_file(
            stderr_capture_path, &stderr_text, &stderr_finalize_error)) {
      std::error_code ec{};
      if (std::filesystem::exists(stderr_capture_path, ec) && !ec) {
        remove_file_noexcept(stderr_capture_path);
        if (out_attempt_error) {
          *out_attempt_error =
              "codex stderr capture read failed: " + stderr_finalize_error;
        }
        return false;
      }
      stderr_text.clear();
    }
    if (!cuwacunu::hero::runtime::write_text_file_atomic(
            stderr_jsonl_path,
            encode_text_lines_as_jsonl("stderr", stderr_text),
            &stderr_finalize_error)) {
      remove_file_noexcept(stderr_capture_path);
      if (out_attempt_error) {
        *out_attempt_error = "codex stderr capture finalization failed: " +
                             stderr_finalize_error;
      }
      return false;
    }
    remove_file_noexcept(stderr_capture_path);
    if (out_session_id) {
      std::string stdout_text{};
      std::string stdout_read_error{};
      (void)cuwacunu::hero::runtime::read_text_file(stdout_path, &stdout_text,
                                                    &stdout_read_error);
      *out_session_id =
          extract_current_thread_id_from_codex_logs(stdout_text, stderr_text);
    }
    if (!invoked) {
      if (out_attempt_error) {
        *out_attempt_error = resume_mode
                                 ? "codex exec resume failed: " + invoke_error
                                 : "codex exec failed: " + invoke_error;
      }
      return false;
    }
    if (exit_code != 0) {
      if (out_attempt_error) {
        *out_attempt_error =
            std::string(resume_mode ? "codex exec resume failed with exit_code="
                                    : "codex exec failed with exit_code=") +
            std::to_string(exit_code);
      }
      return false;
    }
    if (!load_decision_text(out_decision_text, out_attempt_error))
      return false;
    return true;
  };

  std::string decision_text{};
  std::string parsed_session_id{};
  bool decision_ready = false;
  if (had_prior_session) {
    loop->codex_continuity = "resuming";
    std::string resume_error{};
    std::string resume_decision_text{};
    std::string resumed_session_id{};
    if (run_codex_attempt(true, &resume_decision_text, &resumed_session_id,
                          &resume_error)) {
      if (parse_marshal_checkpoint_decision(resume_decision_text, out_decision,
                                            &resume_error)) {
        decision_text = std::move(resume_decision_text);
        loop->codex_continuity = "attached";
        loop->last_resume_error.clear();
        if (!resumed_session_id.empty()) {
          loop->current_thread_id = resumed_session_id;
          if (loop->thread_lineage.empty() ||
              loop->thread_lineage.back() != resumed_session_id) {
            loop->thread_lineage.push_back(resumed_session_id);
          }
        }
        decision_ready = true;
      } else if (out_warning) {
        loop->codex_continuity = "resume_failed";
        loop->last_resume_error = resume_error;
        (void)append_marshal_session_event(
            loop, "codex", "codex.resume_failed",
            "decision_parse_failed: " + resume_error, nullptr);
        append_warning_text(
            out_warning, build_resume_degraded_warning(*loop, checkpoint_index,
                                                       "decision_parse_failed",
                                                       resume_error));
      }
    } else if (out_warning) {
      loop->codex_continuity = "resume_failed";
      loop->last_resume_error = resume_error;
      (void)append_marshal_session_event(
          loop, "codex", "codex.resume_failed",
          "resume_command_failed: " + resume_error, nullptr);
      append_warning_text(out_warning,
                          build_resume_degraded_warning(*loop, checkpoint_index,
                                                        "resume_command_failed",
                                                        resume_error));
    }
  }

  if (!decision_ready) {
    std::string fresh_error{};
    if (!ensure_full_briefing())
      return false;
    if (!run_codex_attempt(false, &decision_text, &parsed_session_id,
                           &fresh_error)) {
      if (error)
        *error = fresh_error;
      return false;
    }
    if (!parse_marshal_checkpoint_decision(decision_text, out_decision,
                                           error)) {
      return false;
    }
    loop->codex_continuity = had_prior_session ? "restarted" : "attached";
    loop->last_resume_error.clear();
    if (!parsed_session_id.empty()) {
      loop->current_thread_id = parsed_session_id;
      if (loop->thread_lineage.empty() ||
          loop->thread_lineage.back() != parsed_session_id) {
        loop->thread_lineage.push_back(parsed_session_id);
      }
    } else {
      loop->current_thread_id.clear();
      loop->codex_continuity = "unavailable";
      std::string ignored{};
      std::ostringstream payload;
      payload << "{"
              << "\"checkpoint_index\":" << checkpoint_index << ","
              << "\"detail\":"
              << json_quote("Codex checkpoint completed without emitting a "
                            "recoverable thread_id; future checkpoints must "
                            "start fresh until this is repaired")
              << ",\"stdout_log\":" << json_quote(loop->codex_stdout_path)
              << ",\"stderr_log\":" << json_quote(loop->codex_stderr_path)
              << "}";
      (void)append_marshal_session_event(
          loop, "codex", "codex.thread_id_missing", payload.str(), &ignored, 0);
      if (out_warning) {
        append_warning_text(
            out_warning, "fresh codex checkpoint completed without a persisted "
                         "session id; future checkpoints will start fresh");
      }
    }
    if (had_prior_session && !parsed_session_id.empty() &&
        parsed_session_id != prior_thread_id) {
      loop->codex_continuity = "restarted";
    }
  }

  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          marshal_session_latest_intent_checkpoint_path(*loop), decision_text,
          error)) {
    return false;
  }
  return true;
}

[[nodiscard]] bool execute_pending_checkpoint(
    app_context_t *app, cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string *out_warning, std::string *error) {
  if (error)
    error->clear();
  if (out_warning)
    out_warning->clear();
  if (!app || !loop) {
    if (error)
      *error = "pending checkpoint inputs are null";
    return false;
  }

  const std::uint64_t checkpoint_index = loop->checkpoint_count + 1;
  objective_hash_snapshot_t objective_before_snapshot{};
  if (!collect_objective_hash_snapshot(
          std::filesystem::path(loop->objective_root),
          &objective_before_snapshot, error)) {
    return false;
  }
  marshal_checkpoint_decision_t decision{};
  std::string checkpoint_warning{};
  if (!run_marshal_checkpoint_with_codex(*app, loop, checkpoint_index,
                                         &decision, &checkpoint_warning,
                                         error)) {
    cuwacunu::hero::marshal::marshal_session_record_t terminal_loop{};
    if (reload_terminal_marshal_session_if_any(*app, loop->marshal_session_id,
                                               &terminal_loop)) {
      *loop = std::move(terminal_loop);
      if (out_warning) {
        append_warning_text(
            out_warning,
            "planning checkpoint interrupted after session became terminal");
      }
      return true;
    }
    loop->lifecycle = "terminal";
    loop->activity = "quiet";
    loop->campaign_status = "none";
    loop->status_detail = *error;
    loop->work_gate = "open";
    loop->finish_reason = "failed";
    loop->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop->updated_at_ms = *loop->finished_at_ms;
    std::string ignored{};
    (void)write_marshal_session(*app, *loop, &ignored);
    (void)append_marshal_session_event(&loop, "marshal", "checkpoint.failed",
                                       *error, &ignored);
    return false;
  }
  if (!persist_attempted_checkpoint_state(
          *app, loop, checkpoint_index, decision, checkpoint_warning, error)) {
    return false;
  }
  objective_hash_snapshot_t objective_after_snapshot{};
  if (!collect_objective_hash_snapshot(
          std::filesystem::path(loop->objective_root),
          &objective_after_snapshot, error)) {
    const std::string failure_detail = error ? *error : std::string{};
    return park_session_idle_after_checkpoint_failure(
        *app, loop, checkpoint_index, failure_detail, error);
  }
  bool objective_mutation_materialized = false;
  if (!write_mutation_checkpoint_summary(
          *app, loop, checkpoint_index, objective_before_snapshot,
          objective_after_snapshot, &objective_mutation_materialized, error)) {
    const std::string failure_detail = error ? *error : std::string{};
    return park_session_idle_after_checkpoint_failure(
        *app, loop, checkpoint_index, failure_detail, error);
  }
  if (objective_mutation_materialized &&
      trim_ascii(loop->last_mutation_checkpoint_path).empty()) {
    loop->last_mutation_checkpoint_path =
        cuwacunu::hero::marshal::marshal_session_mutation_checkpoint_path(
            app->defaults.marshal_root, loop->marshal_session_id,
            checkpoint_index)
            .string();
  }
  if (!validate_marshal_checkpoint_intent_action(
          *app, *loop, checkpoint_index, decision,
          objective_mutation_materialized, error)) {
    const std::string failure_detail = error ? *error : std::string{};
    return park_session_idle_after_checkpoint_failure(
        *app, loop, checkpoint_index, failure_detail, error);
  }

  loop->checkpoint_count = checkpoint_index;
  loop->last_codex_action = decision.intent;
  loop->last_warning.clear();
  if (!checkpoint_warning.empty()) {
    append_warning_text(&loop->last_warning, checkpoint_warning);
  }
  loop->last_intent_checkpoint_path =
      cuwacunu::hero::marshal::marshal_session_intent_checkpoint_path(
          app->defaults.marshal_root, loop->marshal_session_id,
          checkpoint_index)
          .string();
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop->finished_at_ms.reset();
  std::vector<
      cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t>
      delivered_operator_messages{};
  for (auto &pending_message : loop->pending_operator_messages) {
    if (!marshal_operator_message_has_text(pending_message))
      continue;
    pending_message.delivery_status = "delivered";
    pending_message.delivered_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    pending_message.thread_id_at_delivery = trim_ascii(loop->current_thread_id);
    delivered_operator_messages.push_back(pending_message);
    std::string ignored{};
    (void)append_operator_message_event(
        loop, "marshal", "operator.message_delivered", pending_message,
        "staged into checkpoint input", "", "", &ignored);
  }
  loop->pending_operator_messages.clear();

  auto finalize_delivered_operator_messages =
      [&](std::string_view note, std::string_view reply_text,
          std::string *finalize_error) -> bool {
    if (finalize_error)
      finalize_error->clear();
    if (delivered_operator_messages.empty())
      return true;
    const std::string trimmed_reply = trim_ascii(reply_text);
    if (!trimmed_reply.empty()) {
      std::ostringstream memory_note;
      memory_note << "\n\n## Operator Turn " << checkpoint_index << "\n\n"
                  << "Operator: "
                  << trim_ascii(delivered_operator_messages.front().text)
                  << "\n"
                  << "Marshal: " << trimmed_reply << "\n";
      std::string ignored{};
      (void)cuwacunu::hero::runtime::append_text_file(
          loop->memory_path, memory_note.str(), &ignored);
    }
    for (auto pending_message : delivered_operator_messages) {
      pending_message.delivery_status = "handled";
      pending_message.handled_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      if (!append_operator_message_turn(*loop, pending_message, trimmed_reply,
                                        finalize_error)) {
        return false;
      }
      if (!append_operator_message_event(
              loop, "marshal", "operator.message_handled", pending_message,
              note, trimmed_reply, "", finalize_error)) {
        return false;
      }
    }
    return true;
  };

  if (decision.intent == "reply_only") {
    const std::string action_detail =
        "reply_only: " + trim_ascii(decision.reason);
    if (!append_marshal_session_event(loop, "codex", "codex.action_requested",
                                      action_detail, error)) {
      return false;
    }
    if (cuwacunu::hero::marshal::marshal_session_is_review_ready(*loop)) {
      loop->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      loop->updated_at_ms = *loop->finished_at_ms;
      loop->finish_reason = "success";
    } else {
      loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    }
    if (!write_marshal_session(*app, *loop, error))
      return false;
    return finalize_delivered_operator_messages(
        "reply-only operator turn handled", decision.reply_text, error);
  }

  if (decision.intent == "interrupt_campaign") {
    const std::string campaign_cursor = trim_ascii(loop->campaign_cursor);
    cuwacunu::hero::runtime::runtime_campaign_record_t stopped_campaign{};
    std::string runtime_warning{};
    if (!request_runtime_campaign_stop_for_marshal_action(
            *app, *loop, campaign_cursor, false, &stopped_campaign,
            &runtime_warning, error)) {
      return false;
    }
    if (!append_marshal_session_event(loop, "codex", "codex.action_requested",
                                      "interrupt_campaign", error)) {
      return false;
    }
    loop->lifecycle = "live";
    loop->activity = "awaiting_campaign_fact";
    loop->status_detail =
        trim_ascii(decision.reason).empty()
            ? "interrupt_campaign requested by operator message"
            : trim_ascii(decision.reason);
    loop->work_gate = "open";
    loop->finish_reason = "none";
    loop->campaign_status = "stopping";
    loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    if (!write_marshal_session(*app, *loop, error))
      return false;
    if (!finalize_delivered_operator_messages(
            "interrupt requested; active runtime campaign stop initiated",
            decision.reply_text, error)) {
      return false;
    }
    if (!runtime_warning.empty() && out_warning) {
      append_warning_text(out_warning, runtime_warning);
    }
    return true;
  }

  if (decision.intent == "complete" || decision.intent == "terminate") {
    const std::string action_detail =
        decision.intent + ": " + trim_ascii(decision.reason);
    if (!append_marshal_session_event(loop, "codex", "codex.action_requested",
                                      action_detail, error)) {
      return false;
    }
    loop->lifecycle = (decision.intent == "complete") ? "live" : "terminal";
    loop->activity = (decision.intent == "complete") ? "review" : "quiet";
    loop->status_detail = decision.reason;
    loop->work_gate = "open";
    loop->finish_reason =
        (decision.intent == "complete") ? "success" : "terminated";
    loop->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop->updated_at_ms = *loop->finished_at_ms;
    clear_marshal_session_campaign_linkage(loop);
    if (!write_marshal_session(*app, *loop, error))
      return false;
    if (!finalize_delivered_operator_messages(
            (decision.intent == "complete")
                ? "consumed before review-ready parking"
                : "consumed before terminal finish",
            decision.reply_text, error)) {
      return false;
    }
    const std::string event_name = (decision.intent == "complete")
                                       ? "session.review_ready"
                                       : "session.finished";
    if (!append_marshal_session_event(&loop, "marshal", event_name,
                                      decision.reason, error)) {
      return false;
    }
    return true;
  }

  if (decision.intent == "pause_for_clarification" ||
      decision.intent == "request_governance") {
    const std::string block_detail =
        (decision.intent == "pause_for_clarification")
            ? decision.clarification_request
            : (decision.governance_kind + ": " + decision.governance_request);
    if (!append_marshal_session_event(loop, "codex", "codex.action_requested",
                                      "block_work: " + block_detail, error)) {
      return false;
    }
    loop->lifecycle = "live";
    loop->activity = "quiet";
    loop->status_detail = decision.reason;
    loop->work_gate = (decision.intent == "pause_for_clarification")
                          ? "clarification"
                          : "governance";
    loop->finish_reason = "none";
    clear_marshal_session_campaign_linkage(loop);
    if (!write_marshal_session(*app, *loop, error))
      return false;
    if (!finalize_delivered_operator_messages(
            (decision.intent == "pause_for_clarification")
                ? "consumed before clarification block"
                : "consumed before governance block",
            decision.reply_text, error)) {
      return false;
    }
    std::string request_error{};
    if (!materialize_human_request(*app, *loop, checkpoint_index, decision,
                                   &request_error)) {
      if (error)
        *error = request_error;
      return false;
    }
    if (!append_marshal_session_event(loop, "codex", "work.blocked",
                                      block_detail, error)) {
      return false;
    }
    return true;
  }

  if (decision.intent != "launch_campaign") {
    if (error)
      *error = "unsupported planning intent: " + decision.intent;
    return false;
  }
  if (loop->remaining_campaign_launches == 0) {
    if (!append_marshal_session_event(loop, "codex", "codex.action_requested",
                                      "launch_campaign", error)) {
      return false;
    }
    loop->lifecycle = "terminal";
    loop->activity = "quiet";
    loop->status_detail = "campaign-launch budget exhausted";
    loop->work_gate = "open";
    loop->finish_reason = "exhausted";
    loop->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop->updated_at_ms = *loop->finished_at_ms;
    clear_marshal_session_campaign_linkage(loop);
    if (!write_marshal_session(*app, *loop, error))
      return false;
    if (!finalize_delivered_operator_messages(
            "campaign-launch budget exhausted", decision.reply_text, error)) {
      return false;
    }
    return append_marshal_session_event(&loop, "marshal", "session.finished",
                                        loop->status_detail, error);
  }

  if (!append_marshal_session_event(loop, "codex", "codex.action_requested",
                                    "launch_campaign", error)) {
    return false;
  }

  std::string next_campaign_cursor{};
  std::string ignored_campaign_json{};
  std::string runtime_warning{};
  if (!launch_runtime_campaign_for_decision(
          *app, loop, decision,
          "campaign launched from marshal planning checkpoint",
          "campaign.started", &next_campaign_cursor, &ignored_campaign_json,
          &runtime_warning, error)) {
    loop->lifecycle = "terminal";
    loop->activity = "quiet";
    loop->status_detail = *error;
    loop->work_gate = "open";
    loop->finish_reason = "failed";
    loop->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop->updated_at_ms = *loop->finished_at_ms;
    std::string ignored{};
    (void)write_marshal_session(*app, *loop, &ignored);
    (void)append_marshal_session_event(
        &loop, "runtime", "campaign.start_failed", *error, &ignored);
    return false;
  }
  if (!runtime_warning.empty() && out_warning) {
    append_warning_text(out_warning, runtime_warning);
  }
  if (!finalize_delivered_operator_messages(
          "operator message handled before campaign launch",
          decision.reply_text, error)) {
    return false;
  }
  if (!checkpoint_warning.empty()) {
    persist_marshal_session_warning_best_effort(*app, loop, checkpoint_warning);
    if (out_warning)
      append_warning_text(out_warning, checkpoint_warning);
  }
  return true;
}

[[nodiscard]] bool reopen_session_after_terminal_campaign(
    app_context_t *app, std::string_view campaign_cursor,
    std::string *out_warning, std::string *error) {
  if (error)
    error->clear();
  if (out_warning)
    out_warning->clear();
  if (!app) {
    if (error)
      *error = "marshal app pointer is null";
    return false;
  }

  cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
  if (!read_runtime_campaign_direct(*app, campaign_cursor, &campaign, error)) {
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t loop{};
  const std::string marshal_session_id =
      trim_ascii(campaign.marshal_session_id);
  if (marshal_session_id.empty()) {
    if (error)
      *error = "campaign is not linked to a marshal session";
    return false;
  }
  if (!read_marshal_session(*app, marshal_session_id, &loop, error))
    return false;
  if (is_marshal_session_terminal_state(loop.lifecycle))
    return true;

  const std::uint64_t checkpoint_index = loop.checkpoint_count + 1;
  std::string input_checkpoint_json{};
  if (!build_marshal_postcampaign_input_checkpoint_json(
          *app, loop, campaign, checkpoint_index, &input_checkpoint_json,
          error)) {
    loop.lifecycle = "terminal";
    loop.activity = "quiet";
    loop.status_detail = *error;
    loop.work_gate = "open";
    loop.finish_reason = "failed";
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_marshal_session(*app, loop, &ignored);
    (void)append_marshal_session_event(&loop, "marshal", "checkpoint.failed",
                                       *error, &ignored);
    return false;
  }
  const std::filesystem::path input_checkpoint_path =
      cuwacunu::hero::marshal::marshal_session_input_checkpoint_path(
          app->defaults.marshal_root, loop.marshal_session_id,
          checkpoint_index);
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          input_checkpoint_path, input_checkpoint_json, error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          marshal_session_latest_input_checkpoint_path(loop),
          input_checkpoint_json, error)) {
    return false;
  }
  loop.lifecycle = "live";
  loop.activity = "planning";
  loop.status_detail = "waiting for codex planning checkpoint";
  loop.work_gate = "open";
  loop.finish_reason = "none";
  loop.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  clear_marshal_session_campaign_linkage(&loop);
  loop.last_input_checkpoint_path = input_checkpoint_path.string();
  if (!write_marshal_session(*app, loop, error))
    return false;
  if (!append_marshal_session_event(&loop, "marshal", "checkpoint.staged",
                                    input_checkpoint_path.string(), error)) {
    return false;
  }

  return execute_pending_checkpoint(app, &loop, out_warning, error);
}

struct session_runner_lock_guard_t {
  int fd{-1};

  ~session_runner_lock_guard_t() {
    if (fd >= 0) {
      (void)::flock(fd, LOCK_UN);
      (void)::close(fd);
    }
  }
};

[[nodiscard]] bool acquire_session_runner_lock(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    session_runner_lock_guard_t *out_lock, bool *out_already_locked,
    std::string *error) {
  if (error)
    error->clear();
  if (out_already_locked)
    *out_already_locked = false;
  if (!out_lock) {
    if (error)
      *error = "session runner lock output pointer is null";
    return false;
  }
  out_lock->fd = -1;
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
      if (out_already_locked)
        *out_already_locked = true;
      return true;
    }
    if (error)
      *error = "cannot acquire session runner lock: " + loop.marshal_session_id;
    return false;
  }
  out_lock->fd = fd;
  return true;
}

struct session_runner_launch_message_t {
  std::uint64_t runner_pid{0};
  int exec_errno{0};
};

void write_session_runner_launch_message_noexcept(
    int fd, const session_runner_launch_message_t &message) {
  if (fd < 0)
    return;
  const ssize_t ignored = ::write(fd, &message, sizeof(message));
  (void)ignored;
}

[[nodiscard]] bool marshal_session_events_contain_event(
    const app_context_t &app, std::string_view marshal_session_id,
    std::string_view event_name,
    std::optional<std::uint64_t> min_seq = std::nullopt) {
  std::string events_text{};
  std::string ignored{};
  if (!cuwacunu::hero::runtime::read_text_file(
          cuwacunu::hero::marshal::marshal_session_events_path(
              app.defaults.marshal_root, marshal_session_id),
          &events_text, &ignored)) {
    return false;
  }
  std::istringstream in{events_text};
  std::string line{};
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(line);
    if (trimmed.empty())
      continue;
    std::string type{};
    if (!extract_json_string_field(trimmed, "type", &type) ||
        type != std::string(event_name)) {
      continue;
    }
    if (!min_seq.has_value())
      return true;
    std::uint64_t seq = 0;
    if (extract_json_u64_field(trimmed, "seq", &seq) && seq > *min_seq)
      return true;
  }
  return false;
}

enum class operator_message_wait_outcome_t : std::uint8_t {
  Pending = 0,
  Delivered = 1,
  Failed = 2,
};

[[nodiscard]] operator_message_wait_outcome_t
find_operator_message_wait_outcome(std::string_view events_text,
                                   std::uint64_t after_seq,
                                   std::string_view message_id,
                                   std::string *out_reply,
                                   std::string *out_failure) {
  if (out_reply)
    out_reply->clear();
  if (out_failure)
    out_failure->clear();
  operator_message_wait_outcome_t best_outcome =
      operator_message_wait_outcome_t::Pending;
  std::uint64_t best_seq = 0;
  std::istringstream in{std::string(events_text)};
  std::string line{};
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(line);
    if (trimmed.empty())
      continue;
    std::uint64_t seq = 0;
    std::string type{};
    std::string payload{};
    std::string detail{};
    std::string payload_message_id{};
    std::string delivery_status{};
    std::string reply_text{};
    std::string warning{};
    std::string last_error{};
    if (!extract_json_u64_field(trimmed, "seq", &seq) || seq <= after_seq)
      continue;
    if (!extract_json_string_field(trimmed, "type", &type) ||
        type != "operator.message_handled") {
      continue;
    }
    if (!extract_json_object_field(trimmed, "payload", &payload)) {
      continue;
    }
    (void)extract_json_string_field(payload, "message_id", &payload_message_id);
    if (trim_ascii(payload_message_id) != trim_ascii(message_id)) {
      continue;
    }
    (void)extract_json_string_field(payload, "delivery_status",
                                    &delivery_status);
    (void)extract_json_string_field(payload, "reply_text", &reply_text);
    (void)extract_json_string_field(payload, "warning", &warning);
    (void)extract_json_string_field(payload, "last_error", &last_error);
    (void)extract_json_string_field(payload, "detail", &detail);
    const std::string trimmed_reply = trim_ascii(reply_text);
    if (delivery_status == "handled" && !trimmed_reply.empty() &&
        seq >= best_seq) {
      best_outcome = operator_message_wait_outcome_t::Delivered;
      best_seq = seq;
      if (out_reply) {
        *out_reply = trimmed_reply;
      }
      if (out_failure)
        out_failure->clear();
      continue;
    }
    if (delivery_status == "failed" && seq >= best_seq) {
      best_outcome = operator_message_wait_outcome_t::Failed;
      best_seq = seq;
      if (out_failure) {
        *out_failure = trim_ascii(!last_error.empty() ? last_error
                                  : !warning.empty()  ? warning
                                                      : detail);
      }
      if (out_reply)
        out_reply->clear();
    }
  }
  return best_outcome;
}

[[nodiscard]] operator_message_wait_outcome_t wait_for_operator_message_outcome(
    const app_context_t &app, std::string_view marshal_session_id,
    std::uint64_t after_seq, std::string_view message_id,
    std::string *out_reply, std::string *out_failure) {
  if (out_reply)
    out_reply->clear();
  if (out_failure)
    out_failure->clear();
  constexpr auto kWait = std::chrono::milliseconds(2500);
  constexpr useconds_t kPollUs = 50 * 1000;
  const auto deadline = std::chrono::steady_clock::now() + kWait;
  while (std::chrono::steady_clock::now() < deadline) {
    std::string events_text{};
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(
        cuwacunu::hero::marshal::marshal_session_events_path(
            app.defaults.marshal_root, marshal_session_id),
        &events_text, &ignored);
    const operator_message_wait_outcome_t outcome =
        find_operator_message_wait_outcome(events_text, after_seq, message_id,
                                           out_reply, out_failure);
    if (outcome != operator_message_wait_outcome_t::Pending)
      return outcome;
    ::usleep(kPollUs);
  }
  return operator_message_wait_outcome_t::Pending;
}

[[nodiscard]] std::string extract_last_nonempty_line(std::string_view text) {
  std::istringstream in{std::string(text)};
  std::string line{};
  std::string last{};
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(line);
    if (!trimmed.empty()) {
      last = trimmed;
    }
  }
  return last;
}

[[nodiscard]] bool process_pending_live_operator_message(
    app_context_t *app, cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string *out_warning, std::string *error) {
  if (error)
    error->clear();
  if (out_warning)
    out_warning->clear();
  if (!app || !loop) {
    if (error)
      *error = "live operator message inputs are null";
    return false;
  }

  auto it = std::find_if(loop->pending_operator_messages.begin(),
                         loop->pending_operator_messages.end(),
                         [&](const auto &message) {
                           return marshal_operator_message_has_text(message) &&
                                  message.delivery_status == "received";
                         });
  if (it == loop->pending_operator_messages.end())
    return true;

  auto &pending_message = *it;
  pending_message.delivery_attempts += 1;
  pending_message.delivery_mode =
      trim_ascii(loop->current_thread_id).empty() ? "checkpoint" : "live";
  pending_message.thread_id_at_delivery = trim_ascii(loop->current_thread_id);

  const std::string message_id = pending_message.message_id;
  const auto original_message = pending_message;
  const auto fail_pending_message = [&](std::string_view note,
                                        std::string_view failure_text) -> bool {
    std::string failed_text = trim_ascii(failure_text);
    if (failed_text.empty()) {
      failed_text = "operator message delivery failed";
    }
    auto failed_it = std::find_if(
        loop->pending_operator_messages.begin(),
        loop->pending_operator_messages.end(), [&](const auto &message) {
          return trim_ascii(message.message_id) == trim_ascii(message_id);
        });
    cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t
        failed_message = original_message;
    if (failed_it != loop->pending_operator_messages.end()) {
      failed_message = *failed_it;
      loop->pending_operator_messages.erase(failed_it);
    }
    failed_message.handled_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    failed_message.delivery_status = "failed";
    failed_message.last_error = failed_text;
    failed_message.thread_id_at_delivery = trim_ascii(loop->current_thread_id);
    loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    append_warning_text(&loop->last_warning, failed_text);
    if (!write_marshal_session(*app, *loop, error))
      return false;
    std::string ignored{};
    (void)append_marshal_session_event(loop, "marshal", "warning.emitted",
                                       failed_text, &ignored);
    (void)append_operator_message_event(
        loop, "marshal", "operator.message_handled", failed_message, note, "",
        failed_text, &ignored);
    persist_marshal_session_warning_best_effort(*app, loop, failed_text);
    if (out_warning)
      *out_warning = failed_text;
    return true;
  };

  const std::uint64_t checkpoint_index = loop->checkpoint_count + 1;
  const std::filesystem::path prior_input_checkpoint_path =
      marshal_session_latest_input_checkpoint_path(*loop);
  std::string input_checkpoint_json{};
  if (!build_marshal_message_input_checkpoint_json(
          *app, *loop, checkpoint_index, prior_input_checkpoint_path,
          pending_message, &input_checkpoint_json, error)) {
    return fail_pending_message("operator message checkpoint build failed",
                                error ? *error : std::string{});
  }

  const std::filesystem::path input_checkpoint_path =
      cuwacunu::hero::marshal::marshal_session_input_checkpoint_path(
          app->defaults.marshal_root, loop->marshal_session_id,
          checkpoint_index);
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          input_checkpoint_path, input_checkpoint_json, error)) {
    return fail_pending_message("operator message checkpoint write failed",
                                error ? *error : std::string{});
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          marshal_session_latest_input_checkpoint_path(*loop),
          input_checkpoint_json, error)) {
    return fail_pending_message(
        "operator message latest checkpoint write failed",
        error ? *error : std::string{});
  }

  loop->last_input_checkpoint_path = input_checkpoint_path.string();
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!write_marshal_session(*app, *loop, error))
    return false;
  if (!append_marshal_session_event(loop, "marshal", "checkpoint.staged",
                                    input_checkpoint_path.string(), error)) {
    return false;
  }
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!write_marshal_session(*app, *loop, error))
    return false;

  std::string checkpoint_warning{};
  std::string checkpoint_error{};
  if (!execute_pending_checkpoint(app, loop, &checkpoint_warning,
                                  &checkpoint_error)) {
    return fail_pending_message("operator message checkpoint failed",
                                checkpoint_error);
  }
  if (!checkpoint_warning.empty()) {
    persist_marshal_session_warning_best_effort(*app, loop, checkpoint_warning);
    if (out_warning)
      *out_warning = checkpoint_warning;
  }
  return true;
}

[[nodiscard]] std::string
marshal_session_runner_startup_detail(const app_context_t &app,
                                      std::string_view marshal_session_id) {
  const std::filesystem::path stderr_path =
      marshal_session_runner_stderr_path(app, marshal_session_id);
  const std::filesystem::path stdout_path =
      marshal_session_runner_stdout_path(app, marshal_session_id);
  std::string stderr_text{};
  std::string stdout_text{};
  std::string ignored{};
  (void)cuwacunu::hero::runtime::read_text_file(stderr_path, &stderr_text,
                                                &ignored);
  (void)cuwacunu::hero::runtime::read_text_file(stdout_path, &stdout_text,
                                                &ignored);

  std::string detail = "session runner exited before becoming active";
  std::string excerpt = extract_last_nonempty_line(stderr_text);
  if (excerpt.empty()) {
    excerpt = extract_last_nonempty_line(stdout_text);
  }
  if (!excerpt.empty()) {
    detail.append(": ");
    detail.append(excerpt);
  }
  detail.append("; runner_stderr=");
  detail.append(stderr_path.string());
  return detail;
}

[[nodiscard]] bool wait_for_session_runner_startup(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *session,
    bool have_runner_identity, std::uint64_t after_seq, std::string *error) {
  if (error)
    error->clear();
  if (!session) {
    if (error)
      *error = "marshal session runner startup session pointer is null";
    return false;
  }

  constexpr auto kStartupWait = std::chrono::milliseconds(2000);
  constexpr useconds_t kStartupPollUs = 50 * 1000;
  const auto deadline = std::chrono::steady_clock::now() + kStartupWait;
  while (std::chrono::steady_clock::now() < deadline) {
    if (marshal_session_events_contain_event(app, session->marshal_session_id,
                                             "session.runner_active",
                                             after_seq)) {
      return true;
    }
    if (have_runner_identity) {
      bool runner_alive = true;
      std::string runner_alive_error{};
      if (marshal_session_runner_alive(*session, &runner_alive,
                                       &runner_alive_error) &&
          !runner_alive) {
        std::string detail = marshal_session_runner_startup_detail(
            app, session->marshal_session_id);
        if (!runner_alive_error.empty()) {
          detail.append("; runner_probe=");
          detail.append(runner_alive_error);
        }
        session->lifecycle = "terminal";
        session->activity = "quiet";
        session->status_detail = detail;
        session->work_gate = "open";
        session->finish_reason = "failed";
        session->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
        session->updated_at_ms = *session->finished_at_ms;
        clear_marshal_session_campaign_linkage(session);
        clear_marshal_session_runner_identity(session);

        std::string persist_error{};
        if (!write_marshal_session(app, *session, &persist_error)) {
          if (error) {
            *error =
                detail +
                "; additionally failed to persist runner startup failure: " +
                persist_error;
          }
          return false;
        }
        std::string ignored{};
        (void)append_marshal_session_event(
            &session, "marshal", "session.runner_failed", detail, &ignored);
        if (error)
          *error = detail;
        return false;
      }
    }
    ::usleep(kStartupPollUs);
  }
  return true;
}

[[nodiscard]] bool launch_session_runner_process(
    const app_context_t &app, std::string_view marshal_session_id,
    std::uint64_t *out_runner_pid, std::string *error) {
  if (error)
    error->clear();
  if (out_runner_pid)
    *out_runner_pid = 0;

  const std::filesystem::path runner_stdout_path =
      marshal_session_runner_stdout_path(app, marshal_session_id);
  const std::filesystem::path runner_stderr_path =
      marshal_session_runner_stderr_path(app, marshal_session_id);
  const std::filesystem::path runner_binary =
      resolve_marshal_session_runner_binary(app);
  if (runner_binary.empty()) {
    if (error) {
      *error = "cannot resolve marshal session runner binary";
    }
    return false;
  }
  std::error_code dir_ec{};
  std::filesystem::create_directories(runner_stdout_path.parent_path(), dir_ec);
  if (dir_ec) {
    if (error) {
      *error = "cannot create marshal session runner logs dir: " +
               runner_stdout_path.parent_path().string();
    }
    return false;
  }

  int pipe_fds[2]{-1, -1};
#ifdef O_CLOEXEC
  if (::pipe2(pipe_fds, O_CLOEXEC) != 0) {
    if (error)
      *error = "pipe2 failed";
    return false;
  }
#else
  if (::pipe(pipe_fds) != 0) {
    if (error)
      *error = "pipe failed";
    return false;
  }
  (void)::fcntl(pipe_fds[0], F_SETFD, FD_CLOEXEC);
  (void)::fcntl(pipe_fds[1], F_SETFD, FD_CLOEXEC);
#endif

  const pid_t child = ::fork();
  if (child < 0) {
    const std::string msg = "fork failed";
    (void)::close(pipe_fds[0]);
    (void)::close(pipe_fds[1]);
    if (error)
      *error = msg;
    return false;
  }
  if (child == 0) {
    (void)::close(pipe_fds[0]);
    if (::setsid() < 0) {
      session_runner_launch_message_t message{};
      message.exec_errno = errno;
      write_session_runner_launch_message_noexcept(pipe_fds[1], message);
      _exit(127);
    }

    const pid_t grandchild = ::fork();
    if (grandchild < 0) {
      session_runner_launch_message_t message{};
      message.exec_errno = errno;
      write_session_runner_launch_message_noexcept(pipe_fds[1], message);
      _exit(127);
    }
    if (grandchild > 0)
      _exit(0);

    const int devnull = ::open("/dev/null", O_RDONLY);
    if (devnull >= 0) {
      (void)::dup2(devnull, STDIN_FILENO);
      if (devnull > STDERR_FILENO)
        (void)::close(devnull);
    }

    const int stdout_fd =
        ::open(runner_stdout_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stdout_fd < 0) {
      session_runner_launch_message_t message{};
      message.exec_errno = errno;
      write_session_runner_launch_message_noexcept(pipe_fds[1], message);
      _exit(127);
    }
    (void)::dup2(stdout_fd, STDOUT_FILENO);
    if (stdout_fd > STDERR_FILENO)
      (void)::close(stdout_fd);

    const int stderr_fd =
        ::open(runner_stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stderr_fd < 0) {
      session_runner_launch_message_t message{};
      message.exec_errno = errno;
      write_session_runner_launch_message_noexcept(pipe_fds[1], message);
      _exit(127);
    }
    (void)::dup2(stderr_fd, STDERR_FILENO);
    if (stderr_fd > STDERR_FILENO)
      (void)::close(stderr_fd);

    std::vector<std::string> args{};
    args.push_back(runner_binary.string());
    args.push_back("--session-runner");
    args.push_back("--marshal-session-id");
    args.push_back(std::string(marshal_session_id));
    args.push_back("--global-config");
    args.push_back(app.global_config_path.string());
    args.push_back("--hero-config");
    args.push_back(app.hero_config_path.string());

    std::vector<char *> argv{};
    argv.reserve(args.size() + 1);
    for (auto &arg : args)
      argv.push_back(arg.data());
    argv.push_back(nullptr);
    session_runner_launch_message_t message{};
    message.runner_pid = static_cast<std::uint64_t>(::getpid());
    write_session_runner_launch_message_noexcept(pipe_fds[1], message);
    ::execv(argv[0], argv.data());

    message.exec_errno = errno;
    write_session_runner_launch_message_noexcept(pipe_fds[1], message);
    _exit(127);
  }

  (void)::close(pipe_fds[1]);
  int exec_errno = 0;
  std::uint64_t launched_runner_pid = 0;
  session_runner_launch_message_t message{};
  for (;;) {
    const ssize_t n = ::read(pipe_fds[0], &message, sizeof(message));
    if (n == 0)
      break;
    if (n < 0) {
      if (errno == EINTR)
        continue;
      exec_errno = errno;
      break;
    }
    if (n != static_cast<ssize_t>(sizeof(message)))
      continue;
    if (message.runner_pid != 0)
      launched_runner_pid = message.runner_pid;
    if (message.exec_errno != 0)
      exec_errno = message.exec_errno;
  }
  (void)::close(pipe_fds[0]);
  int ignored_status = 0;
  while (::waitpid(child, &ignored_status, 0) < 0 && errno == EINTR) {
  }
  if (exec_errno != 0) {
    if (error) {
      *error = "cannot exec detached marshal session runner: errno=" +
               std::to_string(exec_errno);
    }
    return false;
  }
  if (out_runner_pid)
    *out_runner_pid = launched_runner_pid;
  if (launched_runner_pid == 0) {
    if (error)
      *error = "detached marshal session runner launch did not report a pid";
    return false;
  }
  return true;
}

