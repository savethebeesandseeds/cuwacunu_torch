/* primitives/animation.h */
#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "iinuji/primitives/image.h"

namespace cuwacunu {
namespace iinuji {

struct rgba_animation_t {
  int width{0};
  int height{0};
  int loop_count{0}; // 0 means infinite looping, matching GIF semantics.
  std::vector<rgba_image_t> frames{};
  std::vector<int> frame_delays_ms{};

  bool valid() const {
    if (width <= 0 || height <= 0)
      return false;
    if (frames.empty() || frames.size() != frame_delays_ms.size())
      return false;
    for (std::size_t i = 0; i < frames.size(); ++i) {
      const rgba_image_t &frame = frames[i];
      if (!frame.valid() || frame.width != width || frame.height != height)
        return false;
      if (frame_delays_ms[i] <= 0)
        return false;
    }
    return true;
  }

  std::uint64_t total_duration_ms() const {
    std::uint64_t total = 0;
    for (int delay_ms : frame_delays_ms) {
      total += static_cast<std::uint64_t>(std::max(1, delay_ms));
    }
    return total;
  }
};

namespace animation_detail {

inline std::size_t frame_index_for_elapsed_ms(const rgba_animation_t &animation,
                                              std::uint64_t elapsed_ms) {
  if (!animation.valid())
    return 0u;

  const std::uint64_t total_duration_ms = animation.total_duration_ms();
  if (total_duration_ms == 0u)
    return 0u;

  if (animation.loop_count > 0) {
    const std::uint64_t total_loop_span =
        total_duration_ms * static_cast<std::uint64_t>(animation.loop_count);
    if (elapsed_ms >= total_loop_span)
      return animation.frames.size() - 1u;
  }

  const std::uint64_t cycle_elapsed_ms = elapsed_ms % total_duration_ms;
  std::uint64_t accumulated_ms = 0u;
  for (std::size_t i = 0; i < animation.frame_delays_ms.size(); ++i) {
    accumulated_ms +=
        static_cast<std::uint64_t>(std::max(1, animation.frame_delays_ms[i]));
    if (cycle_elapsed_ms < accumulated_ms)
      return i;
  }
  return animation.frames.size() - 1u;
}

inline const rgba_image_t &
frame_for_elapsed_ms(const rgba_animation_t &animation,
                     std::uint64_t elapsed_ms) {
  return animation.frames[frame_index_for_elapsed_ms(animation, elapsed_ms)];
}

} // namespace animation_detail

inline void
render_animation_grayscale(const rgba_animation_t &animation,
                           std::uint64_t elapsed_ms, int start_x, int start_y,
                           int width_cells, int height_cells,
                           const image_grayscale_options_t &opt = {}) {
  if (!animation.valid() || width_cells <= 0 || height_cells <= 0)
    return;
  render_image_grayscale(
      animation_detail::frame_for_elapsed_ms(animation, elapsed_ms), start_x,
      start_y, width_cells, height_cells, opt);
}

namespace gif_detail {

struct graphics_control_t {
  int disposal_method{0};
  bool has_transparent_index{false};
  std::uint8_t transparent_index{0};
  int delay_ms{100};
};

inline std::uint16_t read_le16(const std::vector<std::uint8_t> &bytes,
                               std::size_t offset) {
  return static_cast<std::uint16_t>(bytes[offset + 0]) |
         (static_cast<std::uint16_t>(bytes[offset + 1]) << 8u);
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

inline int color_table_size_from_packed(std::uint8_t packed) {
  return 1 << ((packed & 0x07u) + 1u);
}

inline bool read_color_table(const std::vector<std::uint8_t> &bytes,
                             std::size_t &offset, int entry_count,
                             std::vector<std::uint8_t> &table,
                             std::string &error) {
  if (entry_count <= 0) {
    table.clear();
    return true;
  }
  const std::size_t table_bytes = static_cast<std::size_t>(entry_count) * 3u;
  if (offset + table_bytes > bytes.size()) {
    error = "truncated gif color table";
    return false;
  }
  table.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
               bytes.begin() +
                   static_cast<std::ptrdiff_t>(offset + table_bytes));
  offset += table_bytes;
  return true;
}

inline bool read_sub_blocks(const std::vector<std::uint8_t> &bytes,
                            std::size_t &offset, std::vector<std::uint8_t> &out,
                            std::string &error) {
  out.clear();
  while (true) {
    if (offset >= bytes.size()) {
      error = "truncated gif sub-block stream";
      return false;
    }
    const std::size_t block_size = bytes[offset++];
    if (block_size == 0u)
      return true;
    if (offset + block_size > bytes.size()) {
      error = "truncated gif sub-block payload";
      return false;
    }
    out.insert(out.end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset),
               bytes.begin() +
                   static_cast<std::ptrdiff_t>(offset + block_size));
    offset += block_size;
  }
}

inline bool skip_sub_blocks(const std::vector<std::uint8_t> &bytes,
                            std::size_t &offset, std::string &error) {
  std::vector<std::uint8_t> scratch{};
  return read_sub_blocks(bytes, offset, scratch, error);
}

inline bool lzw_decode_indices(const std::vector<std::uint8_t> &compressed,
                               int min_code_size, std::size_t expected_count,
                               std::vector<std::uint8_t> &out,
                               std::string &error) {
  out.clear();
  if (min_code_size < 2 || min_code_size > 8) {
    error = "unsupported gif lzw minimum code size";
    return false;
  }

  std::array<std::uint16_t, 4096> prefix{};
  std::array<std::uint8_t, 4096> suffix{};
  std::array<std::uint8_t, 4096> stack{};

  const int clear_code = 1 << min_code_size;
  const int end_code = clear_code + 1;
  for (int i = 0; i < clear_code; ++i) {
    suffix[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(i);
  }

  int code_size = min_code_size + 1;
  int code_mask = (1 << code_size) - 1;
  int available = clear_code + 2;
  int previous_code = -1;
  std::uint8_t first_value = 0;
  std::size_t bit_offset = 0u;

  const auto reset_dictionary = [&]() {
    code_size = min_code_size + 1;
    code_mask = (1 << code_size) - 1;
    available = clear_code + 2;
    previous_code = -1;
  };

  const auto read_code = [&](int &code) -> bool {
    if (bit_offset + static_cast<std::size_t>(code_size) >
        compressed.size() * 8u) {
      return false;
    }
    unsigned int raw = 0u;
    for (int bit = 0; bit < code_size; ++bit) {
      const std::size_t absolute_bit =
          bit_offset + static_cast<std::size_t>(bit);
      const std::size_t byte_index = absolute_bit >> 3u;
      const int bit_index = static_cast<int>(absolute_bit & 0x07u);
      raw |= static_cast<unsigned int>(
          ((compressed[byte_index] >> bit_index) & 0x01u) << bit);
    }
    bit_offset += static_cast<std::size_t>(code_size);
    code = static_cast<int>(raw) & code_mask;
    return true;
  };

  reset_dictionary();
  int code = 0;
  while (read_code(code)) {
    if (code == clear_code) {
      reset_dictionary();
      continue;
    }
    if (code == end_code)
      break;

    int current_code = code;
    int stack_size = 0;
    if (code >= available) {
      if (previous_code < 0) {
        error = "invalid gif lzw stream";
        return false;
      }
      stack[static_cast<std::size_t>(stack_size++)] = first_value;
      code = previous_code;
    }

    while (code >= clear_code) {
      if (code < 0 || code >= available || stack_size >= 4096) {
        error = "corrupt gif lzw dictionary";
        return false;
      }
      stack[static_cast<std::size_t>(stack_size++)] =
          suffix[static_cast<std::size_t>(code)];
      code = prefix[static_cast<std::size_t>(code)];
    }

    if (code < 0 || code >= clear_code) {
      error = "corrupt gif lzw base code";
      return false;
    }

    first_value = suffix[static_cast<std::size_t>(code)];
    stack[static_cast<std::size_t>(stack_size++)] = first_value;

    while (stack_size > 0) {
      out.push_back(stack[static_cast<std::size_t>(--stack_size)]);
      if (out.size() >= expected_count)
        break;
    }

    if (previous_code >= 0 && available < 4096) {
      prefix[static_cast<std::size_t>(available)] =
          static_cast<std::uint16_t>(previous_code);
      suffix[static_cast<std::size_t>(available)] = first_value;
      ++available;
      if (available >= (1 << code_size) && code_size < 12) {
        ++code_size;
        code_mask = (1 << code_size) - 1;
      }
    }

    previous_code = current_code;
    if (out.size() >= expected_count)
      break;
  }

  if (out.size() < expected_count) {
    error = "gif frame decoded fewer pixels than expected";
    return false;
  }
  out.resize(expected_count);
  return true;
}

inline void reorder_interlaced_rows(const std::vector<std::uint8_t> &src,
                                    int width, int height, bool interlaced,
                                    std::vector<std::uint8_t> &dst) {
  dst.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height),
             0u);
  if (!interlaced) {
    std::copy(src.begin(), src.end(), dst.begin());
    return;
  }

