#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "hero/config_hero/hero_config_store.h"
#include "hero/config_hero/hero_config_tools.h"
#include "iinuji/iinuji_cmd/views/common/base.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string resolve_config_relative_path(const std::string &base_file,
                                                const std::string &raw_path) {
  if (raw_path.empty())
    return {};
  const std::filesystem::path raw(raw_path);
  if (raw.is_absolute())
    return raw.lexically_normal().string();
  return (std::filesystem::path(base_file).parent_path() / raw)
      .lexically_normal()
      .string();
}

inline std::string
hero_config_path_from_global_key(const std::string &global_path,
                                 const char *key) {
  std::string value{};
  if (!lookup_global_config_value("REAL_HERO", key, &value))
    return {};
  return resolve_config_relative_path(global_path, value);
}

inline std::string config_family_id(ConfigFileFamily family) {
  switch (family) {
  case ConfigFileFamily::Defaults:
    return "defaults";
  case ConfigFileFamily::Objectives:
    return "objectives";
  case ConfigFileFamily::Optim:
    return "optim";
  case ConfigFileFamily::Temp:
    return "temp";
  }
  return "config";
}

inline std::string config_family_title(ConfigFileFamily family) {
  switch (family) {
  case ConfigFileFamily::Defaults:
    return "Defaults";
  case ConfigFileFamily::Objectives:
    return "Objectives";
  case ConfigFileFamily::Optim:
    return "Optim";
  case ConfigFileFamily::Temp:
    return "Temp";
  }
  return "Config";
}

inline constexpr std::size_t config_family_count() { return 4u; }

inline std::size_t config_family_index(ConfigFileFamily family) {
  switch (family) {
  case ConfigFileFamily::Defaults:
    return 0u;
  case ConfigFileFamily::Objectives:
    return 1u;
  case ConfigFileFamily::Optim:
    return 2u;
  case ConfigFileFamily::Temp:
    return 3u;
  }
  return 0u;
}

inline ConfigFileFamily config_family_from_index(std::size_t index) {
  switch (index % config_family_count()) {
  case 0u:
    return ConfigFileFamily::Defaults;
  case 1u:
    return ConfigFileFamily::Objectives;
  case 2u:
    return ConfigFileFamily::Optim;
  case 3u:
    return ConfigFileFamily::Temp;
  }
  return ConfigFileFamily::Defaults;
}

inline bool config_is_family_focus(const ConfigState &st) {
  return st.browse_focus == ConfigBrowseFocus::Families;
}

inline bool config_is_file_focus(const ConfigState &st) {
  return st.browse_focus == ConfigBrowseFocus::Files;
}

inline bool config_parse_bool(std::string raw, bool default_value = false) {
  raw = to_lower_copy(trim_copy(raw));
  if (raw == "true" || raw == "1" || raw == "yes" || raw == "on")
    return true;
  if (raw == "false" || raw == "0" || raw == "no" || raw == "off")
    return false;
  return default_value;
}

inline std::vector<std::string> config_split_csv_tokens(std::string raw) {
  std::vector<std::string> out{};
  std::string token{};
  std::istringstream iss(raw);
  while (std::getline(iss, token, ',')) {
    token = trim_copy(token);
    if (!token.empty())
      out.push_back(token);
  }
  return out;
}

inline std::vector<std::string>
config_resolve_csv_paths(const std::string &policy_path,
                         const std::string &raw_csv) {
  std::vector<std::string> out{};
  for (const auto &token : config_split_csv_tokens(raw_csv)) {
    const std::string resolved =
        resolve_config_relative_path(policy_path, token);
    if (!resolved.empty())
      out.push_back(resolved);
  }
  return out;
}

inline bool config_path_is_within_root(std::string_view root_raw,
                                       std::string_view path_raw) {
  if (root_raw.empty() || path_raw.empty())
    return false;
  const std::filesystem::path root =
      std::filesystem::path(root_raw).lexically_normal();
  const std::filesystem::path path =
      std::filesystem::path(path_raw).lexically_normal();
  auto root_it = root.begin();
  auto path_it = path.begin();
  for (; root_it != root.end(); ++root_it, ++path_it) {
    if (path_it == path.end() || *root_it != *path_it)
      return false;
  }
  return true;
}

inline bool
config_path_is_within_any_root(const std::vector<std::string> &roots,
                               std::string_view path_raw) {
  for (const auto &root : roots) {
    if (config_path_is_within_root(root, path_raw))
      return true;
  }
  return false;
}

inline std::string config_resolve_selected_session_policy_path(
    const std::string &global_path,
    const cuwacunu::hero::marshal::marshal_session_record_t *selected_session) {
  if (selected_session == nullptr ||
      trim_copy(selected_session->config_policy_path).empty()) {
    return {};
  }
  const std::string candidate = resolve_config_relative_path(
      global_path, selected_session->config_policy_path);
  std::error_code ec{};
  if (!std::filesystem::exists(candidate, ec) ||
      !std::filesystem::is_regular_file(candidate, ec)) {
    return {};
  }
  return candidate;
}

