#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include "iinuji/iinuji_types.h"
#include "iinuji/primitives/animation.h"
#include "iinuji/primitives/art_text.h"
#include "iinuji/primitives/image.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct home_showcase_assets_t {
  rgba_image_t loading_logo_image{};
  std::string loading_logo_error{};
  bool loading_logo_ok{false};

  rgba_animation_t home_animation{};
  std::string home_animation_error{};
  bool home_animation_ok{false};
  bool home_animation_is_dynamic{false};

  rgba_image_t home_wordmark_image{};
};

inline art_text_options_t make_home_showcase_wordmark_text_options() {
  art_text_options_t wordmark_opt{};
  wordmark_opt.pixel_scale = 4;
  wordmark_opt.glyph_spacing = 2;
  wordmark_opt.line_spacing = 1;
  wordmark_opt.padding = 0;
  wordmark_opt.gradient = false;
  wordmark_opt.color_start = "#4fcf6b";
  wordmark_opt.color_end = "#4fcf6b";
  return wordmark_opt;
}

inline rgba_image_t
make_home_showcase_wordmark_reference_image(const art_text_options_t &opt) {
  return rasterize_art_text_rgba("Cuwacunu", opt);
}

inline double
home_showcase_wordmark_width_ratio(const rgba_animation_t &animation,
                                   const rgba_image_t &reference_wordmark) {
  if (animation.width > 0 && reference_wordmark.valid() &&
      reference_wordmark.width > 0) {
    return std::clamp(static_cast<double>(reference_wordmark.width) /
                          static_cast<double>(animation.width),
                      0.35, 0.90);
  }
  return 0.72;
}

inline rgba_image_t make_home_showcase_wordmark_image(
    const art_text_options_t &base_opt, const rgba_image_t &reference_wordmark,
    double width_ratio, int ref_width, int ref_height) {
  if (!reference_wordmark.valid()) {
    return reference_wordmark;
  }

  art_text_options_t wordmark_opt = base_opt;
  const int safe_ref_width = std::max(1, ref_width);
  const int safe_ref_height = std::max(1, ref_height);
  const int target_width =
      std::max(1, static_cast<int>(std::lround(
                      static_cast<double>(safe_ref_width) * width_ratio)));
  const double width_scale = static_cast<double>(target_width) /
                             static_cast<double>(reference_wordmark.width);
  wordmark_opt.pixel_scale =
      std::clamp(static_cast<int>(std::lround(
                     static_cast<double>(base_opt.pixel_scale) * width_scale)),
                 2, 24);
  wordmark_opt.glyph_spacing = std::max(2, (wordmark_opt.pixel_scale + 1) / 2);

  rgba_image_t wordmark = rasterize_art_text_rgba("Cuwacunu", wordmark_opt);
  const int max_height =
      std::max(1, static_cast<int>(std::lround(safe_ref_height * 0.30)));
  while (wordmark.valid() &&
         (wordmark.width > safe_ref_width || wordmark.height > max_height) &&
         wordmark_opt.pixel_scale > 1) {
    --wordmark_opt.pixel_scale;
    wordmark_opt.glyph_spacing =
        std::max(2, (wordmark_opt.pixel_scale + 1) / 2);
    wordmark = rasterize_art_text_rgba("Cuwacunu", wordmark_opt);
  }
  return wordmark;
}

