/* test_iinuji_instructions.cpp */

#include <cstdio>
#include <cmath>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "piaabo/dconfig.h"

#include "iinuji/bnf_compat/iinuji_instructions.h"

// -------------------- robust diagnostics printing --------------------
static void dump_diag_stderr(const cuwacunu::iinuji::instructions_diag_t& diag,
                            const char* header = "iinuji")
{
  if (!diag.errors.empty()) {
    std::fprintf(stderr, "\n[%s] errors:\n", header);
    for (const auto& e : diag.errors) std::fprintf(stderr, "  - %s\n", e.c_str());
  }
  if (!diag.warnings.empty()) {
    std::fprintf(stderr, "\n[%s] warnings:\n", header);
    for (const auto& w : diag.warnings) std::fprintf(stderr, "  - %s\n", w.c_str());
  }
  std::fflush(stderr);
}

static int fatal_exit(cuwacunu::iinuji::NcursesApp* app,
                      const char* stage,
                      const cuwacunu::iinuji::instructions_diag_t& diag,
                      int code = 1)
{
  if (app) app->shutdown(); // stop curses first
  std::fprintf(stderr, "\n[FATAL] stage=%s\n", stage);
  dump_diag_stderr(diag, stage);
  return code;
}

static int fatal_exception(cuwacunu::iinuji::NcursesApp* app,
                           const char* stage,
                           const char* what,
                           int code = 1)
{
  if (app) app->shutdown();
  std::fprintf(stderr, "\n[EXCEPTION] stage=%s : %s\n", stage, what ? what : "(null)");
  std::fflush(stderr);
  return code;
}

// Emit help into the captured stdout stream (so it appears in the _buffer)
static void emit_buffer_help()
{
  std::cout << "=== iinuji buffer demo ===\n";
  std::cout << "Keys:\n";
  std::cout << "  q        : quit\n";
  std::cout << "  o / e    : push ONE stdout / stderr line\n";
  std::cout << "  b / B    : burst 50 / 1200 stdout lines (B exceeds capacity=1000)\n";
  std::cout << "  s        : toggle auto-spam (adds lines over time)\n";
  std::cout << "  u        : update plot data (dispatch data_update)\n";
  std::cout << "  ArrowUp/ArrowDown, PgUp/PgDn : scroll buffer\n";
  std::cout << "  g        : jump to tail\n";
  std::cout << "==========================\n";
}