struct config_objective_request_scope_t {
  std::string root_path{};
  std::string request_path{};
};

inline config_objective_request_scope_t
config_objective_request_scope(std::string_view objective_root,
                               std::string_view display_relative_path) {
  config_objective_request_scope_t out{};
  out.root_path = std::string(objective_root);
  out.request_path = std::string(display_relative_path);

  const std::filesystem::path rel(display_relative_path);
  if (rel.empty())
    return out;

  auto part = rel.begin();
  if (part == rel.end())
    return out;
  const std::filesystem::path first_component = *part;
  ++part;
  if (part == rel.end())
    return out;

  std::filesystem::path request_path{};
  for (; part != rel.end(); ++part)
    request_path /= *part;

  out.root_path = (std::filesystem::path(objective_root) / first_component)
                      .lexically_normal()
                      .string();
  out.request_path = request_path.string();
  return out;
}

inline std::string config_json_quote(std::string_view raw) {
  std::ostringstream oss;
  oss << '"';
  for (const unsigned char uc : raw) {
    switch (uc) {
    case '\\':
      oss << "\\\\";
      break;
    case '"':
      oss << "\\\"";
      break;
    case '\b':
      oss << "\\b";
      break;
    case '\f':
      oss << "\\f";
      break;
    case '\n':
      oss << "\\n";
      break;
    case '\r':
      oss << "\\r";
      break;
    case '\t':
      oss << "\\t";
      break;
    default:
      if (uc < 0x20) {
        static constexpr char kHex[] = "0123456789abcdef";
        oss << "\\u00" << kHex[(uc >> 4) & 0x0f] << kHex[uc & 0x0f];
      } else {
        oss << static_cast<char>(uc);
      }
      break;
    }
  }
  oss << '"';
  return oss.str();
}

inline std::string config_readable_mode(const ConfigFileEntry &file) {
  if (file.payload_loaded && !file.ok)
    return "load error";
  if (file.editable)
    return "editable via Config Hero replace";
  if (file.replace_supported && !file.write_allowed) {
    return "replace-supported, but current write scope excludes this file";
  }
  if (file.extension == ".md")
    return "read only markdown";
  return "read only";
}

inline std::string config_access_indicator(const ConfigFileEntry &file) {
  if (file.payload_loaded && !file.ok)
    return "err";
  if (file.editable)
    return "rw";
  if (file.replace_supported && !file.write_allowed)
    return "scope";
  return "ro";
}

inline std::string config_access_description(const ConfigFileEntry &file) {
  if (file.payload_loaded && !file.ok)
    return "file failed to load";
  if (file.editable)
    return "editable in the current write scope";
  if (file.replace_supported && !file.write_allowed) {
    return "replace supported, but outside the active write scope";
  }
  return "read only in this screen";
}

inline std::string config_default_status_message(const ConfigState &st) {
  if (!st.ok)
    return st.error;
  if (st.files.empty()) {
    return st.status.empty()
               ? std::string("no config files matched the active policy")
               : st.status;
  }
  const auto &file = st.files[st.selected_file];
  if (file.payload_loaded && !file.ok)
    return file.error.empty() ? "selected config file failed to load"
                              : file.error;
  return {};
}

inline void config_set_status(ConfigState &st, std::string message,
                              bool is_error = false) {
  st.status = std::move(message);
  st.status_is_error = is_error;
}

inline ConfigFileEntry *selected_config_file(CmdState &st) {
  if (!config_has_files(st))
    return nullptr;
  return &st.config.files[st.config.selected_file];
}

inline const ConfigFileEntry *selected_config_file(const CmdState &st) {
  if (!config_has_files(st))
    return nullptr;
  return &st.config.files[st.config.selected_file];
}

inline const ConfigFileEntry *
selected_config_file_for_family(const CmdState &st, ConfigFileFamily family) {
  const auto *file = selected_config_file(st);
  if (file == nullptr || file->family != family)
    return nullptr;
  return file;
}

inline ConfigFileEntry *
selected_config_file_for_family(CmdState &st, ConfigFileFamily family) {
  auto *file = selected_config_file(st);
  if (file == nullptr || file->family != family)
    return nullptr;
  return file;
}

inline std::string
config_file_objective_group_key(const ConfigFileEntry &file) {
  if (file.family != ConfigFileFamily::Objectives)
    return {};
  if (!file.request_root_path.empty())
    return file.request_root_path;
  if (!file.root_path.empty())
    return file.root_path;
  if (file.relative_path.empty())
    return {};
  const auto slash = file.relative_path.find_first_of("/\\");
  if (slash == std::string::npos)
    return file.relative_path;
  return file.relative_path.substr(0, slash);
}

