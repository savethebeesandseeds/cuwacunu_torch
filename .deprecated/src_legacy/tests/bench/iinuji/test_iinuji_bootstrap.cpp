// test_iinuji_bootstrap.cpp
#include <algorithm>
#include <memory>
#include <string>

#include <ncursesw/ncurses.h>
#ifdef timeout
#undef timeout
#endif

#include "iinuji/iinuji_render.h"
#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"
#include "piaabo/dlogs.h"

namespace {

__attribute__((constructor(101))) void disable_terminal_logs_pre_main() {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
}

struct DlogTerminalOutputGuard {
  bool prev{true};
  ~DlogTerminalOutputGuard() {
    cuwacunu::piaabo::dlog_set_terminal_output_enabled(prev);
  }
};

void draw_raw_panel(int width, int height, std::string_view term,
                    bool using_dev_tty) {
  WINDOW* win = ::newwin(height, width, 0, 0);
  if (!win) return;

  ::werase(win);
  ::box(win, 0, 0);
  mvwaddnstr(win, 0, 2, " raw ncurses pane ", width - 4);
  mvwaddnstr(win, 2, 2, "This side uses the Human Hero style:", width - 4);
  mvwaddnstr(win, 3, 2, "- NcursesApp bootstrap", width - 4);
  mvwaddnstr(win, 4, 2, "- raw WINDOW*/box/mvwaddnstr/wrefresh", width - 4);

  const std::string line_term = "TERM=" + std::string(term);
  const std::string line_tty =
      std::string("using_dev_tty=") + (using_dev_tty ? "true" : "false");
  mvwaddnstr(win, 6, 2, line_term.c_str(), width - 4);
  mvwaddnstr(win, 7, 2, line_tty.c_str(), width - 4);
  mvwaddnstr(win, height - 2, 2, "Press q to quit", width - 4);
  ::wrefresh(win);
  ::delwin(win);
}

std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> build_iinuji_panel(
    std::string_view term, bool using_dev_tty) {
  using namespace cuwacunu::iinuji;

  auto root = create_grid_container(
      "root", {len_spec::px(3), len_spec::frac(1.0)}, {len_spec::frac(1.0)}, 0,
      0, iinuji_layout_t{},
      iinuji_style_t{"#D8D8D8", "#101014", false, "#5E5E68"});

  auto title = create_text_box(
      "title", "", true, text_align_t::Left, iinuji_layout_t{},
      iinuji_style_t{"#EDEDED", "#202028", true, "#6C6C75", true, false,
                     " iinuji pane "});
  place_in_grid(title, 0, 0);
  root->add_child(title);

  const std::string body_text =
      "This side uses the iinuji object tree.\n\n"
      "- NcursesApp bootstrap\n"
      "- set_global_background(\"#101014\")\n"
      "- create_grid_container / create_text_box\n"
      "- layout_tree / render_tree\n\n"
      "TERM=" +
      std::string(term) + "\nusing_dev_tty=" +
      std::string(using_dev_tty ? "true" : "false") +
      "\n\nIf this pane renders while the raw pane does not, the issue is in "
      "the Human Hero window path.\nIf neither renders, the issue is earlier "
      "in the shared bootstrap path.";

  auto body = create_text_box(
      "body", body_text, true, text_align_t::Left, iinuji_layout_t{},
      iinuji_style_t{"#C8C8CE", "#101014", true, "#5E5E68", false, false,
                     " diagnostics "});
  place_in_grid(body, 1, 0);
  root->add_child(body);
  return root;
}

}  // namespace

int main() try {
  DlogTerminalOutputGuard dlog_guard{
      cuwacunu::piaabo::dlog_terminal_output_enabled()};
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);

  cuwacunu::iinuji::NcursesAppOpts app_opts{};
  app_opts.input_timeout_ms = -1;
  cuwacunu::iinuji::NcursesApp app(app_opts);
#if defined(NCURSES_VERSION)
  ::set_escdelay(25);
#endif

  if (::has_colors()) {
    ::start_color();
    ::use_default_colors();
  }
  cuwacunu::iinuji::set_global_background("#101014");

  const char* term_env = std::getenv("TERM");
  const std::string term = (term_env == nullptr) ? "" : std::string(term_env);
  auto root = build_iinuji_panel(term, app.using_dev_tty());

  for (;;) {
    int H = 0;
    int W = 0;
    getmaxyx(stdscr, H, W);
    if (H < 12 || W < 60) {
      ::erase();
      mvaddnstr(0, 0, "test_iinuji_bootstrap: terminal too small",
                std::max(0, W - 1));
      mvaddnstr(1, 0, "Resize and rerun, or press q to quit.",
                std::max(0, W - 1));
      ::refresh();
      const int ch = ::getch();
      if (ch == 'q' || ch == 'Q' || ch == 27) break;
      continue;
    }

    const int raw_w = std::max(28, W / 2);
    const int tree_x = raw_w;
    const int tree_w = std::max(1, W - tree_x);
    const int content_h = std::max(1, H - 1);

    ::erase();
    cuwacunu::iinuji::layout_tree(root,
                                  cuwacunu::iinuji::Rect{tree_x, 0, tree_w, content_h});
    cuwacunu::iinuji::render_tree(root);
    mvaddnstr(H - 1, 0,
              "test_iinuji_bootstrap | left=raw ncurses | right=iinuji tree | q quit",
              W - 1);
    ::refresh();

    draw_raw_panel(raw_w, content_h, term, app.using_dev_tty());

    const int ch = ::getch();
    if (ch == 'q' || ch == 'Q' || ch == 27) break;
    if (ch == KEY_RESIZE) continue;
  }
  return 0;
} catch (const std::exception& ex) {
  ::endwin();
  log_err("[test_iinuji_bootstrap] exception: %s\n", ex.what());
  return 1;
}
