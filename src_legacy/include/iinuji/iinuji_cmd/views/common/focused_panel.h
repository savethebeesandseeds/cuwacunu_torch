#pragma once

#include <initializer_list>
#include <memory>

#include "iinuji/iinuji_cmd/views/common/viewport.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool panel_has_viewport_target(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &panel) {
  if (!panel || !panel->visible)
    return false;
  const auto target = viewport_content_target(panel);
  if (!target || !target->visible)
    return false;
  return as<cuwacunu::iinuji::textBox_data_t>(target) ||
         as<cuwacunu::iinuji::editorBox_data_t>(target);
}

inline std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
first_viewport_panel(std::initializer_list<
                     std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>>
                         candidates) {
  for (const auto &candidate : candidates) {
    if (panel_has_viewport_target(candidate))
      return candidate;
  }
  return nullptr;
}

inline bool object_screen_contains_point(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &object, int mx,
    int my) {
  if (!object || !object->visible)
    return false;
  const Rect r = object->screen;
  return r.w > 0 && r.h > 0 && mx >= r.x && mx < (r.x + r.w) && my >= r.y &&
         my < (r.y + r.h);
}

inline std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
viewport_panel_at_point(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &root, int mx,
    int my) {
  if (!object_screen_contains_point(root, mx, my))
    return nullptr;
  for (auto it = root->children.rbegin(); it != root->children.rend(); ++it) {
    if (auto found = viewport_panel_at_point(*it, mx, my))
      return found;
  }
  return panel_has_viewport_target(root) ? root : nullptr;
}

inline std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
hover_scroll_panel(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right, int mx,
    int my) {
  return first_viewport_panel(
      {viewport_panel_at_point(right, mx, my),
       viewport_panel_at_point(left, mx, my)});
}

inline std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
active_scroll_panel(
    const CmdState &state,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right) {
  auto left_main = [&]() { return find_visible_object_by_id(left, "left_main"); };
  auto left_nav_panel = [&]() {
    return find_visible_object_by_id(left, "left_nav_panel");
  };
  auto left_worklist_panel = [&]() {
    return find_visible_object_by_id(left, "left_worklist_panel");
  };
  auto right_main = [&]() {
    return find_visible_object_by_id(right, "right_main");
  };
  auto right_aux = [&]() { return find_visible_object_by_id(right, "right_aux"); };
  auto fallback_panel = [&]() {
    return first_viewport_panel({right_main(), left_worklist_panel(),
                                 left_nav_panel(), left_main(), right_aux()});
  };

  switch (state.screen) {
  case ScreenMode::Home:
    if (auto panel =
            first_viewport_panel({left_main(), right_main(), right_aux()}))
      return panel;
    return fallback_panel();
  case ScreenMode::Workbench:
    if (workbench_is_worklist_focus(state.workbench.focus)) {
      if (auto panel = left_worklist_panel())
        return panel;
    } else if (auto panel = left_nav_panel()) {
      return panel;
    }
    return fallback_panel();
  case ScreenMode::Runtime:
    if (runtime_event_viewer_is_open(state) ||
        runtime_text_log_viewer_is_open(state)) {
      if (auto panel = first_viewport_panel({right_main(), right_aux()}))
        return panel;
      return fallback_panel();
    }
    if (runtime_is_worklist_focus(state.runtime.focus)) {
      if (auto panel = left_worklist_panel())
        return panel;
    } else if (runtime_is_menu_focus(state.runtime.focus)) {
      if (auto panel = left_nav_panel())
        return panel;
    }
    return fallback_panel();
  case ScreenMode::Lattice:
    if (lattice_is_worklist_focus(state)) {
      if (auto panel = left_worklist_panel())
        return panel;
    } else if (auto panel = left_nav_panel()) {
      return panel;
    }
    return fallback_panel();
  case ScreenMode::ShellLogs:
    if (auto panel = first_viewport_panel({left_main(), right_main()}))
      return panel;
    return fallback_panel();
  case ScreenMode::Config:
    if (state.config.editor_focus && state.config.editor) {
      if (auto panel =
              first_viewport_panel({right_main(), left_worklist_panel()}))
        return panel;
      return fallback_panel();
    }
    if (config_is_file_focus(state.config)) {
      if (auto panel = left_worklist_panel())
        return panel;
    }
    if (auto panel = left_nav_panel())
      return panel;
    return fallback_panel();
  }
  return fallback_panel();
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
