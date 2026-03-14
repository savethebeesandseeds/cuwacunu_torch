#pragma once

#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "camahjucunu/dsl/jkimyei_campaign/jkimyei_campaign.h"
#include "camahjucunu/dsl/tsiemene_board/tsiemene_board.h"
#include "camahjucunu/dsl/wave_contract_binding/wave_contract_binding.h"
#include "hero/runtime_hero/runtime_job.h"

namespace cuwacunu {
namespace hero {
namespace wave_contract_binding_runtime {

inline constexpr std::string_view kBindingSnapshotBoardFilename = "board.dsl";
inline constexpr std::string_view kBindingSnapshotContractFilename =
    "binding.contract.dsl";
inline constexpr std::string_view kBindingSnapshotWaveFilename = "binding.wave.dsl";

struct resolved_wave_contract_binding_snapshot_t {
  std::string binding_id{};
  std::string source_scope_dsl_path{};
  std::string source_contract_dsl_path{};
  std::string source_wave_dsl_path{};
  std::string board_dsl_path{};
  std::string contract_dsl_path{};
  std::string wave_dsl_path{};
  std::string original_contract_ref{};
  std::string snapshot_contract_ref{};
  std::string wave_ref{};
  std::vector<cuwacunu::camahjucunu::wave_contract_binding_variable_t>
      variables{};
};

namespace detail {

inline constexpr std::string_view kDefaultBoardGrammarPath =
    "/cuwacunu/src/config/bnf/iitepi.board.bnf";
inline constexpr std::string_view kDefaultJkimyeiCampaignGrammarPath =
    "/cuwacunu/src/config/bnf/jkimyei_campaign.bnf";

[[nodiscard]] inline std::string trim_ascii(std::string_view in) {
  return cuwacunu::hero::runtime::trim_ascii(in);
}

[[nodiscard]] inline std::string resolve_path_from_folder(
    const std::filesystem::path& folder, std::string_view raw_path) {
  const std::string cleaned = trim_ascii(raw_path);
  if (cleaned.empty()) return {};
  const std::filesystem::path p(cleaned);
  if (p.is_absolute()) return p.lexically_normal().string();
  return (folder / p).lexically_normal().string();
}

[[nodiscard]] inline bool load_text_file(const std::filesystem::path& path,
                                         std::string* out,
                                         std::string* error) {
  return cuwacunu::hero::runtime::read_text_file(path, out, error);
}

[[nodiscard]] inline bool store_text_file(const std::filesystem::path& path,
                                          std::string_view content,
                                          std::string* error) {
  return cuwacunu::hero::runtime::write_text_file_atomic(path, content, error);
}

[[nodiscard]] inline bool is_ident_char(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-';
}

[[nodiscard]] inline bool token_boundary_before(std::string_view text,
                                                std::size_t pos) {
  if (pos == 0) return true;
  return !is_ident_char(text[pos - 1]);
}

[[nodiscard]] inline bool token_boundary_after(std::string_view text,
                                               std::size_t pos,
                                               std::size_t len) {
  const std::size_t end = pos + len;
  if (end >= text.size()) return true;
  return !is_ident_char(text[end]);
}

[[nodiscard]] inline bool next_block_comment_state(std::string_view line,
                                                   bool in_block_comment) {
  bool in_single_quote = false;
  bool in_double_quote = false;
  for (std::size_t i = 0; i < line.size();) {
    const char c = line[i];
    const char next = (i + 1 < line.size()) ? line[i + 1] : '\0';
    if (in_block_comment) {
      if (c == '*' && next == '/') {
        in_block_comment = false;
        i += 2;
      } else {
        ++i;
      }
      continue;
    }
    if (!in_double_quote && c == '\'') {
      in_single_quote = !in_single_quote;
      ++i;
      continue;
    }
    if (!in_single_quote && c == '"') {
      in_double_quote = !in_double_quote;
      ++i;
      continue;
    }
    if (!in_single_quote && !in_double_quote && c == '/' && next == '*') {
      in_block_comment = true;
      i += 2;
      continue;
    }
    ++i;
  }
  return in_block_comment;
}

[[nodiscard]] inline std::size_t find_token_outside_comments(
    std::string_view line, std::string_view token, bool in_block_comment) {
  bool in_single_quote = false;
  bool in_double_quote = false;
  for (std::size_t i = 0; i < line.size();) {
    const char c = line[i];
    const char next = (i + 1 < line.size()) ? line[i + 1] : '\0';
    if (in_block_comment) {
      if (c == '*' && next == '/') {
        in_block_comment = false;
        i += 2;
      } else {
        ++i;
      }
      continue;
    }
    if (!in_double_quote && c == '\'') {
      in_single_quote = !in_single_quote;
      ++i;
      continue;
    }
    if (!in_single_quote && c == '"') {
      in_double_quote = !in_double_quote;
      ++i;
      continue;
    }
    if (!in_single_quote && !in_double_quote) {
      if (c == '/' && next == '*') {
        in_block_comment = true;
        i += 2;
        continue;
      }
      if (c == '/' && next == '/') break;
      if (c == '#' || c == ';') break;
      if (i + token.size() <= line.size() &&
          line.compare(i, token.size(), token) == 0 &&
          token_boundary_before(line, i) &&
          token_boundary_after(line, i, token.size())) {
        return i;
      }
    }
    ++i;
  }
  return std::string::npos;
}

[[nodiscard]] inline std::string unquote_if_wrapped(std::string value,
                                                    bool* was_quoted = nullptr) {
  if (was_quoted) *was_quoted = false;
  value = trim_ascii(value);
  if (value.size() >= 2) {
    const char first = value.front();
    const char last = value.back();
    if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
      if (was_quoted) *was_quoted = true;
      return value.substr(1, value.size() - 2);
    }
  }
  return value;
}

[[nodiscard]] inline std::string rewrite_assignment_path_absolute(
    const std::string& line, std::string_view key,
    const std::filesystem::path& source_path, bool in_block_comment) {
  const std::size_t key_pos =
      find_token_outside_comments(line, key, in_block_comment);
  if (key_pos == std::string::npos) return line;

  std::size_t assign_pos = std::string::npos;
  for (std::size_t i = key_pos + key.size(); i < line.size(); ++i) {
    const char c = line[i];
    if (c == ':' || c == '=') {
      assign_pos = i;
      break;
    }
    if (c == '/' && i + 1 < line.size() &&
        (line[i + 1] == '/' || line[i + 1] == '*')) {
      return line;
    }
    if (c == '#' || c == ';') return line;
  }
  if (assign_pos == std::string::npos) return line;

  std::size_t value_begin = assign_pos + 1;
  while (value_begin < line.size() &&
         std::isspace(static_cast<unsigned char>(line[value_begin])) != 0) {
    ++value_begin;
  }
  std::size_t value_end = line.find(';', value_begin);
  if (value_end == std::string::npos) return line;
  const std::string raw_value = line.substr(value_begin, value_end - value_begin);
  bool was_quoted = false;
  const std::string unquoted = unquote_if_wrapped(raw_value, &was_quoted);
  if (unquoted.empty()) return line;

  const std::filesystem::path raw_path(unquoted);
  if (raw_path.is_absolute()) return line;
  const std::filesystem::path rewritten =
      (source_path.parent_path() / raw_path).lexically_normal();
  const std::string replacement =
      was_quoted ? ("\"" + rewritten.string() + "\"") : rewritten.string();
  return line.substr(0, value_begin) + replacement + line.substr(value_end);
}

[[nodiscard]] inline std::string rewrite_contract_paths_absolute(
    std::string_view text, const std::filesystem::path& source_contract_path) {
  std::ostringstream out;
  std::size_t pos = 0;
  bool first = true;
  bool in_block_comment = false;
  while (pos <= text.size()) {
    const std::size_t end = text.find('\n', pos);
    std::string line = (end == std::string::npos)
                           ? std::string(text.substr(pos))
                           : std::string(text.substr(pos, end - pos));
    line = rewrite_assignment_path_absolute(line, "CIRCUIT_FILE",
                                            source_contract_path,
                                            in_block_comment);
    if (!first) out << "\n";
    first = false;
    out << line;
    in_block_comment = next_block_comment_state(line, in_block_comment);
    if (end == std::string::npos) break;
    pos = end + 1;
  }
  return out.str();
}

[[nodiscard]] inline std::string rewrite_wave_paths_absolute(
    std::string_view text, const std::filesystem::path& source_wave_path) {
  std::ostringstream out;
  std::size_t pos = 0;
  bool first = true;
  bool in_block_comment = false;
  while (pos <= text.size()) {
    const std::size_t end = text.find('\n', pos);
    std::string line = (end == std::string::npos)
                           ? std::string(text.substr(pos))
                           : std::string(text.substr(pos, end - pos));
    line = rewrite_assignment_path_absolute(line, "SOURCES_DSL_FILE",
                                            source_wave_path, in_block_comment);
    line = rewrite_assignment_path_absolute(line, "CHANNELS_DSL_FILE",
                                            source_wave_path, in_block_comment);
    if (!first) out << "\n";
    first = false;
    out << line;
    in_block_comment = next_block_comment_state(line, in_block_comment);
    if (end == std::string::npos) break;
    pos = end + 1;
  }
  return out.str();
}

[[nodiscard]] inline std::string sanitize_identifier(std::string value) {
  for (char& ch : value) {
    const bool alpha_num = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                           (ch >= '0' && ch <= '9');
    if (!alpha_num) ch = '_';
  }
  std::string out;
  out.reserve(value.size());
  bool last_underscore = false;
  for (const char ch : value) {
    if (ch == '_') {
      if (last_underscore) continue;
      last_underscore = true;
      out.push_back(ch);
      continue;
    }
    last_underscore = false;
    out.push_back(ch);
  }
  while (!out.empty() && out.front() == '_') out.erase(out.begin());
  while (!out.empty() && out.back() == '_') out.pop_back();
  if (out.empty()) out = "unnamed";
  if (!out.empty() && out.front() >= '0' && out.front() <= '9') {
    out.insert(out.begin(), '_');
  }
  return out;
}

[[nodiscard]] inline std::string derive_contract_id_from_snapshot_filename(
    const std::filesystem::path& filename) {
  std::string file_name = filename.filename().string();
  const auto ends_with = [](std::string_view s, std::string_view suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
  };
  if (ends_with(file_name, ".contract.dsl")) {
    file_name.resize(file_name.size() - std::string(".contract.dsl").size());
  } else if (ends_with(file_name, ".dsl")) {
    file_name.resize(file_name.size() - std::string(".dsl").size());
  } else {
    file_name = filename.stem().string();
  }
  return "contract_" + sanitize_identifier(file_name);
}

[[nodiscard]] inline std::string build_snapshot_board_text(
    const cuwacunu::camahjucunu::wave_contract_binding_decl_t& bind,
    std::string_view snapshot_contract_ref,
    std::string_view snapshot_contract_filename,
    std::string_view snapshot_wave_filename) {
  std::ostringstream out;
  out << "ACTIVE_BIND " << bind.id << ";\n\n";
  out << "BOARD {\n";
  out << "  IMPORT_CONTRACT_FILE \"" << snapshot_contract_filename << "\";\n\n";
  out << "  IMPORT_WAVE_FILE \"" << snapshot_wave_filename << "\";\n\n";
  out << "  BIND " << bind.id << " {\n";
  for (const auto& variable : bind.variables) {
    out << "    " << variable.name << " = " << variable.value << ";\n";
  }
  out << "    CONTRACT = " << snapshot_contract_ref << ";\n";
  out << "    WAVE = " << bind.wave_ref << ";\n";
  out << "  };\n";
  out << "}\n";
  return out.str();
}

[[nodiscard]] inline bool wave_text_contains_wave_name(std::string_view text,
                                                       std::string_view wave_name) {
  bool in_block_comment = false;
  bool in_line_comment = false;
  bool in_single_quote = false;
  bool in_double_quote = false;
  for (std::size_t i = 0; i < text.size();) {
    const char c = text[i];
    const char next = (i + 1 < text.size()) ? text[i + 1] : '\0';
    if (in_line_comment) {
      if (c == '\n') in_line_comment = false;
      ++i;
      continue;
    }
    if (in_block_comment) {
      if (c == '*' && next == '/') {
        in_block_comment = false;
        i += 2;
      } else {
        ++i;
      }
      continue;
    }
    if (!in_double_quote && c == '\'') {
      in_single_quote = !in_single_quote;
      ++i;
      continue;
    }
    if (!in_single_quote && c == '"') {
      in_double_quote = !in_double_quote;
      ++i;
      continue;
    }
    if (!in_single_quote && !in_double_quote) {
      if (c == '/' && next == '/') {
        in_line_comment = true;
        i += 2;
        continue;
      }
      if (c == '/' && next == '*') {
        in_block_comment = true;
        i += 2;
        continue;
      }
      if (c == '#') {
        in_line_comment = true;
        ++i;
        continue;
      }
      if (i + 4 <= text.size() && text.compare(i, 4, "WAVE") == 0 &&
          token_boundary_before(text, i) && token_boundary_after(text, i, 4)) {
        std::size_t pos = i + 4;
        while (pos < text.size() &&
               std::isspace(static_cast<unsigned char>(text[pos])) != 0) {
          ++pos;
        }
        const std::size_t name_begin = pos;
        while (pos < text.size() && is_ident_char(text[pos])) ++pos;
        if (pos == name_begin) {
          i += 4;
          continue;
        }
        const std::string found_name = std::string(text.substr(name_begin, pos - name_begin));
        while (pos < text.size() &&
               std::isspace(static_cast<unsigned char>(text[pos])) != 0) {
          ++pos;
        }
        if (pos < text.size() && text[pos] == '{' && found_name == wave_name) {
          return true;
        }
      }
    }
    ++i;
  }
  return false;
}

[[nodiscard]] inline bool resolve_contract_text(
    const std::filesystem::path& source_contract_path,
    const cuwacunu::camahjucunu::wave_contract_binding_decl_t& bind,
    std::string* out_text, std::string* error) {
  if (error) error->clear();
  if (!out_text) {
    if (error) *error = "missing contract output text pointer";
    return false;
  }
  out_text->clear();
  std::string raw_text{};
  if (!load_text_file(source_contract_path, &raw_text, error)) return false;
  std::string resolved{};
  if (!cuwacunu::camahjucunu::resolve_wave_contract_binding_variables_in_text(
          raw_text, bind, &resolved, error)) {
    return false;
  }
  *out_text = rewrite_contract_paths_absolute(resolved, source_contract_path);
  return true;
}

[[nodiscard]] inline bool resolve_wave_text(
    const std::filesystem::path& source_wave_path,
    const cuwacunu::camahjucunu::wave_contract_binding_decl_t& bind,
    std::string* out_text, std::string* error) {
  if (error) error->clear();
  if (!out_text) {
    if (error) *error = "missing wave output text pointer";
    return false;
  }
  out_text->clear();
  std::string raw_text{};
  if (!load_text_file(source_wave_path, &raw_text, error)) return false;
  std::string resolved{};
  if (!cuwacunu::camahjucunu::resolve_wave_contract_binding_variables_in_text(
          raw_text, bind, &resolved, error)) {
    return false;
  }
  *out_text = rewrite_wave_paths_absolute(resolved, source_wave_path);
  return true;
}

[[nodiscard]] inline bool find_matching_wave_import_path(
    const std::vector<cuwacunu::camahjucunu::tsiemene_board_wave_decl_t>& waves,
    const std::filesystem::path& source_scope_path, std::string_view wave_ref,
    std::string* out_path, std::string* error) {
  if (error) error->clear();
  if (out_path) out_path->clear();
  std::optional<std::string> resolved_match{};
  for (const auto& wave_decl : waves) {
    const std::string candidate =
        resolve_path_from_folder(source_scope_path.parent_path(), wave_decl.file);
    std::string text{};
    if (!load_text_file(candidate, &text, error)) return false;
    if (!wave_text_contains_wave_name(text, wave_ref)) continue;
    if (resolved_match.has_value() && *resolved_match != candidate) {
      if (error) {
        *error = "binding wave id is ambiguous across imported wave DSL files: " +
                 std::string(wave_ref);
      }
      return false;
    }
    resolved_match = candidate;
  }
  if (!resolved_match.has_value()) {
    if (error) {
      *error = "binding references unknown WAVE id: " + std::string(wave_ref);
    }
    return false;
  }
  if (out_path) *out_path = *resolved_match;
  return true;
}

[[nodiscard]] inline bool find_matching_wave_import_path(
    const std::vector<cuwacunu::camahjucunu::jkimyei_campaign_wave_decl_t>& waves,
    const std::filesystem::path& source_scope_path, std::string_view wave_ref,
    std::string* out_path, std::string* error) {
  std::vector<cuwacunu::camahjucunu::tsiemene_board_wave_decl_t> compatible{};
  compatible.reserve(waves.size());
  for (const auto& wave : waves) compatible.push_back({wave.id, wave.file});
  return find_matching_wave_import_path(compatible, source_scope_path, wave_ref,
                                        out_path, error);
}

template <typename ContractDeclT>
[[nodiscard]] inline const ContractDeclT* find_contract_decl_by_id(
    const std::vector<ContractDeclT>& contracts, std::string_view contract_ref) {
  for (const auto& contract : contracts) {
    if (contract.id == contract_ref) return &contract;
  }
  return nullptr;
}

template <typename BindDeclT>
[[nodiscard]] inline const BindDeclT* find_bind_decl_by_id(
    const std::vector<BindDeclT>& binds, std::string_view binding_id) {
  for (const auto& bind : binds) {
    if (bind.id == binding_id) return &bind;
  }
  return nullptr;
}

[[nodiscard]] inline bool prepare_snapshot_from_resolved_paths(
    const std::filesystem::path& source_scope_path,
    const std::filesystem::path& source_contract_path,
    const std::filesystem::path& source_wave_path,
    const cuwacunu::camahjucunu::wave_contract_binding_decl_t& bind,
    const std::filesystem::path& output_dir,
    resolved_wave_contract_binding_snapshot_t* out_snapshot,
    std::string* error) {
  if (error) error->clear();
  if (!out_snapshot) {
    if (error) *error = "missing binding snapshot output pointer";
    return false;
  }
  *out_snapshot = {};

  std::string contract_text{};
  if (!resolve_contract_text(source_contract_path, bind, &contract_text, error)) {
    return false;
  }
  std::string wave_text{};
  if (!resolve_wave_text(source_wave_path, bind, &wave_text, error)) {
    return false;
  }

  const std::filesystem::path board_path =
      output_dir / std::string(kBindingSnapshotBoardFilename);
  const std::filesystem::path contract_path =
      output_dir / std::string(kBindingSnapshotContractFilename);
  const std::filesystem::path wave_path =
      output_dir / std::string(kBindingSnapshotWaveFilename);
  const std::string snapshot_contract_ref =
      derive_contract_id_from_snapshot_filename(contract_path.filename());
  const std::string board_text = build_snapshot_board_text(
      bind, snapshot_contract_ref, contract_path.filename().string(),
      wave_path.filename().string());

  if (!store_text_file(contract_path, contract_text, error)) return false;
  if (!store_text_file(wave_path, wave_text, error)) return false;
  if (!store_text_file(board_path, board_text, error)) return false;

  out_snapshot->binding_id = bind.id;
  out_snapshot->source_scope_dsl_path = source_scope_path.string();
  out_snapshot->source_contract_dsl_path = source_contract_path.string();
  out_snapshot->source_wave_dsl_path = source_wave_path.string();
  out_snapshot->board_dsl_path = board_path.string();
  out_snapshot->contract_dsl_path = contract_path.string();
  out_snapshot->wave_dsl_path = wave_path.string();
  out_snapshot->original_contract_ref = bind.contract_ref;
  out_snapshot->snapshot_contract_ref = snapshot_contract_ref;
  out_snapshot->wave_ref = bind.wave_ref;
  out_snapshot->variables = bind.variables;
  return true;
}

}  // namespace detail

[[nodiscard]] inline bool prepare_board_binding_snapshot(
    const std::filesystem::path& source_board_dsl_path,
    std::string_view requested_binding_id, const std::filesystem::path& output_dir,
    resolved_wave_contract_binding_snapshot_t* out_snapshot,
    std::string* error = nullptr) {
  if (error) error->clear();
  std::string grammar_text{};
  if (!detail::load_text_file(std::string(detail::kDefaultBoardGrammarPath),
                              &grammar_text, error)) {
    return false;
  }
  std::string instruction_text{};
  if (!detail::load_text_file(source_board_dsl_path, &instruction_text, error)) {
    return false;
  }

  cuwacunu::camahjucunu::tsiemene_board_instruction_t instruction{};
  try {
    instruction =
        cuwacunu::camahjucunu::dsl::decode_tsiemene_board_from_dsl(
            grammar_text, instruction_text);
  } catch (const std::exception& e) {
    if (error) {
      *error = "cannot decode board DSL '" + source_board_dsl_path.string() +
               "': " + e.what();
    }
    return false;
  }

  const std::string binding_id = [&]() {
    const std::string cleaned = detail::trim_ascii(requested_binding_id);
    return cleaned.empty() ? instruction.active_bind_id : cleaned;
  }();
  const auto* bind = detail::find_bind_decl_by_id(instruction.binds, binding_id);
  if (!bind) {
    if (error) *error = "unknown board binding id: " + binding_id;
    return false;
  }
  const auto* contract_decl =
      detail::find_contract_decl_by_id(instruction.contracts, bind->contract_ref);
  if (!contract_decl) {
    if (error) {
      *error = "binding references unknown CONTRACT id: " + bind->contract_ref;
    }
    return false;
  }

  const std::filesystem::path base_folder = source_board_dsl_path.parent_path();
  const std::filesystem::path source_contract_path =
      detail::resolve_path_from_folder(base_folder, contract_decl->file);
  std::string source_wave_path{};
  if (!detail::find_matching_wave_import_path(instruction.waves, source_board_dsl_path,
                                              bind->wave_ref, &source_wave_path,
                                              error)) {
    return false;
  }

  return detail::prepare_snapshot_from_resolved_paths(
      source_board_dsl_path, source_contract_path, source_wave_path, *bind,
      output_dir, out_snapshot, error);
}

[[nodiscard]] inline bool prepare_campaign_binding_snapshot(
    const std::filesystem::path& source_campaign_dsl_path,
    std::string_view requested_binding_id, const std::filesystem::path& output_dir,
    resolved_wave_contract_binding_snapshot_t* out_snapshot,
    std::string* error = nullptr) {
  if (error) error->clear();
  std::string grammar_text{};
  if (!detail::load_text_file(std::string(detail::kDefaultJkimyeiCampaignGrammarPath),
                              &grammar_text, error)) {
    return false;
  }
  std::string instruction_text{};
  if (!detail::load_text_file(source_campaign_dsl_path, &instruction_text,
                              error)) {
    return false;
  }

  cuwacunu::camahjucunu::jkimyei_campaign_instruction_t instruction{};
  try {
    instruction =
        cuwacunu::camahjucunu::dsl::decode_jkimyei_campaign_from_dsl(
            grammar_text, instruction_text);
  } catch (const std::exception& e) {
    if (error) {
      *error = "cannot decode jkimyei campaign DSL '" +
               source_campaign_dsl_path.string() + "': " + e.what();
    }
    return false;
  }

  const std::string binding_id = detail::trim_ascii(requested_binding_id);
  if (binding_id.empty()) {
    if (error) {
      *error =
          "jkimyei campaign binding snapshot requires an explicit binding id";
    }
    return false;
  }
  const auto* bind = detail::find_bind_decl_by_id(instruction.binds, binding_id);
  if (!bind) {
    if (error) *error = "unknown jkimyei campaign binding id: " + binding_id;
    return false;
  }
  const auto* contract_decl =
      detail::find_contract_decl_by_id(instruction.contracts, bind->contract_ref);
  if (!contract_decl) {
    if (error) {
      *error = "binding references unknown CONTRACT id: " + bind->contract_ref;
    }
    return false;
  }

  const std::filesystem::path base_folder = source_campaign_dsl_path.parent_path();
  const std::filesystem::path source_contract_path =
      detail::resolve_path_from_folder(base_folder, contract_decl->file);
  std::string source_wave_path{};
  if (!detail::find_matching_wave_import_path(
          instruction.waves, source_campaign_dsl_path, bind->wave_ref,
          &source_wave_path, error)) {
    return false;
  }

  return detail::prepare_snapshot_from_resolved_paths(
      source_campaign_dsl_path, source_contract_path, source_wave_path, *bind,
      output_dir, out_snapshot, error);
}

}  // namespace wave_contract_binding_runtime
}  // namespace hero
}  // namespace cuwacunu
