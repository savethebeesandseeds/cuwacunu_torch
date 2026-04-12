/* primitives/art_text.h */
#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

#include "iinuji/primitives/image.h"

namespace cuwacunu {
namespace iinuji {

struct art_text_options_t {
  int pixel_scale{16};
  int glyph_spacing{2};
  int line_spacing{1};
  int padding{1};
  bool gradient{true};
  std::string color_start{"#ffaa00"};
  std::string color_end{"#00aaaa"};
};

namespace art_text_detail {

struct bitmap_glyph_t {
  int width{5};
  std::array<std::uint8_t, 7> rows{};
};

inline bitmap_glyph_t make_glyph(int width, std::array<std::uint8_t, 7> rows) {
  return bitmap_glyph_t{width, rows};
}

inline bitmap_glyph_t uppercase_glyph(char ch) {
  switch (ch) {
  case 'A':
    return make_glyph(5, {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11});
  case 'B':
    return make_glyph(5, {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E});
  case 'C':
    return make_glyph(5, {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E});
  case 'D':
    return make_glyph(5, {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E});
  case 'E':
    return make_glyph(5, {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F});
  case 'F':
    return make_glyph(5, {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10});
  case 'G':
    return make_glyph(5, {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E});
  case 'H':
    return make_glyph(5, {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11});
  case 'I':
    return make_glyph(5, {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F});
  case 'J':
    return make_glyph(5, {0x1F, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C});
  case 'K':
    return make_glyph(5, {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11});
  case 'L':
    return make_glyph(5, {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F});
  case 'M':
    return make_glyph(5, {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11});
  case 'N':
    return make_glyph(5, {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11});
  case 'O':
    return make_glyph(5, {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E});
  case 'P':
    return make_glyph(5, {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10});
  case 'Q':
    return make_glyph(5, {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D});
  case 'R':
    return make_glyph(5, {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11});
  case 'S':
    return make_glyph(5, {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E});
  case 'T':
    return make_glyph(5, {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04});
  case 'U':
    return make_glyph(5, {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E});
  case 'V':
    return make_glyph(5, {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04});
  case 'W':
    return make_glyph(5, {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A});
  case 'X':
    return make_glyph(5, {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11});
  case 'Y':
    return make_glyph(5, {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04});
  case 'Z':
    return make_glyph(5, {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F});
  case '0':
    return make_glyph(5, {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E});
  case '1':
    return make_glyph(5, {0x04, 0x0C, 0x14, 0x04, 0x04, 0x04, 0x1F});
  case '2':
    return make_glyph(5, {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F});
  case '3':
    return make_glyph(5, {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E});
  case '4':
    return make_glyph(5, {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02});
  case '5':
    return make_glyph(5, {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E});
  case '6':
    return make_glyph(5, {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E});
  case '7':
    return make_glyph(5, {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08});
  case '8':
    return make_glyph(5, {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E});
  case '9':
    return make_glyph(5, {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E});
  case '-':
    return make_glyph(5, {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00});
  case '_':
    return make_glyph(5, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F});
  case '.':
    return make_glyph(1, {0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01});
  case ':':
    return make_glyph(1, {0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00});
  case '!':
    return make_glyph(1, {0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01});
  case '?':
    return make_glyph(5, {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04});
  case '/':
    return make_glyph(5, {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10});
  default:
    return make_glyph(5, {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04});
  }
}

inline bitmap_glyph_t lowercase_glyph(char ch) {
  switch (ch) {
  case 'a':
    return make_glyph(5, {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F});
  case 'c':
    return make_glyph(5, {0x00, 0x00, 0x0E, 0x10, 0x10, 0x11, 0x0E});
  case 'n':
    return make_glyph(5, {0x00, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11});
  case 'u':
    return make_glyph(5, {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D});
  case 'w':
    return make_glyph(5, {0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A});
  default:
    return uppercase_glyph(
        static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
  }
}

inline bitmap_glyph_t glyph_for_char(char ch) {
  if (ch == ' ')
    return make_glyph(3, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
  if (std::islower(static_cast<unsigned char>(ch)))
    return lowercase_glyph(ch);
  return uppercase_glyph(
      static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
}

inline std::vector<std::string> split_lines(const std::string &text) {
  std::vector<std::string> lines{};
  std::string current{};
  for (char ch : text) {
    if (ch == '\n') {
      lines.push_back(current);
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  lines.push_back(current);
  return lines;
}

inline detail::rgb8_triplet_t rgb8_or_default(const std::string &token,
                                              int def_r, int def_g, int def_b) {
  int r = def_r;
  int g = def_g;
  int b = def_b;
  (void)rgb8_for_token(token, r, g, b);
  return detail::rgb8_triplet_t{
      static_cast<std::uint8_t>(std::clamp(r, 0, 255)),
      static_cast<std::uint8_t>(std::clamp(g, 0, 255)),
      static_cast<std::uint8_t>(std::clamp(b, 0, 255))};
}

inline detail::rgb8_triplet_t mix_rgb8_linear(const detail::rgb8_triplet_t &lhs,
                                              const detail::rgb8_triplet_t &rhs,
                                              double t) {
  t = std::clamp(t, 0.0, 1.0);
  const double lr =
      detail::srgb_to_linear(static_cast<double>(lhs.r) / 255.0) * (1.0 - t) +
      detail::srgb_to_linear(static_cast<double>(rhs.r) / 255.0) * t;
  const double lg =
      detail::srgb_to_linear(static_cast<double>(lhs.g) / 255.0) * (1.0 - t) +
      detail::srgb_to_linear(static_cast<double>(rhs.g) / 255.0) * t;
  const double lb =
      detail::srgb_to_linear(static_cast<double>(lhs.b) / 255.0) * (1.0 - t) +
      detail::srgb_to_linear(static_cast<double>(rhs.b) / 255.0) * t;
  return detail::rgb8_triplet_t{
      static_cast<std::uint8_t>(std::clamp(
          std::lround(detail::linear_to_srgb(lr) * 255.0), 0l, 255l)),
      static_cast<std::uint8_t>(std::clamp(
          std::lround(detail::linear_to_srgb(lg) * 255.0), 0l, 255l)),
      static_cast<std::uint8_t>(std::clamp(
          std::lround(detail::linear_to_srgb(lb) * 255.0), 0l, 255l))};
}

inline void paint_block(rgba_image_t &image, int x0, int y0, int scale,
                        const detail::rgb8_triplet_t &rgb) {
  for (int py = 0; py < scale; ++py) {
    for (int px = 0; px < scale; ++px) {
      const int x = x0 + px;
      const int y = y0 + py;
      const std::size_t idx =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) +
           static_cast<std::size_t>(x)) *
          4u;
      image.pixels[idx + 0] = rgb.r;
      image.pixels[idx + 1] = rgb.g;
      image.pixels[idx + 2] = rgb.b;
      image.pixels[idx + 3] = 255;
    }
  }
}

} // namespace art_text_detail

inline rgba_image_t
rasterize_art_text_rgba(const std::string &text,
                        const art_text_options_t &opt = {}) {
  const int scale = std::max(1, opt.pixel_scale);
  const int glyph_spacing = std::max(0, opt.glyph_spacing);
  const int line_spacing = std::max(0, opt.line_spacing);
  const int padding = std::max(0, opt.padding);
  const std::vector<std::string> lines = art_text_detail::split_lines(text);

  std::vector<int> line_widths{};
  line_widths.reserve(lines.size());
  int logical_w = 0;
  for (const auto &line : lines) {
    int width = 0;
    bool first = true;
    for (char ch : line) {
      const auto glyph = art_text_detail::glyph_for_char(ch);
      if (!first)
        width += glyph_spacing;
      width += glyph.width;
      first = false;
    }
    line_widths.push_back(width);
    logical_w = std::max(logical_w, width);
  }

  const int line_count = std::max(1, static_cast<int>(lines.size()));
  const int logical_h = line_count * 7 + (line_count - 1) * line_spacing;
  rgba_image_t image{};
  image.width = std::max(1, (logical_w + padding * 2) * scale);
  image.height = std::max(1, (logical_h + padding * 2) * scale);
  image.pixels.assign(static_cast<std::size_t>(image.width) *
                          static_cast<std::size_t>(image.height) * 4u,
                      0u);

  const auto start_rgb =
      art_text_detail::rgb8_or_default(opt.color_start, 255, 170, 0);
  const auto end_rgb =
      art_text_detail::rgb8_or_default(opt.color_end, 0, 170, 170);
  const int gradient_w = std::max(1, logical_w - 1);

  for (int line_idx = 0; line_idx < line_count; ++line_idx) {
    int pen_x = padding;
    const int pen_y = padding + line_idx * (7 + line_spacing);
    for (char ch : lines[static_cast<std::size_t>(line_idx)]) {
      const auto glyph = art_text_detail::glyph_for_char(ch);
      for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < glyph.width; ++col) {
          const int shift = glyph.width - 1 - col;
          if (((glyph.rows[static_cast<std::size_t>(row)] >> shift) & 0x1u) ==
              0u) {
            continue;
          }
          const double t = opt.gradient
                               ? static_cast<double>(pen_x + col - padding) /
                                     static_cast<double>(gradient_w)
                               : 0.0;
          const auto rgb =
              art_text_detail::mix_rgb8_linear(start_rgb, end_rgb, t);
          art_text_detail::paint_block(image, (pen_x + col) * scale,
                                       (pen_y + row) * scale, scale, rgb);
        }
      }
      pen_x += glyph.width + glyph_spacing;
    }
  }

  return image;
}

inline void render_art_text(const std::string &text, int start_x, int start_y,
                            int width_cells, int height_cells,
                            const art_text_options_t &text_opt = {},
                            const image_grayscale_options_t &render_opt = {}) {
  render_image_grayscale(rasterize_art_text_rgba(text, text_opt), start_x,
                         start_y, width_cells, height_cells, render_opt);
}

} // namespace iinuji
} // namespace cuwacunu
