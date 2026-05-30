/* iinuji_app_ncurses.h : Standard ncurses init/teardown for iinuji
   - Uses newterm() on /dev/tty by default so you can safely redirect/capture stdout/stderr.
   - Refcounted init/teardown (like your original).
*/
#pragma once

#include <locale.h>
#include <cstdio>
#include <stdexcept>
#include <ncursesw/ncurses.h>

#include "iinuji/render/renderer.h"
#include "iinuji/ncurses/iinuji_rend_ncurses.h"

namespace cuwacunu {
namespace iinuji {

/* Options so the defaults are standardized but still configurable */
struct NcursesAppOpts {
  // Critical: bind curses to /dev/tty so stdout can be redirected without breaking the UI.
  bool use_dev_tty      = true;
  const char* tty_path  = "/dev/tty";
  bool fallback_initscr = true;  // if /dev/tty newterm fails, fall back to initscr()

  bool cbreak_mode      = true;  // cbreak()
  bool no_echo          = true;  // noecho()
  bool enable_keypad    = true;  // keypad(stdscr, TRUE)
  bool hide_cursor      = true;  // curs_set(0)
  bool enable_colors    = true;  // start_color()
  bool default_colors   = true;  // use_default_colors() if available
  bool clear_on_start   = true;  // clear()+refresh() after init

  // Useful for “pump logs” loops:
  //  -1 => blocking getch (default)
  //   0 => non-blocking (getch returns ERR immediately)
  //  >0 => getch waits up to N ms
  int input_timeout_ms  = -1;
};

/* Internal refcount + global SCREEN/TTY handles (so multiple NcursesApp instances are safe) */
struct NcursesGlobalState {
  int rc = 0;

  bool using_newterm = false;
  FILE* tty_in  = nullptr;
  FILE* tty_out = nullptr;
  SCREEN* screen = nullptr;
};

inline NcursesGlobalState& ncurses_global() {
  static NcursesGlobalState g;
  return g;
}

/* RAII session:
   - Initializes ncurses once (refcounted)
   - Sets iinuji global renderer to an owned NcursesRend
   - Restores previous renderer on destruction
*/
class NcursesApp {
public:
  explicit NcursesApp(const NcursesAppOpts& opt = {}) { init(opt); }
  ~NcursesApp() { shutdown(); }

  NcursesApp(const NcursesApp&) = delete;
  NcursesApp& operator=(const NcursesApp&) = delete;

  NcursesRend& renderer() { return rend_; }
  const NcursesRend& renderer() const { return rend_; }

  // True if curses is bound to /dev/tty via newterm (stdout safe to redirect)
  bool using_dev_tty() const { return using_dev_tty_; }

  // Expose manual shutdown if you want deterministic end before scope exit
  void shutdown() {
    if (!active_) return;

    // Restore previous iinuji renderer
    set_renderer(prev_renderer_);
    prev_renderer_ = nullptr;

    auto& g = ncurses_global();
    if (g.rc > 0) g.rc--;

    // End ncurses only when last user goes away
    if (g.rc == 0) {
      ::endwin();

      // If we created a SCREEN with newterm, free it.
      if (g.using_newterm && g.screen) {
        ::delscreen(g.screen);
        g.screen = nullptr;
      }

      // Close tty handles if used
      if (g.tty_in)  { std::fclose(g.tty_in);  g.tty_in  = nullptr; }
      if (g.tty_out) { std::fclose(g.tty_out); g.tty_out = nullptr; }

      g.using_newterm = false;
    }

    active_ = false;
    using_dev_tty_ = false;
  }

private:
  void init(const NcursesAppOpts& opt) {
    if (active_) return;

    auto& g = ncurses_global();
    if (g.rc == 0) {
      // ncurses global init (only once)
      setlocale(LC_ALL, "");

      bool initialized = false;

      // Preferred init: bind curses to tty (NOT stdout)
      if (opt.use_dev_tty) {
        g.tty_in  = std::fopen(opt.tty_path, "r");
        g.tty_out = std::fopen(opt.tty_path, "w");

        if (g.tty_in && g.tty_out) {
          g.screen = ::newterm(nullptr, g.tty_out, g.tty_in);
          if (g.screen) {
            ::set_term(g.screen);          // make it current
            g.using_newterm = true;
            initialized = true;
          }
        }

        if (!initialized) {
          // cleanup partial
          if (g.tty_in)  { std::fclose(g.tty_in);  g.tty_in  = nullptr; }
          if (g.tty_out) { std::fclose(g.tty_out); g.tty_out = nullptr; }
          g.screen = nullptr;
          g.using_newterm = false;
        }
      }

      // Fallback: classic init binds to stdout (NOT safe if you redirect stdout)
      if (!initialized) {
        if (!opt.fallback_initscr) {
          throw std::runtime_error("NcursesApp: newterm(/dev/tty) failed and fallback_initscr=false");
        }
        ::initscr();
        initialized = true;
      }

      if (opt.cbreak_mode) ::cbreak(); else ::nocbreak();
      if (opt.no_echo)     ::noecho(); else ::echo();
      if (opt.enable_keypad) ::keypad(stdscr, TRUE);
      if (opt.hide_cursor) ::curs_set(0);

      if (opt.input_timeout_ms >= 0) {
        ::timeout(opt.input_timeout_ms);
      }

      if (opt.enable_colors && ::has_colors()) {
        ::start_color();
#ifdef NCURSES_VERSION
        if (opt.default_colors) ::use_default_colors();
#endif
      }

      if (opt.clear_on_start) {
        ::clear();
        ::refresh();
      }

    } else {
      // If newterm is used, ensure our screen is the current one
      if (g.using_newterm && g.screen) ::set_term(g.screen);
    }

    g.rc++;

    // Install iinuji renderer (per-instance)
    prev_renderer_ = set_renderer(&rend_);
    active_ = true;
    using_dev_tty_ = g.using_newterm;
  }

private:
  NcursesRend rend_;
  IRend* prev_renderer_ = nullptr;
  bool active_ = false;
  bool using_dev_tty_ = false;
};

} // namespace iinuji
} // namespace cuwacunu
