#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_render.h"
#include "iinuji/iinuji_utils.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct ui_choice_panel_option_t {
  std::string label{};
  bool enabled{true};
  std::string disabled_status{};
};

struct ui_dialog_window_guard_t {
  WINDOW *win{nullptr};
  ~ui_dialog_window_guard_t() {
    if (win != nullptr)
      delwin(win);
  }
};

inline std::string ui_choice_panel_trim_to_width(const std::string &text,
                                                 int width) {
  if (width <= 0)
    return {};
  if (static_cast<int>(text.size()) <= width)
    return text;
  if (width <= 3)
    return text.substr(0, static_cast<std::size_t>(width));
  return text.substr(0, static_cast<std::size_t>(width - 3)) + "...";
}

inline std::string ui_dialog_trim_ascii(std::string_view text) {
  std::size_t first = 0;
  while (first < text.size() &&
         std::isspace(static_cast<unsigned char>(text[first])) != 0) {
    ++first;
  }
  std::size_t last = text.size();
  while (last > first &&
         std::isspace(static_cast<unsigned char>(text[last - 1])) != 0) {
    --last;
  }
  return std::string(text.substr(first, last - first));
}

inline std::vector<std::string>
ui_dialog_wrap_input_lines(std::string_view text, int width) {
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

inline void ui_dialog_restore_background(WINDOW *snapshot, int snapshot_h,
                                         int snapshot_w) {
  if (snapshot == nullptr)
    return;
  int H = 0;
  int W = 0;
  getmaxyx(stdscr, H, W);
  if (H <= 0 || W <= 0 || snapshot_h <= 0 || snapshot_w <= 0)
    return;
  const int copy_h = std::min(H, snapshot_h);
  const int copy_w = std::min(W, snapshot_w);
  if (copy_h <= 0 || copy_w <= 0)
    return;
  copywin(snapshot, stdscr, 0, 0, 0, 0, copy_h - 1, copy_w - 1, false);
}

inline bool ui_choice_panel_has_enabled_options(
    const std::vector<ui_choice_panel_option_t> &options) {
  for (const auto &option : options) {
    if (option.enabled)
      return true;
  }
  return false;
}

inline std::size_t ui_choice_panel_first_enabled_index(
    const std::vector<ui_choice_panel_option_t> &options) {
  for (std::size_t i = 0; i < options.size(); ++i) {
    if (options[i].enabled)
      return i;
  }
  return 0;
}

inline std::size_t ui_choice_panel_last_enabled_index(
    const std::vector<ui_choice_panel_option_t> &options) {
  for (std::size_t i = options.size(); i > 0; --i) {
    if (options[i - 1].enabled)
      return i - 1;
  }
  return options.empty() ? 0u : (options.size() - 1u);
}

inline std::size_t ui_choice_panel_move_selection(
    const std::vector<ui_choice_panel_option_t> &options, std::size_t current,
    int delta) {
  if (options.empty() || delta == 0)
    return current;
  const bool has_enabled = ui_choice_panel_has_enabled_options(options);
  if (!has_enabled) {
    const auto max_index = static_cast<std::ptrdiff_t>(options.size() - 1u);
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
}

inline std::size_t ui_choice_panel_page_selection(
    const std::vector<ui_choice_panel_option_t> &options, std::size_t current,
    int delta, int page_rows) {
  if (page_rows <= 0)
    return current;
  std::size_t next = current;
  for (int step = 0; step < page_rows; ++step) {
    const std::size_t candidate =
        ui_choice_panel_move_selection(options, next, delta);
    if (candidate == next)
      break;
    next = candidate;
  }
  return next;
}

inline void ui_choice_panel_keep_selection_visible(std::size_t selected,
                                                   std::size_t total_rows,
                                                   int visible_rows,
                                                   std::size_t *top) {
  if (top == nullptr || visible_rows <= 0)
    return;
  const std::size_t max_top =
      total_rows > static_cast<std::size_t>(visible_rows)
          ? total_rows - static_cast<std::size_t>(visible_rows)
          : 0u;
  if (*top > max_top)
    *top = max_top;
  if (selected < *top) {
    *top = selected;
    return;
  }
  const std::size_t window_end = *top + static_cast<std::size_t>(visible_rows);
  if (selected >= window_end) {
    *top = std::min(selected + 1u - static_cast<std::size_t>(visible_rows),
                    max_top);
  }
}

inline std::string ui_choice_panel_join_lines(
    const std::vector<cuwacunu::iinuji::styled_text_line_t> &lines) {
  std::string out{};
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i > 0)
      out.push_back('\n');
    out += styled_text_line_text(lines[i]);
  }
  return out;
}

