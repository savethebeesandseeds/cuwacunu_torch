/* test_iinuji_instructions.cpp */

#include <cstdio>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <utility>

#include "piaabo/dconfig.h"
#include "piaabo/dutils.h"

#include "camahjucunu/BNF/BNF_types.h"
#include "camahjucunu/BNF/BNF_AST.h"
#include "camahjucunu/BNF/BNF_grammar_lexer.h"
#include "camahjucunu/BNF/BNF_grammar_parser.h"
#include "camahjucunu/BNF/BNF_instruction_lexer.h"
#include "camahjucunu/BNF/BNF_instruction_parser.h"

#include "camahjucunu/BNF/implementations/iinuji_renderings/iinuji_renderings.h"

#include "iinuji/bnf_compat/iinuji_instructions.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"

namespace BNF = cuwacunu::camahjucunu::BNF;

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
  // IMPORTANT: stop curses first so terminal output is not overwritten.
  if (app) app->shutdown();

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

// -------------------- instruction decode --------------------
static cuwacunu::camahjucunu::iinuji_renderings_instruction_t read_instruction()
{
  std::string LANGUAGE = cuwacunu::piaabo::dconfig::config_space_t::iinuji_renderings_bnf();
  std::string INPUT    = cuwacunu::piaabo::dconfig::config_space_t::iinuji_renderings_instruction();

  BNF::GrammarLexer glex(LANGUAGE);
  BNF::GrammarParser gparser(glex);
  gparser.parseGrammar();
  BNF::ProductionGrammar grammar = gparser.getGrammar();

  BNF::InstructionLexer ilex(INPUT);
  BNF::InstructionParser iparser(ilex, grammar);
  BNF::ASTNodePtr root = iparser.parse_Instruction(INPUT);

  cuwacunu::camahjucunu::iinuji_renderings_decoder_t decoder;
  return decoder.decode(root.get());
}

