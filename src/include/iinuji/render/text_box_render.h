#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "iinuji/iinuji_ansi.h"
#include "iinuji/render/layout_core.h"

namespace cuwacunu {
namespace iinuji {

inline void render_text(const iinuji_object_t &obj) {
  Rect r = content_rect(obj);
  auto *R = get_renderer();
  if (!R)
    return;

  short pair =
      (short)get_color_pair(obj.style.label_color, obj.style.background_color);
  if (pair == 0)
    pair = (short)get_color_pair("white", obj.style.background_color);
  R->fillRect(r.y, r.x, r.h, r.w, pair);

  auto tb = std::dynamic_pointer_cast<textBox_data_t>(obj.data);
  if (!tb)
    return;

  auto pair_for_emphasis = [&](text_line_emphasis_t e,
                               const std::string &bg) -> short {
    auto resolve_pair = [&](const std::string &fg) -> short {
      short resolved = (short)get_color_pair(fg, bg);
      if (resolved != 0)
        return resolved;
      resolved = (short)get_color_pair(obj.style.label_color, bg);
      return resolved != 0 ? resolved : pair;
    };
    switch (e) {
    case text_line_emphasis_t::Accent:
      return resolve_pair("#C89C3A");
    case text_line_emphasis_t::Success:
      return resolve_pair("#4D7A52");
    case text_line_emphasis_t::Fatal:
      return resolve_pair("#ff0000");
    case text_line_emphasis_t::Error:
      return resolve_pair("#c61c41");
    case text_line_emphasis_t::MutedError:
      return resolve_pair("#9F5A63");
    case text_line_emphasis_t::Warning:
      return resolve_pair("#F0B43C");
    case text_line_emphasis_t::MutedWarning:
      return resolve_pair("#A98534");
    case text_line_emphasis_t::Info:
      return resolve_pair("#96989a");
    case text_line_emphasis_t::Debug:
      return resolve_pair("#3F86C7");
    case text_line_emphasis_t::None:
      return resolve_pair(obj.style.label_color);
    }
    return pair;
  };
  auto fg_for_emphasis = [&](text_line_emphasis_t e) -> std::string {
    switch (e) {
    case text_line_emphasis_t::Accent:
      return "#C89C3A";
    case text_line_emphasis_t::Success:
      return "#4D7A52";
    case text_line_emphasis_t::Fatal:
      return "#ff0000";
    case text_line_emphasis_t::Error:
      return "#c61c41";
    case text_line_emphasis_t::MutedError:
      return "#9F5A63";
    case text_line_emphasis_t::Warning:
      return "#F0B43C";
    case text_line_emphasis_t::MutedWarning:
      return "#A98534";
    case text_line_emphasis_t::Info:
      return "#96989a";
    case text_line_emphasis_t::Debug:
      return "#3F86C7";
    case text_line_emphasis_t::None:
      return obj.style.label_color;
    }
    return obj.style.label_color;
  };
  auto emphasis_is_bold = [](text_line_emphasis_t e) {
    return e == text_line_emphasis_t::Accent ||
           e == text_line_emphasis_t::Fatal ||
           e == text_line_emphasis_t::Error ||
           e == text_line_emphasis_t::Warning;
  };
  auto render_row_window = [&](int y, int x, int width, const ansi::row_t &row,
                               int skip_cols, short fallback_pair,
                               bool base_bold, bool base_inverse) {
    if (width <= 0)
      return;
    int drawn = 0;
    int skip = std::max(0, skip_cols);
    for (const auto &seg : row.segs) {
      const int seg_width = utf8_display_width(seg.text);
      if (skip >= seg_width) {
        skip -= seg_width;
        continue;
      }
      const std::size_t start =
          utf8_byte_offset_for_width(std::string_view(seg.text), skip);
      skip = 0;
      const std::string_view tail = std::string_view(seg.text).substr(start);
      const std::size_t take_bytes =
          utf8_prefix_bytes_for_width(tail, std::max(0, width - drawn));
      if (take_bytes == 0)
        break;
      const std::string shown = std::string(tail.substr(0, take_bytes));
      const int shown_width = utf8_display_width(shown);
      if (shown_width <= 0)
        continue;
      const short seg_pair = seg.pair ? seg.pair : fallback_pair;
      R->putText(y, x + drawn, shown, shown_width, seg_pair,
                 seg.bold || base_bold, seg.inverse || base_inverse);
      drawn += shown_width;
      if (drawn >= width)
        break;
    }
  };

  // Focused input caret rendering:
  // We treat any focused+focusable textBox as an input line (since labels are
  // not focusable).
  if (obj.focused && obj.focusable) {
    // Single-line input behavior
    std::string line = tb->content;
    if (auto p = line.find('\n'); p != std::string::npos)
      line.resize(p);

    const int H = r.h;
    const int W = r.w;
    if (H <= 0 || W <= 0)
      return;

    // reserve last column for caret
    const int vis_w = std::max(0, W - 1);

    const std::size_t len = line.size();
    std::size_t start = 0;
    if (vis_w > 0 && len > (std::size_t)vis_w)
      start = len - (std::size_t)vis_w;

    std::string shown =
        (vis_w > 0) ? line.substr(start, (std::size_t)vis_w) : std::string();

    // draw visible text
    if (vis_w > 0) {
      R->putText(r.y, r.x, shown, vis_w, pair, obj.style.bold,
                 obj.style.inverse);
    }

    // caret position (after last visible char)
    std::size_t caret_off = len - start;
    if (caret_off > (std::size_t)vis_w)
      caret_off = (std::size_t)vis_w;
    int cx = r.x + (int)caret_off;

    // draw caret (inverse to make it obvious)
    if (cx >= r.x && cx < r.x + W) {
      R->putText(r.y, cx, "|", 1, pair, /*bold*/ true, /*inverse*/ true);
    }

    return; // do not fall through to multiline wrap rendering
  }

  const int H = std::max(0, r.h);
  const int W = std::max(0, r.w);
  if (H <= 0 || W <= 0)
    return;

  // Multiline support:
  // - if wrap=true  : wrap each newline-separated line
  // - if wrap=false : still split on '\n' so multiline labels render correctly
  // ANSI-aware path (for dlogs-like output)
  if (ansi::has_esc(tb->content)) {
    struct ansi_render_line_t {
      ansi::row_t row{};
      short pair{0};
      bool bold{false};
      bool inverse{false};
      std::string background_color{};
    };

    auto row_background_pair = [&](const std::string &bg) -> short {
      if (bg.empty())
        return pair;
      short bg_pair = (short)get_color_pair(
          obj.style.label_color.empty() ? "white" : obj.style.label_color, bg);
      if (bg_pair == 0)
        bg_pair = pair;
      return bg_pair;
    };

    std::vector<ansi_render_line_t> ansi_lines{};
    int reserve_v = 0;
    int reserve_h = 0;
    int text_w = W;
    int text_h = H;
    int max_line_len = 0;

    auto build_ansi_lines = [&](int width,
                                std::vector<ansi_render_line_t> *out_lines,
                                int *out_max_line_len) {
      if (out_lines == nullptr)
        return;
      out_lines->clear();
      int max_len = 0;

      auto append_text = [&](const std::string &text,
                             text_line_emphasis_t emphasis,
                             const std::string &background_color) {
        const std::string line_bg = background_color.empty()
                                        ? obj.style.background_color
                                        : background_color;
        const short line_pair = pair_for_emphasis(emphasis, line_bg);
        const bool line_bold = obj.style.bold || emphasis_is_bold(emphasis);
        const bool line_inverse = obj.style.inverse;
        ansi::style_t base;
        base.fg = fg_for_emphasis(emphasis);
        base.bg = line_bg;
        base.bold = line_bold;
        base.inverse = line_inverse;
        base.dim = false;

        const auto phys = split_lines_keep_empty(text);
        for (const auto &pl : phys) {
          std::vector<ansi::row_t> rows;
          const int wrap_width = tb->wrap
                                     ? std::max(1, width)
                                     : std::max(1, static_cast<int>(pl.size()));
          ansi::hard_wrap(pl, wrap_width, base, line_pair, rows);
          if (rows.empty())
            rows.push_back(ansi::row_t{});

          for (std::size_t i = 0; i < rows.size(); ++i) {
            max_len = std::max(max_len, rows[i].len);
            out_lines->push_back(ansi_render_line_t{
                .row = std::move(rows[i]),
                .pair = line_pair,
                .bold = line_bold,
                .inverse = line_inverse,
                .background_color = line_bg,
            });
            if (!tb->wrap)
              break;
          }
        }
      };

      if (!tb->styled_lines.empty()) {
        for (const auto &line : tb->styled_lines) {
          append_text(line.text, line.emphasis, line.background_color);
        }
      } else {
        append_text(tb->content, text_line_emphasis_t::None, {});
      }

      if (out_lines->empty()) {
        out_lines->push_back(ansi_render_line_t{});
      }
      if (out_max_line_len)
        *out_max_line_len = max_len;
    };

    for (int it = 0; it < 3; ++it) {
      text_w = std::max(0, W - reserve_v);
      text_h = std::max(0, H - reserve_h);
      if (text_w <= 0 || text_h <= 0)
        return;

      build_ansi_lines(text_w, &ansi_lines, &max_line_len);

      const bool need_h = (!tb->wrap && H > 1 && max_line_len > text_w);
      const int reserve_h_new = need_h ? 1 : 0;
      const int text_h_if = std::max(0, H - reserve_h_new);
      const bool need_v = static_cast<int>(ansi_lines.size()) > text_h_if;
      const int reserve_v_new = need_v ? 1 : 0;

      if (reserve_h_new == reserve_h && reserve_v_new == reserve_v)
        break;
      reserve_h = reserve_h_new;
      reserve_v = reserve_v_new;
    }

    text_w = std::max(0, W - reserve_v);
    text_h = std::max(0, H - reserve_h);
    if (text_w <= 0 || text_h <= 0)
      return;

    build_ansi_lines(text_w, &ansi_lines, &max_line_len);

    const int max_scroll_y = std::max(0, (int)ansi_lines.size() - text_h);
    const int max_scroll_x = tb->wrap ? 0 : std::max(0, max_line_len - text_w);
    tb->scroll_y = std::clamp(tb->scroll_y, 0, max_scroll_y);
    tb->scroll_x = std::clamp(tb->scroll_x, 0, max_scroll_x);

    for (int row = 0; row < text_h; ++row) {
      const int li = tb->scroll_y + row;
      if (li < 0 || li >= (int)ansi_lines.size())
        break;

      const auto &line = ansi_lines[(std::size_t)li];
      if (!line.background_color.empty()) {
        R->fillRect(r.y + row, r.x, 1, text_w,
                    row_background_pair(line.background_color));
      }
      int colx = r.x;
      if (tb->scroll_x == 0 && reserve_v == 0) {
        if (tb->align == text_align_t::Center) {
          colx = r.x + std::max(0, (text_w - line.row.len) / 2);
        } else if (tb->align == text_align_t::Right) {
          colx = r.x + std::max(0, text_w - line.row.len);
        }
      }

      const int take =
          tb->wrap ? std::min(text_w, line.row.len)
                   : std::min(text_w, std::max(0, line.row.len - tb->scroll_x));
      if (take <= 0)
        continue;
      render_row_window(r.y + row, colx, take, line.row,
                        tb->wrap ? 0 : tb->scroll_x, line.pair, line.bold,
                        line.inverse);
    }

    short bar_pair = (short)get_color_pair(obj.style.border_color,
                                           obj.style.background_color);
    if (bar_pair == 0)
      bar_pair = pair;

    if (reserve_v > 0 && text_h > 0) {
      const int bar_x = r.x + text_w;
      for (int i = 0; i < text_h; ++i) {
        R->putGlyph(r.y + i, bar_x, L'│', bar_pair);
      }

      const int total_rows = std::max(1, (int)ansi_lines.size());
      int thumb_h = (int)std::lround((double)text_h * (double)text_h /
                                     (double)total_rows);
      thumb_h = std::clamp(thumb_h, 1, text_h);
      const int thumb_span = std::max(0, text_h - thumb_h);
      const int thumb_y =
          (max_scroll_y > 0)
              ? (int)std::lround((double)tb->scroll_y * (double)thumb_span /
                                 (double)max_scroll_y)
              : 0;

      for (int i = 0; i < thumb_h; ++i) {
        R->putGlyph(r.y + thumb_y + i, bar_x, L'█', bar_pair);
      }
    }

    if (reserve_h > 0 && text_w > 0) {
      const int bar_y = r.y + text_h;
      for (int i = 0; i < text_w; ++i) {
        R->putGlyph(bar_y, r.x + i, L'─', bar_pair);
      }

      const int total_cols = std::max(1, max_line_len);
      int thumb_w = (int)std::lround((double)text_w * (double)text_w /
                                     (double)total_cols);
      thumb_w = std::clamp(thumb_w, 1, text_w);
      const int thumb_span = std::max(0, text_w - thumb_w);
      const int thumb_x =
          (max_scroll_x > 0)
              ? (int)std::lround((double)tb->scroll_x * (double)thumb_span /
                                 (double)max_scroll_x)
              : 0;

      for (int i = 0; i < thumb_w; ++i) {
        R->putGlyph(bar_y, r.x + thumb_x + i, L'█', bar_pair);
      }

      if (reserve_v > 0) {
        R->putGlyph(bar_y, r.x + text_w, L'┘', bar_pair);
      }
    }
    return;
  }

  // non-ANSI path with scrollable viewport + scrollbars
  int reserve_v = 0;
  int reserve_h = 0;
  int text_w = W;
  int text_h = H;
  int max_line_len = 0;
  struct plain_render_line_t {
    ansi::row_t row{};
    std::string background_color{};
  };
  std::vector<plain_render_line_t> lines;

  auto row_background_pair = [&](const std::string &bg) -> short {
    if (bg.empty())
      return pair;
    short bg_pair = (short)get_color_pair(
        obj.style.label_color.empty() ? "white" : obj.style.label_color, bg);
    if (bg_pair == 0)
      bg_pair = pair;
    return bg_pair;
  };

  auto build_text_lines = [&](int width) {
    lines.clear();
    const int safe_w = std::max(1, width);
    auto append_styled_line = [&](const styled_text_line_t &line) {
      std::vector<styled_text_segment_t> segments = line.segments;
      if (segments.empty()) {
        segments.push_back(styled_text_segment_t{
            .text = line.text,
            .emphasis = line.emphasis,
        });
      }

      const std::string line_bg = line.background_color.empty()
                                      ? obj.style.background_color
                                      : line.background_color;
      ansi::row_t row{};
      int col = 0;
      bool emitted_row = false;
      auto commit_row = [&](bool force) {
        if (force || row.len > 0 || !row.segs.empty() || !emitted_row) {
          lines.push_back(plain_render_line_t{
              .row = std::move(row),
              .background_color = line_bg,
          });
          emitted_row = true;
        }
        row = ansi::row_t{};
        col = 0;
      };

      for (const auto &segment : segments) {
        const short segment_pair = pair_for_emphasis(segment.emphasis, line_bg);
        const bool segment_bold = emphasis_is_bold(segment.emphasis);
        std::string run{};
        for (std::size_t offset = 0; offset < segment.text.size();) {
          const char ch = segment.text[offset];
          if (ch == '\r') {
            ++offset;
            continue;
          }
          if (ch == '\n') {
            if (!run.empty()) {
              ansi::append_plain(row, run, segment_pair, segment_bold, false);
              run.clear();
            }
            commit_row(true);
            ++offset;
            continue;
          }
          const std::size_t bytes =
              utf8_codepoint_bytes(std::string_view(segment.text), offset);
          run.append(segment.text, offset, bytes);
          if (tb->wrap) {
            col += std::max(
                0, utf8_codepoint_display_width(
                       std::string_view(segment.text).substr(offset, bytes)));
            if (col >= safe_w) {
              ansi::append_plain(row, run, segment_pair, segment_bold, false);
              run.clear();
              commit_row(true);
            }
          }
          offset += bytes;
        }
        if (!run.empty()) {
          ansi::append_plain(row, run, segment_pair, segment_bold, false);
        }
      }

      if (!emitted_row || row.len > 0 || !row.segs.empty())
        commit_row(true);
    };

    if (!tb->styled_lines.empty()) {
      for (const auto &line : tb->styled_lines)
        append_styled_line(line);
    } else {
      append_styled_line(styled_text_line_t{
          .text = tb->content,
          .emphasis = text_line_emphasis_t::None,
      });
    }
  };

  // Resolve scrollbar reservations (vertical + horizontal) with a few stable
  // iterations.
  for (int it = 0; it < 3; ++it) {
    text_w = std::max(0, W - reserve_v);
    text_h = std::max(0, H - reserve_h);
    if (text_w <= 0 || text_h <= 0)
      return;

    build_text_lines(text_w);

    max_line_len = 0;
    for (const auto &ln : lines)
      max_line_len = std::max(max_line_len, ln.row.len);

    const bool need_h = (!tb->wrap && H > 1 && max_line_len > text_w);
    const int reserve_h_new = need_h ? 1 : 0;
    const int text_h_if = std::max(0, H - reserve_h_new);
    const bool need_v = ((int)lines.size() > text_h_if);
    const int reserve_v_new = need_v ? 1 : 0;

    if (reserve_h_new == reserve_h && reserve_v_new == reserve_v)
      break;
    reserve_h = reserve_h_new;
    reserve_v = reserve_v_new;
  }

  text_w = std::max(0, W - reserve_v);
  text_h = std::max(0, H - reserve_h);
  if (text_w <= 0 || text_h <= 0)
    return;

  build_text_lines(text_w);

  max_line_len = 0;
  for (const auto &ln : lines)
    max_line_len = std::max(max_line_len, ln.row.len);

  const int max_scroll_y = std::max(0, (int)lines.size() - text_h);
  const int max_scroll_x = tb->wrap ? 0 : std::max(0, max_line_len - text_w);
  tb->scroll_y = std::clamp(tb->scroll_y, 0, max_scroll_y);
  tb->scroll_x = std::clamp(tb->scroll_x, 0, max_scroll_x);

  for (int row = 0; row < text_h; ++row) {
    const int li = tb->scroll_y + row;
    if (li < 0 || li >= (int)lines.size())
      break;

    const auto &line = lines[(std::size_t)li];
    if (!line.background_color.empty()) {
      R->fillRect(r.y + row, r.x, 1, text_w,
                  row_background_pair(line.background_color));
    }
    const int visible_len = line.row.len;
    int colx = r.x;
    if (tb->scroll_x == 0 && reserve_v == 0) {
      if (tb->align == text_align_t::Center)
        colx = r.x + std::max(0, (text_w - visible_len) / 2);
      else if (tb->align == text_align_t::Right)
        colx = r.x + std::max(0, text_w - visible_len);
    }

    if (!tb->wrap) {
      const int skip = tb->scroll_x;
      if (skip >= line.row.len)
        continue;
      const int take = std::min(text_w, line.row.len - skip);
      if (take <= 0)
        continue;
      render_row_window(r.y + row, colx, take, line.row, skip, pair,
                        obj.style.bold, obj.style.inverse);
    } else {
      const int take = std::min(text_w, line.row.len);
      if (take <= 0)
        continue;
      render_row_window(r.y + row, colx, take, line.row, 0, pair,
                        obj.style.bold, obj.style.inverse);
    }
  }

  short bar_pair =
      (short)get_color_pair(obj.style.border_color, obj.style.background_color);
  if (bar_pair == 0)
    bar_pair = pair;

  // Vertical scrollbar
  if (reserve_v > 0 && text_h > 0) {
    const int bar_x = r.x + text_w;
    for (int i = 0; i < text_h; ++i) {
      R->putGlyph(r.y + i, bar_x, L'│', bar_pair);
    }

    const int total_rows = std::max(1, (int)lines.size());
    int thumb_h =
        (int)std::lround((double)text_h * (double)text_h / (double)total_rows);
    thumb_h = std::clamp(thumb_h, 1, text_h);
    const int thumb_span = std::max(0, text_h - thumb_h);
    const int thumb_y =
        (max_scroll_y > 0)
            ? (int)std::lround((double)tb->scroll_y * (double)thumb_span /
                               (double)max_scroll_y)
            : 0;

    for (int i = 0; i < thumb_h; ++i) {
      R->putGlyph(r.y + thumb_y + i, bar_x, L'█', bar_pair);
    }
  }

  // Horizontal scrollbar
  if (reserve_h > 0 && text_w > 0) {
    const int bar_y = r.y + text_h;
    for (int i = 0; i < text_w; ++i) {
      R->putGlyph(bar_y, r.x + i, L'─', bar_pair);
    }

    const int total_cols = std::max(1, max_line_len);
    int thumb_w =
        (int)std::lround((double)text_w * (double)text_w / (double)total_cols);
    thumb_w = std::clamp(thumb_w, 1, text_w);
    const int thumb_span = std::max(0, text_w - thumb_w);
    const int thumb_x =
        (max_scroll_x > 0)
            ? (int)std::lround((double)tb->scroll_x * (double)thumb_span /
                               (double)max_scroll_x)
            : 0;

    for (int i = 0; i < thumb_w; ++i) {
      R->putGlyph(bar_y, r.x + thumb_x + i, L'█', bar_pair);
    }

    if (reserve_v > 0) {
      R->putGlyph(bar_y, r.x + text_w, L'┘', bar_pair);
    }
  }
}

} // namespace iinuji
} // namespace cuwacunu
