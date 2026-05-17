[[nodiscard]] bool collect_interactive_resolution_inputs(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool stop_direct, interactive_resolution_input_t *out, bool *cancelled,
    std::string *error) {
  if (cancelled)
    *cancelled = false;
  if (error)
    error->clear();
  if (!app || !out) {
    if (error)
      *error = "interactive resolution output pointer is null";
    return false;
  }
  *out = interactive_resolution_input_t{};

  if (stop_direct) {
    out->resolution_kind = "terminate";
  } else {
    std::size_t resolution_idx = 0;
    if (!prompt_choice_dialog(
            " Resolution ",
            "Choose how to answer this pending governance request.",
            {"Grant requested governance",
             "Clarify objective or policy without widening authority",
             "Deny requested governance", "Terminate the session"},
            &resolution_idx, cancelled)) {
      return false;
    }
    if (cancelled && *cancelled)
      return true;
    if (resolution_idx == 0) {
      out->resolution_kind = "grant";
    } else if (resolution_idx == 1) {
      out->resolution_kind = "clarify";
    } else if (resolution_idx == 2) {
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
  if (!prompt_text_dialog(prompt_title, prompt_body, &out->reason, false, false,
                          cancelled)) {
    return false;
  }
  if (cancelled && *cancelled)
    return true;

  (void)session;
  return true;
}

[[nodiscard]] bool run_ncurses_operator_console(app_context_t *app,
                                                std::string *error) {
  if (error)
    error->clear();
  if (!app) {
    if (error)
      *error = "human app pointer is null";
    return false;
  }

  try {
    cuwacunu::iinuji::NcursesAppOpts opts{};
    opts.input_timeout_ms = -1;
    opts.clear_on_start = false;
    cuwacunu::iinuji::NcursesApp curses_app(opts);
    init_human_ncurses_theme();
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);
    mouseinterval(0);

    std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        all_sessions{};
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        pending_requests{};
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        pending_reviews{};
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        session_rows{};
    operator_console_view_t current_view = operator_console_view_t::sessions;
    operator_console_state_filter_t session_state_filter =
        operator_console_state_filter_t::all;
    std::size_t selected_session = 0;
    std::size_t selected_request = 0;
    std::size_t selected_review = 0;
    std::size_t detail_scroll = 0;
    std::size_t detail_total_lines = 0;
    std::size_t detail_page_lines = 0;
    detail_viewport_t detail_viewport{};
    bool detail_dirty = true;
    std::string detail_text{};
    std::string detail_session_id{};
    operator_console_view_t detail_view = operator_console_view_t::sessions;
    std::string status = "Scanning operator console...";
    bool status_is_error = false;
    bool dirty = true;
    bool inbox_refresh_pending = true;
    bool inbox_refresh_needs_paint = true;
    bool has_loaded_once = false;

    for (;;) {
      if (inbox_refresh_pending && inbox_refresh_needs_paint)
        dirty = true;

      if (dirty) {
        if (!has_loaded_once && inbox_refresh_pending) {
          render_human_requests_bootstrap_screen(
              app->defaults.operator_id, app->defaults.marshal_root.string(),
              status, status_is_error);
        } else {
          filter_sessions_for_console(all_sessions, session_state_filter,
                                      &session_rows);

          auto &visible =
              current_view == operator_console_view_t::sessions
                  ? session_rows
                  : (current_view == operator_console_view_t::requests
                         ? pending_requests
                         : pending_reviews);
          auto &selected =
              current_view == operator_console_view_t::sessions
                  ? selected_session
                  : (current_view == operator_console_view_t::requests
                         ? selected_request
                         : selected_review);
          if (!visible.empty()) {
            selected = std::min(selected, visible.size() - 1);
          } else {
            selected = 0;
          }

          if (visible.empty()) {
            detail_text.clear();
            detail_session_id.clear();
            detail_scroll = 0;
            detail_total_lines = 0;
            detail_page_lines = 0;
            detail_viewport = detail_viewport_t{};
            detail_dirty = false;
          } else if (detail_dirty ||
                     detail_session_id !=
                         visible[selected].marshal_session_id ||
                     detail_view != current_view) {
            if (current_view == operator_console_view_t::sessions) {
              const bool has_pending_review =
                  find_session_index_by_id(pending_reviews,
                                           visible[selected].marshal_session_id)
                      .has_value();
              detail_text = build_session_console_detail_text(
                  visible[selected], has_pending_review);
            } else {
              detail_text.clear();
              std::string detail_error{};
              const bool read_ok =
                  current_view == operator_console_view_t::requests
                      ? read_request_text_for_session(
                            visible[selected], &detail_text, &detail_error)
                      : read_summary_text_for_session(
                            visible[selected], &detail_text, &detail_error);
              if (!read_ok) {
                detail_text = std::string("<failed to read ") +
                              (current_view == operator_console_view_t::requests
                                   ? "request"
                                   : "review report") +
                              ": " + detail_error + ">";
              }
            }
            detail_session_id = visible[selected].marshal_session_id;
            detail_view = current_view;
            detail_dirty = false;
          }

          if (current_view == operator_console_view_t::sessions) {
            render_human_sessions_screen(
                all_sessions, pending_reviews, pending_requests.size(),
                pending_reviews.size(),
                session_state_filter, app->defaults.operator_id,
                app->defaults.marshal_root.string(), selected, detail_text,
                detail_scroll, &detail_total_lines, &detail_page_lines,
                &detail_scroll, &detail_viewport, status, status_is_error);
          } else if (current_view == operator_console_view_t::requests) {
            render_human_requests_screen(
                pending_requests, pending_reviews.size(),
                app->defaults.operator_id, app->defaults.marshal_root.string(),
                selected, detail_text, detail_scroll, &detail_total_lines,
                &detail_page_lines, &detail_scroll, &detail_viewport, status,
                status_is_error);
          } else {
            render_human_reviews_screen(
                pending_reviews, pending_requests.size(),
                app->defaults.operator_id, app->defaults.marshal_root.string(),
                selected, detail_text, detail_scroll, &detail_total_lines,
                &detail_page_lines, &detail_scroll, &detail_viewport, status,
                status_is_error);
          }
        }
        dirty = false;
      }

      if (inbox_refresh_pending) {
        if (inbox_refresh_needs_paint) {
          inbox_refresh_needs_paint = false;
          continue;
        }

        std::string refresh_error{};
        human_operator_inbox_t inbox{};
        if (!collect_human_operator_inbox(*app, &inbox, true, &refresh_error)) {
          status = refresh_error;
          status_is_error = true;
        } else {
          all_sessions = std::move(inbox.all_sessions);
          pending_requests = std::move(inbox.actionable_requests);
          pending_reviews = std::move(inbox.unacknowledged_summaries);
          sort_sessions_newest_first(&all_sessions);
          sort_sessions_newest_first(&pending_requests);
          sort_sessions_newest_first(&pending_reviews);
          if (!pending_requests.empty()) {
            selected_request =
                std::min(selected_request, pending_requests.size() - 1);
          } else {
            selected_request = 0;
          }
          if (!pending_reviews.empty()) {
            selected_review =
                std::min(selected_review, pending_reviews.size() - 1);
          } else {
            selected_review = 0;
          }
          filter_sessions_for_console(all_sessions, session_state_filter,
                                      &session_rows);
          if (!session_rows.empty()) {
            selected_session =
                std::min(selected_session, session_rows.size() - 1);
          } else {
            selected_session = 0;
          }
          const bool has_sessions = !all_sessions.empty();
          const bool has_requests = !pending_requests.empty();
          const bool has_reviews = !pending_reviews.empty();
          status =
              has_loaded_once
                  ? (has_sessions
                         ? (has_requests
                                ? (has_reviews ? "Refreshed sessions, "
                                                 "requests, and reviews."
                                               : "Refreshed sessions and "
                                                 "pending requests.")
                                : (has_reviews
                                       ? "Refreshed sessions and reviews."
                                       : "Refreshed session inventory."))
                         : "No sessions or pending items.")
                  : (has_sessions
                         ? (has_requests
                                ? (has_reviews
                                       ? "Ready with sessions, requests, and "
                                         "reviews."
                                       : "Ready with sessions and requests.")
                                : (has_reviews
                                       ? "Ready with sessions and reviews."
                                       : "Ready with session inventory."))
                         : "No sessions or pending items. Waiting for Marshal "
                           "Hero.");
          status_is_error = false;
        }
        has_loaded_once = true;
        inbox_refresh_pending = false;
        detail_dirty = true;
        detail_scroll = 0;
        detail_viewport = detail_viewport_t{};
        dirty = true;
        continue;
      }

      const int ch = getch();
      if (ch == 'q' || ch == 'Q' || ch == 27)
        return true;
      if (ch == KEY_RESIZE) {
        dirty = true;
        continue;
      }
      if (ch == KEY_MOUSE) {
        MEVENT me{};
        if (getmouse(&me) == OK &&
            mouse_hits_detail_viewport(detail_viewport, me.x, me.y)) {
          const std::size_t max_scroll =
              detail_viewport.total_lines > detail_viewport.page_lines
                  ? detail_viewport.total_lines - detail_viewport.page_lines
                  : 0u;
          bool handled_mouse = false;
          if (me.bstate & BUTTON4_PRESSED) {
            detail_scroll = detail_scroll > 3 ? detail_scroll - 3 : 0u;
            handled_mouse = true;
          } else if (me.bstate & BUTTON5_PRESSED) {
            detail_scroll = std::min(detail_scroll + 3, max_scroll);
            handled_mouse = true;
          } else if ((me.bstate & BUTTON1_PRESSED) ||
                     (me.bstate & BUTTON1_CLICKED)) {
            if (detail_viewport.scrollbar_x > 0 &&
                me.x == detail_viewport.scrollbar_x && max_scroll > 0) {
              const int relative_y = std::clamp(me.y - detail_viewport.y, 0,
                                                detail_viewport.height - 1);
              detail_scroll =
                  detail_viewport.height <= 1
                      ? max_scroll
                      : static_cast<std::size_t>(
                            (static_cast<unsigned long long>(relative_y) *
                             max_scroll) /
                            static_cast<unsigned long long>(
                                detail_viewport.height - 1));
              handled_mouse = true;
            }
          }
          if (handled_mouse) {
            dirty = true;
            continue;
          }
        }
        continue;
      }
      if (ch == 'r' || ch == 'R') {
        status = "Refreshing operator inbox...";
        status_is_error = false;
        inbox_refresh_pending = true;
        inbox_refresh_needs_paint = true;
        detail_dirty = true;
        detail_scroll = 0;
        detail_viewport = detail_viewport_t{};
        dirty = true;
        continue;
      }
      if (ch == '\t' || ch == KEY_BTAB) {
        std::string active_session_id{};
        if (current_view == operator_console_view_t::sessions) {
          filter_sessions_for_console(all_sessions, session_state_filter,
                                      &session_rows);
          if (!session_rows.empty() && selected_session < session_rows.size()) {
            active_session_id =
                session_rows[selected_session].marshal_session_id;
          }
        } else if (current_view == operator_console_view_t::requests) {
          if (!pending_requests.empty() &&
              selected_request < pending_requests.size()) {
            active_session_id =
                pending_requests[selected_request].marshal_session_id;
          }
        } else if (!pending_reviews.empty() &&
                   selected_review < pending_reviews.size()) {
          active_session_id =
              pending_reviews[selected_review].marshal_session_id;
        }
        cycle_operator_console_view(&current_view);
        if (!active_session_id.empty()) {
          filter_sessions_for_console(all_sessions, session_state_filter,
                                      &session_rows);
          if (current_view == operator_console_view_t::sessions) {
            select_session_by_id(session_rows, active_session_id,
                                 &selected_session);
          } else if (current_view == operator_console_view_t::requests) {
            select_session_by_id(pending_requests, active_session_id,
                                 &selected_request);
          } else {
            select_session_by_id(pending_reviews, active_session_id,
                                 &selected_review);
          }
        }
        detail_dirty = true;
        detail_scroll = 0;
        detail_viewport = detail_viewport_t{};
        dirty = true;
        continue;
      }
      filter_sessions_for_console(all_sessions, session_state_filter,
                                  &session_rows);
      auto &visible = current_view == operator_console_view_t::sessions
                          ? session_rows
                          : (current_view == operator_console_view_t::requests
                                 ? pending_requests
                                 : pending_reviews);
      auto &selected = current_view == operator_console_view_t::sessions
                           ? selected_session
                           : (current_view == operator_console_view_t::requests
                                  ? selected_request
                                  : selected_review);
      if (current_view == operator_console_view_t::sessions &&
          (ch == 'f' || ch == 'F')) {
        cycle_operator_console_state_filter(&session_state_filter);
        selected_session = 0;
        detail_dirty = true;
        detail_scroll = 0;
        detail_viewport = detail_viewport_t{};
        dirty = true;
        continue;
      }
      if (!visible.empty()) {
        selected = std::min(selected, visible.size() - 1);
      }
      if (visible.empty()) {
        dirty = true;
        continue;
      }
      if (ch == KEY_UP || ch == 'k') {
        if (selected > 0)
          --selected;
        detail_dirty = true;
        detail_scroll = 0;
        detail_viewport = detail_viewport_t{};
        dirty = true;
        continue;
      }
      if (ch == KEY_DOWN || ch == 'j') {
        if (selected + 1 < visible.size())
          ++selected;
        detail_dirty = true;
        detail_scroll = 0;
        detail_viewport = detail_viewport_t{};
        dirty = true;
        continue;
      }
      if (ch == KEY_PPAGE) {
        if (detail_page_lines != 0) {
          detail_scroll = detail_scroll > detail_page_lines
                              ? detail_scroll - detail_page_lines
                              : 0u;
          dirty = true;
        }
        continue;
      }
      if (ch == KEY_NPAGE) {
        if (detail_page_lines != 0 && detail_total_lines > detail_page_lines) {
          const std::size_t max_scroll = detail_total_lines - detail_page_lines;
          detail_scroll =
              std::min(detail_scroll + detail_page_lines, max_scroll);
          dirty = true;
        }
        continue;
      }
      if (ch == KEY_HOME) {
        if (detail_scroll != 0) {
          detail_scroll = 0;
          dirty = true;
        }
        continue;
      }
      if (ch == KEY_END) {
        if (detail_page_lines != 0 && detail_total_lines > detail_page_lines) {
          detail_scroll = detail_total_lines - detail_page_lines;
          dirty = true;
        }
        continue;
      }
      if (current_view == operator_console_view_t::sessions) {
        const auto &session = visible[selected];
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
          if (find_session_index_by_id(pending_requests,
                                       session.marshal_session_id)
                  .has_value()) {
            current_view = operator_console_view_t::requests;
            select_session_by_id(pending_requests, session.marshal_session_id,
                                 &selected_request);
            detail_dirty = true;
            detail_scroll = 0;
            dirty = true;
            continue;
          }
          if (find_session_index_by_id(pending_reviews,
                                       session.marshal_session_id)
                  .has_value()) {
            current_view = operator_console_view_t::summaries;
            select_session_by_id(pending_reviews, session.marshal_session_id,
                                 &selected_review);
            detail_dirty = true;
            detail_scroll = 0;
            dirty = true;
            continue;
          }
          status = "This session has no pending request or review lane entry.";
          status_is_error = false;
          dirty = true;
          continue;
        }
        if ((ch == 'p' || ch == 'P') && session.lifecycle == "live" &&
            (session.activity == "planning" ||
             session.campaign_status == "launching" ||
             session.campaign_status == "running" ||
             session.campaign_status == "stopping")) {
          std::string structured{};
          std::string action_error{};
          if (!pause_marshal_session(app, session, false, &structured,
                                     &action_error)) {
            status = action_error;
            status_is_error = true;
            dirty = true;
            continue;
          }
          status = "Paused selected session. Refreshing operator console...";
          status_is_error = false;
          inbox_refresh_pending = true;
          inbox_refresh_needs_paint = true;
          detail_dirty = true;
          detail_scroll = 0;
          dirty = true;
          continue;
        }
        if ((ch == 'u' || ch == 'U') && session.lifecycle == "live" &&
            session.work_gate == "operator_pause") {
          std::string structured{};
          std::string action_error{};
          if (!resume_marshal_session(app, session, &structured,
                                      &action_error)) {
            status = action_error;
            status_is_error = true;
            dirty = true;
            continue;
          }
          status =
              "Resumed operator-paused session. Refreshing operator console...";
          status_is_error = false;
          inbox_refresh_pending = true;
          inbox_refresh_needs_paint = true;
          detail_dirty = true;
          detail_scroll = 0;
          dirty = true;
          continue;
        }
        if ((ch == 'm' || ch == 'M') && session.lifecycle == "live") {
          std::string message{};
          bool cancelled = false;
          if (!prompt_text_dialog(
                  " Message session ",
                  "Provide the operator message Marshal Hero should receive. "
                  "Plain messages can ask questions, update future strategy, "
                  "or request that the active campaign be interrupted and "
                  "replanned.",
                  &message, false, false, &cancelled)) {
            status = "failed to collect operator message";
            status_is_error = true;
            dirty = true;
            continue;
          }
          if (cancelled) {
            status = "Cancelled.";
            status_is_error = false;
            dirty = true;
            continue;
          }
          std::string structured{};
          std::string action_error{};
          if (!message_marshal_session(app, session, message, &structured,
                                        &action_error)) {
            status = action_error;
            status_is_error = true;
            dirty = true;
            continue;
          }
          status = summarize_message_session_structured(structured, true,
                                                       &status_is_error);
          inbox_refresh_pending = true;
          inbox_refresh_needs_paint = true;
          detail_dirty = true;
          detail_scroll = 0;
          dirty = true;
          continue;
        }
        if ((ch == 't' || ch == 'T') && session.lifecycle != "terminal") {
          bool confirmed = false;
          bool cancelled = false;
          if (!prompt_yes_no_dialog(
                  " Terminate session ",
                  "Terminate the selected session? This action is final.",
                  false, &confirmed, &cancelled)) {
            status = "failed to collect session termination confirmation";
            status_is_error = true;
            dirty = true;
            continue;
          }
          if (cancelled || !confirmed) {
            status = "Cancelled.";
            status_is_error = false;
            dirty = true;
            continue;
          }
          std::string structured{};
          std::string action_error{};
          if (!terminate_marshal_session(app, session, false, &structured,
                                         &action_error)) {
            status = action_error;
            status_is_error = true;
            dirty = true;
            continue;
          }
          status = "Terminated selected session from the session cockpit. "
                   "Refreshing...";
          status_is_error = false;
          inbox_refresh_pending = true;
          inbox_refresh_needs_paint = true;
          detail_dirty = true;
          detail_scroll = 0;
          dirty = true;
          continue;
        }
        continue;
      }
      if (current_view == operator_console_view_t::requests &&
          (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == 'c' ||
           ch == 'C' || ch == 's' || ch == 'S')) {
        if (is_clarification_work_gate(visible[selected].work_gate)) {
          std::string answer{};
          bool cancelled = false;
          if (!prompt_text_dialog(" Answer request ",
                                  "Provide the clarification answer that "
                                  "Marshal Hero should continue with.",
                                  &answer, false, false, &cancelled)) {
            status = "failed to collect request answer";
            status_is_error = true;
            dirty = true;
            continue;
          }
          if (cancelled) {
            status = "Cancelled.";
            status_is_error = false;
            dirty = true;
            continue;
          }
          std::string structured{};
          std::string response_error{};
          if (!build_request_answer_and_resume(app, visible[selected], answer,
                                               &structured, &response_error)) {
            status = response_error;
            status_is_error = true;
            dirty = true;
            continue;
          }
          status = "Applied clarification answer. Refreshing operator inbox...";
          status_is_error = false;
          inbox_refresh_pending = true;
          inbox_refresh_needs_paint = true;
          detail_dirty = true;
          detail_scroll = 0;
          dirty = true;
          continue;
        }
        const bool stop_direct = (ch == 's' || ch == 'S');
        interactive_resolution_input_t input{};
        bool cancelled = false;
        std::string input_error{};
        if (!collect_interactive_resolution_inputs(app, visible[selected],
                                                   stop_direct, &input,
                                                   &cancelled, &input_error)) {
          status = input_error;
          status_is_error = true;
          dirty = true;
          continue;
        }
        if (cancelled) {
          status = "Cancelled.";
          status_is_error = false;
          dirty = true;
          continue;
        }
        std::string structured{};
        std::string response_error{};
        if (!build_resolution_and_apply(app, visible[selected],
                                        input.resolution_kind, input.reason,
                                        &structured, &response_error)) {
          status = response_error;
          status_is_error = true;
          dirty = true;
          continue;
        }
        status = "Applied signed governance resolution. Refreshing operator "
                 "inbox...";
        status_is_error = false;
        inbox_refresh_pending = true;
        inbox_refresh_needs_paint = true;
        detail_dirty = true;
        detail_scroll = 0;
        dirty = true;
        continue;
      }
      if (current_view == operator_console_view_t::summaries &&
          (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == 'm' ||
           ch == 'M' || ch == 'a' || ch == 'A')) {
        const bool can_message =
            (visible[selected].lifecycle == "live" &&
             visible[selected].activity == "review");
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
          status = can_message
                       ? "Review-ready report. Use 'm' to send a message, or "
                         "'a' to acknowledge and clear the report."
                       : "Finished review report. Use 'a' to acknowledge and "
                         "clear the report after review.";
          status_is_error = false;
          dirty = true;
          continue;
        }
        if ((ch == 'm' || ch == 'M') && can_message) {
          std::string message{};
          bool cancelled = false;
          if (!prompt_text_dialog(
                  " Message session ",
                  "Provide the operator message Marshal Hero should receive. "
                  "Plain messages can ask questions, update future strategy, "
                  "or request that the active campaign be interrupted and "
                  "replanned.",
                  &message, false, false, &cancelled)) {
            status = "failed to collect operator message";
            status_is_error = true;
            dirty = true;
            continue;
          }
          if (cancelled) {
            status = "Cancelled.";
            status_is_error = false;
            dirty = true;
            continue;
          }
          std::string structured{};
          std::string message_error{};
          if (!message_marshal_session(app, visible[selected], message,
                                       &structured, &message_error)) {
            status = message_error;
            status_is_error = true;
            dirty = true;
            continue;
          }
          status = summarize_message_session_structured(structured, true,
                                                       &status_is_error);
          inbox_refresh_pending = true;
          inbox_refresh_needs_paint = true;
          detail_dirty = true;
          detail_scroll = 0;
          dirty = true;
          continue;
        }
        std::string note{};
        bool cancelled = false;
        if (!prompt_text_dialog(
                " Acknowledge review ",
                "This does not continue the session. It records a signed "
                "acknowledgment and clears the review report from the queue "
                "after review. Provide a required acknowledgment message:",
                &note, false, false, &cancelled)) {
          status = "failed to collect review acknowledgment note";
          status_is_error = true;
          dirty = true;
          continue;
        }
        if (cancelled) {
          status = "Cancelled.";
          status_is_error = false;
          dirty = true;
          continue;
        }
        std::string structured{};
        std::string ack_error{};
        if (!acknowledge_session_summary(app, visible[selected], note,
                                         &structured, &ack_error)) {
          status = ack_error;
          status_is_error = true;
          dirty = true;
          continue;
        }
        status = "Acknowledged review report. Refreshing operator inbox...";
        status_is_error = false;
        inbox_refresh_pending = true;
        inbox_refresh_needs_paint = true;
        detail_dirty = true;
        detail_scroll = 0;
        dirty = true;
        continue;
      }
    }
  } catch (const std::exception &ex) {
    if (error)
      *error = std::string(kNcursesInitErrorPrefix) + ex.what();
    return false;
  }
}
