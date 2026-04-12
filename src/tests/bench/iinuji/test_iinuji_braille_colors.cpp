#include <locale.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <string>
#include <string_view>

#include "iinuji/iinuji_utils.h"
#include "iinuji/ncurses/iinuji_rend_ncurses.h"

namespace {

using cuwacunu::iinuji::get_color;
using cuwacunu::iinuji::get_color_pair;
using cuwacunu::iinuji::NcursesRend;
using cuwacunu::iinuji::rgb8_from_color_id;
using cuwacunu::iinuji::rgb8_to_hex;
using cuwacunu::iinuji::set_global_background;

constexpr const char *kScreenBg = "#101014";
constexpr const char *kDebugPath = "/tmp/iinuji_braille_color_simple.txt";

struct swatch_t {
  const char *name;
  const char *hex;
};

constexpr std::array<swatch_t, 9> kSwatches{{
    {"red", "#ff5555"},
    {"orange", "#ffaa00"},
    {"yellow", "#ffff55"},
    {"green", "#55ff55"},
    {"cyan", "#00aaaa"},
    {"blue", "#0055aa"},
    {"magenta", "#aa55ff"},
    {"gray", "#555555"},
    {"white", "#ffffff"},
}};

constexpr std::wstring_view kAsciiProbe = L"MMMMMMMM";
constexpr std::wstring_view kBrailleFull = L"⣿⣿⣿⣿⣿⣿⣿⣿";
constexpr std::wstring_view kBrailleLadder = L"⠁⠃⠇⠧⠷⠿⣿⣿";

std::string color_id_to_hex(short id) {
  int r = 0;
  int g = 0;
  int b = 0;
  if (!rgb8_from_color_id(static_cast<int>(id), r, g, b)) {
    return "<default>";
  }
  return rgb8_to_hex(r, g, b);
}

void draw_wide_run(NcursesRend &rend, int y, int x, std::wstring_view glyphs,
                   short pair) {
  int dx = 0;
  for (wchar_t ch : glyphs) {
    rend.putGlyph(y, x + dx, ch, pair);
    ++dx;
  }
}

void draw_status_line(int y, int width, const std::string &text) {
  mvhline(y, 0, ' ', width);
  mvaddnstr(y, 0, text.c_str(), std::max(0, width));
}

void write_debug_file() {
  std::ofstream out(kDebugPath, std::ios::trunc);
  if (!out) {
    return;
  }

  out << "screen_background=" << kScreenBg << "\n";
  out << "notes=left swatch uses solid background; ascii/braille use screen "
         "background\n";

  for (const auto &swatch : kSwatches) {
    const short swatch_pair =
        static_cast<short>(get_color_pair(swatch.hex, swatch.hex));
    const short probe_pair =
        static_cast<short>(get_color_pair(swatch.hex, kScreenBg));

    short swatch_fg = -1;
    short swatch_bg = -1;
    short probe_fg = -1;
    short probe_bg = -1;
    if (swatch_pair > 0) {
      pair_content(swatch_pair, &swatch_fg, &swatch_bg);
    }
    if (probe_pair > 0) {
      pair_content(probe_pair, &probe_fg, &probe_bg);
    }

    out << swatch.name << " requested=" << swatch.hex << "\n";
    out << "  color_id=" << get_color(swatch.hex) << "\n";
    out << "  swatch_pair=" << swatch_pair << " fg_id=" << swatch_fg
        << " bg_id=" << swatch_bg << " fg_rgb=" << color_id_to_hex(swatch_fg)
        << " bg_rgb=" << color_id_to_hex(swatch_bg) << "\n";
    out << "  probe_pair=" << probe_pair << " fg_id=" << probe_fg
        << " bg_id=" << probe_bg << " fg_rgb=" << color_id_to_hex(probe_fg)
        << " bg_rgb=" << color_id_to_hex(probe_bg) << "\n";
  }
}

void draw_screen(NcursesRend &rend, int height, int width) {
  erase();

  if (height < 16 || width < 92) {
    draw_status_line(0, width,
                     "test_iinuji_braille_colors: terminal too small");
    draw_status_line(1, width, "Need at least 92x16 | q quits");
    refresh();
    return;
  }

  draw_status_line(0, width,
                   "simple braille color test | solid swatch vs ascii fg vs "
                   "braille fg | q quit");
  draw_status_line(1, width, std::string("debug: ") + kDebugPath);
  draw_status_line(3, width,
                   "name/hex           solid bg         ascii fg         "
                   "braille full     braille varied");

  int row = 5;
  for (const auto &swatch : kSwatches) {
    const short swatch_pair =
        static_cast<short>(get_color_pair(swatch.hex, swatch.hex));
    const short probe_pair =
        static_cast<short>(get_color_pair(swatch.hex, kScreenBg));

    const std::string label = std::string(swatch.name) + " " + swatch.hex;
    mvhline(row, 0, ' ', width);
    mvaddnstr(row, 0, label.c_str(), 18);

    rend.fillRect(row, 18, 1, 14, swatch_pair);
    draw_wide_run(rend, row, 35, kAsciiProbe, probe_pair);
    draw_wide_run(rend, row, 49, kBrailleFull, probe_pair);
    draw_wide_run(rend, row, 67, kBrailleLadder, probe_pair);

    row += 1;
  }

  draw_status_line(row + 1, width,
                   "Expectation: the hue in ascii and braille should match "
                   "the left swatch.");
  refresh();
}

} // namespace

int main() {
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  if (has_colors()) {
    start_color();
    use_default_colors();
  }

  set_global_background(kScreenBg);

  NcursesRend rend{};
  cuwacunu::iinuji::set_renderer(&rend);

  write_debug_file();

  bool running = true;
  while (running) {
    int height = 0;
    int width = 0;
    getmaxyx(stdscr, height, width);
    draw_screen(rend, height, width);

    const int ch = getch();
    if (ch == 'q' || ch == 'Q' || ch == 27) {
      running = false;
    }
  }

  endwin();
  return 0;
}
