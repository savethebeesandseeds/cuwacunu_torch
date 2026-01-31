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
    // IMPORTANT: always set attrs explicitly so cp==0 does not inherit stale colors
    attr_t a = A_NORMAL;
    if (color_pair > 0) a |= COLOR_PAIR(color_pair);
    if (bold)    a |= A_BOLD;
    if (inverse) a |= A_REVERSE;
    attrset(a);

    const char* c = s.c_str();
    int n = (max_w >= 0 ? std::min<int>((int)s.size(), max_w) : (int)s.size());
    mvaddnstr(y, x, c, n);

    attrset(A_NORMAL);
  }

  void putGlyph(int y, int x, wchar_t ch, short color_pair = 0) override {
    wchar_t wbuf[2] = { ch, L'\0' };
    cchar_t cc;
    setcchar(&cc, wbuf, A_NORMAL, (color_pair > 0 ? color_pair : 0), nullptr);
    mvadd_wch(y, x, &cc);
  }

  void fillRect(int y, int x, int h, int w, short color_pair) override {
    // IMPORTANT: ensure cp==0 fills with A_NORMAL, not the previously active color pair
    attr_t a = A_NORMAL;
    if (color_pair > 0) a |= COLOR_PAIR(color_pair);
    attrset(a);
    for (int r = 0; r < h; ++r) {
      mvhline(y + r, x, ' ', w);
    }
    attrset(A_NORMAL);
  }

  // Optional explicit override (base already aliases to putGlyph)
  void putBraille(int y, int x, wchar_t ch, short color_pair = 0) override {
    putGlyph(y, x, ch, color_pair);
  }
};

} // namespace iinuji
} // namespace cuwacunu