inline std::vector<std::size_t>
config_file_indices_for_family(const CmdState &st, ConfigFileFamily family) {
  std::vector<std::size_t> indices{};
  if (!config_has_files(st))
    return indices;
  for (std::size_t i = 0; i < st.config.files.size(); ++i) {
    if (st.config.files[i].family == family)
      indices.push_back(i);
  }
  if (family != ConfigFileFamily::Objectives || indices.size() <= 1u) {
    return indices;
  }
  std::stable_sort(
      indices.begin(), indices.end(), [&](std::size_t lhs, std::size_t rhs) {
        const auto &left = st.config.files[lhs];
        const auto &right = st.config.files[rhs];
        const auto left_group = config_file_objective_group_key(left);
        const auto right_group = config_file_objective_group_key(right);
        if (left_group != right_group)
          return left_group < right_group;
        return left.request_path < right.request_path;
      });
  return indices;
}

inline std::optional<std::size_t>
config_first_file_index_for_family(const CmdState &st,
                                   ConfigFileFamily family) {
  const auto indices = config_file_indices_for_family(st, family);
  if (indices.empty())
    return std::nullopt;
  return indices.front();
}

inline std::optional<std::size_t> config_file_index_by_path(
    const ConfigState &st, std::string_view path,
    std::optional<ConfigFileFamily> family = std::nullopt) {
  if (path.empty())
    return std::nullopt;
  for (std::size_t i = 0; i < st.files.size(); ++i) {
    if (st.files[i].path != path)
      continue;
    if (family.has_value() && st.files[i].family != *family)
      continue;
    return i;
  }
  return std::nullopt;
}

inline bool config_file_load_failed(const ConfigFileEntry &file) {
  return file.payload_loaded && !file.ok;
}

inline void sync_config_editor_from_selection(ConfigState &st) {
  if (!st.editor)
    st.editor = std::make_shared<cuwacunu::iinuji::editorBox_data_t>();
  auto &ed = *st.editor;
  ed.show_header = true;
  ed.show_footer = false;
  ed.show_line_numbers = true;
  ed.show_tabs = true;
  ed.highlight_current_line = true;
  ed.highlight_matching_delimiter = true;
  ed.history_enabled = true;
  ed.status.clear();

  std::string text{};
  std::string path{};
  bool read_only = true;
  if (!st.ok) {
    text = st.error.empty() ? "Config screen failed to load." : st.error;
    ed.status = "policy load failed";
  } else if (st.files.empty()) {
    std::ostringstream oss;
    oss << "No config files matched the active policy.\n\n";
    oss << "Catalog: global\n";
    if (!st.policy_path.empty())
      oss << "Path: " << st.policy_path << "\n";
    oss << "Write scope: "
        << (st.using_session_policy ? "selected marshal" : "global default")
        << "\n";
    if (!st.write_policy_path.empty() &&
        st.write_policy_path != st.policy_path) {
      oss << "Write policy path: " << st.write_policy_path << "\n";
    }
    if (!st.allowed_extensions.empty()) {
      oss << "Allowed extensions: " << st.allowed_extensions << "\n";
    }
    text = oss.str();
    ed.status = "no files";
  } else {
    const auto &file = st.files[st.selected_file];
    path = file.path;
    read_only = !file.editable;
    if (!file.payload_loaded) {
      std::ostringstream oss;
      oss << "File metadata is loaded, but content is not hydrated yet.\n\n";
      oss << "Path: " << file.path << "\n";
      if (!file.relative_path.empty()) {
        oss << "Relative path: " << file.relative_path << "\n";
      }
      oss << "Mode: " << config_readable_mode(file) << "\n";
      oss << "\nPress Enter or e to load this file.\n";
      text = oss.str();
      ed.status = "metadata only";
    } else if (file.ok) {
      text = file.content;
      ed.status = config_readable_mode(file);
    } else {
      std::ostringstream oss;
      oss << "Failed to load file.\n\n";
      oss << "Path: " << file.path << "\n";
      oss << "Error: " << (file.error.empty() ? "<unknown>" : file.error)
          << "\n";
      text = oss.str();
      ed.status = "load error";
    }
  }

  ed.path = std::move(path);
  ed.read_only = read_only;
  cuwacunu::iinuji::primitives::editor_set_text(ed, text);
  ed.dirty = false;
  ed.close_armed = false;
  ed.close_armed_via_escape = false;
}

inline void config_refresh_default_status(ConfigState &st) {
  if (st.status.empty()) {
    st.status = config_default_status_message(st);
    st.status_is_error =
        !st.ok || (!st.files.empty() && !st.files[st.selected_file].ok);
    return;
  }
  if (!st.status_is_error && st.status == config_default_status_message(st))
    return;
}

inline std::string config_replace_tool_name(const ConfigFileEntry &file) {
  switch (file.family) {
  case ConfigFileFamily::Defaults:
    return "hero.config.default.replace";
  case ConfigFileFamily::Objectives:
    return "hero.config.objective.replace";
  case ConfigFileFamily::Optim:
    return "hero.config.optim.replace";
  case ConfigFileFamily::Temp:
    return "hero.config.temp.replace";
  }
  return {};
}

