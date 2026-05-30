[[nodiscard]] bool read_request_text_for_session(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string *out, std::string *error) {
  if (!out) {
    if (error)
      *error = "request text output pointer is null";
    return false;
  }
  return cuwacunu::hero::runtime::read_text_file(session.human_request_path,
                                                 out, error);
}

[[nodiscard]] std::string build_session_console_detail_text(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool has_pending_review) {
  std::ostringstream out;
  out << "objective: " << session.objective_name << "\n"
      << "marshal_session_id: " << session.marshal_session_id << "\n"
      << "operator_state: "
      << operator_session_state_detail(session, has_pending_review) << "\n"
      << "next_action: "
      << operator_session_action_hint(session, has_pending_review) << "\n"
      << "lifecycle: " << session.lifecycle;
  if (session.lifecycle == "live" && session.work_gate != "open") {
    out << " (" << session.work_gate << ")";
  } else if (session.lifecycle == "terminal") {
    out << " (" << session.finish_reason << ")";
  }
  out << "\n"
      << "effort: " << build_summary_effort_text(session) << "\n"
      << "authority_scope: " << session.authority_scope << "\n";
  if (!session.status_detail.empty()) {
    out << "status_detail: " << session.status_detail << "\n";
  }
  if (!session.last_codex_action.empty()) {
    out << "last_intent: " << session.last_codex_action << "\n";
  }
  if (!session.campaign_cursor.empty()) {
    out << "active_campaign: " << session.campaign_cursor << "\n";
  }
  if (!session.last_warning.empty()) {
    out << "last_warning: " << session.last_warning << "\n";
  }
  if (!session.current_thread_id.empty()) {
    out << "current_thread_id: " << session.current_thread_id << "\n";
  }
  if (!session.campaign_dsl_path.empty()) {
    out << "campaign_dsl: " << session.campaign_dsl_path << "\n";
  }
  if (!session.campaign_cursors.empty()) {
    out << "campaign_history: ";
    for (std::size_t i = 0; i < session.campaign_cursors.size(); ++i) {
      if (i != 0)
        out << ", ";
      out << session.campaign_cursors[i];
    }
    out << "\n";
  }

  if (session.lifecycle == "live" &&
      session.work_gate != "open" &&
      is_human_request_work_gate(session.work_gate)) {
    std::string request_text{};
    std::string request_error{};
    out << "\n--- Human Request ---\n";
    if (read_request_text_for_session(session, &request_text, &request_error)) {
      out << request_text;
    } else {
      out << "<failed to read request: " << request_error << ">";
    }
  }

  if (cuwacunu::hero::marshal::is_marshal_session_summary_state(
          session)) {
    std::string summary_text{};
    std::string summary_error{};
    out << "\n\n--- Session Summary ---\n";
    if (read_summary_text_for_session(session, &summary_text, &summary_error)) {
      out << summary_text;
    } else {
      out << "<failed to read summary: " << summary_error << ">";
    }
  }
  return out.str();
}

