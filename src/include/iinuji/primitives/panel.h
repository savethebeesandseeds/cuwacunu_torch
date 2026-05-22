/* primitives/panel.h */
#pragma once

#include <memory>
#include <string>

#include "iinuji/primitives/primitive.h"

namespace cuwacunu {
namespace iinuji {

struct panel_data_t : public iinuji_data_t {};

struct panel_opts_t {
  iinuji_layout_t layout{};
  iinuji_style_t style{};
  focus_mode_t focus_mode{focus_mode_t::None};
};

inline std::shared_ptr<iinuji_object_t> create_panel(const std::string &id,
                                                     panel_opts_t opts = {}) {
  auto object = configure_primitive_object(
      create_object(id, true, opts.layout, opts.style), primitive_role_t::Panel,
      opts.focus_mode);
  object->data = std::make_shared<panel_data_t>();

  auto body = configure_primitive_object(
      create_object(id.empty() ? std::string{} : id + ".body", true,
                    iinuji_layout_t{},
                    iinuji_style_t{opts.style.label_color,
                                   opts.style.background_color,
                                   false,
                                   opts.style.border_color,
                                   opts.style.bold,
                                   opts.style.inverse,
                                   {}}),
      primitive_role_t::Container);
  body->layout.mode = layout_mode_t::Dock;
  body->layout.dock = dock_t::Fill;
  object->add_child(body);
  return object;
}

inline std::shared_ptr<iinuji_object_t>
create_panel(const std::string &id, const iinuji_layout_t &layout,
             const iinuji_style_t &style = {}) {
  panel_opts_t opts{};
  opts.layout = layout;
  opts.style = style;
  return create_panel(id, opts);
}

inline bool is_panel_object(const std::shared_ptr<iinuji_object_t> &object) {
  return object != nullptr &&
         std::dynamic_pointer_cast<panel_data_t>(object->data) != nullptr;
}

inline std::shared_ptr<iinuji_object_t>
panel_body_object(const std::shared_ptr<iinuji_object_t> &object) {
  if (!is_panel_object(object) || object->children.empty())
    return object;
  return object->children.front();
}

} // namespace iinuji
} // namespace cuwacunu
