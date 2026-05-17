[[nodiscard]] std::string ellipsize_text(std::string_view in, int width) {
  if (width <= 0)
    return {};
  if (static_cast<int>(in.size()) <= width)
    return std::string(in);
  if (width <= 3)
    return std::string(in.substr(0, static_cast<std::size_t>(width)));
  return std::string(in.substr(0, static_cast<std::size_t>(width - 3))) + "...";
}

[[nodiscard]] std::vector<std::string> wrap_text_lines(std::string_view text,
                                                       int width) {
  std::vector<std::string> lines{};
  if (width <= 1) {
    lines.emplace_back();
    return lines;
  }
  std::istringstream input{std::string(text)};
  std::string raw{};
  while (std::getline(input, raw)) {
    if (raw.empty()) {
      lines.emplace_back();
      continue;
    }
    std::istringstream words(raw);
    std::string word{};
    std::string current{};
    while (words >> word) {
      while (static_cast<int>(word.size()) > width) {
        if (!current.empty()) {
          lines.push_back(current);
          current.clear();
        }
        lines.push_back(word.substr(0, static_cast<std::size_t>(width)));
        word.erase(0, static_cast<std::size_t>(width));
      }
      if (current.empty()) {
        current = word;
        continue;
      }
      if (static_cast<int>(current.size() + 1 + word.size()) <= width) {
        current.append(" ").append(word);
      } else {
        lines.push_back(current);
        current = word;
      }
    }
    if (!current.empty())
      lines.push_back(current);
  }
  if (lines.empty())
    lines.emplace_back();
  return lines;
}

[[nodiscard]] std::vector<std::string> wrap_input_lines(std::string_view text,
                                                        int width) {
  std::vector<std::string> lines{};
  if (width <= 0) {
    lines.emplace_back();
    return lines;
  }
  lines.emplace_back();
  for (char ch : text) {
    if (ch == '\n') {
      lines.emplace_back();
      continue;
    }
    if (static_cast<int>(lines.back().size()) >= width) {
      lines.emplace_back();
    }
    lines.back().push_back(ch);
  }
  if (lines.empty())
    lines.emplace_back();
  return lines;
}

void init_human_ncurses_theme() {
  if (!has_colors())
    return;
  use_default_colors();
  init_pair(1, COLOR_CYAN, -1);
  init_pair(2, COLOR_WHITE, COLOR_CYAN);
  init_pair(3, COLOR_YELLOW, -1);
  init_pair(4, COLOR_RED, -1);
  init_pair(5, COLOR_GREEN, -1);
  init_pair(6, COLOR_WHITE, -1);
  init_pair(7, COLOR_CYAN, -1);
  init_pair(8, COLOR_YELLOW, -1);
  init_pair(9, COLOR_GREEN, -1);
  init_pair(10, COLOR_MAGENTA, -1);
  init_pair(11, COLOR_BLUE, -1);
  init_pair(12, COLOR_WHITE, COLOR_BLUE);
  init_pair(13, COLOR_YELLOW, COLOR_BLUE);
  bkgd(COLOR_PAIR(6) | ' ');
  attrset(COLOR_PAIR(6));
}

void ensure_human_dialog_colors() {
  if (!has_colors())
    return;
  use_default_colors();
  init_pair(1, COLOR_CYAN, -1);
  init_pair(2, COLOR_WHITE, COLOR_CYAN);
  init_pair(3, COLOR_YELLOW, -1);
  init_pair(4, COLOR_RED, -1);
  init_pair(5, COLOR_GREEN, -1);
  init_pair(6, COLOR_WHITE, -1);
  init_pair(7, COLOR_CYAN, -1);
  init_pair(8, COLOR_YELLOW, -1);
  init_pair(9, COLOR_GREEN, -1);
  init_pair(10, COLOR_MAGENTA, -1);
  init_pair(11, COLOR_BLUE, -1);
  init_pair(12, COLOR_WHITE, COLOR_BLUE);
  init_pair(13, COLOR_YELLOW, COLOR_BLUE);
}

