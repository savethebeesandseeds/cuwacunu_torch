#pragma once

#include <memory>

#include "iinuji/render/border_render.h"
#include "iinuji/render/buffer_box_render.h"
#include "iinuji/render/editor_box_render.h"
#include "iinuji/render/layout_core.h"
#include "iinuji/render/panel_render.h"
#include "iinuji/render/plot_box_render.h"
#include "iinuji/render/text_box_render.h"

namespace cuwacunu {
namespace iinuji {

/* Render whole tree (after layout_tree) */
inline void render_tree(const std::shared_ptr<iinuji_object_t>& node) {
  if (!node || !node->visible) return;

  // Focus frame (borderless focus highlight) must be drawn before content fill,
  // and content_rect() already reserves the 1-cell frame while focused.
  render_focus_frame_bg(*node);

  render_border(*node);

  if (std::dynamic_pointer_cast<plotBox_data_t>(node->data))            render_plot(*node);
  else if (std::dynamic_pointer_cast<bufferBox_data_t>(node->data))     render_buffer(*node);
  else if (std::dynamic_pointer_cast<editorBox_data_t>(node->data))     render_editor(*node);
  else if (std::dynamic_pointer_cast<textBox_data_t>(node->data))       render_text(*node);
  else                                                                  render_panel(*node);

  for (auto& ch : node->children) render_tree(ch);
}

} // namespace iinuji
} // namespace cuwacunu