  static constexpr std::array<int, 4> kPassStarts{{0, 4, 2, 1}};
  static constexpr std::array<int, 4> kPassSteps{{8, 8, 4, 2}};
  std::size_t src_offset = 0u;
  for (std::size_t pass = 0; pass < kPassStarts.size(); ++pass) {
    for (int row = kPassStarts[pass]; row < height; row += kPassSteps[pass]) {
      const std::size_t row_bytes = static_cast<std::size_t>(width);
      std::copy(src.begin() + static_cast<std::ptrdiff_t>(src_offset),
                src.begin() +
                    static_cast<std::ptrdiff_t>(src_offset + row_bytes),
                dst.begin() + static_cast<std::ptrdiff_t>(
                                  static_cast<std::size_t>(row) * row_bytes));
      src_offset += row_bytes;
    }
  }
}

inline void clear_canvas_region(std::vector<std::uint8_t> &canvas,
                                int canvas_width, int canvas_height, int left,
                                int top, int width, int height) {
  for (int row = 0; row < height; ++row) {
    const int dst_y = top + row;
    if (dst_y < 0 || dst_y >= canvas_height)
      continue;
    for (int col = 0; col < width; ++col) {
      const int dst_x = left + col;
      if (dst_x < 0 || dst_x >= canvas_width)
        continue;
      const std::size_t dst_idx = (static_cast<std::size_t>(dst_y) *
                                       static_cast<std::size_t>(canvas_width) +
                                   static_cast<std::size_t>(dst_x)) *
                                  4u;
      canvas[dst_idx + 0] = 0;
      canvas[dst_idx + 1] = 0;
      canvas[dst_idx + 2] = 0;
      canvas[dst_idx + 3] = 0;
    }
  }
}

} // namespace gif_detail

