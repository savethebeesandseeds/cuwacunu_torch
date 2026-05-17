#include <cstdlib>
#include <iostream>
#include <map>
#include <tuple>

#include "iinuji/primitives/animation.h"

namespace {

using cuwacunu::iinuji::image_grayscale_mode_t;
using cuwacunu::iinuji::image_grayscale_options_t;
using cuwacunu::iinuji::IRend;
using cuwacunu::iinuji::rgba_animation_t;
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

rgba_image_t make_solid_rgba(int width, int height, std::uint8_t r,
                             std::uint8_t g, std::uint8_t b,
                             std::uint8_t a = 255) {
  rgba_image_t image{};
  image.width = width;
  image.height = height;
  image.pixels.resize(static_cast<std::size_t>(width) *
                      static_cast<std::size_t>(height) * 4u);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const std::size_t idx =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
           static_cast<std::size_t>(x)) *
          4u;
      image.pixels[idx + 0] = r;
      image.pixels[idx + 1] = g;
      image.pixels[idx + 2] = b;
      image.pixels[idx + 3] = a;
    }
  }
  return image;
}

short color_pair_at(const MockRend &rend, int y, int x) {
  const auto it = rend.cells.find(std::make_tuple(y, x));
  return (it == rend.cells.end()) ? 0 : it->second.color_pair;
}

} // namespace

int main() {
  using cuwacunu::iinuji::decode_animation_rgba_file;
  using cuwacunu::iinuji::decode_apng_rgba_file;
  using cuwacunu::iinuji::render_animation_grayscale;
  using cuwacunu::iinuji::set_renderer;
  using cuwacunu::iinuji::animation_detail::frame_index_for_elapsed_ms;

  rgba_animation_t synthetic{};
  synthetic.width = 2;
  synthetic.height = 2;
  synthetic.loop_count = 0;
  synthetic.frames = {
      make_solid_rgba(2, 2, 255, 0, 0),
      make_solid_rgba(2, 2, 0, 255, 255),
  };
  synthetic.frame_delays_ms = {100, 200};

  TEST_ASSERT(synthetic.valid());
  TEST_ASSERT(synthetic.total_duration_ms() == 300u);
  TEST_ASSERT(frame_index_for_elapsed_ms(synthetic, 0u) == 0u);
  TEST_ASSERT(frame_index_for_elapsed_ms(synthetic, 99u) == 0u);
  TEST_ASSERT(frame_index_for_elapsed_ms(synthetic, 100u) == 1u);
  TEST_ASSERT(frame_index_for_elapsed_ms(synthetic, 299u) == 1u);
  TEST_ASSERT(frame_index_for_elapsed_ms(synthetic, 300u) == 0u);

  synthetic.loop_count = 1;
  TEST_ASSERT(frame_index_for_elapsed_ms(synthetic, 0u) == 0u);
  TEST_ASSERT(frame_index_for_elapsed_ms(synthetic, 350u) == 1u);
  synthetic.loop_count = 0;

  MockRend rend{};
  auto *old_renderer = set_renderer(&rend);

  image_grayscale_options_t opts{};
  opts.mode = image_grayscale_mode_t::HalfBlocks;
  opts.use_color = true;
  opts.background_color = "#101014";
  opts.color_levels = 256;
  opts.color_pair_resolver = [](std::uint8_t r, std::uint8_t g,
                                std::uint8_t b) -> short {
    return static_cast<short>((static_cast<int>(r) << 1) ^
                              (static_cast<int>(g) << 2) ^
                              (static_cast<int>(b) << 3));
  };
  opts.fg_bg_color_pair_resolver =
      [](std::uint8_t fr, std::uint8_t fg, std::uint8_t fb, std::uint8_t br,
         std::uint8_t bg, std::uint8_t bb) -> short {
    return static_cast<short>(
        (static_cast<int>(fr) << 1) ^ (static_cast<int>(fg) << 2) ^
        (static_cast<int>(fb) << 3) ^ (static_cast<int>(br) << 4) ^
        (static_cast<int>(bg) << 5) ^ (static_cast<int>(bb) << 6));
  };

  render_animation_grayscale(synthetic, 0u, 0, 0, 1, 1, opts);
  const short red_pair = color_pair_at(rend, 0, 0);
  TEST_ASSERT(red_pair != 0);

  rend.clear();
  render_animation_grayscale(synthetic, 150u, 0, 0, 1, 1, opts);
  const short cyan_pair = color_pair_at(rend, 0, 0);
  TEST_ASSERT(cyan_pair != 0);
  TEST_ASSERT(cyan_pair != red_pair);

  set_renderer(old_renderer);

  rgba_animation_t decoded_apng{};
  std::string error{};
  TEST_ASSERT(decode_apng_rgba_file("/cuwacunu/src/resources/waajacamaya.apng",
                                    decoded_apng, error));
  TEST_ASSERT(error.empty());
  TEST_ASSERT(decoded_apng.valid());
  TEST_ASSERT(decoded_apng.width > 0);
  TEST_ASSERT(decoded_apng.height > 0);
  TEST_ASSERT(decoded_apng.frames.size() > 1u);
  TEST_ASSERT(decoded_apng.frame_delays_ms.size() ==
              decoded_apng.frames.size());
  TEST_ASSERT(decoded_apng.total_duration_ms() > 0u);

  rgba_animation_t decoded_png{};
  error.clear();
  TEST_ASSERT(decode_animation_rgba_file(
      "/cuwacunu/src/resources/waajacamaya.png", decoded_png, error));
  TEST_ASSERT(error.empty());
  TEST_ASSERT(decoded_png.valid());
  TEST_ASSERT(decoded_png.frames.size() == 1u);
  TEST_ASSERT(decoded_png.frame_delays_ms.size() == 1u);
  TEST_ASSERT(decoded_png.total_duration_ms() > 0u);

  return 0;
}
