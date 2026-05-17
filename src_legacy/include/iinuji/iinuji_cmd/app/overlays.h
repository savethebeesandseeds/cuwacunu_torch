#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "iinuji/iinuji_render.h"
#include "iinuji/primitives/editor.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/iinuji_cmd/commands/iinuji.paths.h"
#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
find_visible_text_box(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box) {
  if (!box || !box->visible) return nullptr;
  if (as<cuwacunu::iinuji::textBox_data_t>(box)) return box;
  for (const auto& child : box->children) {
    if (auto found = find_visible_text_box(child); found) return found;
  }
  return nullptr;
}

inline void scroll_viewport_tree(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box,
                                 int dy, int dx) {
  if (!box || !box->visible) return;
  if (scroll_viewport_by(box, dy, dx)) return;
  for (const auto& child : box->children) {
    scroll_viewport_tree(child, dy, dx);
  }
}

inline void scroll_active_screen(CmdState& state,
                                 const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
                                 const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right,
                                 int dy,
                                 int dx) {
  if (dy == 0 && dx == 0) return;
  if (state.screen == ScreenMode::ShellLogs && (dy != 0 || dx != 0)) {
    state.shell_logs.auto_follow = false;
  }
  if (state.screen == ScreenMode::Runtime && dy != 0 &&
      state.runtime.log_viewer_open) {
    state.runtime.log_viewer_live_follow = false;
  }
  scroll_viewport_tree(left, dy, dx);
  scroll_viewport_tree(right, dy, dx);
}

inline void jump_logs_to_bottom(CmdState& state,
                                const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
                                const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right = nullptr) {
  if (state.screen != ScreenMode::ShellLogs) return;
  if (auto tb = as<cuwacunu::iinuji::textBox_data_t>(find_visible_text_box(left)); tb) {
    tb->scroll_y = std::numeric_limits<int>::max();
    tb->scroll_x = 0;
  }
  if (auto tb = as<cuwacunu::iinuji::textBox_data_t>(find_visible_text_box(right)); tb) {
    tb->scroll_x = 0;
  }
  state.shell_logs.auto_follow = true;
}

struct ScrollCaps {
  bool v{false};
  bool h{false};
};