inline bool decode_gif_rgba_file(const std::string &path, rgba_animation_t &out,
                                 std::string &error) {
  using namespace gif_detail;

  out = {};
  error.clear();

  std::vector<std::uint8_t> bytes{};
  if (!read_file_bytes(path, bytes, error))
    return false;
  if (bytes.size() < 13u) {
    error = "gif file too small";
    return false;
  }
  const std::string signature(reinterpret_cast<const char *>(bytes.data()), 6u);
  if (signature != "GIF87a" && signature != "GIF89a") {
    error = "invalid gif signature";
    return false;
  }

  const int canvas_width = static_cast<int>(read_le16(bytes, 6u));
  const int canvas_height = static_cast<int>(read_le16(bytes, 8u));
  if (canvas_width <= 0 || canvas_height <= 0) {
    error = "gif has invalid canvas size";
    return false;
  }

  out.width = canvas_width;
  out.height = canvas_height;

  const std::uint8_t logical_packed = bytes[10u];
  std::size_t offset = 13u;
  std::vector<std::uint8_t> global_color_table{};
  if ((logical_packed & 0x80u) != 0u) {
    if (!read_color_table(bytes, offset,
                          color_table_size_from_packed(logical_packed),
                          global_color_table, error)) {
      return false;
    }
  }

  std::vector<std::uint8_t> canvas(static_cast<std::size_t>(canvas_width) *
                                       static_cast<std::size_t>(canvas_height) *
                                       4u,
                                   0u);
  graphics_control_t gce{};
  bool saw_netscape_loop = false;

  while (offset < bytes.size()) {
    const std::uint8_t introducer = bytes[offset++];
    if (introducer == 0x3Bu) {
      break;
    }
    if (introducer == 0x21u) {
      if (offset >= bytes.size()) {
        error = "truncated gif extension";
        return false;
      }
      const std::uint8_t label = bytes[offset++];
      if (label == 0xF9u) {
        if (offset + 6u > bytes.size()) {
          error = "truncated graphic control extension";
          return false;
        }
        const std::uint8_t block_size = bytes[offset++];
        if (block_size != 4u) {
          error = "invalid graphic control extension size";
          return false;
        }
        const std::uint8_t packed = bytes[offset++];
        const int delay_cs = static_cast<int>(read_le16(bytes, offset));
        offset += 2u;
        const std::uint8_t transparent_index = bytes[offset++];
        const std::uint8_t terminator = bytes[offset++];
        if (terminator != 0u) {
          error = "unterminated graphic control extension";
          return false;
        }
        gce.disposal_method = (packed >> 2u) & 0x07u;
        gce.has_transparent_index = (packed & 0x01u) != 0u;
        gce.transparent_index = transparent_index;
        gce.delay_ms = (delay_cs > 0) ? (delay_cs * 10) : 100;
        continue;
      }
      if (label == 0xFFu) {
        if (offset >= bytes.size()) {
          error = "truncated application extension";
          return false;
        }
        const std::size_t block_size = bytes[offset++];
        if (offset + block_size > bytes.size()) {
          error = "truncated application identifier";
          return false;
        }
        const std::string app_id(
            reinterpret_cast<const char *>(bytes.data() + offset), block_size);
        offset += block_size;
        std::vector<std::uint8_t> app_payload{};
        if (!read_sub_blocks(bytes, offset, app_payload, error))
          return false;
        if ((app_id == "NETSCAPE2.0" || app_id == "ANIMEXTS1.0") &&
            app_payload.size() >= 3u && app_payload[0] == 0x01u) {
          out.loop_count = static_cast<int>(app_payload[1]) |
                           (static_cast<int>(app_payload[2]) << 8u);
          saw_netscape_loop = true;
        }
        continue;
      }
      if (!skip_sub_blocks(bytes, offset, error))
        return false;
      continue;
    }
    if (introducer != 0x2Cu) {
      error = "unsupported gif block type";
      return false;
    }

    if (offset + 9u > bytes.size()) {
      error = "truncated gif image descriptor";
      return false;
    }

    const int image_left = static_cast<int>(read_le16(bytes, offset));
    offset += 2u;
    const int image_top = static_cast<int>(read_le16(bytes, offset));
    offset += 2u;
    const int image_width = static_cast<int>(read_le16(bytes, offset));
    offset += 2u;
    const int image_height = static_cast<int>(read_le16(bytes, offset));
    offset += 2u;
    const std::uint8_t image_packed = bytes[offset++];
    if (image_width <= 0 || image_height <= 0) {
      error = "gif frame has invalid size";
      return false;
    }

    std::vector<std::uint8_t> local_color_table{};
    if ((image_packed & 0x80u) != 0u) {
      if (!read_color_table(bytes, offset,
                            color_table_size_from_packed(image_packed),
                            local_color_table, error)) {
        return false;
      }
    }
    const std::vector<std::uint8_t> &palette =
        local_color_table.empty() ? global_color_table : local_color_table;
    if (palette.empty()) {
      error = "gif frame is missing a color table";
      return false;
    }

    if (offset >= bytes.size()) {
      error = "truncated gif image data";
      return false;
    }
    const int lzw_min_code_size = static_cast<int>(bytes[offset++]);
    std::vector<std::uint8_t> compressed{};
    if (!read_sub_blocks(bytes, offset, compressed, error))
      return false;

    std::vector<std::uint8_t> decoded_indices{};
    if (!lzw_decode_indices(compressed, lzw_min_code_size,
                            static_cast<std::size_t>(image_width) *
                                static_cast<std::size_t>(image_height),
                            decoded_indices, error)) {
      return false;
    }

    std::vector<std::uint8_t> image_indices{};
    reorder_interlaced_rows(decoded_indices, image_width, image_height,
                            (image_packed & 0x40u) != 0u, image_indices);

    std::vector<std::uint8_t> previous_canvas{};
    if (gce.disposal_method == 3) {
      previous_canvas = canvas;
    }

    for (int row = 0; row < image_height; ++row) {
      const int dst_y = image_top + row;
      if (dst_y < 0 || dst_y >= canvas_height)
        continue;
      for (int col = 0; col < image_width; ++col) {
        const int dst_x = image_left + col;
        if (dst_x < 0 || dst_x >= canvas_width)
          continue;
        const std::size_t src_idx = static_cast<std::size_t>(row) *
                                        static_cast<std::size_t>(image_width) +
                                    static_cast<std::size_t>(col);
        const std::uint8_t color_index = image_indices[src_idx];
        if (gce.has_transparent_index && color_index == gce.transparent_index) {
          continue;
        }
        const std::size_t palette_idx =
            static_cast<std::size_t>(color_index) * 3u;
        if (palette_idx + 2u >= palette.size())
          continue;
        const std::size_t dst_idx =
            (static_cast<std::size_t>(dst_y) *
                 static_cast<std::size_t>(canvas_width) +
             static_cast<std::size_t>(dst_x)) *
            4u;
        canvas[dst_idx + 0] = palette[palette_idx + 0];
        canvas[dst_idx + 1] = palette[palette_idx + 1];
        canvas[dst_idx + 2] = palette[palette_idx + 2];
        canvas[dst_idx + 3] = 255;
      }
    }

    rgba_image_t frame{};
    frame.width = canvas_width;
    frame.height = canvas_height;
    frame.pixels = canvas;
    out.frames.push_back(std::move(frame));
    out.frame_delays_ms.push_back(std::max(1, gce.delay_ms));

    if (gce.disposal_method == 2) {
      clear_canvas_region(canvas, canvas_width, canvas_height, image_left,
                          image_top, image_width, image_height);
    } else if (gce.disposal_method == 3 &&
               previous_canvas.size() == canvas.size()) {
      canvas.swap(previous_canvas);
    }

    gce = graphics_control_t{};
  }

  if (out.frames.empty()) {
    error = "gif contains no image frames";
    return false;
  }
  if (!saw_netscape_loop) {
    out.loop_count = 1;
  }
  return true;
}

