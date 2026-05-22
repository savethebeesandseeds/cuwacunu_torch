/* iinuji_types.h */
#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cuwacunu {
namespace iinuji {

struct Rect {
  int x{0};
  int y{0};
  int w{0};
  int h{0};
};

enum class unit_t { Px, Frac };

struct len_spec {
  unit_t u{unit_t::Frac};
  double v{1.0};

  static len_spec px(int p) {
    return len_spec{unit_t::Px, static_cast<double>(p)};
  }

  static len_spec frac(double f) { return len_spec{unit_t::Frac, f}; }
};

struct grid_spec_t {
  std::vector<len_spec> rows;
  std::vector<len_spec> cols;
  int gap_row{0};
  int gap_col{0};
  int pad_left{0};
  int pad_right{0};
  int pad_top{0};
  int pad_bottom{0};
};

enum class layout_mode_t { Absolute, Normalized, Dock, GridCell };
enum class dock_t { None, Top, Bottom, Left, Right, Fill };

struct iinuji_layout_t {
  layout_mode_t mode{layout_mode_t::Normalized};

  double x{0};
  double y{0};
  double width{1};
  double height{1};
  bool normalized{true};

  dock_t dock{dock_t::None};
  len_spec dock_size{len_spec::frac(0.2)};

  int grid_row{0};
  int grid_col{0};
  int grid_row_span{1};
  int grid_col_span{1};

  int pad_left{0};
  int pad_right{0};
  int pad_top{0};
  int pad_bottom{0};
};

struct iinuji_style_t {
  std::string label_color{"white"};
  std::string background_color{"black"};
  bool border{false};
  std::string border_color{"gray"};
  bool bold{false};
  bool inverse{false};
  std::string title{};
};

enum class primitive_role_t {
  Object,
  Container,
  Panel,
  TextBox,
  BufferBox,
  EditorBox,
  PlotBox,
  ImageBox,
  Animation,
  ArtText,
  Custom,
};

enum class focus_mode_t {
  None,
  Focusable,
  Input,
};

struct iinuji_data_t {
  virtual ~iinuji_data_t() = default;
};

enum class event_type {
  Key,
  MouseDown,
  MouseUp,
  MouseMove,
  Wheel,
  Resize,
  Timer,
  Custom
};

struct event_t {
  event_type type{event_type::Custom};
  int key{0};
  int x{0};
  int y{0};
  int button{0};
  int delta{0};
  int width{0};
  int height{0};
  std::string name;
  std::string payload;
};

struct iinuji_object_t;
struct iinuji_state_t;

using event_handler_t = std::function<void(
    iinuji_state_t &, std::shared_ptr<iinuji_object_t> &, const event_t &)>;

struct iinuji_object_t : public std::enable_shared_from_this<iinuji_object_t> {
  long id_num{0};
  std::string id;
  bool visible{true};
  int z_index{0};

  primitive_role_t primitive_role{primitive_role_t::Object};
  focus_mode_t focus_mode{focus_mode_t::None};
  bool focusable{false};
  bool focused{false};

  iinuji_layout_t layout{};
  iinuji_style_t style{};
  std::shared_ptr<iinuji_data_t> data{};

  Rect screen{};
  std::shared_ptr<grid_spec_t> grid;

  std::weak_ptr<iinuji_object_t> parent;
  std::vector<std::shared_ptr<iinuji_object_t>> children;

  std::unordered_map<event_type, std::vector<event_handler_t>> listeners;

  void add_child(const std::shared_ptr<iinuji_object_t> &child) {
    if (!child)
      return;
    child->parent = shared_from_this();
    children.push_back(child);
  }

  void
  add_children(const std::vector<std::shared_ptr<iinuji_object_t>> &items) {
    for (const auto &child : items)
      add_child(child);
  }

  void on(event_type type, const event_handler_t &fn) {
    listeners[type].push_back(fn);
  }

