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

[[nodiscard]] bool prompt_text_dialog(const std::string &title,
                                      const std::string &label,
                                      std::string *out_value, bool secret,
                                      bool allow_empty, bool *cancelled) {
  if (cancelled)
    *cancelled = false;
  if (!out_value)
    return false;
  ensure_human_dialog_colors();
  std::string buffer = *out_value;
  const int width = std::max(44, std::min(std::max(44, COLS - 6), 108));
  const int body_width = std::max(12, width - 6);
  const auto label_lines = wrap_text_lines(label, body_width);
  const int label_h =
      std::min<int>(4, std::max<int>(1, static_cast<int>(label_lines.size())));
  const int footer_h = 2;
  const int input_h =
      std::max(4, std::min(10, std::max(4, LINES - label_h - footer_h - 8)));
  const int field_box_h = input_h + 2;
  const int height =
      std::max(14, std::min(std::max(14, LINES - 2),
                            label_h + field_box_h + footer_h + 5));
  const int starty = std::max(0, (LINES - height) / 2);
  const int startx = std::max(0, (COLS - width) / 2);
  WINDOW *shadow = nullptr;
  if (starty + 1 < LINES && startx + 2 < COLS) {
    shadow = newwin(std::max(1, height),
                    std::max(1, std::min(width, COLS - startx - 2)), starty + 1,
                    startx + 2);
    if (shadow) {
      if (has_colors())
        wbkgdset(shadow, COLOR_PAIR(11) | ' ');
      werase(shadow);
      wrefresh(shadow);
    }
  }
  WINDOW *win = newwin(height, width, starty, startx);
  if (!win) {
    if (shadow)
      delwin(shadow);
    return false;
  }
  keypad(win, TRUE);
  curs_set(1);

  for (;;) {
    draw_boxed_window(win, title);
    if (has_colors()) {
      wattrset(win, COLOR_PAIR(7));
    }
    for (int i = 0; i < label_h; ++i) {
      mvwaddnstr(win, 2 + i, 3, std::string(body_width, ' ').c_str(),
                 body_width);
      if (i < static_cast<int>(label_lines.size())) {
        mvwaddnstr(win, 2 + i, 3,
                   label_lines[static_cast<std::size_t>(i)].c_str(),
                   body_width);
      }
    }
    const int field_box_y = 2 + label_h + 1;
    const int field_box_x = 3;
    const int field_box_w = width - 6;
    draw_boxed_subwindow_region(win, field_box_y, field_box_x, field_box_h,
                                field_box_w, " input ", 11, 12);

    if (has_colors()) {
      wattrset(win, COLOR_PAIR(3) | A_BOLD);
    } else {
      wattrset(win, A_BOLD);
    }
    const std::string footer_line =
        allow_empty
            ? "Enter accept | Esc cancel | Backspace edit"
            : "Enter accept | Esc cancel | Backspace edit | input required";
    mvwaddnstr(win, height - 3, 3, std::string(width - 6, ' ').c_str(),
               width - 6);
    mvwaddnstr(win, height - 3, 3,
               ellipsize_text(footer_line, width - 6).c_str(), width - 6);
    if (has_colors()) {
      wattrset(win, COLOR_PAIR(6));
    } else {
      wattrset(win, A_NORMAL);
    }
    mvwaddnstr(win, height - 2, 3, std::string(width - 6, ' ').c_str(),
               width - 6);

    const int field_width = std::max(1, field_box_w - 2);
    const int field_y = field_box_y + 1;
    const std::string shown = secret ? std::string(buffer.size(), '*') : buffer;
    const auto wrapped_input =
        wrap_input_lines(shown, std::max(1, field_width - 1));
    const int visible_input_h = std::max(1, field_box_h - 2);
    const int top_line =
        std::max(0, static_cast<int>(wrapped_input.size()) - visible_input_h);
    for (int i = 0; i < visible_input_h; ++i) {
      if (has_colors()) {
        wattrset(win, COLOR_PAIR(12) | A_BOLD);
      }
      mvwaddnstr(win, field_y + i, field_box_x + 1,
                 std::string(field_width, ' ').c_str(), field_width);
      const int src = top_line + i;
      if (src >= 0 && src < static_cast<int>(wrapped_input.size())) {
        mvwaddnstr(win, field_y + i, field_box_x + 1,
                   wrapped_input[static_cast<std::size_t>(src)].c_str(),
                   field_width - 1);
      }
    }
    if (has_colors()) {
      wattrset(win, COLOR_PAIR(13) | A_BOLD);
      if (static_cast<int>(wrapped_input.size()) > visible_input_h) {
        const std::string scroll_text =
            std::to_string(top_line + 1) + "-" +
            std::to_string(top_line + visible_input_h) + "/" +
            std::to_string(wrapped_input.size());
        mvwaddnstr(win, field_box_y,
                   field_box_x + field_box_w - 2 -
                       static_cast<int>(scroll_text.size()),
                   scroll_text.c_str(), static_cast<int>(scroll_text.size()));
      }
    }
    const int cursor_line =
        std::max(0, static_cast<int>(wrapped_input.size()) - 1) - top_line;
    const int cursor_col =
        wrapped_input.empty()
            ? 0
            : std::min<int>(static_cast<int>(wrapped_input.back().size()),
                            std::max(0, field_width - 1));
    wmove(win, field_y + std::clamp(cursor_line, 0, visible_input_h - 1),
          field_box_x + 1 + cursor_col);
    wrefresh(win);

    const int ch = wgetch(win);
    if (ch == 27) {
      if (cancelled)
        *cancelled = true;
      delwin(win);
      if (shadow)
        delwin(shadow);
      curs_set(0);
      return true;
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      if (!allow_empty && trim_ascii(buffer).empty())
        continue;
      *out_value = trim_ascii(buffer);
      delwin(win);
      if (shadow)
        delwin(shadow);
      curs_set(0);
      return true;
    }
    if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
      if (!buffer.empty())
        buffer.pop_back();
      continue;
    }
    if (ch >= 32 && ch <= 126) {
      buffer.push_back(static_cast<char>(ch));
    }
  }
}