namespace apng_detail {

struct shared_chunk_t {
  std::string type{};
  std::vector<std::uint8_t> data{};
};

struct frame_control_t {
  std::uint32_t width{0};
  std::uint32_t height{0};
  std::uint32_t x_offset{0};
  std::uint32_t y_offset{0};
  std::uint16_t delay_num{1};
  std::uint16_t delay_den{10};
  std::uint8_t dispose_op{0};
  std::uint8_t blend_op{0};
  bool valid{false};
};

inline std::uint16_t read_be16(const std::vector<std::uint8_t> &bytes,
                               std::size_t offset) {
  return (static_cast<std::uint16_t>(bytes[offset + 0]) << 8u) |
         (static_cast<std::uint16_t>(bytes[offset + 1]) << 0u);
}

inline void append_be32(std::vector<std::uint8_t> &out, std::uint32_t value) {
  out.push_back(static_cast<std::uint8_t>((value >> 24u) & 0xFFu));
  out.push_back(static_cast<std::uint8_t>((value >> 16u) & 0xFFu));
  out.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xFFu));
  out.push_back(static_cast<std::uint8_t>((value >> 0u) & 0xFFu));
}

inline void append_chunk(std::vector<std::uint8_t> &out,
                         const std::string &type,
                         const std::vector<std::uint8_t> &data) {
  append_be32(out, static_cast<std::uint32_t>(data.size()));
  out.insert(out.end(), type.begin(), type.end());
  out.insert(out.end(), data.begin(), data.end());
  append_be32(out, 0u); // CRC is ignored by our PNG decoder.
}