int main()
{
  cuwacunu::iinuji::NcursesApp* app_ptr = nullptr;

  try {
    // 1) config
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    // 2) decode instruction (no curses yet)
    auto inst = cuwacunu::iinuji::load_instruction_from_config();

    // 3) validate BEFORE curses
    cuwacunu::iinuji::instructions_validate_opts_t vopt{};
    auto vdiag = cuwacunu::iinuji::validate_instruction(inst, vopt);
    if (!vdiag.ok()) {
      dump_diag_stderr(vdiag, "validate_instruction");
      return 1;
    }

    // 4) seed some test data
    cuwacunu::iinuji::FixedInstructionsData data;
    data.set_str(0, "label from data (str0)");
    data.set_str(1, "input initial (str1)");

    {
      std::vector<std::pair<double,double>> pts;
      pts.reserve(120);
      for (int i = 0; i < 120; ++i) {
        double x = (double)i;
        double y = std::sin(x * 0.12);
        pts.push_back({x, y});
      }
      data.set_vec(0, pts);
    }

    // 5) start curses
    cuwacunu::iinuji::NcursesAppOpts aopt{};
    aopt.input_timeout_ms = 50;
    cuwacunu::iinuji::NcursesApp app(aopt);
    app_ptr = &app;

    // (optional but often required for F-keys)
    keypad(stdscr, TRUE);

    // Enable mouse reporting (wheel comes as KEY_MOUSE)
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);
    mouseinterval(0); // reduces delay / improves wheel responsiveness

    // 6) Create session (owns build+router+buffer+keymap)
    cuwacunu::iinuji::instructions_build_opts_t bopt{};
    cuwacunu::iinuji::ncurses_instruction_session_t sess(app, inst, data, bopt, vopt);

    // 7) Build initial screen
    if (!sess.rebuild(/*screen_index=*/0)) {
      cuwacunu::iinuji::instructions_diag_t d;
      if (!sess.built_screens.empty()) d = sess.built_screens[0].diag;
      else d.err("sess.rebuild(0) failed: no built screens");
      return fatal_exit(app_ptr, "sess.rebuild(0)", sess.diag(), 1);
    }

    // Seed help into buffer AFTER router is attached
    // (NOTE: this only works if the screen has sys-stream events wired, otherwise stdout prints to terminal)
    emit_buffer_help();
    (void)sess.pump_streams();

    // Initial render
    sess.render();

    // Demo behavior controls
    bool auto_spam = true;
    int  frame     = 0;
    int  seq_out   = 0;
    int  seq_err   = 0;
    int  tick_plot = 0;

    // 8) loop
    while (true) {
      int ch = ::getch();
      if (ch == 'q') break;

      // --- screen switching (and fallback screen for unconfigured Fn keys) ---
      if (sess.handle_screen_key(ch)) {
        // If rebuild() failed for a configured key, make it fatal like before.
        // (Fallback for unconfigured Fn keys will return true and render normally.)
        if (!sess.active_root()) return fatal_exit(app_ptr, "sess.handle_screen_key", sess.diag(), 1);
        sess.render();
        continue;
      }

      bool changed = false;

      // --- Auto spam ---
      frame++;
      if (auto_spam) {
        if ((frame % 10) == 0) std::cout << "[auto] stdout seq=" << seq_out++ << "\n";
        if ((frame % 25) == 0) std::cerr << "[auto] stderr seq=" << seq_err++ << "\n";
      }

      // --- Key-driven log generation (WRITE FIRST) ---
      if (ch == 'o') std::cout << "[key] stdout one seq=" << seq_out++ << "\n";
      if (ch == 'e') std::cerr << "[key] stderr one seq=" << seq_err++ << "\n";

      if (ch == 'b') {
        for (int i = 0; i < 50; ++i) {
          std::cout << "[burst50] i=" << i << " seq=" << seq_out++ << "\n";
        }
      }

      if (ch == 'B') {
        for (int i = 0; i < 1200; ++i) {
          std::cout << "[burst1200] i=" << i << " seq=" << seq_out++ << "\n";
        }
      }

      if (ch == 's') {
        auto_spam = !auto_spam;
        std::cout << "[key] auto_spam=" << (auto_spam ? "true" : "false") << "\n";
      }

      // --- Pump captured stdout/stderr into buffer ---
      changed |= sess.pump_streams();

      // --- Update plot ---
      if (ch == 'u') {
        std::vector<std::pair<double,double>> pts;
        pts.reserve(120);
        for (int i = 0; i < 120; ++i) {
          double x = (double)i;
          double y = std::sin(x * 0.12 + 0.15 * tick_plot);
          pts.push_back({x, y});
        }
        data.set_vec(0, pts);
        // Update all screens so inactive plots stay in sync too
        for (std::size_t si = 0; si < sess.built_screens.size(); ++si) {
          if (si < sess.built_ok.size() && sess.built_ok[si] && sess.built_screens[si].root) {
            (void)cuwacunu::iinuji::dispatch_event(sess.built_screens[si], "data_update", data, nullptr);
          }
        }
        tick_plot++;
        changed = true;
      }

      // --- Buffer scrolling ---
      changed |= sess.handle_buffer_scroll_key(ch);

      if (ch == KEY_RESIZE) changed = true;

      if (changed) {
        sess.render();
      }
    }

    return 0;

  } catch (const std::exception& ex) {
    return fatal_exception(app_ptr, "main", ex.what(), 1);
  } catch (...) {
    return fatal_exception(app_ptr, "main", "unknown exception", 1);
  }
}
