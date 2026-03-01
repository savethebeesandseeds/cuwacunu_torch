#pragma once

#include <algorithm>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "iinuji/iinuji_render.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/iinuji_cmd/commands/iinuji.paths.h"
#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void scroll_text_box(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box, int dy, int dx) {
  auto tb = as<cuwacunu::iinuji::textBox_data_t>(box);
  if (!tb) return;
  tb->scroll_by(dy, dx);
}

inline void scroll_editor_box(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box, int dy, int dx) {
  auto ed = as<cuwacunu::iinuji::editorBox_data_t>(box);
  if (!ed) return;
  if (dy != 0) ed->top_line = std::max(0, ed->top_line + dy);
  if (dx != 0) ed->left_col = std::max(0, ed->left_col + dx);
}

inline void scroll_active_screen(CmdState& state,
                                 const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
                                 const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right,
                                 int dy,
                                 int dx) {
  if (dy == 0 && dx == 0) return;
  if (state.screen == ScreenMode::Logs && dy != 0) {
    state.logs.auto_follow = false;
  }
  scroll_text_box(left, dy, dx);
  scroll_text_box(right, dy, dx);
  scroll_editor_box(left, dy, dx);
  scroll_editor_box(right, dy, dx);
}

inline void jump_logs_to_bottom(CmdState& state,
                                const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left) {
  if (state.screen != ScreenMode::Logs) return;
  auto tb = as<cuwacunu::iinuji::textBox_data_t>(left);
  if (!tb) return;
  tb->scroll_y = std::numeric_limits<int>::max();
  state.logs.auto_follow = true;
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
  const Rect r = content_rect(*box);
  const int W = std::max(0, r.w);
  const int H = std::max(0, r.h);
  if (W <= 0 || H <= 0) return out;

  if (const auto ed = as<cuwacunu::iinuji::editorBox_data_t>(box); ed) {
    ed->ensure_nonempty();
    const int body_h = std::max(1, H - 1);
    const int total_lines = static_cast<int>(ed->lines.size());
    int max_line_len = 0;
    for (const auto& ln : ed->lines) max_line_len = std::max(max_line_len, static_cast<int>(ln.size()));

    const int ln_w = std::max(3, std::min(W, digits10_i(std::max(1, total_lines)) + 2));
    const int text_w = std::max(1, W - ln_w);
    out.v = total_lines > body_h;
    out.h = max_line_len > text_w;
    return out;
  }

  auto tb = as<cuwacunu::iinuji::textBox_data_t>(box);
  if (!tb) return out;

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

    lines = tb->wrap
        ? wrap_text(tb->content, std::max(1, text_w))
        : split_lines_keep_empty(tb->content);
    max_line_len = 0;
    for (const auto& ln : lines) max_line_len = std::max(max_line_len, static_cast<int>(ln.size()));

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

  lines = tb->wrap
      ? wrap_text(tb->content, std::max(1, text_w))
      : split_lines_keep_empty(tb->content);
  max_line_len = 0;
  for (const auto& ln : lines) max_line_len = std::max(max_line_len, static_cast<int>(ln.size()));

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
  if (state.screen != ScreenMode::Logs || state.help_view) return std::nullopt;
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
  auto lower_ascii = [](std::string s) {
    for (char& c : s) {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
  };
  auto dir_from_macro = [&](const char* dir) {
    return lower_ascii(std::string(dir));
  };
  auto policy_from_macro = [&](const char* p) {
    return lower_ascii(std::string(p));
  };

  push_header("HELP OVERLAY (auto-generated)");
  push_row("close", "Esc or click [x]");
  push_row("scroll", "Arrows, PageUp/PageDown, Home/End");
  rows.emplace_back("", "");

  push_header("BOARD paths.def");
  push_row("scope", "Board control directives, methods, actions, contract DSL segments");
  rows.emplace_back("", "");

  push_header("BOARD directives");
#define BOARD_PATH_DIRECTIVE(ID, TOKEN, SUMMARY) push_row(std::string("directive ") + TOKEN, SUMMARY);
#define BOARD_PATH_METHOD(ID, TOKEN, SUMMARY)
#define BOARD_PATH_ACTION(ID, TOKEN, SUMMARY)
#define BOARD_PATH_DSL_SEGMENT(ID, KEY, SUMMARY)
#include "iitepi/board/board.paths.def"
#undef BOARD_PATH_DSL_SEGMENT
#undef BOARD_PATH_ACTION
#undef BOARD_PATH_METHOD
#undef BOARD_PATH_DIRECTIVE
  rows.emplace_back("", "");

  push_header("BOARD methods");
#define BOARD_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define BOARD_PATH_METHOD(ID, TOKEN, SUMMARY) push_row(std::string("method ") + TOKEN, SUMMARY);
#define BOARD_PATH_ACTION(ID, TOKEN, SUMMARY)
#define BOARD_PATH_DSL_SEGMENT(ID, KEY, SUMMARY)
#include "iitepi/board/board.paths.def"
#undef BOARD_PATH_DSL_SEGMENT
#undef BOARD_PATH_ACTION
#undef BOARD_PATH_METHOD
#undef BOARD_PATH_DIRECTIVE
  rows.emplace_back("", "");

  push_header("BOARD actions");
#define BOARD_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define BOARD_PATH_METHOD(ID, TOKEN, SUMMARY)
#define BOARD_PATH_ACTION(ID, TOKEN, SUMMARY) push_row(std::string("action ") + TOKEN, SUMMARY);
#define BOARD_PATH_DSL_SEGMENT(ID, KEY, SUMMARY)
#include "iitepi/board/board.paths.def"
#undef BOARD_PATH_DSL_SEGMENT
#undef BOARD_PATH_ACTION
#undef BOARD_PATH_METHOD
#undef BOARD_PATH_DIRECTIVE
  rows.emplace_back("", "");

  push_header("BOARD contract DSL segments");
#define BOARD_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define BOARD_PATH_METHOD(ID, TOKEN, SUMMARY)
#define BOARD_PATH_ACTION(ID, TOKEN, SUMMARY)
#define BOARD_PATH_DSL_SEGMENT(ID, KEY, SUMMARY) push_row(std::string("dsl ") + KEY, SUMMARY);
#include "iitepi/board/board.paths.def"
#undef BOARD_PATH_DSL_SEGMENT
#undef BOARD_PATH_ACTION
#undef BOARD_PATH_METHOD
#undef BOARD_PATH_DIRECTIVE
  rows.emplace_back("", "");

  push_header("TSI PATHS.DEF");
  push_row("scope", "TSI directives, methods, components, lanes, endpoints");
  rows.emplace_back("", "");

  push_header("TSI directives");
#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY) push_row(std::string("directive ") + TOKEN, SUMMARY);
#define TSI_PATH_METHOD(ID, TOKEN, SUMMARY)
#define TSI_PATH_COMPONENT(TYPE_ID, CANONICAL, DOMAIN, INSTANCE_POLICY, SUMMARY)
#define TSI_PATH_LANE(TYPE_ID, DIR, DIRECTIVE_ID, KIND, SUMMARY)
#define TSI_PATH_ENDPOINT(TYPE_ID, DIRECTIVE_ID, KIND, SUMMARY)
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_ENDPOINT
#undef TSI_PATH_LANE
#undef TSI_PATH_COMPONENT
#undef TSI_PATH_METHOD
#undef TSI_PATH_DIRECTIVE
  rows.emplace_back("", "");

  push_header("TSI methods");
#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define TSI_PATH_METHOD(ID, TOKEN, SUMMARY) push_row(std::string("method ") + TOKEN, SUMMARY);
#define TSI_PATH_COMPONENT(TYPE_ID, CANONICAL, DOMAIN, INSTANCE_POLICY, SUMMARY)
#define TSI_PATH_LANE(TYPE_ID, DIR, DIRECTIVE_ID, KIND, SUMMARY)
#define TSI_PATH_ENDPOINT(TYPE_ID, DIRECTIVE_ID, KIND, SUMMARY)
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_ENDPOINT
#undef TSI_PATH_LANE
#undef TSI_PATH_COMPONENT
#undef TSI_PATH_METHOD
#undef TSI_PATH_DIRECTIVE
  rows.emplace_back("", "");

  push_header("TSI components");
#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define TSI_PATH_METHOD(ID, TOKEN, SUMMARY)
#define TSI_PATH_COMPONENT(TYPE_ID, CANONICAL, DOMAIN, INSTANCE_POLICY, SUMMARY) \
  push_row(                                                                        \
      std::string("component ") + CANONICAL,                                      \
      std::string("domain=") + lower_ascii(std::string(#DOMAIN)) +               \
          " policy=" + policy_from_macro(#INSTANCE_POLICY) + " | " + SUMMARY);
#define TSI_PATH_LANE(TYPE_ID, DIR, DIRECTIVE_ID, KIND, SUMMARY)
#define TSI_PATH_ENDPOINT(TYPE_ID, DIRECTIVE_ID, KIND, SUMMARY)
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_ENDPOINT
#undef TSI_PATH_LANE
#undef TSI_PATH_COMPONENT
#undef TSI_PATH_METHOD
#undef TSI_PATH_DIRECTIVE
  rows.emplace_back("", "");

  push_header("TSI lanes");
#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define TSI_PATH_METHOD(ID, TOKEN, SUMMARY)
#define TSI_PATH_COMPONENT(TYPE_ID, CANONICAL, DOMAIN, INSTANCE_POLICY, SUMMARY)
#define TSI_PATH_LANE(TYPE_ID, DIR, DIRECTIVE_ID, KIND, SUMMARY)                           \
  push_row(                                                                                 \
      std::string("lane ") + #TYPE_ID + " " + dir_from_macro(#DIR) + " " +                \
          std::string(::tsiemene::directive_id::DIRECTIVE_ID) +                            \
          std::string(::tsiemene::kind_token(::tsiemene::PayloadKind::KIND)),              \
      SUMMARY);
#define TSI_PATH_ENDPOINT(TYPE_ID, DIRECTIVE_ID, KIND, SUMMARY)
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_ENDPOINT
#undef TSI_PATH_LANE
#undef TSI_PATH_COMPONENT
#undef TSI_PATH_METHOD
#undef TSI_PATH_DIRECTIVE
  rows.emplace_back("", "");

  push_header("TSI endpoints");
#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define TSI_PATH_METHOD(ID, TOKEN, SUMMARY)
#define TSI_PATH_COMPONENT(TYPE_ID, CANONICAL, DOMAIN, INSTANCE_POLICY, SUMMARY)
#define TSI_PATH_LANE(TYPE_ID, DIR, DIRECTIVE_ID, KIND, SUMMARY)
#define TSI_PATH_ENDPOINT(TYPE_ID, DIRECTIVE_ID, KIND, SUMMARY)                            \
  push_row(                                                                                 \
      std::string("endpoint ") + #TYPE_ID + " " + std::string(::tsiemene::directive_id::DIRECTIVE_ID) + \
          std::string(::tsiemene::kind_token(::tsiemene::PayloadKind::KIND)),              \
      SUMMARY);
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_ENDPOINT
#undef TSI_PATH_LANE
#undef TSI_PATH_COMPONENT
#undef TSI_PATH_METHOD
#undef TSI_PATH_DIRECTIVE
  rows.emplace_back("", "");
  push_header("================================================================================");
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

inline bool apply_logs_pending_actions(
    CmdState& state,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right) {
  if (state.logs.pending_scroll_y == 0 &&
      state.logs.pending_scroll_x == 0 &&
      !state.logs.pending_jump_home &&
      !state.logs.pending_jump_end) {
    return false;
  }
  if (state.screen != ScreenMode::Logs) {
    state.logs.pending_scroll_y = 0;
    state.logs.pending_scroll_x = 0;
    state.logs.pending_jump_home = false;
    state.logs.pending_jump_end = false;
    return false;
  }

  auto tb = as<cuwacunu::iinuji::textBox_data_t>(left);
  if (state.logs.pending_jump_home) {
    if (tb) {
      tb->scroll_y = 0;
      tb->scroll_x = 0;
    }
    state.logs.auto_follow = false;
  } else if (state.logs.pending_jump_end) {
    if (tb) tb->scroll_y = std::numeric_limits<int>::max();
    state.logs.auto_follow = true;
  }
  if (state.logs.pending_scroll_y != 0 || state.logs.pending_scroll_x != 0) {
    if (state.logs.pending_scroll_y != 0) state.logs.auto_follow = false;
    scroll_text_box(left, state.logs.pending_scroll_y, state.logs.pending_scroll_x);
    scroll_text_box(right, state.logs.pending_scroll_y, state.logs.pending_scroll_x);
  }
  state.logs.pending_scroll_y = 0;
  state.logs.pending_scroll_x = 0;
  state.logs.pending_jump_home = false;
  state.logs.pending_jump_end = false;
  return true;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
