#pragma once

#include <cctype>
#include <cstdint>
#include <iomanip>
#include <map>

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {
inline std::string to_upper_copy(std::string s) {
  for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return s;
}

inline int logs_level_rank(const std::string& level) {
  const std::string u = to_upper_copy(level);
  if (u.find("FATAL") != std::string::npos || u.find("TERMINATION") != std::string::npos) return 50;
  if (u.find("ERROR") != std::string::npos || u.find("ERRNO") != std::string::npos) return 40;
  if (u.find("WARN") != std::string::npos) return 30;
  if (u.find("INFO") != std::string::npos) return 20;
  if (u.find("DEBUG") != std::string::npos) return 10;
  return 20;
}

inline int logs_filter_min_rank(LogsLevelFilter f) {
  switch (f) {
    case LogsLevelFilter::DebugOrHigher:
      return 10;
    case LogsLevelFilter::InfoOrHigher:
      return 20;
    case LogsLevelFilter::WarningOrHigher:
      return 30;
    case LogsLevelFilter::ErrorOrHigher:
      return 40;
    case LogsLevelFilter::FatalOnly:
      return 50;
  }
  return 10;
}

inline std::string logs_filter_label(LogsLevelFilter f) {
  switch (f) {
    case LogsLevelFilter::DebugOrHigher:
      return "DEBUG+";
    case LogsLevelFilter::InfoOrHigher:
      return "INFO+";
    case LogsLevelFilter::WarningOrHigher:
      return "WARNING+";
    case LogsLevelFilter::ErrorOrHigher:
      return "ERROR+";
    case LogsLevelFilter::FatalOnly:
      return "FATAL";
  }
  return "DEBUG+";
}

inline std::string logs_metadata_filter_label(LogsMetadataFilter f) {
  switch (f) {
    case LogsMetadataFilter::Any:
      return "ANY";
    case LogsMetadataFilter::WithAnyMetadata:
      return "META+";
    case LogsMetadataFilter::WithFunction:
      return "FN+";
    case LogsMetadataFilter::WithPath:
      return "PATH+";
    case LogsMetadataFilter::WithCallsite:
      return "CALLSITE+";
  }
  return "ANY";
}

inline bool logs_entry_has_function(const cuwacunu::piaabo::dlog_entry_t& e) {
#if DLOGS_ENABLE_METADATA
  return !e.source_function.empty();
#else
  (void)e;
  return false;
#endif
}

inline bool logs_entry_has_path(const cuwacunu::piaabo::dlog_entry_t& e) {
#if DLOGS_ENABLE_METADATA
  return !e.source_path.empty();
#else
  (void)e;
  return false;
#endif
}

inline bool logs_entry_has_callsite(const cuwacunu::piaabo::dlog_entry_t& e) {
#if DLOGS_ENABLE_METADATA
  return e.callsite_id != 0;
#else
  (void)e;
  return false;
#endif
}

inline bool logs_entry_has_any_metadata(const cuwacunu::piaabo::dlog_entry_t& e) {
#if DLOGS_ENABLE_METADATA
  return logs_entry_has_function(e) ||
         !e.source_file.empty() ||
         logs_entry_has_path(e) ||
         logs_entry_has_callsite(e) ||
         e.pid != 0 ||
         e.monotonic_ns != 0 ||
         e.line != 0;
#else
  (void)e;
  return false;
#endif
}

inline bool logs_accept_metadata_filter(
    const LogsState& settings,
    const cuwacunu::piaabo::dlog_entry_t& e) {
  switch (settings.metadata_filter) {
    case LogsMetadataFilter::Any:
      return true;
    case LogsMetadataFilter::WithAnyMetadata:
      return logs_entry_has_any_metadata(e);
    case LogsMetadataFilter::WithFunction:
      return logs_entry_has_function(e);
    case LogsMetadataFilter::WithPath:
      return logs_entry_has_path(e);
    case LogsMetadataFilter::WithCallsite:
      return logs_entry_has_callsite(e);
  }
  return true;
}

inline bool logs_accept_entry(const LogsState& settings,
                              const cuwacunu::piaabo::dlog_entry_t& e) {
  return logs_level_rank(e.level) >= logs_filter_min_rank(settings.level_filter) &&
         logs_accept_metadata_filter(settings, e);
}

inline cuwacunu::iinuji::text_line_emphasis_t logs_line_emphasis(
    const LogsState& settings,
    const cuwacunu::piaabo::dlog_entry_t& e) {
  if (!settings.show_color) return cuwacunu::iinuji::text_line_emphasis_t::None;
  const int rank = logs_level_rank(e.level);
  if (rank >= 50) return cuwacunu::iinuji::text_line_emphasis_t::Fatal;
  if (rank >= 40) return cuwacunu::iinuji::text_line_emphasis_t::Error;
  if (rank >= 30) return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  if (rank >= 20) return cuwacunu::iinuji::text_line_emphasis_t::Info;
  return cuwacunu::iinuji::text_line_emphasis_t::Debug;
}

inline std::string format_logs_entry(const LogsState& settings,
                                     const cuwacunu::piaabo::dlog_entry_t& e) {
  std::ostringstream oss;
  if (settings.show_date) {
    oss << "[" << e.timestamp << "]";
    oss << " ";
  }
  oss << "[" << e.level << "] ";
  if (settings.show_thread) {
    oss << "[0x" << e.thread << "]";
    oss << " ";
  }
  oss << e.message;
#if DLOGS_ENABLE_METADATA
  if (settings.show_metadata && logs_entry_has_any_metadata(e)) {
    bool first = true;
    oss << " {";
    auto append_field = [&](const std::string& token) {
      if (!first) oss << " ";
      oss << token;
      first = false;
    };
    if (!e.source_function.empty()) append_field("fn=" + e.source_function);
    if (!e.source_file.empty()) {
      std::string file = e.source_file;
      if (e.line > 0) file += ":" + std::to_string(e.line);
      append_field("file=" + file);
    }
    if (!e.source_path.empty()) append_field("path=" + e.source_path);
    if (e.callsite_id != 0) {
      std::ostringstream cs;
      cs << "cs=0x" << std::hex << e.callsite_id << std::dec;
      append_field(cs.str());
    }
    if (e.pid != 0) append_field("pid=" + std::to_string(e.pid));
    if (e.monotonic_ns != 0) {
      append_field("mono_ns=" + std::to_string(e.monotonic_ns));
    }
    oss << "}";
  }
#endif
  return oss.str();
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t> make_logs_left_styled_lines(
    const LogsState& settings,
    const std::vector<cuwacunu::piaabo::dlog_entry_t>& snap) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out;
  std::size_t shown = 0;
  for (const auto& e : snap) {
    if (logs_accept_entry(settings, e)) ++shown;
  }

  auto push = [&](std::string text, cuwacunu::iinuji::text_line_emphasis_t emphasis = cuwacunu::iinuji::text_line_emphasis_t::None) {
    out.push_back(cuwacunu::iinuji::styled_text_line_t{
        .text = std::move(text),
        .emphasis = emphasis,
    });
  };

  push("# dlogs buffer");
  push(
      "# entries=" + std::to_string(snap.size()) +
      " shown=" + std::to_string(shown) +
      " capacity=" + std::to_string(cuwacunu::piaabo::dlog_buffer_capacity()));
  push(
      "# level=" + logs_filter_label(settings.level_filter) +
      " meta_filter=" + logs_metadata_filter_label(settings.metadata_filter) +
      " date=" + (settings.show_date ? std::string("on") : std::string("off")) +
      " thread=" + (settings.show_thread ? std::string("on") : std::string("off")) +
      " metadata=" + (settings.show_metadata ? std::string("on") : std::string("off")) +
      " color=" + (settings.show_color ? std::string("on") : std::string("off")) +
      " follow=" + (settings.auto_follow ? std::string("on") : std::string("off")));
  push("# newest entries at bottom");
  push("");
  if (shown == 0) {
    push("(no logs)");
    return out;
  }
  for (const auto& e : snap) {
    if (!logs_accept_entry(settings, e)) continue;
    push(format_logs_entry(settings, e), logs_line_emphasis(settings, e));
  }
  return out;
}

inline std::string styled_lines_to_text(
    const std::vector<cuwacunu::iinuji::styled_text_line_t>& lines) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i > 0) oss << '\n';
    oss << lines[i].text;
  }
  return oss.str();
}