inline int delay_ms_for_frame(const frame_control_t &control) {
  const std::uint16_t den =
      (control.delay_den == 0u) ? 100u : control.delay_den;
  const double delay_ms = 1000.0 * static_cast<double>(control.delay_num) /
                          static_cast<double>(den);
  return std::max(1, static_cast<int>(std::lround(delay_ms)));
}

inline bool parse_frame_control(const std::vector<std::uint8_t> &chunk_data,
                                frame_control_t &out, std::string &error) {
  if (chunk_data.size() != 26u) {
    error = "invalid apng fcTL chunk size";
    return false;
  }

  out.width = png_detail::read_be32(chunk_data, 4u);
  out.height = png_detail::read_be32(chunk_data, 8u);
  out.x_offset = png_detail::read_be32(chunk_data, 12u);
  out.y_offset = png_detail::read_be32(chunk_data, 16u);
  out.delay_num = read_be16(chunk_data, 20u);
  out.delay_den = read_be16(chunk_data, 22u);
  out.dispose_op = chunk_data[24u];
  out.blend_op = chunk_data[25u];
  out.valid = true;

  if (out.width == 0u || out.height == 0u) {
    error = "apng frame has invalid size";
    return false;
  }
  if (out.dispose_op > 2u) {
    error = "unsupported apng dispose operation";
    return false;
  }
  if (out.blend_op > 1u) {
    error = "unsupported apng blend operation";
    return false;
  }
  return true;
}

