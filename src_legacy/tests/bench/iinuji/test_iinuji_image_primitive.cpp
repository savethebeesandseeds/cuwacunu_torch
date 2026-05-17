#include <cstdlib>
#include <iostream>
#include <map>
#include <tuple>
#include <vector>

#include "iinuji/primitives/image.h"

namespace {

using cuwacunu::iinuji::image_color_kernel_t;
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

rgba_image_t make_solid_rgba(int width, int height, std::uint8_t r,
                             std::uint8_t g, std::uint8_t b, std::uint8_t a) {
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

rgba_image_t
make_braille_pattern_image(unsigned char pattern, std::uint8_t on_r = 0,
                           std::uint8_t on_g = 0, std::uint8_t on_b = 0,
                           std::uint8_t on_a = 255, std::uint8_t off_r = 255,
                           std::uint8_t off_g = 255, std::uint8_t off_b = 255,
                           std::uint8_t off_a = 255) {
  static constexpr unsigned char kBrailleBit[4][2] = {
      {0x01, 0x08},
      {0x02, 0x10},
      {0x04, 0x20},
      {0x40, 0x80},
  };

  rgba_image_t image{};
  image.width = 4;
  image.height = 8;
  image.pixels.resize(static_cast<std::size_t>(image.width) *
                      static_cast<std::size_t>(image.height) * 4u);

  for (int sy = 0; sy < 4; ++sy) {
    for (int sx = 0; sx < 2; ++sx) {
      const bool dot_on = (pattern & kBrailleBit[sy][sx]) != 0;
      for (int py = sy * 2; py < sy * 2 + 2; ++py) {
        for (int px = sx * 2; px < sx * 2 + 2; ++px) {
          const std::size_t idx = (static_cast<std::size_t>(py) *
                                       static_cast<std::size_t>(image.width) +
                                   static_cast<std::size_t>(px)) *
                                  4u;
          image.pixels[idx + 0] = dot_on ? on_r : off_r;
          image.pixels[idx + 1] = dot_on ? on_g : off_g;
          image.pixels[idx + 2] = dot_on ? on_b : off_b;
          image.pixels[idx + 3] = dot_on ? on_a : off_a;
        }
      }
    }
  }
  return image;
}

rgba_image_t make_braille_two_tone_image(
    unsigned char pattern, std::uint8_t left_r, std::uint8_t left_g,
    std::uint8_t left_b, std::uint8_t right_r, std::uint8_t right_g,
    std::uint8_t right_b, std::uint8_t alpha = 255, std::uint8_t off_r = 255,
    std::uint8_t off_g = 255, std::uint8_t off_b = 255,
    std::uint8_t off_a = 255) {
  static constexpr unsigned char kBrailleBit[4][2] = {
      {0x01, 0x08},
      {0x02, 0x10},
      {0x04, 0x20},
      {0x40, 0x80},
  };

  rgba_image_t image{};
  image.width = 4;
  image.height = 8;
  image.pixels.resize(static_cast<std::size_t>(image.width) *
                      static_cast<std::size_t>(image.height) * 4u);

  for (int sy = 0; sy < 4; ++sy) {
    for (int sx = 0; sx < 2; ++sx) {
      const bool dot_on = (pattern & kBrailleBit[sy][sx]) != 0;
      const std::uint8_t r = (sx == 0) ? left_r : right_r;
      const std::uint8_t g = (sx == 0) ? left_g : right_g;
      const std::uint8_t b = (sx == 0) ? left_b : right_b;
      for (int py = sy * 2; py < sy * 2 + 2; ++py) {
        for (int px = sx * 2; px < sx * 2 + 2; ++px) {
          const std::size_t idx = (static_cast<std::size_t>(py) *
                                       static_cast<std::size_t>(image.width) +
                                   static_cast<std::size_t>(px)) *
                                  4u;
          image.pixels[idx + 0] = dot_on ? r : off_r;
          image.pixels[idx + 1] = dot_on ? g : off_g;
          image.pixels[idx + 2] = dot_on ? b : off_b;
          image.pixels[idx + 3] = dot_on ? alpha : off_a;
        }
      }
    }
  }
  return image;
}

rgba_image_t make_vertical_split_rgba(std::uint8_t top_r, std::uint8_t top_g,
                                      std::uint8_t top_b, std::uint8_t top_a,
                                      std::uint8_t bottom_r,
                                      std::uint8_t bottom_g,
                                      std::uint8_t bottom_b,
                                      std::uint8_t bottom_a,
                                      int rows_per_half = 4, int width = 4) {
  rgba_image_t image{};
  image.width = std::max(1, width);
  image.height = std::max(1, rows_per_half * 2);
  image.pixels.resize(static_cast<std::size_t>(image.width) *
                      static_cast<std::size_t>(image.height) * 4u);
  for (int row = 0; row < image.height; ++row) {
    const bool is_top = row < rows_per_half;
    for (int col = 0; col < image.width; ++col) {
      const std::size_t idx = (static_cast<std::size_t>(row) *
                                   static_cast<std::size_t>(image.width) +
                               static_cast<std::size_t>(col)) *
                              4u;
      image.pixels[idx + 0] = is_top ? top_r : bottom_r;
      image.pixels[idx + 1] = is_top ? top_g : bottom_g;
      image.pixels[idx + 2] = is_top ? top_b : bottom_b;
      image.pixels[idx + 3] = is_top ? top_a : bottom_a;
    }
  }
  return image;
}

rgba_image_t make_horizontal_rgba(std::uint8_t left_r, std::uint8_t left_g,
                                  std::uint8_t left_b, std::uint8_t left_a,
                                  std::uint8_t right_r, std::uint8_t right_g,
                                  std::uint8_t right_b, std::uint8_t right_a) {
  rgba_image_t image{};
  image.width = 2;
  image.height = 1;
  image.pixels = {
      left_r, left_g, left_b, left_a, right_r, right_g, right_b, right_a,
  };
  return image;
}

wchar_t glyph_at(const MockRend &rend, int y, int x) {
  const auto it = rend.cells.find(std::make_tuple(y, x));
  return (it == rend.cells.end()) ? L'\0' : it->second.glyph;
}

short color_pair_at(const MockRend &rend, int y, int x) {
  const auto it = rend.cells.find(std::make_tuple(y, x));
  return (it == rend.cells.end()) ? 0 : it->second.color_pair;
}

} // namespace

int main() {
  using cuwacunu::iinuji::render_image_grayscale;
  using cuwacunu::iinuji::set_renderer;
  using cuwacunu::iinuji::detail::average_rgba_for_braille_cell;
  using cuwacunu::iinuji::detail::average_rgba_for_region_samples;
  using cuwacunu::iinuji::detail::best_fit_braille_pattern;
  using cuwacunu::iinuji::detail::bilinear_sample_rgba;
  using cuwacunu::iinuji::detail::braille_cell_samples_t;
  using cuwacunu::iinuji::detail::color_pair_for_braille_cell;
  using cuwacunu::iinuji::detail::fit_image_to_terminal_viewport;
  using cuwacunu::iinuji::detail::mode_rgba_for_braille_cell;
  using cuwacunu::iinuji::detail::quantized_blended_rgb_for_rgba;
  using cuwacunu::iinuji::detail::quantized_visible_rgb_for_rgba;
  using cuwacunu::iinuji::detail::representative_rgba_for_braille_pattern;
  using cuwacunu::iinuji::detail::sample_braille_cell;
  using cuwacunu::iinuji::detail::sampled_rgba_t;
  using cuwacunu::iinuji::detail::select_rgba_for_braille_cell;

  MockRend rend{};
  auto *old_renderer = set_renderer(&rend);

  const auto fringe_sample = bilinear_sample_rgba(
      make_horizontal_rgba(255, 0, 0, 255, 255, 255, 255, 0), 0.5, 0.0);
  TEST_ASSERT(fringe_sample.a > 0.45 && fringe_sample.a < 0.55);
  TEST_ASSERT(fringe_sample.r > 0.95);
  TEST_ASSERT(fringe_sample.g < 0.05);
  TEST_ASSERT(fringe_sample.b < 0.05);

  const auto linear_midpoint = bilinear_sample_rgba(
      make_horizontal_rgba(255, 0, 0, 255, 0, 0, 0, 255), 0.5, 0.0);
  TEST_ASSERT(linear_midpoint.a > 0.99);
  TEST_ASSERT(linear_midpoint.r > 0.72 && linear_midpoint.r < 0.75);
  TEST_ASSERT(linear_midpoint.g < 0.02);
  TEST_ASSERT(linear_midpoint.b < 0.02);

  image_grayscale_options_t blend_opts{};
  blend_opts.color_levels = 256;
  const auto blended_half_red = quantized_blended_rgb_for_rgba(
      sampled_rgba_t{1.0, 0.0, 0.0, 0.5}, blend_opts);
  TEST_ASSERT(blended_half_red.r > 180 && blended_half_red.r < 195);
  TEST_ASSERT(blended_half_red.g == 0);
  TEST_ASSERT(blended_half_red.b == 0);

  const auto visible_half_red = quantized_visible_rgb_for_rgba(
      sampled_rgba_t{1.0, 0.0, 0.0, 0.5}, blend_opts);
  TEST_ASSERT(visible_half_red.r > 250);
  TEST_ASSERT(visible_half_red.g == 0);
  TEST_ASSERT(visible_half_red.b == 0);

  image_grayscale_options_t shadow_opts{};
  shadow_opts.background_color = "#101014";
  shadow_opts.color_levels = 4;
  const auto visible_near_black = quantized_visible_rgb_for_rgba(
      sampled_rgba_t{0.02, 0.02, 0.10, 1.0}, shadow_opts);
  TEST_ASSERT(visible_near_black.r == visible_near_black.g);
  TEST_ASSERT(visible_near_black.g == visible_near_black.b);
  TEST_ASSERT(visible_near_black.r >= 80);

  const auto visible_black = quantized_visible_rgb_for_rgba(
      sampled_rgba_t{0.0, 0.0, 0.0, 1.0}, shadow_opts);
  TEST_ASSERT(visible_black.r == visible_black.g);
  TEST_ASSERT(visible_black.g == visible_black.b);
  TEST_ASSERT(visible_black.r >= 80);

  const auto edge_fit = fit_image_to_terminal_viewport(
      make_solid_rgba(1, 2, 255, 0, 0, 255), 2, 1, true, true);
  const auto edge_region =
      average_rgba_for_region_samples(make_solid_rgba(1, 2, 255, 0, 0, 255),
                                      edge_fit, 0.0, 0.0, 1.0, 2.0, 2, 4);
  TEST_ASSERT(edge_region.a > 0.45 && edge_region.a < 0.55);
  TEST_ASSERT(edge_region.r > 0.95);
  TEST_ASSERT(edge_region.g < 0.05);
  TEST_ASSERT(edge_region.b < 0.05);

  image_grayscale_options_t braille_opts{};
  braille_opts.mode = image_grayscale_mode_t::Braille;

  rend.clear();
  render_image_grayscale(make_braille_pattern_image(0x01), 0, 0, 1, 1,
                         braille_opts);
  TEST_ASSERT(glyph_at(rend, 0, 0) == static_cast<wchar_t>(0x2801));

  rend.clear();
  render_image_grayscale(make_braille_pattern_image(0xFF), 0, 0, 1, 1,
                         braille_opts);
  TEST_ASSERT(glyph_at(rend, 0, 0) == static_cast<wchar_t>(0x28FF));

  rend.clear();
  render_image_grayscale(make_solid_rgba(4, 8, 210, 210, 210, 255), 0, 0, 1, 1,
                         braille_opts);
  TEST_ASSERT(glyph_at(rend, 0, 0) != L' ');

  image_grayscale_options_t color_opts{};
  color_opts.mode = image_grayscale_mode_t::Braille;
  color_opts.use_color = true;
  color_opts.color_pair_resolver = [](std::uint8_t r, std::uint8_t g,
                                      std::uint8_t b) -> short {
    if (r > 200 && g < 80 && b < 80)
      return 17;
    if (r > 120 && g < 120 && b < 120)
      return 27;
    if (r > 120 && g > 120 && b > 120)
      return 37;
    return 0;
  };

  rend.clear();
  render_image_grayscale(make_braille_pattern_image(0xFF, 255, 0, 0), 0, 0, 1,
                         1, color_opts);
  TEST_ASSERT(glyph_at(rend, 0, 0) != L' ');
  TEST_ASSERT(color_pair_at(rend, 0, 0) == 17);

  const rgba_image_t transparent_red =
      make_braille_pattern_image(0xFF, 255, 0, 0, 96, 0, 0, 0, 0);
  const auto transparent_red_fit = fit_image_to_terminal_viewport(
      transparent_red, 1, 1, color_opts.preserve_aspect, color_opts.center);
  const auto transparent_red_samples = sample_braille_cell(
      transparent_red, transparent_red_fit, 0, 0, color_opts);
  TEST_ASSERT(color_pair_for_braille_cell(transparent_red_samples, 0xFF,
                                          color_opts) == 17);

  const rgba_image_t mixed_braille = make_braille_two_tone_image(
      0xFF, 255, 0, 0, 0, 255, 255, 255, 0, 0, 0, 0);
  const auto mixed_braille_fit = fit_image_to_terminal_viewport(
      mixed_braille, 1, 1, color_opts.preserve_aspect, color_opts.center);
  const auto mixed_braille_samples =
      sample_braille_cell(mixed_braille, mixed_braille_fit, 0, 0, color_opts);
  TEST_ASSERT(color_pair_for_braille_cell(mixed_braille_samples, 0xFF,
                                          color_opts) == 17);

  image_grayscale_options_t average_color_opts = color_opts;
  average_color_opts.color_kernel = image_color_kernel_t::Average;
  TEST_ASSERT(color_pair_for_braille_cell(mixed_braille_samples, 0xFF,
                                          average_color_opts) == 37);

  const auto full_red_cell = average_rgba_for_braille_cell(
      sample_braille_cell(make_braille_pattern_image(0xFF, 255, 0, 0),
                          fit_image_to_terminal_viewport(
                              make_braille_pattern_image(0xFF, 255, 0, 0), 1, 1,
                              color_opts.preserve_aspect, color_opts.center),
                          0, 0, color_opts));
  TEST_ASSERT(full_red_cell.a > 0.99);
  TEST_ASSERT(full_red_cell.r > 0.95);
  TEST_ASSERT(full_red_cell.g < 0.05);
  TEST_ASSERT(full_red_cell.b < 0.05);

  braille_cell_samples_t kernel_samples{};
  for (int sy = 0; sy < 4; ++sy) {
    for (int sx = 0; sx < 2; ++sx) {
      kernel_samples.valid[sy][sx] = true;
      kernel_samples.rgba[sy][sx] = sampled_rgba_t{1.0, 0.0, 0.0, 1.0};
    }
  }
  kernel_samples.rgba[3][1] = sampled_rgba_t{0.0, 0.0, 1.0, 1.0};

  image_grayscale_options_t kernel_opts{};
  kernel_opts.use_color = true;
  kernel_opts.color_levels = 256;
  const auto default_mode_rgba =
      select_rgba_for_braille_cell(kernel_samples, kernel_opts);
  TEST_ASSERT(default_mode_rgba.r > 0.95);
  TEST_ASSERT(default_mode_rgba.g < 0.05);
  TEST_ASSERT(default_mode_rgba.b < 0.05);

  kernel_opts.color_kernel = image_color_kernel_t::Average;
  const auto average_mode_rgba =
      select_rgba_for_braille_cell(kernel_samples, kernel_opts);
  TEST_ASSERT(average_mode_rgba.r > 0.80);
  TEST_ASSERT(average_mode_rgba.b > 0.10);

  const auto explicit_mode_rgba =
      mode_rgba_for_braille_cell(kernel_samples, kernel_opts);
  TEST_ASSERT(explicit_mode_rgba.r > 0.95);
  TEST_ASSERT(explicit_mode_rgba.g < 0.05);
  TEST_ASSERT(explicit_mode_rgba.b < 0.05);

  const auto solid_yellow_fit = fit_image_to_terminal_viewport(
      make_solid_rgba(4, 8, 255, 255, 0, 255), 1, 1, color_opts.preserve_aspect,
      color_opts.center);
  const auto solid_yellow_samples =
      sample_braille_cell(make_solid_rgba(4, 8, 255, 255, 0, 255),
                          solid_yellow_fit, 0, 0, color_opts);
  TEST_ASSERT(best_fit_braille_pattern(solid_yellow_samples, color_opts) ==
              static_cast<unsigned char>(0xFF));

  const auto solid_cyan_fit = fit_image_to_terminal_viewport(
      make_solid_rgba(4, 8, 0, 255, 255, 255), 1, 1, color_opts.preserve_aspect,
      color_opts.center);
  const auto solid_cyan_samples =
      sample_braille_cell(make_solid_rgba(4, 8, 0, 255, 255, 255),
                          solid_cyan_fit, 0, 0, color_opts);
  TEST_ASSERT(best_fit_braille_pattern(solid_cyan_samples, color_opts) ==
              static_cast<unsigned char>(0xFF));

  braille_cell_samples_t weighted_braille{};
  weighted_braille.valid[0][0] = true;
  weighted_braille.valid[1][0] = true;
  weighted_braille.valid[0][1] = true;
  weighted_braille.rgba[0][0] = sampled_rgba_t{1.0, 1.0, 0.0, 1.0};
  weighted_braille.rgba[1][0] = sampled_rgba_t{1.0, 1.0, 0.0, 1.0};
  weighted_braille.rgba[0][1] = sampled_rgba_t{0.25, 0.25, 0.25, 1.0};
  weighted_braille.ink[0][0] = 0.39;
  weighted_braille.ink[1][0] = 0.39;
  weighted_braille.ink[0][1] = 0.84;
  const auto weighted_rep = representative_rgba_for_braille_pattern(
      weighted_braille, static_cast<unsigned char>(0x0B), color_opts);
  TEST_ASSERT(weighted_rep.r > 0.95);
  TEST_ASSERT(weighted_rep.g > 0.95);
  TEST_ASSERT(weighted_rep.b < 0.05);

  image_grayscale_options_t half_gray_opts{};
  half_gray_opts.mode = image_grayscale_mode_t::HalfBlocks;

  rend.clear();
  render_image_grayscale(
      make_vertical_split_rgba(0, 0, 0, 255, 255, 255, 255, 255), 0, 0, 1, 1,
      half_gray_opts);
  TEST_ASSERT(glyph_at(rend, 0, 0) == L'\u2580');

  rend.clear();
  render_image_grayscale(make_solid_rgba(4, 8, 128, 128, 128, 255), 0, 0, 1, 1,
                         half_gray_opts);
  TEST_ASSERT(glyph_at(rend, 0, 0) == L'\u2592');

  image_grayscale_options_t half_color_opts{};
  half_color_opts.mode = image_grayscale_mode_t::HalfBlocks;
  half_color_opts.use_color = true;
  half_color_opts.fg_bg_color_pair_resolver =
      [](std::uint8_t fg_r, std::uint8_t fg_g, std::uint8_t fg_b,
         std::uint8_t bg_r, std::uint8_t bg_g, std::uint8_t bg_b) -> short {
    return (fg_r > 200 && fg_g < 80 && fg_b < 80 && bg_r < 80 && bg_g < 80 &&
            bg_b > 200)
               ? 29
               : 0;
  };

  rend.clear();
  render_image_grayscale(
      make_vertical_split_rgba(255, 0, 0, 255, 0, 0, 255, 255), 0, 0, 1, 1,
      half_color_opts);
  TEST_ASSERT(glyph_at(rend, 0, 0) == L'\u2580');
  TEST_ASSERT(color_pair_at(rend, 0, 0) == 29);

  set_renderer(old_renderer);
  std::cout
      << "[ALL TESTS PASSED] headless primitive checks; run "
         "test_iinuji_image_demo or test_iinuji_image to view the image.\n";
  return 0;
}
