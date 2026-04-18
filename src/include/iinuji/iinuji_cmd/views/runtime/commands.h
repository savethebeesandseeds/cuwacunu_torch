#pragma once

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "iinuji/iinuji_ansi.h"
#include "iinuji/iinuji_cmd/views/common.h"
#include "iinuji/iinuji_cmd/views/workbench/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void set_runtime_status(CmdState &st, std::string status, bool is_error);

inline bool runtime_session_is_active(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return !cuwacunu::hero::marshal::is_marshal_session_summary_state(session);
}

inline std::string runtime_session_scope_label(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return runtime_session_is_active(session) ? "active" : "historic";
}

inline std::string runtime_session_state_text(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::ostringstream oss;
  oss << session.lifecycle;
  if (session.lifecycle == "live" && !session.work_gate.empty() &&
      session.work_gate != "open") {
    oss << " (" << session.work_gate << ")";
  } else if (session.lifecycle == "terminal" &&
             !session.finish_reason.empty() &&
             session.finish_reason != "none") {
    oss << " (" << session.finish_reason << ")";
  } else if (session.lifecycle == "live" && !session.activity.empty()) {
    oss << " (" << session.activity << ")";
  }
  return oss.str();
}

inline bool runtime_campaign_is_active(
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign) {
  return campaign.state == "launching" || campaign.state == "running" ||
         campaign.state == "stopping";
}

inline std::string runtime_campaign_scope_label(
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign) {
  return runtime_campaign_is_active(campaign) ? "active" : "historic";
}

inline bool runtime_job_is_active(
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  return job.state == "launching" || job.state == "running" ||
         job.state == "stopping" || job.state == "orphaned";
}

inline std::string runtime_job_scope_label(
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  return runtime_job_is_active(job) ? "active" : "historic";
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
find_runtime_session_by_id(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        &sessions,
    std::string_view marshal_session_id) {
  for (const auto &session : sessions) {
    if (session.marshal_session_id == marshal_session_id)
      return session;
  }
  return std::nullopt;
}

inline std::optional<std::size_t> runtime_find_session_index(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        &sessions,
    std::string_view marshal_session_id) {
  for (std::size_t i = 0; i < sessions.size(); ++i) {
    if (sessions[i].marshal_session_id == marshal_session_id)
      return i;
  }
  return std::nullopt;
}

inline std::optional<std::size_t> runtime_find_campaign_index(
    const std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t>
        &campaigns,
    std::string_view campaign_cursor) {
  for (std::size_t i = 0; i < campaigns.size(); ++i) {
    if (campaigns[i].campaign_cursor == campaign_cursor)
      return i;
  }
  return std::nullopt;
}

inline std::optional<std::size_t> runtime_find_job_index(
    const std::vector<cuwacunu::hero::runtime::runtime_job_record_t> &jobs,
    std::string_view job_cursor) {
  for (std::size_t i = 0; i < jobs.size(); ++i) {
    if (jobs[i].job_cursor == job_cursor)
      return i;
  }
  return std::nullopt;
}

inline std::optional<cuwacunu::hero::runtime::runtime_campaign_record_t>
find_runtime_campaign_by_cursor(
    const std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t>
        &campaigns,
    std::string_view campaign_cursor) {
  for (const auto &campaign : campaigns) {
    if (campaign.campaign_cursor == campaign_cursor)
      return campaign;
  }
  return std::nullopt;
}

inline std::optional<cuwacunu::hero::runtime::runtime_job_record_t>
find_runtime_job_by_cursor(
    const std::vector<cuwacunu::hero::runtime::runtime_job_record_t> &jobs,
    std::string_view job_cursor) {
  for (const auto &job : jobs) {
    if (job.job_cursor == job_cursor)
      return job;
  }
  return std::nullopt;
}

inline std::string runtime_row_kind_label(RuntimeRowKind kind) {
  switch (kind) {
  case RuntimeRowKind::Device:
    return "device";
  case RuntimeRowKind::Session:
    return "session";
  case RuntimeRowKind::Campaign:
    return "campaign";
  case RuntimeRowKind::Job:
    return "job";
  }
  return "session";
}

inline std::size_t runtime_lane_index(RuntimeLane lane) {
  if (runtime_is_device_lane(lane))
    return 0;
  if (runtime_is_session_lane(lane))
    return 1;
  if (runtime_is_campaign_lane(lane))
    return 2;
  if (runtime_is_job_lane(lane))
    return 3;
  return 0;
}

inline RuntimeLane runtime_lane_from_index(std::size_t index) {
  switch (index) {
  case 0:
    return kRuntimeDeviceLane;
  case 1:
    return kRuntimeSessionLane;
  case 2:
    return kRuntimeCampaignLane;
  case 3:
    return kRuntimeJobLane;
  default:
    return kRuntimeDeviceLane;
  }
}

inline constexpr std::string_view kRuntimeNoneSessionSelection =
    "::runtime:none:session::";
inline constexpr std::string_view kRuntimeNoneCampaignSelection =
    "::runtime:none:campaign::";

inline bool runtime_is_none_session_selection(std::string_view value) {
  return value == kRuntimeNoneSessionSelection;
}

inline bool runtime_is_none_campaign_selection(std::string_view value) {
  return value == kRuntimeNoneCampaignSelection;
}

inline std::string runtime_none_branch_label() { return "<none>"; }

inline std::string runtime_log_viewer_kind_label(RuntimeLogViewerKind kind) {
  switch (kind) {
  case RuntimeLogViewerKind::JobStdout:
    return "job stdout";
  case RuntimeLogViewerKind::JobStderr:
    return "job stderr";
  case RuntimeLogViewerKind::MarshalEvents:
    return "marshal events";
  case RuntimeLogViewerKind::MarshalCodexStdout:
    return "codex stdout";
  case RuntimeLogViewerKind::MarshalCodexStderr:
    return "codex stderr";
  case RuntimeLogViewerKind::CampaignStdout:
    return "campaign stdout";
  case RuntimeLogViewerKind::CampaignStderr:
    return "campaign stderr";
  case RuntimeLogViewerKind::JobTrace:
    return "job trace";
  case RuntimeLogViewerKind::ArtifactFile:
    return "artifact";
  case RuntimeLogViewerKind::None:
    break;
  }
  return "log";
}

inline bool runtime_log_viewer_kind_supports_follow(RuntimeLogViewerKind kind) {
  switch (kind) {
  case RuntimeLogViewerKind::JobStdout:
  case RuntimeLogViewerKind::JobStderr:
  case RuntimeLogViewerKind::MarshalEvents:
  case RuntimeLogViewerKind::MarshalCodexStdout:
  case RuntimeLogViewerKind::MarshalCodexStderr:
  case RuntimeLogViewerKind::CampaignStdout:
  case RuntimeLogViewerKind::CampaignStderr:
  case RuntimeLogViewerKind::JobTrace:
    return true;
  case RuntimeLogViewerKind::ArtifactFile:
  case RuntimeLogViewerKind::None:
    return false;
  }
  return false;
}

inline bool
runtime_log_viewer_kind_uses_structured_view(RuntimeLogViewerKind kind) {
  switch (kind) {
  case RuntimeLogViewerKind::MarshalEvents:
  case RuntimeLogViewerKind::MarshalCodexStdout:
  case RuntimeLogViewerKind::MarshalCodexStderr:
    return true;
  case RuntimeLogViewerKind::JobStdout:
  case RuntimeLogViewerKind::JobStderr:
  case RuntimeLogViewerKind::CampaignStdout:
  case RuntimeLogViewerKind::CampaignStderr:
  case RuntimeLogViewerKind::JobTrace:
  case RuntimeLogViewerKind::ArtifactFile:
  case RuntimeLogViewerKind::None:
    return false;
  }
  return false;
}

inline bool runtime_event_viewer_is_open(const CmdState &st) {
  return st.runtime.log_viewer_open &&
         runtime_log_viewer_kind_uses_structured_view(
             st.runtime.log_viewer_kind) &&
         !st.runtime.log_viewer_path.empty();
}

inline bool runtime_text_log_viewer_is_open(const CmdState &st) {
  return st.runtime.log_viewer_open && st.runtime.log_viewer != nullptr;
}

inline bool runtime_log_viewer_is_open(const CmdState &st) {
  return st.runtime.log_viewer_open &&
         (st.runtime.log_viewer != nullptr || runtime_event_viewer_is_open(st));
}

inline std::size_t runtime_event_viewer_entry_count(const CmdState &st);
inline bool runtime_event_viewer_has_entries(const CmdState &st);
inline bool runtime_event_viewer_is_at_end(const CmdState &st);
inline void runtime_snap_event_viewer_to_end(CmdState &st);

inline std::string runtime_log_viewer_title(const CmdState &st) {
  if (!runtime_log_viewer_is_open(st))
    return {};
  return st.runtime.log_viewer_title.empty()
             ? runtime_log_viewer_kind_label(st.runtime.log_viewer_kind)
             : st.runtime.log_viewer_title;
}

inline bool runtime_log_viewer_supports_follow(const CmdState &st) {
  return runtime_log_viewer_kind_supports_follow(st.runtime.log_viewer_kind);
}

inline void clear_runtime_log_viewer(CmdState &st) {
  st.runtime.log_viewer_open = false;
  st.runtime.log_viewer_kind = RuntimeLogViewerKind::None;
  st.runtime.log_viewer_path.clear();
  st.runtime.log_viewer_title.clear();
  st.runtime.log_viewer.reset();
  st.runtime.marshal_event_viewer.entries.clear();
  st.runtime.marshal_event_viewer.selected_entry = 0;
  st.runtime.log_viewer_live_follow = true;
  st.runtime.log_viewer_last_poll_ms = 0;
  st.runtime.log_viewer_last_size = 0;
  st.runtime.log_viewer_last_write_tick = 0;
}

inline void clear_runtime_job_excerpts(CmdState &st) {
  st.runtime.job_stdout_excerpt.clear();
  st.runtime.job_stderr_excerpt.clear();
  st.runtime.job_trace_excerpt.clear();
}

struct RuntimeLogViewerFileStamp {
  bool ok{false};
  std::uintmax_t size{0};
  std::int64_t write_tick{0};
};

inline RuntimeLogViewerFileStamp
runtime_probe_log_viewer_file(const std::string &path) {
  RuntimeLogViewerFileStamp stamp{};
  if (path.empty())
    return stamp;

  std::error_code ec{};
  const std::filesystem::path fs_path(path);
  if (!std::filesystem::exists(fs_path, ec) || ec)
    return stamp;

  ec.clear();
  stamp.size = std::filesystem::file_size(fs_path, ec);
  if (ec)
    return RuntimeLogViewerFileStamp{};

  ec.clear();
  const auto last_write = std::filesystem::last_write_time(fs_path, ec);
  if (ec)
    return RuntimeLogViewerFileStamp{};

  stamp.ok = true;
  stamp.write_tick = static_cast<std::int64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          last_write.time_since_epoch())
          .count());
  return stamp;
}

inline void
runtime_record_log_viewer_stamp(CmdState &st,
                                const RuntimeLogViewerFileStamp &stamp) {
  if (!stamp.ok)
    return;
  st.runtime.log_viewer_last_size = stamp.size;
  st.runtime.log_viewer_last_write_tick = stamp.write_tick;
}

inline bool
runtime_log_viewer_stamp_matches(const CmdState &st,
                                 const RuntimeLogViewerFileStamp &stamp) {
  return stamp.ok && stamp.size == st.runtime.log_viewer_last_size &&
         stamp.write_tick == st.runtime.log_viewer_last_write_tick;
}

inline void
runtime_snap_log_viewer_to_end(cuwacunu::iinuji::editorBox_data_t &editor) {
  editor.ensure_nonempty();
  const int last_line = static_cast<int>(editor.lines.size()) - 1;
  const int body_h =
      std::max(1, editor.last_body_h > 0 ? editor.last_body_h : 24);
  editor.cursor_line = std::max(0, last_line);
  editor.cursor_col = static_cast<int>(
      editor.lines[static_cast<std::size_t>(editor.cursor_line)].size());
  editor.top_line = std::max(0, static_cast<int>(editor.lines.size()) - body_h);
  editor.left_col = 0;
  editor.preferred_col = -1;
  editor.ensure_nonempty();
}

inline void runtime_restore_log_viewer_viewport(
    cuwacunu::iinuji::editorBox_data_t &dst,
    const cuwacunu::iinuji::editorBox_data_t &src) {
  dst.ensure_nonempty();
  const int last_line = static_cast<int>(dst.lines.size()) - 1;
  dst.cursor_line = std::clamp(src.cursor_line, 0, std::max(0, last_line));
  dst.cursor_col = std::clamp(
      src.cursor_col, 0,
      static_cast<int>(
          dst.lines[static_cast<std::size_t>(dst.cursor_line)].size()));
  dst.preferred_col = src.preferred_col;
  dst.top_line = std::max(0, src.top_line);
  dst.left_col = std::max(0, src.left_col);
  dst.last_body_h = std::max(dst.last_body_h, src.last_body_h);
  dst.last_lineno_w = src.last_lineno_w;
  dst.last_text_w = src.last_text_w;
  const int max_top = std::max(0, static_cast<int>(dst.lines.size()) -
                                      std::max(1, dst.last_body_h));
  dst.top_line = std::min(dst.top_line, max_top);
  dst.ensure_nonempty();
}

inline bool
runtime_log_viewer_is_at_end(const cuwacunu::iinuji::editorBox_data_t &editor) {
  if (editor.lines.empty())
    return true;
  const int last_line = static_cast<int>(editor.lines.size()) - 1;
  const int body_h = std::max(1, editor.last_body_h);
  return editor.cursor_line >= last_line &&
         editor.top_line + body_h >= static_cast<int>(editor.lines.size());
}

inline void runtime_refresh_log_viewer_follow_state(CmdState &st) {
  if (!runtime_text_log_viewer_is_open(st) || !st.runtime.log_viewer)
    return;
  st.runtime.log_viewer_live_follow =
      runtime_log_viewer_is_at_end(*st.runtime.log_viewer);
}

inline void runtime_keep_log_viewer_following(CmdState &st) {
  if (!runtime_log_viewer_is_open(st) ||
      !runtime_log_viewer_supports_follow(st) ||
      !st.runtime.log_viewer_live_follow) {
    return;
  }
  if (runtime_event_viewer_is_open(st)) {
    runtime_snap_event_viewer_to_end(st);
    return;
  }
  if (st.runtime.log_viewer) {
    runtime_snap_log_viewer_to_end(*st.runtime.log_viewer);
  }
}

struct RuntimeAnsiColorSegment {
  std::size_t begin{0};
  std::size_t end{0};
  std::string fg{};
  std::string bg{};
};

inline void runtime_parse_ansi_editor_line(
    const std::string &raw_line, std::string *out_text,
    std::vector<RuntimeAnsiColorSegment> *out_segments) {
  if (out_text)
    out_text->clear();
  if (out_segments)
    out_segments->clear();

  cuwacunu::iinuji::ansi::style_t base{};
  cuwacunu::iinuji::ansi::style_t st = base;
  cuwacunu::iinuji::ansi::style_t run_style = st;
  std::size_t run_begin = 0;
  bool run_active = false;

  auto flush_run = [&]() {
    if (!run_active || out_text == nullptr || out_segments == nullptr)
      return;
    if (!run_style.fg.empty() || !run_style.bg.empty()) {
      out_segments->push_back(RuntimeAnsiColorSegment{
          .begin = run_begin,
          .end = out_text->size(),
          .fg = run_style.fg,
          .bg = run_style.bg,
      });
    }
    run_active = false;
  };

  std::size_t i = 0;
  while (i < raw_line.size()) {
    const unsigned char c = static_cast<unsigned char>(raw_line[i]);
    if (c == 0x1b && i + 1 < raw_line.size() && raw_line[i + 1] == '[') {
      flush_run();
      std::vector<int> params{};
      char final = 0;
      std::size_t next_i = i;
      if (cuwacunu::iinuji::ansi::parse_csi(raw_line, i, next_i, params,
                                            final)) {
        if (final == 'm') {
          cuwacunu::iinuji::ansi::apply_sgr(params, st, base);
        }
        i = next_i;
        continue;
      }
      ++i;
      continue;
    }
    if (c == '\r' || c < 32) {
      ++i;
      continue;
    }

    if (out_text != nullptr) {
      if (!run_active) {
        run_style = st;
        run_begin = out_text->size();
        run_active = true;
      } else if (run_style.fg != st.fg || run_style.bg != st.bg) {
        flush_run();
        run_style = st;
        run_begin = out_text->size();
        run_active = true;
      }
      out_text->push_back(static_cast<char>(c));
    }
    ++i;
  }

  flush_run();
}