void render_human_sessions_screen(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        &all_sessions,
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        &pending_reviews,
    std::size_t request_count, std::size_t summary_count,
    operator_console_state_filter_t state_filter, std::string_view operator_id,
    std::string_view marshal_root, std::size_t selected,
    std::string_view detail_text, std::size_t detail_scroll,
    std::size_t *out_detail_total_lines, std::size_t *out_detail_page_lines,
    std::size_t *out_actual_detail_scroll,
    detail_viewport_t *out_detail_viewport, std::string_view status_line,
    bool status_is_error) {
  if (out_detail_total_lines)
    *out_detail_total_lines = 0;
  if (out_detail_page_lines)
    *out_detail_page_lines = 0;
  if (out_actual_detail_scroll)
    *out_actual_detail_scroll = 0;
  if (out_detail_viewport)
    *out_detail_viewport = detail_viewport_t{};

  const int W = COLS;
  const int H = LINES;
  if (W < 56 || H < 12) {
    erase();
    mvaddnstr(
        0, 0,
        "Human Hero: terminal too small. Resize or use hero.human.* tools.",
        std::max(0, W - 1));
    refresh();
    return;
  }

  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> sessions{};
  filter_sessions_for_console(all_sessions, state_filter, &sessions);

  const int header_h = 4;
  const int footer_h = 3;
  const int body_h = H - header_h - footer_h;
  const int left_w = std::clamp(W / 3, 28, std::max(28, W - 24));
  const int right_w = W - left_w;

  if (has_colors())
    attrset(COLOR_PAIR(6));
  erase();

  draw_boxed_region(0, 0, header_h, W, " Human Hero ");
  attron(COLOR_PAIR(1) | A_BOLD);
  const std::string header_line =
      "Operator session console | tracked: " +
      std::to_string(all_sessions.size()) +
      " | requests: " + std::to_string(request_count) +
      " | reviews: " + std::to_string(summary_count) + " | lane: " +
      std::string(
          operator_console_view_label(operator_console_view_t::sessions)) +
      " | overview filter: " +
      std::string(operator_console_state_filter_label(state_filter)) +
      " | operator: " +
      std::string(operator_id.empty() ? "<unset>" : operator_id);
  mvaddnstr(1, 2, ellipsize_text(header_line, W - 4).c_str(), W - 4);
  attroff(COLOR_PAIR(1) | A_BOLD);
  draw_status_line(2, 2, W - 4, status_line, status_is_error);

  draw_boxed_region(header_h, 0, body_h, left_w,
                    sessions.empty() ? " Overview " : " Tracked Sessions ");
  if (all_sessions.empty()) {
    draw_centered_region_line(header_h + 2, 0, left_w, "No Sessions", 5, true);
    draw_centered_region_line(
        header_h + 4, 0, left_w,
        "Start a Marshal Hero session to populate this console.");
    draw_centered_region_line(header_h + 6, 0, left_w, "Press r to refresh.");
    draw_centered_region_line(header_h + 7, 0, left_w,
                              "Press q to leave the console.");
  } else if (sessions.empty()) {
    draw_centered_region_line(header_h + 2, 0, left_w, "No Matching Sessions",
                              3, true);
    draw_centered_region_line(
        header_h + 4, 0, left_w,
        "The current state filter hides all known sessions.");
    draw_centered_region_line(header_h + 6, 0, left_w,
                              "Press f to cycle the overview filter.");
    draw_centered_region_line(header_h + 7, 0, left_w,
                              "Press Tab to cycle lanes.");
  } else {
    const int max_rows = body_h - 2;
    std::size_t top = 0;
    if (selected >= static_cast<std::size_t>(max_rows)) {
      top = selected - static_cast<std::size_t>(max_rows) + 1;
    }
    for (int row = 0; row < max_rows; ++row) {
      const std::size_t idx = top + static_cast<std::size_t>(row);
      if (idx >= sessions.size())
        break;
      const auto &session = sessions[idx];
      const bool has_pending_review =
          find_session_index_by_id(pending_reviews, session.marshal_session_id)
              .has_value();
      const std::string label =
          session_state_badge(session, has_pending_review) + " " +
                                session.objective_name + " | " +
                                session.marshal_session_id;
      const int row_y = header_h + 1 + row;
      clear_region_line(row_y, 1, left_w - 2);
      const int accent_color = session_accent_color(session);
      if (idx == selected) {
        attron(COLOR_PAIR(2) | A_BOLD);
      } else if (accent_color > 0) {
        attron(COLOR_PAIR(accent_color) | A_BOLD);
      }
      mvaddnstr(row_y, 2, ellipsize_text(label, left_w - 4).c_str(),
                left_w - 4);
      if (idx == selected) {
        attroff(COLOR_PAIR(2) | A_BOLD);
      } else if (accent_color > 0) {
        attroff(COLOR_PAIR(accent_color) | A_BOLD);
      }
    }
  }

  draw_boxed_region(header_h, left_w, body_h, right_w,
                    sessions.empty() ? " Overview " : " Session Detail ");
  if (!sessions.empty()) {
    const auto &session = sessions[selected];
    const bool has_pending_review =
        find_session_index_by_id(pending_reviews, session.marshal_session_id)
            .has_value();
    const int accent_color = session_accent_color(session);
    std::ostringstream meta;
    meta << "session=" << session.marshal_session_id
         << "  state="
         << operator_session_state_detail(session, has_pending_review);
    meta << "  lifecycle=" << session.lifecycle;
    if (session.lifecycle == "live" && session.work_gate != "open")
      meta << "(" << session.work_gate << ")";
    if (session.lifecycle == "terminal")
      meta << "(" << session.finish_reason << ")";
    attron(COLOR_PAIR(accent_color) | A_BOLD);
    mvaddnstr(header_h + 1, left_w + 2,
              ellipsize_text(meta.str(), right_w - 4).c_str(), right_w - 4);
    attroff(COLOR_PAIR(accent_color) | A_BOLD);
    const std::string subtitle =
        "running=" +
        std::to_string(count_sessions_in_operator_state(
            all_sessions, operator_session_state_t::running)) +
        " waiting_clarification=" +
        std::to_string(count_sessions_in_operator_state(
            all_sessions, operator_session_state_t::waiting_clarification)) +
        " waiting_governance=" +
        std::to_string(count_sessions_in_operator_state(
            all_sessions, operator_session_state_t::waiting_governance)) +
        " review=" +
        std::to_string(count_sessions_in_operator_state(
            all_sessions, operator_session_state_t::review)) +
        " done=" +
        std::to_string(count_sessions_in_operator_state(
            all_sessions, operator_session_state_t::done));
    mvaddnstr(header_h + 2, left_w + 2,
              ellipsize_text(subtitle, right_w - 4).c_str(), right_w - 4);
    const int detail_x = left_w + 2;
    const int detail_y = header_h + 4;
    const int detail_width = std::max(1, right_w - 5);
    const int detail_height = std::max(1, body_h - 7);
    const int scrollbar_x = detail_x + detail_width + 1;
    std::size_t actual_detail_scroll = 0;
    const std::size_t detail_total_lines = draw_wrapped_region_block_scrolled(
        detail_y, detail_x, detail_width, detail_height, detail_text,
        detail_scroll, &actual_detail_scroll);
    draw_vertical_scrollbar(
        detail_y, scrollbar_x, detail_height, actual_detail_scroll,
        static_cast<std::size_t>(detail_height), detail_total_lines);
    draw_detail_scroll_indicator(
        header_h + body_h - 2, left_w + 2, right_w - 4, actual_detail_scroll,
        static_cast<std::size_t>(detail_height), detail_total_lines);
    if (out_detail_total_lines)
      *out_detail_total_lines = detail_total_lines;
    if (out_detail_page_lines) {
      *out_detail_page_lines = static_cast<std::size_t>(detail_height);
    }
    if (out_actual_detail_scroll)
      *out_actual_detail_scroll = actual_detail_scroll;
    if (out_detail_viewport) {
      *out_detail_viewport = detail_viewport_t{
          .x = detail_x,
          .y = detail_y,
          .width = detail_width,
          .height = detail_height,
          .scrollbar_x = scrollbar_x,
          .top_line = actual_detail_scroll,
          .page_lines = static_cast<std::size_t>(detail_height),
          .total_lines = detail_total_lines};
    }
  } else {
    std::ostringstream out;
    out << "This is the overview lane.\n\n"
        << "Operator: "
        << (operator_id.empty() ? "<unset>" : std::string(operator_id)) << "\n"
        << "Marshal root: " << marshal_root << "\n\n"
        << "What to do here:\n"
        << "- inspect every known session, not only pending operator work\n"
        << "- use f to cycle overview filters\n"
        << "- use Tab to cycle between overview, requests, and reviews\n"
        << "- use pause/resume/continue/terminate controls from the cockpit\n\n"
        << "Non-interactive alternative:\n"
        << "hero.marshal.list_sessions";
    draw_wrapped_region_block(header_h + 2, left_w + 2, right_w - 4, body_h - 4,
                              out.str());
  }

  draw_boxed_region(header_h + body_h, 0, footer_h, W, " Controls ");
  std::string controls =
      sessions.empty()
          ? "Tab cycle lanes  f cycle filter  r refresh  q quit"
          : "Tab cycle lanes  f cycle filter  Up/Down select  Wheel/PgUp/PgDn "
            "scroll detail  Home/End jump  Enter open request/review  p pause  "
            "u resume  c continue  t terminate  r refresh  q quit";
  mvaddnstr(header_h + body_h + 1, 2, ellipsize_text(controls, W - 4).c_str(),
            W - 4);

  refresh();
}

