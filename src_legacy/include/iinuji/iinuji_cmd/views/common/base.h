#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "iinuji/primitives/editor.h"
#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_cmd/commands/iinuji.paths.h"
#include "iinuji/iinuji_cmd/state.h"
#include "piaabo/dconfig.h"
#include "piaabo/djson_parsing.h"

#ifdef timeout
#undef timeout
#endif

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string to_lower_copy(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

inline bool parse_positive_index(std::string_view s, std::size_t* out) {
  if (!out || s.empty()) return false;
  std::size_t v = 0;
  for (const char c : s) {
    if (c < '0' || c > '9') return false;
    v = v * 10 + (std::size_t)(c - '0');
  }
  if (v == 0) return false;
  *out = v;
  return true;
}

inline std::string short_type(std::string_view full) {
  const std::size_t p = full.rfind('.');
  if (p == std::string_view::npos) return std::string(full);
  return std::string(full.substr(p + 1));
}

inline std::string trim_to_width(const std::string& s, int width) {
  if (width <= 0) return {};
  if ((int)s.size() <= width) return s;
  if (width <= 3) return s.substr(0, (std::size_t)width);
  return s.substr(0, (std::size_t)(width - 3)) + "...";
}

inline std::string trim_copy(const std::string& s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

inline bool read_text_file_safe(const std::string& path, std::string* out, std::string* error) {
  if (out) out->clear();
  if (error) error->clear();
  if (path.empty()) {
    if (error) *error = "path is empty";
    return false;
  }
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error) *error = "cannot open file";
    return false;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  if (out) *out = ss.str();
  return true;
}

inline std::string format_file_status(const std::string& path) {
  namespace fs = std::filesystem;
  std::ostringstream oss;
  oss << path;
  if (path.empty()) {
    oss << " (unset)";
    return oss.str();
  }
  std::error_code ec;
  if (!fs::exists(path, ec)) {
    oss << " (missing)";
    return oss.str();
  }
  if (!fs::is_regular_file(path, ec)) {
    oss << " (not-regular)";
    return oss.str();
  }
  const auto bytes = fs::file_size(path, ec);
  if (!ec) oss << " (" << bytes << " bytes)";
  return oss.str();
}

inline std::string masked_preview(const std::string& s) {
  if (s.empty()) return "<empty>";
  std::string clean = trim_copy(s);
  if (clean.empty()) return "<empty>";
  if (clean.size() <= 4) return std::string(clean.size(), '*');
  std::string out = clean;
  for (std::size_t i = 2; i + 2 < out.size(); ++i) out[i] = '*';
  return out;
}

inline bool lookup_global_config_value(const std::string& section,
                                       const std::string& key,
                                       std::string* out) {
  if (!out) return false;
  const auto sec_it = cuwacunu::iitepi::config_space_t::config.find(section);
  if (sec_it == cuwacunu::iitepi::config_space_t::config.end()) return false;
  const auto key_it = sec_it->second.find(key);
  if (key_it == sec_it->second.end()) return false;
  *out = key_it->second;
  return true;
}

inline bool lookup_contract_config_value(const std::string& section,
                                         const std::string& key,
                                         const cuwacunu::iitepi::contract_hash_t& contract_hash,
                                         std::string* out) {
  if (!out) return false;
  const auto contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
  const auto sec_it = contract_itself->config.find(section);
  if (sec_it == contract_itself->config.end()) return false;
  const auto key_it = sec_it->second.find(key);
  if (key_it == sec_it->second.end()) return false;
  *out = key_it->second;
  return true;
}

inline bool lookup_config_value(const std::string& section,
                                const std::string& key,
                                const cuwacunu::iitepi::contract_hash_t& contract_hash,
                                std::string* out) {
  return lookup_global_config_value(section, key, out) ||
         lookup_contract_config_value(section, key, contract_hash, out);
}

inline std::string mark_selected_line(std::string line) {
  const std::size_t first_non_space = line.find_first_not_of(' ');
  if (first_non_space != std::string::npos && line[first_non_space] == '>') return line;
  if (!line.empty() && line.front() == ' ') {
    line.front() = '>';
    return line;
  }
  if (!line.empty() && line.front() == '>') return line;
  if (line.empty()) return ">";
  line.insert(line.begin(), '>');
  return line;
}

template <class T>
inline std::shared_ptr<T> as(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& obj) {
  if (!obj) return nullptr;
  return std::dynamic_pointer_cast<T>(obj->data);
}

inline std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> panel_content_target(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box) {
  auto target = cuwacunu::iinuji::panel_body_object(box);
  if (box && target && target != box) {
    target->style.label_color = box->style.label_color;
    target->style.background_color = box->style.background_color;
    target->style.border_color = box->style.border_color;
    target->style.bold = box->style.bold;
    target->style.inverse = box->style.inverse;
  }
  return target;
}

inline void set_text_box(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box,
                         std::string text,
                         bool wrap) {
  auto target = panel_content_target(box);
  if (!target) return;
  auto tb = as<cuwacunu::iinuji::textBox_data_t>(target);
  if (!tb) {
    target->data =
        std::make_shared<cuwacunu::iinuji::textBox_data_t>("", wrap, text_align_t::Left);
    tb = as<cuwacunu::iinuji::textBox_data_t>(target);
    if (!tb) return;
  }
  tb->content = std::move(text);
  tb->wrap = wrap;
  tb->clear_styled_lines();
}

inline void set_text_box_styled_lines(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box,
    const std::vector<cuwacunu::iinuji::styled_text_line_t>& lines,
    bool wrap) {
  auto target = panel_content_target(box);
  if (!target) return;
  auto tb = as<cuwacunu::iinuji::textBox_data_t>(target);
  if (!tb) {
    target->data =
        std::make_shared<cuwacunu::iinuji::textBox_data_t>("", wrap, text_align_t::Left);
    tb = as<cuwacunu::iinuji::textBox_data_t>(target);
    if (!tb) return;
  }
  std::ostringstream oss;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i > 0) oss << '\n';
    oss << cuwacunu::iinuji::styled_text_line_text(lines[i]);
  }
  tb->content = oss.str();
  tb->styled_lines = lines;
  tb->wrap = wrap;
}

