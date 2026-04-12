#include <locale.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_utils.h"
#include "iinuji/ncurses/iinuji_rend_ncurses.h"
#include "iinuji/primitives/animation.h"

namespace {

using cuwacunu::iinuji::image_grayscale_mode_t;
using cuwacunu::iinuji::image_grayscale_options_t;
using cuwacunu::iinuji::NcursesRend;
using cuwacunu::iinuji::rgba_animation_t;
using steady_clock_t = std::chrono::steady_clock;

void draw_status_line(int y, int width, const std::string &text) {
  mvaddnstr(y, 0, text.c_str(), std::max(0, width - 1));
}

std::uint64_t elapsed_ms_since(const steady_clock_t::time_point &start_time) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          steady_clock_t::now() - start_time)
          .count());
}

} // namespace

int main(int argc, char **argv) {
  const std::string animation_path =
      (argc > 1) ? std::string(argv[1])
                 : "/cuwacunu/src/resources/waajacamaya.apng";

  rgba_animation_t animation{};
  std::string error{};
  if (!cuwacunu::iinuji::decode_animation_rgba_file(animation_path, animation,
                                                    error)) {
    std::cerr << "[test_iinuji_animation_demo] " << animation_path << ": "
              << error << "\n";
    return 1;
  }

  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  timeout(33);
  if (has_colors()) {
    start_color();
    use_default_colors();
  }
  cuwacunu::iinuji::set_global_background("#101014");

  NcursesRend rend{};
  cuwacunu::iinuji::set_renderer(&rend);

  image_grayscale_options_t braille_opts{};
  braille_opts.mode = image_grayscale_mode_t::Braille;
  braille_opts.background_color = "#101014";
  braille_opts.color_levels = 4;

  image_grayscale_options_t half_block_opts{};
  half_block_opts.mode = image_grayscale_mode_t::HalfBlocks;
  half_block_opts.background_color = "#101014";
  half_block_opts.color_levels = 4;

  bool running = true;
  bool paused = false;
  bool color_enabled = true;
  steady_clock_t::time_point start_time = steady_clock_t::now();
  std::uint64_t paused_elapsed_ms = 0u;

  while (running) {
    const std::uint64_t elapsed_ms =
        paused ? paused_elapsed_ms : elapsed_ms_since(start_time);
    const std::size_t frame_index =
        cuwacunu::iinuji::animation_detail::frame_index_for_elapsed_ms(
            animation, elapsed_ms);

    int height = 0;
    int width = 0;
    getmaxyx(stdscr, height, width);

    erase();
    if (height < 5 || width < 28) {
      draw_status_line(
          0, width,
          "test_iinuji_animation_demo: terminal too small for 2-way view");
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

    draw_status_line(
        0, width,
        std::string("iinuji animation primitive | left=half-block | "
                    "right=braille | color=") +
            (color_enabled ? "color" : "grayscale") + " | " +
            (paused ? "paused" : "playing") +
            " | c toggle color | space pause | r restart | q quit");
    draw_status_line(1, width,
                     animation_path + " | " + std::to_string(animation.width) +
                         "x" + std::to_string(animation.height) + " | frame " +
                         std::to_string(frame_index + 1u) + "/" +
                         std::to_string(animation.frames.size()) + " | loop=" +
                         std::to_string(animation.loop_count) + " | total=" +
                         std::to_string(animation.total_duration_ms()) + "ms");

    mvaddnstr(body_y - 1, 2, "half-blocks", std::max(0, left_w - 4));
    mvaddnstr(body_y - 1, right_x + 2, "braille", std::max(0, right_w - 4));
    if (right_x > 0 && right_x < width) {
      for (int row = body_y - 1; row < height; ++row) {
        mvaddch(row, right_x - 1, ACS_VLINE);
      }
    }

    half_block_opts.use_color = color_enabled;
    braille_opts.use_color = color_enabled;

    cuwacunu::iinuji::render_animation_grayscale(
        animation, elapsed_ms, 0, body_y, left_w - 1, body_h, half_block_opts);
    cuwacunu::iinuji::render_animation_grayscale(
        animation, elapsed_ms, right_x, body_y, right_w, body_h, braille_opts);
    refresh();

    const int ch = getch();
    if (ch == ERR) {
      continue;
    }
    if (ch == 'q' || ch == 'Q' || ch == 27) {
      running = false;
    } else if (ch == 'c' || ch == 'C') {
      color_enabled = !color_enabled;
    } else if (ch == 'r' || ch == 'R') {
      paused = false;
      paused_elapsed_ms = 0u;
      start_time = steady_clock_t::now();
    } else if (ch == ' ') {
      if (paused) {
        paused = false;
        start_time = steady_clock_t::now() -
                     std::chrono::milliseconds(
                         static_cast<long long>(paused_elapsed_ms));
      } else {
        paused_elapsed_ms = elapsed_ms;
        paused = true;
      }
    }
  }

  endwin();
  return 0;
}