[[nodiscard]] bool prompt_choice_dialog(const std::string &title,
                                        const std::string &prompt,
                                        const std::vector<std::string> &options,
                                        std::size_t *out_index,
                                        bool *cancelled) {
  std::vector<prompt_choice_option_t> choice_options{};
  choice_options.reserve(options.size());
  for (const auto &option : options) {
    choice_options.push_back(prompt_choice_option_t{
        .label = option,
        .enabled = true,
    });
  }
  return prompt_choice_dialog(title, prompt, choice_options, out_index,
                              cancelled);
}

[[nodiscard]] bool
prompt_choice_dialog(const std::string &title, const std::string &prompt,
                     const std::vector<prompt_choice_option_t> &options,
                     std::size_t *out_index, bool *cancelled) {
  if (cancelled)
    *cancelled = false;
  if (!out_index || options.empty())
    return false;
  ensure_human_dialog_colors();

  auto first_enabled_index = [&]() -> std::optional<std::size_t> {
    for (std::size_t i = 0; i < options.size(); ++i) {
      if (options[i].enabled)
        return i;
    }
    return std::nullopt;
  };
  auto move_selection = [&](std::size_t current, int delta) -> std::size_t {
    if (options.empty() || delta == 0)
      return current;
    const auto first_enabled = first_enabled_index();
    if (!first_enabled.has_value()) {
      const auto max_index = static_cast<std::ptrdiff_t>(options.size() - 1);
      const auto next = std::clamp(static_cast<std::ptrdiff_t>(current) +
                                       static_cast<std::ptrdiff_t>(delta),
                                   static_cast<std::ptrdiff_t>(0), max_index);
      return static_cast<std::size_t>(next);
    }
    std::ptrdiff_t probe = static_cast<std::ptrdiff_t>(current);
    const std::ptrdiff_t step = delta < 0 ? -1 : 1;
    while (true) {
      probe += step;
      if (probe < 0 || probe >= static_cast<std::ptrdiff_t>(options.size())) {
        return current;
      }
      if (options[static_cast<std::size_t>(probe)].enabled) {
        return static_cast<std::size_t>(probe);
      }
    }
  };

  std::size_t selected = std::min<std::size_t>(*out_index, options.size() - 1);
  if (const auto first_enabled = first_enabled_index();
      first_enabled.has_value() && !options[selected].enabled) {
    selected = *first_enabled;
  }

  const int requested_height =
      std::max(8, static_cast<int>(options.size()) + 7);
  const int height =
      std::max(8, std::min(requested_height, std::max(8, LINES - 2)));
  const int width = std::max(24, std::min(std::max(24, COLS - 2), 96));
  const int starty = std::max(0, (LINES - height) / 2);
  const int startx = std::max(0, (COLS - width) / 2);
  const int visible_rows = std::max(1, height - 6);
  WINDOW *win = newwin(height, width, starty, startx);
  if (!win)
    return false;
  keypad(win, TRUE);

  std::size_t top = 0;
  auto keep_selected_visible = [&]() {
    if (selected < top) {
      top = selected;
    } else if (selected >= top + static_cast<std::size_t>(visible_rows)) {
      top = selected + 1 - static_cast<std::size_t>(visible_rows);
    }
  };
  keep_selected_visible();

  for (;;) {
    draw_boxed_window(win, title);
    draw_wrapped_block(win, 2, 2, width - 4, 2, prompt);

    for (int row = 0; row < visible_rows; ++row) {
      const int screen_row = 4 + row;
      const std::size_t idx = top + static_cast<std::size_t>(row);
      if (idx >= options.size()) {
        wattrset(win, COLOR_PAIR(6));
        mvwaddnstr(win, screen_row, 2, std::string(width - 4, ' ').c_str(),
                   width - 4);
        continue;
      }

      const bool is_selected = (idx == selected);
      const bool is_enabled = options[idx].enabled;
      const std::string line =
          std::string(is_selected ? "> " : "  ") + options[idx].label;
      int attrs = COLOR_PAIR(6);

      if (is_selected && is_enabled) {
        attrs = COLOR_PAIR(2) | A_BOLD;
      } else if (is_enabled) {
        attrs = COLOR_PAIR(6);
      } else if (is_selected) {
        attrs = COLOR_PAIR(4) | A_BOLD;
      } else {
        attrs = COLOR_PAIR(4) | A_DIM;
      }

      wattrset(win, attrs);
      mvwaddnstr(win, screen_row, 2, std::string(width - 4, ' ').c_str(),
                 width - 4);
      mvwaddnstr(win, screen_row, 2, line.c_str(), width - 4);
      wattrset(win, COLOR_PAIR(6));
    }

    if (options.size() > static_cast<std::size_t>(visible_rows)) {
      const std::string scroll_text =
          std::to_string(top + 1) + "-" +
          std::to_string(std::min(top + static_cast<std::size_t>(visible_rows),
                                  options.size())) +
          "/" + std::to_string(options.size());
      mvwaddnstr(win, height - 3,
                 width - 2 - static_cast<int>(scroll_text.size()),
                 scroll_text.c_str(), static_cast<int>(scroll_text.size()));
    }

    const bool has_enabled = first_enabled_index().has_value();
    const std::string footer =
        has_enabled ? "Up/Down=move  Enter=accept  Esc=cancel  red=unavailable"
                    : "No actions are available right now. Esc=cancel";
    mvwaddnstr(win, height - 2, 2, std::string(width - 4, ' ').c_str(),
               width - 4);
    mvwaddnstr(win, height - 2, 2, footer.c_str(), width - 4);
    wrefresh(win);

    const int ch = wgetch(win);
    if (ch == 27) {
      if (cancelled)
        *cancelled = true;
      delwin(win);
      return true;
    }
    if (ch == KEY_UP || ch == 'k') {
      const std::size_t next = move_selection(selected, -1);
      if (next != selected) {
        selected = next;
        keep_selected_visible();
      }
      continue;
    }
    if (ch == KEY_DOWN || ch == 'j') {
      const std::size_t next = move_selection(selected, +1);
      if (next != selected) {
        selected = next;
        keep_selected_visible();
      }
      continue;
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      if (!options[selected].enabled) {
        beep();
        continue;
      }
      *out_index = selected;
      delwin(win);
      return true;
    }
  }
}

[[nodiscard]] bool prompt_yes_no_dialog(const std::string &title,
                                        const std::string &prompt,
                                        bool default_yes, bool *out_value,
                                        bool *cancelled) {
  if (!out_value)
    return false;
  std::size_t idx = default_yes ? 1u : 0u;
  if (!prompt_choice_dialog(title, prompt,
                            std::vector<std::string>{"No", "Yes"}, &idx,
                            cancelled)) {
    return false;
  }
  if (cancelled && *cancelled)
    return true;
  *out_value = (idx == 1u);
  return true;
}

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
