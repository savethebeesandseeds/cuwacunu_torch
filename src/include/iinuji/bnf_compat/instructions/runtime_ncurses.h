#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <ncursesw/ncurses.h>
#include <fstream>

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

static constexpr const char* kFooterScreenName = "iinuji_footer";

static constexpr const char* kBuiltinFooterScreenDSL = R"DSL(
SCREEN _screen
  __tickness 1
  __name iinuji_footer
  __text_color #c8c8c8
  __line_color #444444
  __back_color #202020

  PANEL _rectangle
    __coords 0,0
    __z_index 0
    __shape 100,100
    __title false ""
    __border false

    ; left: status
    FIGURE _label
      __coords 0,0
      __shape 65,100
      __value "F+N: switch screens"
      __type nowrap
      __border false
    ENDFIGURE

    ; right: terminal input (visible via different background + caret rendering)
    FIGURE _input_box
      __coords 65,0
      __shape 35,100
      __value ""
      __border false
      __back_color #000060
      __text_color #ffffff
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

  std::unique_ptr<cuwacunu::camahjucunu::iinuji_screen_t> footer_spec{};

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
    load_footer_spec();
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
    draw_editor_footers();
    update_hw_cursor();
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

  bool handle_key(int ch)
  {
    auto r = handle_screen_key(ch);
    if (r != screen_key_result_e::NotHandled) return true;

    if (handle_mouse_key(ch)) return true;
    if (handle_focus_key(ch)) return true;
    if (handle_text_input_key(ch)) return true;
    if (handle_editor_key(ch)) return true;
    if (handle_buffer_scroll_key(ch)) return true;

    return false;
  }

  static bool read_file_lines(const std::string& path, std::vector<std::string>& out, std::string& err)
  {
    out.clear();
    err.clear();
    if (path.empty()) { err = "empty path"; return false; }
    std::ifstream in(path);
    if (!in) { err = "open failed"; return false; }
    std::string line;
    while (std::getline(in, line)) {
      if (!line.empty() && line.back() == '\r') line.pop_back();
      out.push_back(std::move(line));
    }
    if (out.empty()) out.emplace_back();
    return true;
  }

  static bool write_file_lines(const std::string& path, const std::vector<std::string>& in, std::string& err)
  {
    err.clear();
    if (path.empty()) { err = "empty path"; return false; }
    std::ofstream out(path, std::ios::trunc);
    if (!out) { err = "open failed"; return false; }
    for (size_t i=0; i<in.size(); ++i) {
      out << in[i];
      if (i + 1 < in.size()) out << "\n";
    }
    out << "\n";
    if (!out) { err = "write failed"; return false; }
    return true;
  }

  bool handle_editor_key(int ch)
  {
    auto* B = active_built();
    if (!B) return false;
    const std::string* pfid = current_focus_id(*B);
    if (!pfid) return false;
    const std::string& fid = *pfid;
    auto itKind = B->figure_kind_by_id.find(fid);
    if (itKind == B->figure_kind_by_id.end()) return false;
    if (itKind->second != "_text_editor") return false;

    auto itObj = B->figure_object_by_id.find(fid);
    if (itObj == B->figure_object_by_id.end() || !itObj->second) return false;

    auto ed = std::dynamic_pointer_cast<editorBox_data_t>(itObj->second->data);
    if (!ed) return false;
    ed->ensure_nonempty();

    Rect cr = content_rect(*itObj->second);
    const int W = std::max(0, cr.w);
    const int H = std::max(0, cr.h);
    const bool has_footer = (H >= 3);
    const int body_h = has_footer ? std::max(1, H - 2) : std::max(1, H - 1);

    int ln_w = ed->last_lineno_w;
    // Defensive: last_lineno_w may be uninitialized / stale / larger than W
    if (ln_w < 3 || ln_w > W) {
      int digits = 1;
      int nlines = std::max(1, (int)ed->lines.size());
      int t = nlines;
      while (t >= 10) { t /= 10; ++digits; }
      ln_w = std::min(W, digits + 2);
      if (ln_w < 3) ln_w = std::min(W, 3);
    }
    ed->last_lineno_w = ln_w;
    const int text_w = std::max(1, W - ln_w);

    auto ensure_visible = [&](){
      ed->ensure_nonempty();
      ed->cursor_line = std::clamp(ed->cursor_line, 0, (int)ed->lines.size() - 1);
      ed->cursor_col  = std::clamp(ed->cursor_col,  0, (int)ed->lines[(size_t)ed->cursor_line].size());
      if (ed->cursor_line < ed->top_line) ed->top_line = ed->cursor_line;
      if (ed->cursor_line >= ed->top_line + body_h) ed->top_line = ed->cursor_line - body_h + 1;
      int max_top = std::max(0, (int)ed->lines.size() - body_h);
      ed->top_line = std::clamp(ed->top_line, 0, max_top);
      if (ed->cursor_col < ed->left_col) ed->left_col = ed->cursor_col;
      if (ed->cursor_col >= ed->left_col + text_w) ed->left_col = ed->cursor_col - text_w + 1;
      if (ed->left_col < 0) ed->left_col = 0;
    };

    // CRITICAL: clamp/sanitize editor state BEFORE any ed->lines[ cursor_line ] access.
    // Without this, a garbage/uninitialized cursor_line can make typing “do nothing”
    // (or crash) even when the widget is visually focused.
    ensure_visible();

    auto is_enter = [&](int k){
#ifdef KEY_ENTER
      if (k == KEY_ENTER) return true;
#endif
      return (k == '\n' || k == '\r');
    };

    // ESC clears “armed” close state (and can clear status)
    if (ch == 27) {
      ed->close_armed = false;
      // optional: keep last status unless it was the close warning
      if (ed->status.find("unsaved changes") != std::string::npos) ed->status.clear();
      return true;
    }

    // Home/End (“init/end”) keys
#ifdef KEY_HOME
    if (ch == KEY_HOME) {
      ed->preferred_col = -1;
      ed->cursor_col = 0;
      ensure_visible();
      return true;
    }
#endif
#ifdef KEY_BEG
    if (ch == KEY_BEG) { // some terminals send KEY_BEG instead of HOME
      ed->preferred_col = -1;
      ed->cursor_col = 0;
      ensure_visible();
      return true;
    }
#endif
#ifdef KEY_END
    if (ch == KEY_END) {
      ed->preferred_col = -1;
      ed->cursor_col = (int)ed->lines[(size_t)ed->cursor_line].size();
      ensure_visible();
      return true;
    }
#endif

    // Ctrl+S save
    if (ch == 19) {
      ed->close_armed = false;
      if (ed->read_only) { ed->status = "read-only"; return true; }
      if (ed->path.empty()) { ed->status = "no path (set __value)"; return true; }
      std::string err;
      if (write_file_lines(ed->path, ed->lines, err)) { ed->dirty = false; ed->status = "saved"; }
      else ed->status = "save failed: " + err;
      return true;
    }

    // Ctrl+R reload/init
    if (ch == 18) {
      ed->close_armed = false;
      if (ed->path.empty()) { ed->status = "no path"; return true; }
      std::vector<std::string> tmp;
      std::string err;
      if (read_file_lines(ed->path, tmp, err)) {
        ed->lines = std::move(tmp);
        ed->dirty = false;
        ed->cursor_line = 0; ed->cursor_col = 0;
        ed->top_line = 0; ed->left_col = 0;
        ed->status = "reloaded";
      } else ed->status = "reload failed: " + err;
      return true;
    }

    // Ctrl+Q close/discard (end)
    if (ch == 17) {
      if (!ed->dirty) {
        ed->lines.assign(1, std::string());
        ed->cursor_line = ed->cursor_col = 0;
        ed->top_line = ed->left_col = 0;
        ed->path.clear();
        ed->status = "closed";
        ed->close_armed = false;
        return true;
      }
      if (!ed->close_armed) {
        ed->close_armed = true;
        ed->status = "unsaved changes: Ctrl+Q again to discard";
        return true;
      }
      ed->lines.assign(1, std::string());
      ed->cursor_line = ed->cursor_col = 0;
      ed->top_line = ed->left_col = 0;
      ed->path.clear();
      ed->dirty = false;
      ed->status = "discarded";
      ed->close_armed = false;
      return true;
    }

    // Navigation
    if (ch == KEY_LEFT)  { ed->preferred_col = -1; if (ed->cursor_col>0) ed->cursor_col--; else if (ed->cursor_line>0){ ed->cursor_line--; ed->cursor_col=(int)ed->lines[(size_t)ed->cursor_line].size(); } ensure_visible(); return true; }
    if (ch == KEY_RIGHT) { ed->preferred_col = -1; { int len=(int)ed->lines[(size_t)ed->cursor_line].size(); if (ed->cursor_col<len) ed->cursor_col++; else if (ed->cursor_line+1<(int)ed->lines.size()){ ed->cursor_line++; ed->cursor_col=0; } } ensure_visible(); return true; }
    if (ch == KEY_UP)    { if (ed->preferred_col<0) ed->preferred_col=ed->cursor_col; if (ed->cursor_line>0) ed->cursor_line--; ed->cursor_col=std::min(ed->preferred_col,(int)ed->lines[(size_t)ed->cursor_line].size()); ensure_visible(); return true; }
    if (ch == KEY_DOWN)  { if (ed->preferred_col<0) ed->preferred_col=ed->cursor_col; if (ed->cursor_line+1<(int)ed->lines.size()) ed->cursor_line++; ed->cursor_col=std::min(ed->preferred_col,(int)ed->lines[(size_t)ed->cursor_line].size()); ensure_visible(); return true; }

    // Ctrl+A / Ctrl+E home/end fallbacks
    if (ch == 1) { ed->preferred_col=-1; ed->cursor_col=0; ensure_visible(); return true; }
    if (ch == 5) { ed->preferred_col=-1; ed->cursor_col=(int)ed->lines[(size_t)ed->cursor_line].size(); ensure_visible(); return true; }

    if (ch == KEY_PPAGE) { ed->preferred_col=-1; int step=std::max(1,body_h-1); ed->top_line=std::max(0,ed->top_line-step); ed->cursor_line=std::max(0,ed->cursor_line-step); ed->cursor_col=std::min(ed->cursor_col,(int)ed->lines[(size_t)ed->cursor_line].size()); ensure_visible(); return true; }
    if (ch == KEY_NPAGE) { ed->preferred_col=-1; int step=std::max(1,body_h-1); int max_top=std::max(0,(int)ed->lines.size()-body_h); ed->top_line=std::min(max_top,ed->top_line+step); ed->cursor_line=std::min((int)ed->lines.size()-1,ed->cursor_line+step); ed->cursor_col=std::min(ed->cursor_col,(int)ed->lines[(size_t)ed->cursor_line].size()); ensure_visible(); return true; }

    // Enter
    if (is_enter(ch)) {
      if (ed->read_only) { ed->status="read-only"; return true; }
      std::string& line = ed->lines[(size_t)ed->cursor_line];
      std::string right = line.substr((size_t)ed->cursor_col);
      line.resize((size_t)ed->cursor_col);
      ed->lines.insert(ed->lines.begin()+ed->cursor_line+1, std::move(right));
      ed->cursor_line++; ed->cursor_col=0;
      if (!ed->dirty) { ed->status = "modified"; } ed->dirty=true; ed->close_armed=false; ed->preferred_col=-1;
      ensure_visible();
      return true;
    }

    // Backspace
    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      if (ed->read_only) { ed->status="read-only"; return true; }
      if (ed->cursor_col>0) {
        auto& line = ed->lines[(size_t)ed->cursor_line];
        line.erase((size_t)ed->cursor_col-1,1);
        ed->cursor_col--;
        if (!ed->dirty) { ed->status = "modified"; } ed->dirty=true; ed->close_armed=false;
      } else if (ed->cursor_line>0) {
        auto& prev = ed->lines[(size_t)ed->cursor_line-1];
        std::string cur = std::move(ed->lines[(size_t)ed->cursor_line]);
        int new_col = (int)prev.size();
        prev += cur;
        ed->lines.erase(ed->lines.begin()+ed->cursor_line);
        ed->cursor_line--; ed->cursor_col=new_col;
        if (!ed->dirty) { ed->status = "modified"; } ed->dirty=true; ed->close_armed=false;
      }
      ensure_visible();
      return true;
    }

    // Delete
