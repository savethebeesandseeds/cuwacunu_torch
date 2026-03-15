#pragma once

#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"
#include "camahjucunu/dsl/wave_contract_binding/wave_contract_binding.h"
#include "hero/runtime_hero/runtime_job.h"
#include "iitepi/config_space_t.h"

namespace cuwacunu {
namespace hero {
namespace wave_contract_binding_runtime {

inline constexpr std::string_view kBindingSnapshotCampaignFilename = "campaign.dsl";
inline constexpr std::string_view kBindingSnapshotContractFilename =
    "binding.contract.dsl";
inline constexpr std::string_view kBindingSnapshotWaveFilename = "binding.wave.dsl";
inline constexpr std::string_view kBindingSnapshotInstructionsDirname =
    "instructions";

struct resolved_wave_contract_binding_snapshot_t {
  std::string binding_id{};
  std::string source_scope_dsl_path{};
  std::string source_contract_dsl_path{};
  std::string source_wave_dsl_path{};
  std::string campaign_dsl_path{};
  std::string contract_dsl_path{};
  std::string wave_dsl_path{};
  std::string original_contract_ref{};
  std::string snapshot_contract_ref{};
  std::string wave_ref{};
  std::vector<cuwacunu::camahjucunu::wave_contract_binding_variable_t>
      variables{};
};

namespace detail {

inline constexpr std::string_view kDefaultCampaignGrammarPath =
    "/cuwacunu/src/config/bnf/iitepi.campaign.bnf";
inline constexpr std::string_view kDefaultVicregConfigFilename =
    "default.tsi.wikimyei.representation.vicreg.dsl";
inline constexpr std::string_view kDefaultValueEstimationConfigFilename =
    "default.tsi.wikimyei.inference.mdn.value_estimation.dsl";
inline constexpr std::string_view kDefaultTransferMatrixConfigFilename =
    "default.tsi.wikimyei.inference.transfer_matrix_evaluation.dsl";

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

[[nodiscard]] inline std::string configured_campaign_grammar_path() {
  try {
    const std::string configured =
        cuwacunu::iitepi::config_space_t::get<std::string>(
            "BNF", "iitepi_campaign_grammar_filename",
            std::string(kDefaultCampaignGrammarPath));
    const std::string trimmed = trim_ascii(configured);
    if (!trimmed.empty()) {
      const std::filesystem::path configured_path(trimmed);
      if (configured_path.is_absolute()) {
        return configured_path.lexically_normal().string();
      }
      const std::filesystem::path base_folder =
          cuwacunu::iitepi::config_space_t::config_folder.empty()
              ? std::filesystem::path(DEFAULT_CONFIG_FOLDER)
              : std::filesystem::path(cuwacunu::iitepi::config_space_t::config_folder);
      return (base_folder / configured_path).lexically_normal().string();
    }
  } catch (const std::exception&) {
  }
  return std::string(kDefaultCampaignGrammarPath);
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

[[nodiscard]] inline bool ends_with_ascii(std::string_view value,
                                          std::string_view suffix) {
  return value.size() >= suffix.size() &&
         value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

struct dsl_path_assignment_t {
  std::size_t replace_begin{0};
  std::size_t replace_end{0};
  bool quoted{false};
  char quote_char{'"'};
  std::string raw_value{};
};

struct staged_dsl_graph_state_t {
  std::map<std::string, std::filesystem::path, std::less<>> source_to_snapshot{};
  std::map<std::string, std::string, std::less<>> snapshot_to_source{};
  std::vector<std::string> active_sources{};
};

[[nodiscard]] inline std::string normalize_path_key(
    const std::filesystem::path& path) {
  return path.lexically_normal().string();
}

[[nodiscard]] inline bool looks_like_dsl_path_value(std::string_view value) {
  const std::string trimmed = trim_ascii(value);
  return ends_with_ascii(trimmed, ".dsl");
}

[[nodiscard]] inline bool is_active_source(
    const staged_dsl_graph_state_t& state, std::string_view source_key) {
  for (const auto& active : state.active_sources) {
    if (active == source_key) return true;
  }
  return false;
}

[[nodiscard]] inline bool find_dsl_path_assignment(
    std::string_view line, bool in_block_comment, dsl_path_assignment_t* out) {
  if (!out) return false;
  *out = {};
  if (in_block_comment) return false;

  bool in_single_quote = false;
  bool in_double_quote = false;
  std::size_t assign_pos = std::string::npos;
  std::size_t colon_pos = std::string::npos;
  for (std::size_t i = 0; i < line.size();) {
    const char c = line[i];
    const char next = (i + 1 < line.size()) ? line[i + 1] : '\0';
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
      if (c == '/' && next == '/') break;
      if (c == '/' && next == '*') break;
      if (c == '#') break;
      if (c == '=') {
        assign_pos = i;
        break;
      }
      if (c == ':' && colon_pos == std::string::npos) {
        colon_pos = i;
      }
    }
    ++i;
  }
  if (assign_pos == std::string::npos) assign_pos = colon_pos;
  if (assign_pos == std::string::npos) return false;

  std::size_t value_begin = assign_pos + 1;
  while (value_begin < line.size() &&
         std::isspace(static_cast<unsigned char>(line[value_begin])) != 0) {
    ++value_begin;
  }
  if (value_begin >= line.size()) return false;

  if (line[value_begin] == '"' || line[value_begin] == '\'') {
    const char quote = line[value_begin];
    std::size_t value_end = value_begin + 1;
    while (value_end < line.size() && line[value_end] != quote) ++value_end;
    if (value_end >= line.size()) return false;
    out->replace_begin = value_begin;
    out->replace_end = value_end + 1;
    out->quoted = true;
    out->quote_char = quote;
    out->raw_value = std::string(line.substr(value_begin + 1,
                                             value_end - value_begin - 1));
    return looks_like_dsl_path_value(out->raw_value);
  }

  std::size_t value_end = line.size();
  in_single_quote = false;
  in_double_quote = false;
  for (std::size_t i = value_begin; i < line.size();) {
    const char c = line[i];
    const char next = (i + 1 < line.size()) ? line[i + 1] : '\0';
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
      if (c == ';' || c == '#') {
        value_end = i;
        break;
      }
      if (c == '/' && next == '/') {
        value_end = i;
        break;
      }
      if (c == '/' && next == '*') {
        value_end = i;
        break;
      }
    }
    ++i;
  }
  while (value_end > value_begin &&
         std::isspace(static_cast<unsigned char>(line[value_end - 1])) != 0) {
    --value_end;
  }
  if (value_end <= value_begin) return false;
  out->replace_begin = value_begin;
  out->replace_end = value_end;
  out->quoted = false;
  out->raw_value = std::string(line.substr(value_begin, value_end - value_begin));
  return looks_like_dsl_path_value(out->raw_value);
}

[[nodiscard]] inline std::filesystem::path make_unique_snapshot_path(
    const std::filesystem::path& output_dir,
    const std::filesystem::path& preferred_filename, std::string_view source_key,
    staged_dsl_graph_state_t* state) {
  if (state == nullptr) return output_dir / preferred_filename.filename();
  const std::string normalized_source = std::string(source_key);
  if (const auto it = state->source_to_snapshot.find(normalized_source);
      it != state->source_to_snapshot.end()) {
    return it->second;
  }

  std::filesystem::path filename =
      preferred_filename.filename().empty() ? std::filesystem::path("unnamed.dsl")
                                            : preferred_filename.filename();
  const std::string original_name = filename.filename().string();
  if (original_name.rfind("default.", 0) == 0 && original_name.size() > 8) {
    filename = std::filesystem::path(original_name.substr(8));
  }
  std::filesystem::path candidate = (output_dir / filename).lexically_normal();
  const std::string candidate_key = candidate.string();
  if (const auto it = state->snapshot_to_source.find(candidate_key);
      it == state->snapshot_to_source.end() || it->second == normalized_source) {
    state->source_to_snapshot[normalized_source] = candidate;
    state->snapshot_to_source[candidate_key] = normalized_source;
    return candidate;
  }

  const std::string stem = filename.stem().string();
  const std::string extension = filename.extension().string();
  for (std::size_t i = 1; i != 10000; ++i) {
    const std::filesystem::path numbered =
        output_dir / (stem + "." + std::to_string(i) + extension);
    const std::string numbered_key = numbered.lexically_normal().string();
    const auto it = state->snapshot_to_source.find(numbered_key);
    if (it == state->snapshot_to_source.end() || it->second == normalized_source) {
      const auto normalized = numbered.lexically_normal();
      state->source_to_snapshot[normalized_source] = normalized;
      state->snapshot_to_source[normalized.string()] = normalized_source;
      return normalized;
    }
  }
  return (output_dir / filename).lexically_normal();
}

[[nodiscard]] inline bool is_contract_like_source(
    const std::filesystem::path& source_path, std::string_view resolved_text) {
  const std::string filename = source_path.filename().string();
  if (filename.find(".contract.dsl") != std::string::npos) return true;
  return resolved_text.find("-----BEGIN IITEPI CONTRACT-----") !=
         std::string_view::npos;
}

[[nodiscard]] inline std::vector<std::filesystem::path>
implicit_contract_module_sources(const std::filesystem::path& source_contract_path) {
  const std::filesystem::path folder = source_contract_path.parent_path();
  return {
      folder / std::string(kDefaultVicregConfigFilename),
      folder / std::string(kDefaultValueEstimationConfigFilename),
      folder / std::string(kDefaultTransferMatrixConfigFilename),
  };
}

[[nodiscard]] inline bool stage_dsl_graph_file(
    const std::filesystem::path& source_path,
    const cuwacunu::camahjucunu::wave_contract_binding_decl_t& bind,
    const std::filesystem::path& target_path, const std::filesystem::path& output_dir,
    staged_dsl_graph_state_t* state, std::string* error);

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

[[nodiscard]] inline std::string build_snapshot_campaign_text(
    const cuwacunu::camahjucunu::wave_contract_binding_decl_t& bind,
    std::string_view snapshot_contract_ref,
    std::string_view snapshot_contract_filename,
    std::string_view snapshot_wave_filename) {
  std::ostringstream out;
  out << "CAMPAIGN {\n";
  out << "  IMPORT_CONTRACT_FILE \"" << snapshot_contract_filename << "\";\n\n";
  out << "  IMPORT_WAVE_FILE \"" << snapshot_wave_filename << "\";\n\n";
  out << "  BIND " << bind.id << " {\n";
  for (const auto& variable : bind.variables) {
    out << "    " << variable.name << " = " << variable.value << ";\n";
  }
  out << "    CONTRACT = " << snapshot_contract_ref << ";\n";
  out << "    WAVE = " << bind.wave_ref << ";\n";
  out << "  };\n";
  out << "\n";
  out << "  RUN " << bind.id << ";\n";
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

[[nodiscard]] inline bool find_matching_wave_import_path(
    const std::vector<cuwacunu::camahjucunu::iitepi_campaign_wave_decl_t>& waves,
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

[[nodiscard]] inline bool stage_dsl_graph_file(
    const std::filesystem::path& source_path,
    const cuwacunu::camahjucunu::wave_contract_binding_decl_t& bind,
    const std::filesystem::path& target_path, const std::filesystem::path& output_dir,
    staged_dsl_graph_state_t* state, std::string* error) {
  if (error) error->clear();
  if (!state) {
    if (error) *error = "missing staged DSL graph state";
    return false;
  }
  const std::filesystem::path normalized_source = source_path.lexically_normal();
  const std::filesystem::path normalized_target = target_path.lexically_normal();
  const std::string source_key = normalize_path_key(normalized_source);
  const std::string target_key = normalize_path_key(normalized_target);

  if (const auto it = state->source_to_snapshot.find(source_key);
      it != state->source_to_snapshot.end() && it->second != normalized_target) {
    if (error) {
      *error = "conflicting snapshot targets for DSL source: " + source_key;
    }
    return false;
  }
  if (const auto it = state->snapshot_to_source.find(target_key);
      it != state->snapshot_to_source.end() && it->second != source_key) {
    if (error) {
      *error = "conflicting staged DSL destination: " + normalized_target.string();
    }
    return false;
  }

  state->source_to_snapshot[source_key] = normalized_target;
  state->snapshot_to_source[target_key] = source_key;
  if (std::filesystem::exists(normalized_target) &&
      !is_active_source(*state, source_key)) {
    return true;
  }
  if (is_active_source(*state, source_key)) return true;

  state->active_sources.push_back(source_key);
  const auto cleanup_active = [&]() {
    if (!state->active_sources.empty() && state->active_sources.back() == source_key) {
      state->active_sources.pop_back();
      return;
    }
    for (auto it = state->active_sources.begin(); it != state->active_sources.end();
         ++it) {
      if (*it == source_key) {
        state->active_sources.erase(it);
        break;
      }
    }
  };

  std::string raw_text{};
  if (!load_text_file(normalized_source, &raw_text, error)) {
    cleanup_active();
    return false;
  }

  std::string resolved_text{};
  if (!cuwacunu::camahjucunu::resolve_wave_contract_binding_variables_in_text(
          raw_text, bind, &resolved_text, error)) {
    cleanup_active();
    return false;
  }

  std::ostringstream rebuilt{};
  std::size_t pos = 0;
  bool first = true;
  bool in_block_comment = false;
  while (pos <= resolved_text.size()) {
    const std::size_t end = resolved_text.find('\n', pos);
    std::string line = (end == std::string::npos)
                           ? resolved_text.substr(pos)
                           : resolved_text.substr(pos, end - pos);

    dsl_path_assignment_t path_assignment{};
    if (find_dsl_path_assignment(line, in_block_comment, &path_assignment)) {
      const std::filesystem::path child_source(
          resolve_path_from_folder(normalized_source.parent_path(),
                                   path_assignment.raw_value));
      std::error_code exists_ec{};
      if (!std::filesystem::exists(child_source, exists_ec) ||
          !std::filesystem::is_regular_file(child_source, exists_ec)) {
        cleanup_active();
        if (error) {
          *error = "staged DSL graph references missing DSL file: " +
                   child_source.string();
        }
        return false;
      }

      const std::filesystem::path child_target = make_unique_snapshot_path(
          output_dir, child_source.filename(), normalize_path_key(child_source), state);
      if (!stage_dsl_graph_file(child_source, bind, child_target, output_dir, state,
                                error)) {
        cleanup_active();
        return false;
      }

      std::filesystem::path relative_child =
          child_target.lexically_relative(normalized_target.parent_path());
      if (relative_child.empty()) relative_child = child_target.filename();
      const std::string replacement = path_assignment.quoted
                                          ? std::string(1, path_assignment.quote_char) +
                                                relative_child.string() +
                                                std::string(1, path_assignment.quote_char)
                                          : relative_child.string();
      line = line.substr(0, path_assignment.replace_begin) + replacement +
             line.substr(path_assignment.replace_end);
    }

    if (!first) rebuilt << "\n";
    first = false;
    rebuilt << line;
    in_block_comment = next_block_comment_state(line, in_block_comment);
    if (end == std::string::npos) break;
    pos = end + 1;
  }

  if (is_contract_like_source(normalized_source, resolved_text)) {
    for (const auto& implicit_source : implicit_contract_module_sources(normalized_source)) {
      std::error_code exists_ec{};
      if (!std::filesystem::exists(implicit_source, exists_ec) ||
          !std::filesystem::is_regular_file(implicit_source, exists_ec)) {
        continue;
      }
      const std::filesystem::path implicit_target = make_unique_snapshot_path(
          output_dir, implicit_source.filename(),
          normalize_path_key(implicit_source), state);
      if (!stage_dsl_graph_file(implicit_source, bind, implicit_target, output_dir,
                                state, error)) {
        cleanup_active();
        return false;
      }
    }
  }

  std::error_code mkdir_ec{};
  std::filesystem::create_directories(normalized_target.parent_path(), mkdir_ec);
  if (mkdir_ec) {
    cleanup_active();
    if (error) {
      *error = "cannot create staged DSL directory: " +
               normalized_target.parent_path().string();
    }
    return false;
  }
  if (!store_text_file(normalized_target, rebuilt.str(), error)) {
    cleanup_active();
    return false;
  }
  cleanup_active();
  return true;
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

  const std::filesystem::path campaign_path =
      output_dir / std::string(kBindingSnapshotInstructionsDirname) /
      std::string(kBindingSnapshotCampaignFilename);
  const std::filesystem::path contract_path =
      output_dir / std::string(kBindingSnapshotInstructionsDirname) /
      std::string(kBindingSnapshotContractFilename);
  const std::filesystem::path wave_path =
      output_dir / std::string(kBindingSnapshotInstructionsDirname) /
      std::string(kBindingSnapshotWaveFilename);
  const std::filesystem::path instructions_dir = campaign_path.parent_path();
  const std::string snapshot_contract_ref =
      derive_contract_id_from_snapshot_filename(contract_path.filename());
  const std::string campaign_text = build_snapshot_campaign_text(
      bind, snapshot_contract_ref, contract_path.filename().string(),
      wave_path.filename().string());

  staged_dsl_graph_state_t staged_graph{};
  if (!stage_dsl_graph_file(source_contract_path, bind, contract_path, instructions_dir,
                            &staged_graph, error)) {
    return false;
  }
  if (!stage_dsl_graph_file(source_wave_path, bind, wave_path, instructions_dir,
                            &staged_graph, error)) {
    return false;
  }
  if (!store_text_file(campaign_path, campaign_text, error)) return false;

  out_snapshot->binding_id = bind.id;
  out_snapshot->source_scope_dsl_path = source_scope_path.string();
  out_snapshot->source_contract_dsl_path = source_contract_path.string();
  out_snapshot->source_wave_dsl_path = source_wave_path.string();
  out_snapshot->campaign_dsl_path = campaign_path.string();
  out_snapshot->contract_dsl_path = contract_path.string();
  out_snapshot->wave_dsl_path = wave_path.string();
  out_snapshot->original_contract_ref = bind.contract_ref;
  out_snapshot->snapshot_contract_ref = snapshot_contract_ref;
  out_snapshot->wave_ref = bind.wave_ref;
  out_snapshot->variables = bind.variables;
  return true;
}

}  // namespace detail

[[nodiscard]] inline bool prepare_campaign_binding_snapshot(
    const std::filesystem::path& source_campaign_dsl_path,
    std::string_view requested_binding_id, const std::filesystem::path& output_dir,
    resolved_wave_contract_binding_snapshot_t* out_snapshot,
    std::string* error = nullptr) {
  if (error) error->clear();
  std::string grammar_text{};
  if (!detail::load_text_file(detail::configured_campaign_grammar_path(),
                              &grammar_text, error)) {
    return false;
  }
  std::string instruction_text{};
  if (!detail::load_text_file(source_campaign_dsl_path, &instruction_text,
                              error)) {
    return false;
  }

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  try {
    instruction =
        cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
            grammar_text, instruction_text);
  } catch (const std::exception& e) {
    if (error) {
      *error = "cannot decode iitepi campaign DSL '" +
               source_campaign_dsl_path.string() + "': " + e.what();
    }
    return false;
  }

  const std::string binding_id = detail::trim_ascii(requested_binding_id);
  if (binding_id.empty()) {
    if (error) {
      *error = "campaign binding snapshot requires an explicit binding id";
    }
    return false;
  }
  const auto* bind = detail::find_bind_decl_by_id(instruction.binds, binding_id);
  if (!bind) {
    if (error) *error = "unknown campaign binding id: " + binding_id;
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