inline void ui_choice_panel_configure_box(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &box,
    const std::vector<cuwacunu::iinuji::styled_text_line_t> &lines, bool wrap,
    int scroll_y = 0) {
  if (!box)
    return;
  auto tb =
      std::dynamic_pointer_cast<cuwacunu::iinuji::textBox_data_t>(box->data);
  if (!tb)
    return;
  tb->content = ui_choice_panel_join_lines(lines);
  tb->styled_lines = lines;
  tb->wrap = wrap;
  tb->scroll_y = std::max(0, scroll_y);
  tb->scroll_x = 0;
  tb->tracked_selected_row = -1;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
ui_choice_panel_prompt_lines(std::string prompt, int width) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> lines{};
  const auto wrapped =
      wrap_text(prompt.empty() ? std::string("Select action.") : prompt,
                std::max(1, width));
  lines.reserve(wrapped.size());
  for (const auto &line : wrapped) {
    lines.push_back(cuwacunu::iinuji::styled_text_line_t{
        .text = line,
        .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info,
    });
  }
  if (lines.empty()) {
    lines.push_back(cuwacunu::iinuji::styled_text_line_t{
        .text = "Select action.",
        .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info,
    });
  }
  return lines;
}

template <typename Fn>
inline void ui_run_blocking_dialog(const std::string &title,
                                   const std::string &prompt, Fn &&fn) {
  auto *renderer = get_renderer();
  ui_dialog_window_guard_t background_snapshot{dupwin(stdscr)};
  if (!renderer || background_snapshot.win == nullptr) {
    fn();
    return;
  }

  int snapshot_h = 0;
  int snapshot_w = 0;
  getmaxyx(background_snapshot.win, snapshot_h, snapshot_w);

  int H = 0;
  int W = 0;
  getmaxyx(stdscr, H, W);
  if (H < 8 || W < 36) {
    fn();
    return;
  }

  const short shadow_pair =
      static_cast<short>(get_color_pair("#0B0D11", "#0B0D11"));
  constexpr auto kFrameDelay = std::chrono::milliseconds(140);

  auto render_frame = [&](std::size_t tick) {
    int frame_h = 0;
    int frame_w = 0;
    getmaxyx(stdscr, frame_h, frame_w);
    if (frame_h < 8 || frame_w < 36) {
      return;
    }

    int prompt_width = 0;
    for (const auto &line : split_lines_keep_empty(prompt)) {
      prompt_width = std::max(
          prompt_width, static_cast<int>(ui_dialog_trim_ascii(line).size()));
    }

    const int max_width = std::max(36, frame_w - 4);
    const int width = std::min(
        max_width, std::max(38, std::min(72, prompt_width + 10)));
    const int text_width = std::max(1, width - 6);

    auto prompt_lines = ui_choice_panel_prompt_lines(
        prompt.empty() ? std::string("Working...") : prompt, text_width);
    std::string pulse_frame = "○●○";
    switch (tick % 4u) {
    case 0u:
      pulse_frame = "●○○";
      break;
    case 1u:
      pulse_frame = "○●○";
      break;
    case 2u:
      pulse_frame = "○○●";
      break;
    case 3u:
      pulse_frame = "○●○";
      break;
    }
    std::vector<cuwacunu::iinuji::styled_text_line_t> footer_lines{
        cuwacunu::iinuji::styled_text_line_t{
            .text = "working",
            .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Debug,
        },
        cuwacunu::iinuji::styled_text_line_t{
            .text = pulse_frame,
            .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Accent,
        },
    };

    const int prompt_rows = std::max(1, static_cast<int>(prompt_lines.size()));
    const int footer_rows = std::max(1, static_cast<int>(footer_lines.size()));
    const int height =
        std::clamp(2 + prompt_rows + 1 + footer_rows, 7,
                   std::max(7, frame_h - 2));
    const int starty = std::max(0, (frame_h - height) / 2);
    const int startx = std::max(0, (frame_w - width) / 2);

    ui_dialog_restore_background(background_snapshot.win, snapshot_h,
                                 snapshot_w);
    if (height > 0 && width > 0) {
      const int shadow_y = std::min(frame_h, starty + 1);
      const int shadow_x = std::min(frame_w, startx + 2);
      if (shadow_y < frame_h && shadow_x < frame_w) {
        renderer->fillRect(shadow_y, shadow_x,
                           std::min(height, frame_h - shadow_y),
                           std::min(width, frame_w - shadow_x), shadow_pair);
      }
    }

    iinuji_layout_t panel_layout{};
    auto panel = create_panel(
        "__ui_blocking_dialog__", panel_layout,
        iinuji_style_t{
            "#E2E6EE", "#151A22", true, "#5C6678", false, false,
            " " + (title.empty() ? std::string("Working") : title) + " "});
    auto body = panel_body_object(panel);
    auto prompt_box = create_text_box(
        "__ui_blocking_dialog_prompt__", "", false, text_align_t::Left,
        iinuji_layout_t{},
        iinuji_style_t{"#D6DDEA", "#151A22", false, "#151A22"});
    auto footer_box = create_text_box(
        "__ui_blocking_dialog_footer__", "", false, text_align_t::Center,
        iinuji_layout_t{},
        iinuji_style_t{"#8B94A6", "#151A22", false, "#151A22"});
    if (body) {
      body->grid = std::make_shared<cuwacunu::iinuji::grid_spec_t>();
      body->grid->rows = {len_spec::px(prompt_rows), len_spec::px(footer_rows)};
      body->grid->cols = {len_spec::frac(1.0)};
      body->grid->gap_row = 1;
      place_in_grid(prompt_box, 0, 0);
      place_in_grid(footer_box, 1, 0);
      body->add_children({prompt_box, footer_box});
    }

    ui_choice_panel_configure_box(prompt_box, prompt_lines, false);
    ui_choice_panel_configure_box(footer_box, footer_lines, false);

    layout_tree(panel, Rect{startx, starty, width, height});
    ::curs_set(0);
    render_tree(panel);
    ::refresh();
  };

  auto future = std::async(std::launch::async, [&]() { fn(); });
  std::size_t tick = 0u;
  try {
    while (true) {
      render_frame(tick);
      if (future.wait_for(kFrameDelay) == std::future_status::ready) {
        break;
      }
      ++tick;
    }
    future.get();
  } catch (...) {
    ui_dialog_restore_background(background_snapshot.win, snapshot_h,
                                 snapshot_w);
    ::refresh();
    throw;
  }

  ui_dialog_restore_background(background_snapshot.win, snapshot_h, snapshot_w);
  ::refresh();
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
ui_choice_panel_option_lines(
    const std::vector<ui_choice_panel_option_t> &options, std::size_t selected,
    int width) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> lines{};
  lines.reserve(options.size());
  const std::string enabled_bg = "#223142";
  const std::string disabled_bg = "#312126";

  for (std::size_t i = 0; i < options.size(); ++i) {
    const bool is_selected = (i == selected);
    const bool is_enabled = options[i].enabled;
    const std::string badge = is_enabled ? std::string{} : " [unavailable]";
    const int label_budget =
        std::max(1, width - 2 - static_cast<int>(badge.size()));
    const std::string label =
        ui_choice_panel_trim_to_width(options[i].label, label_budget);

    std::vector<cuwacunu::iinuji::styled_text_segment_t> segments{};
    segments.push_back(cuwacunu::iinuji::styled_text_segment_t{
        .text = is_selected ? "> " : "  ",
        .emphasis =
            is_selected
                ? (is_enabled ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                              : cuwacunu::iinuji::text_line_emphasis_t::Warning)
                : cuwacunu::iinuji::text_line_emphasis_t::Debug,
    });
    segments.push_back(cuwacunu::iinuji::styled_text_segment_t{
        .text = label,
        .emphasis = is_enabled
                        ? cuwacunu::iinuji::text_line_emphasis_t::None
                        : cuwacunu::iinuji::text_line_emphasis_t::MutedError,
    });
    if (!badge.empty()) {
      segments.push_back(cuwacunu::iinuji::styled_text_segment_t{
          .text = badge,
          .emphasis = cuwacunu::iinuji::text_line_emphasis_t::MutedWarning,
      });
    }

    auto line = make_segmented_styled_text_line(
        std::move(segments),
        is_enabled ? cuwacunu::iinuji::text_line_emphasis_t::None
                   : cuwacunu::iinuji::text_line_emphasis_t::MutedError);
    if (is_selected) {
      line.background_color = is_enabled ? enabled_bg : disabled_bg;
    }
    lines.push_back(std::move(line));
  }
  return lines;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
ui_choice_panel_visible_option_lines(
    const std::vector<ui_choice_panel_option_t> &options, std::size_t selected,
    std::size_t top, int visible_rows, int width) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> lines{};
  const int rows = std::max(1, visible_rows);
  lines.reserve(static_cast<std::size_t>(rows));
  const auto rendered = ui_choice_panel_option_lines(options, selected, width);
  for (int row = 0; row < rows; ++row) {
    const std::size_t idx = top + static_cast<std::size_t>(row);
    if (idx < options.size()) {
      lines.push_back(rendered[idx]);
    } else {
      lines.push_back(cuwacunu::iinuji::styled_text_line_t{
          .text = "",
          .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None,
      });
    }
  }
  return lines;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
ui_choice_panel_scrollbar_lines(std::size_t total_rows, std::size_t top,
                                int visible_rows) {
  const int rows = std::max(1, visible_rows);
  std::vector<cuwacunu::iinuji::styled_text_line_t> lines{};
  lines.reserve(static_cast<std::size_t>(rows));

  if (total_rows == 0u || total_rows <= static_cast<std::size_t>(rows)) {
    for (int i = 0; i < rows; ++i) {
      lines.push_back(cuwacunu::iinuji::styled_text_line_t{
          .text = " ",
          .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info,
      });
    }
    return lines;
  }

  const double ratio =
      static_cast<double>(rows) / static_cast<double>(std::max<std::size_t>(
                                      total_rows, static_cast<std::size_t>(1)));
  const int thumb_size = std::max(
      1, static_cast<int>(std::round(ratio * static_cast<double>(rows))));
  const std::size_t max_top = total_rows > static_cast<std::size_t>(rows)
                                  ? total_rows - static_cast<std::size_t>(rows)
                                  : 0u;
  const double position_ratio =
      max_top == 0u ? 0.0
                    : static_cast<double>(top) / static_cast<double>(max_top);
  const int thumb_top =
      std::clamp(static_cast<int>(std::round(
                     position_ratio * static_cast<double>(rows - thumb_size))),
                 0, std::max(0, rows - thumb_size));

  for (int i = 0; i < rows; ++i) {
    const bool in_thumb = i >= thumb_top && i < (thumb_top + thumb_size);
    lines.push_back(cuwacunu::iinuji::styled_text_line_t{
        .text = in_thumb ? "#" : ":",
        .emphasis = in_thumb
                        ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                        : cuwacunu::iinuji::text_line_emphasis_t::MutedWarning,
    });
  }
  return lines;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
ui_choice_panel_footer_lines(
    const std::vector<ui_choice_panel_option_t> &options, std::size_t selected,
    std::size_t top, int visible_rows, int width) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> lines{};
  const bool has_enabled = ui_choice_panel_has_enabled_options(options);
  const bool overflow =
      options.size() > static_cast<std::size_t>(std::max(1, visible_rows));
  const std::string primary =
      has_enabled
          ? (overflow ? "Up/Down move | PgUp/PgDn jump | Home/End | Enter | Esc"
                      : "Up/Down move | Home/End | Enter accept | Esc cancel")
          : "No actions are available right now. Esc closes.";
  lines.push_back(cuwacunu::iinuji::styled_text_line_t{
      .text = ui_choice_panel_trim_to_width(primary, width),
      .emphasis = has_enabled ? cuwacunu::iinuji::text_line_emphasis_t::Debug
                              : cuwacunu::iinuji::text_line_emphasis_t::Warning,
  });

  std::string secondary{};
  if (!options.empty() && !options[selected].enabled &&
      !options[selected].disabled_status.empty()) {
    secondary = options[selected].disabled_status;
  } else {
    const std::size_t last_visible =
        std::min(top + static_cast<std::size_t>(std::max(1, visible_rows)),
                 options.size());
    secondary = std::to_string(options.empty() ? 0u : (top + 1u)) + "-" +
                std::to_string(last_visible) + "/" +
                std::to_string(options.size()) +
                " shown | muted red = unavailable";
  }
  lines.push_back(cuwacunu::iinuji::styled_text_line_t{
      .text = ui_choice_panel_trim_to_width(secondary, width),
      .emphasis = (!options.empty() && !options[selected].enabled)
                      ? cuwacunu::iinuji::text_line_emphasis_t::MutedWarning
                      : cuwacunu::iinuji::text_line_emphasis_t::Info,
  });
  return lines;
}

[[nodiscard]] inline bool
ui_prompt_choice_panel(const std::string &title, const std::string &prompt,
                       const std::vector<ui_choice_panel_option_t> &options,
                       std::size_t *out_index, bool *cancelled) {
  if (cancelled)
    *cancelled = false;
  if (!out_index || options.empty())
    return false;

  auto *renderer = get_renderer();
  if (!renderer)
    return false;

  ui_dialog_window_guard_t background_snapshot{dupwin(stdscr)};
  int snapshot_h = 0;
  int snapshot_w = 0;
  if (background_snapshot.win != nullptr) {
    getmaxyx(background_snapshot.win, snapshot_h, snapshot_w);
  }

  std::size_t selected = std::min<std::size_t>(*out_index, options.size() - 1u);
  if (ui_choice_panel_has_enabled_options(options) &&
      !options[selected].enabled) {
    selected = ui_choice_panel_first_enabled_index(options);
  }
  std::size_t top = 0;
  bool dirty = true;
  int last_h = -1;
  int last_w = -1;

  while (true) {
    int H = 0;
    int W = 0;
    getmaxyx(stdscr, H, W);
    if (H < 12 || W < 32)
      return false;
    if (H != last_h || W != last_w) {
      last_h = H;
      last_w = W;
      dirty = true;
    }

    const int max_width = std::max(32, W - 2);
    int longest_label = 0;
    for (const auto &option : options) {
      const int badge = option.enabled ? 0 : 14;
      longest_label = std::max(
          longest_label, static_cast<int>(option.label.size()) + badge + 2);
    }
    int prompt_width = 0;
    for (const auto &line : split_lines_keep_empty(prompt)) {
      prompt_width = std::max(prompt_width, static_cast<int>(line.size()));
    }
    const int width =
        std::min(max_width,
                 std::max(38, std::max(longest_label + 12, prompt_width + 12)));
    const int options_col_width = 1;
    const int scrollbar_gap = 1;
    const int text_width =
        std::max(1, width - 6 - options_col_width - scrollbar_gap);

    auto prompt_lines = ui_choice_panel_prompt_lines(prompt, text_width);
    const int max_height = std::max(10, H - 1);
    const int footer_rows = 2;
    const int reserved_rows = 2 + footer_rows + 2;
    const int prompt_rows =
        std::min<int>(static_cast<int>(prompt_lines.size()),
                      std::max(1, max_height - reserved_rows - 1));
    if (static_cast<int>(prompt_lines.size()) > prompt_rows) {
      prompt_lines.resize(static_cast<std::size_t>(prompt_rows));
    }

    int visible_rows = std::max(
        1, std::min<int>(static_cast<int>(options.size()),
                         max_height - (2 + prompt_rows + footer_rows + 2)));
    const int height = std::clamp(
        2 + prompt_rows + 1 + visible_rows + 1 + footer_rows, 9, max_height);
    const int starty = std::max(0, (H - height) / 2);
    const int startx = std::max(0, (W - width) / 2);

    ui_choice_panel_keep_selection_visible(selected, options.size(),
                                           visible_rows, &top);

    if (dirty) {
      const short shadow_pair =
          static_cast<short>(get_color_pair("#0B0D11", "#0B0D11"));
      ui_dialog_restore_background(background_snapshot.win, snapshot_h,
                                   snapshot_w);
      if (height > 0 && width > 0) {
        const int shadow_y = std::min(H, starty + 1);
        const int shadow_x = std::min(W, startx + 2);
        const int shadow_h = std::max(0, height);
        const int shadow_w = std::max(0, width);
        if (shadow_y < H && shadow_x < W) {
          renderer->fillRect(shadow_y, shadow_x,
                             std::min(shadow_h, H - shadow_y),
                             std::min(shadow_w, W - shadow_x), shadow_pair);
        }
      }

      // The modal sizing math already budgets for the panel border and grid
      // gaps, so keep outer padding at zero or the last rows will clip.
      iinuji_layout_t panel_layout{};

      auto panel = create_panel(
          "__ui_choice_panel__", panel_layout,
          iinuji_style_t{
              "#E2E6EE", "#151A22", true, "#5C6678", false, false,
              " " + (title.empty() ? std::string("Actions") : title) + " "});

      auto body = panel_body_object(panel);
      if (body) {
        body->grid = std::make_shared<cuwacunu::iinuji::grid_spec_t>();
        body->grid->rows = {len_spec::px(prompt_rows),
                            len_spec::px(visible_rows),
                            len_spec::px(footer_rows)};
        body->grid->cols = {len_spec::frac(1.0)};
        body->grid->gap_row = 1;
      }

      auto prompt_box = create_text_box(
          "__ui_choice_panel_prompt__", "", false, text_align_t::Left,
          iinuji_layout_t{},
          iinuji_style_t{"#A8B2C2", "#151A22", false, "#151A22"});
      auto options_row = create_grid_container(
          "__ui_choice_panel_options_row__", {len_spec::frac(1.0)},
          {len_spec::frac(1.0), len_spec::px(options_col_width)}, 0,
          scrollbar_gap, iinuji_layout_t{},
          iinuji_style_t{"#E2E6EE", "#151A22", false, "#151A22"});
      auto options_box = create_text_box(
          "__ui_choice_panel_options__", "", false, text_align_t::Left,
          iinuji_layout_t{},
          iinuji_style_t{"#E2E6EE", "#151A22", false, "#151A22"});
      auto scrollbar_box = create_text_box(
          "__ui_choice_panel_scrollbar__", "", false, text_align_t::Left,
          iinuji_layout_t{},
          iinuji_style_t{"#7F8798", "#151A22", false, "#151A22"});
      auto footer_box = create_text_box(
          "__ui_choice_panel_footer__", "", false, text_align_t::Left,
          iinuji_layout_t{},
          iinuji_style_t{"#8B94A6", "#151A22", false, "#151A22"});

      if (body) {
        place_in_grid(prompt_box, 0, 0);
        place_in_grid(options_row, 1, 0);
        place_in_grid(footer_box, 2, 0);
        body->add_children({prompt_box, options_row, footer_box});
      }

      if (options_row) {
        place_in_grid(options_box, 0, 0);
        place_in_grid(scrollbar_box, 0, 1);
        options_row->add_children({options_box, scrollbar_box});
      }

      ui_choice_panel_configure_box(prompt_box, prompt_lines, false);
      ui_choice_panel_configure_box(
          options_box,
          ui_choice_panel_visible_option_lines(options, selected, top,
                                               visible_rows, text_width),
          false);
      ui_choice_panel_configure_box(
          scrollbar_box,
          ui_choice_panel_scrollbar_lines(options.size(), top, visible_rows),
          false);
      ui_choice_panel_configure_box(
          footer_box,
          ui_choice_panel_footer_lines(options, selected, top, visible_rows,
                                       text_width),
          false);

      layout_tree(panel, Rect{startx, starty, width, height});
      render_tree(panel);
      ::refresh();
      dirty = false;
    }

    const int ch = getch();
    if (ch == ERR)
      continue;
    if (ch == KEY_RESIZE) {
      dirty = true;
      continue;
    }
    if (ch == 27) {
      if (cancelled)
        *cancelled = true;
      return true;
    }
    if (ch == KEY_UP || ch == 'k') {
      const std::size_t next =
          ui_choice_panel_move_selection(options, selected, -1);
      if (next != selected) {
        selected = next;
        dirty = true;
      }
      continue;
    }
    if (ch == KEY_DOWN || ch == 'j') {
      const std::size_t next =
          ui_choice_panel_move_selection(options, selected, +1);
      if (next != selected) {
        selected = next;
        dirty = true;
      }
      continue;
    }
    if (ch == KEY_PPAGE) {
      const std::size_t next =
          ui_choice_panel_page_selection(options, selected, -1, visible_rows);
      if (next != selected) {
        selected = next;
        dirty = true;
      }
      continue;
    }
    if (ch == KEY_NPAGE) {
      const std::size_t next =
          ui_choice_panel_page_selection(options, selected, +1, visible_rows);
      if (next != selected) {
        selected = next;
        dirty = true;
      }
      continue;
    }
    if (ch == KEY_HOME) {
      const std::size_t next =
          ui_choice_panel_has_enabled_options(options)
              ? ui_choice_panel_first_enabled_index(options)
              : 0u;
      if (next != selected) {
        selected = next;
        dirty = true;
      }
      continue;
    }
    if (ch == KEY_END) {
      const std::size_t next = ui_choice_panel_has_enabled_options(options)
                                   ? ui_choice_panel_last_enabled_index(options)
                                   : (options.size() - 1u);
      if (next != selected) {
        selected = next;
        dirty = true;
      }
      continue;
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      if (!options[selected].enabled) {
        ::beep();
        dirty = true;
        continue;
      }
      *out_index = selected;
      return true;
    }
  }
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
ui_text_dialog_input_lines(const std::vector<std::string> &wrapped_lines,
                           int top_line, int visible_rows, bool secret) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> lines{};
  const int rows = std::max(1, visible_rows);
  lines.reserve(static_cast<std::size_t>(rows));
  (void)secret;
  for (int row = 0; row < rows; ++row) {
    const int src = top_line + row;
    if (src >= 0 && src < static_cast<int>(wrapped_lines.size())) {
      lines.push_back(cuwacunu::iinuji::styled_text_line_t{
          .text = wrapped_lines[static_cast<std::size_t>(src)],
          .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None,
          .background_color = "#1C2532",
      });
    } else {
      lines.push_back(cuwacunu::iinuji::styled_text_line_t{
          .text = "",
          .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None,
          .background_color = "#1C2532",
      });
    }
  }
  return lines;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
ui_text_dialog_footer_lines(bool allow_empty, int width,
                            std::size_t total_lines, int top_line,
                            int visible_rows) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> lines{};
  const std::string primary =
      allow_empty
          ? "Enter accept | Esc cancel | Backspace edit"
          : "Enter accept | Esc cancel | Backspace edit | input required";
  lines.push_back(cuwacunu::iinuji::styled_text_line_t{
      .text = ui_choice_panel_trim_to_width(primary, width),
      .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Debug,
  });
  std::string secondary{};
  if (total_lines > static_cast<std::size_t>(std::max(1, visible_rows))) {
    secondary =
        std::to_string(static_cast<std::size_t>(top_line) + 1u) + "-" +
        std::to_string(std::min<std::size_t>(
            total_lines, static_cast<std::size_t>(top_line + visible_rows))) +
        "/" + std::to_string(total_lines) + " shown";
  } else {
    secondary = "Type your response below.";
  }
  lines.push_back(cuwacunu::iinuji::styled_text_line_t{
      .text = ui_choice_panel_trim_to_width(secondary, width),
      .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info,
  });
  return lines;
}

[[nodiscard]] inline bool ui_prompt_text_dialog(const std::string &title,
                                                const std::string &prompt,
                                                std::string *out_value,
                                                bool secret, bool allow_empty,
                                                bool *cancelled) {
  if (cancelled)
    *cancelled = false;
  if (out_value == nullptr)
    return false;

  auto *renderer = get_renderer();
  if (!renderer)
    return false;

  ui_dialog_window_guard_t background_snapshot{dupwin(stdscr)};
  int snapshot_h = 0;
  int snapshot_w = 0;
  if (background_snapshot.win != nullptr) {
    getmaxyx(background_snapshot.win, snapshot_h, snapshot_w);
  }

  std::string buffer = *out_value;
  bool dirty = true;
  int last_h = -1;
  int last_w = -1;

  while (true) {
    int H = 0;
    int W = 0;
    getmaxyx(stdscr, H, W);
    if (H < 14 || W < 42)
      return false;
    if (H != last_h || W != last_w) {
      last_h = H;
      last_w = W;
      dirty = true;
    }

    const int width = std::max(42, std::min(112, W - 4));
    const int text_width = std::max(1, width - 8);
    auto prompt_lines = ui_choice_panel_prompt_lines(prompt, text_width);
    const int prompt_rows = std::min<int>(static_cast<int>(prompt_lines.size()),
                                          std::max(2, (H - 10) / 3));
    if (static_cast<int>(prompt_lines.size()) > prompt_rows) {
      prompt_lines.resize(static_cast<std::size_t>(prompt_rows));
    }

    const int footer_rows = 2;
    const int input_rows =
        std::max(4, std::min(10, H - prompt_rows - footer_rows - 8));
    const int height =
        std::min(H - 2, prompt_rows + input_rows + footer_rows + 6);
    const int starty = std::max(0, (H - height) / 2);
    const int startx = std::max(0, (W - width) / 2);
    const int input_scrollbar_w = 1;
    const int input_gap = 1;
    const int input_text_width =
        std::max(1, text_width - input_scrollbar_w - input_gap);

    const std::string shown = secret ? std::string(buffer.size(), '*') : buffer;
    const auto wrapped_input =
        ui_dialog_wrap_input_lines(shown, std::max(1, input_text_width - 1));
    const int top_line =
        std::max(0, static_cast<int>(wrapped_input.size()) - input_rows);
    const int cursor_line =
        std::max(0, static_cast<int>(wrapped_input.size()) - 1) - top_line;
    const int cursor_col =
        wrapped_input.empty()
            ? 0
            : std::min<int>(static_cast<int>(wrapped_input.back().size()),
                            std::max(0, input_text_width - 1));

    if (dirty) {
      const short shadow_pair =
          static_cast<short>(get_color_pair("#0B0D11", "#0B0D11"));
      ui_dialog_restore_background(background_snapshot.win, snapshot_h,
                                   snapshot_w);
      if (height > 0 && width > 0) {
        const int shadow_y = std::min(H, starty + 1);
        const int shadow_x = std::min(W, startx + 2);
        if (shadow_y < H && shadow_x < W) {
          renderer->fillRect(shadow_y, shadow_x, std::min(height, H - shadow_y),
                             std::min(width, W - shadow_x), shadow_pair);
        }
      }

      // Match the choice panel sizing contract: border only, no extra outer
      // padding, so prompt/input/footer rows stay fully visible.
      iinuji_layout_t panel_layout{};

      auto panel = create_panel(
          "__ui_text_dialog__", panel_layout,
          iinuji_style_t{"#E2E6EE", "#151A22", true, "#5C6678", false, false,
                         " " + (title.empty() ? std::string("Input") : title) +
                             " "});
      auto body = panel_body_object(panel);
      auto prompt_box = create_text_box(
          "__ui_text_dialog_prompt__", "", false, text_align_t::Left,
          iinuji_layout_t{},
          iinuji_style_t{"#A8B2C2", "#151A22", false, "#151A22"});
      auto input_panel =
          create_panel("__ui_text_dialog_input_panel__", iinuji_layout_t{},
                       iinuji_style_t{"#E2E6EE", "#1C2532", true, "#4F6B8A",
                                      false, false, " input "});
      auto footer_box = create_text_box(
          "__ui_text_dialog_footer__", "", false, text_align_t::Left,
          iinuji_layout_t{},
          iinuji_style_t{"#8B94A6", "#151A22", false, "#151A22"});
      if (body) {
        body->grid = std::make_shared<cuwacunu::iinuji::grid_spec_t>();
        body->grid->rows = {len_spec::px(prompt_rows),
                            len_spec::px(input_rows + 2),
                            len_spec::px(footer_rows)};
        body->grid->cols = {len_spec::frac(1.0)};
        body->grid->gap_row = 1;
        place_in_grid(prompt_box, 0, 0);
        place_in_grid(input_panel, 1, 0);
        place_in_grid(footer_box, 2, 0);
        body->add_children({prompt_box, input_panel, footer_box});
      }

      auto input_body = panel_body_object(input_panel);
      auto input_row = create_grid_container(
          "__ui_text_dialog_input_row__", {len_spec::frac(1.0)},
          {len_spec::frac(1.0), len_spec::px(input_scrollbar_w)}, 0, input_gap,
          iinuji_layout_t{},
          iinuji_style_t{"#E2E6EE", "#1C2532", false, "#1C2532"});
      auto input_box = create_text_box(
          "__ui_text_dialog_input_box__", "", false, text_align_t::Left,
          iinuji_layout_t{},
          iinuji_style_t{"#E2E6EE", "#1C2532", false, "#1C2532"});
      auto scrollbar_box = create_text_box(
          "__ui_text_dialog_input_scroll__", "", false, text_align_t::Left,
          iinuji_layout_t{},
          iinuji_style_t{"#7F8798", "#1C2532", false, "#1C2532"});
      if (input_body) {
        input_body->grid = std::make_shared<cuwacunu::iinuji::grid_spec_t>();
        input_body->grid->rows = {len_spec::frac(1.0)};
        input_body->grid->cols = {len_spec::frac(1.0)};
        place_in_grid(input_row, 0, 0);
        input_body->add_child(input_row);
      }
      if (input_row) {
        place_in_grid(input_box, 0, 0);
        place_in_grid(scrollbar_box, 0, 1);
        input_row->add_children({input_box, scrollbar_box});
      }

      ui_choice_panel_configure_box(prompt_box, prompt_lines, false);
      ui_choice_panel_configure_box(
          input_box,
          ui_text_dialog_input_lines(wrapped_input, top_line, input_rows,
                                     secret),
          false);
      ui_choice_panel_configure_box(
          scrollbar_box,
          ui_choice_panel_scrollbar_lines(wrapped_input.size(),
                                          static_cast<std::size_t>(top_line),
                                          input_rows),
          false);
      ui_choice_panel_configure_box(
          footer_box,
          ui_text_dialog_footer_lines(allow_empty, text_width,
                                      wrapped_input.size(), top_line,
                                      input_rows),
          false);

      layout_tree(panel, Rect{startx, starty, width, height});
      render_tree(panel);

      const Rect input_rect = content_rect(*input_box);
      const int cursor_y =
          input_rect.y +
          std::clamp(cursor_line, 0, std::max(0, input_rect.h - 1));
      const int cursor_x =
          input_rect.x +
          std::clamp(cursor_col, 0, std::max(0, input_rect.w - 1));
      ::move(cursor_y, cursor_x);
      ::curs_set(1);
      ::refresh();
      dirty = false;
    }

    const int ch = getch();
    if (ch == ERR)
      continue;
    if (ch == KEY_RESIZE) {
      dirty = true;
      continue;
    }
    if (ch == 27) {
      if (cancelled)
        *cancelled = true;
      ::curs_set(0);
      return true;
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      if (!allow_empty && ui_dialog_trim_ascii(buffer).empty()) {
        ::beep();
        dirty = true;
        continue;
      }
      *out_value = ui_dialog_trim_ascii(buffer);
      ::curs_set(0);
      return true;
    }
    if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b' || ch == 8) {
      if (!buffer.empty()) {
        buffer.pop_back();
        dirty = true;
      }
      continue;
    }
    if (ch >= 32 && ch <= 126) {
      buffer.push_back(static_cast<char>(ch));
      dirty = true;
      continue;
    }
  }
}

[[nodiscard]] inline bool
ui_prompt_yes_no_dialog(const std::string &title, const std::string &prompt,
                        bool default_yes, bool *out_value, bool *cancelled) {
  if (out_value == nullptr)
    return false;
  std::vector<ui_choice_panel_option_t> options{
      ui_choice_panel_option_t{.label = "No", .enabled = true},
      ui_choice_panel_option_t{.label = "Yes", .enabled = true},
  };
  std::size_t selected = default_yes ? 1u : 0u;
  if (!ui_prompt_choice_panel(title, prompt, options, &selected, cancelled)) {
    return false;
  }
  if (cancelled != nullptr && *cancelled)
    return true;
  *out_value = (selected == 1u);
  return true;
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
