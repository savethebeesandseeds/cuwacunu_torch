/* primitives/art_text.h */
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "iinuji/primitives/image.h"

namespace cuwacunu {
namespace iinuji {

enum class art_text_align_t {
  Left,
  Center,
  Right,
};

struct art_text_options_t {
  int pixel_scale{16};
  int glyph_spacing{2};
  int line_spacing{1};
  int padding{1};
  bool gradient{true};
  bool gradient_per_line{true};
  art_text_align_t align{art_text_align_t::Left};
  // When true, the bundled bitmap font's lowercase glyphs are used.
  // When false (default), lowercase letters fall back to their uppercase glyphs.
  bool preserve_sparse_lowercase{false};
  std::string color_start{"#ffaa00"};
  std::string color_end{"#00aaaa"};
};

inline image_grayscale_options_t default_art_text_render_options() {
  image_grayscale_options_t opt{};
  opt.mode = image_grayscale_mode_t::Braille;
  opt.use_color = true;
  opt.center = false;
  opt.sample_nearest = true;
  return opt;
}

namespace art_text_detail {

inline constexpr int kGlyphHeight = 7;
inline constexpr int kDefaultGlyphWidth = 5;
inline constexpr int kSpaceWidth = 3;
inline constexpr int kTabWidth = 4;

struct bitmap_glyph_t {
  int width{kDefaultGlyphWidth};
  std::array<std::uint8_t, kGlyphHeight> rows{};
};

inline bitmap_glyph_t make_glyph(int width,
                                 std::array<std::uint8_t, kGlyphHeight> rows) {
  return bitmap_glyph_t{width, rows};
}

inline bool ascii_is_lower(char ch) { return ch >= 'a' && ch <= 'z'; }

inline char ascii_upper(char ch) {
  return ascii_is_lower(ch) ? static_cast<char>(ch - 'a' + 'A') : ch;
}

inline bool ascii_is_printable(unsigned char ch) {
  return ch >= 0x20u && ch <= 0x7Eu;
}

inline bool is_utf8_continuation(unsigned char ch) {
  return (ch & 0xC0u) == 0x80u;
}

inline std::size_t utf8_span(const std::string &text, std::size_t index) {
  if (index >= text.size()) {
    return 0u;
  }
  const unsigned char lead = static_cast<unsigned char>(text[index]);
  std::size_t expected = 1u;
  if ((lead & 0xE0u) == 0xC0u) {
    expected = 2u;
  } else if ((lead & 0xF0u) == 0xE0u) {
    expected = 3u;
  } else if ((lead & 0xF8u) == 0xF0u) {
    expected = 4u;
  }

  std::size_t span = 1u;
  while (span < expected && index + span < text.size() &&
         is_utf8_continuation(static_cast<unsigned char>(text[index + span]))) {
    ++span;
  }
  return span;
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
    return make_glyph(3, {0x07, 0x02, 0x02, 0x02, 0x02, 0x02, 0x07});
  case 'J':
    return make_glyph(4, {0x07, 0x02, 0x02, 0x02, 0x0A, 0x0A, 0x04});
  case 'K':
    return make_glyph(5, {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11});
  case 'L':
    return make_glyph(5, {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F});
  case 'M':
    return make_glyph(5, {0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11});
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
    return make_glyph(5, {0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E});
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
  case 'e':
    return make_glyph(5, {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E});
  case 'b':
    return make_glyph(5, {0x00, 0x00, 0x1E, 0x11, 0x1F, 0x11, 0x1E});
  case 'c':
    return make_glyph(5, {0x00, 0x00, 0x0E, 0x10, 0x10, 0x11, 0x0E});
  case 'd':
    return make_glyph(5, {0x00, 0x00, 0x0F, 0x11, 0x11, 0x11, 0x0F});
  case 'f':
    return make_glyph(5, {0x00, 0x02, 0x1F, 0x02, 0x02, 0x02, 0x02});
  case 'g':
    return make_glyph(5, {0x00, 0x00, 0x0F, 0x11, 0x0F, 0x01, 0x0E});
  case 'h':
    return make_glyph(5, {0x00, 0x10, 0x10, 0x16, 0x19, 0x11, 0x11});
  case 'i':
    return make_glyph(5, {0x00, 0x04, 0x00, 0x0C, 0x04, 0x04, 0x0E});
  case 'j':
    return make_glyph(5, {0x00, 0x08, 0x00, 0x0C, 0x04, 0x04, 0x18});
  case 'm':
    return make_glyph(5, {0x00, 0x00, 0x1A, 0x15, 0x15, 0x15, 0x15});
  case 'n':
    return make_glyph(5, {0x00, 0x00, 0x1E, 0x11, 0x11, 0x11, 0x11});
  case 'o':
    return make_glyph(5, {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E});
  case 'k':
    return make_glyph(5, {0x00, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12});
  case 'l':
    return make_glyph(5, {0x00, 0x08, 0x00, 0x08, 0x08, 0x08, 0x07});
  case 'p':
    return make_glyph(5, {0x00, 0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10});
  case 'q':
    return make_glyph(5, {0x00, 0x00, 0x0E, 0x11, 0x11, 0x0E, 0x01});
  case 'u':
    return make_glyph(5, {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D});
  case 't':
    return make_glyph(5, {0x00, 0x04, 0x1E, 0x04, 0x04, 0x04, 0x08});
  case 'w':
    return make_glyph(5, {0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A});
  case 'r':
    return make_glyph(5, {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10});
  case 's':
    return make_glyph(5, {0x00, 0x00, 0x0F, 0x10, 0x0E, 0x01, 0x1E});
  case 'y':
    return make_glyph(5, {0x00, 0x00, 0x11, 0x11, 0x11, 0x0F, 0x01});
  case 'v':
    return make_glyph(5, {0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04});
  case 'x':
    return make_glyph(5, {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11});
  case 'z':
    return make_glyph(5, {0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F});
  default:
    return uppercase_glyph(ascii_upper(ch));
  }
}

inline bitmap_glyph_t glyph_for_char(char ch, bool preserve_sparse_lowercase) {
  if (ch == ' ')
    return make_glyph(kSpaceWidth, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
  if (preserve_sparse_lowercase && ascii_is_lower(ch))
    return lowercase_glyph(ch);
  return uppercase_glyph(ascii_upper(ch));
}

inline std::vector<std::string> split_lines(const std::string &text) {
  std::vector<std::string> lines(1u);
  for (std::size_t i = 0; i < text.size();) {
    const unsigned char ch = static_cast<unsigned char>(text[i]);
    if (ch == '\r') {
      if (i + 1u < text.size() && text[i + 1u] == '\n') {
        ++i;
      }
      lines.emplace_back();
      ++i;
      continue;
    }
    if (ch == '\n') {
      lines.emplace_back();
      ++i;
      continue;
    }
    if (ch == '\t') {
      lines.back().append(kTabWidth, ' ');
      ++i;
      continue;
    }
    if (ascii_is_printable(ch)) {
      lines.back().push_back(static_cast<char>(ch));
      ++i;
      continue;
    }
    lines.back().push_back('?');
    i += utf8_span(text, i);
  }
  return lines;
}

inline image_color::rgb8_triplet_t
rgb8_or_default(const std::string &token, int def_r, int def_g, int def_b) {
  int r = def_r;
  int g = def_g;
  int b = def_b;
  (void)rgb8_for_token(token, r, g, b);
  return image_color::rgb8_triplet_t{
      static_cast<std::uint8_t>(std::clamp(r, 0, 255)),
      static_cast<std::uint8_t>(std::clamp(g, 0, 255)),
      static_cast<std::uint8_t>(std::clamp(b, 0, 255))};
}

inline image_color::rgb8_triplet_t
mix_rgb8_linear(const image_color::rgb8_triplet_t &lhs,
                const image_color::rgb8_triplet_t &rhs, double t) {
  t = std::clamp(t, 0.0, 1.0);
  const double lr =
      image_color::srgb_to_linear(static_cast<double>(lhs.r) / 255.0) *
          (1.0 - t) +
      image_color::srgb_to_linear(static_cast<double>(rhs.r) / 255.0) * t;
  const double lg =
      image_color::srgb_to_linear(static_cast<double>(lhs.g) / 255.0) *
          (1.0 - t) +
      image_color::srgb_to_linear(static_cast<double>(rhs.g) / 255.0) * t;
  const double lb =
      image_color::srgb_to_linear(static_cast<double>(lhs.b) / 255.0) *
          (1.0 - t) +
      image_color::srgb_to_linear(static_cast<double>(rhs.b) / 255.0) * t;
  return image_color::rgb8_triplet_t{
      static_cast<std::uint8_t>(std::clamp(
          std::lround(image_color::linear_to_srgb(lr) * 255.0), 0l, 255l)),
      static_cast<std::uint8_t>(std::clamp(
          std::lround(image_color::linear_to_srgb(lg) * 255.0), 0l, 255l)),
      static_cast<std::uint8_t>(std::clamp(
          std::lround(image_color::linear_to_srgb(lb) * 255.0), 0l, 255l))};
}

inline void paint_block(rgba_image_t &image, int x0, int y0, int scale,
                        const image_color::rgb8_triplet_t &rgb) {
  for (int py = 0; py < scale; ++py) {
    const int y = y0 + py;
    const std::size_t row_start =
        (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width)) *
        4u;
    for (int px = 0; px < scale; ++px) {
      const int x = x0 + px;
      const std::size_t idx = row_start + static_cast<std::size_t>(x) * 4u;
      image.pixels[idx + 0] = rgb.r;
      image.pixels[idx + 1] = rgb.g;
      image.pixels[idx + 2] = rgb.b;
      image.pixels[idx + 3] = 255;
    }
  }
}

inline int line_align_offset(int block_width, int line_width,
                             art_text_align_t align) {
  const int slack = std::max(0, block_width - line_width);
  switch (align) {
  case art_text_align_t::Center:
    return slack / 2;
  case art_text_align_t::Right:
    return slack;
  case art_text_align_t::Left:
  default:
    return 0;
  }
}

inline std::vector<image_color::rgb8_triplet_t>
build_gradient_lut(const image_color::rgb8_triplet_t &start_rgb,
                   const image_color::rgb8_triplet_t &end_rgb,
                   int column_count) {
  const int safe_columns = std::max(1, column_count);
  const int denom = std::max(1, safe_columns - 1);
  std::vector<image_color::rgb8_triplet_t> lut{};
  lut.reserve(static_cast<std::size_t>(safe_columns));
  for (int x = 0; x < safe_columns; ++x) {
    const double t = static_cast<double>(x) / static_cast<double>(denom);
    lut.push_back(mix_rgb8_linear(start_rgb, end_rgb, t));
  }
  return lut;
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
  std::vector<std::vector<art_text_detail::bitmap_glyph_t>> line_glyphs{};
  line_widths.reserve(lines.size());
  line_glyphs.reserve(lines.size());
  int logical_w = 0;
  for (const auto &line : lines) {
    int width = 0;
    bool first = true;
    auto &glyphs = line_glyphs.emplace_back();
    glyphs.reserve(line.size());
    for (char ch : line) {
      const auto glyph =
          art_text_detail::glyph_for_char(ch, opt.preserve_sparse_lowercase);
      glyphs.push_back(glyph);
      if (!first)
        width += glyph_spacing;
      width += glyph.width;
      first = false;
    }
    line_widths.push_back(width);
    logical_w = std::max(logical_w, width);
  }

  const int line_count = std::max(1, static_cast<int>(lines.size()));
  const int logical_h = line_count * art_text_detail::kGlyphHeight +
                        (line_count - 1) * line_spacing;
  const std::int64_t image_w64 =
      std::max<std::int64_t>(1, (static_cast<std::int64_t>(logical_w) +
                                 static_cast<std::int64_t>(padding) * 2ll) *
                                    static_cast<std::int64_t>(scale));
  const std::int64_t image_h64 =
      std::max<std::int64_t>(1, (static_cast<std::int64_t>(logical_h) +
                                 static_cast<std::int64_t>(padding) * 2ll) *
                                    static_cast<std::int64_t>(scale));
  if (image_w64 > std::numeric_limits<int>::max() ||
      image_h64 > std::numeric_limits<int>::max()) {
    return {};
  }

  rgba_image_t image{};
  image.width = static_cast<int>(image_w64);
  image.height = static_cast<int>(image_h64);
  const std::size_t pixel_count = static_cast<std::size_t>(image.width) *
                                  static_cast<std::size_t>(image.height);
  if (pixel_count >
      std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(4u)) {
    return {};
  }
  image.pixels.assign(pixel_count * 4u, 0u);

  const auto start_rgb =
      art_text_detail::rgb8_or_default(opt.color_start, 255, 170, 0);
  const auto end_rgb =
      art_text_detail::rgb8_or_default(opt.color_end, 0, 170, 170);
  const auto global_gradient_lut = art_text_detail::build_gradient_lut(
      start_rgb, end_rgb, std::max(1, logical_w));

  for (int line_idx = 0; line_idx < line_count; ++line_idx) {
    const int line_width = line_widths[static_cast<std::size_t>(line_idx)];
    const int line_offset =
        art_text_detail::line_align_offset(logical_w, line_width, opt.align);
    int pen_x = padding + line_offset;
    int line_x = 0;
    const int pen_y =
        padding + line_idx * (art_text_detail::kGlyphHeight + line_spacing);
    const auto line_gradient_lut =
        opt.gradient && opt.gradient_per_line
            ? art_text_detail::build_gradient_lut(start_rgb, end_rgb,
                                                  std::max(1, line_width))
            : std::vector<image_color::rgb8_triplet_t>{};
    const auto &glyphs = line_glyphs[static_cast<std::size_t>(line_idx)];
    for (const auto &glyph : glyphs) {
      for (int row = 0; row < art_text_detail::kGlyphHeight; ++row) {
        for (int col = 0; col < glyph.width; ++col) {
          const int shift = glyph.width - 1 - col;
          if (((glyph.rows[static_cast<std::size_t>(row)] >> shift) & 0x1u) ==
              0u) {
            continue;
          }
          const auto &rgb =
              opt.gradient
                  ? (opt.gradient_per_line
                         ? line_gradient_lut[static_cast<std::size_t>(
                               std::clamp(line_x + col, 0,
                                          std::max(0, line_width - 1)))]
                         : global_gradient_lut[static_cast<std::size_t>(
                               std::clamp(pen_x + col - padding, 0,
                                          std::max(0, logical_w - 1)))])
                  : start_rgb;
          art_text_detail::paint_block(image, (pen_x + col) * scale,
                                       (pen_y + row) * scale, scale, rgb);
        }
      }
      pen_x += glyph.width + glyph_spacing;
      line_x += glyph.width + glyph_spacing;
    }
  }

  return image;
}

inline void render_art_text(const std::string &text, int start_x, int start_y,
                            int width_cells, int height_cells,
                            const art_text_options_t &text_opt = {},
                            const image_grayscale_options_t &render_opt =
                                default_art_text_render_options()) {
  render_image_grayscale(rasterize_art_text_rgba(text, text_opt), start_x,
                         start_y, width_cells, height_cells, render_opt);
}

} // namespace iinuji
} // namespace cuwacunu