// -------------------- rendering helper --------------------
static void render_built(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& root)
{
  auto* R = cuwacunu::iinuji::get_renderer();
  if (!R || !root) return;

  int rows = 0, cols = 0;
  R->size(rows, cols);

  cuwacunu::iinuji::layout_tree(root, cuwacunu::iinuji::Rect{0, 0, cols, rows});
  cuwacunu::iinuji::render_tree(root);
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
    auto inst = read_instruction();

    // Debug print (pre-curses) so you can confirm capacity decoding immediately.
    if (!inst.screens.empty() && !inst.screens[0].panels.empty() &&
        inst.screens[0].panels[0].figures.size() > 1)
    {
      auto& F = inst.screens[0].panels[0].figures[1];
      std::cerr << "FIG kind_raw=" << F.kind_raw
            << " has_capacity=" << (F.has_capacity ? "true" : "false")
            << " capacity=" << F.capacity
            << " type_raw=" << F.type_raw
            << "\n";
    }

    // 3) validate BEFORE curses so errors print cleanly
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
    aopt.input_timeout_ms = 50; // lets us pump logs even without keypress
    cuwacunu::iinuji::NcursesApp app(aopt);
    app_ptr = &app;

    // 6) build UI using real terminal size
    int rows = 0, cols = 0;
    app.renderer().size(rows, cols);

    cuwacunu::iinuji::instructions_build_opts_t bopt{};
    auto built = cuwacunu::iinuji::build_ui_for_screen(inst, 0, data, cols, rows, bopt, vopt);

    if (!built.diag.ok() || !built.root) {
      return fatal_exit(app_ptr, "build_ui_for_screen", built.diag, 1);
    }

    // 7) find the buffer object so scrolling works
    std::string buf_id;
    std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> buf_obj;

    for (const auto& kv : built.figure_kind_by_id) {
      if (kv.second == "_buffer") {
        buf_id = kv.first;
        auto it = built.figure_object_by_id.find(buf_id);
        if (it != built.figure_object_by_id.end()) buf_obj = it->second;
        break;
      }
    }

    // 8) attach router (captures std::cout/std::cerr)
    auto router = cuwacunu::iinuji::sys_stream_router_t::attach_for(built, /*passthrough=*/false);

    // Seed buffer immediately AFTER router attaches
    if (router) {
      emit_buffer_help();

      // show which buffer object we found
      if (!buf_id.empty()) std::cout << "[boot] buffer figure id: " << buf_id << "\n";
      else std::cout << "[boot] WARNING: no _buffer figure found (scrolling disabled)\n";

      std::cout << "[boot] stdout capture is ON\n";
      std::cerr << "[boot] stderr capture is ON\n";

      // dispatch what we just wrote into the buffer
      (void)router->pump(built, data);
    }

    // initial render
    app.renderer().clear();
    render_built(built.root);
    app.renderer().flush();

    // Demo behavior controls
    bool auto_spam = true;  // start ON so you see growth without pressing keys
    int  frame     = 0;
    int  seq_out   = 0;
    int  seq_err   = 0;
    int  tick_plot = 0;

    // 9) loop
    while (true) {
      int ch = ::getch();
      if (ch == 'q') break;

      bool changed = false;

      // --- Auto spam: generates lines even with no keypress ---
      // With input_timeout_ms=50, loop ~20 Hz:
      //  frame%10  -> ~2 stdout lines/sec
      //  frame%25  -> ~0.8 stderr lines/sec
      frame++;
      if (auto_spam) {
        if ((frame % 10) == 0) std::cout << "[auto] stdout seq=" << seq_out++ << "\n";
        if ((frame % 25) == 0) std::cerr << "[auto] stderr seq=" << seq_err++ << "\n";
      }

      // --- Key-driven log generation (WRITE FIRST) ---
      // IMPORTANT: write to cout/cerr BEFORE pumping so it appears immediately.
      if (ch == 'o') std::cout << "[key] stdout one seq=" << seq_out++ << "\n";
      if (ch == 'e') std::cerr << "[key] stderr one seq=" << seq_err++ << "\n";

      if (ch == 'b') {
        for (int i = 0; i < 50; ++i) {
          std::cout << "[burst50] i=" << i << " seq=" << seq_out++ << "\n";
        }
      }

      if (ch == 'B') {
        // exceed capacity=1000 so you can observe truncation/rolling
        for (int i = 0; i < 1200; ++i) {
          std::cout << "[burst1200] i=" << i << " seq=" << seq_out++ << "\n";
        }
      }

      if (ch == 's') {
        auto_spam = !auto_spam;
        std::cout << "[key] auto_spam=" << (auto_spam ? "true" : "false") << "\n";
      }

      // --- Pump captured stdout/stderr into buffer (PUMP AFTER WRITES) ---
      if (router) changed |= router->pump(built, data);

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
        (void)cuwacunu::iinuji::dispatch_event(built, "data_update", data, nullptr);
        tick_plot++;
        changed = true;
      }

      // --- Buffer scrolling ---
      if (buf_obj) {
        auto bb = std::dynamic_pointer_cast<cuwacunu::iinuji::bufferBox_data_t>(buf_obj->data);
        if (bb) {
          if (ch == KEY_UP)    { bb->scroll_by(+1);  changed = true; }
          if (ch == KEY_DOWN)  { bb->scroll_by(-1);  changed = true; }
          if (ch == KEY_PPAGE) { bb->scroll_by(+10); changed = true; }
          if (ch == KEY_NPAGE) { bb->scroll_by(-10); changed = true; }
          if (ch == 'g')       { bb->jump_tail();    changed = true; }
        }
      }

      if (ch == KEY_RESIZE) changed = true;

      if (changed) {
        app.renderer().clear();
        render_built(built.root);
        app.renderer().flush();
      }
    }

    return 0;

  } catch (const std::exception& ex) {
    return fatal_exception(app_ptr, "main", ex.what(), 1);
  } catch (...) {
    return fatal_exception(app_ptr, "main", "unknown exception", 1);
  }
}
