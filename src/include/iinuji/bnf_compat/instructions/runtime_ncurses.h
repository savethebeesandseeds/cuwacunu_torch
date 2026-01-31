#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <ncursesw/ncurses.h>

#include "iinuji/bnf_compat/iinuji_instructions.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"

namespace cuwacunu::iinuji {

// NOTE: This helper is defined later in this header, but we call it from
// member functions defined *inside* the class body, so we must declare it first.
inline void render_root(std::shared_ptr<iinuji_object_t> root);

inline int parse_screen_key_to_ncurses(const std::string& key_raw)
{
  if (key_raw.empty() || key_raw == "<empty>") return -1;

  std::string low = key_raw;
  std::transform(low.begin(), low.end(), low.begin(),
                 [](unsigned char c){ return (char)std::tolower(c); });

  // F0 is a logical key used for the default screen; do NOT map it to ncurses.
  if (low == "f0" || low == "f+0") return -1;

  if (key_raw.rfind("F+", 0) == 0) {
    try {
      int n = std::stoi(key_raw.substr(2));
      if (n > 0) return KEY_F(n);
    } catch (...) {}
    return -1;
  }

  if (key_raw.size() == 1) return (unsigned char)key_raw[0];
  return -1;
}

// Built-in default screen DSL (used only if the user does NOT provide a screen with __key Default)
static constexpr const char* kBuiltinDefaultScreenDSL = R"DSL(
; default screen
; Note: Default is specified when __key F+0
SCREEN _screen
  __tickness 2
  __name default_screen
  __text_color #ffcc66
  __line_color #ffaa33
  __back_color #0e0e0e
  __key F+0

  ; default screen panel container
  PANEL _rectangle
    __coords 0,0
    __z_index 0
    __shape 100,100
    __title true "Default Screen"
    __border true
    __line_color #cc6666

    ; Informational fallback label
    FIGURE _label
      __coords 25,25
      __shape 50,50
      __value "This screen is not available.\n Modify the iinuji_renderings.instruction file to add it."
      __type normal
      __border true
    ENDFIGURE
  ENDPANEL
ENDSCREEN
)DSL";

static inline bool is_default_key_raw(const std::string& s)
{
  if (s.empty() || s == "<empty>") return false;
  std::string t = s;
  std::transform(t.begin(), t.end(), t.begin(),
                 [](unsigned char c){ return (char)std::tolower(c); });
  return (t == "f0" || t == "f+0"); // accept either spelling
}

struct ncurses_instruction_session_t {
  NcursesApp& app;

  // Owned “effective instruction” so we can inject a built-in Default screen if missing.
  cuwacunu::camahjucunu::iinuji_renderings_instruction_t inst_eff;

  IInstructionsData& data;
  instructions_build_opts_t bopt{};
  instructions_validate_opts_t vopt{};

  std::unordered_map<int, std::size_t> screen_for_key;

  std::size_t active_screen = 0;
  std::vector<instructions_build_result_t> built_screens{};
  std::vector<bool> built_ok{};
  std::unique_ptr<sys_stream_router_t> router{};

  // Active-screen buffer pointer for scrolling
  std::string buf_id;
  std::shared_ptr<iinuji_object_t> buf_obj;

  // Default screen support
  std::size_t default_screen_index = (std::size_t)-1;
  bool default_is_builtin = false;

  // Diagnostics
  instructions_diag_t last_diag{};

  enum class screen_key_result_e { NotHandled, Switched, Fallback, Error };

  const instructions_diag_t& diag() const { return last_diag; }

  ncurses_instruction_session_t(NcursesApp& a,
                                const cuwacunu::camahjucunu::iinuji_renderings_instruction_t& i,
                                IInstructionsData& d,
                                instructions_build_opts_t bo = {},
                                instructions_validate_opts_t vo = {})
    : app(a), inst_eff(i), data(d), bopt(bo), vopt(vo)
  {
    ensure_default_screen_present();
    rebuild_keymap();
  }

  // --- Public convenience ---
  std::shared_ptr<iinuji_object_t> active_root()
  {
    if (auto* b = active_built()) return b->root;
    return nullptr;
  }

  void render(bool clear_first = true)
  {
    if (clear_first) app.renderer().clear();
    cuwacunu::iinuji::render_root(active_root());
    app.renderer().flush();
  }

  // dispatch an event into ALL built screens (so inactive screens stay in sync)
  instructions_diag_t dispatch_event_all(const std::string& event_name,
                                         IInstructionsData& data_ref,
                                         const dispatch_payload_t* payload = nullptr)
  {
    instructions_diag_t out;
    for (std::size_t si = 0; si < built_screens.size(); ++si) {
      if (si < built_ok.size() && built_ok[si] && built_screens[si].root) {
        out.merge(dispatch_event(built_screens[si], event_name, data_ref, payload));
      }
    }
    return out;
  }

