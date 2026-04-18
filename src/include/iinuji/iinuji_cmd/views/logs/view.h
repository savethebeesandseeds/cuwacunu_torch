#pragma once

#include <array>
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

inline bool logs_message_has_nonempty_warning_payload(
    const std::string& message) {
  if (message.empty()) return false;
  const std::string lower = to_lower_copy(message);

  const auto is_token_char = [](char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
  };

  const auto bare_match_has_boundaries = [&](std::size_t pos,
                                             std::size_t len) {
    const bool left_ok = pos == 0 || !is_token_char(lower[pos - 1]);
    const std::size_t after = pos + len;
    const bool right_ok = after >= lower.size() || !is_token_char(lower[after]);
    return left_ok && right_ok;
  };

  const auto key_has_warning_payload = [&](const std::string& key) {
    const std::string quoted = "\"" + key + "\"";
    std::size_t pos = 0;
    while (pos < lower.size()) {
      const std::size_t quoted_pos = lower.find(quoted, pos);
      const std::size_t bare_pos = lower.find(key, pos);
      std::size_t match_pos = std::string::npos;
      std::size_t key_end = std::string::npos;

      if (quoted_pos != std::string::npos &&
          (bare_pos == std::string::npos || quoted_pos <= bare_pos)) {
        match_pos = quoted_pos;
        key_end = quoted_pos + quoted.size();
      } else if (bare_pos != std::string::npos &&
                 bare_match_has_boundaries(bare_pos, key.size())) {
        match_pos = bare_pos;
        key_end = bare_pos + key.size();
      } else if (bare_pos != std::string::npos) {
        pos = bare_pos + key.size();
        continue;
      } else {
        break;
      }

      std::size_t value_pos = key_end;
      while (value_pos < lower.size() &&
             std::isspace(static_cast<unsigned char>(lower[value_pos]))) {
        ++value_pos;
      }
      if (value_pos >= lower.size() || lower[value_pos] != ':') {
        pos = match_pos + 1;
        continue;
      }

      ++value_pos;
      while (value_pos < lower.size() &&
             std::isspace(static_cast<unsigned char>(lower[value_pos]))) {
        ++value_pos;
      }
      if (value_pos >= lower.size()) return false;

      if (lower.compare(value_pos, 2, "[]") == 0) {
        pos = value_pos + 2;
        continue;
      }
      if (lower.compare(value_pos, 4, "null") == 0 ||
          lower.compare(value_pos, 5, "false") == 0 ||
          lower.compare(value_pos, 2, "\"\"") == 0) {
        pos = value_pos + 1;
        continue;
      }
      if (lower[value_pos] == '[') {
        ++value_pos;
        while (value_pos < lower.size() &&
               std::isspace(static_cast<unsigned char>(lower[value_pos]))) {
          ++value_pos;
        }
        if (value_pos < lower.size() && lower[value_pos] == ']') {
          pos = value_pos + 1;
          continue;
        }
      }
      return true;
    }
    return false;
  };

  return key_has_warning_payload("warning") ||
         key_has_warning_payload("warnings") ||
         key_has_warning_payload("dev_warning") ||
         key_has_warning_payload("dev_warnings");
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

inline std::string logs_pad_right(std::string value, std::size_t width) {
  if (value.size() >= width) {
    return value;
  }
  value.append(width - value.size(), ' ');
  return value;
}

inline std::string logs_bool_badge(bool enabled) {
  return enabled ? "[on]" : "[off]";
}

inline std::string logs_setting_value_badge(const std::string& value) {
  return "[" + value + "]";
}

inline std::string logs_selected_setting_hint(std::size_t idx) {
  switch (idx) {
    case 0:
      return "Level sets the minimum severity that remains visible.";
    case 1:
      return "Meta filter narrows the stream to entries with matching metadata.";
    case 2:
      return "Date toggles the wall-clock prefix for each visible log line.";
    case 3:
      return "Thread toggles the worker id column.";
    case 4:
      return "Metadata appends fn/file/path/callsite fields when available.";
    case 5:
      return "Color keeps severity emphasis in the left log stream.";
    case 6:
      return "Follow pins the stream to the newest entry as new logs arrive.";
    case 7:
      return "Mouse capture keeps ncurses interaction instead of terminal select/copy.";
    default:
      return "Use Up/Down to select a setting and Left/Right to change it.";
  }
}

inline std::string logs_setting_line(std::size_t selected_setting,
                                     std::size_t idx,
                                     std::string label,
                                     std::string value) {
  std::string line = "  " + logs_pad_right(std::move(label), 12) + " " +
                     std::move(value);
  if (idx == selected_setting) {
    return mark_selected_line(std::move(line));
  }
  return line;
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
    const ShellLogsState& settings,
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

inline bool logs_accept_entry(const ShellLogsState& settings,
                              const cuwacunu::piaabo::dlog_entry_t& e) {
  return logs_level_rank(e.level) >= logs_filter_min_rank(settings.level_filter) &&
         logs_accept_metadata_filter(settings, e);
}

inline cuwacunu::iinuji::text_line_emphasis_t logs_line_emphasis(
    const ShellLogsState& settings,
    const cuwacunu::piaabo::dlog_entry_t& e) {
  if (!settings.show_color) return cuwacunu::iinuji::text_line_emphasis_t::None;
  const int rank = logs_level_rank(e.level);
  if (rank >= 50) return cuwacunu::iinuji::text_line_emphasis_t::Fatal;
  if (rank >= 40) return cuwacunu::iinuji::text_line_emphasis_t::Error;
  if (rank >= 30 || logs_message_has_nonempty_warning_payload(e.message)) {
    return cuwacunu::iinuji::text_line_emphasis_t::MutedWarning;
  }
  if (rank >= 20) return cuwacunu::iinuji::text_line_emphasis_t::Info;
  return cuwacunu::iinuji::text_line_emphasis_t::Debug;
}

inline std::string format_logs_entry(const ShellLogsState& settings,
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
    const ShellLogsState& settings,
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

  push("# shell dlogs buffer");
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
    if (snap.empty()) {
      push("(no logs captured yet)");
    } else {
      push("(no logs match the current filters)");
      push("# tip: set level=DEBUG+ and meta_filter=ANY to see everything");
    }
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
    oss << cuwacunu::iinuji::styled_text_line_text(lines[i]);
  }
  return oss.str();
}

inline std::string make_logs_left(const ShellLogsState& settings,
                                  const std::vector<cuwacunu::piaabo::dlog_entry_t>& snap) {
  return styled_lines_to_text(make_logs_left_styled_lines(settings, snap));
}

inline std::string make_logs_right(const ShellLogsState& settings,
                                   const std::vector<cuwacunu::piaabo::dlog_entry_t>& snap) {
  std::size_t shown = 0;
  std::size_t fatal_count = 0;
  std::size_t error_count = 0;
  std::size_t warning_count = 0;
  std::size_t info_count = 0;
  std::size_t debug_count = 0;
  for (const auto& e : snap) {
    const int rank = logs_level_rank(e.level);
    if (rank >= 50) {
      ++fatal_count;
    } else if (rank >= 40) {
      ++error_count;
    } else if (rank >= 30) {
      ++warning_count;
    } else if (rank >= 20) {
      ++info_count;
    } else {
      ++debug_count;
    }
    if (logs_accept_entry(settings, e)) ++shown;
  }
  const std::size_t hidden = snap.size() >= shown ? (snap.size() - shown) : 0;

  std::ostringstream oss;
  oss << "Shell Logs Control Deck\n";
  oss << "  tuned for live operator triage\n";
  oss << "\nCapture\n";
  oss << "  visible     " << shown << " / " << snap.size() << "\n";
  oss << "  hidden      " << hidden << "\n";
  oss << "  capacity    " << cuwacunu::piaabo::dlog_buffer_capacity() << "\n";
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
      oss << "  seq         " << first_seq << " .. " << last_seq << "\n";
      oss << "  first seen  " << first_ts << "\n";
      oss << "  latest      " << last_ts << "\n";
    }
  }
  oss << "\nLens\n";
  oss << "  level       " << logs_filter_label(settings.level_filter) << "\n";
  oss << "  meta        " << logs_metadata_filter_label(settings.metadata_filter)
      << "\n";
  oss << "  fields      date " << (settings.show_date ? "on" : "off")
      << " | thread " << (settings.show_thread ? "on" : "off") << "\n";
  oss << "              meta " << (settings.show_metadata ? "on" : "off")
      << " | color " << (settings.show_color ? "on" : "off") << "\n";
  oss << "  behavior    follow " << (settings.auto_follow ? "on" : "off")
      << " | mouse "
      << (settings.mouse_capture ? "capture" : "select/copy") << "\n";

  oss << "\nSettings\n";
  oss << "  Up/Down select | Left/Right change\n";
  oss << logs_setting_line(settings.selected_setting, 0, "level",
                           logs_setting_value_badge(
                               logs_filter_label(settings.level_filter)))
      << "\n";
  oss << logs_setting_line(
             settings.selected_setting, 1, "meta filter",
             logs_setting_value_badge(
                 logs_metadata_filter_label(settings.metadata_filter)))
      << "\n";
  oss << logs_setting_line(settings.selected_setting, 2, "date",
                           logs_bool_badge(settings.show_date))
      << "\n";
  oss << logs_setting_line(settings.selected_setting, 3, "thread",
                           logs_bool_badge(settings.show_thread))
      << "\n";
  oss << logs_setting_line(settings.selected_setting, 4, "metadata",
                           logs_bool_badge(settings.show_metadata))
      << "\n";
  oss << logs_setting_line(settings.selected_setting, 5, "color",
                           logs_bool_badge(settings.show_color))
      << "\n";
  oss << logs_setting_line(settings.selected_setting, 6, "follow",
                           logs_bool_badge(settings.auto_follow))
      << "\n";
  oss << logs_setting_line(
             settings.selected_setting, 7, "mouse",
             logs_setting_value_badge(settings.mouse_capture ? "capture"
                                                             : "select/copy"))
      << "\n";

  oss << "\nSelected\n";
  oss << "  " << logs_selected_setting_hint(settings.selected_setting) << "\n";

  oss << "\nSignal Mix\n";
  oss << "  fatal       " << fatal_count << "\n";
  oss << "  error       " << error_count << "\n";
  oss << "  warning     " << warning_count << "\n";
  oss << "  info        " << info_count << "\n";
  oss << "  debug       " << debug_count << "\n";

  oss << "\nQuick Keys\n";
  oss << "  Home/End    jump oldest/newest\n";
  oss << "  f / Esc     full screen / split\n";
  oss << "  wheel       scroll panels\n";
  oss << "  help        command guide\n";
  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