void draw_boxed_window(WINDOW *win, std::string_view title) {
  if (!win)
    return;
  if (has_colors()) {
    wbkgdset(win, COLOR_PAIR(6) | ' ');
    wattrset(win, COLOR_PAIR(6));
  }
  werase(win);
  box(win, 0, 0);
  if (!title.empty()) {
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    mvwaddnstr(win, 0, 2, title.data(), static_cast<int>(title.size()));
    wattroff(win, COLOR_PAIR(1) | A_BOLD);
  }
}

void draw_boxed_subwindow_region(WINDOW *win, int y, int x, int height,
                                 int width, std::string_view title,
                                 int border_pair, int fill_pair) {
  if (!win || height < 2 || width < 2)
    return;
  for (int row = 0; row < height; ++row) {
    if (fill_pair > 0)
      wattrset(win, COLOR_PAIR(fill_pair));
    mvwaddnstr(win, y + row, x, std::string(width, ' ').c_str(), width);
  }
  if (border_pair > 0)
    wattrset(win, COLOR_PAIR(border_pair));
  mvwaddch(win, y, x, ACS_ULCORNER);
  mvwhline(win, y, x + 1, ACS_HLINE, width - 2);
  mvwaddch(win, y, x + width - 1, ACS_URCORNER);
  for (int row = 1; row < height - 1; ++row) {
    mvwaddch(win, y + row, x, ACS_VLINE);
    mvwaddch(win, y + row, x + width - 1, ACS_VLINE);
  }
  mvwaddch(win, y + height - 1, x, ACS_LLCORNER);
  mvwhline(win, y + height - 1, x + 1, ACS_HLINE, width - 2);
  mvwaddch(win, y + height - 1, x + width - 1, ACS_LRCORNER);
  if (!title.empty() && width > 4) {
    wattron(win, COLOR_PAIR(border_pair) | A_BOLD);
    mvwaddnstr(win, y, x + 2, title.data(), width - 4);
    wattroff(win, COLOR_PAIR(border_pair) | A_BOLD);
  }
  if (has_colors()) {
    wattrset(win, COLOR_PAIR(6));
  } else {
    wattrset(win, A_NORMAL);
  }
}

void draw_boxed_region(int y, int x, int height, int width,
                       std::string_view title) {
  if (height < 2 || width < 2)
    return;
  mvaddch(y, x, ACS_ULCORNER);
  mvhline(y, x + 1, ACS_HLINE, width - 2);
  mvaddch(y, x + width - 1, ACS_URCORNER);
  for (int row = 1; row < height - 1; ++row) {
    mvaddch(y + row, x, ACS_VLINE);
    mvaddch(y + row, x + width - 1, ACS_VLINE);
  }
  mvaddch(y + height - 1, x, ACS_LLCORNER);
  mvhline(y + height - 1, x + 1, ACS_HLINE, width - 2);
  mvaddch(y + height - 1, x + width - 1, ACS_LRCORNER);
  if (!title.empty() && width > 4) {
    attron(COLOR_PAIR(1) | A_BOLD);
    mvaddnstr(y, x + 2, title.data(), width - 4);
    attroff(COLOR_PAIR(1) | A_BOLD);
  }
}

void clear_region_line(int y, int x, int width) {
  if (width <= 0)
    return;
  mvaddnstr(y, x, std::string(static_cast<std::size_t>(width), ' ').c_str(),
            width);
}

void draw_wrapped_region_block(int y, int x, int width, int height,
                               std::string_view text) {
  if (width <= 0 || height <= 0)
    return;
  const auto lines = wrap_text_lines(text, width);
  for (int i = 0; i < height; ++i)
    clear_region_line(y + i, x, width);
  for (int i = 0; i < height && i < static_cast<int>(lines.size()); ++i) {
    mvaddnstr(y + i, x, lines[static_cast<std::size_t>(i)].c_str(), width);
  }
}

