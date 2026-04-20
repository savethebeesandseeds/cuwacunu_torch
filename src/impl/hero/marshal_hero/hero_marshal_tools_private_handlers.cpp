[[nodiscard]] bool handle_tool_start_session(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error);
[[nodiscard]] bool handle_tool_list_sessions(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error);
[[nodiscard]] bool handle_tool_get_session(app_context_t *app,
                                           const std::string &arguments_json,
                                           std::string *out_structured,
                                           std::string *out_error);
[[nodiscard]] bool handle_tool_reconcile_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error);
[[nodiscard]] bool handle_tool_pause_session(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error);
[[nodiscard]] bool handle_tool_resume_session(app_context_t *app,
                                              const std::string &arguments_json,
                                              std::string *out_structured,
                                              std::string *out_error);
[[nodiscard]] bool handle_tool_message_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error);
[[nodiscard]] bool handle_tool_archive_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error);
[[nodiscard]] bool handle_tool_terminate_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error);

#define HERO_MARSHAL_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER)       \
  {NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
constexpr marshal_tool_descriptor_t kMarshalTools[] = {
#include "hero/marshal_hero/hero_marshal_tools.def"
};
#undef HERO_MARSHAL_TOOL

[[nodiscard]] const marshal_tool_descriptor_t *
find_marshal_tool_descriptor(std::string_view tool_name) {
  for (const auto &tool : kMarshalTools) {
    if (tool.name == tool_name)
      return &tool;
  }
  return nullptr;
}

[[nodiscard]] bool handle_tool_start_session(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_objective_dsl_path{};
  (void)extract_json_string_field(arguments_json, "marshal_objective_dsl_path",
                                  &marshal_objective_dsl_path);
  marshal_objective_dsl_path = trim_ascii(marshal_objective_dsl_path);
  marshal_start_session_overrides_t start_overrides{};
  if (!parse_start_session_overrides(arguments_json, &start_overrides,
                                     out_error)) {
    return false;
  }

  const std::filesystem::path source_marshal_objective_dsl_path =
      resolve_requested_marshal_objective_source_path(
          *app, marshal_objective_dsl_path, out_error);
  if (source_marshal_objective_dsl_path.empty())
    return false;

  cuwacunu::hero::marshal::marshal_session_record_t loop{};
  if (!initialize_marshal_session_record(*app,
                                         source_marshal_objective_dsl_path,
                                         start_overrides, &loop, out_error)) {
    return false;
  }

  std::vector<std::string> warnings{};
  const std::uint64_t checkpoint_index = 1;
  std::string input_checkpoint_json{};
  if (!build_marshal_bootstrap_input_checkpoint_json(
          *app, loop, checkpoint_index, &input_checkpoint_json, out_error)) {
    loop.lifecycle = "terminal";
    loop.activity = "quiet";
    loop.status_detail = *out_error;
    loop.work_gate = "open";
    loop.finish_reason = "failed";
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_marshal_session(*app, loop, &ignored);
    (void)append_marshal_session_event(&loop, "marshal", "checkpoint.failed",
                                       *out_error, &ignored);
    return false;
  }
  const std::filesystem::path input_checkpoint_path =
      cuwacunu::hero::marshal::marshal_session_input_checkpoint_path(
          app->defaults.marshal_root, loop.marshal_session_id,
          checkpoint_index);
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          input_checkpoint_path, input_checkpoint_json, out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          marshal_session_latest_input_checkpoint_path(loop),
          input_checkpoint_json, out_error)) {
    return false;
  }
  loop.lifecycle = "live";
  loop.activity = "planning";
  loop.status_detail = "waiting for first codex planning checkpoint";
  loop.work_gate = "open";
  loop.finish_reason = "none";
  loop.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop.last_input_checkpoint_path = input_checkpoint_path.string();
  clear_marshal_session_runner_identity(&loop);
  if (!write_marshal_session(*app, loop, out_error))
    return false;
  if (!append_marshal_session_event(&loop, "marshal", "checkpoint.staged",
                                    input_checkpoint_path.string(),
                                    out_error)) {
    return false;
  }
  loop.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!write_marshal_session(*app, loop, out_error))
    return false;

  std::string bookkeeping_error{};
  std::uint64_t runner_pid = 0;
  const std::uint64_t startup_wait_seq = loop.last_event_seq;
  if (!launch_session_runner_process(*app, loop.marshal_session_id, &runner_pid,
                                     &bookkeeping_error)) {
    warnings.push_back("marshal session runner launch failed: " +
                       bookkeeping_error);
    loop.lifecycle = "terminal";
    loop.activity = "quiet";
    loop.status_detail = "session runner launch failed: " + bookkeeping_error;
    loop.work_gate = "open";
    loop.finish_reason = "failed";
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_marshal_session(*app, loop, &ignored);
    (void)append_marshal_session_event(
        &loop, "marshal", "session.runner_failed", bookkeeping_error, &ignored);
    *out_error = bookkeeping_error;
    return false;
  }
  bool runner_identity_captured = false;
  std::string identity_error{};
  if (!capture_process_identity_for_session(runner_pid, &loop,
                                            &identity_error)) {
    warnings.push_back("marshal session runner identity degraded: " +
                       identity_error);
  } else {
    runner_identity_captured = true;
    loop.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    std::string ignored{};
    (void)write_marshal_session(*app, loop, &ignored);
  }
  if (!wait_for_session_runner_startup(*app, &loop, runner_identity_captured,
                                       startup_wait_seq, out_error)) {
    return false;
  }
  if (!warnings.empty()) {
    std::string combined_warning{};
    for (const std::string &warning : warnings) {
      append_warning_text(&combined_warning, warning);
    }
    persist_marshal_session_warning_best_effort(*app, &loop, combined_warning);
  }

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(loop.marshal_session_id) +
      ",\"session\":" + marshal_session_to_json(loop) +
      ",\"campaign_cursor\":\"\",\"campaign\":null" +
      ",\"warnings\":" + json_array_from_strings(warnings) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_list_sessions(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string lifecycle_filter{};
  std::size_t limit = 0;
  std::size_t offset = 0;
  bool newest_first = true;
  (void)extract_json_string_field(arguments_json, "lifecycle",
                                  &lifecycle_filter);
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);
  (void)extract_json_bool_field(arguments_json, "newest_first", &newest_first);
  lifecycle_filter = trim_ascii(lifecycle_filter);

  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> sessions{};
  if (!list_marshal_sessions_reconciled(*app, &sessions, out_error))
    return false;
  if (!lifecycle_filter.empty()) {
    sessions.erase(std::remove_if(sessions.begin(), sessions.end(),
                                  [&](const auto &session) {
                                    return session.lifecycle !=
                                           lifecycle_filter;
                                  }),
                   sessions.end());
  }
  std::sort(sessions.begin(), sessions.end(),
            [newest_first](const auto &a, const auto &b) {
              if (a.started_at_ms != b.started_at_ms) {
                return newest_first ? (a.started_at_ms > b.started_at_ms)
                                    : (a.started_at_ms < b.started_at_ms);
              }
              return newest_first
                         ? (a.marshal_session_id > b.marshal_session_id)
                         : (a.marshal_session_id < b.marshal_session_id);
            });

  const std::size_t total = sessions.size();
  const std::size_t off = std::min(offset, sessions.size());
  std::size_t count = limit;
  if (count == 0)
    count = sessions.size() - off;
  count = std::min(count, sessions.size() - off);

  std::ostringstream sessions_json;
  sessions_json << "[";
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0)
      sessions_json << ",";
    sessions_json << marshal_session_to_json(sessions[off + i]);
  }
  sessions_json << "]";

  *out_structured =
      "{\"marshal_session_id\":\"\",\"count\":" + std::to_string(count) +
      ",\"total\":" + std::to_string(total) +
      ",\"lifecycle\":" + json_quote(lifecycle_filter) +
      ",\"sessions\":" + sessions_json.str() + "}";
  return true;
}