inline std::string config_read_tool_name(const ConfigFileEntry &file) {
  switch (file.family) {
  case ConfigFileFamily::Defaults:
    return "hero.config.default.read";
  case ConfigFileFamily::Objectives:
    return "hero.config.objective.read";
  case ConfigFileFamily::Optim:
    return "hero.config.optim.read";
  case ConfigFileFamily::Temp:
    return "hero.config.temp.read";
  }
  return {};
}

inline void config_refresh_entry_identity(ConfigFileEntry *file) {
  if (file == nullptr)
    return;
  file->family_id = config_family_id(file->family);
  file->family_title = config_family_title(file->family);
  file->extension =
      to_lower_copy(std::filesystem::path(file->path).extension().string());
  file->title = file->relative_path.empty()
                    ? std::filesystem::path(file->path).filename().string()
                    : file->relative_path;
  if (file->request_path.empty()) {
    file->request_path = file->family == ConfigFileFamily::Defaults ||
                                 file->family == ConfigFileFamily::Temp
                             ? file->path
                             : file->relative_path;
  }
  if (file->family != ConfigFileFamily::Defaults &&
      file->request_root_path.empty()) {
    file->request_root_path = file->root_path;
  }
  file->id = file->family_id + ":" + file->path;
  file->editable = file->replace_supported && file->write_allowed;
}

inline ConfigFileEntry
make_config_file_entry(ConfigFileFamily family, std::string root_path,
                       std::string path, std::string relative_path,
                       bool replace_supported, bool policy_write_allowed) {
  ConfigFileEntry entry{};
  entry.family = family;
  entry.root_path = std::move(root_path);
  entry.path = std::move(path);
  entry.relative_path = std::move(relative_path);
  entry.replace_supported = replace_supported;
  entry.write_allowed = policy_write_allowed;
  entry.ok = true;
  entry.payload_loaded = false;
  config_refresh_entry_identity(&entry);
  return entry;
}

inline bool config_invoke_tool(const std::string &tool_name,
                               const std::string &arguments_json,
                               cuwacunu::hero::mcp::hero_config_store_t *store,
                               cmd_json_value_t *out_structured,
                               std::string *error) {
  if (error)
    error->clear();
  std::string result_json{};
  if (!cuwacunu::hero::mcp::execute_tool_json(tool_name, arguments_json, store,
                                              &result_json, error)) {
    return false;
  }
  return cmd_parse_tool_structured_content(result_json, out_structured, error);
}

inline std::string config_show_value(const cmd_json_value_t *structured,
                                     std::string_view key) {
  const auto *entries = cmd_json_field(structured, "entries");
  if (entries == nullptr || entries->type != cmd_json_type_t::ARRAY ||
      !entries->arrayValue) {
    return {};
  }
  for (const auto &entry : *entries->arrayValue) {
    if (cmd_json_string(cmd_json_field(&entry, "key")) == key) {
      return cmd_json_string(cmd_json_field(&entry, "value"));
    }
  }
  return {};
}

inline std::string config_build_read_request_json(const ConfigFileEntry &file) {
  std::ostringstream oss;
  oss << "{";
  switch (file.family) {
  case ConfigFileFamily::Defaults:
    oss << "\"path\":"
        << config_json_quote(file.request_path.empty() ? file.path
                                                       : file.request_path);
    break;
  case ConfigFileFamily::Objectives:
    oss << "\"objective_root\":"
        << config_json_quote(file.request_root_path.empty()
                                 ? file.root_path
                                 : file.request_root_path)
        << ",";
    oss << "\"path\":"
        << config_json_quote(file.request_path.empty() ? file.relative_path
                                                       : file.request_path);
    break;
  case ConfigFileFamily::Optim:
  case ConfigFileFamily::Temp:
    oss << "\"path\":"
        << config_json_quote(file.request_path.empty()
                                 ? (file.family == ConfigFileFamily::Temp
                                        ? file.path
                                        : file.relative_path)
                                 : file.request_path);
    break;
  }
  oss << "}";
  return oss.str();
}

inline std::string
config_build_replace_request_json(const ConfigFileEntry &file,
                                  std::string_view content) {
  std::ostringstream oss;
  oss << "{";
  switch (file.family) {
  case ConfigFileFamily::Defaults:
    oss << "\"path\":"
        << config_json_quote(file.request_path.empty() ? file.path
                                                       : file.request_path)
        << ",";
    break;
  case ConfigFileFamily::Objectives:
    oss << "\"objective_root\":"
        << config_json_quote(file.request_root_path.empty()
                                 ? file.root_path
                                 : file.request_root_path)
        << ",";
    oss << "\"path\":"
        << config_json_quote(file.request_path.empty() ? file.relative_path
                                                       : file.request_path)
        << ",";
    break;
  case ConfigFileFamily::Optim:
  case ConfigFileFamily::Temp:
    oss << "\"path\":"
        << config_json_quote(file.request_path.empty()
                                 ? (file.family == ConfigFileFamily::Temp
                                        ? file.path
                                        : file.relative_path)
                                 : file.request_path)
        << ",";
    break;
  }
  oss << "\"content\":" << config_json_quote(content);
  if (!file.sha256.empty()) {
    oss << ",\"expected_sha256\":" << config_json_quote(file.sha256);
  }
  oss << "}";
  return oss.str();
}