[[nodiscard]] std::size_t
draw_wrapped_region_block_scrolled(int y, int x, int width, int height,
                                   std::string_view text, std::size_t top_line,
                                   std::size_t *out_actual_top_line = nullptr) {
  if (out_actual_top_line)
    *out_actual_top_line = 0;
  if (width <= 0 || height <= 0)
    return 0;
  const auto lines = wrap_text_lines(text, width);
  const std::size_t total_lines = lines.size();
  const std::size_t max_top_line =
      total_lines > static_cast<std::size_t>(height)
          ? total_lines - static_cast<std::size_t>(height)
          : 0u;
  const std::size_t actual_top_line = std::min(top_line, max_top_line);
  if (out_actual_top_line)
    *out_actual_top_line = actual_top_line;
  for (int i = 0; i < height; ++i)
    clear_region_line(y + i, x, width);
  for (int i = 0; i < height; ++i) {
    const std::size_t src = actual_top_line + static_cast<std::size_t>(i);
    if (src >= total_lines)
      break;
    mvaddnstr(y + i, x, lines[src].c_str(), width);
  }
  return total_lines;
}

void draw_centered_region_line(int y, int x, int width, std::string_view text,
                               int color_pair = 0, bool bold = false) {
  if (width <= 0)
    return;
  const std::string shown = ellipsize_text(text, std::max(1, width - 2));
  const int line_x =
      x + std::max(1, (width - static_cast<int>(shown.size())) / 2);
  if (color_pair > 0)
    attron(COLOR_PAIR(color_pair));
  if (bold)
    attron(A_BOLD);
  mvaddnstr(y, line_x, shown.c_str(), width - 2);
  if (bold)
    attroff(A_BOLD);
  if (color_pair > 0)
    attroff(COLOR_PAIR(color_pair));
}

void draw_wrapped_block(WINDOW *win, int y, int x, int width, int height,
                        std::string_view text) {
  if (!win || width <= 0 || height <= 0)
    return;
  const auto lines = wrap_text_lines(text, width);
  for (int i = 0; i < height; ++i) {
    mvwaddnstr(win, y + i, x, std::string(width, ' ').c_str(), width);
  }
  for (int i = 0; i < height && i < static_cast<int>(lines.size()); ++i) {
    mvwaddnstr(win, y + i, x, lines[static_cast<std::size_t>(i)].c_str(),
               width);
  }
}

void draw_centered_line(WINDOW *win, int y, int width, std::string_view text,
                        int color_pair = 0, bool bold = false) {
  if (!win || width <= 0 || y < 0)
    return;
  const std::string shown = ellipsize_text(text, std::max(1, width - 2));
  const int x = std::max(1, (width - static_cast<int>(shown.size())) / 2);
  if (color_pair > 0)
    wattron(win, COLOR_PAIR(color_pair));
  if (bold)
    wattron(win, A_BOLD);
  mvwaddnstr(win, y, x, shown.c_str(), width - 2);
  if (bold)
    wattroff(win, A_BOLD);
  if (color_pair > 0)
    wattroff(win, COLOR_PAIR(color_pair));
}

[[nodiscard]] int session_accent_color(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (session.lifecycle == "live" && session.work_gate != "open") {
    return session.work_gate == "governance" ? 8 : 7;
  }
  if (session.lifecycle == "live" && session.activity == "review")
    return 9;
  if (session.lifecycle == "terminal") {
    if (session.finish_reason == "failed" ||
        session.finish_reason == "terminated") {
      return 4;
    }
    if (session.finish_reason == "exhausted")
      return 3;
    return 11;
  }
  if (session.campaign_status == "launching" ||
      session.campaign_status == "running" ||
      session.campaign_status == "stopping")
    return 1;
  return 6;
}

