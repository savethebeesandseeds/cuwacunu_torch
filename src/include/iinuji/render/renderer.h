/* render/renderer.h */
#pragma once
#include <string>
#include <cwchar>

namespace cuwacunu {
namespace iinuji {

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
