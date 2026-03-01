#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "hashimyei/hashimyei_identity.h"
#include "iinuji/primitives/editor.h"
#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_cmd/catalog.h"
#include "iinuji/iinuji_cmd/commands/iinuji.paths.h"
#include "iinuji/iinuji_cmd/state.h"
#include "camahjucunu/dsl/tsiemene_board/tsiemene_board.h"
#include "piaabo/dconfig.h"
#include "piaabo/dlogs.h"
#include "tsiemene/tsi.directive.registry.h"

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

inline std::string resolve_configured_board_contract_path() {
  const std::string configured_board = cuwacunu::iitepi::config_space_t::get<std::string>(
      "GENERAL", GENERAL_BOARD_CONFIG_KEY);
  const std::filesystem::path board_path(configured_board);
  const std::string resolved_board_path = board_path.is_absolute()
      ? board_path.string()
      : (std::filesystem::path(cuwacunu::iitepi::config_space_t::config_folder) /
         board_path)
            .string();
  const std::string binding_id = cuwacunu::iitepi::config_space_t::get<std::string>(
      "GENERAL", GENERAL_BOARD_BINDING_KEY);
  const auto board_hash =
      cuwacunu::iitepi::board_space_t::register_board_file(resolved_board_path);
  const auto board_itself =
      cuwacunu::iitepi::board_space_t::board_itself(board_hash);
  const auto& board_instruction = board_itself->board.decoded();

  const cuwacunu::camahjucunu::tsiemene_board_bind_decl_t* bind = nullptr;
  for (const auto& b : board_instruction.binds) {
    if (b.id == binding_id) {
      bind = &b;
      break;
    }
  }
  if (!bind) return {};

  const cuwacunu::camahjucunu::tsiemene_board_contract_decl_t* contract_decl = nullptr;
  for (const auto& c : board_instruction.contracts) {
    if (c.id == bind->contract_ref) {
      contract_decl = &c;
      break;
    }
  }
  if (!contract_decl) return {};

  std::filesystem::path p(contract_decl->file);
  if (!p.is_absolute()) {
    p = std::filesystem::path(board_itself->config_folder) / p;
  }
  return p.lexically_normal().string();
}

inline cuwacunu::iitepi::contract_hash_t
resolve_configured_board_contract_hash() {
  const auto path = resolve_configured_board_contract_path();
  const auto hash =
      cuwacunu::iitepi::contract_space_t::register_contract_file(path);
  cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(hash);
  return hash;
}

inline const std::vector<std::string>& training_hashimyei_catalog() {
  return cuwacunu::hashimyei::known_hashimyeis();
}

inline std::size_t clamp_training_hash_index(std::size_t idx) {
  const std::size_t n = training_hashimyei_catalog().size();
  if (n == 0) return 0;
  return (idx < n) ? idx : 0;
}

inline std::string dir_token(tsiemene::DirectiveDir d) {
  return (d == tsiemene::DirectiveDir::In) ? "in" : "out";
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

inline void set_text_box(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box,
                         std::string text,
                         bool wrap) {
  if (!box) return;
  auto tb = as<cuwacunu::iinuji::textBox_data_t>(box);
  if (!tb) {
    box->data = std::make_shared<cuwacunu::iinuji::textBox_data_t>("", wrap, text_align_t::Left);
    tb = as<cuwacunu::iinuji::textBox_data_t>(box);
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
  if (!box) return;
  auto tb = as<cuwacunu::iinuji::textBox_data_t>(box);
  if (!tb) {
    box->data = std::make_shared<cuwacunu::iinuji::textBox_data_t>("", wrap, text_align_t::Left);
    tb = as<cuwacunu::iinuji::textBox_data_t>(box);
    if (!tb) return;
  }
  std::ostringstream oss;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i > 0) oss << '\n';
    oss << lines[i].text;
  }
  tb->content = oss.str();
  tb->styled_lines = lines;
  tb->wrap = wrap;
}

inline void set_editor_box(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box,
    const std::shared_ptr<cuwacunu::iinuji::editorBox_data_t>& editor) {
  if (!box || !editor) return;
  box->data = editor;
}

inline void append_log(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& log_box,
                       std::string text,
                       std::string label = {},
                       std::string color = {}) {
  auto bb = as<cuwacunu::iinuji::bufferBox_data_t>(log_box);
  if (!bb) return;
  bb->push_line(std::move(text), std::move(label), std::move(color));
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