inline bool build_frame_png(const std::vector<std::uint8_t> &ihdr_template,
                            const std::vector<shared_chunk_t> &shared_chunks,
                            const frame_control_t &control,
                            const std::vector<std::uint8_t> &compressed_data,
                            std::vector<std::uint8_t> &png_bytes,
                            std::string &error) {
  if (ihdr_template.size() != 13u) {
    error = "apng is missing a valid IHDR template";
    return false;
  }

  std::vector<std::uint8_t> ihdr = ihdr_template;
  ihdr[0] = static_cast<std::uint8_t>((control.width >> 24u) & 0xFFu);
  ihdr[1] = static_cast<std::uint8_t>((control.width >> 16u) & 0xFFu);
  ihdr[2] = static_cast<std::uint8_t>((control.width >> 8u) & 0xFFu);
  ihdr[3] = static_cast<std::uint8_t>((control.width >> 0u) & 0xFFu);
  ihdr[4] = static_cast<std::uint8_t>((control.height >> 24u) & 0xFFu);
  ihdr[5] = static_cast<std::uint8_t>((control.height >> 16u) & 0xFFu);
  ihdr[6] = static_cast<std::uint8_t>((control.height >> 8u) & 0xFFu);
  ihdr[7] = static_cast<std::uint8_t>((control.height >> 0u) & 0xFFu);

  static constexpr std::array<std::uint8_t, 8> kPngSig = {
      0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
  png_bytes.assign(kPngSig.begin(), kPngSig.end());
  append_chunk(png_bytes, "IHDR", ihdr);
  for (const shared_chunk_t &chunk : shared_chunks) {
    append_chunk(png_bytes, chunk.type, chunk.data);
  }
  append_chunk(png_bytes, "IDAT", compressed_data);
  append_chunk(png_bytes, "IEND", {});
  return true;
}

inline void clear_region(std::vector<std::uint8_t> &canvas, int canvas_width,
                         int canvas_height, const frame_control_t &control) {
  gif_detail::clear_canvas_region(
      canvas, canvas_width, canvas_height, static_cast<int>(control.x_offset),
      static_cast<int>(control.y_offset), static_cast<int>(control.width),
      static_cast<int>(control.height));
}

inline void alpha_blend_over(std::uint8_t *dst, const std::uint8_t *src) {
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

inline bool compose_frame_on_canvas(std::vector<std::uint8_t> &canvas,
                                    int canvas_width, int canvas_height,
                                    const rgba_image_t &frame_image,
                                    const frame_control_t &control,
                                    std::string &error) {
  if (!frame_image.valid() ||
      frame_image.width != static_cast<int>(control.width) ||
      frame_image.height != static_cast<int>(control.height)) {
    error = "decoded apng subframe size mismatch";
    return false;
  }
  if (control.x_offset + control.width >
          static_cast<std::uint32_t>(canvas_width) ||
      control.y_offset + control.height >
          static_cast<std::uint32_t>(canvas_height)) {
    error = "apng frame exceeds animation canvas";
    return false;
  }

  for (int row = 0; row < frame_image.height; ++row) {
    for (int col = 0; col < frame_image.width; ++col) {
      const std::size_t src_idx =
          (static_cast<std::size_t>(row) *
               static_cast<std::size_t>(frame_image.width) +
           static_cast<std::size_t>(col)) *
          4u;
      const int dst_x = static_cast<int>(control.x_offset) + col;
      const int dst_y = static_cast<int>(control.y_offset) + row;
      const std::size_t dst_idx = (static_cast<std::size_t>(dst_y) *
                                       static_cast<std::size_t>(canvas_width) +
                                   static_cast<std::size_t>(dst_x)) *
                                  4u;

      const std::uint8_t *src_px = frame_image.pixels.data() + src_idx;
      std::uint8_t *dst_px = canvas.data() + dst_idx;
      if (control.blend_op == 0u) {
        dst_px[0] = src_px[0];
        dst_px[1] = src_px[1];
        dst_px[2] = src_px[2];
        dst_px[3] = src_px[3];
      } else {
        alpha_blend_over(dst_px, src_px);
      }
    }
  }
  return true;
}

inline void wrap_static_image_as_animation(const rgba_image_t &image,
                                           rgba_animation_t &out) {
  out = {};
  out.width = image.width;
  out.height = image.height;
  out.loop_count = 1;
  out.frames.push_back(image);
  out.frame_delays_ms.push_back(100);
}

inline bool ends_with_gif_extension(const std::string &path) {
  static constexpr char kSuffix[] = ".gif";
  if (path.size() < sizeof(kSuffix) - 1u)
    return false;
  const std::size_t base = path.size() - (sizeof(kSuffix) - 1u);
  for (std::size_t i = 0; i < sizeof(kSuffix) - 1u; ++i) {
    const unsigned char lhs = static_cast<unsigned char>(path[base + i]);
    const unsigned char rhs = static_cast<unsigned char>(kSuffix[i]);
    if (std::tolower(lhs) != std::tolower(rhs))
      return false;
  }
  return true;
}

} // namespace apng_detail

inline bool decode_apng_rgba_file(const std::string &path,
                                  rgba_animation_t &out, std::string &error) {
  using namespace apng_detail;

  out = {};
  error.clear();

  std::vector<std::uint8_t> bytes{};
  if (!png_detail::read_file_bytes(path, bytes, error))
    return false;
  if (bytes.size() < 8u) {
    error = "png file too small";
    return false;
  }

  static constexpr std::array<std::uint8_t, 8> kPngSig = {
      0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
  if (!std::equal(kPngSig.begin(), kPngSig.end(), bytes.begin())) {
    error = "invalid png signature";
    return false;
  }

  std::vector<std::uint8_t> ihdr_template{};
  std::vector<shared_chunk_t> shared_chunks{};
  bool saw_actl = false;
  bool locked_shared_chunks = false;
  bool current_frame_active = false;
  frame_control_t current_control{};
  std::vector<std::uint8_t> current_frame_data{};

  std::vector<std::uint8_t> canvas{};

  const auto finalize_frame = [&]() -> bool {
    if (!current_frame_active)
      return true;
    if (current_frame_data.empty()) {
      error = "apng frame is missing image data";
      return false;
    }

    std::vector<std::uint8_t> frame_png{};
    if (!build_frame_png(ihdr_template, shared_chunks, current_control,
                         current_frame_data, frame_png, error)) {
      return false;
    }

    rgba_image_t frame_region{};
    if (!png_detail::decode_png_rgba_bytes(frame_png, frame_region, error)) {
      error = std::string("failed to decode apng frame: ") + error;
      return false;
    }

    std::vector<std::uint8_t> previous_canvas{};
    if (current_control.dispose_op == 2u) {
      previous_canvas = canvas;
    }

    if (!compose_frame_on_canvas(canvas, out.width, out.height, frame_region,
                                 current_control, error)) {
      return false;
    }

    rgba_image_t full_frame{};
    full_frame.width = out.width;
    full_frame.height = out.height;
    full_frame.pixels = canvas;
    out.frames.push_back(std::move(full_frame));
    out.frame_delays_ms.push_back(delay_ms_for_frame(current_control));

    if (current_control.dispose_op == 1u) {
      clear_region(canvas, out.width, out.height, current_control);
    } else if (current_control.dispose_op == 2u &&
               previous_canvas.size() == canvas.size()) {
      canvas.swap(previous_canvas);
    }

    current_frame_active = false;
    current_control = frame_control_t{};
    current_frame_data.clear();
    return true;
  };

  std::size_t offset = 8u;
  while (offset + 12u <= bytes.size()) {
    const std::uint32_t chunk_len = png_detail::read_be32(bytes, offset);
    offset += 4u;
    if (offset + 4u + static_cast<std::size_t>(chunk_len) + 4u > bytes.size()) {
      error = "truncated png chunk";
      return false;
    }

    const std::string chunk_type(
        reinterpret_cast<const char *>(bytes.data() + offset), 4u);
    offset += 4u;
    const std::size_t data_offset = offset;
    offset += static_cast<std::size_t>(chunk_len);
    offset += 4u; // skip CRC

    const std::vector<std::uint8_t> chunk_data(
        bytes.begin() + static_cast<std::ptrdiff_t>(data_offset),
        bytes.begin() + static_cast<std::ptrdiff_t>(data_offset + chunk_len));

    if (chunk_type == "IHDR") {
      if (chunk_data.size() != 13u) {
        error = "invalid png ihdr chunk size";
        return false;
      }
      ihdr_template = chunk_data;
      out.width = static_cast<int>(png_detail::read_be32(chunk_data, 0u));
      out.height = static_cast<int>(png_detail::read_be32(chunk_data, 4u));
      if (out.width <= 0 || out.height <= 0) {
        error = "png has invalid dimensions";
        return false;
      }
      canvas.assign(static_cast<std::size_t>(out.width) *
                        static_cast<std::size_t>(out.height) * 4u,
                    0u);
      continue;
    }
    if (chunk_type == "acTL") {
      if (chunk_data.size() != 8u) {
        error = "invalid apng acTL chunk size";
        return false;
      }
      saw_actl = true;
      out.loop_count = static_cast<int>(png_detail::read_be32(chunk_data, 4u));
      continue;
    }

    if (chunk_type == "fcTL") {
      if (current_frame_active && !finalize_frame())
        return false;
      if (!parse_frame_control(chunk_data, current_control, error))
        return false;
      current_frame_active = true;
      current_frame_data.clear();
      locked_shared_chunks = true;
      continue;
    }

    if (chunk_type == "IDAT") {
      if (saw_actl) {
        if (!current_frame_active) {
          error = "apng IDAT encountered without an active frame control";
          return false;
        }
        current_frame_data.insert(current_frame_data.end(), chunk_data.begin(),
                                  chunk_data.end());
        locked_shared_chunks = true;
      }
      continue;
    }

    if (chunk_type == "fdAT") {
      if (!current_frame_active) {
        error = "apng fdAT encountered without an active frame control";
        return false;
      }
      if (chunk_data.size() < 4u) {
        error = "truncated apng fdAT chunk";
        return false;
      }
      current_frame_data.insert(current_frame_data.end(),
                                chunk_data.begin() + 4, chunk_data.end());
      locked_shared_chunks = true;
      continue;
    }

    if (chunk_type == "IEND") {
      break;
    }

    if (!locked_shared_chunks && chunk_type != "acTL") {
      shared_chunks.push_back(shared_chunk_t{chunk_type, chunk_data});
    }
  }

  if (!saw_actl) {
    rgba_image_t image{};
    if (!png_detail::decode_png_rgba_bytes(bytes, image, error))
      return false;
    wrap_static_image_as_animation(image, out);
    return true;
  }

  if (current_frame_active && !finalize_frame())
    return false;

  if (out.frames.empty()) {
    error = "apng contains no animation frames";
    return false;
  }
  return true;
}

inline bool decode_animation_rgba_file(const std::string &path,
                                       rgba_animation_t &out,
                                       std::string &error) {
  if (apng_detail::ends_with_gif_extension(path))
    return decode_gif_rgba_file(path, out, error);
  return decode_apng_rgba_file(path, out, error);
}

} // namespace iinuji
} // namespace cuwacunu