[[nodiscard]] operator_session_state_t operator_session_state(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (session.lifecycle == "live" && session.work_gate != "open") {
    if (is_clarification_work_gate(session.work_gate)) {
      return operator_session_state_t::waiting_clarification;
    }
    if (session.work_gate == "governance") {
      return operator_session_state_t::waiting_governance;
    }
    if (session.work_gate == "operator_pause") {
      return operator_session_state_t::operator_paused;
    }
    return operator_session_state_t::unknown;
  }
  if (session.lifecycle == "live" && session.activity == "review")
    return operator_session_state_t::review;
  if (session.lifecycle == "terminal")
    return operator_session_state_t::done;
  if (session.lifecycle == "live") {
    return operator_session_state_t::running;
  }
  return operator_session_state_t::unknown;
}

[[nodiscard]] std::string_view
operator_session_state_label(operator_session_state_t state) {
  switch (state) {
  case operator_session_state_t::running:
    return "Running";
  case operator_session_state_t::waiting_clarification:
    return "Waiting: Clarification";
  case operator_session_state_t::waiting_governance:
    return "Waiting: Governance";
  case operator_session_state_t::operator_paused:
    return "Operator Paused";
  case operator_session_state_t::review:
    return "Review";
  case operator_session_state_t::done:
    return "Done";
  case operator_session_state_t::unknown:
    break;
  }
  return "Unknown";
}

[[nodiscard]] std::string operator_session_state_detail(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool has_pending_review) {
  std::ostringstream out;
  if (session.lifecycle == "live" && session.activity == "review" &&
      !has_pending_review) {
    out << "Review";
    if (!session.finish_reason.empty() && session.finish_reason != "none") {
      out << " (" << session.finish_reason << ")";
    }
    return out.str();
  }
  out << operator_session_state_label(operator_session_state(session));
  if (session.activity == "planning") {
    out << " (planning)";
  } else if (session.campaign_status == "launching" ||
             session.campaign_status == "running" ||
             session.campaign_status == "stopping") {
    out << " (campaign active)";
  } else if (session.lifecycle == "live" && session.activity == "review") {
    if (session.finish_reason == "failed") {
      out << " (review after failure)";
    } else {
      out << " (review pending)";
    }
  } else if (session.lifecycle == "terminal" &&
             !session.finish_reason.empty() &&
             session.finish_reason != "none") {
    out << " (" << session.finish_reason << ")";
  }
  return out.str();
}

[[nodiscard]] std::string operator_session_action_hint(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool has_pending_review) {
  const auto state = operator_session_state(session);
  if (state == operator_session_state_t::review && !has_pending_review) {
    return "Continue (optional)";
  }
  if (state == operator_session_state_t::done && !has_pending_review) {
    return "<none>";
  }
  switch (state) {
  case operator_session_state_t::running:
    return "Pause | Terminate";
  case operator_session_state_t::waiting_clarification:
    return "Answer clarification";
  case operator_session_state_t::waiting_governance:
    return "Grant | Deny | Clarify | Terminate";
  case operator_session_state_t::operator_paused:
    return "Resume | Terminate";
  case operator_session_state_t::review:
    return "Continue | Acknowledge";
  case operator_session_state_t::done:
    return "Acknowledge";
  case operator_session_state_t::unknown:
    break;
  }
  return "<none>";
}

[[nodiscard]] std::string session_state_badge(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool has_pending_review) {
  if (session.lifecycle == "live" && session.activity == "review" &&
      !has_pending_review) {
    if (session.finish_reason == "interrupted")
      return "[INTERRUPTED]";
    if (session.finish_reason == "failed")
      return "[FAILED]";
    return "[IDLE]";
  }
  switch (operator_session_state(session)) {
  case operator_session_state_t::waiting_clarification:
    return "[CLARIFY]";
  case operator_session_state_t::waiting_governance:
    return "[GOV]";
  case operator_session_state_t::operator_paused:
    return "[PAUSED]";
  case operator_session_state_t::review:
    return "[REVIEW]";
  case operator_session_state_t::done:
    if (session.finish_reason == "success")
      return "[DONE]";
    if (session.finish_reason == "exhausted")
      return "[EXHAUSTED]";
    if (session.finish_reason == "failed")
      return "[FAILED]";
    if (session.finish_reason == "terminated")
      return "[TERMINATED]";
    return "[DONE]";
  case operator_session_state_t::running:
    return "[RUNNING]";
  case operator_session_state_t::unknown:
    break;
  }
  return "[SESSION]";
}