inline bool
config_fill_file_payload(cuwacunu::hero::mcp::hero_config_store_t *store,
                         ConfigFileEntry *file) {
  if (file == nullptr)
    return false;
  file->saved_content.clear();
  file->content.clear();
  file->sha256.clear();
  file->error.clear();
  file->payload_loaded = false;
  file->ok = false;

  const std::string tool_name = config_read_tool_name(*file);
  if (tool_name.empty()) {
    file->error = "no Config Hero read tool for selected file";
    file->payload_loaded = true;
    return false;
  }

  cmd_json_value_t structured{};
  if (!config_invoke_tool(tool_name, config_build_read_request_json(*file),
                          store, &structured, &file->error)) {
    file->payload_loaded = true;
    return false;
  }

  file->path = cmd_json_string(cmd_json_field(&structured, "path"), file->path);
  file->relative_path = cmd_json_string(
      cmd_json_field(&structured, "relative_path"), file->relative_path);
  file->sha256 = cmd_json_string(cmd_json_field(&structured, "sha256"));
  file->replace_supported =
      cmd_json_bool(cmd_json_field(&structured, "replace_supported"),
                    file->replace_supported);
  file->saved_content = cmd_json_string(cmd_json_field(&structured, "content"));
  file->content = file->saved_content;
  file->ok = true;
  file->payload_loaded = true;
  config_refresh_entry_identity(file);
  return true;
}

inline std::string config_effective_write_policy_path(const ConfigState &st);

inline bool config_load_file_payload(ConfigState &st, ConfigFileEntry *file,
                                     bool force, std::string *error) {
  if (error)
    error->clear();
  if (file == nullptr) {
    if (error)
      *error = "no config file selected";
    return false;
  }
  if (file->payload_loaded && file->ok && !force)
    return true;

  const std::string store_policy_path =
      trim_copy(st.policy_path).empty() ? config_effective_write_policy_path(st)
                                        : st.policy_path;
  cuwacunu::hero::mcp::hero_config_store_t store(
      store_policy_path, cuwacunu::iitepi::config_space_t::config_file_path);
  std::string load_error{};
  if (!store.load(&load_error)) {
    if (error) {
      *error = "failed to load config policy: " + load_error;
    }
    return false;
  }
  if (!config_fill_file_payload(&store, file)) {
    if (error) {
      *error = file->error.empty() ? "failed to load selected config file"
                                   : file->error;
    }
    return false;
  }
  return true;
}

inline bool config_append_default_files_from_tool(
    const cmd_json_value_t *structured, bool policy_write_allowed,
    std::vector<ConfigFileEntry> *out_files, std::set<std::string> *seen_paths,
    std::vector<std::string> *out_roots, std::string *error) {
  if (error)
    error->clear();
  if (structured == nullptr || out_files == nullptr || seen_paths == nullptr ||
      out_roots == nullptr) {
    if (error)
      *error = "invalid default list output target";
    return false;
  }

  out_roots->clear();
  for (const auto &root :
       cmd_json_string_array(cmd_json_field(structured, "roots"))) {
    if (!root.empty())
      out_roots->push_back(root);
  }

  const auto *files = cmd_json_field(structured, "files");
  if (files == nullptr || files->type != cmd_json_type_t::ARRAY ||
      !files->arrayValue) {
    if (error)
      *error = "hero.config.default.list missing files array";
    return false;
  }

  for (const auto &file_value : *files->arrayValue) {
    if (file_value.type != cmd_json_type_t::OBJECT)
      continue;
    ConfigFileEntry entry = make_config_file_entry(
        ConfigFileFamily::Defaults,
        cmd_json_string(cmd_json_field(&file_value, "root")),
        cmd_json_string(cmd_json_field(&file_value, "path")),
        cmd_json_string(cmd_json_field(&file_value, "relative_path")),
        cmd_json_bool(cmd_json_field(&file_value, "replace_supported"), false),
        policy_write_allowed);
    if (entry.path.empty())
      continue;
    if (!seen_paths->insert(entry.path).second)
      continue;
    out_files->push_back(std::move(entry));
  }
  return true;
}

