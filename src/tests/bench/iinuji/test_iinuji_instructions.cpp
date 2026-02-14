/* test_iinuji_instructions.cpp */
#define DLOGS_USE_IOSTREAMS 1

#include <cstdio>
#include <cmath>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <termios.h>
#include <unistd.h>

#include "piaabo/dconfig.h"
#include "piaabo/dlogs.h"
#include "iinuji/bnf_compat/iinuji_instructions.h"


static void disable_xon_xoff()
{
  termios tio{};
  if (tcgetattr(STDIN_FILENO, &tio) != 0) return;
  tio.c_iflag &= ~(IXON | IXOFF);
#ifdef IXANY
  tio.c_iflag &= ~IXANY;
#endif
  (void)tcsetattr(STDIN_FILENO, TCSANOW, &tio);
}

// -------------------- robust diagnostics printing --------------------
static void dump_diag_stderr(const cuwacunu::iinuji::instructions_diag_t& diag,
                            const char* header = "iinuji")
{
  if (!diag.errors.empty()) {
    log_err("\n[%s] errors:\n", header);
    for (const auto& e : diag.errors) log_err("  - %s\n", e.c_str());
  }
  if (!diag.warnings.empty()) {
    log_warn("\n[%s] warnings:\n", header);
    for (const auto& w : diag.warnings) log_err("  - %s\n", w.c_str());
  }
}

static int fatal_exit(cuwacunu::iinuji::NcursesApp* app,
                      const char* stage,
                      const cuwacunu::iinuji::instructions_diag_t& diag,
                      int code = 1)
{
  if (app) app->shutdown(); // stop curses first
  log_err("\n[FATAL] stage=%s\n", stage);
  dump_diag_stderr(diag, stage);
  return code;
}

static int fatal_exception(cuwacunu::iinuji::NcursesApp* app,
                           const char* stage,
                           const char* what,
                           int code = 1)
{
  if (app) app->shutdown();
  log_err("\n[EXCEPTION] stage=%s : %s\n", stage, what ? what : "(null)");
  return code;
}