#ifdef KEY_DC
    if (ch == KEY_DC) {
      if (ed->read_only) { ed->status="read-only"; return true; }
      auto& line = ed->lines[(size_t)ed->cursor_line];
      if (ed->cursor_col < (int)line.size()) {
        line.erase((size_t)ed->cursor_col,1);
        if (!ed->dirty) { ed->status = "modified"; } ed->dirty=true; ed->close_armed=false;
      } else if (ed->cursor_line+1 < (int)ed->lines.size()) {
        line += ed->lines[(size_t)ed->cursor_line+1];
        ed->lines.erase(ed->lines.begin()+ed->cursor_line+1);
        if (!ed->dirty) { ed->status = "modified"; } ed->dirty=true; ed->close_armed=false;
      }
      ensure_visible();
      return true;
    }
#endif

    // Insert printable ASCII
    if (ch >= 32 && ch < 127) {
      if (ed->read_only) { ed->status="read-only"; return true; }
      auto& line = ed->lines[(size_t)ed->cursor_line];
      line.insert(line.begin()+ed->cursor_col, (char)ch);
      ed->cursor_col++;
      if (!ed->dirty) { ed->status = "modified"; } ed->dirty=true; ed->close_armed=false; ed->preferred_col=-1;
      ensure_visible();
      return true;
    }

    return false;
  }

  bool handle_text_input_key(int ch)
  {
    auto* B = active_built();
    if (!B) return false;
    const std::string* pfid = current_focus_id(*B);
    if (!pfid) return false;
    const std::string& fid = *pfid;

    auto itKind = B->figure_kind_by_id.find(fid);
    if (itKind == B->figure_kind_by_id.end()) return false;
    if (itKind->second != "_input_box") return false;

    auto itObj = B->figure_object_by_id.find(fid);
    if (itObj == B->figure_object_by_id.end() || !itObj->second) return false;

    auto tb = std::dynamic_pointer_cast<textBox_data_t>(itObj->second->data);
    if (!tb) return false;

    const bool is_terminal =
      (!B->terminal_input_id.empty() && fid == B->terminal_input_id);

    if (is_terminal) {
      ensure_terminal_prompt_prefix(tb->content);
    }

    auto is_enter = [&](int k){
#ifdef KEY_ENTER
      if (k == KEY_ENTER) return true;
#endif
      return (k == '\n' || k == '\r');
    };

    // Commit line
    if (is_enter(ch)) {
      std::string line = tb->content;
      if (is_terminal) line = strip_terminal_prompt_prefix(line);

      // If this input box has triggers, dispatch them as _action with payload
      auto itT = B->triggers_by_figure_id.find(fid);
      if (itT != B->triggers_by_figure_id.end()) {
        dispatch_payload_t p;
        p.has_str = true;
        p.str = line;

        for (const auto& ev : itT->second) {
          if (is_unset_token(ev)) continue;
          (void)dispatch_event_all(ev, data, &p);
        }
      }

      // Terminal behavior: echo to stdout (captured into buffer via sys_stream_router)
      if (!B->terminal_input_id.empty() && fid == B->terminal_input_id) {
        if (!line.empty()) std::cout << line << std::endl;

        // Clear terminal input on ALL screens so it stays consistent across screen switches
        for (std::size_t si=0; si<built_screens.size(); ++si) {
          if (si < built_ok.size() && !built_ok[si]) continue;
          auto& Bs = built_screens[si];
          if (!Bs.terminal_input) continue;
          auto tbs = std::dynamic_pointer_cast<textBox_data_t>(Bs.terminal_input->data);
          if (tbs) {
            tbs->content.assign(terminal_prompt());
          }
        }
      }

      return true;
    }

    // Editing: backspace
    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      if (is_terminal) {
        const std::size_t n = terminal_prompt_len();
        if (tb->content.size() > n) tb->content.pop_back();
      } else {
        if (!tb->content.empty()) tb->content.pop_back();
      }
      return true;
    }

    // Ctrl+U clears line
    if (ch == 21) {
      if (is_terminal) tb->content.assign(terminal_prompt());
      else tb->content.clear();
      return true;
    }

    // Append printable ASCII
    if (ch >= 32 && ch < 127) {
      if (is_terminal) ensure_terminal_prompt_prefix(tb->content);
      tb->content.push_back((char)ch);
      return true;
    }

    return false;
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
    if (ch == KEY_MOUSE) return handle_mouse_key(ch);
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

  bool handle_mouse_key(int ch)
  {
    if (ch != KEY_MOUSE) return false;
    MEVENT ev{};
    if (getmouse(&ev) != OK) return false;

    auto root = active_root();
    if (!root) return false;

    auto* B = active_built();
    if (!B) return false;

    int rows=0, cols=0;
    app.renderer().size(rows, cols);
    layout_tree(root, Rect{0,0,cols,rows});

    auto picked = pick_topmost(root, ev.x, ev.y);
    if (!picked) return false;

    auto climb = [&](std::shared_ptr<iinuji_object_t> o)->std::shared_ptr<iinuji_object_t> {
      while (o) {
        // Always prefer editor/buffer by data type
        if (std::dynamic_pointer_cast<editorBox_data_t>(o->data)) return o;
        if (std::dynamic_pointer_cast<bufferBox_data_t>(o->data)) return o;

        // Only allow focusable objects that are actual FIGURE ids
        if (o->focusable) {
          if (B->figure_kind_by_id.find(o->id) != B->figure_kind_by_id.end())
            return o;
        }

        o = o->parent.lock();
      }
      return nullptr;
    };

    auto target = climb(picked);
    if (!target) return false;

#ifdef BUTTON1_PRESSED
    if (ev.bstate & BUTTON1_PRESSED) {
      bool handled = false;
      if (auto ed = std::dynamic_pointer_cast<editorBox_data_t>(target->data)) {

        // ALWAYS focus editor on click
        handled |= focus_to_id(target->id);

        // If focus_to_id fails for any reason, force the visual focus and normalize
        if (!handled) {
          target->focused = true;
          (void)resolve_focus_id(*B);
          handled = true;
        }

        Rect cr = content_rect(*target);

        const bool has_footer = (cr.h >= 3);               // header + body + footer
        const int body_y = cr.y + 1;                       // skip header row
        const int body_h = std::max(0, cr.h - (has_footer ? 2 : 1)); // exclude footer if present

        if (body_h > 0 && ev.y >= body_y && ev.y < body_y + body_h) {
          int row = ev.y - body_y;

          ed->ensure_nonempty();
          int li = std::clamp(ed->top_line + row, 0, (int)ed->lines.size() - 1);

          int ln_w = ed->last_lineno_w;
          if (ln_w < 3 || ln_w > cr.w) {
            int digits = 1, t = std::max(1,(int)ed->lines.size());
            while (t >= 10) { t /= 10; ++digits; }
            ln_w = std::min(std::max(0, cr.w), digits + 2);
            if (ln_w < 3) ln_w = std::min(std::max(0, cr.w), 3);
          }
          ed->last_lineno_w = ln_w;

          int x0 = cr.x + ln_w;
          int col = ed->left_col + std::max(0, ev.x - x0);
          col = std::clamp(col, 0, (int)ed->lines[(size_t)li].size());

          ed->cursor_line = li;
          ed->cursor_col  = col;
          ed->preferred_col = -1;
          ed->close_armed = false;
        }

        return true;
      }

      // Non-editor focusable widgets
      if (target->focusable) {
        handled |= focus_to_id(target->id);
        return handled;
      }

      return false;

    }
#endif

    const bool wheel_up =
#ifdef BUTTON4_PRESSED
      (ev.bstate & BUTTON4_PRESSED)
#else
      false
#endif
      ;
    const bool wheel_dn =
#ifdef BUTTON5_PRESSED
      (ev.bstate & BUTTON5_PRESSED)
#else
      false
#endif
      ;
    if (!(wheel_up || wheel_dn)) return false;

    Rect cr = content_rect(*target);
    int visible = std::max(1, cr.h);
    constexpr int kSmallHeightRows = 8;
    constexpr int kMaxWheelStep    = 12;
    int step = (visible > kSmallHeightRows) ? std::clamp((visible + 5) / 6, 2, kMaxWheelStep) : 1;

#ifdef BUTTON_SHIFT
    if (ev.bstate & BUTTON_SHIFT) step *= 4;
#endif
#ifdef BUTTON_CTRL
    if (ev.bstate & BUTTON_CTRL) step *= 2;
#endif

    if (auto bb = std::dynamic_pointer_cast<bufferBox_data_t>(target->data)) {
      if (wheel_up) { bb->scroll_by(+step); return true; }
      if (wheel_dn) { bb->scroll_by(-step); return true; }
    }
    if (auto ed = std::dynamic_pointer_cast<editorBox_data_t>(target->data)) {
      ed->ensure_nonempty();
      const bool has_footer = (cr.h >= 3);
      const int view_h = std::max(1, cr.h - (has_footer ? 2 : 1));
      int max_top = std::max(0, (int)ed->lines.size() - view_h);
      if (wheel_up) ed->top_line = std::max(0, ed->top_line - step);
      if (wheel_dn) ed->top_line = std::min(max_top, ed->top_line + step);
      return true;
    }
    return false;
  }

  // Tab navigation:
  //  - TAB cycles forward
  //  - Shift+TAB cycles backward (KEY_BTAB in ncurses)
  // Returns true if handled (caller should re-render).
  bool handle_focus_key(int ch)
  {
    // Forward tab
    if (ch == '\t' || ch == 9
#ifdef KEY_TAB
        || ch == KEY_TAB
#endif
    ) {
      return focus_cycle(/*reverse=*/false);
    }

    // Back tab (Shift+Tab)
#ifdef KEY_BTAB
    if (ch == KEY_BTAB) {
      return focus_cycle(/*reverse=*/true);
    }
#endif

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
      built_screens[si] = build_ui_for_screen(inst_eff, si, data, cols, rows, bopt, vopt, footer_spec.get());
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
    update_global_menu_bars();
    ensure_terminal_prompt_all();

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
    update_global_menu_bars();
    ensure_terminal_prompt_all();

    return true;
  }