inline bool config_append_objective_files_from_tool(
    const cmd_json_value_t *structured, bool policy_write_allowed,
    std::vector<ConfigFileEntry> *out_files, std::set<std::string> *seen_paths,
    std::string *error) {
  if (error)
    error->clear();
  if (structured == nullptr || out_files == nullptr || seen_paths == nullptr) {
    if (error)
      *error = "invalid objective list output target";
    return false;
  }

  const std::string objective_root =
      cmd_json_string(cmd_json_field(structured, "objective_root"));
  const auto *files = cmd_json_field(structured, "files");
  if (files == nullptr || files->type != cmd_json_type_t::ARRAY ||
      !files->arrayValue) {
    if (error)
      *error = "hero.config.objective.list missing files array";
    return false;
  }

  for (const auto &file_value : *files->arrayValue) {
    if (file_value.type != cmd_json_type_t::OBJECT)
      continue;
    const std::string display_relative_path =
        cmd_json_string(cmd_json_field(&file_value, "relative_path"));
    const config_objective_request_scope_t request_scope =
        config_objective_request_scope(objective_root, display_relative_path);
    ConfigFileEntry entry = make_config_file_entry(
        ConfigFileFamily::Objectives, request_scope.root_path,
        cmd_json_string(cmd_json_field(&file_value, "path")),
        display_relative_path,
        cmd_json_bool(cmd_json_field(&file_value, "replace_supported"), false),
        policy_write_allowed);
    entry.request_root_path = request_scope.root_path;
    entry.request_path = request_scope.request_path;
    if (entry.path.empty())
      continue;
    if (!seen_paths->insert(entry.path).second)
      continue;
    out_files->push_back(std::move(entry));
  }
  return true;
}

inline bool config_append_optim_files_from_tool(
    const cmd_json_value_t *structured, bool policy_write_allowed,
    std::vector<ConfigFileEntry> *out_files, std::set<std::string> *seen_paths,
    ConfigState *out_state, std::string *error) {
  if (error)
    error->clear();
  if (structured == nullptr || out_files == nullptr || seen_paths == nullptr ||
      out_state == nullptr) {
    if (error)
      *error = "invalid optim list output target";
    return false;
  }

  out_state->optim_root =
      cmd_json_string(cmd_json_field(structured, "optim_root"));
  out_state->optim_archive_path =
      cmd_json_string(cmd_json_field(structured, "archive_path"));
  out_state->optim_public_keep_path =
      cmd_json_string(cmd_json_field(structured, "public_keep_path"));

  const auto *files = cmd_json_field(structured, "files");
  if (files == nullptr || files->type != cmd_json_type_t::ARRAY ||
      !files->arrayValue) {
    if (error)
      *error = "hero.config.optim.list missing files array";
    return false;
  }

  for (const auto &file_value : *files->arrayValue) {
    if (file_value.type != cmd_json_type_t::OBJECT)
      continue;
    ConfigFileEntry entry = make_config_file_entry(
        ConfigFileFamily::Optim, out_state->optim_root,
        cmd_json_string(cmd_json_field(&file_value, "path")),
        cmd_json_string(cmd_json_field(&file_value, "relative_path")),
        cmd_json_bool(cmd_json_field(&file_value, "replace_supported"), false),
        policy_write_allowed);
    if (entry.path.empty())
      continue;
    if (!seen_paths->insert(entry.path).second)
      continue;
    out_files->push_back(std::move(entry));
  }
  return true;
}

inline bool config_append_temp_files_from_tool(
    const cmd_json_value_t *structured, bool policy_write_allowed,
    std::vector<ConfigFileEntry> *out_files, std::set<std::string> *seen_paths,
    std::vector<std::string> *out_roots, std::string *error) {
  if (error)
    error->clear();
  if (structured == nullptr || out_files == nullptr || seen_paths == nullptr ||
      out_roots == nullptr) {
    if (error)
      *error = "invalid temp list output target";
    return false;
  }

  out_roots->clear();
  for (const auto &root :
       cmd_json_string_array(cmd_json_field(structured, "roots"))) {
    if (!root.empty())
      out_roots->push_back(root);
  }

  const auto *files = cmd_json_field(structured, "files");
  if (files == nullptr || files->type != cmd_json_type_t::ARRAY ||
      !files->arrayValue) {
    if (error)
      *error = "hero.config.temp.list missing files array";
    return false;
  }

  for (const auto &file_value : *files->arrayValue) {
    if (file_value.type != cmd_json_type_t::OBJECT)
      continue;
    ConfigFileEntry entry = make_config_file_entry(
        ConfigFileFamily::Temp,
        cmd_json_string(cmd_json_field(&file_value, "root")),
        cmd_json_string(cmd_json_field(&file_value, "path")),
        cmd_json_string(cmd_json_field(&file_value, "relative_path")),
        cmd_json_bool(cmd_json_field(&file_value, "replace_supported"), false),
        policy_write_allowed);
    if (entry.path.empty())
      continue;
    if (!seen_paths->insert(entry.path).second)
      continue;
    out_files->push_back(std::move(entry));
  }
  return true;
}

inline std::string config_effective_write_policy_path(const ConfigState &st) {
  return trim_copy(st.write_policy_path).empty() ? st.policy_path
                                                 : st.write_policy_path;
}

inline void config_apply_write_scope(ConfigState *st) {
  if (st == nullptr)
    return;
  for (auto &file : st->files) {
    file.write_allowed =
        st->policy_write_allowed &&
        config_path_is_within_any_root(st->write_roots, file.path);
    config_refresh_entry_identity(&file);
  }
}

