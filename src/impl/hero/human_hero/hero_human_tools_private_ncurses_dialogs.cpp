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