inline home_showcase_assets_t
load_home_showcase_assets(const std::string &loading_logo_path,
                          const std::string &home_animation_path) {
  home_showcase_assets_t assets{};

  assets.loading_logo_ok = decode_png_rgba_file(
      loading_logo_path, assets.loading_logo_image, assets.loading_logo_error);
  assets.home_animation_ok = decode_animation_rgba_file(
      home_animation_path, assets.home_animation, assets.home_animation_error);
  assets.home_animation_is_dynamic =
      assets.home_animation_ok && assets.home_animation.frames.size() > 1u;

  const art_text_options_t wordmark_opt =
      make_home_showcase_wordmark_text_options();
  const rgba_image_t reference_wordmark =
      make_home_showcase_wordmark_reference_image(wordmark_opt);
  const double width_ratio = home_showcase_wordmark_width_ratio(
      assets.home_animation, reference_wordmark);

  assets.home_wordmark_image = make_home_showcase_wordmark_image(
      wordmark_opt, reference_wordmark, width_ratio,
      assets.home_animation_ok
          ? assets.home_animation.width
          : (assets.loading_logo_ok ? assets.loading_logo_image.width
                                    : reference_wordmark.width),
      assets.home_animation_ok
          ? assets.home_animation.height
          : (assets.loading_logo_ok ? assets.loading_logo_image.height
                                    : reference_wordmark.height));
  return assets;
}

inline image_grayscale_options_t make_home_showcase_render_options() {
  image_grayscale_options_t opt{};
  opt.mode = image_grayscale_mode_t::Braille;
  opt.use_color = true;
  opt.background_color = "#101014";
  opt.color_levels = 4;
  return opt;
}

inline void alpha_blend_home_showcase_rgba_over(std::uint8_t *dst,
                                                const std::uint8_t *src) {
  const double src_a = static_cast<double>(src[3]) / 255.0;
  if (src_a <= 0.0)
    return;
  if (src_a >= 1.0) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    return;
  }

  const double dst_a = static_cast<double>(dst[3]) / 255.0;
  const double out_a = src_a + dst_a * (1.0 - src_a);
  if (out_a <= 1.0e-9) {
    dst[0] = 0;
    dst[1] = 0;
    dst[2] = 0;
    dst[3] = 0;
    return;
  }

  const double src_r =
      detail::srgb_to_linear(static_cast<double>(src[0]) / 255.0);
  const double src_g =
      detail::srgb_to_linear(static_cast<double>(src[1]) / 255.0);
  const double src_b =
      detail::srgb_to_linear(static_cast<double>(src[2]) / 255.0);
  const double dst_r =
      detail::srgb_to_linear(static_cast<double>(dst[0]) / 255.0);
  const double dst_g =
      detail::srgb_to_linear(static_cast<double>(dst[1]) / 255.0);
  const double dst_b =
      detail::srgb_to_linear(static_cast<double>(dst[2]) / 255.0);

  const double out_r = (src_r * src_a + dst_r * dst_a * (1.0 - src_a)) / out_a;
  const double out_g = (src_g * src_a + dst_g * dst_a * (1.0 - src_a)) / out_a;
  const double out_b = (src_b * src_a + dst_b * dst_a * (1.0 - src_a)) / out_a;

  dst[0] = static_cast<std::uint8_t>(
      std::clamp(std::lround(detail::linear_to_srgb(out_r) * 255.0), 0l, 255l));
  dst[1] = static_cast<std::uint8_t>(
      std::clamp(std::lround(detail::linear_to_srgb(out_g) * 255.0), 0l, 255l));
  dst[2] = static_cast<std::uint8_t>(
      std::clamp(std::lround(detail::linear_to_srgb(out_b) * 255.0), 0l, 255l));
  dst[3] = static_cast<std::uint8_t>(
      std::clamp(std::lround(out_a * 255.0), 0l, 255l));
}

