#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "iinuji/primitives/editor.h"
#include "iinuji/render/layout_core.h"

namespace cuwacunu {
namespace iinuji {

inline int digits10_i(int v) {
  if (v < 0) v = -v;
  int d = 1;
  while (v >= 10) {
    v /= 10;
    ++d;
  }
  return d;
}

enum class editor_status_tone_t {
  Neutral = 0,
  Info,
  Success,
  Warning,
  Error,
};

inline std::string lower_ascii_copy_editor_render(std::string s) {
  std::transform(
      s.begin(),
      s.end(),
      s.begin(),
      [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return s;
}

inline editor_status_tone_t classify_editor_status_tone(std::string_view status) {
  if (status.empty()) return editor_status_tone_t::Neutral;
  const std::string lower = lower_ascii_copy_editor_render(std::string(status));
  if (lower.find("fail") != std::string::npos ||
      lower.find("error") != std::string::npos ||
      lower.find("invalid") != std::string::npos ||
      lower.find("cannot") != std::string::npos) {
    return editor_status_tone_t::Error;
  }
  if (lower.find("unsaved") != std::string::npos ||
      lower.find("discard") != std::string::npos ||
      lower.find("disabled") != std::string::npos) {
    return editor_status_tone_t::Warning;
  }
  if (lower.find("saved") != std::string::npos ||
      lower.find("loaded") != std::string::npos ||
      lower.find("accepted") != std::string::npos ||
      lower == "valid") {
    return editor_status_tone_t::Success;
  }
  if (lower.find("completion") != std::string::npos ||
      lower.find("editing") != std::string::npos ||
      lower.find("indent") != std::string::npos ||
      lower.find("command mode") != std::string::npos) {
    return editor_status_tone_t::Info;
  }
  return editor_status_tone_t::Neutral;
}

inline std::string ellipsize_middle_editor(std::string_view s, int max_w) {
  if (max_w <= 0) return {};
  if (static_cast<int>(s.size()) <= max_w) return std::string(s);
  if (max_w <= 3) return std::string(static_cast<std::size_t>(max_w), '.');
  const int body = max_w - 3;
  const int head = body / 2;
  const int tail = body - head;
  return std::string(s.substr(0, static_cast<std::size_t>(head))) +
         "..." +
         std::string(s.substr(s.size() - static_cast<std::size_t>(tail)));
}

inline std::string editor_footer_state_text(const editorBox_data_t& ed) {
  std::string state{};
  if (ed.close_armed) {
    state = "Unsaved changes";
  } else if (ed.dirty) {
    state = "Modified";
  } else if (ed.read_only) {
    state = "Read only";
  } else {
    state = "Saved";
  }

  if (!ed.status.empty()) {
    const std::string lower = lower_ascii_copy_editor_render(ed.status);
    if (state.empty() || lower.find(lower_ascii_copy_editor_render(state)) != std::string::npos) {
      return ed.status;
    }
    return state + " | " + ed.status;
  }
  return state;
}

inline std::string editor_footer_hint_text(const editorBox_data_t& ed) {
  if (ed.close_armed) {
    return std::string("Ctrl+S save+close | ") +
           (ed.close_armed_via_escape ? "Esc discard" : "Ctrl+Q discard") +
           " | any other key keep editing";
  }
  if (ed.read_only) return "Esc or Ctrl+Q close";
  return "Ctrl+S save | Ctrl+Z undo | Ctrl+Y redo | Ctrl+Q close";
}

inline std::string editor_current_line_bg_token(const std::string& base_bg) {
  int r = 0;
  int g = 0;
  int b = 0;
  if (!rgb8_for_token(base_bg, r, g, b)) return "#11161f";
  const int luma = (54 * r + 183 * g + 19 * b) / 256;
  if (luma < 96) {
    r = std::min(255, r + 18);
    g = std::min(255, g + 18);
    b = std::min(255, b + 26);
    return rgb8_to_hex(r, g, b);
  }
  return darken_color_token(base_bg, 0.92);
}

inline short editor_status_pair(editor_status_tone_t tone, const std::string& bg, short fallback_pair) {
  const char* fg = nullptr;
  switch (tone) {
    case editor_status_tone_t::Info: fg = "#89B4FA"; break;
    case editor_status_tone_t::Success: fg = "#76C893"; break;
    case editor_status_tone_t::Warning: fg = "#F2B880"; break;
    case editor_status_tone_t::Error: fg = "#E57A7A"; break;
    case editor_status_tone_t::Neutral:
    default: fg = "#A8A8AF"; break;
  }
  const short pair = static_cast<short>(get_color_pair(fg, bg));
  return pair == 0 ? fallback_pair : pair;
}

struct editor_rendered_slice_t {
  std::string text{};
  std::vector<short> pairs{};
};

inline editor_rendered_slice_t build_editor_visible_slice(const editorBox_data_t& ed,
                                                          std::string_view line,
                                                          std::span<const short> raw_pairs,
                                                          short base_pair,
                                                          short accent_pair,
                                                          int text_w) {
  editor_rendered_slice_t out{};
  if (text_w <= 0) return out;

  out.text.reserve(static_cast<std::size_t>(text_w));
  out.pairs.reserve(static_cast<std::size_t>(text_w));

  const int left_visual_col = std::max(0, ed.left_col);
  const int raw_start = primitives::editor_raw_column_for_visual_column(ed, line, left_visual_col);
  int visual_col = left_visual_col;
  for (int raw = raw_start;
       raw < static_cast<int>(line.size()) && static_cast<int>(out.text.size()) < text_w;
       ++raw) {
    const char ch = line[static_cast<std::size_t>(raw)];
    const short raw_pair =
        (raw < static_cast<int>(raw_pairs.size())) ? raw_pairs[static_cast<std::size_t>(raw)] : base_pair;
    if (ch == '\t') {
      const int adv = primitives::editor_visual_advance(ed, ch, visual_col);
      for (int i = 0;
           i < adv && static_cast<int>(out.text.size()) < text_w;
           ++i, ++visual_col) {
        out.text.push_back(ed.show_tabs ? (i == 0 ? '>' : '.') : ' ');
        out.pairs.push_back(ed.show_tabs ? accent_pair : raw_pair);
      }
      continue;
    }
    if ((static_cast<unsigned char>(ch) < 32u) || ch == 127) {
      out.text.push_back('?');
      out.pairs.push_back(accent_pair);
      ++visual_col;
      continue;
    }
    out.text.push_back(ch);
    out.pairs.push_back(raw_pair);
    ++visual_col;
  }
  return out;
}

inline void render_editor_segmented_text(
    IRend* R,
    int y,
    int x,
    const std::string& text,
    std::span<const short> pairs) {
  if (!R || text.empty()) return;
  int offset = 0;
  while (offset < static_cast<int>(text.size())) {
    const short pair =
        (offset < static_cast<int>(pairs.size())) ? pairs[static_cast<std::size_t>(offset)] : 0;
    int end = offset + 1;
    while (end < static_cast<int>(text.size()) &&
           end < static_cast<int>(pairs.size()) &&
           pairs[static_cast<std::size_t>(end)] == pair) {
      ++end;
    }
    R->putText(
        y,
        x + offset,
        text.substr(static_cast<std::size_t>(offset), static_cast<std::size_t>(end - offset)),
        end - offset,
        pair,
        false,
        false);
    offset = end;
  }
}

inline std::string editor_gutter_text(int line_number, int width, bool current_line) {
  if (width <= 0) return {};
  std::string gutter(static_cast<std::size_t>(width), ' ');
  gutter[0] = current_line ? '>' : ' ';
  if (width == 1) return gutter;

  const std::string num = std::to_string(line_number);
  const int number_end = width - 1;
  int number_start = std::max(1, number_end - static_cast<int>(num.size()));
  for (int i = 0; i < static_cast<int>(num.size()) && number_start + i < number_end; ++i) {
    gutter[static_cast<std::size_t>(number_start + i)] = num[static_cast<std::size_t>(i)];
  }
  gutter[static_cast<std::size_t>(width - 1)] = '|';
  return gutter;
}

inline void render_editor(const iinuji_object_t& obj) {
  Rect r = content_rect(obj);
  auto* R = get_renderer();
  if (!R) return;

  short base_pair = static_cast<short>(get_color_pair(obj.style.label_color, obj.style.background_color));
  R->fillRect(r.y, r.x, r.h, r.w, base_pair);

  auto ed = std::dynamic_pointer_cast<editorBox_data_t>(obj.data);
  if (!ed) return;
  ed->ensure_nonempty();

  const int H = std::max(0, r.h);
  const int W = std::max(0, r.w);
  if (H <= 0 || W <= 0) return;

  const short path_pair =
      static_cast<short>(get_color_pair("#D8DEE9", obj.style.background_color));
  const short mode_pair = static_cast<short>(get_color_pair(
      ed->read_only ? "#E57A7A" : "#7FD4C6",
      obj.style.background_color));
  const short dirty_pair =
      static_cast<short>(get_color_pair("#F2B880", obj.style.background_color));
  const editor_status_tone_t status_tone = classify_editor_status_tone(ed->status);
  const short right_pair = editor_status_pair(status_tone, obj.style.background_color, base_pair);
  const short hint_pair =
      static_cast<short>(get_color_pair("#7E8696", obj.style.background_color));
  const short success_pair =
      static_cast<short>(get_color_pair("#76C893", obj.style.background_color));

  const int header_h = (ed->show_header && H > 0) ? 1 : 0;
  const int footer_h = (ed->show_footer && H - header_h > 1) ? 1 : 0;
  const int body_y = r.y + header_h;
  const int body_h = std::max(0, H - header_h - footer_h);

  if (header_h > 0) {
    const int total_lines = std::max(1, static_cast<int>(ed->lines.size()));
    std::string file = ed->path.empty() ? std::string("<new file>") : ed->path;
    const std::string mode = ed->read_only ? "[RO]" : "[INS]";

    char pos_buf[96];
    std::snprintf(
        pos_buf,
        sizeof(pos_buf),
        "Ln %d/%d Col %d",
        ed->cursor_line + 1,
        total_lines,
        ed->cursor_col + 1);

    std::string right = pos_buf;
    if (ed->dirty) {
      right += " | modified";
    } else if (ed->read_only) {
      right += " | read only";
    }
    if (static_cast<int>(right.size()) > W) {
      right = right.substr(right.size() - static_cast<std::size_t>(W));
    }

    const int right_w = std::min(W, static_cast<int>(right.size()));
    const int right_x = r.x + W - right_w;

    int left_x = r.x;
    if (ed->dirty && left_x < right_x) {
      R->putText(r.y, left_x, "*", 1, dirty_pair == 0 ? base_pair : dirty_pair, true, false);
      left_x += 1;
      if (left_x < right_x) {
        R->putText(r.y, left_x, " ", 1, base_pair, false, false);
        ++left_x;
      }
    }

    if (left_x < right_x) {
      const short pair = mode_pair == 0 ? base_pair : mode_pair;
      const int width = std::min(right_x - left_x, static_cast<int>(mode.size()));
      if (width > 0) {
        R->putText(r.y, left_x, mode, width, pair, true, false);
        left_x += width;
      }
      if (left_x < right_x) {
        R->putText(r.y, left_x, " ", 1, base_pair, false, false);
        ++left_x;
      }
    }

    if (left_x < right_x) {
      const int available = right_x - left_x;
      const std::string compact = ellipsize_middle_editor(file, available);
      const short pair = path_pair == 0 ? base_pair : path_pair;
      if (!compact.empty()) {
        R->putText(r.y, left_x, compact, available, pair, false, false);
      }
    }

    if (right_w > 0) {
      R->putText(r.y, right_x, right, right_w, path_pair == 0 ? base_pair : path_pair, false, false);
    }
  }

  if (footer_h > 0) {
    const int footer_y = r.y + H - 1;
    const std::string state = ellipsize_middle_editor(editor_footer_state_text(*ed), W);
    const std::string hint = ellipsize_middle_editor(editor_footer_hint_text(*ed), W);
    const int hint_w = std::min(W, static_cast<int>(hint.size()));
    const int hint_x = r.x + W - hint_w;
    int state_x = r.x;
    const int state_w = std::max(0, hint_x - state_x - (hint_w > 0 ? 1 : 0));

    if (!state.empty() && state_w > 0) {
      const std::string shown_state = ellipsize_middle_editor(state, state_w > 0 ? state_w : W);
      const short state_pair = ed->close_armed
                                   ? (dirty_pair == 0 ? right_pair : dirty_pair)
                                   : (ed->dirty ? (dirty_pair == 0 ? right_pair : dirty_pair)
                                                : (ed->read_only ? (mode_pair == 0 ? right_pair : mode_pair)
                                                                 : (success_pair == 0 ? right_pair : success_pair)));
      const int draw_w = std::min(W, static_cast<int>(shown_state.size()));
      if (draw_w > 0) {
        R->putText(footer_y, state_x, shown_state, draw_w, state_pair, true, false);
      }
    }

    if (hint_w > 0) {
      R->putText(
          footer_y,
          hint_x,
          hint,
          hint_w,
          hint_pair == 0 ? base_pair : hint_pair,
          false,
          false);
    }
  }

  if (body_h <= 0) {
    ed->last_body_h = 0;
    ed->last_lineno_w = 0;
    ed->last_text_w = 0;
    return;
  }

  const int total_lines = std::max(1, static_cast<int>(ed->lines.size()));
  const int digits = digits10_i(total_lines);
  int ln_w = 0;
  if (ed->show_line_numbers) {
    ln_w = std::min(W, digits + 3);
    if (ln_w < 4) ln_w = std::min(W, 4);
  }
  const int text_w = std::max(0, W - ln_w);

  ed->last_body_h = body_h;
  ed->last_lineno_w = ln_w;
  ed->last_text_w = text_w;

  if (ed->top_line < 0) ed->top_line = 0;
  if (ed->top_line > total_lines - 1) ed->top_line = total_lines - 1;
  if (ed->left_col < 0) ed->left_col = 0;

  const std::string current_bg = editor_current_line_bg_token(obj.style.background_color);
  const short gutter_pair =
      static_cast<short>(get_color_pair("#7E8696", obj.style.background_color));
  const short current_gutter_pair =
      static_cast<short>(get_color_pair("#E3C779", current_bg));
  const short delimiter_pair =
      static_cast<short>(get_color_pair("#F6D06F", current_bg));
  const short delimiter_base_pair =
      static_cast<short>(get_color_pair("#F6D06F", obj.style.background_color));

  const auto delimiter_match =
      (ed->highlight_matching_delimiter && obj.focused && obj.focusable)
          ? primitives::editor_matching_delimiter_at_cursor(*ed)
          : std::optional<primitives::editor_delimiter_match_t>{};

  for (int row = 0; row < body_h; ++row) {
    const int li = ed->top_line + row;
    const bool line_exists = li >= 0 && li < static_cast<int>(ed->lines.size());
    const bool current_line = line_exists && li == ed->cursor_line && ed->highlight_current_line;
    const std::string row_bg = current_line ? current_bg : obj.style.background_color;
    const short row_base_pair =
        static_cast<short>(get_color_pair(obj.style.label_color, row_bg));
    const short row_gutter_pair = current_line
                                      ? (current_gutter_pair == 0 ? row_base_pair : current_gutter_pair)
                                      : (gutter_pair == 0 ? row_base_pair : gutter_pair);
    const short accent_pair =
        static_cast<short>(get_color_pair("#6D7890", row_bg));
    const short row_accent_pair = accent_pair == 0 ? row_base_pair : accent_pair;

    if (current_line) {
      R->fillRect(body_y + row, r.x, 1, W, row_base_pair);
    }

    if (ln_w > 0) {
      const std::string gutter =
          line_exists ? editor_gutter_text(li + 1, ln_w, current_line) : std::string(static_cast<std::size_t>(ln_w), ' ');
      R->putText(body_y + row, r.x, gutter, ln_w, row_gutter_pair, current_line, false);
    }

    if (!line_exists || text_w <= 0) continue;

    const std::string& line = ed->lines[static_cast<std::size_t>(li)];
    std::vector<short> raw_colors(line.size(), row_base_pair);
    if (ed->line_colorizer && !line.empty()) {
      ed->line_colorizer(*ed, li, line, raw_colors, row_base_pair, row_bg);
      if (static_cast<int>(raw_colors.size()) < static_cast<int>(line.size())) {
        raw_colors.resize(line.size(), row_base_pair);
      }
    }

    const editor_rendered_slice_t slice = build_editor_visible_slice(
        *ed,
        line,
        raw_colors,
        row_base_pair,
        row_accent_pair,
        text_w);
    render_editor_segmented_text(
        R,
        body_y + row,
        r.x + ln_w,
        slice.text,
        slice.pairs);
  }

  auto draw_delimiter = [&](int line_index, int raw_col) {
    if (text_w <= 0) return;
    if (line_index < ed->top_line || line_index >= ed->top_line + body_h) return;
    if (line_index < 0 || line_index >= static_cast<int>(ed->lines.size())) return;
    const std::string& line = ed->lines[static_cast<std::size_t>(line_index)];
    if (raw_col < 0 || raw_col >= static_cast<int>(line.size())) return;
    const unsigned char ch = static_cast<unsigned char>(line[static_cast<std::size_t>(raw_col)]);
    if (ch < 32u || ch == 127u || line[static_cast<std::size_t>(raw_col)] == '\t') return;

    const int left_visual_col = std::max(0, ed->left_col);
    const int raw_visual_col = primitives::editor_visual_column_for_raw_column(*ed, line, raw_col);
    const int xoff = raw_visual_col - left_visual_col;
    if (xoff < 0 || xoff >= text_w) return;

    const bool current_line = line_index == ed->cursor_line && ed->highlight_current_line;
    const short pair = current_line
                           ? (delimiter_pair == 0 ? base_pair : delimiter_pair)
                           : (delimiter_base_pair == 0 ? base_pair : delimiter_base_pair);
    R->putText(
        body_y + (line_index - ed->top_line),
        r.x + ln_w + xoff,
        std::string(1, static_cast<char>(ch)),
        1,
        pair,
        true,
        true);
  };

  if (delimiter_match.has_value()) {
    draw_delimiter(delimiter_match->anchor_line, delimiter_match->anchor_col);
    draw_delimiter(delimiter_match->match_line, delimiter_match->match_col);
  }

  if (W > 0 && body_h > 0) {
    if (ed->top_line > 0) R->putGlyph(body_y, r.x + (W - 1), L'↑', base_pair);
    if (ed->top_line + body_h < static_cast<int>(ed->lines.size())) {
      R->putGlyph(body_y + (body_h - 1), r.x + (W - 1), L'↓', base_pair);
    }
    if (ed->left_col > 0) {
      const int top_hint_y = header_h > 0 ? r.y : body_y;
      R->putGlyph(top_hint_y, r.x + (W - 1), L'←', base_pair);
    }
  }

  if (obj.focused && obj.focusable && body_h > 0 && text_w > 0) {
    const int crow = ed->cursor_line - ed->top_line;
    if (crow >= 0 && crow < body_h) {
      const std::string& line = ed->lines[static_cast<std::size_t>(ed->cursor_line)];
      const int cursor_visual_col =
          primitives::editor_visual_column_for_raw_column(*ed, line, ed->cursor_col);
      const int left_visual_col = std::max(0, ed->left_col);
      const int ccol = cursor_visual_col - left_visual_col;
      const int cx = r.x + ln_w + std::clamp(ccol, 0, std::max(0, text_w - 1));
      const int cy = body_y + crow;
      if (cx >= r.x + ln_w && cx < r.x + W) {
        R->putText(cy, cx, "|", 1, base_pair, true, true);
      }
    }
  }
}

}  // namespace iinuji
}  // namespace cuwacunu
