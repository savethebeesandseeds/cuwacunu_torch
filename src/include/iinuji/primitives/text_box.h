/* primitives/text_box.h */
#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "iinuji/primitives/primitive.h"

namespace cuwacunu {
namespace iinuji {

enum class text_align_t { Left, Center, Right };

enum class text_line_emphasis_t {
  None = 0,
  Accent,
  Success,
  Fatal,
  Error,
  MutedError,
  Warning,
  MutedWarning,
  Info,
  Debug,
};

struct styled_text_segment_t {
  std::string text{};
  text_line_emphasis_t emphasis{text_line_emphasis_t::None};
};

struct styled_text_line_t {
  std::string text{};
  text_line_emphasis_t emphasis{text_line_emphasis_t::None};
  std::vector<styled_text_segment_t> segments{};
  std::string background_color{};
};

inline std::string styled_text_line_text(const styled_text_line_t &line) {
  if (line.segments.empty())
    return line.text;
  std::size_t total = 0;
  for (const auto &segment : line.segments)
    total += segment.text.size();
  std::string out{};
  out.reserve(total);
  for (const auto &segment : line.segments)
    out += segment.text;
  return out;
}

inline styled_text_line_t make_segmented_styled_text_line(
    std::vector<styled_text_segment_t> segments,
    text_line_emphasis_t fallback_emphasis = text_line_emphasis_t::None) {
  styled_text_line_t line{};
  line.segments = std::move(segments);
  line.text = styled_text_line_text(line);
  line.emphasis = fallback_emphasis;
  return line;
}

struct text_box_data_t : public iinuji_data_t {
  std::string content;
  std::vector<styled_text_line_t> styled_lines{};
  bool wrap{true};
  text_align_t align{text_align_t::Left};
  int tracked_selected_row{-1};
  int tracked_selected_row_end{-1};
  int scroll_y{0};
  int scroll_x{0};

  text_box_data_t(std::string s, bool w = true,
                  text_align_t a = text_align_t::Left)
      : content(std::move(s)), wrap(w), align(a) {}

  void scroll_by(int dy, int dx = 0) {
    scroll_y = std::max(0, scroll_y + dy);
    scroll_x = std::max(0, scroll_x + dx);
  }

  void clear_styled_lines() { styled_lines.clear(); }
};

using textBox_data_t = text_box_data_t;

struct text_box_opts_t {
  std::string content{};
  bool wrap{true};
  text_align_t align{text_align_t::Left};
  iinuji_layout_t layout{};
  iinuji_style_t style{};
  focus_mode_t focus_mode{focus_mode_t::None};
};

inline std::shared_ptr<iinuji_object_t>
create_text_box(const std::string &id, text_box_opts_t opts = {}) {
  auto object = configure_primitive_object(
      create_object(id, true, opts.layout, opts.style),
      primitive_role_t::TextBox, opts.focus_mode);
  object->data = std::make_shared<text_box_data_t>(std::move(opts.content),
                                                   opts.wrap, opts.align);
  return object;
}

inline std::shared_ptr<iinuji_object_t>
create_text_box(const std::string &id, std::string content, bool wrap = true,
                text_align_t align = text_align_t::Left,
                const iinuji_layout_t &layout = {},
                const iinuji_style_t &style = {},
                focus_mode_t focus_mode = focus_mode_t::None) {
  text_box_opts_t opts{};
  opts.content = std::move(content);
  opts.wrap = wrap;
  opts.align = align;
  opts.layout = layout;
  opts.style = style;
  opts.focus_mode = focus_mode;
  return create_text_box(id, std::move(opts));
}

inline std::shared_ptr<iinuji_object_t>
create_input_line(const std::string &id, text_box_opts_t opts = {}) {
  opts.wrap = false;
  opts.align = text_align_t::Left;
  opts.focus_mode = focus_mode_t::Input;
  return create_text_box(id, std::move(opts));
}

inline std::shared_ptr<iinuji_object_t>
create_input_line(const std::string &id, std::string content,
                  const iinuji_layout_t &layout = {},
                  const iinuji_style_t &style = {}) {
  text_box_opts_t opts{};
  opts.content = std::move(content);
  opts.layout = layout;
  opts.style = style;
  return create_input_line(id, std::move(opts));
}

inline primitive_key_result_t handle_text_box_key(text_box_data_t &box,
                                                  int key) {
  primitive_key_result_t result{};
  switch (standard_key_action(key)) {
  case key_action_t::Backspace:
  case key_action_t::Delete:
    result.handled = true;
    if (!box.content.empty()) {
      box.content.pop_back();
      result.content_changed = true;
      result.status = "editing";
    }
    break;
  case key_action_t::Enter:
    result.handled = true;
    result.close_requested = true;
    break;
  case key_action_t::Cancel:
    result.handled = true;
    result.close_requested = true;
    break;
  case key_action_t::Printable:
    result.handled = true;
    box.content.push_back(static_cast<char>(key));
    result.content_changed = true;
    result.status = "editing";
    break;
  default:
    break;
  }
  return result;
}

inline primitive_key_result_t
handle_text_box_key(const std::shared_ptr<iinuji_object_t> &object, int key) {
  if (!object)
    return {};
  auto box = std::dynamic_pointer_cast<text_box_data_t>(object->data);
  if (!box)
    return {};
  return handle_text_box_key(*box, key);
}

} // namespace iinuji
} // namespace cuwacunu