inline void set_editor_box(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box,
    const std::shared_ptr<cuwacunu::iinuji::editorBox_data_t>& editor) {
  auto target = panel_content_target(box);
  if (!target || !editor) return;
  target->data = editor;
  target->focusable = box->focusable;
  target->focused = box->focused;
}

inline void set_plot_box(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box,
    const std::vector<std::vector<std::pair<double, double>>>& series,
    const std::vector<cuwacunu::iinuji::plot_series_cfg_t>& cfg,
    const cuwacunu::iinuji::plotbox_opts_t& opts) {
  auto target = panel_content_target(box);
  if (!target) return;
  auto pb = as<cuwacunu::iinuji::plotBox_data_t>(target);
  if (!pb) {
    target->data = std::make_shared<cuwacunu::iinuji::plotBox_data_t>();
    pb = as<cuwacunu::iinuji::plotBox_data_t>(target);
    if (!pb) return;
  }
  pb->series = series;
  pb->series_cfg = cfg;
  pb->opts = opts;
}

inline void append_log(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& log_box,
                       std::string text,
                       std::string label = {},
                       std::string color = {}) {
  auto bb = as<cuwacunu::iinuji::bufferBox_data_t>(log_box);
  if (!bb) return;
  bb->push_line(std::move(text), std::move(label), std::move(color));
}

using cmd_json_value_t = cuwacunu::piaabo::JsonValue;
using cmd_json_type_t = cuwacunu::piaabo::JsonValueType;