inline ConfigState load_config_view_from_global_config(
    const std::string &global_path,
    const cuwacunu::hero::marshal::marshal_session_record_t *selected_session =
        nullptr,
    const std::string &preferred_selected_path = {}) {
  ConfigState out{};
  if (global_path.empty()) {
    out.ok = false;
    out.error = "missing global config path";
    sync_config_editor_from_selection(out);
    return out;
  }

  out.policy_path =
      hero_config_path_from_global_key(global_path, "config_hero_dsl_filename");
  out.write_policy_path = out.policy_path;
  out.using_session_policy = false;

  if (out.policy_path.empty()) {
    out.ok = false;
    out.error = "missing [REAL_HERO].config_hero_dsl_filename";
    sync_config_editor_from_selection(out);
    return out;
  }

  cuwacunu::hero::mcp::hero_config_store_t store(out.policy_path, global_path);
  std::string load_error{};
  if (!store.load(&load_error)) {
    out.ok = false;
    out.error = "failed to load config policy: " + load_error;
    sync_config_editor_from_selection(out);
    return out;
  }

  cmd_json_value_t show{};
  if (!config_invoke_tool("hero.config.show", "{}", &store, &show,
                          &load_error)) {
    out.ok = false;
    out.error = "failed to read config policy via Config Hero: " + load_error;
    sync_config_editor_from_selection(out);
    return out;
  }

  out.ok = true;
  out.allowed_extensions = config_show_value(&show, "allowed_extensions");
  out.policy_write_allowed =
      config_parse_bool(config_show_value(&show, "allow_local_write"), false);
  out.write_roots = config_resolve_csv_paths(
      out.policy_path, config_show_value(&show, "write_roots"));

  std::set<std::string> seen_paths{};
  std::vector<std::string> warnings{};
  const std::string session_policy_path =
      config_resolve_selected_session_policy_path(global_path,
                                                  selected_session);
  if (!session_policy_path.empty()) {
    cuwacunu::hero::mcp::hero_config_store_t write_store(session_policy_path,
                                                         global_path);
    cmd_json_value_t write_show{};
    std::string write_error{};
    if (write_store.load(&write_error) &&
        config_invoke_tool("hero.config.show", "{}", &write_store, &write_show,
                           &write_error)) {
      out.write_policy_path = session_policy_path;
      out.using_session_policy = true;
      out.policy_write_allowed = config_parse_bool(
          config_show_value(&write_show, "allow_local_write"), false);
      out.write_roots = config_resolve_csv_paths(
          session_policy_path, config_show_value(&write_show, "write_roots"));
    } else if (!write_error.empty()) {
      warnings.push_back("selected marshal write scope unavailable: " +
                         write_error);
    }
  }

  cmd_json_value_t default_list{};
  if (!config_invoke_tool("hero.config.default.list", "{}", &store,
                          &default_list, &load_error) ||
      !config_append_default_files_from_tool(&default_list, false, &out.files,
                                             &seen_paths, &out.default_roots,
                                             &load_error)) {
    out.ok = false;
    out.error =
        load_error.empty() ? "failed to load default config files" : load_error;
    sync_config_editor_from_selection(out);
    return out;
  }

  cmd_json_value_t temp_list{};
  if (!config_invoke_tool("hero.config.temp.list", "{}", &store, &temp_list,
                          &load_error) ||
      !config_append_temp_files_from_tool(&temp_list, false, &out.files,
                                          &seen_paths, &out.temp_roots,
                                          &load_error)) {
    out.ok = false;
    out.error =
        load_error.empty() ? "failed to load temp config files" : load_error;
    sync_config_editor_from_selection(out);
    return out;
  }

  out.objective_roots = config_resolve_csv_paths(
      out.policy_path, config_show_value(&show, "objective_roots"));
  for (const auto &objective_root : out.objective_roots) {
    cmd_json_value_t objective_list{};
    const std::string args = std::string("{\"objective_root\":") +
                             config_json_quote(objective_root) + "}";
    if (!config_invoke_tool("hero.config.objective.list", args, &store,
                            &objective_list, &load_error)) {
      warnings.push_back(load_error);
      continue;
    }
    if (!config_append_objective_files_from_tool(
            &objective_list, false, &out.files, &seen_paths, &load_error)) {
      out.ok = false;
      out.error = load_error;
      sync_config_editor_from_selection(out);
      return out;
    }
  }

  cmd_json_value_t optim_list{};
  if (config_invoke_tool("hero.config.optim.list", "{}", &store, &optim_list,
                         &load_error)) {
    if (!config_append_optim_files_from_tool(&optim_list, false, &out.files,
                                             &seen_paths, &out, &load_error)) {
      out.ok = false;
      out.error = load_error;
      sync_config_editor_from_selection(out);
      return out;
    }
    const std::string note =
        cmd_json_string(cmd_json_field(&optim_list, "note"));
    if (!note.empty())
      warnings.push_back(note);
  } else if (!load_error.empty()) {
    warnings.push_back(load_error);
  }

  config_apply_write_scope(&out);

  if (!preferred_selected_path.empty()) {
    for (std::size_t i = 0; i < out.files.size(); ++i) {
      if (out.files[i].path == preferred_selected_path) {
        out.selected_file = i;
        break;
      }
    }
  }
  if (!out.files.empty()) {
    out.selected_family = out.files[out.selected_file].family;
  }

  if (!warnings.empty()) {
    config_set_status(out, warnings.front(), false);
  } else if (out.files.empty()) {
    std::ostringstream oss;
    oss << "no config files matched " << out.allowed_extensions;
    oss << " under the global config policy";
    config_set_status(out, oss.str(), false);
  } else {
    config_set_status(out, config_default_status_message(out),
                      !out.files[out.selected_file].ok);
  }
  sync_config_editor_from_selection(out);
  return out;
}

