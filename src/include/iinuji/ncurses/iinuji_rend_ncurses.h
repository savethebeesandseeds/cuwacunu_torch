/* iinuji_rend_ncurses.h */
#pragma once
#include <ncursesw/ncurses.h>
#include <cwchar>
#include <string>
#include "iinuji/iinuji_rend.h"

namespace cuwacunu {
namespace iinuji {

struct NcursesRend : public IRend {
  void size(int& h, int& w) override { getmaxyx(stdscr, h, w); }
  void clear() override { ::clear(); }
  void flush() override { ::refresh(); }

  void putText(int y, int x, const std::string& s, int max_w = -1,
               short color_pair = 0, bool bold=false, bool inverse=false) override {
    if (color_pair > 0) attron(COLOR_PAIR(color_pair));
    if (bold)    attron(A_BOLD);
    if (inverse) attron(A_REVERSE);

    const char* c = s.c_str();
    int n = (max_w >= 0 ? std::min<int>((int)s.size(), max_w) : (int)s.size());
    mvaddnstr(y, x, c, n);

    if (bold)    attroff(A_BOLD);
    if (inverse) attroff(A_REVERSE);
    if (color_pair > 0) attroff(COLOR_PAIR(color_pair));
  }

  void putGlyph(int y, int x, wchar_t ch, short color_pair = 0) override {
    wchar_t wbuf[2] = { ch, L'\0' };
    cchar_t cc;
    setcchar(&cc, wbuf, A_NORMAL, (color_pair > 0 ? color_pair : 0), nullptr);
    mvadd_wch(y, x, &cc);
  }

  void fillRect(int y, int x, int h, int w, short color_pair) override {
    if (color_pair > 0) attron(COLOR_PAIR(color_pair));
    for (int r = 0; r < h; ++r)
      for (int c = 0; c < w; ++c)
        mvaddch(y + r, x + c, ' ' | (color_pair > 0 ? COLOR_PAIR(color_pair) : 0));
    if (color_pair > 0) attroff(COLOR_PAIR(color_pair));
  }

  // Optional explicit override (base already aliases to putGlyph)
  void putBraille(int y, int x, wchar_t ch, short color_pair = 0) override {
    putGlyph(y, x, ch, color_pair);
  }
};

} // namespace iinuji
} // namespace cuwacunu