[[nodiscard]] std::string_view
operator_console_view_label(operator_console_view_t view) {
  switch (view) {
  case operator_console_view_t::sessions:
    return "overview";
  case operator_console_view_t::requests:
    return "requests";
  case operator_console_view_t::summaries:
    return "reviews";
  }
  return "overview";
}

[[nodiscard]] std::string_view
operator_console_state_filter_label(operator_console_state_filter_t filter) {
  switch (filter) {
  case operator_console_state_filter_t::all:
    return "all";
  case operator_console_state_filter_t::working:
    return "planning";
  case operator_console_state_filter_t::campaign_active:
    return "campaign";
  case operator_console_state_filter_t::blocked:
    return "blocked/waiting";
  case operator_console_state_filter_t::review:
    return "review/history";
  case operator_console_state_filter_t::terminal:
    return "done";
  }
  return "all";
}

[[nodiscard]] bool session_matches_state_filter(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    operator_console_state_filter_t filter) {
  switch (filter) {
  case operator_console_state_filter_t::all:
    return true;
  case operator_console_state_filter_t::working:
    return session.lifecycle == "live" && session.activity == "planning";
  case operator_console_state_filter_t::campaign_active:
    return session.campaign_status == "launching" ||
           session.campaign_status == "running" ||
           session.campaign_status == "stopping";
  case operator_console_state_filter_t::blocked:
    return session.lifecycle == "live" && session.work_gate != "open";
  case operator_console_state_filter_t::review:
    return session.lifecycle == "live" && session.activity == "review";
  case operator_console_state_filter_t::terminal:
    return session.lifecycle == "terminal";
  }
  return true;
}

void filter_sessions_for_console(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        &all_sessions,
    operator_console_state_filter_t filter,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out) {
  if (!out)
    return;
  out->clear();
  out->reserve(all_sessions.size());
  for (const auto &session : all_sessions) {
    if (session_matches_state_filter(session, filter))
      out->push_back(session);
  }
}

[[nodiscard]] std::size_t count_sessions_in_operator_state(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        &sessions,
    operator_session_state_t state) {
  return static_cast<std::size_t>(std::count_if(
      sessions.begin(), sessions.end(), [state](const auto &session) {
        return operator_session_state(session) == state;
      }));
}

[[nodiscard]] std::optional<std::size_t> find_session_index_by_id(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        &sessions,
    std::string_view marshal_session_id) {
  for (std::size_t i = 0; i < sessions.size(); ++i) {
    if (sessions[i].marshal_session_id == marshal_session_id)
      return i;
  }
  return std::nullopt;
}

void select_session_by_id(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        &sessions,
    std::string_view marshal_session_id, std::size_t *selected) {
  if (!selected)
    return;
  const auto found = find_session_index_by_id(sessions, marshal_session_id);
  if (found.has_value())
    *selected = *found;
}

void cycle_operator_console_view(operator_console_view_t *view) {
  if (!view)
    return;
  switch (*view) {
  case operator_console_view_t::sessions:
    *view = operator_console_view_t::requests;
    return;
  case operator_console_view_t::requests:
    *view = operator_console_view_t::summaries;
    return;
  case operator_console_view_t::summaries:
    *view = operator_console_view_t::sessions;
    return;
  }
}

