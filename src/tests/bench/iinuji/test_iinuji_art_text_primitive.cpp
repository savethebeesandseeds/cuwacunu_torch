#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <tuple>

#include "iinuji/primitives/art_text.h"

namespace {

using cuwacunu::iinuji::art_text_align_t;
using cuwacunu::iinuji::art_text_options_t;
using cuwacunu::iinuji::image_grayscale_mode_t;
using cuwacunu::iinuji::image_grayscale_options_t;
using cuwacunu::iinuji::IRend;
using cuwacunu::iinuji::rgba_image_t;

struct MockRend final : public IRend {
  struct cell_t {
    wchar_t glyph{L'\0'};
    short color_pair{0};
  };
  std::map<std::tuple<int, int>, cell_t> cells{};

  void size(int &h, int &w) override {
    h = 0;
    w = 0;
  }
  void clear() override { cells.clear(); }
  void flush() override {}
  void putText(int, int, const std::string &, int, short, bool, bool) override {
  }
  void putGlyph(int y, int x, wchar_t ch, short color_pair) override {
    cells[std::make_tuple(y, x)] = cell_t{ch, color_pair};
  }
  void fillRect(int, int, int, int, short) override {}
};

[[noreturn]] void fail(const char *expr, const char *file, int line) {
  std::cerr << "[TEST FAIL] " << file << ":" << line << " (" << expr << ")\n";
  std::exit(1);
}

#define TEST_ASSERT(expr)                                                      \
  do {                                                                         \
    if (!(expr))                                                               \
      fail(#expr, __FILE__, __LINE__);                                         \
  } while (false)

std::size_t count_opaque_pixels(const rgba_image_t &image) {
  std::size_t count = 0;
  for (std::size_t idx = 3; idx < image.pixels.size(); idx += 4u) {
    count += (image.pixels[idx] != 0u) ? 1u : 0u;
  }
  return count;
}

int leftmost_opaque_pixel_in_rows(const rgba_image_t &image, int y_begin,
                                  int y_end) {
  int leftmost = image.width;
  const int start_y = std::max(0, y_begin);
  const int stop_y = std::min(image.height, y_end);
  for (int y = start_y; y < stop_y; ++y) {
    const std::size_t row_start = static_cast<std::size_t>(y) *
                                  static_cast<std::size_t>(image.width) * 4u;
    for (int x = 0; x < image.width; ++x) {
      if (image.pixels[row_start + static_cast<std::size_t>(x) * 4u + 3u] !=
          0u) {
        leftmost = std::min(leftmost, x);
        break;
      }
    }
  }
  return leftmost;
}

bool has_nonblank_glyph(const MockRend &rend) {
  for (const auto &[key, cell] : rend.cells) {
    (void)key;
    if (cell.glyph != L'\0' && cell.glyph != L' ') {
      return true;
    }
  }
  return false;
}

bool has_color_pair(const MockRend &rend, short color_pair) {
  for (const auto &[key, cell] : rend.cells) {
    (void)key;
    if (cell.color_pair == color_pair) {
      return true;
    }
  }
  return false;
}

} // namespace

int main() {
  using cuwacunu::iinuji::default_art_text_render_options;
  using cuwacunu::iinuji::rasterize_art_text_rgba;
  using cuwacunu::iinuji::render_art_text;
  using cuwacunu::iinuji::set_renderer;

  art_text_options_t text_opt{};
  text_opt.pixel_scale = 4;
  text_opt.color_start = "#ff5555";
  text_opt.color_end = "#00aaaa";

  const rgba_image_t image = rasterize_art_text_rgba("Cuwacunu", text_opt);
  TEST_ASSERT(image.valid());
  TEST_ASSERT(image.width > image.height);
  TEST_ASSERT(count_opaque_pixels(image) > 0u);
  TEST_ASSERT(count_opaque_pixels(image) <
              static_cast<std::size_t>(image.width) *
                  static_cast<std::size_t>(image.height));

  art_text_options_t wider_text_opt = text_opt;
  wider_text_opt.glyph_spacing = text_opt.glyph_spacing + 1;
  const rgba_image_t wider_image =
      rasterize_art_text_rgba("Cuwacunu", wider_text_opt);
  TEST_ASSERT(wider_image.width > image.width);

  const image_grayscale_options_t art_render_defaults =
      default_art_text_render_options();
  TEST_ASSERT(art_render_defaults.mode == image_grayscale_mode_t::Braille);
  TEST_ASSERT(art_render_defaults.use_color);
  TEST_ASSERT(!art_render_defaults.center);
  TEST_ASSERT(art_render_defaults.sample_nearest);

  art_text_options_t ascii_text_opt{};
  ascii_text_opt.pixel_scale = 1;
  ascii_text_opt.padding = 0;
  ascii_text_opt.gradient = false;

  const rgba_image_t lf_image = rasterize_art_text_rgba("A\nB", ascii_text_opt);
  const rgba_image_t crlf_image =
      rasterize_art_text_rgba("A\r\nB", ascii_text_opt);
  TEST_ASSERT(crlf_image.width == lf_image.width);
  TEST_ASSERT(crlf_image.height == lf_image.height);
  TEST_ASSERT(count_opaque_pixels(crlf_image) == count_opaque_pixels(lf_image));

  const std::string all_letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const std::string all_numbers = "0123456789";
  const std::string all_lowercase = "abcdefghijklmnopqrstuvwxyz";
  const rgba_image_t all_letters_image =
      rasterize_art_text_rgba(all_letters, ascii_text_opt);
  const rgba_image_t all_numbers_image =
      rasterize_art_text_rgba(all_numbers, ascii_text_opt);
  art_text_options_t all_lowercase_opt = ascii_text_opt;
  all_lowercase_opt.preserve_sparse_lowercase = true;
  const rgba_image_t all_lowercase_image =
      rasterize_art_text_rgba(all_lowercase, all_lowercase_opt);
  const rgba_image_t all_letters_numbers_image =
      rasterize_art_text_rgba(all_letters + "\n" + all_numbers,
                              ascii_text_opt);
  TEST_ASSERT(all_letters_image.valid());
  TEST_ASSERT(all_numbers_image.valid());
  TEST_ASSERT(all_lowercase_image.valid());
  TEST_ASSERT(all_letters_numbers_image.valid());
  TEST_ASSERT(count_opaque_pixels(all_letters_image) > 0u);
  TEST_ASSERT(count_opaque_pixels(all_lowercase_image) > 0u);
  TEST_ASSERT(count_opaque_pixels(all_numbers_image) > 0u);
  TEST_ASSERT(count_opaque_pixels(all_letters_numbers_image) >
              count_opaque_pixels(all_letters_image));

  const rgba_image_t spaces_image =
      rasterize_art_text_rgba("A    B", ascii_text_opt);
  const rgba_image_t tab_image =
      rasterize_art_text_rgba("A\tB", ascii_text_opt);
  TEST_ASSERT(tab_image.width == spaces_image.width);

  art_text_options_t left_align_opt = ascii_text_opt;
  left_align_opt.align = art_text_align_t::Left;
  const rgba_image_t left_aligned_image =
      rasterize_art_text_rgba("AB\nA", left_align_opt);
  art_text_options_t center_align_opt = left_align_opt;
  center_align_opt.align = art_text_align_t::Center;
  const rgba_image_t centered_image =
      rasterize_art_text_rgba("AB\nA", center_align_opt);
  const int second_line_y0 = cuwacunu::iinuji::art_text_detail::kGlyphHeight +
                             center_align_opt.line_spacing;
  const int second_line_y1 =
      second_line_y0 + cuwacunu::iinuji::art_text_detail::kGlyphHeight;
  TEST_ASSERT(leftmost_opaque_pixel_in_rows(centered_image, second_line_y0,
                                            second_line_y1) >
              leftmost_opaque_pixel_in_rows(left_aligned_image, second_line_y0,
                                            second_line_y1));

  MockRend rend{};
  auto *old_renderer = set_renderer(&rend);

  image_grayscale_options_t half_gray_opts{};
  half_gray_opts.mode = image_grayscale_mode_t::HalfBlocks;
  rend.clear();
  render_art_text("Cuwacunu", 0, 0, 24, 6, text_opt, half_gray_opts);
  TEST_ASSERT(has_nonblank_glyph(rend));

  image_grayscale_options_t braille_gray_opts{};
  braille_gray_opts.mode = image_grayscale_mode_t::Braille;
  rend.clear();
  render_art_text("Cuwacunu", 0, 0, 24, 6, text_opt, braille_gray_opts);
  TEST_ASSERT(has_nonblank_glyph(rend));

  art_text_options_t red_text_opt{};
  red_text_opt.pixel_scale = 4;
  red_text_opt.gradient = false;
  red_text_opt.color_start = "#ff5555";
  red_text_opt.color_end = "#ff5555";

  image_grayscale_options_t color_opts{};
  color_opts.mode = image_grayscale_mode_t::Braille;
  color_opts.use_color = true;
  color_opts.color_pair_resolver = [](std::uint8_t r, std::uint8_t g,
                                      std::uint8_t b) -> short {
    return (r > 200 && g < 100 && b < 100) ? 17 : 0;
  };

  rend.clear();
  render_art_text("Cuwacunu", 0, 0, 24, 6, red_text_opt, color_opts);
  TEST_ASSERT(has_nonblank_glyph(rend));
  TEST_ASSERT(has_color_pair(rend, 17));

  rend.clear();
  render_art_text(all_letters + "\n" + all_numbers, 0, 0, 40, 10, text_opt,
                  braille_gray_opts);
  TEST_ASSERT(has_nonblank_glyph(rend));
  rend.clear();
  render_art_text(all_lowercase, 0, 0, 40, 10, all_lowercase_opt,
                  braille_gray_opts);
  TEST_ASSERT(has_nonblank_glyph(rend));

  set_renderer(old_renderer);
  std::cout << "[ALL TESTS PASSED] art text primitive checks; run "
               "test_iinuji_art_text_demo to view all glyph rows.\n";
  return 0;
}
