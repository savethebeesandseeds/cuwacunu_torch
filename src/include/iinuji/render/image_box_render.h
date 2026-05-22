/* render/image_box_render.h */
#pragma once

#include <memory>

#include "iinuji/primitives/image.h"
#include "iinuji/render/layout_core.h"

namespace cuwacunu {
namespace iinuji {

inline void render_image_box(const iinuji_object_t &obj) {
  auto image_box = std::dynamic_pointer_cast<image_box_data_t>(obj.data);
  if (!image_box)
    return;
  Rect r = content_rect(obj);
  render_image_grayscale(image_box->image, r.x, r.y, r.w, r.h,
                         image_box->render_options);
}

} // namespace iinuji
} // namespace cuwacunu