inline const cmd_json_value_t* cmd_json_field(const cmd_json_value_t* object,
                                              std::string_view key) {
  if (object == nullptr || object->type != cmd_json_type_t::OBJECT ||
      !object->objectValue) {
    return nullptr;
  }
  const auto it = object->objectValue->find(std::string(key));
  if (it == object->objectValue->end()) return nullptr;
  return &it->second;
}

inline std::string cmd_json_string(const cmd_json_value_t* value,
                                   std::string fallback = {}) {
  if (value == nullptr) return fallback;
  if (value->type == cmd_json_type_t::STRING) return value->stringValue;
  return fallback;
}

inline bool cmd_json_bool(const cmd_json_value_t* value, bool fallback = false) {
  if (value == nullptr) return fallback;
  if (value->type == cmd_json_type_t::BOOLEAN) return value->boolValue;
  return fallback;
}

inline std::uint64_t cmd_json_u64(const cmd_json_value_t* value,
                                  std::uint64_t fallback = 0) {
  if (value == nullptr) return fallback;
  if (value->type == cmd_json_type_t::NUMBER && value->numberValue >= 0.0) {
    return static_cast<std::uint64_t>(value->numberValue);
  }
  return fallback;
}

inline std::optional<std::uint64_t> cmd_json_optional_u64(
    const cmd_json_value_t* value) {
  if (value == nullptr || value->type == cmd_json_type_t::NULL_TYPE) {
    return std::nullopt;
  }
  if (value->type == cmd_json_type_t::NUMBER && value->numberValue >= 0.0) {
    return static_cast<std::uint64_t>(value->numberValue);
  }
  return std::nullopt;
}

inline std::optional<std::int64_t> cmd_json_optional_i64(
    const cmd_json_value_t* value) {
  if (value == nullptr || value->type == cmd_json_type_t::NULL_TYPE) {
    return std::nullopt;
  }
  if (value->type == cmd_json_type_t::NUMBER) {
    return static_cast<std::int64_t>(value->numberValue);
  }
  return std::nullopt;
}

inline std::vector<std::string> cmd_json_string_array(
    const cmd_json_value_t* value) {
  std::vector<std::string> out{};
  if (value == nullptr || value->type != cmd_json_type_t::ARRAY ||
      !value->arrayValue) {
    return out;
  }
  out.reserve(value->arrayValue->size());
  for (const auto& entry : *value->arrayValue) {
    if (entry.type == cmd_json_type_t::STRING) out.push_back(entry.stringValue);
  }
  return out;
}

inline bool cmd_parse_tool_structured_content(std::string_view tool_result_json,
                                              cmd_json_value_t* out_structured,
                                              std::string* error) {
  if (error) error->clear();
  if (out_structured == nullptr) {
    if (error) *error = "structured content output pointer is null";
    return false;
  }
  *out_structured = cmd_json_value_t{};
  try {
    const cmd_json_value_t root =
        cuwacunu::piaabo::JsonParser(std::string(tool_result_json)).parse();
    if (cmd_json_bool(cmd_json_field(&root, "isError"), false)) {
      const auto* structured = cmd_json_field(&root, "structuredContent");
      std::string message{};
      if (structured != nullptr) {
        message = cmd_json_string(cmd_json_field(structured, "error"));
        if (message.empty()) {
          message = cmd_json_string(cmd_json_field(structured, "message"));
        }
      }
      if (message.empty()) {
        const auto* content = cmd_json_field(&root, "content");
        if (content != nullptr && content->type == cmd_json_type_t::ARRAY &&
            content->arrayValue && !content->arrayValue->empty()) {
          message =
              cmd_json_string(cmd_json_field(&content->arrayValue->front(), "text"));
        }
      }
      if (error) *error = message.empty() ? "tool returned error" : message;
      return false;
    }
    const auto* structured = cmd_json_field(&root, "structuredContent");
    if (structured == nullptr) {
      if (error) *error = "tool result missing structuredContent";
      return false;
    }
    *out_structured = *structured;
    return true;
  } catch (const std::exception& ex) {
    if (error) *error = ex.what();
    return false;
  }
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
