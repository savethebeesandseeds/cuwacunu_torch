#include <cstdlib>
#include <iostream>
#include <map>
#include <tuple>

#include "iinuji/primitives/art_text.h"

namespace {

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

  set_renderer(old_renderer);
  std::cout << "[ALL TESTS PASSED] art text primitive checks; run "
               "test_iinuji_art_text_demo to view \"Cuwacunu\".\n";
  return 0;
}
