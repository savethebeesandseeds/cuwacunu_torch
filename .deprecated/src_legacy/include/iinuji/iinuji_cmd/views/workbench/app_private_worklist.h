
inline bool message_workbench_session(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string message{};
  bool cancelled = false;
  if (!ui_prompt_text_dialog(
          " Message session ",
          "Provide the operator message Marshal Hero should receive. Plain "
          "messages can ask questions, update future strategy, or request "
          "that the active campaign be interrupted and replanned.",
          &message, false, false, &cancelled)) {
    set_workbench_status(st, "failed to collect operator message", true);
    return true;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  bool ok = false;
  ui_run_blocking_dialog(
      " Sending message ",
      "Dispatching operator message to Marshal Hero.",
      [&]() {
        ok = cuwacunu::hero::human_mcp::message_marshal_session(
            &st.workbench.app, session, message, &structured, &error);
      });
  if (!ok) {
    set_workbench_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  std::string status = "Delivered message to session.";
  bool status_is_error = false;
  try {
    const cmd_json_value_t parsed =
        cuwacunu::piaabo::JsonParser(structured).parse();
    const std::string delivery =
        cmd_json_string(cmd_json_field(&parsed, "delivery"));
    const std::string reply_text =
        cmd_json_string(cmd_json_field(&parsed, "reply_text"));
    const std::string warning =
        cmd_json_string(cmd_json_field(&parsed, "warning"));
    const auto compact = [](std::string text) {
      text = trim_copy(text);
      for (char &ch : text) {
        if (ch == '\n' || ch == '\r' || ch == '\t')
          ch = ' ';
      }
      constexpr std::size_t kMax = 180;
      if (text.size() > kMax) {
        text = text.substr(0, kMax - 3);
        text += "...";
      }
      return text;
    };
    if (delivery == "failed") {
      status_is_error = true;
      status = compact(warning.empty() ? "Marshal message delivery failed."
                                       : warning);
    } else if (delivery == "delivered" && !trim_copy(reply_text).empty()) {
      status = "Marshal: " + compact(reply_text);
    } else if (delivery == "queued") {
      status = "Marshal is processing the message.";
    }
  } catch (...) {
  }
  set_workbench_status(st, std::move(status), status_is_error);
  return true;
}

inline bool answer_workbench_request(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string answer{};
  bool cancelled = false;
  if (!ui_prompt_text_dialog(
          " Answer request ",
          "Provide the clarification answer that Marshal Hero should continue "
          "with.",
          &answer, false, false, &cancelled)) {
    set_workbench_status(st, "failed to collect request answer", true);
    return true;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  bool ok = false;
  ui_run_blocking_dialog(
      " Applying answer ",
      "Submitting clarification answer to Marshal Hero.",
      [&]() {
        ok = cuwacunu::hero::human_mcp::build_request_answer_and_resume(
            &st.workbench.app, session, answer, &structured, &error);
      });
  if (!ok) {
    set_workbench_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_workbench_status(st, "Applied clarification answer.", false);
  return true;
}

inline bool collect_workbench_resolution_input(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool stop_direct,
    cuwacunu::hero::human_mcp::interactive_resolution_input_t *out,
    bool *cancelled) {
  if (cancelled)
    *cancelled = false;
  if (out == nullptr)
    return false;
  *out = cuwacunu::hero::human_mcp::interactive_resolution_input_t{};

  if (stop_direct) {
    out->resolution_kind = "terminate";
  } else {
    std::vector<ui_choice_panel_option_t> options{
        ui_choice_panel_option_t{.label = "Grant requested governance",
                                 .enabled = true},
        ui_choice_panel_option_t{
            .label = "Clarify objective or policy without widening authority",
            .enabled = true},
        ui_choice_panel_option_t{.label = "Deny requested governance",
                                 .enabled = true},
        ui_choice_panel_option_t{.label = "Terminate the session",
                                 .enabled = true},
    };
    std::size_t resolution_idx = 0u;
    if (!ui_prompt_choice_panel(
            " Resolution ",
            "Choose how to answer this pending governance request.", options,
            &resolution_idx, cancelled)) {
      return false;
    }
    if (cancelled && *cancelled)
      return true;
    if (resolution_idx == 0u) {
      out->resolution_kind = "grant";
    } else if (resolution_idx == 1u) {
      out->resolution_kind = "clarify";
    } else if (resolution_idx == 2u) {
      out->resolution_kind = "deny";
    } else {
      out->resolution_kind = "terminate";
    }
  }

  const std::string prompt_title =
      (out->resolution_kind == "grant")     ? " Grant rationale "
      : (out->resolution_kind == "clarify") ? " Clarification "
      : (out->resolution_kind == "deny")    ? " Denial rationale "
                                            : " Termination rationale ";
  const std::string prompt_body =
      (out->resolution_kind == "grant")
          ? "Why grant this governance request? This reason is shown to "
            "Marshal Hero."
      : (out->resolution_kind == "clarify")
          ? "What clarification should Marshal Hero incorporate on the next "
            "planning checkpoint?"
      : (out->resolution_kind == "deny")
          ? "Why deny this governance request? This reason is shown to Marshal "
            "Hero."
          : "Why terminate this session? This reason is shown to Marshal Hero.";
  if (!ui_prompt_text_dialog(prompt_title, prompt_body, &out->reason, false,
                             false, cancelled)) {
    return false;
  }
  if (cancelled && *cancelled)
    return true;

  (void)session;
  return true;
}

inline bool apply_workbench_request_resolution(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool stop_direct) {
  cuwacunu::hero::human_mcp::interactive_resolution_input_t input{};
  bool cancelled = false;
  if (!collect_workbench_resolution_input(session, stop_direct, &input,
                                      &cancelled)) {
    set_workbench_status(st, "failed to collect governance resolution input", true);
    return true;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  bool ok = false;
  ui_run_blocking_dialog(
      " Applying resolution ",
      "Submitting signed governance resolution to Marshal Hero.",
      [&]() {
        ok = cuwacunu::hero::human_mcp::build_resolution_and_apply(
            &st.workbench.app, session, input.resolution_kind, input.reason,
            &structured, &error);
      });
  if (!ok) {
    set_workbench_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_workbench_status(st, "Applied signed governance resolution.", false);
  return true;
}

inline bool acknowledge_workbench_review(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string note{};
  bool cancelled = false;
  if (!ui_prompt_text_dialog(
          " Acknowledge review ",
          "This does not continue the session. It records a signed "
          "acknowledgment and clears the review report from the inbox after "
          "review. Provide a required acknowledgment message:",
          &note, false, false, &cancelled)) {
    set_workbench_status(st, "failed to collect review acknowledgment note", true);
    return true;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  bool ok = false;
  ui_run_blocking_dialog(
      " Acknowledging review ",
      "Recording the signed review acknowledgment.",
      [&]() {
        ok = cuwacunu::hero::human_mcp::acknowledge_session_summary(
            &st.workbench.app, session, note, &structured, &error);
      });
  if (!ok) {
    set_workbench_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_workbench_status(st, "Acknowledged review report.", false);
  return true;
}

inline bool archive_workbench_review(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string note{};
  bool cancelled = false;
  if (!ui_prompt_text_dialog(
          " Archive review ",
          "This is final for this session. It records a signed archive "
          "message, marks the Marshal terminal, and releases the objective so "
          "Booster can launch a fresh Marshal for it. Provide a required "
          "archive message:",
          &note, false, false, &cancelled)) {
    set_workbench_status(st, "failed to collect review archive note", true);
    return true;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  bool ok = false;
  ui_run_blocking_dialog(
      " Archiving review ",
      "Recording archive note and releasing the objective bundle.",
      [&]() {
        ok = cuwacunu::hero::human_mcp::archive_session_summary(
            &st.workbench.app, session, note, &structured, &error);
      });
  if (!ok) {
    set_workbench_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_workbench_status(st, "Archived review and released objective.", false);
  return true;
}

inline bool
dispatch_workbench_popup_action(CmdState &st,
                                  const workbench_popup_action_t &action) {
  switch (action.kind) {
  case workbench_popup_action_kind_t::PauseSession: {
    const auto selected = selected_operator_visible_session_record(st);
    if (!selected.has_value())
      return false;
    std::string structured{};
    std::string error{};
    bool ok = false;
    ui_run_blocking_dialog(
        " Pausing session ",
        "Sending pause request to Marshal Hero.",
        [&]() {
          ok = cuwacunu::hero::human_mcp::pause_marshal_session(
              &st.workbench.app, *selected, false, &structured, &error);
        });
    if (!ok) {
      set_workbench_status(st, error, true);
      return true;
    }
    refresh_hero_shell_views(st);
    set_workbench_status(st, "Paused selected session.", false);
    return true;
  }
  case workbench_popup_action_kind_t::ResumeOperatorPause: {
    const auto selected = selected_operator_visible_session_record(st);
    if (!selected.has_value())
      return false;
    std::string structured{};
    std::string error{};
    bool ok = false;
    ui_run_blocking_dialog(
        " Resuming session ",
        "Sending resume request to Marshal Hero.",
        [&]() {
          ok = cuwacunu::hero::human_mcp::resume_marshal_session(
              &st.workbench.app, *selected, &structured, &error);
        });
    if (!ok) {
      set_workbench_status(st, error, true);
      return true;
    }
    refresh_hero_shell_views(st);
    set_workbench_status(st, "Resumed operator-paused session.", false);
    return true;
  }
  case workbench_popup_action_kind_t::MessageSession: {
    const auto selected = selected_operator_visible_session_record(st);
    if (!selected.has_value())
      return false;
    return message_workbench_session(st, *selected);
  }
  case workbench_popup_action_kind_t::TerminateSession: {
    const auto selected = selected_operator_visible_session_record(st);
    if (!selected.has_value())
      return false;
    bool confirmed = false;
    bool cancelled = false;
    if (!ui_prompt_yes_no_dialog(
            " Terminate session ",
            "Terminate the selected session? This action is final.", false,
            &confirmed, &cancelled)) {
      set_workbench_status(st, "failed to collect session termination confirmation",
                       true);
      return true;
    }
    if (cancelled || !confirmed) {
      set_workbench_status(st, "Cancelled.", false);
      return true;
    }
    std::string structured{};
    std::string error{};
    bool ok = false;
    ui_run_blocking_dialog(
        " Terminating session ",
        "Sending terminate request to Marshal Hero.",
        [&]() {
          ok = cuwacunu::hero::human_mcp::terminate_marshal_session(
              &st.workbench.app, *selected, false, &structured, &error);
        });
    if (!ok) {
      set_workbench_status(st, error, true);
      return true;
    }
    refresh_hero_shell_views(st);
    set_workbench_status(st, "Terminated selected session.", false);
    return true;
  }
  case workbench_popup_action_kind_t::AnswerRequest: {
    const auto selected = selected_workbench_request_record(st);
    if (!selected.has_value())
      return false;
    return answer_workbench_request(st, *selected);
  }
  case workbench_popup_action_kind_t::ResolveRequest: {
    const auto selected = selected_workbench_request_record(st);
    if (!selected.has_value())
      return false;
    return apply_workbench_request_resolution(st, *selected, false);
  }
  case workbench_popup_action_kind_t::TerminateGovernanceRequest: {
    const auto selected = selected_workbench_request_record(st);
    if (!selected.has_value())
      return false;
    return apply_workbench_request_resolution(st, *selected, true);
  }
  case workbench_popup_action_kind_t::AcknowledgeReview: {
    const auto selected = selected_workbench_review_record(st);
    if (!selected.has_value())
      return false;
    return acknowledge_workbench_review(st, *selected);
  }
  case workbench_popup_action_kind_t::ArchiveReview: {
    const auto selected = selected_workbench_review_record(st);
    if (!selected.has_value())
      return false;
    return archive_workbench_review(st, *selected);
  }
  }
  return false;
}

inline bool open_workbench_action_menu(CmdState &st) {
  const auto actions = workbench_popup_actions(st);
  if (actions.empty()) {
    set_workbench_status(st, "No actions available for the selected row.", false);
    return true;
  }

  std::size_t selected = 0u;
  bool cancelled = false;
  if (!prompt_action_choice_panel("Workbench Actions",
                                  "Select workbench action.", actions,
                                  &selected, &cancelled)) {
    set_workbench_status(st, "failed to open action menu", true);
    return true;
  }
  if (cancelled) {
    set_workbench_status(st, {}, false);
    return true;
  }
  const auto &action = selected_action_or_last(actions, selected);
  if (!action.enabled) {
    set_workbench_status(st,
                     action.disabled_status.empty()
                         ? "Selected action is currently unavailable."
                         : action.disabled_status,
                     false);
    return true;
  }
  return dispatch_workbench_popup_action(st, action);
}

inline bool handle_workbench_key(CmdState &st, int ch) {
  if (st.screen != ScreenMode::Workbench)
    return false;
  if ((ch == 27 || ch == KEY_LEFT || ch == 'h') &&
      workbench_is_worklist_focus(st.workbench.focus)) {
    focus_workbench_navigation(st);
    return true;
  }
  if (workbench_is_navigation_focus(st.workbench.focus)) {
    if (ch == KEY_UP || ch == 'k') {
      return select_prev_workbench_section(st);
    }
    if (ch == KEY_DOWN || ch == 'j') {
      return select_next_workbench_section(st);
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      focus_workbench_worklist(st);
      return true;
    }
    if (ch == KEY_RIGHT || ch == 'l') {
      focus_workbench_worklist(st);
      return true;
    }
    return false;
  }

  if (workbench_is_booster_section(st.workbench.section)) {
    if (ch == KEY_UP || ch == 'k') {
      return select_prev_workbench_booster_action(st);
    }
    if (ch == KEY_DOWN || ch == 'j') {
      return select_next_workbench_booster_action(st);
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == 'a') {
      return dispatch_workbench_booster_action(
          st, selected_workbench_booster_action(st));
    }
    return false;
  }

  if (ch == KEY_UP || ch == 'k')
    return select_prev_workbench_row(st);
  if (ch == KEY_DOWN || ch == 'j')
    return select_next_workbench_row(st);
  if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
    return open_workbench_action_menu(st);
  }
  return false;
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
