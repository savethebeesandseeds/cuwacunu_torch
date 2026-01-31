#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <ncursesw/ncurses.h>

#include "iinuji/bnf_compat/iinuji_instructions.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"

namespace cuwacunu::iinuji {

inline int parse_screen_key_to_ncurses(const std::string& key_raw)
{
  if (key_raw.empty() || key_raw == "<empty>") return -1;
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

struct ncurses_instruction_session_t {
  NcursesApp& app;
  const cuwacunu::camahjucunu::iinuji_renderings_instruction_t& inst;
  IInstructionsData& data;

  instructions_build_opts_t bopt{};
  instructions_validate_opts_t vopt{};

  std::unordered_map<int, std::size_t> screen_for_key;

  std::size_t active_screen = 0;
  std::vector<instructions_build_result_t> built_screens{};
  std::vector<bool> built_ok{};
  std::unique_ptr<sys_stream_router_t> router{};

  std::string buf_id;
  std::shared_ptr<iinuji_object_t> buf_obj;

  // last diagnostics + fallback screen for unconfigured F-keys
  instructions_diag_t last_diag{};
  bool fallback_active = false;
  int  fallback_fn = -1;
  std::shared_ptr<iinuji_object_t> fallback_root{};

  const instructions_diag_t& diag() const { return last_diag; }

  ncurses_instruction_session_t(NcursesApp& a,
                               const cuwacunu::camahjucunu::iinuji_renderings_instruction_t& i,
                               IInstructionsData& d,
                               instructions_build_opts_t bo = {},
                               instructions_validate_opts_t vo = {})
    : app(a), inst(i), data(d), bopt(bo), vopt(vo)
  {
    // build key map once
    screen_for_key.reserve(inst.screens.size());
    for (std::size_t si=0; si<inst.screens.size(); ++si) {
      int k = parse_screen_key_to_ncurses(inst.screens[si].key_raw);
      if (k != -1) screen_for_key[k] = si;
    }
  }

  // render active view (normal screen or fallback)
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

  // Helper: decode KEY_F(n) -> n (portable enough; scans a reasonable range)
  static bool decode_fn_key(int ch, int& out_n)
  {
    for (int n = 1; n <= 64; ++n) {
      if (ch == KEY_F(n)) { out_n = n; return true; }
    }
    return false;
  }

  // Build a fallback root with a red bordered label box (like make_error_box)
  std::shared_ptr<iinuji_object_t> build_unconfigured_fn_root(int fn)
  {
    // Root: fills terminal with default background (cp==0 is fine)
    auto root = create_object("unconfigured_fn_screen");
    root->style.border = false;
    root->style.title.clear();
    root->style.label_color = "white";
    root->style.background_color = "<empty>";
    root->style.border_color = "gray";

    // Message: include configured keys
    std::ostringstream oss;
    oss << "No screen is configured for key F+" << fn << ".\n\n";
    oss << "Configured screens:\n";
    bool any = false;
    for (std::size_t si = 0; si < inst.screens.size(); ++si) {
      const auto& sc = inst.screens[si];
      int k = parse_screen_key_to_ncurses(sc.key_raw);
      if (k == -1) continue;
      // Only list F-keys (since this fallback is for F-keys)
      int n = 0;
      if (!decode_fn_key(k, n)) continue;
      any = true;
      oss << "  F+" << n << "  ->  " << (sc.name.empty() ? "(unnamed)" : sc.name) << "\n";
    }
    if (!any) oss << "  (none)\n";
    oss << "\nFix: add __key F+N to a SCREEN in your instruction.\n";

    // Error box: normalized so it adapts to resizes
    iinuji_layout_t lay{};
    lay.mode = layout_mode_t::Normalized;
    lay.normalized = true;
    lay.x = 0.05;
    lay.y = 0.10;
    lay.width  = 0.90;
    lay.height = 0.45;

    // Reuse your standard error styling (this is a _label/text box)
    auto box = make_error_box("unconfigured_key", oss.str(), lay);
    root->add_child(box);

    // Small hint line under it (optional)
    iinuji_layout_t lay2{};
    lay2.mode = layout_mode_t::Normalized;
    lay2.normalized = true;
    lay2.x = 0.05;
    lay2.y = 0.60;
    lay2.width  = 0.90;
    lay2.height = 0.10;

    iinuji_style_t s2{};
    s2.border = false;
    s2.bold = true;
    s2.label_color = "yellow";
    s2.background_color = "<empty>";
    s2.border_color = "gray";

    auto hint = create_text_box("hint",
      "Press a configured F+N key to switch screens.",
      /*wrap=*/true, text_align_t::Left, lay2, s2);
    root->add_child(hint);

    return root;
  }

  // show fallback view (and disable buffer scrolling while it’s visible)
  void show_unconfigured_fn_screen(int fn)
  {
    fallback_active = true;
    fallback_fn = fn;
    fallback_root = build_unconfigured_fn_root(fn);
    // while fallback is visible, don't allow scroll keys to mutate a hidden buffer
    buf_id.clear();
    buf_obj.reset();
  }

  // one call for "screen key handling":
  // - if configured: switch
  // - if Fn but not configured: show fallback
  // returns true if the active view changed
  bool handle_screen_key(int ch)
  {
    // configured key?
    auto it = screen_for_key.find(ch);
    if (it != screen_for_key.end()) {
      std::size_t next = it->second;
      if (!fallback_active && next == active_screen) return false;
      bool ok = rebuild(next);
      if (ok) fallback_active = false;
      return ok;
    }

    // unconfigured Fn key?
    int fn = 0;
    if (decode_fn_key(ch, fn)) {
      show_unconfigured_fn_screen(fn);
      return true;
    }

    return false;
  }

  // Build every screen once; keeps widget state (buffers, input, etc.) alive across switches.
  bool build_all()
  {
    last_diag = {};
    // critical: destroy router before re-attaching (restores cout/cerr buffers)
    router.reset();

    int rows=0, cols=0;
    app.renderer().size(rows, cols);

    built_screens.clear();
    built_ok.clear();
    built_screens.resize(inst.screens.size());
    built_ok.resize(inst.screens.size(), false);

    std::vector<instructions_build_result_t*> ok_ptrs;
    ok_ptrs.reserve(inst.screens.size());

    for (std::size_t si=0; si<inst.screens.size(); ++si) {
      built_screens[si] = build_ui_for_screen(inst, si, data, cols, rows, bopt, vopt);
      if (built_screens[si].diag.ok() && built_screens[si].root) {
        built_ok[si] = true;
        ok_ptrs.push_back(&built_screens[si]);
      } else {
        // keep diagnostics so tests can print something useful
        last_diag.merge(built_screens[si].diag);
      }
    }

    if (ok_ptrs.empty()) return false;

    // Attach stream router ONCE for union of all screens that use .sys.*
    router = sys_stream_router_t::attach_for_many(ok_ptrs, /*passthrough=*/false);

    // clamp active_screen to a valid built screen
    if (active_screen >= built_ok.size() || !built_ok[active_screen]) {
      active_screen = 0;
      while (active_screen < built_ok.size() && !built_ok[active_screen]) active_screen++;
      if (active_screen >= built_ok.size()) active_screen = 0;
    }

    refresh_active_buffer_ptr();
    return built_ok[active_screen];
  }

  // Backwards name: now means “switch active screen” (no rebuild, no reset).
  bool rebuild(std::size_t screen_index)
  {
    last_diag = {};
    if (built_screens.empty()) {
      if (!build_all()) return false;
    }
    if (screen_index >= built_screens.size() || !built_ok[screen_index]) { 
      if (screen_index < built_screens.size()) last_diag = built_screens[screen_index].diag;
      else last_diag.err("rebuild: screen_index out of range");
      return false;
    }
    active_screen = screen_index;
    fallback_active = false; // leaving fallback if it was visible
    refresh_active_buffer_ptr();
    return true;
  }

  instructions_build_result_t* active_built()
  {
    if (active_screen >= built_screens.size()) return nullptr;
    if (!built_ok.empty() && !built_ok[active_screen]) return nullptr;
    return &built_screens[active_screen];
  }

  std::shared_ptr<iinuji_object_t> active_root()
  {
    if (fallback_active && fallback_root) return fallback_root;
    if (auto* b = active_built()) return b->root;
    return fallback_root; // last resort (can be null)
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


  bool handle_screen_switch_key(int ch)
  {
    auto it = screen_for_key.find(ch);
    if (it == screen_for_key.end()) return false;
    std::size_t next = it->second;
    if (next == active_screen) return false;
    return rebuild(next);
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
    return router->pump_all(ok_ptrs, data);
  }

  bool handle_buffer_scroll_key(int ch)
  {
    if (!buf_obj) return false;
    auto bb = std::dynamic_pointer_cast<bufferBox_data_t>(buf_obj->data);
    if (!bb) return false;

    constexpr int kLineStep = 1;
    constexpr int kPageStep = 10;

    // Tune this: wheel steps per notch
    //  - 3 feels “normal”
    //  - 5–8 feels “fast”
    constexpr int kWheelStep = 6;

    if (ch == KEY_UP)    { bb->scroll_by(+kLineStep); return true; }  // older
    if (ch == KEY_DOWN)  { bb->scroll_by(-kLineStep); return true; }  // newer
    if (ch == KEY_PPAGE) { bb->scroll_by(+kPageStep); return true; }
    if (ch == KEY_NPAGE) { bb->scroll_by(-kPageStep); return true; }

    // Mouse wheel support (ncurses sends wheel as BUTTON4/BUTTON5 on KEY_MOUSE)
    if (ch == KEY_MOUSE) {
      MEVENT ev{};
      if (getmouse(&ev) == OK) {
        // Compute visible buffer height in rows (content area).
        int visible = buf_obj->screen.h;
        if (buf_obj->style.border) visible -= 2;
        visible -= (buf_obj->layout.pad_top + buf_obj->layout.pad_bottom);
        visible = std::max(1, visible);

        // Wheel step policy:
        // - small widgets: 1 line per wheel notch
        // - bigger widgets: scale with height
        //
        // Tune these two knobs:
        constexpr int kSmallHeightRows = 8;   // <= this => step = 1
        constexpr int kMaxWheelStep    = 12;  // clamp

        int step = 1;
        if (visible > kSmallHeightRows) {
          // Example scaling: about ~1 line per 6 visible rows, minimum 2 once "not small".
          // (You can change 6 -> 5 for faster, 7 for slower.)
          step = std::clamp((visible + 5) / 6, 2, kMaxWheelStep);
        }

        // Optional acceleration with modifiers (guarded for portability)
#ifdef BUTTON_SHIFT
        if (ev.bstate & BUTTON_SHIFT) step *= 4;
#endif
#ifdef BUTTON_CTRL
        if (ev.bstate & BUTTON_CTRL) step *= 2;
#endif

        // Wheel up => older (increase scroll distance from tail)
        if (ev.bstate & BUTTON4_PRESSED) { bb->scroll_by(+step); return true; }
        // Wheel down => newer (toward tail)
        if (ev.bstate & BUTTON5_PRESSED) { bb->scroll_by(-step); return true; }
      }
    }
    if (ch == 'g')       { bb->jump_tail();    return true; }
    return false;
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