[[nodiscard]] bool handle_tool_get_session(app_context_t *app,
                                           const std::string &arguments_json,
                                           std::string *out_structured,
                                           std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "get_session requires arguments.marshal_session_id";
    return false;
  }
  cuwacunu::hero::marshal::marshal_session_record_t session{};
  bool reconciled = false;
  if (!read_marshal_session_reconciled(*app, marshal_session_id, &session,
                                       &reconciled, out_error))
    return false;
  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"reconciled\":" + bool_json(reconciled) +
      ",\"session\":" + marshal_session_to_json(session) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_reconcile_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "reconcile_session requires arguments.marshal_session_id";
    return false;
  }
  cuwacunu::hero::marshal::marshal_session_record_t session{};
  bool reconciled = false;
  if (!read_marshal_session_reconciled(*app, marshal_session_id, &session,
                                       &reconciled, out_error)) {
    return false;
  }
  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"reconciled\":" + bool_json(reconciled) +
      ",\"session\":" + marshal_session_to_json(session) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_pause_session(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  bool force = false;
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  (void)extract_json_bool_field(arguments_json, "force", &force);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "pause_session requires arguments.marshal_session_id";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  bool reconciled = false;
  if (!read_marshal_session_reconciled(*app, marshal_session_id, &session,
                                       &reconciled, out_error))
    return false;
  (void)reconciled;
  if (is_marshal_session_terminal_state(session.lifecycle)) {
    *out_structured =
        "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
        ",\"session\":" + marshal_session_to_json(session) +
        ",\"warning\":\"\"}";
    return true;
  }
  if (session.lifecycle == "live" && session.work_gate != "open") {
    const std::string warning =
        "session is already paused with work_gate=" + session.work_gate;
    *out_structured =
        "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
        ",\"session\":" + marshal_session_to_json(session) +
        ",\"warning\":" + json_quote(warning) + "}";
    return true;
  }
  if (cuwacunu::hero::marshal::marshal_session_is_review_ready(session)) {
    const std::string warning =
        "session is already review-ready; use message_session to resume or "
        "terminate_session to close it";
    *out_structured =
        "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
        ",\"session\":" + marshal_session_to_json(session) +
        ",\"warning\":" + json_quote(warning) + "}";
    return true;
  }

  const std::string campaign_cursor = trim_ascii(session.campaign_cursor);
  std::string runtime_warning{};
  bool active_campaign_stop_pending = false;
  cancel_session_checkpoint_best_effort(session, force, &runtime_warning);
  if (!campaign_cursor.empty()) {
    cuwacunu::hero::runtime::runtime_campaign_record_t stopped_campaign{};
    if (!request_runtime_campaign_stop_for_marshal_action(
            *app, session, campaign_cursor, force, &stopped_campaign,
            &runtime_warning, out_error)) {
      return false;
    }
    active_campaign_stop_pending =
        !is_runtime_campaign_terminal_state(trim_ascii(stopped_campaign.state));
  }

  session.lifecycle = "live";
  session.activity = "quiet";
  session.status_detail =
      active_campaign_stop_pending
          ? "pause_session requested by operator; active runtime campaign stop "
            "in progress"
          : "pause_session requested by operator";
  session.work_gate = "operator_pause";
  session.finish_reason = "none";
  session.campaign_status = active_campaign_stop_pending ? "stopping" : "none";
  session.finished_at_ms.reset();
  session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!active_campaign_stop_pending) {
    clear_marshal_session_campaign_linkage(&session);
  }
  clear_marshal_session_runner_identity(&session);
  if (!write_marshal_session(*app, session, out_error))
    return false;
  if (!append_marshal_session_event(&session, "operator", "work.blocked",
                                    session.status_detail, out_error)) {
    return false;
  }
  if (!runtime_warning.empty()) {
    persist_marshal_session_warning_best_effort(*app, &session,
                                                runtime_warning);
  }

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"session\":" + marshal_session_to_json(session) +
      ",\"warning\":" + json_quote(runtime_warning) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_resume_session(app_context_t *app,
                                              const std::string &arguments_json,
                                              std::string *out_structured,
                                              std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  std::string response_path_arg{};
  std::string response_sig_path_arg{};
  std::string clarification_answer_path_arg{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  (void)extract_json_string_field(arguments_json, "governance_resolution_path",
                                  &response_path_arg);
  (void)extract_json_string_field(
      arguments_json, "governance_resolution_sig_path", &response_sig_path_arg);
  (void)extract_json_string_field(arguments_json, "clarification_answer_path",
                                  &clarification_answer_path_arg);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "resume_session requires arguments.marshal_session_id";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  bool reconciled = false;
  if (!read_marshal_session_reconciled(*app, marshal_session_id, &session,
                                       &reconciled, out_error))
    return false;
  (void)reconciled;
  if (is_marshal_session_terminal_state(session.lifecycle)) {
    *out_structured =
        "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
        ",\"session\":" + marshal_session_to_json(session) +
        ",\"warnings\":[]}";
    return true;
  }

  const bool has_governance_resolution =
      !trim_ascii(response_path_arg).empty() ||
      !trim_ascii(response_sig_path_arg).empty();
  const bool has_clarification_answer =
      !trim_ascii(clarification_answer_path_arg).empty();
  if (has_governance_resolution && has_clarification_answer) {
    *out_error = "resume_session cannot accept both governance resolution and "
                 "clarification answer artifacts";
    return false;
  }

  auto stage_followup_and_launch =
      [&](std::string input_checkpoint_json, std::string_view status_detail,
          std::string_view event_detail,
          std::vector<std::string> *warnings) -> bool {
    const std::uint64_t checkpoint_index = session.checkpoint_count + 1;
    const std::filesystem::path input_checkpoint_path =
        cuwacunu::hero::marshal::marshal_session_input_checkpoint_path(
            app->defaults.marshal_root, session.marshal_session_id,
            checkpoint_index);
    if (!cuwacunu::hero::runtime::write_text_file_atomic(
            input_checkpoint_path, input_checkpoint_json, out_error)) {
      return false;
    }
    if (!cuwacunu::hero::runtime::write_text_file_atomic(
            marshal_session_latest_input_checkpoint_path(session),
            input_checkpoint_json, out_error)) {
      return false;
    }
    session.lifecycle = "live";
    session.activity = "planning";
    session.status_detail = std::string(status_detail);
    session.work_gate = "open";
    session.finish_reason = "none";
    session.finished_at_ms.reset();
    session.last_input_checkpoint_path = input_checkpoint_path.string();
    for (auto &pending_message : session.pending_operator_messages) {
      if (!marshal_operator_message_has_text(pending_message))
        continue;
      pending_message.delivered_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      pending_message.delivery_status = "delivered";
      pending_message.handled_at_ms.reset();
      pending_message.thread_id_at_delivery =
          trim_ascii(session.current_thread_id);
      std::string ignored{};
      (void)append_operator_message_event(
          &session, "marshal", "operator.message_delivered", pending_message,
          "staged into resumed checkpoint", "", "", &ignored);
    }
    session.pending_operator_messages.clear();
    session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    clear_marshal_session_runner_identity(&session);
    if (!write_marshal_session(*app, session, out_error))
      return false;
    if (!append_marshal_session_event(&session, "marshal", "checkpoint.staged",
                                      std::string(event_detail), out_error)) {
      return false;
    }
    session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    if (!write_marshal_session(*app, session, out_error))
      return false;
    std::string runner_error{};
    std::uint64_t runner_pid = 0;
    const std::uint64_t startup_wait_seq = session.last_event_seq;
    if (!launch_session_runner_process(*app, session.marshal_session_id,
                                       &runner_pid, &runner_error)) {
      if (warnings) {
        warnings->push_back("marshal session runner launch failed: " +
                            runner_error);
      }
      session.lifecycle = "terminal";
      session.activity = "quiet";
      session.status_detail = "session runner launch failed: " + runner_error;
      session.work_gate = "open";
      session.finish_reason = "failed";
      session.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      session.updated_at_ms = *session.finished_at_ms;
      std::string ignored{};
      (void)write_marshal_session(*app, session, &ignored);
      (void)append_marshal_session_event(
          &session, "marshal", "session.runner_failed", runner_error, &ignored);
      *out_error = runner_error;
      return false;
    }
    bool runner_identity_captured = false;
    std::string identity_error{};
    if (!capture_process_identity_for_session(runner_pid, &session,
                                              &identity_error)) {
      if (warnings) {
        warnings->push_back("marshal session runner identity degraded: " +
                            identity_error);
      }
    } else {
      runner_identity_captured = true;
      session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      std::string ignored{};
      (void)write_marshal_session(*app, session, &ignored);
    }
    if (!wait_for_session_runner_startup(*app, &session,
                                         runner_identity_captured,
                                         startup_wait_seq, out_error)) {
      return false;
    }
    return true;
  };

  std::vector<std::string> warnings{};
  if (has_governance_resolution) {
    if (session.lifecycle != "live" || session.work_gate != "governance") {
      *out_error = "resume_session governance path requires "
                   "session.lifecycle=live and work_gate=governance";
      return false;
    }
    const std::filesystem::path response_path =
        trim_ascii(response_path_arg).empty()
            ? cuwacunu::hero::marshal::
                  marshal_session_human_governance_resolution_latest_path(
                      app->defaults.marshal_root, session.marshal_session_id)
            : std::filesystem::path(resolve_path_from_base_folder(
                  app->global_config_path.parent_path().string(),
                  trim_ascii(response_path_arg)));
    const std::filesystem::path response_sig_path =
        trim_ascii(response_sig_path_arg).empty()
            ? cuwacunu::hero::marshal::
                  marshal_session_human_governance_resolution_latest_sig_path(
                      app->defaults.marshal_root, session.marshal_session_id)
            : std::filesystem::path(resolve_path_from_base_folder(
                  app->global_config_path.parent_path().string(),
                  trim_ascii(response_sig_path_arg)));

    cuwacunu::hero::human::human_resolution_record_t response{};
    std::string signature_hex{};
    if (!load_verified_human_resolution(*app, session, response_path,
                                        response_sig_path, &response,
                                        &signature_hex, out_error)) {
      return false;
    }
    append_human_resolution_note(session, response, response_path,
                                 response_sig_path);
    session.last_warning.clear();
    session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();

    std::string event_detail = response_path.string();
    if (!response.operator_id.empty()) {
      event_detail.append(" by ");
      event_detail.append(response.operator_id);
    }
    if (!append_marshal_session_event(&session, "governance", "work.unblocked",
                                      event_detail, out_error)) {
      return false;
    }

    if (response.resolution_kind == "grant") {
      if (response.governance_kind == "authority_expansion" &&
          response.grant_allow_default_write) {
        session.authority_scope = "objective_plus_defaults";
      } else if (response.governance_kind == "launch_budget_expansion") {
        session.max_campaign_launches +=
            response.grant_additional_campaign_launches;
        session.remaining_campaign_launches +=
            response.grant_additional_campaign_launches;
      }
    } else if (response.resolution_kind == "terminate") {
      session.lifecycle = "terminal";
      session.activity = "quiet";
      session.status_detail =
          "session terminated by governance resolution: " + response.reason;
      session.work_gate = "open";
      session.finish_reason = "terminated";
      session.last_codex_action = "terminate";
      session.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      session.updated_at_ms = *session.finished_at_ms;
      clear_marshal_session_campaign_linkage(&session);
      clear_marshal_session_runner_identity(&session);
      if (!write_marshal_session(*app, session, out_error))
        return false;
      if (!append_marshal_session_event(&session, "governance",
                                        "session.finished",
                                        session.status_detail, out_error)) {
        return false;
      }
      *out_structured =
          "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
          ",\"session\":" + marshal_session_to_json(session) +
          ",\"warnings\":[]}";
      return true;
    }

    const std::uint64_t checkpoint_index = session.checkpoint_count + 1;
    const std::filesystem::path prior_input_checkpoint_path =
        cuwacunu::hero::marshal::marshal_session_latest_input_checkpoint_path(
            app->defaults.marshal_root, session.marshal_session_id);
    std::string input_checkpoint_json{};
    if (!build_marshal_governance_followup_input_checkpoint_json(
            *app, session, checkpoint_index, prior_input_checkpoint_path,
            response_path, response_sig_path, response, &input_checkpoint_json,
            out_error)) {
      return false;
    }
    if (!stage_followup_and_launch(
            std::move(input_checkpoint_json),
            "waiting for codex planning checkpoint after governance resolution",
            prior_input_checkpoint_path.string(), &warnings)) {
      return false;
    }
  } else if (has_clarification_answer) {
    if (session.lifecycle != "live" || session.work_gate != "clarification") {
      *out_error = "resume_session clarification path requires "
                   "session.lifecycle=live and work_gate=clarification";
      return false;
    }
    const std::filesystem::path answer_path =
        std::filesystem::path(resolve_path_from_base_folder(
            app->global_config_path.parent_path().string(),
            trim_ascii(clarification_answer_path_arg)));
    std::string answer_json{};
    if (!cuwacunu::hero::runtime::read_text_file(answer_path, &answer_json,
                                                 out_error)) {
      return false;
    }
    std::string answer{};
    if (!parse_human_clarification_answer_json(
            answer_json, session.marshal_session_id, session.checkpoint_count,
            &answer, out_error)) {
      return false;
    }
    const std::uint64_t checkpoint_index = session.checkpoint_count + 1;
    const std::filesystem::path prior_input_checkpoint_path =
        cuwacunu::hero::marshal::marshal_session_latest_input_checkpoint_path(
            app->defaults.marshal_root, session.marshal_session_id);
    std::string input_checkpoint_json{};
    if (!build_marshal_clarification_followup_checkpoint_json(
            *app, session, checkpoint_index, prior_input_checkpoint_path,
            answer_path, answer, &input_checkpoint_json, out_error)) {
      return false;
    }
    append_memory_note(session, checkpoint_index,
                       "Human clarification answer: " + answer);
    if (!append_marshal_session_event(&session, "operator", "work.unblocked",
                                      answer, out_error)) {
      return false;
    }
    if (!stage_followup_and_launch(
            std::move(input_checkpoint_json),
            "waiting for codex planning checkpoint after human clarification",
            prior_input_checkpoint_path.string(), &warnings)) {
      return false;
    }
  } else {
    if (session.lifecycle != "live" || session.work_gate != "operator_pause") {
      *out_error = "resume_session requires an operator-paused session when no "
                   "human artifacts are provided";
      return false;
    }
    const std::string campaign_cursor = trim_ascii(session.campaign_cursor);
    if (!campaign_cursor.empty()) {
      cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
      if (!read_runtime_campaign_direct(*app, campaign_cursor, &campaign,
                                        out_error)) {
        return false;
      }
      const std::string observed_state = observed_campaign_state(campaign);
      if (!is_runtime_campaign_terminal_state(observed_state)) {
        *out_error =
            "resume_session cannot reopen an operator-paused session while "
            "the retained active runtime campaign stop is still in progress";
        return false;
      }
      clear_marshal_session_campaign_linkage(&session);
    }
    session.lifecycle = "live";
    session.activity = "planning";
    session.status_detail = "resume_session requested by operator";
    session.work_gate = "open";
    session.finish_reason = "none";
    session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    clear_marshal_session_runner_identity(&session);
    if (!write_marshal_session(*app, session, out_error))
      return false;
    if (!append_marshal_session_event(&session, "operator", "work.unblocked",
                                      session.status_detail, out_error)) {
      return false;
    }
    session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    if (!write_marshal_session(*app, session, out_error))
      return false;
    std::string runner_error{};
    std::uint64_t runner_pid = 0;
    const std::uint64_t startup_wait_seq = session.last_event_seq;
    if (!launch_session_runner_process(*app, session.marshal_session_id,
                                       &runner_pid, &runner_error)) {
      warnings.push_back("marshal session runner launch failed: " +
                         runner_error);
      session.lifecycle = "terminal";
      session.activity = "quiet";
      session.status_detail = "session runner launch failed: " + runner_error;
      session.work_gate = "open";
      session.finish_reason = "failed";
      session.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      session.updated_at_ms = *session.finished_at_ms;
      std::string ignored{};
      (void)write_marshal_session(*app, session, &ignored);
      (void)append_marshal_session_event(
          &session, "marshal", "session.runner_failed", runner_error, &ignored);
      *out_error = runner_error;
      return false;
    }
    bool runner_identity_captured = false;
    std::string identity_error{};
    if (!capture_process_identity_for_session(runner_pid, &session,
                                              &identity_error)) {
      warnings.push_back("marshal session runner identity degraded: " +
                         identity_error);
    } else {
      runner_identity_captured = true;
      session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      std::string ignored{};
      (void)write_marshal_session(*app, session, &ignored);
    }
    if (!wait_for_session_runner_startup(*app, &session,
                                         runner_identity_captured,
                                         startup_wait_seq, out_error)) {
      return false;
    }
  }

  if (!warnings.empty()) {
    std::string combined_warning{};
    for (const std::string &warning : warnings) {
      append_warning_text(&combined_warning, warning);
    }
    persist_marshal_session_warning_best_effort(*app, &session,
                                                combined_warning);
  }

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"session\":" + marshal_session_to_json(session) +
      ",\"warnings\":" + json_array_from_strings(warnings) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_message_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  std::string message{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  (void)extract_json_string_field(arguments_json, "message", &message);
  marshal_session_id = trim_ascii(marshal_session_id);
  message = trim_ascii(message);
  if (marshal_session_id.empty()) {
    *out_error = "message_session requires arguments.marshal_session_id";
    return false;
  }
  if (message.empty()) {
    *out_error = "message_session requires arguments.message";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  bool reconciled = false;
  if (!read_marshal_session_reconciled(*app, marshal_session_id, &session,
                                       &reconciled, out_error))
    return false;
  (void)reconciled;
  if (is_marshal_session_terminal_state(session.lifecycle)) {
    *out_error = "message_session requires a live session; terminal sessions "
                 "are not reopenable";
    return false;
  }
  if (session.lifecycle != "live") {
    *out_error = "message_session requires a live session";
    return false;
  }

  const bool preserve_review_posture =
      session.finish_reason == "interrupted" ||
      cuwacunu::hero::marshal::marshal_session_is_review_ready(session);
  if (preserve_review_posture) {
    session.activity = "review";
  }

  std::vector<std::string> warnings{};
  if (marshal_session_runner_process_alive(session)) {
    bool runner_matches_current_binary = true;
    std::string runner_warning{};
    if (!marshal_session_runner_matches_current_binary(
            *app, session, &runner_matches_current_binary, &runner_warning)) {
      *out_error = trim_ascii(runner_warning).empty()
                       ? "failed to inspect marshal session runner binary"
                       : runner_warning;
      return false;
    }
    if (!runner_warning.empty()) {
      warnings.push_back(runner_warning);
    }
    if (!runner_matches_current_binary) {
      std::string terminate_warning{};
      if (!terminate_marshal_session_runner_process(session, &terminate_warning,
                                                    out_error)) {
        return false;
      }
      if (!terminate_warning.empty()) {
        warnings.push_back(terminate_warning);
      }
      clear_marshal_session_runner_identity(&session);
      session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      if (!write_marshal_session(*app, session, out_error))
        return false;
      std::string ignored{};
      (void)append_marshal_session_event(
          &session, "marshal", "session.runner_restarted",
          "stale marshal session runner was terminated before operator "
          "message intake so a fresh runner can handle the message",
          &ignored);
      session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      (void)write_marshal_session(*app, session, &ignored);
    }
  }

  cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t
      operator_message{};
  operator_message.message_id = make_operator_message_id(*app, &session);
  operator_message.text = message;
  operator_message.received_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  operator_message.delivery_mode =
      trim_ascii(session.current_thread_id).empty() ? "checkpoint" : "live";
  if (!append_operator_message_event(
          &session, "operator", "operator.message_received", operator_message,
          "operator message intake", "", "", out_error)) {
    return false;
  }

  session.pending_operator_messages.push_back(operator_message);
  session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (trim_ascii(session.status_detail).empty()) {
    session.status_detail = "operator message accepted for marshal processing";
  }
  if (!write_marshal_session(*app, session, out_error))
    return false;

  const std::uint64_t delivery_wait_seq = session.last_event_seq;
  const bool runner_alive = marshal_session_runner_process_alive(session);
  if (!runner_alive) {
    clear_marshal_session_runner_identity(&session);
    std::string runner_error{};
    std::uint64_t runner_pid = 0;
    const std::uint64_t startup_wait_seq = session.last_event_seq;
    if (!launch_session_runner_process(*app, session.marshal_session_id,
                                       &runner_pid, &runner_error)) {
      warnings.push_back("marshal session runner launch failed: " +
                         runner_error);
      auto failed_it = std::find_if(
          session.pending_operator_messages.begin(),
          session.pending_operator_messages.end(), [&](const auto &item) {
            return trim_ascii(item.message_id) ==
                   trim_ascii(operator_message.message_id);
          });
      auto failed_message = operator_message;
      if (failed_it != session.pending_operator_messages.end()) {
        failed_message = *failed_it;
        session.pending_operator_messages.erase(failed_it);
      }
      failed_message.handled_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      failed_message.delivery_status = "failed";
      failed_message.last_error = runner_error;
      session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      std::string ignored{};
      (void)write_marshal_session(*app, session, &ignored);
      (void)append_marshal_session_event(&session, "marshal", "warning.emitted",
                                         runner_error, &ignored);
      (void)append_operator_message_event(
          &session, "marshal", "operator.message_handled", failed_message,
          "runner launch failed", "", runner_error, &ignored);
      *out_structured =
          "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
          ",\"message_id\":" + json_quote(operator_message.message_id) +
          ",\"delivery\":\"failed\"" +
          ",\"warning\":" + json_quote(runner_error) +
          ",\"session\":" + marshal_session_to_json(session) +
          ",\"warnings\":" + json_array_from_strings(warnings) + "}";
      return true;
    }
    bool runner_identity_captured = false;
    std::string identity_error{};
    if (!capture_process_identity_for_session(runner_pid, &session,
                                              &identity_error)) {
      warnings.push_back("marshal session runner identity degraded: " +
                         identity_error);
    } else {
      runner_identity_captured = true;
      session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      std::string ignored{};
      (void)write_marshal_session(*app, session, &ignored);
    }
    if (!wait_for_session_runner_startup(*app, &session,
                                         runner_identity_captured,
                                         startup_wait_seq, out_error)) {
      return false;
    }
  }

  std::string reply_text{};
  std::string failure_text{};
  const operator_message_wait_outcome_t wait_outcome =
      wait_for_operator_message_outcome(
          *app, session.marshal_session_id, delivery_wait_seq,
          operator_message.message_id, &reply_text, &failure_text);
  if (wait_outcome != operator_message_wait_outcome_t::Pending) {
    cuwacunu::hero::marshal::marshal_session_record_t refreshed{};
    std::string refresh_error{};
    if (read_marshal_session_reconciled(*app, marshal_session_id, &refreshed,
                                        nullptr, &refresh_error)) {
      session = std::move(refreshed);
    } else {
      warnings.push_back("post-message session refresh degraded: " +
                         refresh_error);
    }
    if (wait_outcome == operator_message_wait_outcome_t::Delivered) {
      *out_structured =
          "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
          ",\"message_id\":" + json_quote(operator_message.message_id) +
          ",\"delivery\":\"delivered\"" +
          ",\"reply_text\":" + json_quote(reply_text) +
          ",\"session\":" + marshal_session_to_json(session) +
          ",\"warnings\":" + json_array_from_strings(warnings) + "}";
      return true;
    }
    warnings.push_back("operator message delivery failed: " + failure_text);
    *out_structured =
        "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
        ",\"message_id\":" + json_quote(operator_message.message_id) +
        ",\"delivery\":\"failed\"" +
        ",\"warning\":" + json_quote(failure_text) +
        ",\"session\":" + marshal_session_to_json(session) +
        ",\"warnings\":" + json_array_from_strings(warnings) + "}";
    return true;
  }

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"message_id\":" + json_quote(operator_message.message_id) +
      ",\"delivery\":\"queued\"" + ",\"reply_text\":\"\"" +
      ",\"session\":" + marshal_session_to_json(session) +
      ",\"warnings\":" + json_array_from_strings(warnings) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_archive_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "archive_session requires arguments.marshal_session_id";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  bool reconciled = false;
  if (!read_marshal_session_reconciled(*app, marshal_session_id, &session,
                                       &reconciled, out_error)) {
    return false;
  }
  (void)reconciled;

  if (is_marshal_session_terminal_state(session.lifecycle)) {
    const std::string warning =
        "session is already terminal; objective ownership is already released";
    *out_structured =
        "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
        ",\"session\":" + marshal_session_to_json(session) +
        ",\"warning\":" + json_quote(warning) + "}";
    return true;
  }
  if (!cuwacunu::hero::marshal::marshal_session_is_review_ready(session)) {
    *out_error =
        "archive_session requires a review-ready live session or a terminal "
        "session";
    return false;
  }
  if (!trim_ascii(session.campaign_cursor).empty() ||
      (session.campaign_status != "none" &&
       session.campaign_status != "stopped")) {
    *out_error =
        "archive_session requires the session to have no active runtime "
        "campaign";
    return false;
  }

  session.lifecycle = "terminal";
  session.activity = "quiet";
  session.work_gate = "open";
  session.status_detail =
      "archive_session requested by operator; objective ownership released";
  if (trim_ascii(session.finish_reason).empty() ||
      session.finish_reason == "none") {
    session.finish_reason = "success";
  }
  session.last_codex_action = "archive";
  clear_marshal_session_campaign_linkage(&session);
  session.pending_operator_messages.clear();
  session.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  session.updated_at_ms = *session.finished_at_ms;
  clear_marshal_session_runner_identity(&session);
  if (!write_marshal_session(*app, session, out_error)) {
    return false;
  }
  if (!append_marshal_session_event(&session, "operator", "session.archived",
                                    session.status_detail, out_error)) {
    return false;
  }

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"session\":" + marshal_session_to_json(session) + ",\"warning\":\"\"}";
  return true;
}

[[nodiscard]] bool handle_tool_terminate_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  bool force = false;
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  (void)extract_json_bool_field(arguments_json, "force", &force);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "terminate_session requires arguments.marshal_session_id";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  bool reconciled = false;
  if (!read_marshal_session_reconciled(*app, marshal_session_id, &session,
                                       &reconciled, out_error))
    return false;
  (void)reconciled;
  if (is_marshal_session_terminal_state(session.lifecycle)) {
    *out_structured =
        "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
        ",\"session\":" + marshal_session_to_json(session) +
        ",\"warning\":\"\"}";
    return true;
  }

  const std::string campaign_cursor = trim_ascii(session.campaign_cursor);
  std::string runtime_warning{};
  bool active_campaign_stop_pending = false;
  if (!campaign_cursor.empty()) {
    cuwacunu::hero::runtime::runtime_campaign_record_t stopped_campaign{};
    if (!request_runtime_campaign_stop_for_marshal_action(
            *app, session, campaign_cursor, force, &stopped_campaign,
            &runtime_warning, out_error)) {
      return false;
    }
    active_campaign_stop_pending =
        !is_runtime_campaign_terminal_state(trim_ascii(stopped_campaign.state));
  }
  cancel_session_checkpoint_best_effort(session, force, &runtime_warning);

  session.lifecycle = "terminal";
  session.activity = "quiet";
  session.status_detail =
      active_campaign_stop_pending
          ? "terminate_session requested by operator; active runtime campaign "
            "stop in progress"
          : "terminate_session requested by operator";
  session.work_gate = "open";
  session.finish_reason = "terminated";
  session.last_codex_action = "terminate";
  session.campaign_status = active_campaign_stop_pending ? "stopping" : "none";
  session.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  session.updated_at_ms = *session.finished_at_ms;
  if (!active_campaign_stop_pending) {
    clear_marshal_session_campaign_linkage(&session);
  }
  clear_marshal_session_runner_identity(&session);
  if (!write_marshal_session(*app, session, out_error))
    return false;
  if (!append_marshal_session_event(&session, "operator", "session.finished",
                                    session.status_detail, out_error)) {
    return false;
  }
  if (!runtime_warning.empty()) {
    persist_marshal_session_warning_best_effort(*app, &session,
                                                runtime_warning);
  }

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"session\":" + marshal_session_to_json(session) +
      ",\"warning\":" + json_quote(runtime_warning) + "}";
  return true;
}