void render_human_requests_screen(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        &pending,
    std::size_t summary_count, std::string_view operator_id,
    std::string_view marshal_root, std::size_t selected,
    std::string_view request_text, std::size_t detail_scroll,
    std::size_t *out_detail_total_lines, std::size_t *out_detail_page_lines,
    std::size_t *out_actual_detail_scroll,
    detail_viewport_t *out_detail_viewport, std::string_view status_line,
    bool status_is_error) {
  if (out_detail_total_lines)
    *out_detail_total_lines = 0;
  if (out_detail_page_lines)
    *out_detail_page_lines = 0;
  if (out_actual_detail_scroll)
    *out_actual_detail_scroll = 0;
  if (out_detail_viewport)
    *out_detail_viewport = detail_viewport_t{};
  const int W = COLS;
  const int H = LINES;
  if (W < 56 || H < 12) {
    erase();
    mvaddnstr(
        0, 0,
        "Human Hero: terminal too small. Resize or use hero.human.* tools.",
        std::max(0, W - 1));
    refresh();
    return;
  }
  const int header_h = 4;
  const int footer_h = 3;
  const int body_h = H - header_h - footer_h;
  const int left_w = std::clamp(W / 3, 28, std::max(28, W - 24));
  const int right_w = W - left_w;

  if (has_colors())
    attrset(COLOR_PAIR(6));
  erase();

  draw_boxed_region(0, 0, header_h, W, " Human Hero ");
  attron(COLOR_PAIR(1) | A_BOLD);
  const std::string header_line =
      "Operator session console | requests: " + std::to_string(pending.size()) +
      " | reviews: " + std::to_string(summary_count) + " | lane: " +
      std::string(
          operator_console_view_label(operator_console_view_t::requests)) +
      " | operator: " +
      std::string(operator_id.empty() ? "<unset>" : operator_id);
  mvaddnstr(1, 2, ellipsize_text(header_line, W - 4).c_str(), W - 4);
  attroff(COLOR_PAIR(1) | A_BOLD);
  draw_status_line(2, 2, W - 4, status_line, status_is_error);

  draw_boxed_region(header_h, 0, body_h, left_w,
                    pending.empty() ? " Requests " : " Pending Requests ");
  if (pending.empty()) {
    draw_centered_region_line(header_h + 2, 0, left_w, "No Pending Requests", 5,
                              true);
    draw_centered_region_line(
        header_h + 4, 0, left_w,
        "Marshal Hero has no pending clarification or governance request.");
    draw_centered_region_line(header_h + 6, 0, left_w,
                              summary_count == 0
                                  ? "Press r to refresh the lane."
                                  : "Press Tab to cycle lanes.");
    draw_centered_region_line(header_h + 7, 0, left_w,
                              "Press q to leave the console.");
  } else {
    const int max_rows = body_h - 2;
    std::size_t top = 0;
    if (selected >= static_cast<std::size_t>(max_rows)) {
      top = selected - static_cast<std::size_t>(max_rows) + 1;
    }
    for (int row = 0; row < max_rows; ++row) {
      const std::size_t idx = top + static_cast<std::size_t>(row);
      if (idx >= pending.size())
        break;
      const auto &session = pending[idx];
      const std::string label = session_state_badge(session, false) + " " +
                                session.objective_name + " | " +
                                session.marshal_session_id;
      const int row_y = header_h + 1 + row;
      clear_region_line(row_y, 1, left_w - 2);
      const int accent_color = session_accent_color(session);
      if (idx == selected) {
        attron(COLOR_PAIR(2) | A_BOLD);
      } else if (accent_color > 0) {
        attron(COLOR_PAIR(accent_color) | A_BOLD);
      }
      mvaddnstr(row_y, 2, ellipsize_text(label, left_w - 4).c_str(),
                left_w - 4);
      if (idx == selected) {
        attroff(COLOR_PAIR(2) | A_BOLD);
      } else if (accent_color > 0) {
        attroff(COLOR_PAIR(accent_color) | A_BOLD);
      }
    }
  }

  draw_boxed_region(header_h, left_w, body_h, right_w,
                    pending.empty() ? " Overview " : " Session Request ");
  if (!pending.empty()) {
    const auto &session = pending[selected];
    const int accent_color = session_accent_color(session);
    std::string meta =
        "session=" + session.marshal_session_id +
        "  checkpoint=" + std::to_string(session.checkpoint_count);
    attron(COLOR_PAIR(accent_color) | A_BOLD);
    mvaddnstr(header_h + 1, left_w + 2,
              ellipsize_text(meta, right_w - 4).c_str(), right_w - 4);
    attroff(COLOR_PAIR(accent_color) | A_BOLD);
    mvaddnstr(header_h + 2, left_w + 2,
              ellipsize_text(operator_session_state_detail(session, false) + " | " +
                                 (session.status_detail.empty()
                                      ? session.objective_name
                                      : session.status_detail),
                             right_w - 4)
                  .c_str(),
              right_w - 4);
    const int detail_x = left_w + 2;
    const int detail_y = header_h + 4;
    const int detail_width = std::max(1, right_w - 5);
    const int detail_height = std::max(1, body_h - 7);
    const int scrollbar_x = detail_x + detail_width + 1;
    std::size_t actual_detail_scroll = 0;
    const std::size_t detail_total_lines = draw_wrapped_region_block_scrolled(
        detail_y, detail_x, detail_width, detail_height, request_text,
        detail_scroll, &actual_detail_scroll);
    draw_vertical_scrollbar(
        detail_y, scrollbar_x, detail_height, actual_detail_scroll,
        static_cast<std::size_t>(detail_height), detail_total_lines);
    draw_detail_scroll_indicator(
        header_h + body_h - 2, left_w + 2, right_w - 4, actual_detail_scroll,
        static_cast<std::size_t>(detail_height), detail_total_lines);
    if (out_detail_total_lines)
      *out_detail_total_lines = detail_total_lines;
    if (out_detail_page_lines) {
      *out_detail_page_lines = static_cast<std::size_t>(detail_height);
    }
    if (out_actual_detail_scroll)
      *out_actual_detail_scroll = actual_detail_scroll;
    if (out_detail_viewport) {
      *out_detail_viewport = detail_viewport_t{
          .x = detail_x,
          .y = detail_y,
          .width = detail_width,
          .height = detail_height,
          .scrollbar_x = scrollbar_x,
          .top_line = actual_detail_scroll,
          .page_lines = static_cast<std::size_t>(detail_height),
          .total_lines = detail_total_lines};
    }
  } else {
    std::ostringstream out;
    out << "This lane becomes active when a Marshal Hero session pauses for "
           "clarification or governance.\n\n"
        << "Operator: "
        << (operator_id.empty() ? "<unset>" : std::string(operator_id)) << "\n"
        << "Marshal root: " << marshal_root << "\n\n"
        << "What to do here:\n"
        << "- wait for a paused session to appear\n"
        << "- press Tab to cycle lanes when overview or reviews are relevant\n"
        << "- press r to refresh\n"
        << "- press q to exit\n\n"
        << "Non-interactive alternative:\n"
        << "hero.human.list_requests";
    draw_wrapped_region_block(header_h + 2, left_w + 2, right_w - 4, body_h - 4,
                              out.str());
  }

  draw_boxed_region(header_h + body_h, 0, footer_h, W, " Controls ");
  std::string controls =
      pending.empty()
          ? "r refresh  q quit"
          : "Up/Down select  Wheel/PgUp/PgDn scroll detail  Home/End jump  "
            "Enter inspect/respond  r refresh  q quit";
  if (summary_count > 0)
    controls = "Tab cycle lanes  " + controls;
  mvaddnstr(header_h + body_h + 1, 2, ellipsize_text(controls, W - 4).c_str(),
            W - 4);

  refresh();
}

