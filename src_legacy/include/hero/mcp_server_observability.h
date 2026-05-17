#pragma once

#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>

#include "hero/runtime_dev_loop.h"

namespace cuwacunu {
namespace hero {
namespace mcp_observability {

struct log_paths_t {
  std::filesystem::path stderr_log_path{};
  std::filesystem::path events_log_path{};
};

[[nodiscard]] inline std::filesystem::path resolve_runtime_root_from_global_config(
    const std::filesystem::path& global_config_path, std::string* error = nullptr) {
  if (error) error->clear();
  std::string runtime_root{};
  const std::filesystem::path normalized =
      global_config_path.lexically_normal();
  if (!cuwacunu::hero::runtime_dev::detail::read_ini_value(
          normalized, "GENERAL", "runtime_root", &runtime_root) ||
      cuwacunu::hero::runtime_dev::detail::trim_ascii(runtime_root).empty()) {
    if (error) {
      *error = "missing [GENERAL].runtime_root in " + normalized.string();
    }
    return {};
  }
  const std::filesystem::path base_folder = normalized.parent_path();
  return std::filesystem::path(
             cuwacunu::hero::runtime_dev::detail::resolve_path_from_base_folder(
                 base_folder.string(), runtime_root))
      .lexically_normal();
}

[[nodiscard]] inline std::filesystem::path derive_config_mcp_log_dir(
    const std::filesystem::path& global_config_path, std::string* error = nullptr) {
  const std::filesystem::path runtime_root =
      resolve_runtime_root_from_global_config(global_config_path, error);
  if (runtime_root.empty()) return {};
  return (runtime_root / ".config_hero" / "mcp" / "hero_config_mcp")
      .lexically_normal();
}

[[nodiscard]] inline std::filesystem::path derive_store_backed_mcp_log_dir(
    const std::filesystem::path& store_root, std::string_view server_name) {
  if (store_root.empty() || server_name.empty()) return {};
  return (store_root / "_meta" / "mcp" / std::string(server_name))
      .lexically_normal();
}

[[nodiscard]] inline log_paths_t make_log_paths(
    const std::filesystem::path& log_dir) {
  if (log_dir.empty()) return {};
  return log_paths_t{
      .stderr_log_path = (log_dir / "stderr.log").lexically_normal(),
      .events_log_path = (log_dir / "events.jsonl").lexically_normal(),
  };
}

inline std::filesystem::path& stderr_log_path_storage() {
  static std::filesystem::path path{};
  return path;
}

inline std::filesystem::path& events_log_path_storage() {
  static std::filesystem::path path{};
  return path;
}

inline void configure_log_paths(const log_paths_t& paths) {
  stderr_log_path_storage() = paths.stderr_log_path.lexically_normal();
  events_log_path_storage() = paths.events_log_path.lexically_normal();
}

inline void clear_log_paths() {
  stderr_log_path_storage().clear();
  events_log_path_storage().clear();
}

[[nodiscard]] inline bool append_optional_text_file(
    const std::filesystem::path& path, std::string_view content,
    std::string* error = nullptr) {
  if (path.empty()) {
    if (error) error->clear();
    return true;
  }
  return cuwacunu::hero::runtime::append_text_file(path, content, error);
}

inline void mirror_stderr_text(std::string_view text) {
  std::string ignored{};
  (void)append_optional_text_file(stderr_log_path_storage(), text, &ignored);
}

[[nodiscard]] inline std::string json_escape_copy(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 8);
  for (const unsigned char c : text) {
    switch (c) {
    case '\\':
      out += "\\\\";
      break;
    case '"':
      out += "\\\"";
      break;
    case '\b':
      out += "\\b";
      break;
    case '\f':
      out += "\\f";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if (c < 0x20) {
        constexpr char kDigits[] = "0123456789abcdef";
        out += "\\u00";
        out.push_back(kDigits[(c >> 4) & 0x0f]);
        out.push_back(kDigits[c & 0x0f]);
      } else {
        out.push_back(static_cast<char>(c));
      }
      break;
    }
  }
  return out;
}

inline void append_json_string_field(std::ostringstream& out,
                                     std::string_view key,
                                     std::string_view value) {
  out << ",\"" << key << "\":\"" << json_escape_copy(value) << "\"";
}

inline void append_json_bool_field(std::ostringstream& out, std::string_view key,
                                   bool value) {
  out << ",\"" << key << "\":" << (value ? "true" : "false");
}

inline void append_json_uint64_field(std::ostringstream& out,
                                     std::string_view key,
                                     std::uint64_t value) {
  out << ",\"" << key << "\":" << value;
}

inline void append_json_int_field(std::ostringstream& out, std::string_view key,
                                  int value) {
  out << ",\"" << key << "\":" << value;
}

[[nodiscard]] inline bool append_event_json(
    std::string_view server_name, std::string_view event_name,
    std::string_view extra_fields_json = {}, std::string* error = nullptr) {
  std::ostringstream out;
  out << "{\"timestamp_ms\":" << cuwacunu::hero::runtime::now_ms_utc()
      << ",\"server\":\"" << json_escape_copy(server_name) << "\""
      << ",\"event\":\"" << json_escape_copy(event_name) << "\"";
  if (!extra_fields_json.empty()) out << extra_fields_json;
  out << "}\n";
  return append_optional_text_file(events_log_path_storage(), out.str(), error);
}

}  // namespace mcp_observability
}  // namespace hero
}  // namespace cuwacunu
