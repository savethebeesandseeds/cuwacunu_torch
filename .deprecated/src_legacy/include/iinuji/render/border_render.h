#pragma once

#include <cstddef>
#include <string>

#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/render/renderer.h"

namespace cuwacunu {
namespace iinuji {

inline void render_border(const iinuji_object_t& obj) {
  if (!obj.style.border) return;

  auto* R = get_renderer();
  if (!R) return;

  const Rect s = obj.screen;
  int x = s.x, y = s.y, w = s.w, h = s.h;
  if (w <= 0 || h <= 0) return;

  constexpr double kFocusDarken = 0.8; // 20% darker
  std::string fg = obj.style.border_color;
  std::string bg = obj.style.background_color;

  if (obj.focused && obj.focusable) {
    if (is_unset_color_token(fg)) fg = obj.style.label_color;
    fg = focus_darken_fg_token(fg, kFocusDarken);
    // If bg is terminal-default (<empty>), keep it (can't darken reliably).
    if (!is_unset_color_token(bg)) bg = darken_color_token(bg, kFocusDarken);
  }

  short pair = (short)get_color_pair(fg, bg);

  const wchar_t H = L'─', V = L'│', TL = L'┌', TR = L'┐', BL = L'└', BR = L'┘';
  if (w == 1 || h == 1) {
    // Degenerate: just paint something so focus doesn't crash tiny rects.
    R->fillRect(y, x, h, w, pair);
    return;
  }

  for (int c=1;c<w-1;++c){ R->putGlyph(y, x+c, H, pair); R->putGlyph(y+h-1, x+c, H, pair); }
  for (int r=1;r<h-1;++r){ R->putGlyph(y+r, x, V, pair); R->putGlyph(y+r, x+w-1, V, pair); }
  R->putGlyph(y, x, TL, pair); R->putGlyph(y, x+w-1, TR, pair);
  R->putGlyph(y+h-1, x, BL, pair); R->putGlyph(y+h-1, x+w-1, BR, pair);

  if (!obj.style.title.empty() && w>4) {
    int available = w - 4;
    std::string t = obj.style.title.substr(0, (size_t)available);
    R->putText(y, x+2, t, available, pair);
  }
}

} // namespace iinuji
} // namespace cuwacunu
