/* primitives/buffer_box.h */
#pragma once

#include <algorithm>
#include <cstddef>
#include <deque>
#include <memory>
#include <string>
#include <utility>

#include "iinuji/primitives/primitive.h"

namespace cuwacunu {
namespace iinuji {

enum class buffer_dir_t { UpDown, DownUp };

struct buffer_line_t {
  std::string text;
  std::string label;
  std::string color;
};

struct buffer_box_data_t : public iinuji_data_t {
  std::deque<buffer_line_t> lines;
  std::size_t capacity{1000};
  buffer_dir_t dir{buffer_dir_t::UpDown};
  int scroll{0};
  bool follow_tail{true};
  int wrap_width_last{0};

  buffer_box_data_t(std::size_t cap = 1000,
                    buffer_dir_t d = buffer_dir_t::UpDown)
      : capacity(std::max<std::size_t>(1, cap)), dir(d) {}

  void push_line(std::string s) { push_line(std::move(s), "", ""); }

  void push_line(std::string s, std::string label, std::string color) {
    if (!s.empty() && s.back() == '\r')
      s.pop_back();

    const bool was_at_tail = scroll == 0;

    auto estimate_added_rows = [&](std::size_t text_len,
                                   std::size_t prefix_len) -> int {
      const int width = wrap_width_last;
      if (width <= 0)
        return 1;
      int available = width - static_cast<int>(prefix_len);
      if (available <= 0)
        available = 1;
      if (text_len == 0)
        return 1;
      return 1 + static_cast<int>((text_len - 1) /
                                  static_cast<std::size_t>(available));
    };

    buffer_line_t line;
    line.text = std::move(s);
    line.label = std::move(label);
    line.color = std::move(color);

    lines.push_back(std::move(line));

    while (lines.size() > capacity)
      lines.pop_front();

    const buffer_line_t &stored = lines.back();
    if (!was_at_tail) {
      follow_tail = false;
      std::size_t prefix_len = 0;
      if (!stored.label.empty())
        prefix_len = stored.label.size() + 3;
      scroll += estimate_added_rows(stored.text.size(), prefix_len);
    } else {
      follow_tail = true;
      scroll = 0;
    }

    if (scroll < 0)
      scroll = 0;
  }

  void clear() {
    lines.clear();
    scroll = 0;
    follow_tail = true;
  }

  void scroll_by(int delta) {
    scroll = std::max(0, scroll + delta);
    follow_tail = scroll == 0;
  }

  void jump_tail() {
    scroll = 0;
    follow_tail = true;
  }
};

using bufferBox_data_t = buffer_box_data_t;

struct buffer_box_opts_t {
  std::size_t capacity{1000};
  buffer_dir_t dir{buffer_dir_t::UpDown};
  iinuji_layout_t layout{};
  iinuji_style_t style{};
  focus_mode_t focus_mode{focus_mode_t::None};
};

inline std::shared_ptr<iinuji_object_t>
create_buffer_box(const std::string &id, buffer_box_opts_t opts = {}) {
  auto object = configure_primitive_object(
      create_object(id, true, opts.layout, opts.style),
      primitive_role_t::BufferBox, opts.focus_mode);
  object->data = std::make_shared<buffer_box_data_t>(opts.capacity, opts.dir);
  return object;
}

inline std::shared_ptr<iinuji_object_t>
create_buffer_box(const std::string &id, std::size_t capacity, buffer_dir_t dir,
                  const iinuji_layout_t &layout = {},
                  const iinuji_style_t &style = {},
                  focus_mode_t focus_mode = focus_mode_t::None) {
  buffer_box_opts_t opts{};
  opts.capacity = capacity;
  opts.dir = dir;
  opts.layout = layout;
  opts.style = style;
  opts.focus_mode = focus_mode;
  return create_buffer_box(id, opts);
}

inline primitive_key_result_t handle_buffer_box_key(buffer_box_data_t &box,
                                                    int key) {
  primitive_key_result_t result{};
  switch (standard_key_action(key)) {
  case key_action_t::MoveUp:
    box.scroll_by(1);
    result.handled = true;
    result.viewport_changed = true;
    break;
  case key_action_t::MoveDown:
    box.scroll_by(-1);
    result.handled = true;
    result.viewport_changed = true;
    break;
  case key_action_t::PageUp:
    box.scroll_by(10);
    result.handled = true;
    result.viewport_changed = true;
    break;
  case key_action_t::PageDown:
    box.scroll_by(-10);
    result.handled = true;
    result.viewport_changed = true;
    break;
  case key_action_t::End:
    box.jump_tail();
    result.handled = true;
    result.viewport_changed = true;
    break;
  default:
    break;
  }
  return result;
}

inline primitive_key_result_t
handle_buffer_box_key(const std::shared_ptr<iinuji_object_t> &object, int key) {
  if (!object)
    return {};
  auto box = std::dynamic_pointer_cast<buffer_box_data_t>(object->data);
  if (!box)
    return {};
  return handle_buffer_box_key(*box, key);
}

} // namespace iinuji
} // namespace cuwacunu
