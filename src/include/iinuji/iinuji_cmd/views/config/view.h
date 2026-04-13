#pragma once

#include <iomanip>

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct config_panel_t {
  std::vector<cuwacunu::iinuji::styled_text_line_t> lines{};
  std::optional<std::size_t> selected_line{};
};

inline void
push_config_line(std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
                 std::string text,
                 cuwacunu::iinuji::text_line_emphasis_t emphasis =
                     cuwacunu::iinuji::text_line_emphasis_t::None) {
  out.push_back(cuwacunu::iinuji::styled_text_line_t{
      .text = std::move(text),
      .emphasis = emphasis,
  });
}

inline void push_config_segments(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::vector<cuwacunu::iinuji::styled_text_segment_t> segments,
    cuwacunu::iinuji::text_line_emphasis_t fallback_emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::None) {
  out.push_back(cuwacunu::iinuji::make_segmented_styled_text_line(
      std::move(segments), fallback_emphasis));
}

inline cuwacunu::iinuji::text_line_emphasis_t
config_family_emphasis(ConfigFileFamily family) {
  switch (family) {
  case ConfigFileFamily::Defaults:
    return cuwacunu::iinuji::text_line_emphasis_t::Info;
  case ConfigFileFamily::Objectives:
    return cuwacunu::iinuji::text_line_emphasis_t::Debug;
  case ConfigFileFamily::Optim:
    return cuwacunu::iinuji::text_line_emphasis_t::MutedWarning;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Info;
}

inline cuwacunu::iinuji::text_line_emphasis_t
config_file_row_emphasis(const ConfigFileEntry &file, bool selected) {
  if (!file.ok) {
    return selected ? cuwacunu::iinuji::text_line_emphasis_t::Fatal
                    : cuwacunu::iinuji::text_line_emphasis_t::Error;
  }
  if (selected)
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  if (file.extension == ".dsl") {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  if (file.extension == ".md") {
    return cuwacunu::iinuji::text_line_emphasis_t::Debug;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Info;
}

inline cuwacunu::iinuji::text_line_emphasis_t
config_file_type_emphasis(const ConfigFileEntry &file) {
  return config_file_row_emphasis(file, false);
}

inline cuwacunu::iinuji::text_line_emphasis_t
config_file_path_emphasis(const ConfigFileEntry &file) {
  return file.ok ? cuwacunu::iinuji::text_line_emphasis_t::Info
                 : cuwacunu::iinuji::text_line_emphasis_t::MutedError;
}

inline cuwacunu::iinuji::text_line_emphasis_t
config_file_prefix_emphasis(const ConfigFileEntry &file, bool selected) {
  if (selected)
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  return file.ok ? cuwacunu::iinuji::text_line_emphasis_t::Info
                 : cuwacunu::iinuji::text_line_emphasis_t::MutedError;
}

inline void append_config_section_header(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::string_view title,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Info) {
  push_config_line(out, panel_rule('-', 56), emphasis);
  push_config_line(out, std::string(title), emphasis);
  push_config_line(out, panel_rule('-', 56), emphasis);
}

inline std::string config_meta_prefix(std::string_view key) {
  constexpr std::size_t kConfigMetaKeyWidth = 18;
  std::string prefix = std::string(key) + ":";
  if (prefix.size() < kConfigMetaKeyWidth) {
    prefix.append(kConfigMetaKeyWidth - prefix.size(), ' ');
  } else {
    prefix.push_back(' ');
  }
  return prefix;
}

inline void
append_config_meta_line(std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
                        std::string_view key, std::string value,
                        cuwacunu::iinuji::text_line_emphasis_t emphasis =
                            cuwacunu::iinuji::text_line_emphasis_t::Info) {
  if (value.empty())
    value = "<none>";
  const auto key_emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info;
  const auto value_emphasis =
      (emphasis == cuwacunu::iinuji::text_line_emphasis_t::None ||
       emphasis == cuwacunu::iinuji::text_line_emphasis_t::Debug)
          ? key_emphasis
          : emphasis;
  const auto prefix = config_meta_prefix(key);
  std::size_t start = 0;
  while (start <= value.size()) {
    const std::size_t end = value.find('\n', start);
    const std::string_view line =
        end == std::string::npos
            ? std::string_view(value).substr(start)
            : std::string_view(value).substr(start, end - start);
    const std::string label =
        (start == 0 ? prefix : std::string(prefix.size(), ' '));
    push_config_segments(
        out,
        {cuwacunu::iinuji::styled_text_segment_t{.text = std::move(label),
                                                 .emphasis = key_emphasis},
         cuwacunu::iinuji::styled_text_segment_t{.text = std::string(line),
                                                 .emphasis = value_emphasis}},
        key_emphasis);
    if (end == std::string::npos)
      break;
    start = end + 1;
    if (start == value.size())
      break;
  }
}

inline void append_config_meta_segments(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::string_view key,
    std::vector<cuwacunu::iinuji::styled_text_segment_t> value_segments,
    cuwacunu::iinuji::text_line_emphasis_t fallback_emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Info) {
  const auto prefix = config_meta_prefix(key);
  std::vector<cuwacunu::iinuji::styled_text_segment_t> segments{};
  segments.reserve(value_segments.size() + 1u);
  segments.push_back(cuwacunu::iinuji::styled_text_segment_t{
      .text = prefix,
      .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info,
  });
  if (value_segments.empty()) {
    segments.push_back(cuwacunu::iinuji::styled_text_segment_t{
        .text = "<none>",
        .emphasis = fallback_emphasis,
    });
  } else {
    for (auto &segment : value_segments) {
      segments.push_back(std::move(segment));
    }
  }
  push_config_segments(out, std::move(segments), fallback_emphasis);
}

inline std::string make_config_left(const CmdState &st) {
  if (st.config.editor) {
    return cuwacunu::iinuji::primitives::editor_text(*st.config.editor);
  }
  if (!st.config.ok) {
    return "Config files not loaded.\n\nerror: " + st.config.error;
  }
  return "No config files.";
}

inline std::size_t count_config_family(const CmdState &st,
                                       ConfigFileFamily family) {
  std::size_t count = 0;
  for (const auto &file : st.config.files) {
    if (file.family == family)
      ++count;
  }
  return count;
}

inline std::string config_entry_badge(const ConfigFileEntry &file) {
  return config_access_indicator(file);
}

inline std::string config_entry_badge_padded(const ConfigFileEntry &file) {
  std::string badge = config_entry_badge(file);
  constexpr std::size_t kConfigAccessWidth = 5u;
  if (badge.size() < kConfigAccessWidth)
    badge.append(kConfigAccessWidth - badge.size(), ' ');
  return badge;
}

inline std::string config_family_count_text(const CmdState &st,
                                            ConfigFileFamily family) {
  const std::size_t count = count_config_family(st, family);
  std::ostringstream oss;
  oss << count << " file";
  if (count != 1)
    oss << "s";
  return oss.str();
}

inline cuwacunu::iinuji::text_line_emphasis_t
config_family_row_emphasis(ConfigFileFamily family, bool selected,
                           bool focused) {
  if (selected) {
    return focused ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                   : cuwacunu::iinuji::text_line_emphasis_t::Info;
  }
  return config_family_emphasis(family);
}

inline std::string config_family_row_text(const CmdState &st,
                                          ConfigFileFamily family,
                                          bool selected) {
  std::ostringstream oss;
  oss << (selected ? "> " : "  ") << "["
      << trim_to_width(config_family_title(family), 10) << "] "
      << config_family_count_text(st, family);
  return oss.str();
}

struct config_path_parts_t {
  std::string prefix{};
  std::string filename{};
};

inline config_path_parts_t config_split_path_parts(std::string_view raw_path) {
  config_path_parts_t parts{};
  if (raw_path.empty())
    return parts;
  const auto slash = raw_path.find_last_of("/\\");
  if (slash == std::string::npos) {
    parts.filename = std::string(raw_path);
    return parts;
  }
  parts.prefix = std::string(raw_path.substr(0, slash + 1));
  parts.filename = std::string(raw_path.substr(slash + 1));
  if (parts.filename.empty()) {
    parts.filename = parts.prefix;
    parts.prefix.clear();
  }
  return parts;
}

inline std::vector<cuwacunu::iinuji::styled_text_segment_t>
config_path_segments(std::string_view raw_path,
                     cuwacunu::iinuji::text_line_emphasis_t path_emphasis,
                     cuwacunu::iinuji::text_line_emphasis_t filename_emphasis) {
  std::vector<cuwacunu::iinuji::styled_text_segment_t> segments{};
  if (raw_path.empty()) {
    segments.push_back(cuwacunu::iinuji::styled_text_segment_t{
        .text = "<none>",
        .emphasis = path_emphasis,
    });
    return segments;
  }
  const auto parts = config_split_path_parts(raw_path);
  if (!parts.prefix.empty()) {
    segments.push_back(cuwacunu::iinuji::styled_text_segment_t{
        .text = parts.prefix,
        .emphasis = path_emphasis,
    });
  }
  segments.push_back(cuwacunu::iinuji::styled_text_segment_t{
      .text = parts.filename.empty() ? std::string(raw_path) : parts.filename,
      .emphasis = filename_emphasis,
  });
  return segments;
}

inline std::string config_display_path(const ConfigFileEntry &file) {
  if (file.family == ConfigFileFamily::Objectives &&
      !file.request_path.empty()) {
    return file.request_path;
  }
  return file.relative_path.empty() ? file.path : file.relative_path;
}

inline std::vector<cuwacunu::iinuji::styled_text_segment_t>
config_file_row_segments(const ConfigFileEntry &file, std::size_t ordinal,
                         std::size_t total, bool selected) {
  using cuwacunu::iinuji::styled_text_segment_t;
  std::vector<styled_text_segment_t> segments{};
  const std::size_t width = std::max<std::size_t>(
      2, std::to_string(std::max<std::size_t>(1, total)).size());

  std::ostringstream prefix;
  prefix << (selected ? "> " : "  ") << "["
         << std::setw(static_cast<int>(width)) << ordinal << "] ["
         << config_entry_badge_padded(file) << "] ";
  segments.push_back(styled_text_segment_t{
      .text = prefix.str(),
      .emphasis = config_file_prefix_emphasis(file, selected),
  });
  if (file.family == ConfigFileFamily::Objectives) {
    segments.push_back(styled_text_segment_t{
        .text = "  ",
        .emphasis = config_file_path_emphasis(file),
    });
  }
  auto path_segments = config_path_segments(config_display_path(file),
                                            config_file_path_emphasis(file),
                                            config_file_type_emphasis(file));
  for (auto &segment : path_segments) {
    segments.push_back(std::move(segment));
  }
  return segments;
}

inline std::string config_objective_group_label(const ConfigFileEntry &file) {
  const auto raw = config_file_objective_group_key(file);
  if (raw.empty())
    return "<objective>";
  const auto slash = raw.find_last_of("/\\");
  return std::string((slash == std::string::npos) ? raw
                                                  : raw.substr(slash + 1));
}

inline std::vector<cuwacunu::iinuji::styled_text_segment_t>
config_objective_group_segments(const ConfigFileEntry &file) {
  using cuwacunu::iinuji::styled_text_segment_t;
  return {
      styled_text_segment_t{
          .text = "  objective: ",
          .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info,
      },
      styled_text_segment_t{
          .text = config_objective_group_label(file),
          .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Debug,
      },
  };
}

inline void append_config_legend_line(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::string_view key, std::string indicator, std::string description,
    cuwacunu::iinuji::text_line_emphasis_t indicator_emphasis,
    cuwacunu::iinuji::text_line_emphasis_t description_emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Info) {
  const std::string prefix =
      key.empty() ? std::string(config_meta_prefix("access").size(), ' ')
                  : config_meta_prefix(key);
  push_config_segments(
      out,
      {cuwacunu::iinuji::styled_text_segment_t{
           .text = prefix,
           .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info,
       },
       cuwacunu::iinuji::styled_text_segment_t{
           .text = "[" + indicator + "] ",
           .emphasis = indicator_emphasis,
       },
       cuwacunu::iinuji::styled_text_segment_t{
           .text = std::move(description),
           .emphasis = description_emphasis,
       }},
      description_emphasis);
}

inline void
append_config_legend(std::vector<cuwacunu::iinuji::styled_text_line_t> &out) {
  append_config_section_header(out, "Access Legend");
  append_config_legend_line(out, "access", "rw",
                            "editable in the current write scope",
                            cuwacunu::iinuji::text_line_emphasis_t::Success);
  append_config_legend_line(
      out, {}, "scope", "replace supported, but outside the active write scope",
      cuwacunu::iinuji::text_line_emphasis_t::Info);
  append_config_legend_line(out, {}, "ro", "read only in this screen",
                            cuwacunu::iinuji::text_line_emphasis_t::Info);
  append_config_legend_line(out, {}, "err", "file failed to load",
                            cuwacunu::iinuji::text_line_emphasis_t::Error,
                            cuwacunu::iinuji::text_line_emphasis_t::Error);
}

inline void append_config_root_lines(std::ostringstream &oss,
                                     std::string_view label,
                                     const std::vector<std::string> &roots) {
  if (roots.empty()) {
    append_meta_line(oss, label, "<none>");
    return;
  }
  append_meta_line(oss, label, join_trimmed_values(roots, 6, 72));
}

inline void
append_config_root_lines(std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
                         std::string_view label,
                         const std::vector<std::string> &roots,
                         cuwacunu::iinuji::text_line_emphasis_t emphasis =
                             cuwacunu::iinuji::text_line_emphasis_t::Debug) {
  append_config_meta_line(out, label,
                          roots.empty() ? std::string("<none>")
                                        : join_trimmed_values(roots, 6, 72),
                          emphasis);
}

inline config_panel_t make_config_families_panel(const CmdState &st) {
  config_panel_t panel{};
  auto &out = panel.lines;
  for (std::size_t i = 0; i < config_family_count(); ++i) {
    const auto family = config_family_from_index(i);
    const bool selected = (family == st.config.selected_family);
    if (selected)
      panel.selected_line = out.size();
    push_config_line(out, config_family_row_text(st, family, selected),
                     config_family_row_emphasis(
                         family, selected, config_is_family_focus(st.config)));
  }
  return panel;
}

inline config_panel_t make_config_files_panel(const CmdState &st) {
  config_panel_t panel{};
  auto &out = panel.lines;
  const auto indices =
      config_file_indices_for_family(st, st.config.selected_family);
  if (indices.empty()) {
    push_config_line(out, "(none in selected family)",
                     cuwacunu::iinuji::text_line_emphasis_t::Info);
    return panel;
  }
  std::string current_objective_group{};
  std::size_t visible_index = 1u;
  for (std::size_t i = 0; i < indices.size(); ++i) {
    const std::size_t global_index = indices[i];
    const auto &file = st.config.files[global_index];
    if (st.config.selected_family == ConfigFileFamily::Objectives) {
      const auto objective_group = config_file_objective_group_key(file);
      if (objective_group != current_objective_group) {
        if (!current_objective_group.empty())
          push_config_line(out, {});
        current_objective_group = objective_group;
        push_config_segments(out, config_objective_group_segments(file),
                             cuwacunu::iinuji::text_line_emphasis_t::Info);
      }
    }
    const bool selected = (global_index == st.config.selected_file);
    if (selected)
      panel.selected_line = out.size();
    push_config_segments(
        out,
        config_file_row_segments(file, visible_index, indices.size(), selected),
        config_file_row_emphasis(file, selected));
    ++visible_index;
  }
  return panel;
}

inline void append_config_family_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    ConfigFileFamily family) {
  append_config_meta_line(out, "family", config_family_title(family),
                          config_family_emphasis(family));
  append_config_meta_line(out, "files", config_family_count_text(st, family),
                          config_family_emphasis(family));
  if (family == ConfigFileFamily::Defaults) {
    append_config_root_lines(out, "roots", st.config.default_roots,
                             cuwacunu::iinuji::text_line_emphasis_t::Debug);
  } else if (family == ConfigFileFamily::Objectives) {
    append_config_root_lines(out, "roots", st.config.objective_roots,
                             cuwacunu::iinuji::text_line_emphasis_t::Debug);
  } else {
    append_config_meta_line(out, "root", st.config.optim_root,
                            cuwacunu::iinuji::text_line_emphasis_t::Debug);
    append_config_meta_line(out, "archive", st.config.optim_archive_path,
                            cuwacunu::iinuji::text_line_emphasis_t::Debug);
    append_config_meta_line(out, "public keep",
                            st.config.optim_public_keep_path,
                            cuwacunu::iinuji::text_line_emphasis_t::Debug);
  }
}

inline config_panel_t make_config_right_panel(const CmdState &st) {
  config_panel_t panel{};
  auto &out = panel.lines;

  append_config_section_header(out, "Selected File");
  const ConfigFileEntry *file =
      selected_config_file_for_family(st, st.config.selected_family);
  if (file == nullptr) {
    push_config_line(out, "(no file selected in active family)",
                     cuwacunu::iinuji::text_line_emphasis_t::Info);
    push_config_line(out, {});
  } else {
    append_config_meta_line(out, "open", "Enter or e opens file",
                            cuwacunu::iinuji::text_line_emphasis_t::Accent);
    append_config_meta_line(out, "family", file->family_title,
                            config_family_emphasis(file->family));
    append_config_meta_segments(
        out, "access",
        {cuwacunu::iinuji::styled_text_segment_t{
             .text = "[" + config_entry_badge(*file) + "] ",
             .emphasis = config_file_prefix_emphasis(*file, false),
         },
         cuwacunu::iinuji::styled_text_segment_t{
             .text = config_access_description(*file),
             .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info,
         }},
        config_file_prefix_emphasis(*file, false));
    append_config_meta_line(out, "mode", config_readable_mode(*file),
                            config_file_type_emphasis(*file));
    append_config_meta_line(out, "extension", file->extension,
                            config_file_type_emphasis(*file));
    append_config_meta_segments(
        out, "relative path",
        config_path_segments(file->relative_path,
                             config_file_path_emphasis(*file),
                             config_file_type_emphasis(*file)),
        config_file_path_emphasis(*file));
    append_config_meta_segments(
        out, "path",
        config_path_segments(file->path, config_file_path_emphasis(*file),
                             config_file_type_emphasis(*file)),
        config_file_path_emphasis(*file));
    append_config_meta_line(out, "sha256",
                            file->sha256.empty() ? std::string("<unknown>")
                                                 : file->sha256,
                            cuwacunu::iinuji::text_line_emphasis_t::Debug);
    if (!file->ok) {
      append_config_meta_line(out, "error", file->error,
                              cuwacunu::iinuji::text_line_emphasis_t::Error);
    }
    push_config_line(out, {});
  }

  append_config_section_header(out, "Selected Family");
  append_config_family_detail(out, st, st.config.selected_family);
  push_config_line(out, {});

  append_config_section_header(out, "Write Policy");
  append_config_meta_line(out, "catalog source", "global default");
  append_config_meta_line(out, "catalog policy path", st.config.policy_path,
                          cuwacunu::iinuji::text_line_emphasis_t::Debug);
  append_config_meta_line(out, "write scope",
                          st.config.using_session_policy ? "selected marshal"
                                                         : "global default");
  if (!st.config.write_policy_path.empty() &&
      st.config.write_policy_path != st.config.policy_path) {
    append_config_meta_line(out, "write policy path",
                            st.config.write_policy_path,
                            cuwacunu::iinuji::text_line_emphasis_t::Debug);
  }
  append_config_meta_line(out, "allowed extensions",
                          st.config.allowed_extensions);
  append_config_meta_line(out, "allow_local_write",
                          st.config.policy_write_allowed ? "true" : "false",
                          st.config.policy_write_allowed
                              ? cuwacunu::iinuji::text_line_emphasis_t::Success
                              : cuwacunu::iinuji::text_line_emphasis_t::Info);
  append_config_root_lines(out, "write roots", st.config.write_roots,
                           cuwacunu::iinuji::text_line_emphasis_t::Debug);
  push_config_line(out, {});

  append_config_legend(out);
  return panel;
}

inline std::string make_config_right(const CmdState &st) {
  const auto panel = make_config_right_panel(st);
  std::ostringstream oss;
  for (std::size_t i = 0; i < panel.lines.size(); ++i) {
    if (i > 0)
      oss << '\n';
    oss << cuwacunu::iinuji::styled_text_line_text(panel.lines[i]);
  }
  return oss.str();
}

struct config_styled_box_metrics_t {
  int text_h{0};
  int total_rows{0};
  int max_scroll_y{0};
  int selected_row{0};
};

inline config_styled_box_metrics_t measure_config_styled_box(
    const std::vector<cuwacunu::iinuji::styled_text_line_t> &lines, int width,
    int height, bool wrap, std::optional<std::size_t> selected_line) {
  config_styled_box_metrics_t out{};
  if (width <= 0 || height <= 0)
    return out;

  int reserve_v = 0;
  int reserve_h = 0;
  int text_w = width;
  int text_h = height;
  int total_rows = 0;
  int max_line_len = 0;

  auto measure_lines = [&](int current_width, int *out_total_rows,
                           int *out_max_line_len,
                           std::vector<int> *out_line_offsets) {
    const int safe_w = std::max(1, current_width);
    int rows = 0;
    int line_len = 0;
    if (out_line_offsets)
      out_line_offsets->assign(lines.size(), 0);
    for (std::size_t i = 0; i < lines.size(); ++i) {
      if (out_line_offsets)
        (*out_line_offsets)[i] = rows;
      if (cuwacunu::iinuji::ansi::has_esc(lines[i].text)) {
        cuwacunu::iinuji::ansi::style_t base{};
        const auto physical_lines = split_lines_keep_empty(lines[i].text);
        if (physical_lines.empty()) {
          ++rows;
          continue;
        }
        for (const auto &physical_line : physical_lines) {
          std::vector<cuwacunu::iinuji::ansi::row_t> wrapped_rows{};
          cuwacunu::iinuji::ansi::hard_wrap(physical_line, safe_w, base, 0,
                                            wrapped_rows);
          if (wrapped_rows.empty()) {
            ++rows;
            continue;
          }
          for (std::size_t j = 0; j < wrapped_rows.size(); ++j) {
            line_len = std::max(line_len, wrapped_rows[j].len);
            ++rows;
            if (!wrap)
              break;
          }
          if (!wrap)
            break;
        }
        continue;
      }
      const auto chunks = wrap ? wrap_text(lines[i].text, safe_w)
                               : split_lines_keep_empty(lines[i].text);
      if (chunks.empty()) {
        ++rows;
        continue;
      }
      for (std::size_t j = 0; j < chunks.size(); ++j) {
        line_len = std::max(line_len, static_cast<int>(chunks[j].size()));
        ++rows;
        if (!wrap)
          break;
      }
    }
    if (out_total_rows)
      *out_total_rows = rows;
    if (out_max_line_len)
      *out_max_line_len = line_len;
  };

  for (int it = 0; it < 3; ++it) {
    text_w = std::max(0, width - reserve_v);
    text_h = std::max(0, height - reserve_h);
    if (text_w <= 0 || text_h <= 0)
      return out;

    measure_lines(text_w, &total_rows, &max_line_len, nullptr);

    const bool need_h = (!wrap && max_line_len > text_w);
    const int reserve_h_new = need_h ? 1 : 0;
    const int text_h_if = std::max(0, height - reserve_h_new);
    const bool need_v = total_rows > text_h_if;
    const int reserve_v_new = need_v ? 1 : 0;
    if (reserve_h_new == reserve_h && reserve_v_new == reserve_v)
      break;
    reserve_h = reserve_h_new;
    reserve_v = reserve_v_new;
  }

  text_w = std::max(0, width - reserve_v);
  text_h = std::max(0, height - reserve_h);
  if (text_w <= 0 || text_h <= 0)
    return out;

  std::vector<int> line_offsets{};
  measure_lines(text_w, &total_rows, &max_line_len, &line_offsets);

  out.text_h = text_h;
  out.total_rows = total_rows;
  out.max_scroll_y = std::max(0, total_rows - text_h);
  if (selected_line.has_value() && *selected_line < line_offsets.size()) {
    out.selected_row = line_offsets[*selected_line];
  }
  return out;
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
