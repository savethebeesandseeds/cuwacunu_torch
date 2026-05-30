
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

inline std::vector<std::string>
runtime_split_jsonl_records(const std::string &text);

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
  const auto records = runtime_split_jsonl_records(raw);
  visible_lines.reserve(records.size());
  color_lines->reserve(records.size());
  for (const auto &raw_line : records) {
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
runtime_split_jsonl_records(const std::string &text) {
  std::vector<std::string> records{};
  if (text.empty())
    return records;

  std::size_t i = 0;
  while (i < text.size()) {
    while (i < text.size()) {
      const char c = text[i];
      if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' ||
          c == '\v') {
        ++i;
        continue;
      }
      break;
    }

    if (i >= text.size())
      break;

    if (text[i] != '{') {
      const std::size_t end = text.find('\n', i);
      const std::size_t stop = end == std::string::npos ? text.size() : end;
      std::string line = text.substr(i, stop - i);
      if (!line.empty() && line.back() == '\r')
        line.pop_back();
      records.push_back(std::move(line));
      if (end == std::string::npos)
        break;
      i = end + 1;
      continue;
    }

    const std::size_t start = i;
    bool in_string = false;
    bool escape = false;
    int depth = 0;
    for (; i < text.size(); ++i) {
      const char c = text[i];
      if (in_string) {
        if (escape) {
          escape = false;
          continue;
        }
        if (c == '\\') {
          escape = true;
        } else if (c == '"') {
          in_string = false;
        }
        continue;
      }

      if (c == '"') {
        in_string = true;
      } else if (c == '{') {
        ++depth;
      } else if (c == '}') {
        if (depth > 0) {
          --depth;
          if (depth == 0) {
            ++i;
            break;
          }
        }
      }
    }

    const std::size_t end = i;
    if (end > start) {
      records.push_back(text.substr(start, end - start));
    } else {
      break;
    }
  }
  return records;
}

inline std::vector<std::string>
runtime_tail_text_file_lines(const std::string &path, std::size_t max_lines) {
  std::vector<std::string> out{};
  if (path.empty() || max_lines == 0)
    return out;

  std::ifstream input(path);
  if (!input.is_open())
    return out;

  std::ostringstream raw_stream;
  raw_stream << input.rdbuf();
  const std::string raw = raw_stream.str();
  const auto records = runtime_split_jsonl_records(raw);
  if (records.empty())
    return out;

  const std::size_t start =
      records.size() > max_lines ? records.size() - max_lines : 0;
  out.reserve(records.size() - start);
  for (std::size_t i = start; i < records.size(); ++i) {
    out.push_back(records[i]);
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

inline bool runtime_marshaled_payload_skip_root_key(std::string_view key) {
  static const std::vector<std::string> skipped = {
      "event", "type",  "kind",          "timestamp_ms",  "timestamp",
      "ts",    "ts_ms", "updated_at_ms", "started_at_ms", "finished_at_ms"};
  return std::find(skipped.begin(), skipped.end(), key) != skipped.end();
}

inline void runtime_append_marshaled_payload_fields(
    const cmd_json_value_t *value, std::string prefix,
    std::vector<std::pair<std::string, std::string>> *out, bool root = false) {
  if (value == nullptr || out == nullptr)
    return;

  auto append_scalar = [&](std::string key, const cmd_json_value_t *scalar) {
    if (key.empty() || scalar == nullptr)
      return;
    const std::string text = runtime_event_json_value_text(scalar);
    if (text.empty())
      return;
    out->emplace_back(std::move(key), text);
  };

  switch (value->type) {
  case cmd_json_type_t::OBJECT: {
    if (value->objectValue == nullptr || value->objectValue->empty()) {
      if (!prefix.empty())
        out->emplace_back(std::move(prefix), "{}");
      return;
    }

    auto append_object_field = [&](std::string_view key) {
      if (runtime_marshaled_payload_skip_root_key(key))
        return;
      const auto *child = cmd_json_field(value, key);
      if (child == nullptr)
        return;
      const std::string child_prefix =
          prefix.empty() ? std::string(key) : prefix + "." + std::string(key);
      runtime_append_marshaled_payload_fields(child, child_prefix, out, false);
    };

    if (root) {
      const std::string_view preferred_keys[] = {"detail",
                                                 "summary",
                                                 "text",
                                                 "message",
                                                 "status",
                                                 "checkpoint_index",
                                                 "checkpoint_count",
                                                 "launch_count",
                                                 "lifecycle",
                                                 "intent",
                                                 "intent_kind",
                                                 "work_gate",
                                                 "finish_reason",
                                                 "campaign_status",
                                                 "campaign_cursor",
                                                 "status_detail",
                                                 "activity",
                                                 "tool_name",
                                                 "role",
                                                 "schema",
                                                 "marshal_session_id"};
      std::vector<std::string> seen{};
      seen.reserve(value->objectValue->size());
      for (const auto key : preferred_keys) {
        const auto *child = cmd_json_field(value, key);
        if (child == nullptr || runtime_marshaled_payload_skip_root_key(key))
          continue;
        seen.push_back(std::string(key));
        append_object_field(key);
      }
      for (const auto &kv : *value->objectValue) {
        if (runtime_marshaled_payload_skip_root_key(kv.first))
          continue;
        if (std::find(seen.begin(), seen.end(), kv.first) != seen.end())
          continue;
        const std::string child_prefix =
            prefix.empty() ? kv.first : prefix + "." + kv.first;
        runtime_append_marshaled_payload_fields(&kv.second, child_prefix, out,
                                                false);
      }
      return;
    }

    for (const auto &kv : *value->objectValue) {
      const std::string child_prefix =
          prefix.empty() ? kv.first : prefix + "." + kv.first;
      runtime_append_marshaled_payload_fields(&kv.second, child_prefix, out,
                                              false);
    }
    return;
  }
  case cmd_json_type_t::ARRAY:
    if (value->arrayValue == nullptr || value->arrayValue->empty()) {
      if (!prefix.empty())
        out->emplace_back(std::move(prefix), "[]");
      return;
    }
    for (std::size_t i = 0; i < value->arrayValue->size(); ++i) {
      const std::string child_prefix = prefix + "[" + std::to_string(i) + "]";
      runtime_append_marshaled_payload_fields(&(*value->arrayValue)[i],
                                              child_prefix, out, false);
    }
    return;
  case cmd_json_type_t::STRING:
  case cmd_json_type_t::NUMBER:
  case cmd_json_type_t::BOOLEAN:
  case cmd_json_type_t::NULL_TYPE:
    append_scalar(std::move(prefix), value);
    return;
  }
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

  runtime_append_marshaled_payload_fields(&parsed, {}, &entry.payload, true);
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

