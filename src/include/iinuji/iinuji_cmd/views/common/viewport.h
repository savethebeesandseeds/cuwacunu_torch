#pragma once

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>

#include "iinuji/iinuji_cmd/views/common/base.h"
#include "iinuji/render/layout_core.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
find_visible_object_by_id(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& root,
    std::string_view id) {
  if (!root || !root->visible || id.empty()) return nullptr;
  if (root->id == id) return root;
  for (const auto& child : root->children) {
    if (auto found = find_visible_object_by_id(child, id); found) return found;
  }
  return nullptr;
}

inline std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
viewport_content_target(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& object) {
  if (!object) return nullptr;
  if (cuwacunu::iinuji::is_panel_object(object)) {
    return panel_content_target(object);
  }
  return object;
}

inline bool scroll_viewport_by(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& object, int dy,
    int dx) {
  if (!object || !object->visible || (dy == 0 && dx == 0)) return false;
  auto target = viewport_content_target(object);
  if (!target || !target->visible) return false;

  if (auto tb = as<cuwacunu::iinuji::textBox_data_t>(target)) {
    tb->scroll_by(dy, dx);
    return true;
  }
  if (auto ed = as<cuwacunu::iinuji::editorBox_data_t>(target)) {
    if (dy != 0) ed->top_line = std::max(0, ed->top_line + dy);
    if (dx != 0) ed->left_col = std::max(0, ed->left_col + dx);
    return true;
  }
  return false;
}

inline bool scroll_viewport_home(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& object) {
  if (!object || !object->visible) return false;
  auto target = viewport_content_target(object);
  if (!target || !target->visible) return false;

  if (auto tb = as<cuwacunu::iinuji::textBox_data_t>(target)) {
    tb->scroll_y = 0;
    return true;
  }
  if (auto ed = as<cuwacunu::iinuji::editorBox_data_t>(target)) {
    ed->top_line = 0;
    return true;
  }
  return false;
}

inline bool scroll_viewport_end(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& object) {
  if (!object || !object->visible) return false;
  auto target = viewport_content_target(object);
  if (!target || !target->visible) return false;

  if (auto tb = as<cuwacunu::iinuji::textBox_data_t>(target)) {
    tb->scroll_y = std::numeric_limits<int>::max();
    return true;
  }
  if (auto ed = as<cuwacunu::iinuji::editorBox_data_t>(target)) {
    ed->top_line = std::numeric_limits<int>::max();
    return true;
  }
  return false;
}

inline void reveal_selected_text_row_if_changed(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box,
    int selected_row, int text_h, int max_scroll_y) {
  auto tb = as<cuwacunu::iinuji::textBox_data_t>(panel_content_target(box));
  if (!tb) return;

  const bool selection_changed = (tb->tracked_selected_row != selected_row);
  tb->tracked_selected_row = selected_row;

  if (selection_changed && text_h > 0) {
    const int top = std::max(0, tb->scroll_y);
    const int bottom = top + text_h - 1;
    if (selected_row < top) {
      tb->scroll_y = std::max(0, selected_row - 1);
    } else if (selected_row > bottom) {
      tb->scroll_y = std::min(max_scroll_y, selected_row - text_h + 2);
    }
  }

  tb->scroll_y = std::clamp(tb->scroll_y, 0, max_scroll_y);
}

inline void clear_selected_text_row_tracking(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box) {
  auto tb = as<cuwacunu::iinuji::textBox_data_t>(panel_content_target(box));
  if (!tb) return;
  tb->tracked_selected_row = -1;
}

inline void sync_plain_text_panel_selection(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box,
    const std::optional<std::size_t>& selected_line, std::size_t total_lines,
    bool reset_horizontal_scroll = false) {
  auto tb = as<cuwacunu::iinuji::textBox_data_t>(panel_content_target(box));
  if (!tb || !selected_line.has_value()) {
    clear_selected_text_row_tracking(box);
    return;
  }

  if (reset_horizontal_scroll) {
    tb->scroll_x = 0;
  }

  const Rect r = content_rect(*box);
  const int text_h = std::max(1, r.h);
  const int selected_row = static_cast<int>(*selected_line);
  const int max_scroll =
      std::max(0, static_cast<int>(total_lines) - text_h);
  reveal_selected_text_row_if_changed(box, selected_row, text_h, max_scroll);
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
