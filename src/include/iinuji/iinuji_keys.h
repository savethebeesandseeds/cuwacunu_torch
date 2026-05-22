/* iinuji_keys.h */
#pragma once

namespace cuwacunu {
namespace iinuji {

// Special keys live beyond the Unicode scalar range so they cannot collide with
// byte-oriented terminal input or future Unicode code point input.
inline constexpr int key_special_base = 0x110000;

inline constexpr int key_unknown = key_special_base;
inline constexpr int key_up = key_special_base + 1;
inline constexpr int key_down = key_special_base + 2;
inline constexpr int key_left = key_special_base + 3;
inline constexpr int key_right = key_special_base + 4;
inline constexpr int key_home = key_special_base + 5;
inline constexpr int key_end = key_special_base + 6;
inline constexpr int key_page_up = key_special_base + 7;
inline constexpr int key_page_down = key_special_base + 8;
inline constexpr int key_backspace = key_special_base + 9;
inline constexpr int key_delete = key_special_base + 10;
inline constexpr int key_enter = key_special_base + 11;

inline constexpr int key_ctrl_a = 1;
inline constexpr int key_ctrl_b = 2;
inline constexpr int key_ctrl_d = 4;
inline constexpr int key_ctrl_e = 5;
inline constexpr int key_ctrl_f = 6;
inline constexpr int key_ctrl_k = 11;
inline constexpr int key_ctrl_n = 14;
inline constexpr int key_ctrl_p = 16;
inline constexpr int key_ctrl_q = 17;
inline constexpr int key_ctrl_r = 18;
inline constexpr int key_ctrl_s = 19;
inline constexpr int key_ctrl_w = 23;
inline constexpr int key_ctrl_y = 25;
inline constexpr int key_ctrl_z = 26;
inline constexpr int key_escape = 27;

enum class key_action_t {
  None,
  MoveUp,
  MoveDown,
  MoveLeft,
  MoveRight,
  Home,
  End,
  PageUp,
  PageDown,
  Backspace,
  Delete,
  Enter,
  Tab,
  DeleteToEndOfLine,
  DeletePreviousWord,
  Reload,
  Save,
  Redo,
  Undo,
  Close,
  Cancel,
  Printable,
};

[[nodiscard]] inline bool is_enter_key(int key) {
  return key == key_enter || key == '\n' || key == '\r';
}

[[nodiscard]] inline bool is_backspace_key(int key) {
  return key == key_backspace || key == 127 || key == '\b';
}

[[nodiscard]] inline bool is_printable_ascii_key(int key) {
  return key >= 32 && key <= 126;
}

[[nodiscard]] inline key_action_t standard_key_action(int key) {
  switch (key) {
  case key_up:
  case key_ctrl_p:
    return key_action_t::MoveUp;
  case key_down:
  case key_ctrl_n:
    return key_action_t::MoveDown;
  case key_left:
  case key_ctrl_b:
    return key_action_t::MoveLeft;
  case key_right:
  case key_ctrl_f:
    return key_action_t::MoveRight;
  case key_home:
  case key_ctrl_a:
    return key_action_t::Home;
  case key_end:
  case key_ctrl_e:
    return key_action_t::End;
  case key_page_up:
    return key_action_t::PageUp;
  case key_page_down:
    return key_action_t::PageDown;
  case key_backspace:
  case 127:
  case '\b':
    return key_action_t::Backspace;
  case key_delete:
  case key_ctrl_d:
    return key_action_t::Delete;
  case key_enter:
  case '\n':
  case '\r':
    return key_action_t::Enter;
  case '\t':
    return key_action_t::Tab;
  case key_ctrl_k:
    return key_action_t::DeleteToEndOfLine;
  case key_ctrl_w:
    return key_action_t::DeletePreviousWord;
  case key_ctrl_r:
    return key_action_t::Reload;
  case key_ctrl_s:
    return key_action_t::Save;
  case key_ctrl_y:
    return key_action_t::Redo;
  case key_ctrl_z:
    return key_action_t::Undo;
  case key_ctrl_q:
    return key_action_t::Close;
  case key_escape:
    return key_action_t::Cancel;
  default:
    return is_printable_ascii_key(key) ? key_action_t::Printable
                                       : key_action_t::None;
  }
}

} // namespace iinuji
} // namespace cuwacunu