private:
  // Normalize focus so that keyboard routing always matches what's visually focused.
  // Priority:
  //  1) If any widget in focus_order has .focused=true, use that.
  //  2) Else fall back to focus_index.
  // Also enforces "only one focused=true".
  const std::string* resolve_focus_id(instructions_build_result_t& B)
  {
    if (B.focus_order.empty()) {
      B.focus_index = (std::size_t)-1;
      return nullptr;
    }

    // 1) Find a focused flag in focus_order
    std::size_t flagged = (std::size_t)-1;
    for (std::size_t i = 0; i < B.focus_order.size(); ++i) {
      const std::string& id = B.focus_order[i];
      auto it = B.figure_object_by_id.find(id);
      if (it != B.figure_object_by_id.end() && it->second && it->second->focused) {
        flagged = i;
        break;
      }
    }

    // 2) If focus_index points to something different than the focused flag, prefer the flag.
    if (flagged != (std::size_t)-1) {
      B.focus_index = flagged;
    } else {
      // No focused flag set anywhere -> trust focus_index or default to 0
      if (B.focus_index == (std::size_t)-1 || B.focus_index >= B.focus_order.size()) {
        B.focus_index = 0;
      }
    }

    // 3) Enforce exactly one focused=true (the one at focus_index)
    for (std::size_t i = 0; i < B.focus_order.size(); ++i) {
      const std::string& id = B.focus_order[i];
      auto it = B.figure_object_by_id.find(id);
      if (it != B.figure_object_by_id.end() && it->second) {
        it->second->focused = (i == B.focus_index);
      }
    }

    return &B.focus_order[B.focus_index];
  }

  void update_hw_cursor()
  {
    auto* B = active_built();
    if (!B) { ::curs_set(0); return; }

    const std::string* pfid = current_focus_id(*B);
    if (!pfid) { ::curs_set(0); return; }
    const std::string& fid = *pfid;

    auto itObj  = B->figure_object_by_id.find(fid);
    auto itKind = B->figure_kind_by_id.find(fid);
    if (itObj == B->figure_object_by_id.end() || !itObj->second ||
        itKind == B->figure_kind_by_id.end())
    {
      ::curs_set(0);
      return;
    }

    auto& obj = *itObj->second;
    Rect cr = content_rect(obj);
    if (cr.w <= 0 || cr.h <= 0) { ::curs_set(0); return; }

    int cx = cr.x;
    int cy = cr.y;
    bool show = false;

    // Input box: cursor at end of content (clamped)
    if (itKind->second == "_input_box") {
      auto tb = std::dynamic_pointer_cast<textBox_data_t>(obj.data);
      if (!tb) { ::curs_set(0); return; }
      cy = cr.y;
      cx = cr.x + (int)tb->content.size();
      cx = std::clamp(cx, cr.x, cr.x + cr.w - 1);
      cy = std::clamp(cy, cr.y, cr.y + cr.h - 1);
      show = true;
    }

    // Text editor: cursor at (cursor_line/col) within scrolled viewport
    else if (itKind->second == "_text_editor") {
      auto ed = std::dynamic_pointer_cast<editorBox_data_t>(obj.data);
      if (!ed) { ::curs_set(0); return; }
      ed->ensure_nonempty();

      const int W = std::max(0, cr.w);
      const int H = std::max(0, cr.h);
      const bool has_footer = (H >= 3);
      const int body_h = has_footer ? std::max(1, H - 2) : std::max(1, H - 1);

      int ln_w = ed->last_lineno_w;
      if (ln_w < 3 || ln_w > W) {
        int digits = 1, t = std::max(1, (int)ed->lines.size());
        while (t >= 10) { t /= 10; ++digits; }
        ln_w = std::min(W, digits + 2);
        if (ln_w < 3) ln_w = std::min(W, 3);
      }
      ed->last_lineno_w = ln_w;
      const int text_w = std::max(1, W - ln_w);

      // keep caret visible
      ed->cursor_line = std::clamp(ed->cursor_line, 0, (int)ed->lines.size() - 1);
      ed->cursor_col  = std::clamp(ed->cursor_col,  0, (int)ed->lines[(size_t)ed->cursor_line].size());
      if (ed->cursor_line < ed->top_line) ed->top_line = ed->cursor_line;
      if (ed->cursor_line >= ed->top_line + body_h) ed->top_line = ed->cursor_line - body_h + 1;
      ed->top_line = std::clamp(ed->top_line, 0, std::max(0, (int)ed->lines.size() - body_h));
      if (ed->cursor_col < ed->left_col) ed->left_col = ed->cursor_col;
      if (ed->cursor_col >= ed->left_col + text_w) ed->left_col = ed->cursor_col - text_w + 1;
      if (ed->left_col < 0) ed->left_col = 0;

      const int body_y = cr.y + 1;
      const int body_x = cr.x + ln_w;

      int row = ed->cursor_line - ed->top_line;
      int col = ed->cursor_col  - ed->left_col;

      // If caret offscreen, hide
      if (row < 0 || row >= body_h || col < 0 || col >= text_w) {
        ::curs_set(0);
        return;
      }

      cy = body_y + row;
      cx = body_x + col;
      show = true;
    }

    if (!show) { ::curs_set(0); return; }

    // Clamp to terminal bounds
    int rows=0, cols=0;
    app.renderer().size(rows, cols);
    cx = std::clamp(cx, 0, std::max(0, cols - 1));
    cy = std::clamp(cy, 0, std::max(0, rows - 1));

    ::curs_set(1);
    ::move(cy, cx);
  }

  // Always use the normalizer (so keyboard routing matches visual focus).
  const std::string* current_focus_id(instructions_build_result_t& B)
  {
    return resolve_focus_id(B);
  }

  bool focus_to_id(const std::string& id)
  {
    auto* B = active_built();
    if (!B) return false;
    if (B->focus_order.empty()) return false;
    auto it = std::find(B->focus_order.begin(), B->focus_order.end(), id);
    if (it == B->focus_order.end()) return false;
    set_focus_index(*B, (std::size_t)std::distance(B->focus_order.begin(), it));
    return true;
  }
  
  void set_focus_index(instructions_build_result_t& B, std::size_t idx)
  {
    if (B.focus_order.empty()) { B.focus_index = (std::size_t)-1; return; }
    if (idx >= B.focus_order.size()) idx = 0;

    // clear old
    if (B.focus_index < B.focus_order.size()) {
      const std::string& old_id = B.focus_order[B.focus_index];
      auto it = B.figure_object_by_id.find(old_id);
      if (it != B.figure_object_by_id.end() && it->second) it->second->focused = false;
    }

    B.focus_index = idx;
    const std::string& new_id = B.focus_order[B.focus_index];
    auto it = B.figure_object_by_id.find(new_id);
    if (it != B.figure_object_by_id.end() && it->second) it->second->focused = true;
  }

  bool focus_cycle(bool reverse)
  {
    auto* B = active_built();
    if (!B) return false;
    if (B->focus_order.empty()) return false;

    // If invalid/unset, jump to first
    if (B->focus_index == (std::size_t)-1 || B->focus_index >= B->focus_order.size()) {
      set_focus_index(*B, 0);
      return true;
    }

    std::size_t cur = B->focus_index;
    std::size_t n = B->focus_order.size();
    std::size_t nxt = 0;
    if (!reverse) nxt = (cur + 1) % n;
    else          nxt = (cur == 0) ? (n - 1) : (cur - 1);

    set_focus_index(*B, nxt);
    return true;
  }

  static constexpr const char* terminal_prompt()
  {
    return "terminal:# ";
  }

  static constexpr std::size_t terminal_prompt_len()
  {
    return sizeof("terminal:# ") - 1;
  }

  static inline bool has_terminal_prompt_prefix(const std::string& s)
  {
    const char* p = terminal_prompt();
    const std::size_t n = terminal_prompt_len();
    return (s.size() >= n && s.compare(0, n, p) == 0);
  }

  static inline void ensure_terminal_prompt_prefix(std::string& s)
  {
    const char* p = terminal_prompt();
    if (s.empty()) { s.assign(p); return; }
    if (!has_terminal_prompt_prefix(s)) s.insert(0, p);
  }

  static inline std::string strip_terminal_prompt_prefix(const std::string& s)
  {
    const std::size_t n = terminal_prompt_len();
    if (has_terminal_prompt_prefix(s)) return s.substr(n);
    return s;
  }

  void ensure_terminal_prompt_all()
  {
    for (std::size_t si = 0; si < built_screens.size(); ++si) {
      if (si < built_ok.size() && !built_ok[si]) continue;

      auto& Bs = built_screens[si];
      if (!Bs.terminal_input) continue;

      auto tbs = std::dynamic_pointer_cast<textBox_data_t>(Bs.terminal_input->data);
      if (!tbs) continue;

      ensure_terminal_prompt_prefix(tbs->content);
    }
  }

  void load_footer_spec()
  {
    footer_spec.reset();

    // 1) Extract ALL screens named iinuji_footer from inst_eff (user override or leftovers).
    //    This guarantees the footer can NEVER be rendered as a normal screen.
    for (std::size_t si = 0; si < inst_eff.screens.size(); /* no ++ here */) {
      const auto& s = inst_eff.screens[si];
      if (!is_unset_token(s.name) && to_lower(s.name) == to_lower(kFooterScreenName)) {

        // Keep the first one as the footer spec (if user defined multiple, first wins)
        if (!footer_spec) {
          footer_spec = std::make_unique<cuwacunu::camahjucunu::iinuji_screen_t>(s);
        }

        // Remove from normal screens list
        inst_eff.screens.erase(inst_eff.screens.begin() + (long)si);

        // Keep active_screen stable if we removed something before it
        if (active_screen > si) active_screen--;

        continue; // IMPORTANT: don't increment si (vector shifted)
      }
      ++si;
    }

    // Keep active_screen in range (important if the last screen was removed)
    if (!inst_eff.screens.empty() && active_screen >= inst_eff.screens.size()) {
      active_screen = 0;
    }

    // 2) If no user footer existed, parse the builtin footer DSL into footer_spec directly.
    //    CRITICAL: do NOT append it into inst_eff.screens (no injection).
    if (!footer_spec) {
      try {
        auto tmp = load_instruction_from_string(kBuiltinFooterScreenDSL);
        if (!tmp.screens.empty()) {
          footer_spec = std::make_unique<cuwacunu::camahjucunu::iinuji_screen_t>(tmp.screens[0]);
        }
      } catch (...) {
        // footer_spec stays null -> build.h fallback MUST still create input box (Fix 2)
      }
    }
  }


  instructions_build_result_t* active_built()
  {
    if (active_screen >= built_screens.size()) return nullptr;
    if (!built_ok.empty() && (active_screen >= built_ok.size() || !built_ok[active_screen])) return nullptr;
    return &built_screens[active_screen];
  }

  static std::string basename_of(const std::string& path)
  {
    if (path.empty()) return "<new>";
    std::size_t p = path.find_last_of("/\\");
    return (p == std::string::npos) ? path : path.substr(p + 1);
  }

  static std::string editor_footer_text(const editorBox_data_t& ed)
  {
    std::ostringstream oss;

    // File + dirty indicator
    oss << basename_of(ed.path);
    if (ed.read_only) oss << " [RO]";
    if (ed.dirty)     oss << " [*unsaved*]";

    // Cursor position (1-based for humans)
    oss << "  Ln " << (ed.cursor_line + 1) << " Col " << (ed.cursor_col + 1);

    // Status (saved/reloaded/errors)
    if (!ed.status.empty()) oss << " | " << ed.status;

    // Commands
    oss << " | Ctrl+S save  Ctrl+R reload  Ctrl+Q close  PgUp/PgDn scroll  Home/End";

    return oss.str();
  }

  void draw_editor_footers()
  {
    auto* B = active_built();
    if (!B) return;

    int rows=0, cols=0;
    app.renderer().size(rows, cols);
    if (rows <= 0 || cols <= 0) return;

    for (const auto& kv : B->figure_kind_by_id) {
      if (kv.second != "_text_editor") continue;

      auto itObj = B->figure_object_by_id.find(kv.first);
      if (itObj == B->figure_object_by_id.end() || !itObj->second) continue;

      auto ed = std::dynamic_pointer_cast<editorBox_data_t>(itObj->second->data);
      if (!ed) continue;
      ed->ensure_nonempty();

      Rect cr = content_rect(*itObj->second);
      if (cr.w <= 0 || cr.h <= 0) continue;

      const bool has_footer = (cr.h >= 3);             // header + body + footer
      const int body_y = cr.y + 1;                     // skip header row
      const int body_h = std::max(0, cr.h - (has_footer ? 2 : 1));

      // Compute/validate line-number gutter width
      int ln_w = ed->last_lineno_w;
      const int nlines = std::max(1, (int)ed->lines.size());
      int digits = 1;
      for (int t = nlines; t >= 10; t /= 10) ++digits;

      if (ln_w < 3 || ln_w > cr.w) {
        ln_w = std::min(cr.w, digits + 2);
        if (ln_w < 3) ln_w = std::min(cr.w, 3);
      }
      ed->last_lineno_w = ln_w;

      // --- 1) FIX LINE NUMBERS: draw correct numbers for each visible row ---
      if (body_h > 0 && ln_w > 0) {
        for (int row = 0; row < body_h; ++row) {
          const int li = ed->top_line + row;
          if (li < 0 || li >= (int)ed->lines.size()) break;

          // right-align into (ln_w - 1) columns, keep last digits if needed
          std::string num = std::to_string(li + 1);
          const int avail = std::max(0, ln_w - 1);
          if ((int)num.size() > avail) num = num.substr(num.size() - (size_t)avail);

          std::string cell((size_t)ln_w, ' ');
          const int start = std::max(0, avail - (int)num.size());
          for (int i = 0; i < (int)num.size() && (start + i) < avail; ++i) {
            cell[(size_t)(start + i)] = num[(size_t)i];
          }

          // Clamp row to screen bounds
          const int yy = body_y + row;
          if (yy < 0 || yy >= rows) continue;

          int xx = cr.x;
          int ww = ln_w;

          if (xx < 0) { ww += xx; xx = 0; }
          if (xx >= cols || ww <= 0) continue;
          if (xx + ww > cols) ww = cols - xx;
          if (ww <= 0) continue;

          // highlight current cursor line number
          if (li == ed->cursor_line) wattron(stdscr, A_BOLD);
          else                       wattron(stdscr, A_DIM);

          wmove(stdscr, yy, xx);
          waddnstr(stdscr, cell.c_str(), ww);

          wattroff(stdscr, A_BOLD);
          wattroff(stdscr, A_DIM);
        }
      }

      // --- 2) FOOTER HELP LINE INSIDE THE EDITOR (bottom row) ---
      if (has_footer) {
        int y = cr.y + cr.h - 1;
        int x = cr.x;
        int w = cr.w;

        if (y < 0 || y >= rows) continue;
        if (x < 0) { w += x; x = 0; }
        if (x >= cols || w <= 0) continue;
        if (x + w > cols) w = cols - x;
        if (w <= 0) continue;

        std::string msg = editor_footer_text(*ed);
        std::string out((size_t)w, ' ');
        for (int i=0; i<w && i<(int)msg.size(); ++i) out[(size_t)i] = msg[(size_t)i];

        wattron(stdscr, A_REVERSE);
        wmove(stdscr, y, x);
        waddnstr(stdscr, out.c_str(), w);
        wattroff(stdscr, A_REVERSE);
      }
    }
  }



  void update_global_menu_bars()
  {
    if (built_screens.empty()) return;

    int rows=0, cols=0;
    app.renderer().size(rows, cols);

    // Build "F+N: switch screens | F+1:screenA  F+2:screenB ..."
    std::vector<std::pair<int,std::string>> items;
    items.reserve(screen_for_key.size());
    for (const auto& kv : screen_for_key) {
      int fn = 0;
      if (decode_fn_key(kv.first, fn)) {
        std::string nm = "screen";
        if (kv.second < inst_eff.screens.size() && !is_unset_token(inst_eff.screens[kv.second].name))
          nm = inst_eff.screens[kv.second].name;
        items.push_back({fn, nm});
      }
    }
    std::sort(items.begin(), items.end(),
              [](const auto& a, const auto& b){ return a.first < b.first; });

    std::ostringstream oss;
    oss << "F+N: switch screens";
    if (!items.empty()) {
      oss << " | ";
      for (size_t i=0; i<items.size(); ++i) {
        if (i) oss << "  ";
        oss << "F+" << items[i].first << ":" << items[i].second;
      }
    }
    std::string line = oss.str();
    if (cols > 0 && (int)line.size() > cols) {
      if (cols >= 3) line = line.substr(0, (size_t)cols - 3) + "...";
      else line = line.substr(0, (size_t)cols);
    }

    for (auto& B : built_screens) {
      if (!B.menu_bar) continue;
      auto tb = std::dynamic_pointer_cast<textBox_data_t>(B.menu_bar->data);
      if (tb) { tb->content = line; tb->wrap = false; }
    }
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