void render_human_reviews_screen(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        &pending,
    std::size_t request_count, std::string_view operator_id,
    std::string_view marshal_root, std::size_t selected,
    std::string_view summary_text, std::size_t detail_scroll,
    std::size_t *out_detail_total_lines, std::size_t *out_detail_page_lines,
    std::size_t *out_actual_detail_scroll,
    detail_viewport_t *out_detail_viewport, std::string_view status_line,
    bool status_is_error) {
  if (out_detail_total_lines)
    *out_detail_total_lines = 0;
  if (out_detail_page_lines)
    *out_detail_page_lines = 0;
  if (out_actual_detail_scroll)
    *out_actual_detail_scroll = 0;
  if (out_detail_viewport)
    *out_detail_viewport = detail_viewport_t{};
  const int W = COLS;
  const int H = LINES;
  if (W < 56 || H < 12) {
    erase();
    mvaddnstr(
        0, 0,
        "Human Hero: terminal too small. Resize or use hero.human.* tools.",
        std::max(0, W - 1));
    refresh();
    return;
  }
  const int header_h = 4;
  const int footer_h = 3;
  const int body_h = H - header_h - footer_h;
  const int left_w = std::clamp(W / 3, 28, std::max(28, W - 24));
  const int right_w = W - left_w;

  if (has_colors())
    attrset(COLOR_PAIR(6));
  erase();

  draw_boxed_region(0, 0, header_h, W, " Human Hero ");
  attron(COLOR_PAIR(1) | A_BOLD);
  const std::string header_line =
      "Operator session console | requests: " + std::to_string(request_count) +
      " | reviews: " + std::to_string(pending.size()) + " | lane: " +
      std::string(
          operator_console_view_label(operator_console_view_t::summaries)) +
      " | operator: " +
      std::string(operator_id.empty() ? "<unset>" : operator_id);
  mvaddnstr(1, 2, ellipsize_text(header_line, W - 4).c_str(), W - 4);
  attroff(COLOR_PAIR(1) | A_BOLD);
  draw_status_line(2, 2, W - 4, status_line, status_is_error);

  draw_boxed_region(header_h, 0, body_h, left_w,
                    pending.empty() ? " Reviews " : " Pending Reviews ");
  if (pending.empty()) {
    draw_centered_region_line(header_h + 2, 0, left_w, "No Pending Reviews", 5,
                              true);
    draw_centered_region_line(
        header_h + 4, 0, left_w,
        "Idle and finished Marshal Hero review reports appear here.");
    draw_centered_region_line(header_h + 6, 0, left_w,
                              request_count == 0
                                  ? "Press r to refresh the lane."
                                  : "Press Tab to cycle lanes.");
    draw_centered_region_line(header_h + 7, 0, left_w,
                              "Press q to leave the console.");
  } else {
    const int max_rows = body_h - 2;
    std::size_t top = 0;
    if (selected >= static_cast<std::size_t>(max_rows)) {
      top = selected - static_cast<std::size_t>(max_rows) + 1;
    }
    for (int row = 0; row < max_rows; ++row) {
      const std::size_t idx = top + static_cast<std::size_t>(row);
      if (idx >= pending.size())
        break;
      const auto &session = pending[idx];
      const std::string label = session_state_badge(session, true) + " " +
                                session.objective_name + " | " +
                                session.marshal_session_id;
      const int row_y = header_h + 1 + row;
      clear_region_line(row_y, 1, left_w - 2);
      const int accent_color = session_accent_color(session);
      if (idx == selected) {
        attron(COLOR_PAIR(2) | A_BOLD);
      } else if (accent_color > 0) {
        attron(COLOR_PAIR(accent_color) | A_BOLD);
      }
      mvaddnstr(row_y, 2, ellipsize_text(label, left_w - 4).c_str(),
                left_w - 4);
      if (idx == selected) {
        attroff(COLOR_PAIR(2) | A_BOLD);
      } else if (accent_color > 0) {
        attroff(COLOR_PAIR(accent_color) | A_BOLD);
      }
    }
  }

  draw_boxed_region(header_h, left_w, body_h, right_w,
                    pending.empty() ? " Overview " : " Review Report ");
  if (!pending.empty()) {
    const auto &session = pending[selected];
    const int accent_color = session_accent_color(session);
    std::string meta = "review report | session=" + session.marshal_session_id +
                       "  finish_reason=" + session.finish_reason;
    attron(COLOR_PAIR(accent_color) | A_BOLD);
    mvaddnstr(header_h + 1, left_w + 2,
              ellipsize_text(meta, right_w - 4).c_str(), right_w - 4);
    attroff(COLOR_PAIR(accent_color) | A_BOLD);
    const std::string subtitle = operator_session_state_detail(session, true) +
                                 " | " + build_summary_effort_text(session);
    mvaddnstr(header_h + 2, left_w + 2,
              ellipsize_text(subtitle, right_w - 4).c_str(), right_w - 4);
    const int detail_x = left_w + 2;
    const int detail_y = header_h + 4;
    const int detail_width = std::max(1, right_w - 5);
    const int detail_height = std::max(1, body_h - 7);
    const int scrollbar_x = detail_x + detail_width + 1;
    std::size_t actual_detail_scroll = 0;
    const std::size_t detail_total_lines = draw_wrapped_region_block_scrolled(
        detail_y, detail_x, detail_width, detail_height, summary_text,
        detail_scroll, &actual_detail_scroll);
    draw_vertical_scrollbar(
        detail_y, scrollbar_x, detail_height, actual_detail_scroll,
        static_cast<std::size_t>(detail_height), detail_total_lines);
    draw_detail_scroll_indicator(
        header_h + body_h - 2, left_w + 2, right_w - 4, actual_detail_scroll,
        static_cast<std::size_t>(detail_height), detail_total_lines);
    if (out_detail_total_lines)
      *out_detail_total_lines = detail_total_lines;
    if (out_detail_page_lines) {
      *out_detail_page_lines = static_cast<std::size_t>(detail_height);
    }
    if (out_actual_detail_scroll)
      *out_actual_detail_scroll = actual_detail_scroll;
    if (out_detail_viewport) {
      *out_detail_viewport = detail_viewport_t{
          .x = detail_x,
          .y = detail_y,
          .width = detail_width,
          .height = detail_height,
          .scrollbar_x = scrollbar_x,
          .top_line = actual_detail_scroll,
          .page_lines = static_cast<std::size_t>(detail_height),
          .total_lines = detail_total_lines};
    }
  } else {
    std::ostringstream out;
    out << "This lane tracks review-ready and terminal Marshal Hero review "
           "reports.\n\n"
        << "Operator: "
        << (operator_id.empty() ? "<unset>" : std::string(operator_id)) << "\n"
        << "Marshal root: " << marshal_root << "\n\n"
        << "What to do here:\n"
        << "- inspect the review report\n"
        << "- message review-ready sessions with fresh operator guidance when "
           "you want more work to launch\n"
        << "- acknowledge a report only after review; that signs it off with a "
           "required message and clears it from the queue\n"
        << "- terminal sessions are informational; review-ready sessions are "
           "messageable\n"
        << "- press Tab to cycle lanes when overview or requests are relevant\n"
        << "- press r to refresh\n"
        << "- press q to exit\n\n"
        << "Non-interactive alternative:\n"
        << "hero.human.list_summaries";
    draw_wrapped_region_block(header_h + 2, left_w + 2, right_w - 4, body_h - 4,
                              out.str());
  }

  draw_boxed_region(header_h + body_h, 0, footer_h, W, " Controls ");
  std::string controls =
      pending.empty() ? "r refresh  q quit"
                      : "Up/Down select  Wheel/PgUp/PgDn scroll detail  "
                        "Home/End jump  Enter inspect  m message session  a "
                        "acknowledge review  r refresh  q quit";
  if (request_count > 0)
    controls = "Tab cycle lanes  " + controls;
  mvaddnstr(header_h + body_h + 1, 2, ellipsize_text(controls, W - 4).c_str(),
            W - 4);

  refresh();
}

