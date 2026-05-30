#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>

#include <locale.h>
#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_utils.h"
#include "iinuji/ncurses/iinuji_rend_ncurses.h"
#include "iinuji/primitives/image.h"

namespace {

using cuwacunu::iinuji::image_grayscale_options_t;
using cuwacunu::iinuji::rgba_image_t;

void draw_status_line(int y, int width, const std::string &text) {
  mvaddnstr(y, 0, text.c_str(), std::max(0, width - 1));
}

} // namespace

int main(int argc, char **argv) {
  const std::string image_path =
      (argc > 1) ? std::string(argv[1])
                 : "/cuwacunu/src/resources/waajacamaya.png";

  cuwacunu::iinuji::rgba_image_t image{};
  std::string error{};
  if (!cuwacunu::iinuji::decode_png_rgba_file(image_path, image, error)) {
    std::cerr << "[test_iinuji_image_demo] " << image_path << ": " << error
              << "\n";
    return 1;
  }

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
  cuwacunu::iinuji::set_global_background("#101014");

  cuwacunu::iinuji::NcursesRend rend{};
  cuwacunu::iinuji::set_renderer(&rend);

  cuwacunu::iinuji::image_grayscale_options_t braille_opts{};
  braille_opts.mode = cuwacunu::iinuji::image_grayscale_mode_t::Braille;
  braille_opts.background_color = "#101014";
  braille_opts.color_levels = 4;

  cuwacunu::iinuji::image_grayscale_options_t half_block_opts{};
  half_block_opts.mode = cuwacunu::iinuji::image_grayscale_mode_t::HalfBlocks;
  half_block_opts.background_color = "#101014";
  half_block_opts.color_levels = 4;

  bool running = true;
  bool color_enabled = true;
  while (running) {
    int height = 0;
    int width = 0;
    getmaxyx(stdscr, height, width);

    erase();
    if (height < 5 || width < 28) {
      draw_status_line(
          0, width,
          "test_iinuji_image_demo: terminal too small for 2-way view");
      draw_status_line(1, width, "Resize and press q to quit.");
      refresh();
      const int ch = getch();
      if (ch == 'q' || ch == 'Q' || ch == 27)
        running = false;
      continue;
    }

    const int body_y = 3;
    const int body_h = std::max(1, height - body_y - 1);
    const int left_w = std::max(1, width / 2);
    const int right_x = left_w;
    const int right_w = std::max(1, width - right_x);

    draw_status_line(0, width,
                     std::string("iinuji image primitive | left=half-block | "
                                 "right=braille | color=") +
                         (color_enabled ? "color" : "grayscale") +
                         " | c toggle color | q quit");
    draw_status_line(1, width,
                     image_path + " | " + std::to_string(image.width) + "x" +
                         std::to_string(image.height) +
                         " rgba (transparency preserved)");

    mvaddnstr(body_y - 1, 2, "half-blocks", std::max(0, left_w - 4));
    mvaddnstr(body_y - 1, right_x + 2, "braille", std::max(0, right_w - 4));
    if (right_x > 0 && right_x < width) {
      for (int row = body_y - 1; row < height; ++row) {
        mvaddch(row, right_x - 1, ACS_VLINE);
      }
    }

    half_block_opts.use_color = color_enabled;
    braille_opts.use_color = color_enabled;

    cuwacunu::iinuji::render_image_grayscale(image, 0, body_y, left_w - 1,
                                             body_h, half_block_opts);
    cuwacunu::iinuji::render_image_grayscale(image, right_x, body_y, right_w,
                                             body_h, braille_opts);
    refresh();

    const int ch = getch();
    if (ch == 'q' || ch == 'Q' || ch == 27) {
      running = false;
    } else if (ch == 'c' || ch == 'C') {
      color_enabled = !color_enabled;
    }
  }

  endwin();
  return 0;
}
