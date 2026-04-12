/* primitives/image.h */
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>
#include <vector>

#include "iinuji/iinuji_utils.h"
#include "iinuji/render/renderer.h"

namespace cuwacunu {
namespace iinuji {

extern "C" {
using Bytef = unsigned char;
using uLong = unsigned long;
using uLongf = unsigned long;
int uncompress(Bytef *dest, uLongf *dest_len, const Bytef *src, uLong src_len);
}

struct rgba_image_t {
  int width{0};
  int height{0};
  std::vector<std::uint8_t> pixels{};

  bool valid() const {
    if (width <= 0 || height <= 0)
      return false;
    const std::size_t required =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
    return pixels.size() >= required;
  }
};

enum class image_grayscale_mode_t {
  HalfBlocks,
  Braille,
};

enum class image_color_kernel_t {
  Mode,
  Average,
};

struct image_grayscale_options_t {
  image_grayscale_mode_t mode{image_grayscale_mode_t::Braille};
  image_color_kernel_t color_kernel{image_color_kernel_t::Mode};
  short color_pair{0};
  bool use_color{false};
  bool preserve_aspect{true};
  bool center{true};
  double alpha_cutoff{1.0 / 255.0};
  double blank_threshold{0.02};
  double ink_gamma{1.0};
  double color_ink_floor{0.35};
  double braille_bias{0.0};
  int color_levels{4};
  std::string background_color{"black"};
  std::function<short(std::uint8_t, std::uint8_t, std::uint8_t)>
      color_pair_resolver{};
  std::function<short(std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t,
                      std::uint8_t, std::uint8_t)>
      fg_bg_color_pair_resolver{};
};

namespace detail {

inline constexpr unsigned char kBrailleBit[4][2] = {
    {0x01, 0x08},
    {0x02, 0x10},
    {0x04, 0x20},
    {0x40, 0x80},
};

struct sampled_rgba_t {
  double r{0.0};
  double g{0.0};
  double b{0.0};
  double a{0.0};
};

struct rgb8_triplet_t {
  std::uint8_t r{0};
  std::uint8_t g{0};
  std::uint8_t b{0};
};

struct image_fit_t {
  double x0{0.0};
  double y0{0.0};
  double w{0.0};
  double h{0.0};
};

inline double srgb_to_linear(double c) {
  c = std::clamp(c, 0.0, 1.0);
  if (c <= 0.04045)
    return c / 12.92;
  return std::pow((c + 0.055) / 1.055, 2.4);
}

inline double linear_to_srgb(double c) {
  c = std::clamp(c, 0.0, 1.0);
  if (c <= 0.0031308)
    return 12.92 * c;
  return 1.055 * std::pow(c, 1.0 / 2.4) - 0.055;
}

inline image_fit_t fit_image_to_terminal_viewport(const rgba_image_t &image,
                                                  int width_cells,
                                                  int height_cells,
                                                  bool preserve_aspect,
                                                  bool center) {
  const double viewport_w = static_cast<double>(width_cells);
  const double viewport_h = static_cast<double>(height_cells) * 2.0;
  if (!image.valid() || viewport_w <= 0.0 || viewport_h <= 0.0) {
    return {};
  }

  image_fit_t fit{};
  fit.w = viewport_w;
  fit.h = viewport_h;

  if (preserve_aspect) {
    const double src_aspect =
        static_cast<double>(image.width) / static_cast<double>(image.height);
    const double dst_aspect = viewport_w / viewport_h;
    if (src_aspect > dst_aspect) {
      fit.w = viewport_w;
      fit.h = viewport_w / src_aspect;
    } else {
      fit.h = viewport_h;
      fit.w = viewport_h * src_aspect;
    }
  }

  if (center) {
    fit.x0 = (viewport_w - fit.w) * 0.5;
    fit.y0 = (viewport_h - fit.h) * 0.5;
  }
  return fit;
}

inline bool map_phys_to_image_coords(const rgba_image_t &image,
                                     const image_fit_t &fit, double phys_x,
                                     double phys_y, double &out_x,
                                     double &out_y) {
  if (!image.valid() || fit.w <= 0.0 || fit.h <= 0.0)
    return false;
  if (phys_x < fit.x0 || phys_y < fit.y0 || phys_x >= fit.x0 + fit.w ||
      phys_y >= fit.y0 + fit.h) {
    return false;
  }

  const double nx = (phys_x - fit.x0) / fit.w;
  const double ny = (phys_y - fit.y0) / fit.h;

  out_x = nx * static_cast<double>(std::max(0, image.width - 1));
  out_y = ny * static_cast<double>(std::max(0, image.height - 1));
  return true;
}

inline sampled_rgba_t bilinear_sample_rgba(const rgba_image_t &image, double x,
                                           double y) {
  sampled_rgba_t out{};
  if (!image.valid())
    return out;

  x = std::clamp(x, 0.0, static_cast<double>(image.width - 1));
  y = std::clamp(y, 0.0, static_cast<double>(image.height - 1));

  const int x0 = static_cast<int>(std::floor(x));
  const int y0 = static_cast<int>(std::floor(y));
  const int x1 = std::min(x0 + 1, image.width - 1);
  const int y1 = std::min(y0 + 1, image.height - 1);
  const double tx = x - static_cast<double>(x0);
  const double ty = y - static_cast<double>(y0);

  auto sample = [&](int sx, int sy) -> sampled_rgba_t {
    const std::size_t idx =
        (static_cast<std::size_t>(sy) * static_cast<std::size_t>(image.width) +
         static_cast<std::size_t>(sx)) *
        4u;
    return sampled_rgba_t{
        image.pixels[idx + 0] / 255.0, image.pixels[idx + 1] / 255.0,
        image.pixels[idx + 2] / 255.0, image.pixels[idx + 3] / 255.0};
  };

  const sampled_rgba_t c00 = sample(x0, y0);
  const sampled_rgba_t c10 = sample(x1, y0);
  const sampled_rgba_t c01 = sample(x0, y1);
  const sampled_rgba_t c11 = sample(x1, y1);

  auto mix = [&](double a00, double a10, double a01, double a11) -> double {
    const double top = a00 * (1.0 - tx) + a10 * tx;
    const double bot = a01 * (1.0 - tx) + a11 * tx;
    return top * (1.0 - ty) + bot * ty;
  };

  out.a = mix(c00.a, c10.a, c01.a, c11.a);
  const double premul_r =
      mix(srgb_to_linear(c00.r) * c00.a, srgb_to_linear(c10.r) * c10.a,
          srgb_to_linear(c01.r) * c01.a, srgb_to_linear(c11.r) * c11.a);
  const double premul_g =
      mix(srgb_to_linear(c00.g) * c00.a, srgb_to_linear(c10.g) * c10.a,
          srgb_to_linear(c01.g) * c01.a, srgb_to_linear(c11.g) * c11.a);
  const double premul_b =
      mix(srgb_to_linear(c00.b) * c00.a, srgb_to_linear(c10.b) * c10.a,
          srgb_to_linear(c01.b) * c01.a, srgb_to_linear(c11.b) * c11.a);
  if (out.a > 1.0e-9) {
    out.r = linear_to_srgb(std::clamp(premul_r / out.a, 0.0, 1.0));
    out.g = linear_to_srgb(std::clamp(premul_g / out.a, 0.0, 1.0));
    out.b = linear_to_srgb(std::clamp(premul_b / out.a, 0.0, 1.0));
  }
  return out;
}

inline double ink_for_rgba(const sampled_rgba_t &rgba,
                           const image_grayscale_options_t &opt,
                           bool color_mode) {
  if (rgba.a <= opt.alpha_cutoff)
    return 0.0;

  const double luma = 0.2126 * rgba.r + 0.7152 * rgba.g + 0.0722 * rgba.b;
  const double darkness = std::clamp(1.0 - luma, 0.0, 1.0);
  const double ink =
      color_mode ? rgba.a * (opt.color_ink_floor +
                             (1.0 - opt.color_ink_floor) * darkness)
                 : rgba.a * std::pow(darkness, std::max(0.001, opt.ink_gamma));
  return std::clamp(ink, 0.0, 1.0);
}

inline double sampled_ink(const rgba_image_t &image, const image_fit_t &fit,
                          double phys_x, double phys_y,
                          const image_grayscale_options_t &opt) {
  double src_x = 0.0;
  double src_y = 0.0;
  if (!map_phys_to_image_coords(image, fit, phys_x, phys_y, src_x, src_y)) {
    return 0.0;
  }
  return ink_for_rgba(bilinear_sample_rgba(image, src_x, src_y), opt, false);
}

inline double sampled_color_ink(const rgba_image_t &image,
                                const image_fit_t &fit, double phys_x,
                                double phys_y,
                                const image_grayscale_options_t &opt) {
  double src_x = 0.0;
  double src_y = 0.0;
  if (!map_phys_to_image_coords(image, fit, phys_x, phys_y, src_x, src_y)) {
    return 0.0;
  }
  return ink_for_rgba(bilinear_sample_rgba(image, src_x, src_y), opt, true);
}

inline wchar_t shade_glyph_for_ink(double ink, double blank_threshold) {
  if (ink <= blank_threshold)
    return L' ';
  if (ink < 0.25)
    return L'\u2591';
  if (ink < 0.50)
    return L'\u2592';
  if (ink < 0.75)
    return L'\u2593';
  return L'\u2588';
}

struct braille_cell_samples_t {
  double ink[4][2]{{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}};
  sampled_rgba_t rgba[4][2]{};
  bool valid[4][2]{
      {false, false}, {false, false}, {false, false}, {false, false}};
};

inline double braille_coverage_for_rgba(const sampled_rgba_t &rgba,
                                        const image_grayscale_options_t &opt) {
  if (rgba.a <= opt.alpha_cutoff)
    return 0.0;
  return std::clamp(rgba.a, 0.0, 1.0);
}

inline double braille_ink_for_rgba(const sampled_rgba_t &rgba,
                                   const image_grayscale_options_t &opt) {
  if (!opt.use_color)
    return ink_for_rgba(rgba, opt, false);
  return braille_coverage_for_rgba(rgba, opt);
}

inline braille_cell_samples_t
sample_braille_cell(const rgba_image_t &image, const image_fit_t &fit,
                    int cell_x, int cell_y,
                    const image_grayscale_options_t &opt) {
  braille_cell_samples_t out{};
  for (int sy = 0; sy < 4; ++sy) {
    for (int sx = 0; sx < 2; ++sx) {
      const double phys_x =
          static_cast<double>(cell_x) + (static_cast<double>(sx) + 0.5) * 0.5;
      const double phys_y = static_cast<double>(cell_y) * 2.0 +
                            (static_cast<double>(sy) + 0.5) * 0.5;
      double src_x = 0.0;
      double src_y = 0.0;
      if (!map_phys_to_image_coords(image, fit, phys_x, phys_y, src_x, src_y)) {
        continue;
      }
      const sampled_rgba_t rgba = bilinear_sample_rgba(image, src_x, src_y);
      out.valid[sy][sx] = true;
      out.rgba[sy][sx] = rgba;
      out.ink[sy][sx] = braille_ink_for_rgba(rgba, opt);
    }
  }
  return out;
}

inline double braille_effective_ink(double ink,
                                    const image_grayscale_options_t &opt) {
  return std::clamp(ink + opt.braille_bias, 0.0, 1.0);
}

inline int desired_braille_dot_count(const braille_cell_samples_t &samples,
                                     const image_grayscale_options_t &opt) {
  double total_ink = 0.0;
  double max_ink = 0.0;
  for (int sy = 0; sy < 4; ++sy) {
    for (int sx = 0; sx < 2; ++sx) {
      const double ink = braille_effective_ink(samples.ink[sy][sx], opt);
      total_ink += ink;
      max_ink = std::max(max_ink, ink);
    }
  }

  if (max_ink <= opt.blank_threshold &&
      total_ink <= opt.blank_threshold * 2.0) {
    return 0;
  }

  int desired = std::clamp(static_cast<int>(std::lround(total_ink)), 0, 8);
  if (desired == 0 && max_ink > opt.blank_threshold)
    desired = 1;
  return desired;
}

inline unsigned char
best_fit_braille_pattern(const braille_cell_samples_t &samples,
                         const image_grayscale_options_t &opt) {
  const int desired_on = desired_braille_dot_count(samples, opt);
  if (desired_on <= 0)
    return 0;

  unsigned char best_pattern = 0;
  double best_score = 1.0e18;

  for (int candidate = 0; candidate < 256; ++candidate) {
    double score = 0.0;
    int on_count = 0;
    double captured_ink = 0.0;
    for (int sy = 0; sy < 4; ++sy) {
      for (int sx = 0; sx < 2; ++sx) {
        const double target =
            (candidate & static_cast<int>(kBrailleBit[sy][sx])) ? 1.0 : 0.0;
        const double ink = braille_effective_ink(samples.ink[sy][sx], opt);
        const double delta = ink - target;
        score += delta * delta;
        on_count += (target > 0.5) ? 1 : 0;
        if (target > 0.5)
          captured_ink += ink;
      }
    }
    const double density_delta = static_cast<double>(on_count - desired_on);
    score += density_delta * density_delta;
    score -= 1.0e-3 * captured_ink;
    if (score < best_score) {
      best_score = score;
      best_pattern = static_cast<unsigned char>(candidate);
    }
  }

  return best_pattern;
}

inline sampled_rgba_t
average_rgba_for_region_samples(const rgba_image_t &image,
                                const image_fit_t &fit, double phys_x0,
                                double phys_y0, double phys_w, double phys_h,
                                int sample_cols, int sample_rows) {
  sampled_rgba_t out{};
  double linear_r_sum = 0.0;
  double linear_g_sum = 0.0;
  double linear_b_sum = 0.0;
  double rgb_weight_sum = 0.0;
  double alpha_sum = 0.0;
  const int total_samples = std::max(0, sample_cols) * std::max(0, sample_rows);

  for (int sy = 0; sy < sample_rows; ++sy) {
    for (int sx = 0; sx < sample_cols; ++sx) {
      const double phys_x =
          phys_x0 + (static_cast<double>(sx) + 0.5) *
                        (phys_w / static_cast<double>(sample_cols));
      const double phys_y =
          phys_y0 + (static_cast<double>(sy) + 0.5) *
                        (phys_h / static_cast<double>(sample_rows));
      double src_x = 0.0;
      double src_y = 0.0;
      if (!map_phys_to_image_coords(image, fit, phys_x, phys_y, src_x, src_y)) {
        continue;
      }
      const sampled_rgba_t rgba = bilinear_sample_rgba(image, src_x, src_y);
      const double rgb_w = std::max(0.0, rgba.a);
      linear_r_sum += srgb_to_linear(rgba.r) * rgb_w;
      linear_g_sum += srgb_to_linear(rgba.g) * rgb_w;
      linear_b_sum += srgb_to_linear(rgba.b) * rgb_w;
      rgb_weight_sum += rgb_w;
      alpha_sum += rgba.a;
    }
  }

  if (rgb_weight_sum > 0.0) {
    out.r = linear_to_srgb(linear_r_sum / rgb_weight_sum);
    out.g = linear_to_srgb(linear_g_sum / rgb_weight_sum);
    out.b = linear_to_srgb(linear_b_sum / rgb_weight_sum);
  }
  if (total_samples > 0) {
    out.a = alpha_sum / static_cast<double>(total_samples);
  }
  return out;
}

inline double braille_color_weight(const sampled_rgba_t &rgba,
                                   const image_grayscale_options_t &opt) {
  if (rgba.a <= opt.alpha_cutoff)
    return 0.0;
  return std::max(0.0, rgba.a);
}

inline sampled_rgba_t
representative_rgba_for_braille_pattern(const braille_cell_samples_t &samples,
                                        unsigned char pattern,
                                        const image_grayscale_options_t &opt) {
  struct active_sample_t {
    sampled_rgba_t rgba{};
    double weight{0.0};
    double linear_r{0.0};
    double linear_g{0.0};
    double linear_b{0.0};
  };

  std::vector<active_sample_t> active{};
  active.reserve(8);

  for (int sy = 0; sy < 4; ++sy) {
    for (int sx = 0; sx < 2; ++sx) {
      if (!samples.valid[sy][sx] || (pattern & kBrailleBit[sy][sx]) == 0) {
        continue;
      }
      const sampled_rgba_t &rgba = samples.rgba[sy][sx];
      const double weight = braille_color_weight(rgba, opt);
      if (weight <= 0.0) {
        continue;
      }
      active.push_back(active_sample_t{
          rgba,
          weight,
          srgb_to_linear(rgba.r),
          srgb_to_linear(rgba.g),
          srgb_to_linear(rgba.b),
      });
    }
  }

  if (active.empty()) {
    return {};
  }

  std::size_t best_index = 0;
  double best_cost = 1.0e300;
  for (std::size_t i = 0; i < active.size(); ++i) {
    double cost = 0.0;
    for (std::size_t j = 0; j < active.size(); ++j) {
      const double dr = active[i].linear_r - active[j].linear_r;
      const double dg = active[i].linear_g - active[j].linear_g;
      const double db = active[i].linear_b - active[j].linear_b;
      cost += active[j].weight * (dr * dr + dg * dg + db * db);
    }
    if (cost < best_cost) {
      best_cost = cost;
      best_index = i;
    }
  }

  sampled_rgba_t out = active[best_index].rgba;
  out.a = 1.0;
  return out;
}

inline sampled_rgba_t
average_rgba_for_braille_cell(const braille_cell_samples_t &samples) {
  sampled_rgba_t out{};
  double linear_r_sum = 0.0;
  double linear_g_sum = 0.0;
  double linear_b_sum = 0.0;
  double rgb_weight_sum = 0.0;
  double alpha_sum = 0.0;
  constexpr double kTotalSamples = 8.0;

  for (int sy = 0; sy < 4; ++sy) {
    for (int sx = 0; sx < 2; ++sx) {
      if (!samples.valid[sy][sx]) {
        continue;
      }
      const sampled_rgba_t &rgba = samples.rgba[sy][sx];
      const double weight = std::max(0.0, rgba.a);
      linear_r_sum += srgb_to_linear(rgba.r) * weight;
      linear_g_sum += srgb_to_linear(rgba.g) * weight;
      linear_b_sum += srgb_to_linear(rgba.b) * weight;
      rgb_weight_sum += weight;
      alpha_sum += rgba.a;
    }
  }

  if (rgb_weight_sum > 0.0) {
    out.r = linear_to_srgb(linear_r_sum / rgb_weight_sum);
    out.g = linear_to_srgb(linear_g_sum / rgb_weight_sum);
    out.b = linear_to_srgb(linear_b_sum / rgb_weight_sum);
  }
  out.a = alpha_sum / kTotalSamples;
  return out;
}

inline sampled_rgba_t average_rgba_for_braille_pattern_ink_weighted(
    const braille_cell_samples_t &samples, unsigned char pattern,
    const image_grayscale_options_t &opt) {
  return representative_rgba_for_braille_pattern(samples, pattern, opt);
}

inline double average_ink_for_half_block_half(
    const rgba_image_t &image, const image_fit_t &fit, int cell_x, int cell_y,
    int half_index, const image_grayscale_options_t &opt) {
  double ink_sum = 0.0;
  int sample_count = 0;

  for (int sy = 0; sy < 2; ++sy) {
    for (int sx = 0; sx < 2; ++sx) {
      const double phys_x =
          static_cast<double>(cell_x) + (static_cast<double>(sx) + 0.5) * 0.5;
      const double phys_y = static_cast<double>(cell_y) * 2.0 +
                            static_cast<double>(half_index) +
                            (static_cast<double>(sy) + 0.5) * 0.5;
      ink_sum += opt.use_color
                     ? sampled_color_ink(image, fit, phys_x, phys_y, opt)
                     : sampled_ink(image, fit, phys_x, phys_y, opt);
      ++sample_count;
    }
  }

  return (sample_count > 0) ? (ink_sum / static_cast<double>(sample_count))
                            : 0.0;
}

inline sampled_rgba_t
average_rgba_for_half_block_half(const rgba_image_t &image,
                                 const image_fit_t &fit, int cell_x, int cell_y,
                                 int half_index) {
  return average_rgba_for_region_samples(
      image, fit, static_cast<double>(cell_x),
      static_cast<double>(cell_y) * 2.0 + static_cast<double>(half_index), 1.0,
      1.0, 4, 4);
}

inline std::uint8_t quantize_rgb8_channel(std::uint8_t value, int levels) {
  levels = std::max(2, levels);
  const double normalized = static_cast<double>(value) / 255.0;
  const double scaled =
      std::round(normalized * static_cast<double>(levels - 1));
  const double quantized = scaled / static_cast<double>(levels - 1);
  return static_cast<std::uint8_t>(
      std::clamp(std::lround(quantized * 255.0), 0l, 255l));
}

inline bool same_rgb8(const rgb8_triplet_t &lhs, const rgb8_triplet_t &rhs) {
  return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}

inline rgb8_triplet_t background_rgb8(const image_grayscale_options_t &opt) {
  int r = 0;
  int g = 0;
  int b = 0;
  (void)rgb8_for_token(opt.background_color, r, g, b);
  return rgb8_triplet_t{static_cast<std::uint8_t>(std::clamp(r, 0, 255)),
                        static_cast<std::uint8_t>(std::clamp(g, 0, 255)),
                        static_cast<std::uint8_t>(std::clamp(b, 0, 255))};
}

inline double rgb8_luma(const rgb8_triplet_t &rgb) {
  return 0.2126 * (static_cast<double>(rgb.r) / 255.0) +
         0.7152 * (static_cast<double>(rgb.g) / 255.0) +
         0.0722 * (static_cast<double>(rgb.b) / 255.0);
}

inline double rgba_luma(const sampled_rgba_t &rgba) {
  return 0.2126 * rgba.r + 0.7152 * rgba.g + 0.0722 * rgba.b;
}

inline rgb8_triplet_t
quantized_blended_rgb_for_rgba(const sampled_rgba_t &rgba,
                               const image_grayscale_options_t &opt) {
  const rgb8_triplet_t bg = background_rgb8(opt);
  const double src_r = srgb_to_linear(rgba.r);
  const double src_g = srgb_to_linear(rgba.g);
  const double src_b = srgb_to_linear(rgba.b);
  const double bg_r = srgb_to_linear(static_cast<double>(bg.r) / 255.0);
  const double bg_g = srgb_to_linear(static_cast<double>(bg.g) / 255.0);
  const double bg_b = srgb_to_linear(static_cast<double>(bg.b) / 255.0);

  const int blended_r = static_cast<int>(std::lround(
      linear_to_srgb(src_r * rgba.a + bg_r * (1.0 - rgba.a)) * 255.0));
  const int blended_g = static_cast<int>(std::lround(
      linear_to_srgb(src_g * rgba.a + bg_g * (1.0 - rgba.a)) * 255.0));
  const int blended_b = static_cast<int>(std::lround(
      linear_to_srgb(src_b * rgba.a + bg_b * (1.0 - rgba.a)) * 255.0));

  return rgb8_triplet_t{
      quantize_rgb8_channel(
          static_cast<std::uint8_t>(std::clamp(blended_r, 0, 255)),
          opt.color_levels),
      quantize_rgb8_channel(
          static_cast<std::uint8_t>(std::clamp(blended_g, 0, 255)),
          opt.color_levels),
      quantize_rgb8_channel(
          static_cast<std::uint8_t>(std::clamp(blended_b, 0, 255)),
          opt.color_levels)};
}

inline rgb8_triplet_t
quantized_rgb_for_opaque_rgba(const sampled_rgba_t &rgba,
                              const image_grayscale_options_t &opt) {
  const auto quantize_channel = [&](double channel) -> std::uint8_t {
    return quantize_rgb8_channel(
        static_cast<std::uint8_t>(std::clamp(
            std::lround(std::clamp(channel, 0.0, 1.0) * 255.0), 0l, 255l)),
        opt.color_levels);
  };
  return rgb8_triplet_t{quantize_channel(rgba.r), quantize_channel(rgba.g),
                        quantize_channel(rgba.b)};
}

struct color_mode_bucket_t {
  rgb8_triplet_t key{};
  double weight_sum{0.0};
  double linear_r_sum{0.0};
  double linear_g_sum{0.0};
  double linear_b_sum{0.0};
};

inline void accumulate_mode_bucket(std::vector<color_mode_bucket_t> &buckets,
                                   const sampled_rgba_t &rgba, double weight,
                                   const image_grayscale_options_t &opt) {
  if (weight <= 0.0)
    return;

  const rgb8_triplet_t key = quantized_rgb_for_opaque_rgba(rgba, opt);
  for (color_mode_bucket_t &bucket : buckets) {
    if (!same_rgb8(bucket.key, key))
      continue;
    bucket.weight_sum += weight;
    bucket.linear_r_sum += srgb_to_linear(rgba.r) * weight;
    bucket.linear_g_sum += srgb_to_linear(rgba.g) * weight;
    bucket.linear_b_sum += srgb_to_linear(rgba.b) * weight;
    return;
  }

  buckets.push_back(color_mode_bucket_t{
      key,
      weight,
      srgb_to_linear(rgba.r) * weight,
      srgb_to_linear(rgba.g) * weight,
      srgb_to_linear(rgba.b) * weight,
  });
}

inline bool
dominant_rgba_from_buckets(const std::vector<color_mode_bucket_t> &buckets,
                           sampled_rgba_t &out) {
  if (buckets.empty())
    return false;

  const color_mode_bucket_t *best_bucket = &buckets.front();
  for (const color_mode_bucket_t &bucket : buckets) {
    if (bucket.weight_sum > best_bucket->weight_sum)
      best_bucket = &bucket;
  }

  if (best_bucket->weight_sum <= 0.0)
    return false;

  out.r = linear_to_srgb(best_bucket->linear_r_sum / best_bucket->weight_sum);
  out.g = linear_to_srgb(best_bucket->linear_g_sum / best_bucket->weight_sum);
  out.b = linear_to_srgb(best_bucket->linear_b_sum / best_bucket->weight_sum);
  return true;
}

inline sampled_rgba_t
mode_rgba_for_region_samples(const rgba_image_t &image, const image_fit_t &fit,
                             double phys_x0, double phys_y0, double phys_w,
                             double phys_h, int sample_cols, int sample_rows,
                             const image_grayscale_options_t &opt) {
  sampled_rgba_t out{};
  double alpha_sum = 0.0;
  const int total_samples = std::max(0, sample_cols) * std::max(0, sample_rows);
  std::vector<color_mode_bucket_t> buckets{};
  buckets.reserve(static_cast<std::size_t>(std::max(1, total_samples)));

  for (int sy = 0; sy < sample_rows; ++sy) {
    for (int sx = 0; sx < sample_cols; ++sx) {
      const double phys_x =
          phys_x0 + (static_cast<double>(sx) + 0.5) *
                        (phys_w / static_cast<double>(sample_cols));
      const double phys_y =
          phys_y0 + (static_cast<double>(sy) + 0.5) *
                        (phys_h / static_cast<double>(sample_rows));
      double src_x = 0.0;
      double src_y = 0.0;
      if (!map_phys_to_image_coords(image, fit, phys_x, phys_y, src_x, src_y))
        continue;

      const sampled_rgba_t rgba = bilinear_sample_rgba(image, src_x, src_y);
      alpha_sum += rgba.a;
      accumulate_mode_bucket(buckets, rgba, std::max(0.0, rgba.a), opt);
    }
  }

  if (total_samples > 0) {
    out.a = alpha_sum / static_cast<double>(total_samples);
  }
  (void)dominant_rgba_from_buckets(buckets, out);
  return out;
}

inline sampled_rgba_t
mode_rgba_for_braille_cell(const braille_cell_samples_t &samples,
                           const image_grayscale_options_t &opt) {
  sampled_rgba_t out{};
  double alpha_sum = 0.0;
  std::vector<color_mode_bucket_t> buckets{};
  buckets.reserve(8);

  for (int sy = 0; sy < 4; ++sy) {
    for (int sx = 0; sx < 2; ++sx) {
      if (!samples.valid[sy][sx])
        continue;
      const sampled_rgba_t &rgba = samples.rgba[sy][sx];
      alpha_sum += rgba.a;
      accumulate_mode_bucket(buckets, rgba, std::max(0.0, rgba.a), opt);
    }
  }

  out.a = alpha_sum / 8.0;
  (void)dominant_rgba_from_buckets(buckets, out);
  return out;
}

inline sampled_rgba_t
select_rgba_for_braille_cell(const braille_cell_samples_t &samples,
                             const image_grayscale_options_t &opt) {
  if (opt.color_kernel == image_color_kernel_t::Average)
    return average_rgba_for_braille_cell(samples);
  return mode_rgba_for_braille_cell(samples, opt);
}

inline sampled_rgba_t select_rgba_for_half_block_half(
    const rgba_image_t &image, const image_fit_t &fit, int cell_x, int cell_y,
    int half_index, const image_grayscale_options_t &opt) {
  if (opt.color_kernel == image_color_kernel_t::Average) {
    return average_rgba_for_half_block_half(image, fit, cell_x, cell_y,
                                            half_index);
  }
  return mode_rgba_for_region_samples(image, fit, static_cast<double>(cell_x),
                                      static_cast<double>(cell_y) * 2.0 +
                                          static_cast<double>(half_index),
                                      1.0, 1.0, 4, 4, opt);
}

inline rgb8_triplet_t
quantized_visible_rgb_for_rgba(const sampled_rgba_t &rgba,
                               const image_grayscale_options_t &opt) {
  if (rgba.a <= opt.alpha_cutoff)
    return background_rgb8(opt);

  sampled_rgba_t visible = rgba;
  const double luma = rgba_luma(visible);
  const double max_channel = std::max({visible.r, visible.g, visible.b});
  const rgb8_triplet_t bg = background_rgb8(opt);
  const double bg_luma = rgb8_luma(bg);

  // Very dark colors become visually fragile once we snap them into a coarse
  // terminal palette, especially over a dark blue-black background. Nudge
  // near-black tones toward a neutral gray floor so shadows remain visible and
  // stop drifting into dark blue bins.
  if (luma < 0.16 && max_channel < 0.28) {
    const double lifted_gray =
        std::clamp(std::max({luma, bg_luma + 0.14, 0.18}), 0.0, 1.0);
    visible.r = lifted_gray;
    visible.g = lifted_gray;
    visible.b = lifted_gray;
  }

  return quantized_rgb_for_opaque_rgba(visible, opt);
}

inline short color_pair_for_rgb(const rgb8_triplet_t &rgb,
                                const image_grayscale_options_t &opt) {
  if (opt.color_pair_resolver) {
    return opt.color_pair_resolver(rgb.r, rgb.g, rgb.b);
  }

  const std::string fg_hex = rgb8_to_hex(rgb.r, rgb.g, rgb.b);
  return static_cast<short>(get_color_pair(fg_hex, opt.background_color));
}

inline short color_pair_for_fg_bg(const rgb8_triplet_t &fg,
                                  const rgb8_triplet_t &bg,
                                  const image_grayscale_options_t &opt) {
  if (same_rgb8(bg, background_rgb8(opt))) {
    return color_pair_for_rgb(fg, opt);
  }
  if (opt.fg_bg_color_pair_resolver) {
    return opt.fg_bg_color_pair_resolver(fg.r, fg.g, fg.b, bg.r, bg.g, bg.b);
  }

  const std::string fg_hex = rgb8_to_hex(fg.r, fg.g, fg.b);
  const std::string bg_hex = rgb8_to_hex(bg.r, bg.g, bg.b);
  return static_cast<short>(get_color_pair(fg_hex, bg_hex));
}

inline short color_pair_for_braille_cell(const braille_cell_samples_t &samples,
                                         unsigned char pattern,
                                         const image_grayscale_options_t &opt) {
  if (!opt.use_color)
    return opt.color_pair;

  const sampled_rgba_t cell_rgba = select_rgba_for_braille_cell(samples, opt);
  if (cell_rgba.a <= opt.alpha_cutoff)
    return opt.color_pair;

  if (pattern == 0)
    return opt.color_pair;

  // In transparent braille mode, let the dots carry the cell's visible color
  // while the cell background remains the terminal background. Coverage is
  // already represented by the glyph pattern, so avoid darkening the hue again
  // by blending it back into the terminal background.
  return color_pair_for_rgb(quantized_visible_rgb_for_rgba(cell_rgba, opt),
                            opt);
}

struct rendered_half_block_cell_t {
  wchar_t glyph{L' '};
  short color_pair{0};
};

inline rendered_half_block_cell_t
half_block_cell_for_image(const rgba_image_t &image, const image_fit_t &fit,
                          int cell_x, int cell_y,
                          const image_grayscale_options_t &opt) {
  const double top_ink =
      average_ink_for_half_block_half(image, fit, cell_x, cell_y, 0, opt);
  const double bottom_ink =
      average_ink_for_half_block_half(image, fit, cell_x, cell_y, 1, opt);
  const bool top_on = top_ink > opt.blank_threshold;
  const bool bottom_on = bottom_ink > opt.blank_threshold;

  if (!top_on && !bottom_on) {
    return {};
  }

  if (!opt.use_color) {
    if (top_on && bottom_on) {
      const double avg_ink = 0.5 * (top_ink + bottom_ink);
      const double split = std::abs(top_ink - bottom_ink);
      if (split <= 0.22) {
        return rendered_half_block_cell_t{
            shade_glyph_for_ink(avg_ink, opt.blank_threshold), opt.color_pair};
      }
      return rendered_half_block_cell_t{
          (top_ink >= bottom_ink) ? L'\u2580' : L'\u2584', opt.color_pair};
    }
    if (top_on)
      return rendered_half_block_cell_t{L'\u2580', opt.color_pair};
    return rendered_half_block_cell_t{L'\u2584', opt.color_pair};
  }

  const rgb8_triplet_t bg = background_rgb8(opt);
  const rgb8_triplet_t top_rgb =
      top_on ? quantized_visible_rgb_for_rgba(
                   select_rgba_for_half_block_half(image, fit, cell_x, cell_y,
                                                   0, opt),
                   opt)
             : bg;
  const rgb8_triplet_t bottom_rgb =
      bottom_on ? quantized_visible_rgb_for_rgba(
                      select_rgba_for_half_block_half(image, fit, cell_x,
                                                      cell_y, 1, opt),
                      opt)
                : bg;

  if (top_on && bottom_on) {
    if (same_rgb8(top_rgb, bottom_rgb)) {
      return rendered_half_block_cell_t{L'\u2588',
                                        color_pair_for_rgb(top_rgb, opt)};
    }
    return rendered_half_block_cell_t{
        L'\u2580', color_pair_for_fg_bg(top_rgb, bottom_rgb, opt)};
  }
  if (top_on) {
    return rendered_half_block_cell_t{L'\u2580',
                                      color_pair_for_fg_bg(top_rgb, bg, opt)};
  }
  return rendered_half_block_cell_t{L'\u2584',
                                    color_pair_for_fg_bg(bottom_rgb, bg, opt)};
}

} // namespace detail

inline void render_image_grayscale(const rgba_image_t &image, int start_x,
                                   int start_y, int width_cells,
                                   int height_cells,
                                   const image_grayscale_options_t &opt = {}) {
  if (!image.valid() || width_cells <= 0 || height_cells <= 0)
    return;
  auto *renderer = get_renderer();
  if (renderer == nullptr)
    return;

  const detail::image_fit_t fit = detail::fit_image_to_terminal_viewport(
      image, width_cells, height_cells, opt.preserve_aspect, opt.center);
  if (fit.w <= 0.0 || fit.h <= 0.0)
    return;

  if (opt.mode == image_grayscale_mode_t::HalfBlocks) {
    for (int row = 0; row < height_cells; ++row) {
      for (int col = 0; col < width_cells; ++col) {
        const detail::rendered_half_block_cell_t cell =
            detail::half_block_cell_for_image(image, fit, col, row, opt);
        renderer->putGlyph(start_y + row, start_x + col, cell.glyph,
                           cell.color_pair);
      }
    }
    return;
  }

  for (int row = 0; row < height_cells; ++row) {
    for (int col = 0; col < width_cells; ++col) {
      const detail::braille_cell_samples_t samples =
          detail::sample_braille_cell(image, fit, col, row, opt);
      const unsigned char pattern =
          detail::best_fit_braille_pattern(samples, opt);
      const wchar_t glyph =
          (pattern == 0) ? L' ' : static_cast<wchar_t>(0x2800 + pattern);
      const short color_pair =
          detail::color_pair_for_braille_cell(samples, pattern, opt);
      renderer->putBraille(start_y + row, start_x + col, glyph, color_pair);
    }
  }
}

namespace png_detail {

inline constexpr int kZOk = 0;

inline std::uint32_t read_be32(const std::vector<std::uint8_t> &bytes,
                               std::size_t offset) {
  return (static_cast<std::uint32_t>(bytes[offset + 0]) << 24u) |
         (static_cast<std::uint32_t>(bytes[offset + 1]) << 16u) |
         (static_cast<std::uint32_t>(bytes[offset + 2]) << 8u) |
         (static_cast<std::uint32_t>(bytes[offset + 3]) << 0u);
}

inline std::uint32_t read_be32_ptr(const std::uint8_t *bytes) {
  return (static_cast<std::uint32_t>(bytes[0]) << 24u) |
         (static_cast<std::uint32_t>(bytes[1]) << 16u) |
         (static_cast<std::uint32_t>(bytes[2]) << 8u) |
         (static_cast<std::uint32_t>(bytes[3]) << 0u);
}

inline int paeth_predictor(int a, int b, int c) {
  const int p = a + b - c;
  const int pa = std::abs(p - a);
  const int pb = std::abs(p - b);
  const int pc = std::abs(p - c);
  if (pa <= pb && pa <= pc)
    return a;
  if (pb <= pc)
    return b;
  return c;
}

inline bool read_file_bytes(const std::string &path,
                            std::vector<std::uint8_t> &out,
                            std::string &error) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    error = "unable to open file";
    return false;
  }
  out.assign(std::istreambuf_iterator<char>(in),
             std::istreambuf_iterator<char>());
  if (!in.good() && !in.eof()) {
    error = "failed while reading file";
    return false;
  }
  return true;
}

inline bool decode_png_rgba_bytes(const std::vector<std::uint8_t> &png_bytes,
                                  rgba_image_t &out, std::string &error) {
  out = {};
  error.clear();

  if (png_bytes.size() < 8u) {
    error = "png file too small";
    return false;
  }

  static constexpr std::array<std::uint8_t, 8> kPngSig = {
      0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
  if (!std::equal(kPngSig.begin(), kPngSig.end(), png_bytes.begin())) {
    error = "invalid png signature";
    return false;
  }

  int width = 0;
  int height = 0;
  int bit_depth = 0;
  int color_type = -1;
  int interlace = -1;
  std::vector<std::uint8_t> idat{};

  std::size_t offset = 8u;
  while (offset + 12u <= png_bytes.size()) {
    const std::uint32_t chunk_len = read_be32(png_bytes, offset);
    offset += 4u;
    if (offset + 4u + static_cast<std::size_t>(chunk_len) + 4u >
        png_bytes.size()) {
      error = "truncated png chunk";
      return false;
    }

    const std::string chunk_type(
        reinterpret_cast<const char *>(png_bytes.data() + offset), 4u);
    offset += 4u;
    const std::uint8_t *chunk_data = png_bytes.data() + offset;
    offset += static_cast<std::size_t>(chunk_len);
    offset += 4u;

    if (chunk_type == "IHDR") {
      if (chunk_len != 13u) {
        error = "invalid ihdr chunk size";
        return false;
      }
      width = static_cast<int>(read_be32_ptr(chunk_data + 0u));
      height = static_cast<int>(read_be32_ptr(chunk_data + 4u));
      bit_depth = static_cast<int>(chunk_data[8]);
      color_type = static_cast<int>(chunk_data[9]);
      interlace = static_cast<int>(chunk_data[12]);
    } else if (chunk_type == "IDAT") {
      idat.insert(idat.end(), chunk_data, chunk_data + chunk_len);
    } else if (chunk_type == "IEND") {
      break;
    }
  }

  if (width <= 0 || height <= 0) {
    error = "missing png dimensions";
    return false;
  }
  if (bit_depth != 8) {
    error = "only 8-bit png files are supported";
    return false;
  }
  if (interlace != 0) {
    error = "interlaced png files are not supported";
    return false;
  }
  if (idat.empty()) {
    error = "png has no idat payload";
    return false;
  }

  int channels = 0;
  switch (color_type) {
  case 0:
    channels = 1;
    break;
  case 2:
    channels = 3;
    break;
  case 4:
    channels = 2;
    break;
  case 6:
    channels = 4;
    break;
  default:
    error = "unsupported png color type";
    return false;
  }

  const std::size_t row_bytes =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(channels);
  const std::size_t inflated_size =
      static_cast<std::size_t>(height) * (1u + row_bytes);
  std::vector<std::uint8_t> inflated(inflated_size);
  uLongf inflated_len = static_cast<uLongf>(inflated.size());
  const int zrc = uncompress(inflated.data(), &inflated_len, idat.data(),
                             static_cast<uLong>(idat.size()));
  if (zrc != kZOk || inflated_len != inflated.size()) {
    error = "failed to inflate png idat stream";
    return false;
  }

  std::vector<std::uint8_t> recon(static_cast<std::size_t>(height) * row_bytes,
                                  0u);
  for (int row = 0; row < height; ++row) {
    const std::size_t src_row =
        static_cast<std::size_t>(row) * (1u + row_bytes);
    const std::uint8_t filter = inflated[src_row];
    const std::uint8_t *src = inflated.data() + src_row + 1u;
    std::uint8_t *dst =
        recon.data() + static_cast<std::size_t>(row) * row_bytes;

    for (std::size_t i = 0; i < row_bytes; ++i) {
      const int left =
          (i >= static_cast<std::size_t>(channels)) ? dst[i - channels] : 0;
      const int up =
          (row > 0)
              ? recon[(static_cast<std::size_t>(row) - 1u) * row_bytes + i]
              : 0;
      const int up_left =
          (row > 0 && i >= static_cast<std::size_t>(channels))
              ? recon[(static_cast<std::size_t>(row) - 1u) * row_bytes + i -
                      channels]
              : 0;

      int value = static_cast<int>(src[i]);
      switch (filter) {
      case 0:
        break;
      case 1:
        value += left;
        break;
      case 2:
        value += up;
        break;
      case 3:
        value += (left + up) / 2;
        break;
      case 4:
        value += paeth_predictor(left, up, up_left);
        break;
      default:
        error = "unsupported png filter type";
        return false;
      }
      dst[i] = static_cast<std::uint8_t>(value & 0xFF);
    }
  }

  out.width = width;
  out.height = height;
  out.pixels.resize(static_cast<std::size_t>(width) *
                    static_cast<std::size_t>(height) * 4u);

  for (int row = 0; row < height; ++row) {
    const std::uint8_t *src =
        recon.data() + static_cast<std::size_t>(row) * row_bytes;
    for (int col = 0; col < width; ++col) {
      const std::size_t src_idx =
          static_cast<std::size_t>(col) * static_cast<std::size_t>(channels);
      const std::size_t dst_idx =
          (static_cast<std::size_t>(row) * static_cast<std::size_t>(width) +
           static_cast<std::size_t>(col)) *
          4u;

      std::uint8_t r = 0;
      std::uint8_t g = 0;
      std::uint8_t b = 0;
      std::uint8_t a = 255;
      switch (color_type) {
      case 0:
        r = g = b = src[src_idx + 0];
        break;
      case 2:
        r = src[src_idx + 0];
        g = src[src_idx + 1];
        b = src[src_idx + 2];
        break;
      case 4:
        r = g = b = src[src_idx + 0];
        a = src[src_idx + 1];
        break;
      case 6:
        r = src[src_idx + 0];
        g = src[src_idx + 1];
        b = src[src_idx + 2];
        a = src[src_idx + 3];
        break;
      }

      out.pixels[dst_idx + 0] = r;
      out.pixels[dst_idx + 1] = g;
      out.pixels[dst_idx + 2] = b;
      out.pixels[dst_idx + 3] = a;
    }
  }

  return true;
}

} // namespace png_detail

inline bool decode_png_rgba_file(const std::string &path, rgba_image_t &out,
                                 std::string &error) {
  std::vector<std::uint8_t> png_bytes{};
  if (!png_detail::read_file_bytes(path, png_bytes, error))
    return false;
  return png_detail::decode_png_rgba_bytes(png_bytes, out, error);
}

} // namespace iinuji
} // namespace cuwacunu