inline rgba_image_t
composite_home_wordmark_over_image(const rgba_image_t &base_image,
                                   const rgba_image_t &wordmark_image) {
  const int image_shift_x = -std::max(18, base_image.width / 7);
  const int image_shift_y = std::max(4, base_image.height / 28);
  if (!base_image.valid() || !wordmark_image.valid())
    return base_image;

  const int side_margin = std::max(2, wordmark_image.width / 24);
  const int side_pad =
      std::max(side_margin,
               std::max(0, (wordmark_image.width - base_image.width + 1) / 2) +
                   side_margin);
  const int bottom_margin = std::max(2, base_image.height / 40);
  const int wordmark_overlap = std::max(2, wordmark_image.height / 3);
  const int bottom_pad =
      std::max(bottom_margin + 1,
               wordmark_image.height - wordmark_overlap + bottom_margin);

  rgba_image_t composite{};
  composite.width = std::max(1, base_image.width + 2 * side_pad);
  composite.height =
      std::max(1, base_image.height + image_shift_y + bottom_pad);
  composite.pixels.assign(static_cast<std::size_t>(composite.width) *
                              static_cast<std::size_t>(composite.height) * 4u,
                          0u);

  const int image_x =
      std::clamp(side_pad + image_shift_x, 0,
                 std::max(0, composite.width - base_image.width));
  const int image_y = image_shift_y;
  for (int row = 0; row < base_image.height; ++row) {
    const int dst_y = image_y + row;
    if (dst_y < 0 || dst_y >= composite.height)
      continue;
    for (int col = 0; col < base_image.width; ++col) {
      const int dst_x = image_x + col;
      if (dst_x < 0 || dst_x >= composite.width)
        continue;
      const std::size_t src_idx =
          (static_cast<std::size_t>(row) *
               static_cast<std::size_t>(base_image.width) +
           static_cast<std::size_t>(col)) *
          4u;
      const std::size_t dst_idx =
          (static_cast<std::size_t>(dst_y) *
               static_cast<std::size_t>(composite.width) +
           static_cast<std::size_t>(dst_x)) *
          4u;
      composite.pixels[dst_idx + 0] = base_image.pixels[src_idx + 0];
      composite.pixels[dst_idx + 1] = base_image.pixels[src_idx + 1];
      composite.pixels[dst_idx + 2] = base_image.pixels[src_idx + 2];
      composite.pixels[dst_idx + 3] = base_image.pixels[src_idx + 3];
    }
  }

  const int overlay_x =
      std::max(0, (composite.width - wordmark_image.width) / 2);
  const int overlay_y = std::clamp(
      image_y + base_image.height - wordmark_overlap, 0,
      std::max(0, composite.height - wordmark_image.height - bottom_margin));

  for (int row = 0; row < wordmark_image.height; ++row) {
    const int dst_y = overlay_y + row;
    if (dst_y < 0 || dst_y >= composite.height)
      continue;
    for (int col = 0; col < wordmark_image.width; ++col) {
      const int dst_x = overlay_x + col;
      if (dst_x < 0 || dst_x >= composite.width)
        continue;
      const std::size_t src_idx =
          (static_cast<std::size_t>(row) *
               static_cast<std::size_t>(wordmark_image.width) +
           static_cast<std::size_t>(col)) *
          4u;
      const std::size_t dst_idx =
          (static_cast<std::size_t>(dst_y) *
               static_cast<std::size_t>(composite.width) +
           static_cast<std::size_t>(dst_x)) *
          4u;
      alpha_blend_home_showcase_rgba_over(composite.pixels.data() + dst_idx,
                                          wordmark_image.pixels.data() +
                                              src_idx);
    }
  }

  return composite;
}

inline Rect home_showcase_stage_rect(const Rect &area, double height_ratio,
                                     int top_padding_rows = 0,
                                     int bottom_reserved_rows = 1) {
  const int stage_x = area.x;
  const int stage_y =
      std::min(area.y + area.h - 1, area.y + std::max(0, top_padding_rows));
  const int stage_w = std::max(1, area.w);
  const int max_stage_h = std::max(1, area.h - std::max(0, top_padding_rows) -
                                          bottom_reserved_rows);
  const int desired_h = std::max(
      1, static_cast<int>(std::lround(static_cast<double>(area.h) *
                                      std::clamp(height_ratio, 0.1, 1.0))));
  const int stage_h = std::min(max_stage_h, desired_h);
  return Rect{stage_x, stage_y, stage_w, stage_h};
}

inline void
render_home_showcase_static(const rgba_image_t &image, const Rect &area,
                            const image_grayscale_options_t &render_opt) {
  if (!image.valid() || area.w <= 0 || area.h <= 0)
    return;
  render_image_grayscale(image, area.x, area.y, area.w, area.h, render_opt);
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