  // Handle:
  // - configured key => switch screen
  // - unconfigured Fn => go to Default screen (override or builtin)
  screen_key_result_e handle_screen_key(int ch)
  {
    // configured key?
    auto it = screen_for_key.find(ch);
    if (it != screen_for_key.end()) {
      if (rebuild(it->second)) return screen_key_result_e::Switched;
      return screen_key_result_e::Error;
    }

    // unconfigured Fn => Default
    int fn = 0;
    if (decode_fn_key(ch, fn)) {
      if (default_screen_index == (std::size_t)-1) {
        last_diag = {};
        last_diag.err("No Default screen available (missing __key F0 and builtin parse failed)");
        return screen_key_result_e::Error;
      }
      if (!rebuild(default_screen_index)) {
        return screen_key_result_e::Error;
      }
      update_builtin_default_message(fn);
      return screen_key_result_e::Fallback;
    }

    return screen_key_result_e::NotHandled;
  }

  bool pump_streams()
  {
    if (!router) return false;
    if (built_screens.empty()) return false;

    std::vector<instructions_build_result_t*> ok_ptrs;
    ok_ptrs.reserve(built_screens.size());
    for (std::size_t i=0; i<built_screens.size(); ++i) {
      if (i < built_ok.size() && built_ok[i] && built_screens[i].root) {
        ok_ptrs.push_back(&built_screens[i]);
      }
    }

    // Requires the “many-screens” router you already started using.
    return router->pump_all(ok_ptrs, data);
  }

  bool handle_buffer_scroll_key(int ch)
  {
    if (!buf_obj) return false;
    auto bb = std::dynamic_pointer_cast<bufferBox_data_t>(buf_obj->data);
    if (!bb) return false;

    constexpr int kLineStep = 1;
    constexpr int kPageStep = 10;

    if (ch == KEY_UP)    { bb->scroll_by(+kLineStep); return true; }  // older
    if (ch == KEY_DOWN)  { bb->scroll_by(-kLineStep); return true; }  // newer
    if (ch == KEY_PPAGE) { bb->scroll_by(+kPageStep); return true; }
    if (ch == KEY_NPAGE) { bb->scroll_by(-kPageStep); return true; }
    if (ch == 'g')       { bb->jump_tail();           return true; }

    // Mouse wheel support (ncurses sends wheel as BUTTON4/BUTTON5 on KEY_MOUSE)
    if (ch == KEY_MOUSE) {
      MEVENT ev{};
      if (getmouse(&ev) == OK) {
        // Only scroll if the wheel event is over the buffer widget
        const Rect R = buf_obj->screen;
        if (!(ev.x >= R.x && ev.x < R.x + R.w && ev.y >= R.y && ev.y < R.y + R.h)) {
          return false;
        }

        // Visible height in rows (content area)
        int visible = buf_obj->screen.h;
        if (buf_obj->style.border) visible -= 2;
        visible -= (buf_obj->layout.pad_top + buf_obj->layout.pad_bottom);
        visible = std::max(1, visible);

        // Wheel step policy:
        // - small widgets: 1 line per notch
        // - bigger widgets: scale with height
        constexpr int kSmallHeightRows = 8;   // <= this => step = 1
        constexpr int kMaxWheelStep    = 12;  // clamp

        int step = 1;
        if (visible > kSmallHeightRows) {
          // ~1 line per 6 rows, minimum 2 once “not small”
          step = std::clamp((visible + 5) / 6, 2, kMaxWheelStep);
        }

#ifdef BUTTON_SHIFT
        if (ev.bstate & BUTTON_SHIFT) step *= 4;
#endif
#ifdef BUTTON_CTRL
        if (ev.bstate & BUTTON_CTRL) step *= 2;
#endif

        // Wheel up => older (increase scroll)
        if (ev.bstate & BUTTON4_PRESSED) { bb->scroll_by(+step); return true; }
        // Wheel down => newer (toward tail)
        if (ev.bstate & BUTTON5_PRESSED) { bb->scroll_by(-step); return true; }
      }
    }

    return false;
  }

  // --- Build / switch logic ---

  bool build_all()
  {
    last_diag = {};

    // critical: destroy router before re-attaching (restores cout/cerr buffers)
    router.reset();

    int rows=0, cols=0;
    app.renderer().size(rows, cols);

    built_screens.clear();
    built_ok.clear();
    built_screens.resize(inst_eff.screens.size());
    built_ok.resize(inst_eff.screens.size(), false);

    std::vector<instructions_build_result_t*> ok_ptrs;
    ok_ptrs.reserve(inst_eff.screens.size());

    for (std::size_t si=0; si<inst_eff.screens.size(); ++si) {
      built_screens[si] = build_ui_for_screen(inst_eff, si, data, cols, rows, bopt, vopt);
      if (built_screens[si].diag.ok() && built_screens[si].root) {
        built_ok[si] = true;
        ok_ptrs.push_back(&built_screens[si]);
      } else {
        last_diag.merge(built_screens[si].diag);
      }
    }

    if (ok_ptrs.empty()) {
      last_diag.err("build_all(): no screens built successfully");
      return false;
    }

    // Attach stream router ONCE for union of all screens that use .sys.*
    router = sys_stream_router_t::attach_for_many(ok_ptrs, /*passthrough=*/false);

    // Clamp active_screen to a valid built screen
    if (active_screen >= built_ok.size() || !built_ok[active_screen]) {
      std::size_t s = 0;
      while (s < built_ok.size() && !built_ok[s]) s++;
      active_screen = (s < built_ok.size()) ? s : 0;
    }

    refresh_active_buffer_ptr();
    return true;
  }