inline ConfigState load_config_view_from_state(const CmdState &st) {
  const auto selected_session = selected_operator_session_record(st);

  std::string preferred_selected_path{};
  if (config_has_files(st)) {
    preferred_selected_path = st.config.files[st.config.selected_file].path;
  }

  return load_config_view_from_global_config(
      cuwacunu::iitepi::config_space_t::config_file_path,
      selected_session.has_value() ? &(*selected_session) : nullptr,
      preferred_selected_path);
}

inline ConfigState load_config_view_from_config() {
  return load_config_view_from_global_config(
      cuwacunu::iitepi::config_space_t::config_file_path);
}

inline std::shared_ptr<ConfigAsyncState>
ensure_config_async_state(ConfigState &st) {
  if (!st.async)
    st.async = std::make_shared<ConfigAsyncState>();
  return st.async;
}

inline bool config_is_refresh_pending(const ConfigState &st) {
  return st.async != nullptr && st.async->refresh_pending;
}

inline bool queue_config_refresh(CmdState &st) {
  if (st.config.editor_focus)
    return false;
  const std::shared_ptr<ConfigAsyncState> async =
      ensure_config_async_state(st.config);
  if (async->refresh_pending) {
    async->refresh_requeue = true;
    return false;
  }

  std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
      selected_session{};
  selected_session = selected_operator_session_record(st);

  std::string preferred_selected_path{};
  if (config_has_files(st)) {
    preferred_selected_path = st.config.files[st.config.selected_file].path;
  }

  async->refresh_pending = true;
  async->refresh_requeue = false;
  const std::string global_path =
      cuwacunu::iitepi::config_space_t::config_file_path;
  async->refresh_future = std::async(
      std::launch::async,
      [global_path, selected_session,
       preferred_selected_path]() -> std::shared_ptr<ConfigState> {
        auto out = std::make_shared<ConfigState>();
        *out = load_config_view_from_global_config(
            global_path,
            selected_session.has_value() ? &(*selected_session) : nullptr,
            preferred_selected_path);
        return out;
      });
  return true;
}

inline bool poll_config_async_updates(CmdState &st) {
  const std::shared_ptr<ConfigAsyncState> async =
      ensure_config_async_state(st.config);
  if (!async->refresh_pending || !async->refresh_future.valid() ||
      async->refresh_future.wait_for(std::chrono::milliseconds(0)) !=
          std::future_status::ready) {
    return false;
  }

  std::shared_ptr<ConfigState> snapshot = async->refresh_future.get();
  async->refresh_pending = false;
  if (snapshot) {
    const std::string previous_status = st.config.status;
    const bool previous_status_is_error = st.config.status_is_error;
    const ConfigBrowseFocus previous_browse_focus = st.config.browse_focus;
    const ConfigFileFamily previous_family = st.config.selected_family;
    const std::string previous_selected_path =
        (config_has_files(st) &&
         st.config.selected_file < st.config.files.size())
            ? st.config.files[st.config.selected_file].path
            : std::string{};
    const std::shared_ptr<ConfigAsyncState> preserved_async = st.config.async;
    st.config = *snapshot;
    st.config.async = preserved_async;
    st.config.browse_focus = previous_browse_focus;
    if (const auto idx = config_file_index_by_path(
            st.config, previous_selected_path, previous_family);
        idx.has_value()) {
      st.config.selected_file = *idx;
      st.config.selected_family = previous_family;
    } else if (const auto idx =
                   config_file_index_by_path(st.config, previous_selected_path);
               idx.has_value()) {
      st.config.selected_file = *idx;
      st.config.selected_family = st.config.files[*idx].family;
    } else {
      clamp_selected_config_file(st);
      st.config.selected_family = previous_family;
      const auto *selected = selected_config_file(st);
      if (selected == nullptr ||
          selected->family != st.config.selected_family) {
        if (const auto idx = config_first_file_index_for_family(
                st, st.config.selected_family);
            idx.has_value()) {
          st.config.selected_file = *idx;
        }
      }
    }
    clamp_selected_config_file(st);
    sync_config_editor_from_selection(st.config);
    const bool preserve_previous_status =
        !previous_status.empty() && !previous_status_is_error &&
        previous_status != "Loading config view...";
    if (preserve_previous_status && st.config.status.empty()) {
      config_set_status(st.config, previous_status, false);
    }
  }
  if (async->refresh_requeue) {
    async->refresh_requeue = false;
    (void)queue_config_refresh(st);
  }
  return true;
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