void cycle_operator_console_state_filter(
    operator_console_state_filter_t *filter) {
  if (!filter)
    return;
  switch (*filter) {
  case operator_console_state_filter_t::all:
    *filter = operator_console_state_filter_t::working;
    return;
  case operator_console_state_filter_t::working:
    *filter = operator_console_state_filter_t::campaign_active;
    return;
  case operator_console_state_filter_t::campaign_active:
    *filter = operator_console_state_filter_t::blocked;
    return;
  case operator_console_state_filter_t::blocked:
    *filter = operator_console_state_filter_t::review;
    return;
  case operator_console_state_filter_t::review:
    *filter = operator_console_state_filter_t::terminal;
    return;
  case operator_console_state_filter_t::terminal:
    *filter = operator_console_state_filter_t::all;
    return;
  }
}

void draw_detail_scroll_indicator(int y, int x, int width, std::size_t top_line,
                                  std::size_t page_lines,
                                  std::size_t total_lines) {
  clear_region_line(y, x, width);
  if (width <= 0 || page_lines == 0 || total_lines == 0)
    return;
  const std::size_t shown_from = top_line + 1;
  const std::size_t shown_to = std::min(total_lines, top_line + page_lines);
  std::ostringstream out;
  if (total_lines > page_lines) {
    out << "Scroll Wheel/Bar/PgUp/PgDn/Home/End | lines " << shown_from << "-"
        << shown_to << "/" << total_lines;
  } else {
    out << "Lines " << shown_from << "-" << shown_to << "/" << total_lines;
  }
  attron(COLOR_PAIR(3));
  mvaddnstr(y, x, ellipsize_text(out.str(), width).c_str(), width);
  attroff(COLOR_PAIR(3));
}

void draw_status_line(int y, int x, int width, std::string_view status_line,
                      bool status_is_error) {
  clear_region_line(y, x, width);
  if (width <= 0 || status_line.empty())
    return;
  const std::string shown =
      "status: " + ellipsize_text(status_line, std::max(1, width - 8));
  attron(COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);
  mvaddnstr(y, x, shown.c_str(), width);
  attroff(COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);
}

void draw_vertical_scrollbar(int y, int x, int height, std::size_t top_line,
                             std::size_t page_lines, std::size_t total_lines) {
  if (height <= 0)
    return;
  for (int row = 0; row < height; ++row) {
    mvaddch(y + row, x, ' ');
  }
  if (page_lines == 0 || total_lines == 0)
    return;
  attron(COLOR_PAIR(11));
  for (int row = 0; row < height; ++row) {
    mvaddch(y + row, x, ACS_VLINE);
  }
  attroff(COLOR_PAIR(11));
  if (total_lines <= page_lines)
    return;
  const std::size_t max_top_line = total_lines - page_lines;
  const int thumb_h = std::clamp(
      static_cast<int>(
          (page_lines * static_cast<std::size_t>(height) + total_lines - 1) /
          total_lines),
      1, height);
  const int travel = std::max(0, height - thumb_h);
  const int thumb_top =
      max_top_line == 0
          ? 0
          : static_cast<int>((top_line * static_cast<std::size_t>(travel) +
                              max_top_line / 2) /
                             max_top_line);
  attron(COLOR_PAIR(1) | A_BOLD);
  for (int row = 0; row < thumb_h; ++row) {
    mvaddch(y + thumb_top + row, x, ACS_CKBOARD);
  }
  attroff(COLOR_PAIR(1) | A_BOLD);
}

[[nodiscard]] bool mouse_hits_detail_viewport(const detail_viewport_t &viewport,
                                              int mouse_x, int mouse_y) {
  if (viewport.width <= 0 || viewport.height <= 0)
    return false;
  const int right_edge = viewport.scrollbar_x > 0
                             ? viewport.scrollbar_x
                             : viewport.x + viewport.width - 1;
  return mouse_x >= viewport.x && mouse_x <= right_edge &&
         mouse_y >= viewport.y && mouse_y < viewport.y + viewport.height;
}

