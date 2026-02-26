#pragma once

#include "iinuji/render/layout_core.h"

namespace cuwacunu {
namespace iinuji {

inline void render_panel(const iinuji_object_t& obj) {
  Rect r = content_rect(obj);
  auto* R = get_renderer();
  if (!R) return;
  short pair = (short)get_color_pair(obj.style.label_color, obj.style.background_color);
  R->fillRect(r.y, r.x, r.h, r.w, pair);
}

} // namespace iinuji
} // namespace cuwacunu