inline Rect merge_overlay_rects(const Rect& a, const Rect& b) {
  const int x0 = std::min(a.x, b.x);
  const int y0 = std::min(a.y, b.y);
  const int x1 = std::max(a.x + a.w, b.x + b.w);
  const int y1 = std::max(a.y + a.h, b.y + b.h);
  return Rect{x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
}

inline ScrollCaps panel_scroll_caps(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box) {
  ScrollCaps out{};
  if (!box || !box->visible) return out;
  const Rect r = content_rect(*box);
  const int W = std::max(0, r.w);
  const int H = std::max(0, r.h);
  if (W <= 0 || H <= 0) return out;

  if (const auto ed = as<cuwacunu::iinuji::editorBox_data_t>(box); ed) {
    ed->ensure_nonempty();
    const int body_h = std::max(1, H - 1);
    const int total_lines = static_cast<int>(ed->lines.size());
    int max_line_len = 0;
    for (const auto& ln : ed->lines) {
      max_line_len = std::max(
          max_line_len,
          cuwacunu::iinuji::primitives::editor_visual_column_for_raw_column(
              *ed,
              ln,
              static_cast<int>(ln.size())));
    }

    int ln_w = 0;
    if (ed->show_line_numbers) {
      ln_w = std::min(W, digits10_i(std::max(1, total_lines)) + 3);
      if (ln_w < 4) ln_w = std::min(W, 4);
    }
    const int text_w = std::max(1, W - ln_w);
    out.v = total_lines > body_h;
    out.h = max_line_len > text_w;
    return out;
  }

  auto tb = as<cuwacunu::iinuji::textBox_data_t>(box);
  if (!tb) {
    for (const auto& child : box->children) {
      const auto child_caps = panel_scroll_caps(child);
      out.v = out.v || child_caps.v;
      out.h = out.h || child_caps.h;
    }
    return out;
  }

  int reserve_v = 0;
  int reserve_h = 0;
  int text_w = W;
  int text_h = H;
  int max_line_len = 0;
  std::vector<std::string> lines;

  for (int it = 0; it < 3; ++it) {
    text_w = std::max(0, W - reserve_v);
    text_h = std::max(0, H - reserve_h);
    if (text_w <= 0 || text_h <= 0) return out;

    if (cuwacunu::iinuji::ansi::has_esc(tb->content)) {
      max_line_len = 0;
      lines.clear();
      const auto phys = split_lines_keep_empty(tb->content);
      for (const auto& pl : phys) {
        std::vector<cuwacunu::iinuji::ansi::row_t> rows;
        const int wrap_width =
            tb->wrap ? std::max(1, text_w)
                     : std::max(1, static_cast<int>(pl.size()));
        cuwacunu::iinuji::ansi::style_t base{};
        cuwacunu::iinuji::ansi::hard_wrap(pl, wrap_width, base, 0, rows);
        if (rows.empty()) rows.push_back(cuwacunu::iinuji::ansi::row_t{});
        for (std::size_t i = 0; i < rows.size(); ++i) {
          lines.push_back(std::string((std::size_t)rows[i].len, 'x'));
          max_line_len = std::max(max_line_len, rows[i].len);
          if (!tb->wrap) break;
        }
      }
    } else {
      lines = tb->wrap
          ? wrap_text(tb->content, std::max(1, text_w))
          : split_lines_keep_empty(tb->content);
      max_line_len = 0;
      for (const auto& ln : lines) {
        max_line_len = std::max(max_line_len, static_cast<int>(ln.size()));
      }
    }

    const bool need_h = (!tb->wrap && max_line_len > text_w);
    const int reserve_h_new = need_h ? 1 : 0;
    const int text_h_if = std::max(0, H - reserve_h_new);
    const bool need_v = static_cast<int>(lines.size()) > text_h_if;
    const int reserve_v_new = need_v ? 1 : 0;

    if (reserve_h_new == reserve_h && reserve_v_new == reserve_v) break;
    reserve_h = reserve_h_new;
    reserve_v = reserve_v_new;
  }

  text_w = std::max(0, W - reserve_v);
  text_h = std::max(0, H - reserve_h);
  if (text_w <= 0 || text_h <= 0) return out;

  if (cuwacunu::iinuji::ansi::has_esc(tb->content)) {
    max_line_len = 0;
    lines.clear();
    const auto phys = split_lines_keep_empty(tb->content);
    for (const auto& pl : phys) {
      std::vector<cuwacunu::iinuji::ansi::row_t> rows;
      const int wrap_width =
          tb->wrap ? std::max(1, text_w)
                   : std::max(1, static_cast<int>(pl.size()));
      cuwacunu::iinuji::ansi::style_t base{};
      cuwacunu::iinuji::ansi::hard_wrap(pl, wrap_width, base, 0, rows);
      if (rows.empty()) rows.push_back(cuwacunu::iinuji::ansi::row_t{});
      for (std::size_t i = 0; i < rows.size(); ++i) {
        lines.push_back(std::string((std::size_t)rows[i].len, 'x'));
        max_line_len = std::max(max_line_len, rows[i].len);
        if (!tb->wrap) break;
      }
    }
  } else {
    lines = tb->wrap
        ? wrap_text(tb->content, std::max(1, text_w))
        : split_lines_keep_empty(tb->content);
    max_line_len = 0;
    for (const auto& ln : lines) {
      max_line_len = std::max(max_line_len, static_cast<int>(ln.size()));
    }
  }

  out.v = static_cast<int>(lines.size()) > text_h;
  out.h = (!tb->wrap && max_line_len > text_w);
  return out;
}

inline std::optional<Rect> merged_workspace_area(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right) {
  Rect area{};
  bool have = false;
  for (const auto& box : {left, right}) {
    const Rect r = content_rect(*box);
    if (r.w <= 0 || r.h <= 0) continue;
    if (!have) {
      area = r;
      have = true;
    } else {
      area = merge_overlay_rects(area, r);
    }
  }
  if (!have) return std::nullopt;
  if (area.w < 20 || area.h < 8) return std::nullopt;
  return area;
}

inline bool close_corner_hit(const Rect& area, int mx, int my) {
  const int close_x0 = area.x + std::max(0, area.w - 4);
  const int close_x1 = close_x0 + 2;
  const int close_y = area.y;
  return my == close_y && mx >= close_x0 && mx <= close_x1;
}

inline std::optional<Rect> logs_scroll_control_area(
    const CmdState& state,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left) {
  if (state.screen != ScreenMode::ShellLogs || state.help_view)
    return std::nullopt;
  const Rect r = content_rect(*left);
  if (r.w < 4 || r.h < 3) return std::nullopt;
  return r;
}

inline bool logs_jump_top_hit(const CmdState& state,
                              const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
                              int mx,
                              int my) {
  const auto area = logs_scroll_control_area(state, left);
  if (!area.has_value()) return false;
  const int x0 = area->x + std::max(0, area->w - 3);
  const int x1 = x0 + 2;
  const int y = area->y;
  return my == y && mx >= x0 && mx <= x1;
}

inline bool logs_jump_bottom_hit(const CmdState& state,
                                 const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
                                 int mx,
                                 int my) {
  const auto area = logs_scroll_control_area(state, left);
  if (!area.has_value()) return false;
  const int x0 = area->x + std::max(0, area->w - 3);
  const int x1 = x0 + 2;
  const int y = area->y + area->h - 1;
  return my == y && mx >= x0 && mx <= x1;
}

inline std::string workspace_zoom_button_label(const CmdState& state) {
  return workspace_is_current_screen_zoomed(state) ? "[split]" : "[full]";
}

inline std::optional<Rect> workspace_zoom_button_area(
    const CmdState& state,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right) {
  if (!workspace_screen_supports_zoom(state.screen) || state.help_view) {
    return std::nullopt;
  }

  const auto& box =
      workspace_current_screen_uses_left_panel_zoom(state) ? left : right;
  if (!box || !box->visible) return std::nullopt;

  const Rect s = box->screen;
  const std::string label = workspace_zoom_button_label(state);
  if (s.w < static_cast<int>(label.size()) + 4 || s.h < 1) return std::nullopt;
  const int x = s.x + std::max(2, s.w - static_cast<int>(label.size()) - 2);
  return Rect{x, s.y, static_cast<int>(label.size()), 1};
}

inline bool workspace_zoom_button_hit(
    const CmdState& state,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right, int mx,
    int my) {
  const auto area = workspace_zoom_button_area(state, left, right);
  if (!area.has_value()) return false;
  return my == area->y && mx >= area->x && mx < area->x + area->w;
}

enum class EditorJumpHintKind : std::uint8_t {
  None = 0,
  Top = 1,
  Bottom = 2,
  Left = 3,
};

struct EditorJumpHintTarget {
  std::shared_ptr<cuwacunu::iinuji::editorBox_data_t> editor{};
  EditorJumpHintKind kind{EditorJumpHintKind::None};
};

inline std::optional<EditorJumpHintTarget> editor_jump_hint_target(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box, int mx,
    int my) {
  if (!box || !box->visible) return std::nullopt;

  for (auto it = box->children.rbegin(); it != box->children.rend(); ++it) {
    if (const auto child_target = editor_jump_hint_target(*it, mx, my);
        child_target.has_value()) {
      return child_target;
    }
  }

  auto ed = as<cuwacunu::iinuji::editorBox_data_t>(box);
  if (!ed) return std::nullopt;

  const Rect r = content_rect(*box);
  const int W = std::max(0, r.w);
  const int H = std::max(0, r.h);
  if (W <= 0 || H <= 0) return std::nullopt;

  const int header_h = (ed->show_header && H > 0) ? 1 : 0;
  const int footer_h = (ed->show_footer && H - header_h > 1) ? 1 : 0;
  const int body_y = r.y + header_h;
  const int body_h = std::max(0, H - header_h - footer_h);
  if (body_h <= 0) return std::nullopt;

  const int hint_x = r.x + (W - 1);
  if (mx != hint_x) return std::nullopt;

  const int top_hint_y = (header_h > 0) ? r.y : body_y;
  if (ed->left_col > 0 && my == top_hint_y) {
    return EditorJumpHintTarget{ed, EditorJumpHintKind::Left};
  }
  if (ed->top_line > 0 && my == body_y) {
    return EditorJumpHintTarget{ed, EditorJumpHintKind::Top};
  }
  if (ed->top_line + body_h < static_cast<int>(ed->lines.size()) &&
      my == body_y + (body_h - 1)) {
    return EditorJumpHintTarget{ed, EditorJumpHintKind::Bottom};
  }
  return std::nullopt;
}

inline bool apply_editor_jump_hint(CmdState& state,
                                   const EditorJumpHintTarget& target) {
  if (!target.editor || target.kind == EditorJumpHintKind::None) return false;

  auto& ed = *target.editor;
  ed.ensure_nonempty();
  const int body_h = std::max(1, ed.last_body_h > 0 ? ed.last_body_h : 24);

  switch (target.kind) {
    case EditorJumpHintKind::Top:
      ed.top_line = 0;
      ed.cursor_line = 0;
      ed.cursor_col = 0;
      ed.preferred_col = -1;
      if (state.screen == ScreenMode::Runtime &&
          state.runtime.log_viewer == target.editor) {
        state.runtime.log_viewer_live_follow = false;
      }
      return true;
    case EditorJumpHintKind::Bottom: {
      const int last_line = static_cast<int>(ed.lines.size()) - 1;
      ed.cursor_line = std::max(0, last_line);
      ed.cursor_col = static_cast<int>(
          ed.lines[static_cast<std::size_t>(ed.cursor_line)].size());
      ed.top_line = std::max(0, static_cast<int>(ed.lines.size()) - body_h);
      ed.preferred_col = -1;
      if (state.screen == ScreenMode::Runtime &&
          state.runtime.log_viewer == target.editor) {
        state.runtime.log_viewer_live_follow = true;
      }
      return true;
    }
    case EditorJumpHintKind::Left:
      ed.left_col = 0;
      return true;
    case EditorJumpHintKind::None:
      break;
  }
  return false;
}

inline bool help_overlay_close_hit(const CmdState& state,
                                   const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
                                   const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right,
                                   int mx,
                                   int my) {
  if (!state.help_view) return false;
  const auto area = merged_workspace_area(left, right);
  if (!area.has_value()) return false;
  return close_corner_hit(*area, mx, my);
}

inline void render_help_overlay(
    CmdState& state,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right) {
  if (!state.help_view) return;
  const auto area_opt = merged_workspace_area(left, right);
  if (!area_opt.has_value()) return;
  auto* R = get_renderer();
  if (!R) return;

  const Rect area = *area_opt;
  const short bg_pair = static_cast<short>(get_color_pair("#E8EDF5", "#11151C"));
  const short hint_pair = static_cast<short>(get_color_pair("#FFD26E", "#11151C"));

  R->fillRect(area.y, area.x, area.h, area.w, bg_pair);
  {
    constexpr const char* kClose = "[x]";
    const int close_x = area.x + std::max(0, area.w - 4);
    R->putText(area.y, close_x, kClose, 3, hint_pair, true, false);
  }

  const int inner_x = area.x + 1;
  const int inner_y = area.y + 1;
  const int inner_w = std::max(0, area.w - 2);
  const int inner_h = std::max(0, area.h - 2);
  if (inner_w <= 0 || inner_h <= 0) return;

  std::vector<std::pair<std::string, std::string>> rows;
  rows.reserve(
      12 + canonical_paths::call_help_entries().size() +
      canonical_paths::pattern_entries().size() +
      canonical_paths::alias_entries().size());

  auto push_header = [&](std::string title) {
    rows.emplace_back(std::move(title), "");
  };
  auto push_row = [&](std::string cmd, std::string comment) {
    rows.emplace_back("  " + std::move(cmd), std::move(comment));
  };

  push_header("HELP OVERLAY (auto-generated)");
  push_row("close", "Esc or click [x]");
  push_row("scroll", "Arrows, PageUp/PageDown, Home/End");
  rows.emplace_back("", "");

  push_header("IINUJI PATHS.DEF");
  push_row("scope", "Canonical UI calls, dynamic patterns, aliases");
  rows.emplace_back("", "");

  push_header("IINUJI canonical calls");
  for (const auto& entry : canonical_paths::call_help_entries()) {
    push_row(std::string(entry.first), std::string(entry.second));
  }
  rows.emplace_back("", "");

  push_header("IINUJI canonical patterns");
  for (const auto& entry : canonical_paths::pattern_entries()) {
    push_row(
        std::string(entry.text),
        std::string(canonical_paths::pattern_id_name(entry.id)) +
            (entry.summary.empty() ? "" : (" | " + std::string(entry.summary))));
  }
  rows.emplace_back("", "");

  push_header("IINUJI aliases");
  for (const auto& alias : canonical_paths::alias_entries()) {
    push_row(std::string(alias.first), std::string(alias.second));
  }
  rows.emplace_back("", "");
  push_row("note", "Primitive translation disabled. Use canonical paths or aliases.");

  int max_cmd_len = 0;
  for (const auto& row : rows) {
    max_cmd_len = std::max(max_cmd_len, static_cast<int>(row.first.size()));
  }

  const int gap = 1;
  const int min_cmd_w = 18;
  const int min_cmt_w = 20;
  const int max_cmd_share = std::max(min_cmd_w, inner_w * 40 / 100);

  int cmd_w = std::max(min_cmd_w, max_cmd_len + 1);
  cmd_w = std::min(cmd_w, max_cmd_share);
  cmd_w = std::min(cmd_w, std::max(min_cmd_w, inner_w - gap - min_cmt_w));

  int cmt_w = inner_w - cmd_w - gap;
  if (cmt_w < min_cmt_w) {
    cmt_w = std::max(1, min_cmt_w);
    cmd_w = std::max(1, inner_w - gap - cmt_w);
  }
  if (cmd_w <= 0 || cmt_w <= 0) return;

  std::ostringstream cmd_text;
  std::ostringstream cmt_text;
  std::vector<cuwacunu::iinuji::styled_text_line_t> cmd_lines;
  std::vector<cuwacunu::iinuji::styled_text_line_t> cmt_lines;
  cmd_lines.reserve(rows.size());
  cmt_lines.reserve(rows.size());

  for (const auto& row : rows) {
    const bool is_header = (!row.first.empty() && row.second.empty());
    cmd_lines.push_back(cuwacunu::iinuji::styled_text_line_t{
        .text = row.first,
        .emphasis = is_header
            ? cuwacunu::iinuji::text_line_emphasis_t::Accent
            : cuwacunu::iinuji::text_line_emphasis_t::None,
    });
    cmt_lines.push_back(cuwacunu::iinuji::styled_text_line_t{
        .text = row.second,
        .emphasis = is_header
            ? cuwacunu::iinuji::text_line_emphasis_t::Accent
            : cuwacunu::iinuji::text_line_emphasis_t::None,
    });
    cmd_text << row.first << "\n";
    cmt_text << row.second << "\n";
  }

  auto cmd_overlay = create_text_box(
      "__help_overlay_cmd__",
      cmd_text.str(),
      false,
      text_align_t::Left,
      iinuji_layout_t{},
      iinuji_style_t{"#A7D4FF", "#11151C", false, "#5E5E68"});
  cmd_overlay->screen = Rect{inner_x, inner_y, cmd_w, inner_h};
  auto cmd_tb = as<cuwacunu::iinuji::textBox_data_t>(cmd_overlay);
  if (!cmd_tb) return;
  cmd_tb->styled_lines = cmd_lines;
  cmd_tb->scroll_y = std::max(0, state.help_scroll_y);
  cmd_tb->scroll_x = std::max(0, state.help_scroll_x);
  render_text(*cmd_overlay);

  auto cmt_overlay = create_text_box(
      "__help_overlay_cmt__",
      cmt_text.str(),
      false,
      text_align_t::Left,
      iinuji_layout_t{},
      iinuji_style_t{"#95A1B5", "#11151C", false, "#5E5E68"});
  cmt_overlay->screen = Rect{inner_x + cmd_w + gap, inner_y, cmt_w, inner_h};
  auto cmt_tb = as<cuwacunu::iinuji::textBox_data_t>(cmt_overlay);
  if (!cmt_tb) return;
  cmt_tb->styled_lines = cmt_lines;
  cmt_tb->scroll_y = std::max(0, state.help_scroll_y);
  cmt_tb->scroll_x = std::max(0, state.help_scroll_x);
  render_text(*cmt_overlay);

  state.help_scroll_y = std::max(cmd_tb->scroll_y, cmt_tb->scroll_y);
  state.help_scroll_x = std::max(cmd_tb->scroll_x, cmt_tb->scroll_x);
}

inline void render_logs_scroll_controls(
    const CmdState& state,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left) {
  const auto area = logs_scroll_control_area(state, left);
  if (!area.has_value()) return;
  auto* R = get_renderer();
  if (!R) return;
  const short pair = static_cast<short>(get_color_pair("#FFD26E", "#101014"));
  const int x = area->x + std::max(0, area->w - 3);
  R->putText(area->y, x, "[^]", 3, pair, true, false);
  R->putText(area->y + area->h - 1, x, "[v]", 3, pair, true, false);
}

inline void render_workspace_zoom_controls(
    const CmdState& state,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right) {
  const auto area = workspace_zoom_button_area(state, left, right);
  if (!area.has_value()) return;
  auto* R = get_renderer();
  if (!R) return;
  const std::string bg = workspace_current_screen_uses_left_panel_zoom(state)
                             ? left->style.background_color
                             : right->style.background_color;
  const short pair = static_cast<short>(get_color_pair("#FFD26E", bg));
  const std::string label = workspace_zoom_button_label(state);
  R->putText(area->y, area->x, label, area->w, pair, true, false);
}

inline bool apply_logs_pending_actions(
    CmdState& state,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right) {
  if (state.shell_logs.pending_scroll_y == 0 &&
      state.shell_logs.pending_scroll_x == 0 &&
      !state.shell_logs.pending_jump_home &&
      !state.shell_logs.pending_jump_end) {
    return false;
  }
  if (state.screen != ScreenMode::ShellLogs) {
    state.shell_logs.pending_scroll_y = 0;
    state.shell_logs.pending_scroll_x = 0;
    state.shell_logs.pending_jump_home = false;
    state.shell_logs.pending_jump_end = false;
    return false;
  }

  auto tb = as<cuwacunu::iinuji::textBox_data_t>(find_visible_text_box(left));
  auto right_tb =
      as<cuwacunu::iinuji::textBox_data_t>(find_visible_text_box(right));
  if (state.shell_logs.pending_jump_home) {
    if (tb) {
      tb->scroll_y = 0;
      tb->scroll_x = 0;
    }
    if (right_tb) right_tb->scroll_x = 0;
    state.shell_logs.auto_follow = false;
  } else if (state.shell_logs.pending_jump_end) {
    if (tb) {
      tb->scroll_y = std::numeric_limits<int>::max();
      tb->scroll_x = 0;
    }
    if (right_tb) right_tb->scroll_x = 0;
    state.shell_logs.auto_follow = true;
  }
  if (state.shell_logs.pending_scroll_y != 0 ||
      state.shell_logs.pending_scroll_x != 0) {
    if (state.shell_logs.pending_scroll_y != 0 ||
        state.shell_logs.pending_scroll_x != 0) {
      state.shell_logs.auto_follow = false;
    }
    scroll_viewport_tree(left, state.shell_logs.pending_scroll_y,
                         state.shell_logs.pending_scroll_x);
    scroll_viewport_tree(right, state.shell_logs.pending_scroll_y,
                         state.shell_logs.pending_scroll_x);
  }
  state.shell_logs.pending_scroll_y = 0;
  state.shell_logs.pending_scroll_x = 0;
  state.shell_logs.pending_jump_home = false;
  state.shell_logs.pending_jump_end = false;
  return true;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
