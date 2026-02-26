#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "iinuji/iinuji_ansi.h"
#include "iinuji/render/layout_core.h"

namespace cuwacunu {
namespace iinuji {

inline void render_buffer(const iinuji_object_t& obj) {
  Rect r = content_rect(obj);
  auto* R = get_renderer();
  if (!R) return;

  // Base/background fill uses the widget default colors.
  short base_pair = (short)get_color_pair(obj.style.label_color, obj.style.background_color);
  R->fillRect(r.y, r.x, r.h, r.w, base_pair);

  auto bb = std::dynamic_pointer_cast<bufferBox_data_t>(obj.data);
  if (!bb) return;

  const int H = std::max(0, r.h);
  const int W = std::max(0, r.w);
  if (H <= 0 || W <= 0) return;

  const int n = (int)bb->lines.size();
  if (n == 0) return;

  // feed width hint back into the model
  bb->wrap_width_last = W;

  // Color selection:
  //   - if L.color present => use it (EVENT __color)
  //   - else => obj.style.label_color (FIGURE __text_color)
  auto line_pair_for = [&](const buffer_line_t& L) -> short {
    const std::string& fg = (!L.color.empty()) ? L.color : obj.style.label_color;
    short cp = (short)get_color_pair(fg, obj.style.background_color);
    return (cp > 0 ? cp : base_pair);
  };

  struct vis_row_t { std::vector<ansi::row_t> rows; }; // each entry is exactly one visible row (rows.size()==1)
  std::vector<vis_row_t> vis;
  vis.reserve((size_t)n * 2);

  auto push_wrapped = [&](const buffer_line_t& L){
    std::string prefix;
    if (!L.label.empty()) prefix = "[" + L.label + "] ";
    const int prefix_len = (int)prefix.size();
    const int avail = std::max(1, W - prefix_len);

    short base_pair_line = line_pair_for(L);

    // ANSI base style for this buffer line:
    ansi::style_t base;
    base.fg = (!L.color.empty()) ? L.color : obj.style.label_color;
    base.bg = obj.style.background_color;
    base.bold = obj.style.bold;
    base.inverse = obj.style.inverse;
    base.dim = false;

    // Wrap the payload with ANSI awareness (hard wrap)
    std::vector<ansi::row_t> payload_rows;
    ansi::hard_wrap(L.text, avail, base, base_pair_line, payload_rows);
    if (payload_rows.empty()) payload_rows.push_back(ansi::row_t{});

    for (size_t i=0;i<payload_rows.size();++i) {
      ansi::row_t full;

      // prefix / indentation uses the line base color pair
      if (i == 0) {
        if (!prefix.empty()) ansi::append_plain(full, prefix, base_pair_line, obj.style.bold, obj.style.inverse);
      } else {
        if (prefix_len > 0) ansi::append_plain(full, std::string((size_t)prefix_len, ' '), base_pair_line, obj.style.bold, obj.style.inverse);
      }

      // append payload segments
      for (const auto& seg : payload_rows[i].segs) {
        ansi::row_t tmp;
        tmp.segs.push_back(seg);
        tmp.len = (int)seg.text.size();
        // merge: render_row() is segment-based so we can just push segs
        if (!full.segs.empty()) {
          auto& last = full.segs.back();
          if (last.pair == seg.pair && last.bold == seg.bold && last.inverse == seg.inverse) {
            last.text += seg.text;
          } else {
            full.segs.push_back(seg);
          }
        } else {
          full.segs.push_back(seg);
        }
      }

      // Clamp visible len (best-effort; segment lengths are ASCII)
      int len = 0;
      for (const auto& s : full.segs) len += (int)s.text.size();
      full.len = len;

      vis_row_t vr;
      vr.rows.push_back(std::move(full)); // exactly one row
      vis.push_back(std::move(vr));
    }
  };

  if (bb->dir == buffer_dir_t::UpDown) {
    for (const auto& L : bb->lines) push_wrapped(L); // oldest..newest
  } else {
    for (int i=(int)bb->lines.size()-1; i>=0; --i) push_wrapped(bb->lines[(size_t)i]); // newest..oldest
  }

  const int total = (int)vis.size();
  if (total <= 0) return;

  const int max_scroll = std::max(0, total - H);
  if (bb->scroll < 0) bb->scroll = 0;
  if (bb->scroll > max_scroll) bb->scroll = max_scroll;
  bb->follow_tail = (bb->scroll == 0);

  int start = 0;
  if (bb->dir == buffer_dir_t::UpDown) start = std::max(0, total - H - bb->scroll);
  else                                start = bb->scroll;

  for (int row=0; row<H; ++row) {
    int idx = start + row;
    if (idx < 0 || idx >= total) break;
    const auto& one = vis[(size_t)idx].rows;
    if (!one.empty()) {
      ansi::render_row(r.y + row, r.x, W, one[0], base_pair, obj.style.bold, obj.style.inverse);
    }
  }

  if (W > 0) {
    if (start > 0)        R->putGlyph(r.y,        r.x + (W-1), L'↑', base_pair);
    if (start + H < total) R->putGlyph(r.y + (H-1), r.x + (W-1), L'↓', base_pair);
  }
}

} // namespace iinuji
} // namespace cuwacunu