// Emit help into the captured stdout stream (so it appears in the _buffer)
static void emit_buffer_help()
{
  log_info("=== iinuji buffer demo ===\n");
  log_info("UI:\n");
  log_info("  Tab / Shift+Tab           : focus next/prev input/plot\n");
  log_info("  Type in focused input box : edits the input\n");
  log_info("  Enter                     : commit input (terminal input echoes to stdout)\n");
  log_info("  ArrowUp/ArrowDown, PgUp/PgDn : scroll buffer\n");
  log_info("  g                         : jump to tail\n");
  log_info("\n");
  log_info(" Commands (require Alt):\n");
  log_info("\n");
  log_info("  Alt+q : quit\n");
  log_info("  Alt+o : push ONE stdout line\n");
  log_info("  Alt+e : push ONE stderr line\n");
  log_info("  Alt+b : burst 50 stdout lines\n");
  log_info("  Alt+B : burst 1200 stdout lines (exceeds capacity=1000)\n");
  log_info("  Alt+s : toggle auto-spam (background periodic stdout/stderr)\n");
  log_info("  Alt+u : update plot data (dispatch data_update)\n");
  log_info("==========================\n");
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
    aopt.input_timeout_ms = 50; // makes getch() return ERR periodically (for pumping streams)
    cuwacunu::iinuji::NcursesApp app(aopt);
    app_ptr = &app;

    // Disable Ctrl+S 
    disable_xon_xoff();

    // Enable mouse reporting (wheel comes as KEY_MOUSE)
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);
    mouseinterval(0);

    // 6) Create session
    cuwacunu::iinuji::instructions_build_opts_t bopt{};
    cuwacunu::iinuji::ncurses_instruction_session_t sess(app, inst, data, bopt, vopt);

    // 7) Activate first screen
    if (!sess.rebuild(/*screen_index=*/0)) {
      return fatal_exit(app_ptr, "sess.rebuild(0)", sess.diag(), 1);
    }

    // Seed help into buffer AFTER router is attached
    emit_buffer_help();
    (void)sess.pump_streams();

    sess.render();

    // --- Alt decoding helper ---
    // Terminal Alt is usually encoded as: ESC (27) + next byte.
    // We "peek" the next byte with timeout(0) so we don't block the UI.
    auto decode_alt = [&](int& ch, bool& alt){
      alt = false;
      if (ch != 27) return; // ESC
      ::timeout(0);         // non-blocking peek
      int next = ::getch();
      ::timeout(aopt.input_timeout_ms); // restore main poll
      if (next != ERR) {
        alt = true;
        ch = next;
      }
    };

    // Demo behavior controls
    bool auto_spam = false;
    int  frame     = 0;
    int  seq_out   = 0;
    int  seq_err   = 0;
    int  tick_plot = 0;

    while (true) {
      int ch = ::getch();

      bool alt = false;
      decode_alt(ch, alt);

      // 1) Screen switching + Default fallback (function keys, etc)
      auto r = sess.handle_screen_key(ch);
      if (r == decltype(r)::Error) return fatal_exit(app_ptr, "handle_screen_key", sess.diag(), 1);
      if (r != decltype(r)::NotHandled) { sess.render(); continue; }

      bool changed = false;

      // 2) UI FIRST: typing / focus / scroll
      //    If UI consumes the key, do NOT treat it as a command.
      bool ui_handled = false;
      ui_handled |= sess.handle_text_input_key(ch);     // <-- makes _input_box editable
      ui_handled |= sess.handle_editor_key(ch);     // <-- makes _text_editor editable
      ui_handled |= sess.handle_focus_key(ch);
      ui_handled |= sess.handle_buffer_scroll_key(ch);

      // Background activity + stream pump happen regardless of key source
      frame++;
      if (auto_spam) {
        if ((frame % 10) == 0) log_info("[auto] stdout seq=%d\n", seq_out++);
        if ((frame % 25) == 0) log_err("[auto] stderr seq=%d\n", seq_err++);
        if ((frame % 15) == 0) log_warn("[auto] stderr seq=%d\n", seq_err++);
      }

      changed |= sess.pump_streams();

      // Resize should always cause re-render
      if (ch == KEY_RESIZE) changed = true;

      if (ui_handled) {
        // UI consumed it (typing/tab/scroll). Just re-render if needed.
        if (changed) sess.render();
        else sess.render();
        continue;
      }

      // 3) Commands REQUIRE Alt.
      // If Alt is not held, we do NOT treat keys as commands.
      if (alt) {
        if (ch == 'q') break;

        if (ch == 'o') log_info("[key] stdout one seq=%d\n", seq_out++);
        if (ch == 'e') log_err("[key] stderr one seq=%d\n", seq_err++);

        if (ch == 'b') {
          for (int i = 0; i < 50; ++i) {
            log_info("[burst50] i=%s seq=%d\n", i, seq_out++);
          }
        }

        if (ch == 'B') {
          for (int i = 0; i < 1200; ++i) {
            log_info("[burst1200] i=%s seq=%d\n", i, seq_out++);
          }
        }

        if (ch == 's') {
          auto_spam = !auto_spam;
          log_info("[key] auto_spam=%s\n", (auto_spam ? "true" : "false"));
        }

        if (ch == 'u') {
          std::vector<std::pair<double,double>> pts;
          pts.reserve(120);
          for (int i = 0; i < 120; ++i) {
            double x = (double)i;
            double y = std::sin(x * 0.12 + 0.15 * tick_plot);
            pts.push_back({x, y});
          }
          data.set_vec(0, pts);

          (void)sess.dispatch_event_all("data_update", data, nullptr);

          tick_plot++;
          changed = true;
        }

        // We wrote to cout/cerr, so pump again to reflect it immediately
        changed |= sess.pump_streams();
      }

      if (changed) sess.render();
    }

    return 0;

  } catch (const std::exception& ex) {
    return fatal_exception(app_ptr, "main", ex.what(), 1);
  } catch (...) {
    return fatal_exception(app_ptr, "main", "unknown exception", 1);
  }
}