inline std::shared_ptr<cuwacunu::iinuji::editorBox_data_t>
runtime_build_log_viewer_editor(const std::string &path, std::string *error) {
  std::string raw{};
  if (!read_text_file_safe(path, &raw, error))
    return {};

  auto editor = std::make_shared<cuwacunu::iinuji::editorBox_data_t>(path);
  editor->show_header = true;
  editor->show_footer = false;
  editor->show_line_numbers = true;
  editor->show_tabs = false;
  editor->highlight_current_line = true;
  editor->highlight_matching_delimiter = false;
  editor->history_enabled = false;
  editor->read_only = true;
  editor->last_body_h = 24;

  auto color_lines =
      std::make_shared<std::vector<std::vector<RuntimeAnsiColorSegment>>>();
  std::vector<std::string> visible_lines{};
  const auto physical_lines = split_lines_keep_empty(raw);
  visible_lines.reserve(physical_lines.size());
  color_lines->reserve(physical_lines.size());
  for (const auto &raw_line : physical_lines) {
    std::string visible{};
    std::vector<RuntimeAnsiColorSegment> segments{};
    runtime_parse_ansi_editor_line(raw_line, &visible, &segments);
    visible_lines.push_back(std::move(visible));
    color_lines->push_back(std::move(segments));
  }

  std::ostringstream visible_text{};
  for (std::size_t i = 0; i < visible_lines.size(); ++i) {
    if (i > 0)
      visible_text << '\n';
    visible_text << visible_lines[i];
  }
  cuwacunu::iinuji::primitives::editor_set_text(*editor, visible_text.str());
  editor->dirty = false;
  editor->close_armed = false;
  editor->close_armed_via_escape = false;
  editor->status = "read only log viewer";
  editor->line_colorizer = [color_lines](
                               const cuwacunu::iinuji::editorBox_data_t &,
                               int line_index, const std::string &line,
                               std::vector<short> &out_colors,
                               short row_base_pair, const std::string &row_bg) {
    if (line_index < 0 || line_index >= static_cast<int>(color_lines->size())) {
      return;
    }
    const auto &segments = (*color_lines)[static_cast<std::size_t>(line_index)];
    if (segments.empty())
      return;
    for (const auto &segment : segments) {
      const std::size_t begin = std::min(segment.begin, line.size());
      const std::size_t end = std::min(segment.end, line.size());
      if (begin >= end)
        continue;
      const std::string fg =
          segment.fg.empty() ? std::string("#D8D8D8") : segment.fg;
      const std::string bg = segment.bg.empty() ? row_bg : segment.bg;
      short pair = static_cast<short>(get_color_pair(fg, bg));
      if (pair == 0)
        pair = row_base_pair;
      for (std::size_t i = begin; i < end; ++i) {
        out_colors[i] = pair;
      }
    }
  };
  return editor;
}

inline std::vector<std::string>
runtime_tail_text_file_lines(const std::string &path, std::size_t max_lines) {
  std::vector<std::string> out{};
  if (path.empty() || max_lines == 0)
    return out;

  std::ifstream input(path);
  if (!input.is_open())
    return out;

  out.reserve(max_lines);
  std::string line{};
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (out.size() == max_lines) {
      out.erase(out.begin());
    }
    out.push_back(line);
  }
  return out;
}

inline bool
runtime_try_parse_event_json_object_line(const std::string &raw_line,
                                         cmd_json_value_t *out) {
  if (out == nullptr)
    return false;
  *out = cmd_json_value_t{};
  try {
    *out = cuwacunu::piaabo::JsonParser(raw_line).parse();
  } catch (...) {
    return false;
  }
  return out->type == cmd_json_type_t::OBJECT;
}

inline std::string runtime_event_json_first_string(
    const cmd_json_value_t *object, std::string_view key_a,
    std::string_view key_b = {}, std::string_view key_c = {},
    std::string_view key_d = {}, std::string_view key_e = {}) {
  const std::string_view keys[] = {key_a, key_b, key_c, key_d, key_e};
  for (const auto key : keys) {
    if (key.empty())
      continue;
    const std::string value = cmd_json_string(cmd_json_field(object, key));
    if (!value.empty())
      return value;
  }
  return {};
}

inline const cmd_json_value_t *
runtime_event_json_find_value_deep(const cmd_json_value_t *value,
                                   std::string_view key, int depth = 2) {
  if (value == nullptr || key.empty() || depth < 0)
    return nullptr;
  if (value->type == cmd_json_type_t::OBJECT && value->objectValue != nullptr) {
    if (const auto *direct = cmd_json_field(value, key); direct != nullptr)
      return direct;
    static const std::string_view preferred_children[] = {
        "item",     "message", "delta", "payload", "data", "result",
        "response", "error",   "usage", "content", "call", "tool_call"};
    for (const auto child_key : preferred_children) {
      if (const auto *child = cmd_json_field(value, child_key);
          child != nullptr && (child->type == cmd_json_type_t::OBJECT ||
                               child->type == cmd_json_type_t::ARRAY)) {
        if (const auto *match =
                runtime_event_json_find_value_deep(child, key, depth - 1);
            match != nullptr) {
          return match;
        }
      }
    }
    for (const auto &kv : *value->objectValue) {
      if (kv.second.type != cmd_json_type_t::OBJECT &&
          kv.second.type != cmd_json_type_t::ARRAY) {
        continue;
      }
      if (const auto *match =
              runtime_event_json_find_value_deep(&kv.second, key, depth - 1);
          match != nullptr) {
        return match;
      }
    }
    return nullptr;
  }
  if (value->type == cmd_json_type_t::ARRAY && value->arrayValue != nullptr) {
    const std::size_t limit =
        std::min<std::size_t>(value->arrayValue->size(), 6);
    for (std::size_t i = 0; i < limit; ++i) {
      if (const auto *match = runtime_event_json_find_value_deep(
              &(*value->arrayValue)[i], key, depth - 1);
          match != nullptr) {
        return match;
      }
    }
  }
  return nullptr;
}

inline std::string runtime_event_json_first_string_deep(
    const cmd_json_value_t *object, std::string_view key_a,
    std::string_view key_b = {}, std::string_view key_c = {},
    std::string_view key_d = {}, std::string_view key_e = {}) {
  const std::string_view keys[] = {key_a, key_b, key_c, key_d, key_e};
  for (const auto key : keys) {
    if (key.empty())
      continue;
    const auto *value = runtime_event_json_find_value_deep(object, key);
    if (value == nullptr)
      continue;
    if (value->type == cmd_json_type_t::STRING && !value->stringValue.empty()) {
      return value->stringValue;
    }
  }
  return {};
}

