/* render/renderer.h */
#pragma once
#include <string>
#include <string_view>
#include <algorithm>
#include <cwchar>
#include <wchar.h>

namespace cuwacunu {
namespace iinuji {

inline std::size_t utf8_codepoint_bytes(std::string_view s,
                                        std::size_t offset) {
  if (offset >= s.size()) return 0;
  const unsigned char lead = static_cast<unsigned char>(s[offset]);
  std::size_t bytes = 1;
  if ((lead & 0x80u) == 0) {
    bytes = 1;
  } else if ((lead & 0xE0u) == 0xC0u) {
    bytes = 2;
  } else if ((lead & 0xF0u) == 0xE0u) {
    bytes = 3;
  } else if ((lead & 0xF8u) == 0xF0u) {
    bytes = 4;
  }
  if (offset + bytes > s.size()) return 1;
  for (std::size_t i = 1; i < bytes; ++i) {
    const unsigned char c = static_cast<unsigned char>(s[offset + i]);
    if ((c & 0xC0u) != 0x80u) return 1;
  }
  return bytes;
}

inline int utf8_codepoint_display_width(std::string_view bytes) {
  if (bytes.empty()) return 0;
  std::mbstate_t state{};
  wchar_t wc = 0;
  const std::size_t converted =
      std::mbrtowc(&wc, bytes.data(), bytes.size(), &state);
  if (converted == static_cast<std::size_t>(-1) ||
      converted == static_cast<std::size_t>(-2) || converted == 0) {
    return 1;
  }
  const int width = ::wcwidth(wc);
  return width < 0 ? 1 : width;
}

inline int utf8_display_width(std::string_view text) {
  int total = 0;
  for (std::size_t offset = 0; offset < text.size();) {
    const std::size_t bytes = utf8_codepoint_bytes(text, offset);
    if (bytes == 0) break;
    total += std::max(0, utf8_codepoint_display_width(text.substr(offset, bytes)));
    offset += bytes;
  }
  return total;
}

inline std::size_t utf8_prefix_bytes_for_width(std::string_view text,
                                               int max_width) {
  if (max_width < 0) return text.size();
  int used = 0;
  std::size_t offset = 0;
  while (offset < text.size()) {
    const std::size_t bytes = utf8_codepoint_bytes(text, offset);
    if (bytes == 0) break;
    const int glyph_width =
        std::max(0, utf8_codepoint_display_width(text.substr(offset, bytes)));
    if (used + glyph_width > max_width) break;
    used += glyph_width;
    offset += bytes;
  }
  return offset;
}

inline std::size_t utf8_byte_offset_for_width(std::string_view text,
                                              int skip_width) {
  if (skip_width <= 0) return 0;
  int used = 0;
  std::size_t offset = 0;
  while (offset < text.size()) {
    const std::size_t bytes = utf8_codepoint_bytes(text, offset);
    if (bytes == 0) break;
    const int glyph_width =
        std::max(0, utf8_codepoint_display_width(text.substr(offset, bytes)));
    if (used + glyph_width > skip_width) break;
    used += glyph_width;
    offset += bytes;
  }
  return offset;
}

struct IRend {
  virtual ~IRend() = default;

  // Terminal control
  virtual void size(int& h, int& w) = 0;
  virtual void clear() = 0;
  virtual void flush() = 0;

  // Drawing
  virtual void putText(int y, int x, const std::string& s, int max_w = -1,
                       short color_pair = 0, bool bold=false, bool inverse=false) = 0;
  virtual void putGlyph(int y, int x, wchar_t ch, short color_pair = 0) = 0;
  virtual void fillRect(int y, int x, int h, int w, short color_pair) = 0;

  // Convenience for braille
  virtual void putBraille(int y, int x, wchar_t ch, short color_pair = 0) {
    putGlyph(y, x, ch, color_pair);
  }
};

// Global active renderer (header-only)
inline IRend*& renderer_slot() {
  static IRend* g_rend = nullptr;
  return g_rend;
}
inline IRend* set_renderer(IRend* r) {
  IRend*& slot = renderer_slot();
  IRend* old = slot;
  slot = r;
  return old;
}
inline IRend* get_renderer() {
  return renderer_slot();
}

} // namespace iinuji
} // namespace cuwacunu