  void off(event_type type) { listeners.erase(type); }
  void toggle_visible() { visible = !visible; }
};

[[nodiscard]] inline bool object_accepts_focus(const iinuji_object_t &object) {
  return object.focusable || object.focus_mode != focus_mode_t::None;
}

[[nodiscard]] inline bool object_has_focus(const iinuji_object_t &object) {
  return object.focused && object_accepts_focus(object);
}

inline void set_object_focus_mode(iinuji_object_t &object,
                                  focus_mode_t focus_mode) {
  object.focus_mode = focus_mode;
  object.focusable = focus_mode != focus_mode_t::None;
  if (!object.focusable)
    object.focused = false;
}

inline void
set_object_focus_mode(const std::shared_ptr<iinuji_object_t> &object,
                      focus_mode_t focus_mode) {
  if (object)
    set_object_focus_mode(*object, focus_mode);
}

struct iinuji_state_t {
  std::shared_ptr<iinuji_object_t> root;
  std::shared_ptr<iinuji_object_t> focused;
  bool running{true};
  bool in_ncurses_mode{true};
  int last_key{0};

  std::unordered_map<std::string, std::weak_ptr<iinuji_object_t>> id_index;

  std::shared_ptr<iinuji_object_t> by_id(const std::string &name) const {
    auto it = id_index.find(name);
    if (it == id_index.end())
      return nullptr;
    return it->second.lock();
  }

  void register_id(const std::string &name,
                   const std::shared_ptr<iinuji_object_t> &object) {
    if (!name.empty() && object) {
      id_index[name] = object;
      object->id = name;
    }
  }
};

inline void
collect_focusable_objects(const std::shared_ptr<iinuji_object_t> &node,
                          std::vector<std::shared_ptr<iinuji_object_t>> &out) {
  if (!node || !node->visible)
    return;
  if (object_accepts_focus(*node))
    out.push_back(node);
  for (const auto &child : node->children)
    collect_focusable_objects(child, out);
}

[[nodiscard]] inline std::vector<std::shared_ptr<iinuji_object_t>>
focusable_objects(const std::shared_ptr<iinuji_object_t> &root) {
  std::vector<std::shared_ptr<iinuji_object_t>> out;
  collect_focusable_objects(root, out);
  return out;
}

[[nodiscard]] inline bool
set_focused_object(iinuji_state_t &state,
                   const std::shared_ptr<iinuji_object_t> &next) {
  if (next && !object_accepts_focus(*next))
    return false;
  if (state.focused)
    state.focused->focused = false;
  state.focused = next;
  if (state.focused) {
    if (state.focused->focus_mode == focus_mode_t::None)
      state.focused->focus_mode = focus_mode_t::Focusable;
    state.focused->focusable = true;
    state.focused->focused = true;
  }
  return true;
}

[[nodiscard]] inline bool focus_first_object(iinuji_state_t &state) {
  auto items = focusable_objects(state.root);
  if (items.empty())
    return set_focused_object(state, nullptr);
  return set_focused_object(state, items.front());
}

[[nodiscard]] inline bool focus_next_object(iinuji_state_t &state,
                                            int direction = 1) {
  auto items = focusable_objects(state.root);
  if (items.empty())
    return set_focused_object(state, nullptr);

  if (direction == 0)
    direction = 1;

  int current = -1;
  for (std::size_t i = 0; i < items.size(); ++i) {
    if (items[i] == state.focused) {
      current = static_cast<int>(i);
      break;
    }
  }

  const int count = static_cast<int>(items.size());
  const int start = current < 0 ? (direction > 0 ? -1 : 0) : current;
  const int next = (start + direction + count) % count;
  return set_focused_object(state, items[static_cast<std::size_t>(next)]);
}

[[nodiscard]] inline bool focus_prev_object(iinuji_state_t &state) {
  return focus_next_object(state, -1);
}

inline std::shared_ptr<iinuji_state_t>
initialize_iinuji_state(const std::shared_ptr<iinuji_object_t> &root,
                        bool in_ncurses_mode = true) {
  auto state = std::make_shared<iinuji_state_t>();
  state->root = root;
  state->running = true;
  state->in_ncurses_mode = in_ncurses_mode;
  (void)focus_first_object(*state);
  return state;
}

inline std::shared_ptr<iinuji_object_t>
create_object(const std::string &id = "", bool visible = true,
              const iinuji_layout_t &layout = {},
              const iinuji_style_t &style = {}) {
  static long counter = 0;
  auto object = std::make_shared<iinuji_object_t>();
  object->id_num = counter++;
  object->visible = visible;
  object->layout = layout;
  object->style = style;
  object->id = id;
  return object;
}

} // namespace iinuji
} // namespace cuwacunu
