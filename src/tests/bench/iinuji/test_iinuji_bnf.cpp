// src/tests/bench/iinuji/test_iinuji_bnf.cpp
#include <string>
#include <vector>

#include "iinuji/iinuji_bnf.h"                    // the interpreter you added
#include "iinuji/ncurses/iinuji_rend_ncurses.h"   // concrete IRend
#include "piaabo/dutils.h"                        // optional logging macros if you use them

// Short aliases
namespace specs = cuwacunu::camahjucunu::iinuji_specs;

int main() {
  using namespace cuwacunu::iinuji;

  // 1) Initialize ncurses renderer
  initscr(); noecho(); cbreak(); keypad(stdscr, TRUE);
  start_color(); use_default_colors();
  NcursesRend rend;
  set_renderer(&rend);

  try {
    // 2) Build a *programmatic* iinuji spec (no BNF decoding needed here)
    specs::iinuji_renderings_instruction_t spec;
    specs::screen_t screen;
    screen.fcode = "F3";
    screen.title = "iinuji_bnf demo (F3)";

    // (Optional) define args if you want to bind; demo binding renders anyway
    // specs::arg_t arg2; arg2.name="Arg2"; screen.args.push_back(arg2);

    // Top banner (text) panel
    {
      specs::panel_t p;
      p.id   = "Banner";
      p.type = "text";
      p.x = 2; p.y = 1; p.w = 76; p.h = 3;
      // A 'text' draw op is optional; the interpreter will still show something;
      // If you have a text-binding, you can push d.op="text" here.
      screen.panels.push_back(p);
    }

    // Upper plot: curve demo
    {
      specs::panel_t p;
      p.id   = "PlotCurve";
      p.type = "plot";
      p.x = 2; p.y = 5; p.w = 76; p.h = 12;
      specs::draw_call_t d;
      d.op = "curve";
      d.kv["color"] = "#58A6FF";
      p.draws.push_back(d);
      screen.panels.push_back(p);
    }

    // Lower plot: embedding (scatter) demo
    {
      specs::panel_t p;
      p.id   = "PlotEmbed";
      p.type = "plot";
      p.x = 2; p.y = 19; p.w = 76; p.h = 12;
      specs::draw_call_t d;
      d.op = "embedding";
      d.kv["color"] = "#A78BFA";
      p.draws.push_back(d);
      screen.panels.push_back(p);
    }

    spec.screens.push_back(screen);

    // 3) Render one screen using the interpreter
    //    Passing nullptr uses the built-in DemoBinding (sine/cos/scatter)
    render_iinuji_screen_once(spec, "F3", /*binding*/nullptr);

    // 4) Pause so you can see it, then clean up
    getch();
    endwin();
    return 0;

  } catch (const std::exception& ex) {
    // Make sure ncurses is reset on error
    endwin();
    fprintf(stderr, "[test_iinuji_bnf] ERROR: %s\n", ex.what());
    return 1;
  } catch (...) {
    endwin();
    fprintf(stderr, "[test_iinuji_bnf] Unknown error\n");
    return 2;
  }
}