inline std::string
runtime_event_json_value_text(const cmd_json_value_t *value) {
  if (value == nullptr)
    return {};
  switch (value->type) {
  case cmd_json_type_t::STRING:
    return value->stringValue;
  case cmd_json_type_t::BOOLEAN:
    return value->boolValue ? "true" : "false";
  case cmd_json_type_t::NUMBER: {
    const double number = value->numberValue;
    const auto as_i64 = static_cast<std::int64_t>(number);
    if (static_cast<double>(as_i64) == number) {
      return std::to_string(as_i64);
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << number;
    std::string text = oss.str();
    while (!text.empty() && text.back() == '0')
      text.pop_back();
    if (!text.empty() && text.back() == '.')
      text.pop_back();
    return text;
  }
  case cmd_json_type_t::NULL_TYPE:
    return "null";
  case cmd_json_type_t::ARRAY:
    if (value->arrayValue == nullptr)
      return "[]";
    {
      std::vector<std::string> parts{};
      for (const auto &entry : *value->arrayValue) {
        const std::string item = runtime_event_json_value_text(&entry);
        if (item.empty())
          continue;
        parts.push_back(item);
        if (parts.size() >= 4)
          break;
      }
      if (parts.empty())
        return "[]";
      return "[" + join_trimmed_values(parts, 4, 40) + "]";
    }
  case cmd_json_type_t::OBJECT: {
    const std::string nested = runtime_event_json_first_string_deep(
        value, "text", "message", "summary", "detail", "status");
    if (!nested.empty())
      return "{" + trim_to_width(nested, 40) + "}";
    return "{...}";
  }
  }
  return {};
}

inline std::optional<std::uint64_t>
runtime_event_json_u64_deep(const cmd_json_value_t *object,
                            std::string_view key) {
  const auto *value = runtime_event_json_find_value_deep(object, key);
  if (value == nullptr)
    return std::nullopt;
  if (value->type == cmd_json_type_t::NUMBER && value->numberValue >= 0.0) {
    return static_cast<std::uint64_t>(value->numberValue);
  }
  if (value->type == cmd_json_type_t::STRING) {
    const std::string text = trim_copy(value->stringValue);
    if (text.empty())
      return std::nullopt;
    std::uint64_t parsed = 0;
    std::istringstream iss(text);
    iss >> parsed;
    if (iss && iss.eof())
      return parsed;
  }
  return std::nullopt;
}

inline std::uint64_t
runtime_event_json_timestamp_ms(const cmd_json_value_t *object) {
  const std::string_view keys[] = {"timestamp_ms",
                                   "ts_ms",
                                   "updated_at_ms",
                                   "started_at_ms",
                                   "finished_at_ms",
                                   "timestamp",
                                   "ts"};
  for (const auto key : keys) {
    const auto value = runtime_event_json_u64_deep(object, key);
    if (!value.has_value() || *value == 0)
      continue;
    if (key == "timestamp" || key == "ts") {
      if (*value >= 1000000000000ULL)
        return *value;
      if (*value >= 1000000000ULL)
        return *value * 1000ULL;
    }
    return *value;
  }
  return 0;
}

inline cuwacunu::iinuji::text_line_emphasis_t
runtime_marshaled_event_emphasis_from_name(std::string_view event_name) {
  const std::string key = to_lower_copy(std::string(event_name));
  if (key.find("fail") != std::string::npos ||
      key.find("error") != std::string::npos ||
      key.find("terminate") != std::string::npos ||
      key.find("abort") != std::string::npos ||
      key.find("denied") != std::string::npos ||
      key.find("reject") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::MutedError;
  }
  if (key.find("warn") != std::string::npos ||
      key.find("pause") != std::string::npos ||
      key.find("park") != std::string::npos ||
      key.find("degrad") != std::string::npos ||
      key.find("block") != std::string::npos ||
      key.find("review") != std::string::npos ||
      key.find("govern") != std::string::npos ||
      key.find("clarif") != std::string::npos ||
      key.find("queue") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (key.find("launch") != std::string::npos ||
      key.find("resume") != std::string::npos ||
      key.find("active") != std::string::npos ||
      key.find("finish") != std::string::npos ||
      key.find("complete") != std::string::npos ||
      key.find("deliver") != std::string::npos ||
      key.find("handle") != std::string::npos ||
      key.find("unblock") != std::string::npos ||
      key.find("archive") != std::string::npos ||
      key.find("ack") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  if (key.find("start") != std::string::npos ||
      key.find("create") != std::string::npos ||
      key.find("checkpoint") != std::string::npos ||
      key.find("stage") != std::string::npos ||
      key.find("request") != std::string::npos ||
      key.find("message") != std::string::npos ||
      key.find("intent") != std::string::npos ||
      key.find("tool") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Info;
}

inline RuntimeMarshalEventViewerEntry
runtime_build_marshaled_event_entry(const std::string &raw_line) {
  RuntimeMarshalEventViewerEntry entry{};
  entry.raw_line = raw_line;
  entry.summary = trim_copy(raw_line);
  entry.emphasis = cuwacunu::iinuji::text_line_emphasis_t::Debug;

  cmd_json_value_t parsed{};
  if (!runtime_try_parse_event_json_object_line(raw_line, &parsed)) {
    entry.event_name = "raw";
    if (entry.summary.empty())
      entry.summary = "<empty>";
    return entry;
  }

  entry.timestamp_ms = runtime_event_json_timestamp_ms(&parsed);
  entry.event_name =
      runtime_event_json_first_string_deep(&parsed, "event", "type", "kind");
  if (entry.event_name.empty())
    entry.event_name = "event";
  entry.summary = runtime_event_json_first_string_deep(
      &parsed, "detail", "summary", "text", "message", "status");
  if (entry.summary.empty())
    entry.summary = trim_copy(raw_line);
  entry.emphasis = runtime_marshaled_event_emphasis_from_name(entry.event_name);

  std::vector<std::pair<std::string, std::string>> metadata{};
  auto append_meta = [&](std::string_view key) {
    if (key.empty())
      return;
    const auto *value = cmd_json_field(&parsed, key);
    if (value == nullptr)
      return;
    const std::string text = runtime_event_json_value_text(value);
    if (text.empty() || text == "{...}")
      return;
    for (const auto &existing : metadata) {
      if (existing.first == key)
        return;
    }
    metadata.emplace_back(std::string(key), text);
  };

  const std::string_view preferred_keys[] = {
      "checkpoint_index", "checkpoint_count", "launch_count", "lifecycle",
      "intent",           "intent_kind",      "work_gate",    "finish_reason",
      "campaign_status",  "campaign_cursor",  "tool_name",    "role",
      "stream",           "line_index"};
  for (const auto key : preferred_keys) {
    append_meta(key);
    if (metadata.size() >= 5)
      break;
  }

  if (metadata.size() < 5 && parsed.objectValue != nullptr) {
    static const std::vector<std::string> ignored_keys = {
        "timestamp_ms", "event", "type",    "kind",   "detail",
        "summary",      "text",  "message", "status", "marshal_session_id",
        "schema"};
    for (const auto &kv : *parsed.objectValue) {
      if (std::find(ignored_keys.begin(), ignored_keys.end(), kv.first) !=
          ignored_keys.end()) {
        continue;
      }
      append_meta(kv.first);
      if (metadata.size() >= 5)
        break;
    }
  }

  entry.metadata = std::move(metadata);
  return entry;
}

inline cuwacunu::iinuji::text_line_emphasis_t
runtime_codex_line_emphasis(std::string_view kind) {
  const std::string key = to_lower_copy(std::string(kind));
  if (key.find("error") != std::string::npos ||
      key.find("fail") != std::string::npos ||
      key.find("fatal") != std::string::npos ||
      key.find("abort") != std::string::npos ||
      key.find("refus") != std::string::npos ||
      key.find("denied") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::MutedError;
  }
  if (key.find("stderr") != std::string::npos ||
      key.find("warn") != std::string::npos ||
      key.find("retry") != std::string::npos ||
      key.find("wait") != std::string::npos ||
      key.find("pending") != std::string::npos ||
      key.find("interrupt") != std::string::npos ||
      key.find("cancel") != std::string::npos ||
      key.find("block") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (key.find("tool") != std::string::npos ||
      key.find("patch") != std::string::npos ||
      key.find("exec") != std::string::npos ||
      key.find("reason") != std::string::npos ||
      key.find("think") != std::string::npos ||
      key.find("plan") != std::string::npos ||
      key.find("analysis") != std::string::npos ||
      key.find("commentary") != std::string::npos ||
      key.find("call") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  }
  if (key.find("done") != std::string::npos ||
      key.find("complete") != std::string::npos ||
      key.find("success") != std::string::npos ||
      key.find("result") != std::string::npos ||
      key.find("finish") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  if (key.find("message") != std::string::npos ||
      key.find("assistant") != std::string::npos ||
      key.find("stdout") != std::string::npos ||
      key.find("item") != std::string::npos ||
      key.find("thread") != std::string::npos ||
      key.find("turn") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Info;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Debug;
}

inline std::string
runtime_codex_stream_fallback_label(RuntimeLogViewerKind kind) {
  return kind == RuntimeLogViewerKind::MarshalCodexStderr ? "stderr" : "stdout";
}

inline RuntimeMarshalEventViewerEntry
runtime_build_codex_stream_entry(const std::string &raw_line,
                                 std::string_view fallback_kind) {
  RuntimeMarshalEventViewerEntry entry{};
  entry.raw_line = raw_line;
  entry.summary = trim_copy(raw_line);
  entry.event_name = std::string(fallback_kind);
  entry.emphasis = runtime_codex_line_emphasis(fallback_kind);

  cmd_json_value_t parsed{};
  if (!runtime_try_parse_event_json_object_line(raw_line, &parsed)) {
    if (entry.summary.empty())
      entry.summary = "<empty>";
    return entry;
  }

  entry.timestamp_ms = runtime_event_json_timestamp_ms(&parsed);
  std::string kind = runtime_event_json_first_string_deep(
      &parsed, "type", "event", "kind", "stream");
  if (kind.empty())
    kind = std::string(fallback_kind);
  const std::string role =
      runtime_event_json_first_string_deep(&parsed, "role");
  const std::string stream =
      runtime_event_json_first_string_deep(&parsed, "stream");
  const std::string tool_name =
      runtime_event_json_first_string_deep(&parsed, "tool_name");
  const std::string item_type = runtime_event_json_first_string_deep(
      cmd_json_field(&parsed, "item"), "type", "kind");
  std::string detail = runtime_event_json_first_string_deep(
      &parsed, "text", "detail", "message", "summary", "status");
  if (detail.empty()) {
    detail = runtime_event_json_first_string_deep(
        cmd_json_field(&parsed, "item"), "text", "message", "summary");
  }
  if (detail.empty() && !item_type.empty()) {
    detail = item_type;
  }
  if (detail.empty()) {
    detail = trim_copy(raw_line);
  }

  entry.event_name = kind;
  if (!role.empty() && to_lower_copy(role) != to_lower_copy(kind)) {
    entry.event_name += "/" + role;
  }
  entry.emphasis = runtime_codex_line_emphasis(entry.event_name + " " + detail);
  entry.summary = detail;

  std::vector<std::pair<std::string, std::string>> metadata{};
  auto append_meta = [&](std::string key, std::string value) {
    value = trim_copy(std::move(value));
    if (key.empty() || value.empty() || value == "{...}")
      return;
    for (const auto &existing : metadata) {
      if (existing.first == key)
        return;
    }
    metadata.emplace_back(std::move(key), std::move(value));
  };

  append_meta("stream", stream);
  append_meta("tool", tool_name);
  if (const auto *line_index =
          runtime_event_json_find_value_deep(&parsed, "line_index");
      line_index != nullptr) {
    append_meta("line", runtime_event_json_value_text(line_index));
  }
  if (const auto *item = cmd_json_field(&parsed, "item");
      item != nullptr && item->type == cmd_json_type_t::OBJECT) {
    append_meta("item",
                runtime_event_json_first_string_deep(item, "type", "kind"));
    append_meta("id", runtime_event_json_first_string_deep(item, "id"));
  }
  append_meta("thread",
              runtime_event_json_first_string_deep(&parsed, "thread_id"));
  append_meta("turn", runtime_event_json_first_string_deep(&parsed, "turn_id"));
  if (metadata.size() < 5 && parsed.objectValue != nullptr) {
    const std::string_view preferred_keys[] = {"id", "code", "status", "reason",
                                               "role"};
    for (const auto key : preferred_keys) {
      const auto *value = runtime_event_json_find_value_deep(&parsed, key);
      if (value == nullptr)
        continue;
      append_meta(std::string(key), runtime_event_json_value_text(value));
      if (metadata.size() >= 5)
        break;
    }
  }

  entry.metadata = std::move(metadata);
  return entry;
}

inline bool runtime_read_structured_log_entries(
    const std::string &path, std::vector<RuntimeMarshalEventViewerEntry> *out,
    RuntimeLogViewerKind kind, std::string *error) {
  if (out == nullptr)
    return false;
  out->clear();
  std::string raw{};
  if (!read_text_file_safe(path, &raw, error))
    return false;
  const auto lines = split_lines_keep_empty(raw);
  out->reserve(lines.size());
  for (const auto &line : lines) {
    if (trim_copy(line).empty())
      continue;
    if (kind == RuntimeLogViewerKind::MarshalEvents) {
      out->push_back(runtime_build_marshaled_event_entry(line));
    } else {
      out->push_back(runtime_build_codex_stream_entry(
          line, runtime_codex_stream_fallback_label(kind)));
    }
  }
  return true;
}

inline std::size_t runtime_event_viewer_entry_count(const CmdState &st) {
  return st.runtime.marshal_event_viewer.entries.size();
}

inline bool runtime_event_viewer_has_entries(const CmdState &st) {
  return runtime_event_viewer_entry_count(st) != 0;
}

inline bool runtime_event_viewer_is_at_end(const CmdState &st) {
  return !runtime_event_viewer_has_entries(st) ||
         st.runtime.marshal_event_viewer.selected_entry + 1 >=
             runtime_event_viewer_entry_count(st);
}

inline void runtime_snap_event_viewer_to_end(CmdState &st) {
  if (!runtime_event_viewer_has_entries(st)) {
    st.runtime.marshal_event_viewer.selected_entry = 0;
    return;
  }
  st.runtime.marshal_event_viewer.selected_entry =
      runtime_event_viewer_entry_count(st) - 1;
}

inline bool runtime_open_log_viewer(CmdState &st, const std::string &path,
                                    RuntimeLogViewerKind kind,
                                    std::string title,
                                    bool announce_status = true) {
  const bool supports_follow = runtime_log_viewer_kind_supports_follow(kind);
  if (runtime_log_viewer_kind_uses_structured_view(kind)) {
    const bool preserve_selection = runtime_event_viewer_is_open(st) &&
                                    st.runtime.log_viewer_path == path &&
                                    st.runtime.log_viewer_kind == kind;
    const bool previous_follow = st.runtime.log_viewer_live_follow;
    const std::size_t previous_selected =
        st.runtime.marshal_event_viewer.selected_entry;
    std::string error{};
    std::vector<RuntimeMarshalEventViewerEntry> entries{};
    if (!runtime_read_structured_log_entries(path, &entries, kind, &error)) {
      set_runtime_status(st,
                         error.empty()
                             ? "failed to open " +
                                   runtime_log_viewer_kind_label(kind) + " file"
                             : error,
                         true);
      return true;
    }

    st.runtime.log_viewer.reset();
    st.runtime.log_viewer_open = true;
    st.runtime.log_viewer_kind = kind;
    st.runtime.log_viewer_path = path;
    st.runtime.log_viewer_title = std::move(title);
    st.runtime.log_viewer_last_poll_ms = 0;
    st.runtime.marshal_event_viewer.entries = std::move(entries);
    st.runtime.marshal_event_viewer.selected_entry = 0;

    if (preserve_selection && !previous_follow) {
      st.runtime.log_viewer_live_follow = false;
      if (runtime_event_viewer_has_entries(st)) {
        st.runtime.marshal_event_viewer.selected_entry = std::min(
            previous_selected, runtime_event_viewer_entry_count(st) - 1);
      }
    } else {
      st.runtime.log_viewer_live_follow = supports_follow;
      if (supports_follow)
        runtime_snap_event_viewer_to_end(st);
    }

    runtime_record_log_viewer_stamp(st, runtime_probe_log_viewer_file(path));
    if (announce_status) {
      set_runtime_status(
          st,
          "Opened " +
              runtime_log_viewer_kind_label(st.runtime.log_viewer_kind) +
              " in the right panel.",
          false);
    }
    return true;
  }

  const auto previous = st.runtime.log_viewer;
  const bool preserve_viewport = runtime_log_viewer_is_open(st) && previous &&
                                 st.runtime.log_viewer_path == path &&
                                 st.runtime.log_viewer_kind == kind;
  std::string error{};
  auto editor = runtime_build_log_viewer_editor(path, &error);
  if (!editor) {
    set_runtime_status(
        st, error.empty() ? "failed to open runtime log file" : error, true);
    return true;
  }
  if (preserve_viewport && !st.runtime.log_viewer_live_follow) {
    runtime_restore_log_viewer_viewport(*editor, *previous);
  } else if (supports_follow) {
    st.runtime.log_viewer_live_follow = true;
    if (previous)
      editor->last_body_h =
          std::max(editor->last_body_h, previous->last_body_h);
    runtime_snap_log_viewer_to_end(*editor);
  } else {
    st.runtime.log_viewer_live_follow = false;
    if (previous)
      editor->last_body_h =
          std::max(editor->last_body_h, previous->last_body_h);
  }
  st.runtime.marshal_event_viewer.entries.clear();
  st.runtime.marshal_event_viewer.selected_entry = 0;
  st.runtime.log_viewer = std::move(editor);
  st.runtime.log_viewer_open = true;
  st.runtime.log_viewer_kind = kind;
  st.runtime.log_viewer_path = path;
  st.runtime.log_viewer_title = std::move(title);
  st.runtime.log_viewer_last_poll_ms = 0;
  runtime_record_log_viewer_stamp(st, runtime_probe_log_viewer_file(path));
  if (announce_status) {
    set_runtime_status(
        st,
        "Opened " + runtime_log_viewer_kind_label(st.runtime.log_viewer_kind) +
            " in the right panel.",
        false);
  }
  return true;
}

inline bool runtime_reload_log_viewer(CmdState &st,
                                      bool announce_status = true) {
  if (!runtime_log_viewer_is_open(st) || st.runtime.log_viewer_path.empty()) {
    return false;
  }
  const std::string title = st.runtime.log_viewer_title;
  const RuntimeLogViewerKind kind = st.runtime.log_viewer_kind;
  const bool ok = runtime_open_log_viewer(st, st.runtime.log_viewer_path, kind,
                                          title, false);
  if (ok && announce_status) {
    set_runtime_status(
        st,
        "Reloaded " +
            runtime_log_viewer_kind_label(st.runtime.log_viewer_kind) + ".",
        false);
  }
  return ok;
}

inline bool runtime_toggle_log_viewer_follow(CmdState &st) {
  if (!runtime_log_viewer_is_open(st))
    return false;
  if (!runtime_log_viewer_supports_follow(st)) {
    set_runtime_status(st, "This viewer does not support live-follow.", false);
    return true;
  }
  st.runtime.log_viewer_live_follow = !st.runtime.log_viewer_live_follow;
  if (st.runtime.log_viewer_live_follow) {
    if (runtime_event_viewer_is_open(st)) {
      runtime_snap_event_viewer_to_end(st);
    } else if (st.runtime.log_viewer) {
      runtime_snap_log_viewer_to_end(*st.runtime.log_viewer);
    }
  }
  set_runtime_status(st,
                     st.runtime.log_viewer_live_follow
                         ? "Runtime log live-follow enabled."
                         : "Runtime log live-follow paused.",
                     false);
  return true;
}

inline bool runtime_poll_live_log_viewer(CmdState &st, std::uint64_t now_ms) {
  if (!runtime_log_viewer_is_open(st) || st.runtime.log_viewer_path.empty()) {
    return false;
  }
  if (!runtime_log_viewer_supports_follow(st))
    return false;
  constexpr std::uint64_t kRuntimeLogViewerPollPeriodMs = 700;
  if (st.runtime.log_viewer_last_poll_ms != 0 &&
      now_ms <
          st.runtime.log_viewer_last_poll_ms + kRuntimeLogViewerPollPeriodMs) {
    return false;
  }
  st.runtime.log_viewer_last_poll_ms = now_ms;
  const auto stamp = runtime_probe_log_viewer_file(st.runtime.log_viewer_path);
  if (!stamp.ok || runtime_log_viewer_stamp_matches(st, stamp))
    return false;
  return runtime_reload_log_viewer(st, false);
}

inline bool runtime_select_prev_event_viewer_entry(CmdState &st) {
  if (!runtime_event_viewer_has_entries(st) ||
      st.runtime.marshal_event_viewer.selected_entry == 0) {
    return false;
  }
  --st.runtime.marshal_event_viewer.selected_entry;
  st.runtime.log_viewer_live_follow = false;
  return true;
}

inline bool runtime_select_next_event_viewer_entry(CmdState &st) {
  if (!runtime_event_viewer_has_entries(st) ||
      st.runtime.marshal_event_viewer.selected_entry + 1 >=
          runtime_event_viewer_entry_count(st)) {
    return false;
  }
  ++st.runtime.marshal_event_viewer.selected_entry;
  st.runtime.log_viewer_live_follow = false;
  return true;
}

inline bool runtime_select_first_event_viewer_entry(CmdState &st) {
  if (!runtime_event_viewer_has_entries(st))
    return false;
  st.runtime.marshal_event_viewer.selected_entry = 0;
  st.runtime.log_viewer_live_follow = false;
  return true;
}

inline bool runtime_select_last_event_viewer_entry(CmdState &st,
                                                   bool enable_follow) {
  if (!runtime_event_viewer_has_entries(st))
    return false;
  runtime_snap_event_viewer_to_end(st);
  st.runtime.log_viewer_live_follow = enable_follow;
  return true;
}

inline bool runtime_page_event_viewer_entries(CmdState &st, int delta) {
  if (!runtime_event_viewer_has_entries(st) || delta == 0)
    return false;
  const auto max_index =
      static_cast<std::ptrdiff_t>(runtime_event_viewer_entry_count(st) - 1);
  const auto current = static_cast<std::ptrdiff_t>(
      st.runtime.marshal_event_viewer.selected_entry);
  const auto next = std::clamp(current + static_cast<std::ptrdiff_t>(delta),
                               static_cast<std::ptrdiff_t>(0), max_index);
  if (next == current)
    return false;
  st.runtime.marshal_event_viewer.selected_entry =
      static_cast<std::size_t>(next);
  st.runtime.log_viewer_live_follow = false;
  return true;
}

inline std::uint64_t runtime_monotonic_now_ms() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

inline std::uint64_t runtime_wall_now_ms() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

inline std::vector<std::string>
runtime_split_csv_fields(const std::string &line) {
  std::vector<std::string> fields{};
  std::string current{};
  bool in_quotes = false;
  for (char c : line) {
    if (c == '"') {
      in_quotes = !in_quotes;
      continue;
    }
    if (c == ',' && !in_quotes) {
      fields.push_back(trim_copy(current));
      current.clear();
      continue;
    }
    current.push_back(c);
  }
  fields.push_back(trim_copy(current));
  return fields;
}

inline bool runtime_parse_u64_token(std::string token, std::uint64_t *out) {
  if (out == nullptr)
    return false;
  token = trim_copy(token);
  if (token.empty() || token == "N/A" || token == "[N/A]")
    return false;
  std::istringstream iss(token);
  std::uint64_t value = 0;
  iss >> value;
  if (!iss || !iss.eof())
    return false;
  *out = value;
  return true;
}

inline bool runtime_parse_int_token(std::string token, int *out) {
  if (out == nullptr)
    return false;
  token = trim_copy(token);
  if (token.empty() || token == "N/A" || token == "[N/A]")
    return false;
  std::istringstream iss(token);
  int value = 0;
  iss >> value;
  if (!iss || !iss.eof())
    return false;
  *out = value;
  return true;
}

inline bool runtime_parse_double_token(std::string token, double *out) {
  if (out == nullptr)
    return false;
  token = trim_copy(token);
  if (token.empty() || token == "N/A" || token == "[N/A]")
    return false;
  std::istringstream iss(token);
  double value = 0.0;
  iss >> value;
  if (!iss || !iss.eof())
    return false;
  *out = value;
  return true;
}

inline bool runtime_capture_command_output(const std::string &command,
                                           std::string *out, int *status_code) {
  if (out)
    out->clear();
  if (status_code)
    *status_code = -1;

  FILE *pipe = ::popen(command.c_str(), "r");
  if (pipe == nullptr)
    return false;

  char buffer[4096];
  std::ostringstream oss{};
  while (::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    oss << buffer;
  }
  const int status = ::pclose(pipe);
  if (out)
    *out = oss.str();
  if (status_code)
    *status_code = status;
  return status == 0;
}

inline void runtime_parse_meminfo_text(const std::string &text,
                                       RuntimeDeviceSnapshot *out) {
  if (out == nullptr)
    return;
  for (const auto &line : split_lines_keep_empty(text)) {
    std::istringstream iss(line);
    std::string key{};
    std::uint64_t value = 0;
    iss >> key >> value;
    if (!iss)
      continue;
    if (key == "MemTotal:") {
      out->mem_total_kib = value;
    } else if (key == "MemAvailable:") {
      out->mem_available_kib = value;
    }
  }
}

inline void runtime_parse_proc_stat_text(const std::string &text,
                                         RuntimeDeviceSnapshot *out) {
  if (out == nullptr)
    return;
  for (const auto &line : split_lines_keep_empty(text)) {
    if (line.rfind("cpu ", 0) != 0)
      continue;
    std::istringstream iss(line.substr(4));
    (void)(iss >> out->cpu_user_ticks >> out->cpu_nice_ticks >>
           out->cpu_system_ticks >> out->cpu_idle_ticks >>
           out->cpu_iowait_ticks >> out->cpu_irq_ticks >>
           out->cpu_softirq_ticks >> out->cpu_steal_ticks);
    break;
  }
}

inline void runtime_compute_cpu_usage(RuntimeDeviceSnapshot *current,
                                      const RuntimeDeviceSnapshot *previous) {
  if (current == nullptr || previous == nullptr)
    return;
  const std::uint64_t prev_total =
      previous->cpu_user_ticks + previous->cpu_nice_ticks +
      previous->cpu_system_ticks + previous->cpu_idle_ticks +
      previous->cpu_iowait_ticks + previous->cpu_irq_ticks +
      previous->cpu_softirq_ticks + previous->cpu_steal_ticks;
  const std::uint64_t curr_total =
      current->cpu_user_ticks + current->cpu_nice_ticks +
      current->cpu_system_ticks + current->cpu_idle_ticks +
      current->cpu_iowait_ticks + current->cpu_irq_ticks +
      current->cpu_softirq_ticks + current->cpu_steal_ticks;
  if (curr_total <= prev_total)
    return;

  const std::uint64_t total_delta = curr_total - prev_total;
  const std::uint64_t idle_prev =
      previous->cpu_idle_ticks + previous->cpu_iowait_ticks;
  const std::uint64_t idle_curr =
      current->cpu_idle_ticks + current->cpu_iowait_ticks;
  const std::uint64_t idle_delta =
      idle_curr >= idle_prev ? (idle_curr - idle_prev) : 0;
  const std::uint64_t busy_delta =
      total_delta >= idle_delta ? (total_delta - idle_delta) : 0;
  current->cpu_usage_pct =
      std::clamp(static_cast<int>((100.0 * static_cast<double>(busy_delta)) /
                                      static_cast<double>(total_delta) +
                                  0.5),
                 0, 100);

  const std::uint64_t iowait_delta =
      current->cpu_iowait_ticks >= previous->cpu_iowait_ticks
          ? (current->cpu_iowait_ticks - previous->cpu_iowait_ticks)
          : 0;
  current->cpu_iowait_pct =
      std::clamp(static_cast<int>((100.0 * static_cast<double>(iowait_delta)) /
                                      static_cast<double>(total_delta) +
                                  0.5),
                 0, 100);
}

inline void runtime_collect_host_device_snapshot(RuntimeDeviceSnapshot *out) {
  if (out == nullptr)
    return;
  out->host_name.clear();
  out->cpu_count = std::max(1u, std::thread::hardware_concurrency());

  std::string hostname{};
  std::string ignored_error{};
  if (read_text_file_safe("/proc/sys/kernel/hostname", &hostname,
                          &ignored_error)) {
    out->host_name = trim_copy(hostname);
  }

  std::string loadavg{};
  if (read_text_file_safe("/proc/loadavg", &loadavg, &ignored_error)) {
    std::istringstream iss(loadavg);
    (void)(iss >> out->load1 >> out->load5 >> out->load15);
  }

  std::string proc_stat{};
  if (read_text_file_safe("/proc/stat", &proc_stat, &ignored_error)) {
    runtime_parse_proc_stat_text(proc_stat, out);
  }

  std::string meminfo{};
  if (read_text_file_safe("/proc/meminfo", &meminfo, &ignored_error)) {
    runtime_parse_meminfo_text(meminfo, out);
  }
}

inline void runtime_collect_gpu_device_snapshot(RuntimeDeviceSnapshot *out) {
  if (out == nullptr)
    return;

  out->gpu_query_attempted = true;
  out->gpus.clear();
  out->gpu_processes.clear();
  out->gpu_error.clear();

  std::string gpu_output{};
  int gpu_status = -1;
  const bool gpu_ok = runtime_capture_command_output(
      "LC_ALL=C nvidia-smi "
      "--query-gpu=index,name,uuid,utilization.gpu,utilization.memory,"
      "memory.used,memory.total,temperature.gpu "
      "--format=csv,noheader,nounits 2>&1",
      &gpu_output, &gpu_status);
  if (!gpu_ok) {
    const std::string error_text = trim_copy(gpu_output);
    if (to_lower_copy(error_text).find("not found") != std::string::npos) {
      out->gpu_query_attempted = false;
      out->gpu_error = "nvidia-smi unavailable";
      return;
    }
    out->gpu_error =
        error_text.empty() ? "nvidia-smi query failed" : error_text;
    return;
  }

  for (const auto &raw_line : split_lines_keep_empty(gpu_output)) {
    const std::string line = trim_copy(raw_line);
    if (line.empty())
      continue;
    const auto fields = runtime_split_csv_fields(line);
    if (fields.size() < 8)
      continue;

    RuntimeDeviceGpuSnapshot gpu{};
    gpu.index = fields[0];
    gpu.name = fields[1];
    gpu.uuid = fields[2];
    (void)runtime_parse_int_token(fields[3], &gpu.util_gpu_pct);
    (void)runtime_parse_int_token(fields[4], &gpu.util_mem_pct);
    (void)runtime_parse_u64_token(fields[5], &gpu.memory_used_mib);
    (void)runtime_parse_u64_token(fields[6], &gpu.memory_total_mib);
    (void)runtime_parse_int_token(fields[7], &gpu.temperature_c);
    out->gpus.push_back(std::move(gpu));
  }

  if (out->gpus.empty()) {
    out->gpu_error = "no NVIDIA GPU detected";
    return;
  }

  std::string proc_output{};
  int proc_status = -1;
  const bool proc_ok = runtime_capture_command_output(
      "LC_ALL=C nvidia-smi "
      "--query-compute-apps=pid,gpu_uuid,used_gpu_memory "
      "--format=csv,noheader,nounits 2>/dev/null",
      &proc_output, &proc_status);
  (void)proc_status;
  if (!proc_ok)
    return;

  for (const auto &raw_line : split_lines_keep_empty(proc_output)) {
    const std::string line = trim_copy(raw_line);
    if (line.empty())
      continue;
    if (to_lower_copy(line).find("no running processes") != std::string::npos) {
      continue;
    }
    const auto fields = runtime_split_csv_fields(line);
    if (fields.size() < 3)
      continue;
    RuntimeDeviceGpuProcessSnapshot proc{};
    if (!runtime_parse_u64_token(fields[0], &proc.pid))
      continue;
    proc.gpu_uuid = fields[1];
    (void)runtime_parse_u64_token(fields[2], &proc.used_memory_mib);
    out->gpu_processes.push_back(std::move(proc));
  }
}

inline RuntimeDeviceSnapshot runtime_collect_device_snapshot() {
  RuntimeDeviceSnapshot snapshot{};
  snapshot.collected_at_ms = runtime_wall_now_ms();
  runtime_collect_host_device_snapshot(&snapshot);
  runtime_collect_gpu_device_snapshot(&snapshot);
  snapshot.ok = true;

  if (snapshot.host_name.empty() && snapshot.mem_total_kib == 0 &&
      snapshot.gpus.empty() && !snapshot.gpu_query_attempted) {
    snapshot.error = "device snapshot unavailable";
  }
  return snapshot;
}

struct RuntimePidResourceSnapshot {
  bool alive{false};
  std::string name{};
  std::string state{};
  std::uint64_t rss_kib{0};
};

inline RuntimePidResourceSnapshot
runtime_probe_pid_resources(std::uint64_t pid) {
  RuntimePidResourceSnapshot snapshot{};
  if (pid == 0)
    return snapshot;

  snapshot.alive = cuwacunu::hero::runtime::pid_alive(pid);
  std::string status_text{};
  std::string ignored_error{};
  if (!read_text_file_safe("/proc/" + std::to_string(pid) + "/status",
                           &status_text, &ignored_error)) {
    return snapshot;
  }

  snapshot.alive = true;
  for (const auto &line : split_lines_keep_empty(status_text)) {
    if (line.rfind("Name:", 0) == 0) {
      snapshot.name = trim_copy(line.substr(5));
      continue;
    }
    if (line.rfind("State:", 0) == 0) {
      snapshot.state = trim_copy(line.substr(6));
      continue;
    }
    if (line.rfind("VmRSS:", 0) == 0) {
      std::istringstream iss(line.substr(6));
      std::uint64_t rss_kib = 0;
      if (iss >> rss_kib)
        snapshot.rss_kib = rss_kib;
    }
  }
  return snapshot;
}

inline std::size_t runtime_device_item_count(const CmdState &st) {
  return std::size_t{1} + st.runtime.device.gpus.size();
}

inline std::optional<std::size_t>
runtime_selected_device_index(const CmdState &st) {
  const std::size_t total = runtime_device_item_count(st);
  if (total == 0)
    return std::nullopt;
  return std::min(st.runtime.selected_device_index, total - 1);
}

inline bool runtime_apply_device_snapshot(CmdState &st,
                                          RuntimeDeviceSnapshot snapshot) {
  if (st.runtime.device.collected_at_ms != 0) {
    runtime_compute_cpu_usage(&snapshot, &st.runtime.device);
  }
  st.runtime.device = std::move(snapshot);
  st.runtime.device_history.push_back(st.runtime.device);
  constexpr std::size_t kRuntimeDeviceHistoryLimit = 96;
  if (st.runtime.device_history.size() > kRuntimeDeviceHistoryLimit) {
    st.runtime.device_history.erase(
        st.runtime.device_history.begin(),
        st.runtime.device_history.begin() +
            static_cast<std::ptrdiff_t>(st.runtime.device_history.size() -
                                        kRuntimeDeviceHistoryLimit));
  }
  const std::size_t total = runtime_device_item_count(st);
  if (total == 0) {
    st.runtime.selected_device_index = 0;
  } else {
    st.runtime.selected_device_index =
        std::min(st.runtime.selected_device_index, total - 1);
  }
  return true;
}

inline bool queue_runtime_device_refresh(CmdState &st) {
  if (st.runtime.device_pending) {
    st.runtime.device_requeue = true;
    return false;
  }
  st.runtime.device_pending = true;
  st.runtime.device_requeue = false;
  st.runtime.device_last_refresh_ms = runtime_monotonic_now_ms();
  st.runtime.device_future =
      std::async(std::launch::async, []() -> RuntimeDeviceSnapshot {
        return runtime_collect_device_snapshot();
      });
  return true;
}

inline bool runtime_poll_device_refresh(CmdState &st, std::uint64_t now_ms) {
  constexpr std::uint64_t kRuntimeDeviceRefreshPeriodMs = 700;
  if (st.screen != ScreenMode::Runtime)
    return false;
  if (st.runtime.device_pending)
    return false;
  if (st.runtime.device_last_refresh_ms != 0 &&
      now_ms <
          st.runtime.device_last_refresh_ms + kRuntimeDeviceRefreshPeriodMs) {
    return false;
  }
  return queue_runtime_device_refresh(st);
}

inline bool runtime_open_job_log_viewer(CmdState &st,
                                        RuntimeLogViewerKind kind) {
  if (!st.runtime.job.has_value())
    return false;
  const auto &job = *st.runtime.job;
  std::string path{};
  switch (kind) {
  case RuntimeLogViewerKind::JobStdout:
    path = job.stdout_path;
    break;
  case RuntimeLogViewerKind::JobStderr:
    path = job.stderr_path;
    break;
  case RuntimeLogViewerKind::JobTrace:
    path = job.trace_path;
    break;
  case RuntimeLogViewerKind::MarshalEvents:
  case RuntimeLogViewerKind::MarshalCodexStdout:
  case RuntimeLogViewerKind::MarshalCodexStderr:
  case RuntimeLogViewerKind::CampaignStdout:
  case RuntimeLogViewerKind::CampaignStderr:
  case RuntimeLogViewerKind::ArtifactFile:
  case RuntimeLogViewerKind::None:
    break;
  }
  if (path.empty()) {
    set_runtime_status(st,
                       "Selected job does not have a " +
                           runtime_log_viewer_kind_label(kind) + " path.",
                       true);
    return true;
  }
  return runtime_open_log_viewer(st, path, kind,
                                 runtime_log_viewer_kind_label(kind) + " | " +
                                     trim_to_width(job.job_cursor, 28));
}

inline bool runtime_open_campaign_log_viewer(CmdState &st,
                                             RuntimeLogViewerKind kind) {
  if (!st.runtime.campaign.has_value())
    return false;
  const auto &campaign = *st.runtime.campaign;
  std::string path{};
  switch (kind) {
  case RuntimeLogViewerKind::CampaignStdout:
    path = campaign.stdout_path;
    break;
  case RuntimeLogViewerKind::CampaignStderr:
    path = campaign.stderr_path;
    break;
  case RuntimeLogViewerKind::JobStdout:
  case RuntimeLogViewerKind::JobStderr:
  case RuntimeLogViewerKind::MarshalEvents:
  case RuntimeLogViewerKind::MarshalCodexStdout:
  case RuntimeLogViewerKind::MarshalCodexStderr:
  case RuntimeLogViewerKind::JobTrace:
  case RuntimeLogViewerKind::ArtifactFile:
  case RuntimeLogViewerKind::None:
    break;
  }
  if (path.empty()) {
    set_runtime_status(st,
                       "Selected campaign does not have a " +
                           runtime_log_viewer_kind_label(kind) + " path.",
                       true);
    return true;
  }
  return runtime_open_log_viewer(
      st, path, kind,
      runtime_log_viewer_kind_label(kind) + " | " +
          trim_to_width(campaign.campaign_cursor, 28));
}

inline bool runtime_open_session_log_viewer(CmdState &st,
                                            RuntimeLogViewerKind kind) {
  if (!st.runtime.session.has_value())
    return false;
  const auto &session = *st.runtime.session;
  std::string path{};
  switch (kind) {
  case RuntimeLogViewerKind::MarshalEvents:
    path = session.events_path;
    break;
  case RuntimeLogViewerKind::MarshalCodexStdout:
    path = session.codex_stdout_path;
    break;
  case RuntimeLogViewerKind::MarshalCodexStderr:
    path = session.codex_stderr_path;
    break;
  case RuntimeLogViewerKind::JobStdout:
  case RuntimeLogViewerKind::JobStderr:
  case RuntimeLogViewerKind::CampaignStdout:
  case RuntimeLogViewerKind::CampaignStderr:
  case RuntimeLogViewerKind::JobTrace:
  case RuntimeLogViewerKind::ArtifactFile:
  case RuntimeLogViewerKind::None:
    break;
  }
  if (path.empty()) {
    set_runtime_status(st,
                       "Selected marshal does not have a " +
                           runtime_log_viewer_kind_label(kind) + " path.",
                       true);
    return true;
  }
  return runtime_open_log_viewer(
      st, path, kind,
      runtime_log_viewer_kind_label(kind) + " | " +
          trim_to_width(session.marshal_session_id, 28));
}

inline bool runtime_open_artifact_viewer(CmdState &st, const std::string &path,
                                         std::string title,
                                         std::string missing_status) {
  if (path.empty()) {
    set_runtime_status(st,
                       missing_status.empty() ? "artifact path is not recorded"
                                              : std::move(missing_status),
                       true);
    return true;
  }
  return runtime_open_log_viewer(st, path, RuntimeLogViewerKind::ArtifactFile,
                                 std::move(title));
}

inline bool runtime_parse_marshal_session_json(
    const cmd_json_value_t *value,
    cuwacunu::hero::marshal::marshal_session_record_t *out,
    std::string *error) {
  if (error)
    error->clear();
  if (value == nullptr || value->type != cmd_json_type_t::OBJECT) {
    if (error)
      *error = "invalid marshal session payload";
    return false;
  }
  if (out == nullptr) {
    if (error)
      *error = "missing marshal session output";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  session.schema =
      cmd_json_string(cmd_json_field(value, "schema"), session.schema);
  session.marshal_session_id =
      cmd_json_string(cmd_json_field(value, "marshal_session_id"));
  session.lifecycle =
      cmd_json_string(cmd_json_field(value, "lifecycle"), session.lifecycle);
  session.status_detail =
      cmd_json_string(cmd_json_field(value, "status_detail"));
  session.work_gate =
      cmd_json_string(cmd_json_field(value, "work_gate"), session.work_gate);
  session.finish_reason = cmd_json_string(
      cmd_json_field(value, "finish_reason"), session.finish_reason);
  session.activity =
      cmd_json_string(cmd_json_field(value, "activity"), session.activity);
  session.objective_name =
      cmd_json_string(cmd_json_field(value, "objective_name"));
  session.global_config_path =
      cmd_json_string(cmd_json_field(value, "global_config_path"));
  session.source_marshal_objective_dsl_path = cmd_json_string(
      cmd_json_field(value, "source_marshal_objective_dsl_path"));
  session.source_campaign_dsl_path =
      cmd_json_string(cmd_json_field(value, "source_campaign_dsl_path"));
  session.source_marshal_objective_md_path = cmd_json_string(
      cmd_json_field(value, "source_marshal_objective_md_path"));
  session.source_marshal_guidance_md_path =
      cmd_json_string(cmd_json_field(value, "source_marshal_guidance_md_path"));
  session.session_root = cmd_json_string(cmd_json_field(value, "session_root"));
  session.objective_root =
      cmd_json_string(cmd_json_field(value, "objective_root"));
  session.campaign_dsl_path =
      cmd_json_string(cmd_json_field(value, "campaign_dsl_path"));
  session.marshal_objective_dsl_path =
      cmd_json_string(cmd_json_field(value, "marshal_objective_dsl_path"));
  session.marshal_objective_md_path =
      cmd_json_string(cmd_json_field(value, "marshal_objective_md_path"));
  session.marshal_guidance_md_path =
      cmd_json_string(cmd_json_field(value, "marshal_guidance_md_path"));
  session.hero_marshal_dsl_path =
      cmd_json_string(cmd_json_field(value, "hero_marshal_dsl_path"));
  session.config_policy_path =
      cmd_json_string(cmd_json_field(value, "config_policy_path"));
  session.briefing_path =
      cmd_json_string(cmd_json_field(value, "briefing_path"));
  session.memory_path = cmd_json_string(cmd_json_field(value, "memory_path"));
  session.human_request_path =
      cmd_json_string(cmd_json_field(value, "human_request_path"));
  session.events_path = cmd_json_string(cmd_json_field(value, "events_path"));
  session.codex_stdout_path =
      cmd_json_string(cmd_json_field(value, "codex_stdout_path"));
  session.codex_stderr_path =
      cmd_json_string(cmd_json_field(value, "codex_stderr_path"));
  session.current_thread_id =
      cmd_json_string(cmd_json_field(value, "current_thread_id"));
  session.resolved_marshal_codex_binary =
      cmd_json_string(cmd_json_field(value, "resolved_marshal_codex_binary"));
  session.resolved_marshal_codex_model =
      cmd_json_string(cmd_json_field(value, "resolved_marshal_codex_model"));
  session.resolved_marshal_codex_reasoning_effort = cmd_json_string(
      cmd_json_field(value, "resolved_marshal_codex_reasoning_effort"));
  session.started_at_ms = cmd_json_u64(cmd_json_field(value, "started_at_ms"));
  session.updated_at_ms = cmd_json_u64(cmd_json_field(value, "updated_at_ms"));
  session.finished_at_ms =
      cmd_json_optional_u64(cmd_json_field(value, "finished_at_ms"));
  session.checkpoint_count =
      cmd_json_u64(cmd_json_field(value, "checkpoint_count"));
  session.launch_count = cmd_json_u64(cmd_json_field(value, "launch_count"));
  session.max_campaign_launches =
      cmd_json_u64(cmd_json_field(value, "max_campaign_launches"));
  session.remaining_campaign_launches =
      cmd_json_u64(cmd_json_field(value, "remaining_campaign_launches"));
  session.authority_scope = cmd_json_string(
      cmd_json_field(value, "authority_scope"), session.authority_scope);
  session.campaign_status = cmd_json_string(
      cmd_json_field(value, "campaign_status"), session.campaign_status);
  session.campaign_cursor =
      cmd_json_string(cmd_json_field(value, "campaign_cursor"));
  session.last_codex_action =
      cmd_json_string(cmd_json_field(value, "last_codex_action"));
  session.last_warning = cmd_json_string(cmd_json_field(value, "last_warning"));
  session.last_input_checkpoint_path =
      cmd_json_string(cmd_json_field(value, "last_input_checkpoint_path"));
  session.last_intent_checkpoint_path =
      cmd_json_string(cmd_json_field(value, "last_intent_checkpoint_path"));
  session.last_mutation_checkpoint_path =
      cmd_json_string(cmd_json_field(value, "last_mutation_checkpoint_path"));
  session.campaign_cursors =
      cmd_json_string_array(cmd_json_field(value, "campaign_cursors"));
  if (session.marshal_session_id.empty()) {
    if (error)
      *error = "marshal session payload missing marshal_session_id";
    return false;
  }
  *out = std::move(session);
  return true;
}

inline bool runtime_parse_campaign_json(
    const cmd_json_value_t *value,
    cuwacunu::hero::runtime::runtime_campaign_record_t *out,
    std::string *error) {
  if (error)
    error->clear();
  if (value == nullptr || value->type != cmd_json_type_t::OBJECT) {
    if (error)
      *error = "invalid runtime campaign payload";
    return false;
  }
  if (out == nullptr) {
    if (error)
      *error = "missing runtime campaign output";
    return false;
  }

  cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
  campaign.schema =
      cmd_json_string(cmd_json_field(value, "schema"), campaign.schema);
  campaign.campaign_cursor =
      cmd_json_string(cmd_json_field(value, "campaign_cursor"));
  campaign.boot_id = cmd_json_string(cmd_json_field(value, "boot_id"));
  campaign.state =
      cmd_json_string(cmd_json_field(value, "state"), campaign.state);
  campaign.state_detail =
      cmd_json_string(cmd_json_field(value, "state_detail"));
  campaign.marshal_session_id =
      cmd_json_string(cmd_json_field(value, "marshal_session_id"));
  campaign.global_config_path =
      cmd_json_string(cmd_json_field(value, "global_config_path"));
  campaign.source_campaign_dsl_path =
      cmd_json_string(cmd_json_field(value, "source_campaign_dsl_path"));
  campaign.campaign_dsl_path =
      cmd_json_string(cmd_json_field(value, "campaign_dsl_path"));
  campaign.reset_runtime_state =
      cmd_json_bool(cmd_json_field(value, "reset_runtime_state"), false);
  campaign.stdout_path = cmd_json_string(cmd_json_field(value, "stdout_path"));
  campaign.stderr_path = cmd_json_string(cmd_json_field(value, "stderr_path"));
  campaign.started_at_ms = cmd_json_u64(cmd_json_field(value, "started_at_ms"));
  campaign.updated_at_ms = cmd_json_u64(cmd_json_field(value, "updated_at_ms"));
  campaign.finished_at_ms =
      cmd_json_optional_u64(cmd_json_field(value, "finished_at_ms"));
  campaign.runner_pid =
      cmd_json_optional_u64(cmd_json_field(value, "runner_pid"));
  campaign.runner_start_ticks =
      cmd_json_optional_u64(cmd_json_field(value, "runner_start_ticks"));
  campaign.current_run_index =
      cmd_json_optional_u64(cmd_json_field(value, "current_run_index"));
  campaign.run_bind_ids =
      cmd_json_string_array(cmd_json_field(value, "run_bind_ids"));
  campaign.job_cursors =
      cmd_json_string_array(cmd_json_field(value, "job_cursors"));
  if (campaign.campaign_cursor.empty()) {
    if (error)
      *error = "runtime campaign payload missing campaign_cursor";
    return false;
  }
  *out = std::move(campaign);
  return true;
}

inline bool
runtime_parse_job_json(const cmd_json_value_t *value,
                       cuwacunu::hero::runtime::runtime_job_record_t *out,
                       std::string *error) {
  if (error)
    error->clear();
  if (value == nullptr || value->type != cmd_json_type_t::OBJECT) {
    if (error)
      *error = "invalid runtime job payload";
    return false;
  }
  if (out == nullptr) {
    if (error)
      *error = "missing runtime job output";
    return false;
  }

  cuwacunu::hero::runtime::runtime_job_record_t job{};
  job.schema = cmd_json_string(cmd_json_field(value, "schema"), job.schema);
  job.job_cursor = cmd_json_string(cmd_json_field(value, "job_cursor"));
  job.job_kind =
      cmd_json_string(cmd_json_field(value, "job_kind"), job.job_kind);
  job.boot_id = cmd_json_string(cmd_json_field(value, "boot_id"));
  job.state = cmd_json_string(cmd_json_field(value, "state"), job.state);
  job.state_detail = cmd_json_string(cmd_json_field(value, "state_detail"));
  job.worker_binary = cmd_json_string(cmd_json_field(value, "worker_binary"));
  job.worker_command = cmd_json_string(cmd_json_field(value, "worker_command"));
  job.global_config_path =
      cmd_json_string(cmd_json_field(value, "global_config_path"));
  job.source_campaign_dsl_path =
      cmd_json_string(cmd_json_field(value, "source_campaign_dsl_path"));
  job.source_contract_dsl_path =
      cmd_json_string(cmd_json_field(value, "source_contract_dsl_path"));
  job.source_wave_dsl_path =
      cmd_json_string(cmd_json_field(value, "source_wave_dsl_path"));
  job.campaign_dsl_path =
      cmd_json_string(cmd_json_field(value, "campaign_dsl_path"));
  job.contract_dsl_path =
      cmd_json_string(cmd_json_field(value, "contract_dsl_path"));
  job.wave_dsl_path = cmd_json_string(cmd_json_field(value, "wave_dsl_path"));
  job.binding_id = cmd_json_string(cmd_json_field(value, "binding_id"));
  job.requested_device =
      cmd_json_string(cmd_json_field(value, "requested_device"));
  job.resolved_device =
      cmd_json_string(cmd_json_field(value, "resolved_device"));
  job.device_source_section =
      cmd_json_string(cmd_json_field(value, "device_source_section"));
  job.device_contract_hash =
      cmd_json_string(cmd_json_field(value, "device_contract_hash"));
  job.device_error = cmd_json_string(cmd_json_field(value, "device_error"));
  job.cuda_required =
      cmd_json_bool(cmd_json_field(value, "cuda_required"), false);
  job.reset_runtime_state =
      cmd_json_bool(cmd_json_field(value, "reset_runtime_state"), false);
  job.stdout_path = cmd_json_string(cmd_json_field(value, "stdout_path"));
  job.stderr_path = cmd_json_string(cmd_json_field(value, "stderr_path"));
  job.trace_path = cmd_json_string(cmd_json_field(value, "trace_path"));
  job.started_at_ms = cmd_json_u64(cmd_json_field(value, "started_at_ms"));
  job.updated_at_ms = cmd_json_u64(cmd_json_field(value, "updated_at_ms"));
  job.finished_at_ms =
      cmd_json_optional_u64(cmd_json_field(value, "finished_at_ms"));
  job.runner_pid = cmd_json_optional_u64(cmd_json_field(value, "runner_pid"));
  job.runner_start_ticks =
      cmd_json_optional_u64(cmd_json_field(value, "runner_start_ticks"));
  job.target_pid = cmd_json_optional_u64(cmd_json_field(value, "target_pid"));
  job.target_pgid = cmd_json_optional_u64(cmd_json_field(value, "target_pgid"));
  job.target_start_ticks =
      cmd_json_optional_u64(cmd_json_field(value, "target_start_ticks"));
  job.exit_code = cmd_json_optional_i64(cmd_json_field(value, "exit_code"));
  job.term_signal = cmd_json_optional_i64(cmd_json_field(value, "term_signal"));
  if (job.job_cursor.empty()) {
    if (error)
      *error = "runtime job payload missing job_cursor";
    return false;
  }
  *out = std::move(job);
  return true;
}

inline bool runtime_invoke_runtime_tool(
    cuwacunu::hero::runtime_mcp::app_context_t *app,
    const std::string &tool_name, const std::string &arguments_json,
    cmd_json_value_t *out_structured, std::string *error) {
  if (error)
    error->clear();
  std::string result_json{};
  if (app == nullptr) {
    if (error)
      *error = "missing Runtime Hero app context";
    return false;
  }
  if (!cuwacunu::hero::runtime_mcp::execute_tool_json(
          tool_name, arguments_json, app, &result_json, error)) {
    return false;
  }
  return cmd_parse_tool_structured_content(result_json, out_structured, error);
}

inline bool runtime_invoke_runtime_tool(CmdState &st,
                                        const std::string &tool_name,
                                        const std::string &arguments_json,
                                        cmd_json_value_t *out_structured,
                                        std::string *error) {
  return runtime_invoke_runtime_tool(&st.runtime.app, tool_name, arguments_json,
                                     out_structured, error);
}

inline bool runtime_invoke_marshal_tool(
    cuwacunu::hero::marshal_mcp::app_context_t *app,
    const std::string &tool_name, const std::string &arguments_json,
    cmd_json_value_t *out_structured, std::string *error) {
  if (error)
    error->clear();
  std::string result_json{};
  if (app == nullptr) {
    if (error)
      *error = "missing Marshal Hero app context";
    return false;
  }
  if (!cuwacunu::hero::marshal_mcp::execute_tool_json(
          tool_name, arguments_json, app, &result_json, error)) {
    return false;
  }
  return cmd_parse_tool_structured_content(result_json, out_structured, error);
}

inline bool runtime_invoke_marshal_tool(CmdState &st,
                                        const std::string &tool_name,
                                        const std::string &arguments_json,
                                        cmd_json_value_t *out_structured,
                                        std::string *error) {
  return runtime_invoke_marshal_tool(&st.runtime.marshal_app, tool_name,
                                     arguments_json, out_structured, error);
}

inline bool runtime_load_sessions_via_hero(
    bool marshal_ok, const std::string &marshal_error,
    cuwacunu::hero::marshal_mcp::app_context_t *marshal_app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out,
    std::string *error) {
  if (error)
    error->clear();
  if (out == nullptr) {
    if (error)
      *error = "missing runtime sessions output";
    return false;
  }
  out->clear();
  if (!marshal_ok) {
    if (error) {
      *error = marshal_error.empty() ? "Marshal Hero defaults unavailable"
                                     : marshal_error;
    }
    return false;
  }

  cmd_json_value_t structured{};
  if (!runtime_invoke_marshal_tool(marshal_app, "hero.marshal.list_sessions",
                                   "{}", &structured, error)) {
    return false;
  }
  const auto *sessions = cmd_json_field(&structured, "sessions");
  if (sessions == nullptr || sessions->type != cmd_json_type_t::ARRAY ||
      !sessions->arrayValue) {
    if (error)
      *error = "hero.marshal.list_sessions missing sessions array";
    return false;
  }
  out->reserve(sessions->arrayValue->size());
  for (const auto &entry : *sessions->arrayValue) {
    cuwacunu::hero::marshal::marshal_session_record_t session{};
    if (!runtime_parse_marshal_session_json(&entry, &session, error))
      return false;
    out->push_back(std::move(session));
  }
  std::sort(out->begin(), out->end(), [](const auto &a, const auto &b) {
    if (a.updated_at_ms != b.updated_at_ms)
      return a.updated_at_ms > b.updated_at_ms;
    return a.marshal_session_id > b.marshal_session_id;
  });
  return true;
}

inline bool runtime_load_campaigns_via_hero(
    cuwacunu::hero::runtime_mcp::app_context_t *runtime_app,
    std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> *out,
    std::string *error) {
  if (error)
    error->clear();
  if (out == nullptr) {
    if (error)
      *error = "missing runtime campaigns output";
    return false;
  }
  out->clear();

  cmd_json_value_t structured{};
  if (!runtime_invoke_runtime_tool(runtime_app, "hero.runtime.list_campaigns",
                                   "{}", &structured, error)) {
    return false;
  }
  const auto *campaigns = cmd_json_field(&structured, "campaigns");
  if (campaigns == nullptr || campaigns->type != cmd_json_type_t::ARRAY ||
      !campaigns->arrayValue) {
    if (error)
      *error = "hero.runtime.list_campaigns missing campaigns array";
    return false;
  }
  out->reserve(campaigns->arrayValue->size());
  for (const auto &entry : *campaigns->arrayValue) {
    cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
    if (!runtime_parse_campaign_json(&entry, &campaign, error))
      return false;
    out->push_back(std::move(campaign));
  }
  std::sort(out->begin(), out->end(), [](const auto &a, const auto &b) {
    if (a.updated_at_ms != b.updated_at_ms)
      return a.updated_at_ms > b.updated_at_ms;
    return a.campaign_cursor > b.campaign_cursor;
  });
  return true;
}

inline bool runtime_load_jobs_via_hero(
    cuwacunu::hero::runtime_mcp::app_context_t *runtime_app,
    std::vector<cuwacunu::hero::runtime::runtime_job_record_t> *out,
    std::string *error) {
  if (error)
    error->clear();
  if (out == nullptr) {
    if (error)
      *error = "missing runtime jobs output";
    return false;
  }
  out->clear();

  cmd_json_value_t structured{};
  if (!runtime_invoke_runtime_tool(runtime_app, "hero.runtime.list_jobs", "{}",
                                   &structured, error)) {
    return false;
  }
  const auto *jobs = cmd_json_field(&structured, "jobs");
  if (jobs == nullptr || jobs->type != cmd_json_type_t::ARRAY ||
      !jobs->arrayValue) {
    if (error)
      *error = "hero.runtime.list_jobs missing jobs array";
    return false;
  }
  out->reserve(jobs->arrayValue->size());
  for (const auto &entry : *jobs->arrayValue) {
    cuwacunu::hero::runtime::runtime_job_record_t job{};
    if (!runtime_parse_job_json(&entry, &job, error))
      return false;
    out->push_back(std::move(job));
  }
  std::sort(out->begin(), out->end(), [](const auto &a, const auto &b) {
    if (a.updated_at_ms != b.updated_at_ms)
      return a.updated_at_ms > b.updated_at_ms;
    return a.job_cursor > b.job_cursor;
  });
  return true;
}

inline bool runtime_load_sessions_via_hero(
    CmdState &st,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out,
    std::string *error) {
  return runtime_load_sessions_via_hero(st.runtime.marshal_ok,
                                        st.runtime.marshal_error,
                                        &st.runtime.marshal_app, out, error);
}

inline bool runtime_load_campaigns_via_hero(
    CmdState &st,
    std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> *out,
    std::string *error) {
  return runtime_load_campaigns_via_hero(&st.runtime.app, out, error);
}

inline bool runtime_load_jobs_via_hero(
    CmdState &st,
    std::vector<cuwacunu::hero::runtime::runtime_job_record_t> *out,
    std::string *error) {
  return runtime_load_jobs_via_hero(&st.runtime.app, out, error);
}

inline void runtime_assign_excerpt_from_tool(CmdState &st,
                                             const std::string &tool_name,
                                             const std::string &arguments_json,
                                             std::string *out_excerpt) {
  if (out_excerpt == nullptr)
    return;
  out_excerpt->clear();

  cmd_json_value_t structured{};
  std::string error{};
  if (!runtime_invoke_runtime_tool(st, tool_name, arguments_json, &structured,
                                   &error)) {
    *out_excerpt = "<" + tool_name + " unavailable: " + error + ">";
    return;
  }
  *out_excerpt = cmd_json_string(cmd_json_field(&structured, "text"));
  if (out_excerpt->empty())
    *out_excerpt = "<empty>";
}

inline void runtime_assign_excerpt_from_tool(
    cuwacunu::hero::runtime_mcp::app_context_t *runtime_app,
    const std::string &tool_name, const std::string &arguments_json,
    std::string *out_excerpt) {
  if (out_excerpt == nullptr)
    return;
  out_excerpt->clear();

  cmd_json_value_t structured{};
  std::string error{};
  if (!runtime_invoke_runtime_tool(runtime_app, tool_name, arguments_json,
                                   &structured, &error)) {
    *out_excerpt = "<" + tool_name + " unavailable: " + error + ">";
    return;
  }
  *out_excerpt = cmd_json_string(cmd_json_field(&structured, "text"));
  if (out_excerpt->empty())
    *out_excerpt = "<empty>";
}

inline RuntimeExcerptSnapshot runtime_collect_excerpt_snapshot(
    cuwacunu::hero::runtime_mcp::app_context_t runtime_app,
    const cuwacunu::hero::runtime::runtime_job_record_t &job,
    RuntimeDetailMode detail_mode) {
  RuntimeExcerptSnapshot snapshot{};
  snapshot.job_cursor = job.job_cursor;
  snapshot.detail_mode = detail_mode;
  switch (detail_mode) {
  case RuntimeDetailMode::Manifest:
    return snapshot;
  case RuntimeDetailMode::Log:
    if (!job.stdout_path.empty()) {
      runtime_assign_excerpt_from_tool(
          &runtime_app, "hero.runtime.tail_log",
          std::string("{\"job_cursor\":") + config_json_quote(job.job_cursor) +
              ",\"stream\":\"stdout\",\"lines\":10}",
          &snapshot.stdout_excerpt);
    }
    if (!job.stderr_path.empty()) {
      runtime_assign_excerpt_from_tool(
          &runtime_app, "hero.runtime.tail_log",
          std::string("{\"job_cursor\":") + config_json_quote(job.job_cursor) +
              ",\"stream\":\"stderr\",\"lines\":10}",
          &snapshot.stderr_excerpt);
    }
    return snapshot;
  case RuntimeDetailMode::Trace:
    if (!job.trace_path.empty()) {
      runtime_assign_excerpt_from_tool(&runtime_app, "hero.runtime.tail_trace",
                                       std::string("{\"job_cursor\":") +
                                           config_json_quote(job.job_cursor) +
                                           ",\"lines\":14}",
                                       &snapshot.trace_excerpt);
    }
    return snapshot;
  }
  return snapshot;
}

inline void refresh_runtime_job_excerpts(CmdState &st) {
  clear_runtime_job_excerpts(st);
  if (!st.runtime.job.has_value())
    return;
  const auto &job = *st.runtime.job;
  switch (st.runtime.detail_mode) {
  case RuntimeDetailMode::Manifest:
    return;
  case RuntimeDetailMode::Log:
    if (!job.stdout_path.empty()) {
      runtime_assign_excerpt_from_tool(
          st, "hero.runtime.tail_log",
          std::string("{\"job_cursor\":") + config_json_quote(job.job_cursor) +
              ",\"stream\":\"stdout\",\"lines\":10}",
          &st.runtime.job_stdout_excerpt);
    }
    if (!job.stderr_path.empty()) {
      runtime_assign_excerpt_from_tool(
          st, "hero.runtime.tail_log",
          std::string("{\"job_cursor\":") + config_json_quote(job.job_cursor) +
              ",\"stream\":\"stderr\",\"lines\":10}",
          &st.runtime.job_stderr_excerpt);
    }
    return;
  case RuntimeDetailMode::Trace:
    if (!job.trace_path.empty()) {
      runtime_assign_excerpt_from_tool(st, "hero.runtime.tail_trace",
                                       std::string("{\"job_cursor\":") +
                                           config_json_quote(job.job_cursor) +
                                           ",\"lines\":14}",
                                       &st.runtime.job_trace_excerpt);
    }
    return;
  }
}

inline void focus_runtime_menu(CmdState &st) {
  st.runtime.focus = kRuntimeMenuFocus;
}

inline void focus_runtime_worklist(CmdState &st) {
  st.runtime.focus = kRuntimeWorklistFocus;
}

inline void clear_runtime_drill_filters(CmdState &st) {
  st.runtime.campaign_filter_active = false;
  st.runtime.job_filter_active = false;
}

inline bool queue_runtime_excerpt_refresh(CmdState &st);
inline bool poll_runtime_async_updates(CmdState &st);

inline std::string
runtime_session_id_for_campaign(const RuntimeState &runtime,
                                std::string_view campaign_cursor);

inline std::string runtime_campaign_cursor_for_job(const RuntimeState &runtime,
                                                   std::string_view job_cursor);

inline std::vector<std::size_t>
runtime_child_campaign_indices(const CmdState &st);

inline std::vector<std::size_t>
runtime_orphan_campaign_indices(const CmdState &st) {
  std::vector<std::size_t> indices{};
  indices.reserve(st.runtime.campaigns.size());
  for (std::size_t i = 0; i < st.runtime.campaigns.size(); ++i) {
    if (runtime_session_id_for_campaign(st.runtime,
                                        st.runtime.campaigns[i].campaign_cursor)
            .empty()) {
      indices.push_back(i);
    }
  }
  return indices;
}

inline std::vector<std::size_t> runtime_orphan_job_indices(const CmdState &st) {
  std::vector<std::size_t> indices{};
  indices.reserve(st.runtime.jobs.size());
  for (std::size_t i = 0; i < st.runtime.jobs.size(); ++i) {
    if (runtime_campaign_cursor_for_job(st.runtime,
                                        st.runtime.jobs[i].job_cursor)
            .empty()) {
      indices.push_back(i);
    }
  }
  return indices;
}

inline bool runtime_has_none_session_row(const CmdState &st) {
  return !runtime_orphan_campaign_indices(st).empty() ||
         !runtime_orphan_job_indices(st).empty();
}

inline bool runtime_has_none_campaign_row(const CmdState &st) {
  return !runtime_orphan_job_indices(st).empty();
}

inline bool runtime_child_has_none_campaign_row(const CmdState &st) {
  return runtime_is_none_session_selection(st.runtime.selected_session_id) &&
         runtime_has_none_campaign_row(st);
}

inline std::size_t runtime_session_row_count(const CmdState &st) {
  return st.runtime.sessions.size() +
         (runtime_has_none_session_row(st) ? 1u : 0u);
}

inline std::size_t runtime_campaign_row_count(const CmdState &st) {
  return st.runtime.campaigns.size() +
         (runtime_has_none_campaign_row(st) ? 1u : 0u);
}

inline std::size_t runtime_child_campaign_row_count(const CmdState &st) {
  return runtime_child_campaign_indices(st).size() +
         (runtime_child_has_none_campaign_row(st) ? 1u : 0u);
}

inline std::vector<std::size_t>
runtime_child_campaign_indices(const CmdState &st) {
  if (st.runtime.selected_session_id.empty())
    return {};
  if (runtime_is_none_session_selection(st.runtime.selected_session_id)) {
    return runtime_orphan_campaign_indices(st);
  }
  const auto it = st.runtime.campaign_indices_by_session.find(
      st.runtime.selected_session_id);
  if (it == st.runtime.campaign_indices_by_session.end())
    return {};
  return it->second;
}

inline std::vector<std::size_t> runtime_child_job_indices(const CmdState &st) {
  if (st.runtime.selected_campaign_cursor.empty())
    return {};
  if (runtime_is_none_campaign_selection(st.runtime.selected_campaign_cursor)) {
    return runtime_orphan_job_indices(st);
  }
  const auto it = st.runtime.job_indices_by_campaign.find(
      st.runtime.selected_campaign_cursor);
  if (it == st.runtime.job_indices_by_campaign.end())
    return {};
  return it->second;
}

inline std::vector<std::size_t>
runtime_visible_campaign_indices(const CmdState &st) {
  std::vector<std::size_t> indices{};
  indices.reserve(st.runtime.campaigns.size());
  for (std::size_t i = 0; i < st.runtime.campaigns.size(); ++i) {
    indices.push_back(i);
  }
  return indices;
}

inline std::vector<std::size_t>
runtime_visible_job_indices(const CmdState &st) {
  std::vector<std::size_t> indices{};
  indices.reserve(st.runtime.jobs.size());
  for (std::size_t i = 0; i < st.runtime.jobs.size(); ++i) {
    indices.push_back(i);
  }
  return indices;
}

inline std::size_t runtime_lane_count(const CmdState &st, RuntimeLane lane) {
  if (runtime_is_device_lane(lane))
    return runtime_device_item_count(st);
  if (runtime_is_session_lane(lane))
    return runtime_session_row_count(st);
  if (runtime_is_campaign_lane(lane))
    return runtime_campaign_row_count(st);
  return st.runtime.jobs.size();
}

inline std::size_t current_runtime_lane_count(const CmdState &st) {
  return runtime_lane_count(st, st.runtime.lane);
}

inline bool runtime_has_worklist_items(const CmdState &st) {
  if (runtime_is_device_lane(st.runtime.lane)) {
    return runtime_device_item_count(st) != 0;
  }
  if (runtime_is_session_lane(st.runtime.lane) &&
      st.runtime.job_filter_active) {
    return !runtime_child_job_indices(st).empty();
  }
  if (runtime_is_session_lane(st.runtime.lane) &&
      st.runtime.campaign_filter_active) {
    return !runtime_child_campaign_indices(st).empty() ||
           runtime_child_has_none_campaign_row(st);
  }
  if (runtime_is_campaign_lane(st.runtime.lane) &&
      st.runtime.job_filter_active) {
    return !runtime_child_job_indices(st).empty();
  }
  return current_runtime_lane_count(st) != 0;
}

inline std::optional<std::size_t>
runtime_selected_session_index(const CmdState &st) {
  const bool has_none = runtime_has_none_session_row(st);
  if (st.runtime.sessions.empty() && !has_none)
    return std::nullopt;
  if (runtime_is_none_session_selection(st.runtime.selected_session_id)) {
    return has_none ? std::optional<std::size_t>{st.runtime.sessions.size()}
                    : std::nullopt;
  }
  if (st.runtime.selected_session_id.empty()) {
    if (!st.runtime.sessions.empty())
      return std::size_t{0};
    if (has_none)
      return std::optional<std::size_t>{0};
    return std::nullopt;
  }
  if (!st.runtime.sessions.empty()) {
    return runtime_find_session_index(st.runtime.sessions,
                                      st.runtime.selected_session_id)
        .value_or(std::size_t{0});
  }
  if (has_none)
    return std::size_t{0};
  return std::nullopt;
}

inline std::optional<std::size_t>
runtime_selected_campaign_index(const CmdState &st) {
  const auto visible = runtime_visible_campaign_indices(st);
  const bool has_none = runtime_has_none_campaign_row(st);
  if (visible.empty() && !has_none)
    return std::nullopt;
  if (runtime_is_none_campaign_selection(st.runtime.selected_campaign_cursor)) {
    return has_none ? std::optional<std::size_t>{visible.size()} : std::nullopt;
  }
  if (st.runtime.selected_campaign_cursor.empty()) {
    if (!visible.empty())
      return std::size_t{0};
    if (has_none)
      return std::optional<std::size_t>{0};
    return std::nullopt;
  }
  for (std::size_t i = 0; i < visible.size(); ++i) {
    if (st.runtime.campaigns[visible[i]].campaign_cursor ==
        st.runtime.selected_campaign_cursor) {
      return i;
    }
  }
  if (!visible.empty())
    return std::size_t{0};
  if (has_none)
    return std::optional<std::size_t>{0};
  return std::nullopt;
}

inline std::optional<std::size_t>
runtime_selected_child_campaign_index(const CmdState &st) {
  const auto visible = runtime_child_campaign_indices(st);
  const bool has_none = runtime_child_has_none_campaign_row(st);
  if (visible.empty() && !has_none)
    return std::nullopt;
  if (runtime_is_none_campaign_selection(st.runtime.selected_campaign_cursor)) {
    return has_none ? std::optional<std::size_t>{visible.size()} : std::nullopt;
  }
  if (st.runtime.selected_campaign_cursor.empty()) {
    if (!visible.empty())
      return std::size_t{0};
    if (has_none)
      return std::optional<std::size_t>{0};
    return std::nullopt;
  }
  for (std::size_t i = 0; i < visible.size(); ++i) {
    if (st.runtime.campaigns[visible[i]].campaign_cursor ==
        st.runtime.selected_campaign_cursor) {
      return i;
    }
  }
  if (!visible.empty())
    return std::size_t{0};
  if (has_none)
    return std::optional<std::size_t>{0};
  return std::nullopt;
}

inline std::optional<std::size_t>
runtime_selected_job_index(const CmdState &st) {
  const auto visible = runtime_visible_job_indices(st);
  if (visible.empty())
    return std::nullopt;
  if (st.runtime.selected_job_cursor.empty())
    return std::size_t{0};
  for (std::size_t i = 0; i < visible.size(); ++i) {
    if (st.runtime.jobs[visible[i]].job_cursor ==
        st.runtime.selected_job_cursor) {
      return i;
    }
  }
  return std::size_t{0};
}

inline std::optional<std::size_t>
runtime_selected_child_job_index(const CmdState &st) {
  const auto visible = runtime_child_job_indices(st);
  if (visible.empty())
    return std::nullopt;
  if (st.runtime.selected_job_cursor.empty())
    return std::size_t{0};
  for (std::size_t i = 0; i < visible.size(); ++i) {
    if (st.runtime.jobs[visible[i]].job_cursor ==
        st.runtime.selected_job_cursor) {
      return i;
    }
  }
  return std::size_t{0};
}

inline std::optional<std::size_t>
runtime_current_lane_selected_index(const CmdState &st) {
  if (runtime_is_device_lane(st.runtime.lane)) {
    return runtime_selected_device_index(st);
  }
  if (runtime_is_session_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active) {
      return runtime_selected_child_job_index(st);
    }
    if (st.runtime.campaign_filter_active) {
      return runtime_selected_child_campaign_index(st);
    }
    return runtime_selected_session_index(st);
  }
  if (runtime_is_campaign_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active) {
      return runtime_selected_child_job_index(st);
    }
    return runtime_selected_campaign_index(st);
  }
  return runtime_selected_job_index(st);
}

inline std::string
runtime_parent_campaign_cursor_from_job_cursor(std::string_view job_cursor) {
  const std::string trimmed = cuwacunu::hero::runtime::trim_ascii(job_cursor);
  const std::size_t marker =
      trimmed.find(cuwacunu::hero::runtime::kRuntimeJobCursorChildMarker);
  if (marker == std::string::npos || marker == 0)
    return {};
  return trimmed.substr(0, marker);
}

inline bool runtime_session_links_campaign(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string_view campaign_cursor) {
  if (campaign_cursor.empty())
    return false;
  if (session.campaign_cursor == campaign_cursor)
    return true;
  return std::find(
             session.campaign_cursors.begin(), session.campaign_cursors.end(),
             std::string(campaign_cursor)) != session.campaign_cursors.end();
}

inline std::string
runtime_session_id_for_campaign(const RuntimeState &runtime,
                                std::string_view campaign_cursor) {
  if (campaign_cursor.empty())
    return {};
  if (const auto it =
          runtime.campaign_session_by_cursor.find(std::string(campaign_cursor));
      it != runtime.campaign_session_by_cursor.end()) {
    return it->second;
  }
  const auto campaign =
      find_runtime_campaign_by_cursor(runtime.campaigns, campaign_cursor);
  if (campaign.has_value() && !campaign->marshal_session_id.empty()) {
    return campaign->marshal_session_id;
  }
  for (const auto &session : runtime.sessions) {
    if (runtime_session_links_campaign(session, campaign_cursor)) {
      return session.marshal_session_id;
    }
  }
  return {};
}

inline std::string
runtime_campaign_cursor_for_job(const RuntimeState &runtime,
                                std::string_view job_cursor) {
  if (const auto it =
          runtime.job_campaign_by_cursor.find(std::string(job_cursor));
      it != runtime.job_campaign_by_cursor.end()) {
    return it->second;
  }
  std::string campaign_cursor =
      runtime_parent_campaign_cursor_from_job_cursor(job_cursor);
  if (!campaign_cursor.empty())
    return campaign_cursor;
  for (const auto &campaign : runtime.campaigns) {
    if (std::find(campaign.job_cursors.begin(), campaign.job_cursors.end(),
                  std::string(job_cursor)) != campaign.job_cursors.end()) {
      return campaign.campaign_cursor;
    }
  }
  return {};
}

inline std::string runtime_session_id_for_job(const RuntimeState &runtime,
                                              std::string_view job_cursor) {
  if (const auto it =
          runtime.job_session_by_cursor.find(std::string(job_cursor));
      it != runtime.job_session_by_cursor.end()) {
    return it->second;
  }
  return runtime_session_id_for_campaign(
      runtime, runtime_campaign_cursor_for_job(runtime, job_cursor));
}

inline void rebuild_runtime_lineage_cache(CmdState &st) {
  st.runtime.campaign_session_by_cursor.clear();
  st.runtime.job_campaign_by_cursor.clear();
  st.runtime.job_session_by_cursor.clear();
  st.runtime.campaign_indices_by_session.clear();
  st.runtime.job_indices_by_campaign.clear();
  st.runtime.job_indices_by_session.clear();
  st.runtime.runner_job_index_by_pid.clear();
  st.runtime.target_job_index_by_pid.clear();

  for (std::size_t i = 0; i < st.runtime.campaigns.size(); ++i) {
    const auto &campaign = st.runtime.campaigns[i];
    std::string marshal_session_id = campaign.marshal_session_id;
    if (marshal_session_id.empty()) {
      for (const auto &session : st.runtime.sessions) {
        if (runtime_session_links_campaign(session, campaign.campaign_cursor)) {
          marshal_session_id = session.marshal_session_id;
          break;
        }
      }
    }
    if (!marshal_session_id.empty()) {
      st.runtime.campaign_session_by_cursor.emplace(campaign.campaign_cursor,
                                                    marshal_session_id);
      st.runtime.campaign_indices_by_session[marshal_session_id].push_back(i);
    }
  }

  for (std::size_t i = 0; i < st.runtime.jobs.size(); ++i) {
    const auto &job = st.runtime.jobs[i];
    if (job.runner_pid.has_value()) {
      st.runtime.runner_job_index_by_pid.emplace(*job.runner_pid, i);
    }
    if (job.target_pid.has_value()) {
      st.runtime.target_job_index_by_pid.emplace(*job.target_pid, i);
    }
    std::string campaign_cursor =
        runtime_parent_campaign_cursor_from_job_cursor(job.job_cursor);
    if (campaign_cursor.empty()) {
      for (const auto &campaign : st.runtime.campaigns) {
        if (std::find(campaign.job_cursors.begin(), campaign.job_cursors.end(),
                      job.job_cursor) != campaign.job_cursors.end()) {
          campaign_cursor = campaign.campaign_cursor;
          break;
        }
      }
    }
    if (campaign_cursor.empty()) {
      continue;
    }

    st.runtime.job_campaign_by_cursor.emplace(job.job_cursor, campaign_cursor);
    st.runtime.job_indices_by_campaign[campaign_cursor].push_back(i);

    const auto session_it =
        st.runtime.campaign_session_by_cursor.find(campaign_cursor);
    if (session_it != st.runtime.campaign_session_by_cursor.end() &&
        !session_it->second.empty()) {
      st.runtime.job_session_by_cursor.emplace(job.job_cursor,
                                               session_it->second);
      st.runtime.job_indices_by_session[session_it->second].push_back(i);
    }
  }
}

inline std::string latest_runtime_campaign_cursor_for_session(
    const RuntimeState &runtime, std::string_view marshal_session_id) {
  const auto session =
      find_runtime_session_by_id(runtime.sessions, marshal_session_id);
  if (session.has_value()) {
    if (!session->campaign_cursor.empty() &&
        runtime_find_campaign_index(runtime.campaigns, session->campaign_cursor)
            .has_value()) {
      return session->campaign_cursor;
    }
    for (auto it = session->campaign_cursors.rbegin();
         it != session->campaign_cursors.rend(); ++it) {
      if (runtime_find_campaign_index(runtime.campaigns, *it).has_value()) {
        return *it;
      }
    }
  }
  for (const auto &campaign : runtime.campaigns) {
    if (runtime_session_id_for_campaign(runtime, campaign.campaign_cursor) ==
        marshal_session_id) {
      return campaign.campaign_cursor;
    }
  }
  return {};
}

inline std::string latest_runtime_job_cursor_for_campaign(
    const RuntimeState &runtime,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign) {
  for (auto it = campaign.job_cursors.rbegin();
       it != campaign.job_cursors.rend(); ++it) {
    if (runtime_find_job_index(runtime.jobs, *it).has_value())
      return *it;
  }
  for (const auto &job : runtime.jobs) {
    if (runtime_parent_campaign_cursor_from_job_cursor(job.job_cursor) ==
        campaign.campaign_cursor) {
      return job.job_cursor;
    }
  }
  return {};
}

inline std::string
latest_runtime_job_cursor_for_campaign(const RuntimeState &runtime,
                                       std::string_view campaign_cursor) {
  const auto campaign =
      find_runtime_campaign_by_cursor(runtime.campaigns, campaign_cursor);
  if (!campaign.has_value())
    return {};
  return latest_runtime_job_cursor_for_campaign(runtime, *campaign);
}

inline std::string latest_runtime_orphan_campaign_cursor(const CmdState &st) {
  const auto orphan_campaigns = runtime_orphan_campaign_indices(st);
  if (orphan_campaigns.empty())
    return {};
  return st.runtime.campaigns[orphan_campaigns.front()].campaign_cursor;
}

inline std::string latest_runtime_orphan_job_cursor(const CmdState &st) {
  const auto orphan_jobs = runtime_orphan_job_indices(st);
  if (orphan_jobs.empty())
    return {};
  return st.runtime.jobs[orphan_jobs.front()].job_cursor;
}

inline void clear_runtime_inventory_selection(CmdState &st) {
  st.runtime.anchor_session_id.clear();
  st.runtime.family_anchor_lane = st.runtime.lane;
  st.runtime.selected_session_id.clear();
  st.runtime.selected_campaign_cursor.clear();
  st.runtime.selected_job_cursor.clear();
  st.runtime.session.reset();
  st.runtime.campaign.reset();
  st.runtime.job.reset();
  clear_runtime_job_excerpts(st);
}

inline void update_runtime_selection_records(CmdState &st) {
  st.runtime.session =
      st.runtime.selected_session_id.empty()
          ? std::nullopt
          : find_runtime_session_by_id(st.runtime.sessions,
                                       st.runtime.selected_session_id);
  st.runtime.campaign =
      st.runtime.selected_campaign_cursor.empty()
          ? std::nullopt
          : find_runtime_campaign_by_cursor(
                st.runtime.campaigns, st.runtime.selected_campaign_cursor);
  st.runtime.job = st.runtime.selected_job_cursor.empty()
                       ? std::nullopt
                       : find_runtime_job_by_cursor(
                             st.runtime.jobs, st.runtime.selected_job_cursor);

  if (st.runtime.session.has_value()) {
    st.runtime.anchor_session_id = st.runtime.session->marshal_session_id;
    return;
  }
  if (st.runtime.campaign.has_value()) {
    st.runtime.anchor_session_id = st.runtime.campaign->marshal_session_id;
    return;
  }
  st.runtime.anchor_session_id.clear();
}

inline bool select_prev_runtime_lane(CmdState &st) {
  const std::size_t idx = runtime_lane_index(st.runtime.lane);
  if (idx == 0)
    return false;
  st.runtime.lane = runtime_lane_from_index(idx - 1);
  clear_runtime_drill_filters(st);
  return true;
}

inline bool select_next_runtime_lane(CmdState &st) {
  const std::size_t idx = runtime_lane_index(st.runtime.lane);
  if (idx >= 3)
    return false;
  st.runtime.lane = runtime_lane_from_index(idx + 1);
  clear_runtime_drill_filters(st);
  return true;
}

inline void
select_runtime_session_and_related(CmdState &st,
                                   std::string_view marshal_session_id) {
  if (runtime_is_none_session_selection(marshal_session_id)) {
    st.runtime.family_anchor_lane = kRuntimeSessionLane;
    st.runtime.selected_session_id = std::string(kRuntimeNoneSessionSelection);
    st.runtime.anchor_session_id.clear();
    st.runtime.selected_campaign_cursor =
        latest_runtime_orphan_campaign_cursor(st);
    if (st.runtime.selected_campaign_cursor.empty() &&
        runtime_has_none_campaign_row(st)) {
      st.runtime.selected_campaign_cursor =
          std::string(kRuntimeNoneCampaignSelection);
    }
    if (runtime_is_none_campaign_selection(
            st.runtime.selected_campaign_cursor)) {
      st.runtime.selected_job_cursor = latest_runtime_orphan_job_cursor(st);
    } else {
      st.runtime.selected_job_cursor = latest_runtime_job_cursor_for_campaign(
          st.runtime, st.runtime.selected_campaign_cursor);
    }
    update_runtime_selection_records(st);
    (void)queue_runtime_excerpt_refresh(st);
    return;
  }
  const auto session_index =
      runtime_find_session_index(st.runtime.sessions, marshal_session_id);
  if (!session_index.has_value()) {
    clear_runtime_inventory_selection(st);
    update_runtime_selection_records(st);
    return;
  }
  st.runtime.family_anchor_lane = kRuntimeSessionLane;
  st.runtime.selected_session_id =
      st.runtime.sessions[*session_index].marshal_session_id;
  st.runtime.anchor_session_id = st.runtime.selected_session_id;
  st.runtime.selected_campaign_cursor =
      latest_runtime_campaign_cursor_for_session(
          st.runtime, st.runtime.selected_session_id);
  st.runtime.selected_job_cursor = latest_runtime_job_cursor_for_campaign(
      st.runtime, st.runtime.selected_campaign_cursor);
  update_runtime_selection_records(st);
  (void)queue_runtime_excerpt_refresh(st);
}

inline void
select_runtime_campaign_and_related(CmdState &st,
                                    std::string_view campaign_cursor) {
  const bool preserve_none_session_context =
      runtime_is_session_lane(st.runtime.lane) &&
      (st.runtime.campaign_filter_active || st.runtime.job_filter_active) &&
      runtime_is_none_session_selection(st.runtime.selected_session_id);
  const auto campaign_index =
      runtime_find_campaign_index(st.runtime.campaigns, campaign_cursor);
  if (!campaign_index.has_value()) {
    clear_runtime_inventory_selection(st);
    update_runtime_selection_records(st);
    return;
  }
  const auto &campaign = st.runtime.campaigns[*campaign_index];
  st.runtime.family_anchor_lane = kRuntimeCampaignLane;
  st.runtime.selected_campaign_cursor = campaign.campaign_cursor;
  st.runtime.selected_session_id =
      runtime_session_id_for_campaign(st.runtime, campaign.campaign_cursor);
  if (st.runtime.selected_session_id.empty() && preserve_none_session_context) {
    st.runtime.selected_session_id = std::string(kRuntimeNoneSessionSelection);
  }
  st.runtime.anchor_session_id = st.runtime.selected_session_id;
  st.runtime.selected_job_cursor =
      latest_runtime_job_cursor_for_campaign(st.runtime, campaign);
  update_runtime_selection_records(st);
  (void)queue_runtime_excerpt_refresh(st);
}

inline void select_runtime_none_campaign_and_related(
    CmdState &st, std::string_view session_selection_for_context = {}) {
  st.runtime.family_anchor_lane = kRuntimeCampaignLane;
  st.runtime.selected_campaign_cursor =
      std::string(kRuntimeNoneCampaignSelection);
  st.runtime.selected_session_id = std::string(session_selection_for_context);
  st.runtime.anchor_session_id.clear();
  st.runtime.selected_job_cursor = latest_runtime_orphan_job_cursor(st);
  update_runtime_selection_records(st);
  (void)queue_runtime_excerpt_refresh(st);
}

inline void select_runtime_job_and_related(CmdState &st,
                                           std::string_view job_cursor) {
  const bool preserve_none_session_context =
      runtime_is_session_lane(st.runtime.lane) &&
      st.runtime.job_filter_active &&
      runtime_is_none_session_selection(st.runtime.selected_session_id);
  const bool preserve_none_campaign_context =
      ((runtime_is_session_lane(st.runtime.lane) &&
        st.runtime.job_filter_active) ||
       (runtime_is_campaign_lane(st.runtime.lane) &&
        st.runtime.job_filter_active)) &&
      runtime_is_none_campaign_selection(st.runtime.selected_campaign_cursor);
  const auto job_index = runtime_find_job_index(st.runtime.jobs, job_cursor);
  if (!job_index.has_value()) {
    clear_runtime_inventory_selection(st);
    update_runtime_selection_records(st);
    return;
  }
  const auto &job = st.runtime.jobs[*job_index];
  st.runtime.family_anchor_lane = kRuntimeJobLane;
  st.runtime.selected_job_cursor = job.job_cursor;
  st.runtime.selected_campaign_cursor =
      runtime_campaign_cursor_for_job(st.runtime, job.job_cursor);
  if (st.runtime.selected_campaign_cursor.empty() &&
      preserve_none_campaign_context) {
    st.runtime.selected_campaign_cursor =
        std::string(kRuntimeNoneCampaignSelection);
  }
  st.runtime.selected_session_id =
      runtime_session_id_for_job(st.runtime, job.job_cursor);
  if (st.runtime.selected_session_id.empty() && preserve_none_session_context) {
    st.runtime.selected_session_id = std::string(kRuntimeNoneSessionSelection);
  }
  st.runtime.anchor_session_id = st.runtime.selected_session_id;
  update_runtime_selection_records(st);
  (void)queue_runtime_excerpt_refresh(st);
}

inline bool runtime_select_session_row_at_index(CmdState &st,
                                                std::size_t index) {
  if (index < st.runtime.sessions.size()) {
    select_runtime_session_and_related(
        st, st.runtime.sessions[index].marshal_session_id);
    return true;
  }
  if (runtime_has_none_session_row(st) && index == st.runtime.sessions.size()) {
    select_runtime_session_and_related(st, kRuntimeNoneSessionSelection);
    return true;
  }
  return false;
}

inline bool runtime_select_campaign_row_at_index(CmdState &st,
                                                 std::size_t index) {
  const auto visible = runtime_visible_campaign_indices(st);
  if (index < visible.size()) {
    select_runtime_campaign_and_related(
        st, st.runtime.campaigns[visible[index]].campaign_cursor);
    return true;
  }
  if (runtime_has_none_campaign_row(st) && index == visible.size()) {
    select_runtime_none_campaign_and_related(st);
    return true;
  }
  return false;
}

inline bool runtime_select_child_campaign_row_at_index(CmdState &st,
                                                       std::size_t index) {
  const auto visible = runtime_child_campaign_indices(st);
  if (index < visible.size()) {
    select_runtime_campaign_and_related(
        st, st.runtime.campaigns[visible[index]].campaign_cursor);
    return true;
  }
  if (runtime_child_has_none_campaign_row(st) && index == visible.size()) {
    select_runtime_none_campaign_and_related(st,
                                             st.runtime.selected_session_id);
    return true;
  }
  return false;
}

inline bool select_prev_runtime_item(CmdState &st) {
  if (runtime_is_device_lane(st.runtime.lane)) {
    const std::size_t idx = runtime_selected_device_index(st).value_or(0);
    if (idx == 0)
      return false;
    st.runtime.selected_device_index = idx - 1;
    return true;
  }
  if (runtime_is_session_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active) {
      const auto visible = runtime_child_job_indices(st);
      if (visible.empty())
        return false;
      const std::size_t idx = runtime_selected_child_job_index(st).value_or(0);
      if (idx == 0)
        return false;
      select_runtime_job_and_related(
          st, st.runtime.jobs[visible[idx - 1]].job_cursor);
      return true;
    }
    if (st.runtime.campaign_filter_active) {
      const std::size_t total = runtime_child_campaign_row_count(st);
      if (total == 0)
        return false;
      const std::size_t idx =
          runtime_selected_child_campaign_index(st).value_or(0);
      if (idx == 0)
        return false;
      return runtime_select_child_campaign_row_at_index(st, idx - 1);
    }
    const std::size_t total = runtime_session_row_count(st);
    if (total == 0)
      return false;
    const std::size_t idx = runtime_selected_session_index(st).value_or(0);
    if (idx == 0)
      return false;
    return runtime_select_session_row_at_index(st, idx - 1);
  }
  if (runtime_is_campaign_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active) {
      const auto visible = runtime_child_job_indices(st);
      if (visible.empty())
        return false;
      const std::size_t idx = runtime_selected_child_job_index(st).value_or(0);
      if (idx == 0)
        return false;
      select_runtime_job_and_related(
          st, st.runtime.jobs[visible[idx - 1]].job_cursor);
      return true;
    }
    const std::size_t total = runtime_campaign_row_count(st);
    if (total == 0)
      return false;
    const std::size_t idx = runtime_selected_campaign_index(st).value_or(0);
    if (idx == 0)
      return false;
    return runtime_select_campaign_row_at_index(st, idx - 1);
  }
  const auto visible = runtime_visible_job_indices(st);
  if (visible.empty())
    return false;
  const std::size_t idx = runtime_selected_job_index(st).value_or(0);
  if (idx == 0)
    return false;
  select_runtime_job_and_related(st,
                                 st.runtime.jobs[visible[idx - 1]].job_cursor);
  return true;
}

inline bool select_next_runtime_item(CmdState &st) {
  if (runtime_is_device_lane(st.runtime.lane)) {
    const std::size_t idx = runtime_selected_device_index(st).value_or(0);
    const std::size_t total = runtime_device_item_count(st);
    if (idx + 1 >= total)
      return false;
    st.runtime.selected_device_index = idx + 1;
    return true;
  }
  if (runtime_is_session_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active) {
      const auto visible = runtime_child_job_indices(st);
      if (visible.empty())
        return false;
      const std::size_t idx = runtime_selected_child_job_index(st).value_or(0);
      if (idx + 1 >= visible.size())
        return false;
      select_runtime_job_and_related(
          st, st.runtime.jobs[visible[idx + 1]].job_cursor);
      return true;
    }
    if (st.runtime.campaign_filter_active) {
      const std::size_t total = runtime_child_campaign_row_count(st);
      if (total == 0)
        return false;
      const std::size_t idx =
          runtime_selected_child_campaign_index(st).value_or(0);
      if (idx + 1 >= total)
        return false;
      return runtime_select_child_campaign_row_at_index(st, idx + 1);
    }
    const std::size_t total = runtime_session_row_count(st);
    if (total == 0)
      return false;
    const std::size_t idx = runtime_selected_session_index(st).value_or(0);
    if (idx + 1 >= total)
      return false;
    return runtime_select_session_row_at_index(st, idx + 1);
  }
  if (runtime_is_campaign_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active) {
      const auto visible = runtime_child_job_indices(st);
      if (visible.empty())
        return false;
      const std::size_t idx = runtime_selected_child_job_index(st).value_or(0);
      if (idx + 1 >= visible.size())
        return false;
      select_runtime_job_and_related(
          st, st.runtime.jobs[visible[idx + 1]].job_cursor);
      return true;
    }
    const std::size_t total = runtime_campaign_row_count(st);
    if (total == 0)
      return false;
    const std::size_t idx = runtime_selected_campaign_index(st).value_or(0);
    if (idx + 1 >= total)
      return false;
    return runtime_select_campaign_row_at_index(st, idx + 1);
  }
  const auto visible = runtime_visible_job_indices(st);
  if (visible.empty())
    return false;
  const std::size_t idx = runtime_selected_job_index(st).value_or(0);
  if (idx + 1 >= visible.size())
    return false;
  select_runtime_job_and_related(st,
                                 st.runtime.jobs[visible[idx + 1]].job_cursor);
  return true;
}

inline std::string runtime_detail_mode_label(RuntimeDetailMode mode) {
  switch (mode) {
  case RuntimeDetailMode::Manifest:
    return "manifest";
  case RuntimeDetailMode::Log:
    return "log";
  case RuntimeDetailMode::Trace:
    return "trace";
  }
  return "manifest";
}

inline void cycle_runtime_detail_mode(CmdState &st) {
  switch (st.runtime.detail_mode) {
  case RuntimeDetailMode::Manifest:
    st.runtime.detail_mode = RuntimeDetailMode::Log;
    break;
  case RuntimeDetailMode::Log:
    st.runtime.detail_mode = RuntimeDetailMode::Trace;
    break;
  case RuntimeDetailMode::Trace:
    st.runtime.detail_mode = RuntimeDetailMode::Manifest;
    break;
  }
  (void)queue_runtime_excerpt_refresh(st);
}

inline void set_runtime_status(CmdState &st, std::string status,
                               bool is_error) {
  st.runtime.status = std::move(status);
  st.runtime.status_is_error = is_error;
  if (is_error) {
    st.runtime.ok = false;
    st.runtime.error = st.runtime.status;
  } else {
    st.runtime.error.clear();
  }
}

inline RuntimeRefreshSnapshot runtime_collect_refresh_snapshot(
    cuwacunu::hero::runtime_mcp::app_context_t runtime_app, bool marshal_ok,
    cuwacunu::hero::marshal_mcp::app_context_t marshal_app,
    std::string marshal_error) {
  RuntimeRefreshSnapshot snapshot{};
  if (runtime_app.defaults.campaigns_root.empty()) {
    snapshot.error = "Runtime campaigns_root is not configured.";
    return snapshot;
  }

  if (marshal_ok) {
    if (!runtime_load_sessions_via_hero(marshal_ok, marshal_error, &marshal_app,
                                        &snapshot.sessions,
                                        &snapshot.session_list_error)) {
      snapshot.sessions.clear();
    }
  } else if (!marshal_error.empty()) {
    snapshot.session_list_error = std::move(marshal_error);
  }

  std::string campaign_error{};
  if (!runtime_load_campaigns_via_hero(&runtime_app, &snapshot.campaigns,
                                       &campaign_error)) {
    snapshot.error = campaign_error.empty() ? "failed to load runtime campaigns"
                                            : campaign_error;
    return snapshot;
  }

  if (!runtime_load_jobs_via_hero(&runtime_app, &snapshot.jobs,
                                  &snapshot.job_list_error)) {
    snapshot.jobs.clear();
  }

  snapshot.ok = true;
  return snapshot;
}

inline bool runtime_apply_refresh_snapshot(CmdState &st,
                                           RuntimeRefreshSnapshot snapshot) {
  if (!snapshot.ok) {
    set_runtime_status(st,
                       snapshot.error.empty()
                           ? "failed to load runtime inventory"
                           : snapshot.error,
                       true);
    return false;
  }

  const std::string previous_session_id = st.runtime.selected_session_id;
  const std::string previous_campaign_cursor =
      st.runtime.selected_campaign_cursor;
  const std::string previous_job_cursor = st.runtime.selected_job_cursor;
  const RuntimeLane previous_family_anchor_lane = st.runtime.family_anchor_lane;

  st.runtime.sessions = std::move(snapshot.sessions);
  st.runtime.campaigns = std::move(snapshot.campaigns);
  st.runtime.jobs = std::move(snapshot.jobs);
  rebuild_runtime_lineage_cache(st);
  clear_runtime_inventory_selection(st);
  update_runtime_selection_records(st);
  clear_runtime_job_excerpts(st);

  bool restored = false;
  const auto restore_previous_for_lane = [&](RuntimeLane lane) -> bool {
    if (runtime_is_session_lane(lane)) {
      if (runtime_is_none_session_selection(previous_session_id) &&
          runtime_has_none_session_row(st)) {
        select_runtime_session_and_related(st, kRuntimeNoneSessionSelection);
        return true;
      }
      if (!previous_session_id.empty() &&
          runtime_find_session_index(st.runtime.sessions, previous_session_id)
              .has_value()) {
        select_runtime_session_and_related(st, previous_session_id);
        return true;
      }
      return false;
    }
    if (runtime_is_campaign_lane(lane)) {
      if (runtime_is_none_campaign_selection(previous_campaign_cursor)) {
        if (runtime_is_none_session_selection(previous_session_id) &&
            runtime_has_none_session_row(st)) {
          select_runtime_session_and_related(st, kRuntimeNoneSessionSelection);
          if (runtime_child_has_none_campaign_row(st)) {
            select_runtime_none_campaign_and_related(
                st, std::string(kRuntimeNoneSessionSelection));
            return true;
          }
        }
        if (runtime_has_none_campaign_row(st)) {
          select_runtime_none_campaign_and_related(st);
          return true;
        }
      }
      if (!previous_campaign_cursor.empty() &&
          runtime_find_campaign_index(st.runtime.campaigns,
                                      previous_campaign_cursor)
              .has_value()) {
        if (runtime_is_none_session_selection(previous_session_id) &&
            runtime_has_none_session_row(st) &&
            runtime_session_id_for_campaign(st.runtime,
                                            previous_campaign_cursor)
                .empty()) {
          select_runtime_session_and_related(st, kRuntimeNoneSessionSelection);
        }
        select_runtime_campaign_and_related(st, previous_campaign_cursor);
        return true;
      }
      return false;
    }
    if (!previous_job_cursor.empty() &&
        runtime_find_job_index(st.runtime.jobs, previous_job_cursor)
            .has_value()) {
      const std::string parent_campaign_cursor =
          runtime_campaign_cursor_for_job(st.runtime, previous_job_cursor);
      const std::string parent_session_id =
          runtime_session_id_for_job(st.runtime, previous_job_cursor);
      if (runtime_is_none_session_selection(previous_session_id) &&
          runtime_has_none_session_row(st) && parent_session_id.empty()) {
        select_runtime_session_and_related(st, kRuntimeNoneSessionSelection);
      }
      if (runtime_is_none_campaign_selection(previous_campaign_cursor) &&
          runtime_has_none_campaign_row(st) && parent_campaign_cursor.empty()) {
        select_runtime_none_campaign_and_related(
            st, runtime_is_none_session_selection(previous_session_id)
                    ? std::string(kRuntimeNoneSessionSelection)
                    : std::string{});
      }
      select_runtime_job_and_related(st, previous_job_cursor);
      return true;
    }
    return false;
  };

  restored = restore_previous_for_lane(previous_family_anchor_lane);
  if (!restored)
    restored = restore_previous_for_lane(kRuntimeJobLane);
  if (!restored)
    restored = restore_previous_for_lane(kRuntimeCampaignLane);
  if (!restored)
    restored = restore_previous_for_lane(kRuntimeSessionLane);

  if (!restored) {
    const auto anchor_session = selected_operator_session_record(st);
    if (anchor_session.has_value() &&
        runtime_find_session_index(st.runtime.sessions,
                                   anchor_session->marshal_session_id)
            .has_value()) {
      select_runtime_session_and_related(st,
                                         anchor_session->marshal_session_id);
      restored = true;
    }
  }
  if (!restored && !st.runtime.sessions.empty()) {
    select_runtime_session_and_related(
        st, st.runtime.sessions.front().marshal_session_id);
    restored = true;
  }
  if (!restored && runtime_has_none_session_row(st)) {
    select_runtime_session_and_related(st, kRuntimeNoneSessionSelection);
    restored = true;
  }
  if (!restored && !st.runtime.campaigns.empty()) {
    select_runtime_campaign_and_related(
        st, st.runtime.campaigns.front().campaign_cursor);
    restored = true;
  }
  if (!restored && runtime_has_none_campaign_row(st)) {
    select_runtime_none_campaign_and_related(st);
    restored = true;
  }
  if (!restored && !st.runtime.jobs.empty()) {
    select_runtime_job_and_related(st, st.runtime.jobs.front().job_cursor);
    restored = true;
  }
  if (!restored) {
    clear_runtime_inventory_selection(st);
    update_runtime_selection_records(st);
  }

  st.runtime.ok = true;
  st.runtime.error.clear();
  if (!snapshot.session_list_error.empty() ||
      !snapshot.job_list_error.empty()) {
    std::string warning = "Runtime inventory partially loaded.";
    if (!snapshot.session_list_error.empty()) {
      warning += " sessions: " + snapshot.session_list_error + ".";
    }
    if (!snapshot.job_list_error.empty()) {
      warning += " jobs: " + snapshot.job_list_error + ".";
    }
    set_runtime_status(st, warning, false);
  } else {
    set_runtime_status(st, {}, false);
  }
  return true;
}

inline bool queue_runtime_refresh(CmdState &st) {
  if (st.runtime.refresh_pending) {
    st.runtime.refresh_requeue = true;
    return false;
  }
  st.runtime.refresh_pending = true;
  st.runtime.refresh_requeue = false;
  auto runtime_app = st.runtime.app;
  auto marshal_app = st.runtime.marshal_app;
  const bool marshal_ok = st.runtime.marshal_ok;
  const std::string marshal_error = st.runtime.marshal_error;
  st.runtime.refresh_future =
      std::async(std::launch::async,
                 [runtime_app = std::move(runtime_app), marshal_ok,
                  marshal_app = std::move(marshal_app),
                  marshal_error]() mutable -> RuntimeRefreshSnapshot {
                   return runtime_collect_refresh_snapshot(
                       std::move(runtime_app), marshal_ok,
                       std::move(marshal_app), marshal_error);
                 });
  return true;
}

inline bool queue_runtime_excerpt_refresh(CmdState &st) {
  clear_runtime_job_excerpts(st);
  if (!st.runtime.job.has_value() ||
      st.runtime.detail_mode == RuntimeDetailMode::Manifest) {
    st.runtime.excerpt_requeue = false;
    return false;
  }
  if (st.runtime.excerpt_pending) {
    st.runtime.excerpt_requeue = true;
    return false;
  }
  st.runtime.excerpt_pending = true;
  st.runtime.excerpt_requeue = false;
  auto runtime_app = st.runtime.app;
  const auto job = *st.runtime.job;
  const RuntimeDetailMode detail_mode = st.runtime.detail_mode;
  st.runtime.excerpt_future =
      std::async(std::launch::async,
                 [runtime_app = std::move(runtime_app), job,
                  detail_mode]() mutable -> RuntimeExcerptSnapshot {
                   return runtime_collect_excerpt_snapshot(
                       std::move(runtime_app), job, detail_mode);
                 });
  return true;
}

inline bool poll_runtime_async_updates(CmdState &st) {
  bool dirty = false;
  if (st.runtime.refresh_pending && st.runtime.refresh_future.valid() &&
      st.runtime.refresh_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    RuntimeRefreshSnapshot snapshot = st.runtime.refresh_future.get();
    st.runtime.refresh_pending = false;
    dirty = runtime_apply_refresh_snapshot(st, std::move(snapshot)) || dirty;
    if (st.runtime.refresh_requeue) {
      st.runtime.refresh_requeue = false;
      (void)queue_runtime_refresh(st);
    }
  }
  if (st.runtime.excerpt_pending && st.runtime.excerpt_future.valid() &&
      st.runtime.excerpt_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    RuntimeExcerptSnapshot snapshot = st.runtime.excerpt_future.get();
    st.runtime.excerpt_pending = false;
    bool applied = false;
    if (st.runtime.job.has_value() &&
        st.runtime.job->job_cursor == snapshot.job_cursor &&
        st.runtime.detail_mode == snapshot.detail_mode) {
      st.runtime.job_stdout_excerpt = std::move(snapshot.stdout_excerpt);
      st.runtime.job_stderr_excerpt = std::move(snapshot.stderr_excerpt);
      st.runtime.job_trace_excerpt = std::move(snapshot.trace_excerpt);
      applied = true;
    }
    if (st.runtime.excerpt_requeue || !applied) {
      st.runtime.excerpt_requeue = false;
      (void)queue_runtime_excerpt_refresh(st);
    }
    dirty = applied || dirty;
  }
  if (st.runtime.device_pending && st.runtime.device_future.valid() &&
      st.runtime.device_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    RuntimeDeviceSnapshot snapshot = st.runtime.device_future.get();
    st.runtime.device_pending = false;
    const bool applied = runtime_apply_device_snapshot(st, std::move(snapshot));
    if (st.runtime.device_requeue) {
      st.runtime.device_requeue = false;
      (void)queue_runtime_device_refresh(st);
    }
    dirty = applied || dirty;
  }
  return dirty;
}

inline bool refresh_runtime_state(CmdState &st) {
  const bool applied = runtime_apply_refresh_snapshot(
      st, runtime_collect_refresh_snapshot(
              st.runtime.app, st.runtime.marshal_ok, st.runtime.marshal_app,
              st.runtime.marshal_error));
  st.runtime.device_last_refresh_ms = runtime_monotonic_now_ms();
  (void)runtime_apply_device_snapshot(st, runtime_collect_device_snapshot());
  return applied;
}

template <class AppendLog>
inline void append_runtime_show_lines(const CmdState &st,
                                      AppendLog &&append_log) {
  append_log("screen=runtime", "show", "#d8d8ff");
  append_log("detail_mode=" + runtime_detail_mode_label(st.runtime.detail_mode),
             "show", "#d8d8ff");
  append_log("lane=" + runtime_lane_label(st.runtime.lane) +
                 " focus=" + runtime_focus_label(st.runtime.focus),
             "show", "#d8d8ff");
  if (st.runtime.refresh_pending) {
    append_log("runtime_refresh=background", "show", "#d8d8ff");
  }
  if (st.runtime.excerpt_pending) {
    append_log("runtime_excerpt=background", "show", "#d8d8ff");
  }
  if (!st.runtime.anchor_session_id.empty()) {
    append_log("anchor_session_id=" + st.runtime.anchor_session_id, "show",
               "#d8d8ff");
  }
  append_log("sessions=" + std::to_string(st.runtime.sessions.size()) +
                 " campaigns=" + std::to_string(st.runtime.campaigns.size()) +
                 " jobs=" + std::to_string(st.runtime.jobs.size()),
             "show", "#d8d8ff");
  std::size_t detached_campaigns = 0;
  for (const auto &campaign : st.runtime.campaigns) {
    if (runtime_session_id_for_campaign(st.runtime, campaign.campaign_cursor)
            .empty()) {
      ++detached_campaigns;
    }
  }
  std::size_t orphan_jobs = 0;
  std::size_t sessionless_jobs = 0;
  for (const auto &job : st.runtime.jobs) {
    const std::string campaign_cursor =
        runtime_campaign_cursor_for_job(st.runtime, job.job_cursor);
    const std::string marshal_session_id =
        runtime_session_id_for_job(st.runtime, job.job_cursor);
    if (campaign_cursor.empty()) {
      ++orphan_jobs;
    } else if (marshal_session_id.empty()) {
      ++sessionless_jobs;
    }
  }
  append_log("detached_campaigns=" + std::to_string(detached_campaigns) +
                 " orphan_jobs=" + std::to_string(orphan_jobs) +
                 " sessionless_jobs=" + std::to_string(sessionless_jobs),
             "show", "#d8d8ff");
  if (!st.runtime.selected_session_id.empty()) {
    append_log("selected_session_id=" + st.runtime.selected_session_id, "show",
               "#d8d8ff");
  }
  if (!st.runtime.selected_campaign_cursor.empty()) {
    append_log("selected_campaign=" + st.runtime.selected_campaign_cursor,
               "show", "#d8d8ff");
  }
  if (!st.runtime.selected_job_cursor.empty()) {
    append_log("selected_job=" + st.runtime.selected_job_cursor, "show",
               "#d8d8ff");
  }
  if (const auto idx = runtime_current_lane_selected_index(st);
      idx.has_value()) {
    append_log("lane_selection=" + std::to_string(*idx + 1) + "/" +
                   std::to_string(current_runtime_lane_count(st)),
               "show", "#d8d8ff");
  }
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
