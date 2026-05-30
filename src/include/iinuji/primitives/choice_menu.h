#pragma once

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "iinuji/primitives/text_box.h"

namespace cuwacunu {
namespace iinuji {
namespace primitives {

inline constexpr int kChoiceMenuHeaderRows = 5;

struct choice_menu_option_t {
  std::string label{};
  std::string detail{};
  bool enabled{true};
  text_line_emphasis_t label_emphasis{text_line_emphasis_t::None};
  text_line_emphasis_t detail_emphasis{text_line_emphasis_t::Info};
  std::string background_color{};
};

struct choice_menu_model_t {
  std::string title{};
  std::vector<choice_menu_option_t> options{};
  std::size_t selected{0};
  int scroll_y{0};
  std::size_t visible_reserve{10u};
  std::size_t label_width{46u};
  std::size_t detail_width{58u};
};

struct choice_menu_columns_t {
  std::vector<styled_text_line_t> labels{};
  std::vector<styled_text_line_t> details{};
};

inline std::string choice_menu_fit(std::string text, std::size_t width) {
  if (text.size() <= width)
    return text;
  if (width == 0u)
    return {};
  if (width == 1u)
    return "~";
  text.resize(width - 1u);
  text.push_back('~');
  return text;
}

inline std::size_t choice_menu_clamped_selection(std::size_t selected,
                                                 std::size_t total) {
  if (total == 0u)
    return 0u;
  return std::min(selected, total - 1u);
}

inline int choice_menu_scroll_for_selection(std::size_t selected, int scroll_y,
                                            std::size_t total,
                                            std::size_t visible_reserve) {
  if (total == 0u)
    return 0;
  const int visible =
      std::max(1, static_cast<int>(std::min(visible_reserve, total)));
  const int selected_row =
      static_cast<int>(choice_menu_clamped_selection(selected, total));
  const int max_top = std::max(0, static_cast<int>(total) - visible);
  int top =
      scroll_y > kChoiceMenuHeaderRows ? scroll_y - kChoiceMenuHeaderRows : 0;
  top = std::clamp(top, 0, max_top);
  if (selected_row < top || selected_row >= top + visible)
    top = std::clamp(selected_row, 0, max_top);
  return top == 0 ? 0 : kChoiceMenuHeaderRows + top;
}

inline std::string choice_menu_range_text(std::size_t total, std::size_t top,
                                          std::size_t visible) {
  if (total == 0u)
    return "0-0/0";
  const std::size_t last = std::min(total, top + visible);
  return std::to_string(top + 1u) + "-" + std::to_string(last) + "/" +
         std::to_string(total);
}

inline std::string choice_menu_scrollbar_text(std::size_t total,
                                              std::size_t top,
                                              std::size_t visible) {
  constexpr std::size_t kWidth = 10u;
  if (total == 0u)
    return "░░░░░░░░░░";
  if (total <= visible)
    return "██████████";

  const std::size_t thumb =
      std::max<std::size_t>(1u, (visible * kWidth + total - 1u) / total);
  const std::size_t max_top = total > visible ? total - visible : 0u;
  const std::size_t span = kWidth > thumb ? kWidth - thumb : 0u;
  const std::size_t thumb_top =
      max_top == 0u ? 0u
                    : std::min(span, (top * span + max_top / 2u) / max_top);

  std::string out{};
  for (std::size_t i = 0; i < kWidth; ++i)
    out += (i >= thumb_top && i < thumb_top + thumb) ? "█" : "░";
  return out;
}

inline void choice_menu_push_row(
    choice_menu_columns_t &columns, std::string label, std::string detail,
    std::size_t label_width, std::size_t detail_width,
    text_line_emphasis_t label_emphasis = text_line_emphasis_t::None,
    text_line_emphasis_t detail_emphasis = text_line_emphasis_t::Info,
    std::string background_color = {}) {
  columns.labels.push_back(styled_text_line_t{
      .text = choice_menu_fit(std::move(label), label_width),
      .emphasis = label_emphasis,
      .background_color = background_color,
  });
  columns.details.push_back(styled_text_line_t{
      .text = choice_menu_fit(std::move(detail), detail_width),
      .emphasis = detail_emphasis,
      .background_color = std::move(background_color),
  });
}

inline choice_menu_columns_t choice_menu_columns(choice_menu_model_t model) {
  choice_menu_columns_t columns{};
  const std::size_t total = model.options.size();
  model.selected = choice_menu_clamped_selection(model.selected, total);
  model.scroll_y = choice_menu_scroll_for_selection(
      model.selected, model.scroll_y, total, model.visible_reserve);

  choice_menu_push_row(columns, model.title.empty() ? "MENU" : model.title, "",
                       model.label_width, model.detail_width,
                       text_line_emphasis_t::Accent,
                       text_line_emphasis_t::Debug);
  choice_menu_push_row(columns, "Enter", "run selected option",
                       model.label_width, model.detail_width,
                       text_line_emphasis_t::Warning);
  choice_menu_push_row(columns, "Esc / [x]", "close menu", model.label_width,
                       model.detail_width, text_line_emphasis_t::Warning);
  choice_menu_push_row(columns, "Up/Down or j/k",
                       "move selection; PgUp/PgDn page; Home/End edge",
                       model.label_width, model.detail_width);

  const std::size_t visible =
      total == 0u ? 0u : std::min(model.visible_reserve, total);
  const std::size_t top =
      model.scroll_y > kChoiceMenuHeaderRows
          ? static_cast<std::size_t>(model.scroll_y - kChoiceMenuHeaderRows)
          : 0u;
  choice_menu_push_row(
      columns, "visible",
      choice_menu_range_text(total, top, visible) + " shown | " +
          choice_menu_scrollbar_text(total, top, visible),
      model.label_width, model.detail_width, text_line_emphasis_t::Accent);

  for (std::size_t i = 0; i < model.options.size(); ++i) {
    const auto &option = model.options[i];
    const bool selected = i == model.selected;
    const bool enabled = option.enabled;
    text_line_emphasis_t label_emphasis =
        selected ? text_line_emphasis_t::Warning : option.label_emphasis;
    text_line_emphasis_t detail_emphasis =
        selected ? text_line_emphasis_t::Warning : option.detail_emphasis;
    if (!enabled) {
      label_emphasis = text_line_emphasis_t::MutedWarning;
      detail_emphasis = text_line_emphasis_t::MutedWarning;
    }
    choice_menu_push_row(columns, (selected ? "> " : "  ") + option.label,
                         option.detail, model.label_width, model.detail_width,
                         label_emphasis, detail_emphasis,
                         selected ? "#2A2418" : option.background_color);
  }
  return columns;
}

} // namespace primitives
} // namespace iinuji
} // namespace cuwacunu