  // “rebuild” now means “switch active screen”; screens are built once and persist.
  bool rebuild(std::size_t screen_index)
  {
    last_diag = {};

    if (built_screens.empty()) {
      if (!build_all()) {
        if (last_diag.ok()) last_diag.err("build_all() failed (no screens built)");
        return false;
      }
    }

    if (screen_index >= built_screens.size()) {
      last_diag.err("rebuild: screen_index out of range");
      return false;
    }

    if (screen_index >= built_ok.size() || !built_ok[screen_index] || !built_screens[screen_index].root) {
      last_diag = built_screens[screen_index].diag;
      if (last_diag.ok()) last_diag.err("rebuild failed: screen not buildable");
      return false;
    }

    active_screen = screen_index;
    refresh_active_buffer_ptr();
    return true;
  }

private:
  instructions_build_result_t* active_built()
  {
    if (active_screen >= built_screens.size()) return nullptr;
    if (!built_ok.empty() && (active_screen >= built_ok.size() || !built_ok[active_screen])) return nullptr;
    return &built_screens[active_screen];
  }

  void refresh_active_buffer_ptr()
  {
    buf_id.clear();
    buf_obj.reset();

    auto* b = active_built();
    if (!b) return;

    for (const auto& kv : b->figure_kind_by_id) {
      if (kv.second == "_buffer") {
        buf_id = kv.first;
        auto it = b->figure_object_by_id.find(buf_id);
        if (it != b->figure_object_by_id.end()) buf_obj = it->second;
        break;
      }
    }
  }

  void rebuild_keymap()
  {
    screen_for_key.clear();
    screen_for_key.reserve(inst_eff.screens.size());
    for (std::size_t si=0; si<inst_eff.screens.size(); ++si) {
      int k = parse_screen_key_to_ncurses(inst_eff.screens[si].key_raw);
      if (k != -1) screen_for_key[k] = si;
    }
  }

  void ensure_default_screen_present()
  {
    // 1) user override: screen with __key Default
    for (std::size_t si=0; si<inst_eff.screens.size(); ++si) {
      if (is_default_key_raw(inst_eff.screens[si].key_raw)) {
        default_screen_index = si;
        default_is_builtin = false;
        return;
      }
    }

    // 2) otherwise inject built-in Default screen from DSL
    try {
      auto tmp = load_instruction_from_string(kBuiltinDefaultScreenDSL);
      if (!tmp.screens.empty()) {
        inst_eff.screens.push_back(tmp.screens[0]);
        default_screen_index = inst_eff.screens.size() - 1;
        default_is_builtin = true;
      }
    } catch (...) {
      default_screen_index = (std::size_t)-1;
      default_is_builtin = false;
    }
  }

  static bool decode_fn_key(int ch, int& out_n)
  {
    for (int n=1; n<=64; ++n) {
      if (ch == KEY_F(n)) { out_n = n; return true; }
    }
    return false;
  }

  // Only modifies the BUILTIN Default screen. User override screens are untouched.
  void update_builtin_default_message(int fn)
  {
    if (!default_is_builtin) return;
    if (default_screen_index == (std::size_t)-1) return;
    if (default_screen_index >= built_screens.size()) return;

    auto& B = built_screens[default_screen_index];
    if (!B.root) return;

    const std::string label_id = "default_screen.panel0.fig0._label";
    auto it = B.figure_object_by_id.find(label_id);
    if (it == B.figure_object_by_id.end()) return;

    auto tb = std::dynamic_pointer_cast<textBox_data_t>(it->second->data);
    if (!tb) return;

    // list configured F-keys
    std::vector<int> fns;
    fns.reserve(screen_for_key.size());
    for (const auto& kv : screen_for_key) {
      int n = 0;
      if (decode_fn_key(kv.first, n)) fns.push_back(n);
    }
    std::sort(fns.begin(), fns.end());
    fns.erase(std::unique(fns.begin(), fns.end()), fns.end());

    std::ostringstream oss;
    oss << "No screen is configured for F+" << fn << ".\n\n";
    oss << "Configured F-keys:\n";
    if (fns.empty()) {
      oss << "  (none)\n";
    } else {
      for (int n : fns) oss << "  F+" << n << "\n";
    }
    oss << "\nFix: add a SCREEN with __key F+" << fn << "\n";
    oss << "Or override this fallback by defining a SCREEN with __key F0.\n";

    tb->content = oss.str();
    tb->wrap = true;
  }
};

inline void render_root(std::shared_ptr<iinuji_object_t> root)
{
  auto* R = get_renderer();
  if (!R || !root) return;

  int rows=0, cols=0;
  R->size(rows, cols);
  layout_tree(root, Rect{0,0,cols,rows});
  render_tree(root);
}

} // namespace cuwacunu::iinuji