inline std::string make_logs_left(const LogsState& settings,
                                  const std::vector<cuwacunu::piaabo::dlog_entry_t>& snap) {
  return styled_lines_to_text(make_logs_left_styled_lines(settings, snap));
}

inline std::string make_logs_right(const LogsState& settings,
                                   const std::vector<cuwacunu::piaabo::dlog_entry_t>& snap) {
  std::map<std::string, std::size_t> level_counts;
  std::size_t shown = 0;
  std::size_t metadata_any = 0;
  std::size_t metadata_fn = 0;
  std::size_t metadata_path = 0;
  std::size_t metadata_callsite = 0;
  for (const auto& e : snap) {
    ++level_counts[e.level];
    if (logs_entry_has_any_metadata(e)) ++metadata_any;
    if (logs_entry_has_function(e)) ++metadata_fn;
    if (logs_entry_has_path(e)) ++metadata_path;
    if (logs_entry_has_callsite(e)) ++metadata_callsite;
    if (logs_accept_entry(settings, e)) ++shown;
  }

  auto setting_line = [&](std::size_t idx, const std::string& text) {
    const std::string line = "  " + text;
    if (idx == settings.selected_setting) return mark_selected_line(line);
    return line;
  };

  std::ostringstream oss;
  oss << "Logs view\n";
  oss << "  source: piaabo/dlogs.h buffer\n";
  oss << "  entries: " << shown << " shown / " << snap.size() << " total"
      << " / " << cuwacunu::piaabo::dlog_buffer_capacity() << "\n";
#if DLOGS_ENABLE_METADATA
  oss << "  metadata: on\n";
#else
  oss << "  metadata: off\n";
  if (settings.metadata_filter != LogsMetadataFilter::Any ||
      settings.show_metadata) {
    oss << "  metadata capture is disabled at build-time (DLOGS_ENABLE_METADATA=0)\n";
  }
#endif
  oss << "  metadata coverage: any=" << metadata_any
      << " fn=" << metadata_fn
      << " path=" << metadata_path
      << " callsite=" << metadata_callsite
      << "\n";
  if (shown > 0) {
    std::uint64_t first_seq = 0;
    std::uint64_t last_seq = 0;
    std::string first_ts;
    std::string last_ts;
    bool have = false;
    for (const auto& e : snap) {
      if (!logs_accept_entry(settings, e)) continue;
      if (!have) {
        have = true;
        first_seq = e.seq;
        first_ts = e.timestamp;
      }
      last_seq = e.seq;
      last_ts = e.timestamp;
    }
    if (have) {
      oss << "  seq: " << first_seq << " .. " << last_seq << "\n";
      oss << "  first: " << first_ts << "\n";
      oss << "  last : " << last_ts << "\n";
#if DLOGS_ENABLE_METADATA
      for (auto it = snap.rbegin(); it != snap.rend(); ++it) {
        if (!logs_accept_entry(settings, *it)) continue;
        if (!logs_entry_has_any_metadata(*it)) continue;
        if (!it->source_function.empty()) {
          oss << "  last function: " << it->source_function << "\n";
        }
        if (!it->source_file.empty()) {
          oss << "  last file: " << it->source_file;
          if (it->line > 0) oss << ":" << it->line;
          oss << "\n";
        }
        if (!it->source_path.empty()) {
          oss << "  last path: " << it->source_path << "\n";
        }
        if (it->callsite_id != 0) {
          oss << "  last callsite_id: 0x"
              << std::hex << it->callsite_id << std::dec << "\n";
        }
        if (it->pid != 0) {
          oss << "  last pid: " << it->pid << "\n";
        }
        if (it->monotonic_ns != 0) {
          oss << "  last mono_ns: " << it->monotonic_ns << "\n";
        }
        if (it->source_function.empty() &&
            it->source_file.empty() &&
            it->source_path.empty() &&
            it->callsite_id == 0 &&
            it->pid == 0 &&
            it->monotonic_ns == 0) {
          oss << "  last metadata: none\n";
        }
        break;
      }
#else
      if (settings.metadata_filter != LogsMetadataFilter::Any ||
          settings.show_metadata) {
        oss << "  metadata detail unavailable (build with DLOGS_ENABLE_METADATA=1)\n";
      }
#endif
    }
  }
  oss << "\nSettings (Up/Down select, Left/Right change)\n";
  oss << setting_line(0, "log level : " + logs_filter_label(settings.level_filter)) << "\n";
  oss << setting_line(1, "metadata filter : " + logs_metadata_filter_label(settings.metadata_filter))
      << "\n";
  oss << setting_line(2, std::string("show date : ") + (settings.show_date ? "on" : "off")) << "\n";
  oss << setting_line(3, std::string("show thread id : ") + (settings.show_thread ? "on" : "off"))
      << "\n";
  oss << setting_line(4, std::string("show metadata : ") + (settings.show_metadata ? "on" : "off"))
      << "\n";
  oss << setting_line(5, std::string("show color : ") + (settings.show_color ? "on" : "off")) << "\n";
  oss << setting_line(6, std::string("auto follow : ") + (settings.auto_follow ? "on" : "off"))
      << "\n";
  oss << setting_line(7, std::string("mouse capture : ") + (settings.mouse_capture ? "on" : "off (select/copy)"))
      << "\n";
  oss << "\nLevels\n";
  if (level_counts.empty()) {
    oss << "  (none)\n";
  } else {
    for (const auto& kv : level_counts) {
      oss << "  " << kv.first << " : " << kv.second << "\n";
    }
  }
  oss << "\nCommands\n";
  static const auto kLogsCallCommands = [] {
    auto out = canonical_paths::call_texts_by_prefix({"iinuji.logs."});
    const auto screen = canonical_paths::call_texts_by_prefix({"iinuji.screen.logs("});
    const auto show = canonical_paths::call_texts_by_prefix({"iinuji.show.logs("});
    out.insert(out.end(), screen.begin(), screen.end());
    out.insert(out.end(), show.begin(), show.end());
    return out;
  }();
  for (const auto cmd : kLogsCallCommands) oss << "  " << cmd << "\n";
  oss << "  aliases: logs, f8, logs clear\n";
  oss << "  primitive translation: disabled\n";
  oss << "\nKeys\n";
  oss << "  F8 : open logs screen\n";
  oss << "  [^] top-right click or Home : jump to oldest\n";
  oss << "  [v] bottom-right click or End : jump to newest\n";
  oss << "  wheel : vertical scroll both panels\n";
  oss << "  Shift/Ctrl/Alt+wheel : horizontal scroll both panels\n";
  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