void render_human_requests_bootstrap_screen(std::string_view operator_id,
                                            std::string_view marshal_root,
                                            std::string_view status_line,
                                            bool status_is_error) {
  const int W = COLS;
  const int H = LINES;
  if (W < 56 || H < 10) {
    erase();
    mvaddnstr(
        0, 0,
        "Human Hero: terminal too small. Resize or use hero.human.* tools.",
        std::max(0, W - 1));
    refresh();
    return;
  }

  const int header_h = 3;
  const int footer_h = 3;
  const int body_h = std::max(5, H - header_h - footer_h);

  if (has_colors())
    attrset(COLOR_PAIR(6));
  erase();
  draw_boxed_region(0, 0, header_h, W, " Human Hero ");
  attron(COLOR_PAIR(1) | A_BOLD);
  mvaddnstr(1, 2, "Operator console bootstrap", W - 4);
  attroff(COLOR_PAIR(1) | A_BOLD);

  draw_boxed_region(header_h, 0, body_h, W, " Boot ");
  draw_wrapped_region_block(
      header_h + 2, 2, W - 4, body_h - 4,
      std::string("Initializing operator console...\n\n"
                  "[work] scanning Marshal Hero session console\n\n") +
          "operator: " +
          std::string(operator_id.empty() ? "<unset>" : operator_id) + "\n" +
          "marshal_root: " + std::string(marshal_root) +
          "\n\n"
          "The operator console will appear when the scan completes.");

  draw_boxed_region(header_h + body_h, 0, footer_h, W, " Status ");
  attron(COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);
  mvaddnstr(header_h + body_h + 1, 2,
            ellipsize_text(status_line, W - 4).c_str(), W - 4);
  attroff(COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);

  refresh();
}

