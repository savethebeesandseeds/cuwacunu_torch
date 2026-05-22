/* primitives/primitive.h */
#pragma once

#include <memory>
#include <string>

#include "iinuji/iinuji_keys.h"
#include "iinuji/iinuji_types.h"

namespace cuwacunu {
namespace iinuji {

struct primitive_key_result_t {
  bool handled{false};
  bool content_changed{false};
  bool viewport_changed{false};
  bool close_requested{false};
  std::string status{};
};

inline std::shared_ptr<iinuji_object_t>
configure_primitive_object(const std::shared_ptr<iinuji_object_t> &object,
                           primitive_role_t role,
                           focus_mode_t focus_mode = focus_mode_t::None) {
  if (!object)
    return object;
  object->primitive_role = role;
  set_object_focus_mode(object, focus_mode);
  return object;
}

[[nodiscard]] inline bool primitive_has_focus(const iinuji_object_t &object) {
  return object_has_focus(object);
}

[[nodiscard]] inline bool
primitive_has_focus(const std::shared_ptr<iinuji_object_t> &object) {
  return object != nullptr && primitive_has_focus(*object);
}

} // namespace iinuji
} // namespace cuwacunu
