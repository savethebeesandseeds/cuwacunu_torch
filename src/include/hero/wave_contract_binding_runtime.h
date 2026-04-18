#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"
#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "camahjucunu/dsl/wave_contract_binding/wave_contract_binding.h"
#include "hero/hashimyei_hero/hashimyei_catalog.h"
#include "hero/hashimyei_hero/hashimyei_identity.h"
#include "hero/hashimyei_hero/hashimyei_report_fragments.h"
#include "hero/runtime_hero/runtime_job.h"
#include "iitepi/contract_space_t.h"
#include "iitepi/config_space_t.h"

namespace cuwacunu {
namespace hero {
namespace wave_contract_binding_runtime {

inline constexpr std::string_view kBindingSnapshotCampaignFilename =
    "campaign.dsl";
inline constexpr std::string_view kBindingSnapshotContractFilename =
    "binding.contract.dsl";
inline constexpr std::string_view kBindingSnapshotWaveFilename =
    "binding.wave.dsl";
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

struct resolved_mount_selection_explanation_t {
  std::string wave_binding_id{};
  std::string component_kind{};
  std::string family{};
  std::string selector_kind{};
  std::string selector_value{};
  bool resolved{false};
  bool catalog_checked{false};
  bool catalog_manifest_found{false};
  std::string resolved_hashimyei{};
  std::string resolved_canonical_path{};
  std::string summary{};
  std::string details{};
};

struct resolved_binding_selection_explanation_t {
  std::string source_campaign_dsl_path{};
  std::string source_contract_dsl_path{};
  std::string source_wave_dsl_path{};
  std::string binding_id{};
  std::string contract_ref{};
  std::string wave_ref{};
  std::string contract_hash{};
  std::string dock_hash{};
  bool resolved{false};
  std::string summary{};
  std::string details{};
  std::vector<resolved_mount_selection_explanation_t> mounts{};
};

namespace detail {

[[nodiscard]] inline std::string trim_ascii(std::string_view in) {
  return cuwacunu::hero::runtime::trim_ascii(in);
}

[[nodiscard]] inline std::string
resolve_path_from_folder(const std::filesystem::path &folder,
                         std::string_view raw_path) {
  const std::string cleaned = trim_ascii(raw_path);
  if (cleaned.empty())
    return {};
  const std::filesystem::path p(cleaned);
  if (p.is_absolute())
    return p.lexically_normal().string();
  return (folder / p).lexically_normal().string();
}

[[nodiscard]] inline bool load_text_file(const std::filesystem::path &path,
                                         std::string *out, std::string *error) {
  return cuwacunu::hero::runtime::read_text_file(path, out, error);
}

[[nodiscard]] inline std::string configured_campaign_grammar_path() {
  try {
    const std::string configured =
        cuwacunu::iitepi::config_space_t::get<std::string>(
            "BNF", "iitepi_campaign_grammar_filename", std::string{});
    const std::string trimmed = trim_ascii(configured);
    if (!trimmed.empty()) {
      const std::filesystem::path configured_path(trimmed);
      if (configured_path.is_absolute()) {
        return configured_path.lexically_normal().string();
      }
      const std::filesystem::path base_folder =
          std::filesystem::path(
              cuwacunu::iitepi::config_space_t::config_file_path)
              .parent_path();
      return (base_folder / configured_path).lexically_normal().string();
    }
  } catch (const std::exception &) {
  }
  return {};
}

[[nodiscard]] inline std::string configured_wave_grammar_path() {
  try {
    const std::string configured =
        cuwacunu::iitepi::config_space_t::get<std::string>(
            "BNF", "iitepi_wave_grammar_filename", std::string{});
    const std::string trimmed = trim_ascii(configured);
    if (!trimmed.empty()) {
      const std::filesystem::path configured_path(trimmed);
      if (configured_path.is_absolute()) {
        return configured_path.lexically_normal().string();
      }
      const std::filesystem::path base_folder =
          std::filesystem::path(
              cuwacunu::iitepi::config_space_t::config_file_path)
              .parent_path();
      return (base_folder / configured_path).lexically_normal().string();
    }
  } catch (const std::exception &) {
  }
  return {};
}

[[nodiscard]] inline bool store_text_file(const std::filesystem::path &path,
                                          std::string_view content,
                                          std::string *error) {
  return cuwacunu::hero::runtime::write_text_file_atomic(path, content, error);
}

[[nodiscard]] inline bool is_ident_char(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-';
}

[[nodiscard]] inline bool token_boundary_before(std::string_view text,
                                                std::size_t pos) {
  if (pos == 0)
    return true;
  return !is_ident_char(text[pos - 1]);
}

[[nodiscard]] inline bool
token_boundary_after(std::string_view text, std::size_t pos, std::size_t len) {
  const std::size_t end = pos + len;
  if (end >= text.size())
    return true;
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

[[nodiscard]] inline std::size_t
find_token_outside_comments(std::string_view line, std::string_view token,
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
    if (!in_single_quote && !in_double_quote) {
      if (c == '/' && next == '*') {
        in_block_comment = true;
        i += 2;
        continue;
      }
      if (c == '/' && next == '/')
        break;
      if (c == '#' || c == ';')
        break;
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

[[nodiscard]] inline std::string
unquote_if_wrapped(std::string value, bool *was_quoted = nullptr) {
  if (was_quoted)
    *was_quoted = false;
  value = trim_ascii(value);
  if (value.size() >= 2) {
    const char first = value.front();
    const char last = value.back();
    if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
      if (was_quoted)
        *was_quoted = true;
      return value.substr(1, value.size() - 2);
    }
  }
  return value;
}

[[nodiscard]] inline bool ends_with_ascii(std::string_view value,
                                          std::string_view suffix) {
  return value.size() >= suffix.size() &&
         value.compare(value.size() - suffix.size(), suffix.size(), suffix) ==
             0;
}

struct dsl_path_assignment_t {
  std::size_t replace_begin{0};
  std::size_t replace_end{0};
  bool quoted{false};
  char quote_char{'"'};
  std::string raw_value{};
};

struct staged_dsl_graph_state_t {
  std::map<std::string, std::filesystem::path, std::less<>>
      source_to_snapshot{};
  std::map<std::string, std::string, std::less<>> snapshot_to_source{};
  std::vector<std::string> active_sources{};
};

[[nodiscard]] inline std::string
normalize_path_key(const std::filesystem::path &path) {
  return path.lexically_normal().string();
}

[[nodiscard]] inline bool looks_like_dsl_path_value(std::string_view value) {
  const std::string trimmed = trim_ascii(value);
  return ends_with_ascii(trimmed, ".dsl");
}

[[nodiscard]] inline bool
is_active_source(const staged_dsl_graph_state_t &state,
                 std::string_view source_key) {
  for (const auto &active : state.active_sources) {
    if (active == source_key)
      return true;
  }
  return false;
}

[[nodiscard]] inline bool find_dsl_path_assignment(std::string_view line,
                                                   bool in_block_comment,
                                                   dsl_path_assignment_t *out) {
  if (!out)
    return false;
  *out = {};
  if (in_block_comment)
    return false;

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
      if (c == '/' && next == '/')
        break;
      if (c == '/' && next == '*')
        break;
      if (c == '#')
        break;
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
  if (assign_pos == std::string::npos)
    assign_pos = colon_pos;
  if (assign_pos == std::string::npos)
    return false;

  std::size_t value_begin = assign_pos + 1;
  while (value_begin < line.size() &&
         std::isspace(static_cast<unsigned char>(line[value_begin])) != 0) {
    ++value_begin;
  }
  if (value_begin >= line.size())
    return false;

  if (line[value_begin] == '"' || line[value_begin] == '\'') {
    const char quote = line[value_begin];
    std::size_t value_end = value_begin + 1;
    while (value_end < line.size() && line[value_end] != quote)
      ++value_end;
    if (value_end >= line.size())
      return false;
    out->replace_begin = value_begin;
    out->replace_end = value_end + 1;
    out->quoted = true;
    out->quote_char = quote;
    out->raw_value =
        std::string(line.substr(value_begin + 1, value_end - value_begin - 1));
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
  if (value_end <= value_begin)
    return false;
  out->replace_begin = value_begin;
  out->replace_end = value_end;
  out->quoted = false;
  out->raw_value =
      std::string(line.substr(value_begin, value_end - value_begin));
  return looks_like_dsl_path_value(out->raw_value);
}

[[nodiscard]] inline std::filesystem::path
make_unique_snapshot_path(const std::filesystem::path &output_dir,
                          const std::filesystem::path &preferred_filename,
                          std::string_view source_key,
                          staged_dsl_graph_state_t *state) {
  if (state == nullptr)
    return output_dir / preferred_filename.filename();
  const std::string normalized_source = std::string(source_key);
  if (const auto it = state->source_to_snapshot.find(normalized_source);
      it != state->source_to_snapshot.end()) {
    return it->second;
  }

  std::filesystem::path filename = preferred_filename.filename().empty()
                                       ? std::filesystem::path("unnamed.dsl")
                                       : preferred_filename.filename();
  std::filesystem::path candidate = (output_dir / filename).lexically_normal();
  const std::string candidate_key = candidate.string();
  if (const auto it = state->snapshot_to_source.find(candidate_key);
      it == state->snapshot_to_source.end() ||
      it->second == normalized_source) {
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
    if (it == state->snapshot_to_source.end() ||
        it->second == normalized_source) {
      const auto normalized = numbered.lexically_normal();
      state->source_to_snapshot[normalized_source] = normalized;
      state->snapshot_to_source[normalized.string()] = normalized_source;
      return normalized;
    }
  }
  return (output_dir / filename).lexically_normal();
}

[[nodiscard]] inline std::string
strip_contract_line_comments(std::string_view line, bool *in_block_comment) {
  bool block_comment =
      (in_block_comment != nullptr) ? *in_block_comment : false;
  bool in_single_quote = false;
  bool in_double_quote = false;
  std::string out;
  out.reserve(line.size());

  for (std::size_t i = 0; i < line.size();) {
    const char c = line[i];
    const char next = (i + 1 < line.size()) ? line[i + 1] : '\0';
    if (block_comment) {
      if (c == '*' && next == '/') {
        block_comment = false;
        i += 2;
      } else {
        ++i;
      }
      continue;
    }
    if (!in_single_quote && !in_double_quote && c == '/' && next == '*') {
      block_comment = true;
      i += 2;
      continue;
    }
    if (!in_single_quote && !in_double_quote && c == '/' && next == '/') {
      break;
    }
    if (!in_single_quote && !in_double_quote && c == '#') {
      break;
    }
    if (!in_double_quote && c == '\'') {
      in_single_quote = !in_single_quote;
      out.push_back(c);
      ++i;
      continue;
    }
    if (!in_single_quote && c == '"') {
      in_double_quote = !in_double_quote;
      out.push_back(c);
      ++i;
      continue;
    }
    out.push_back(c);
    ++i;
  }

  if (in_block_comment != nullptr) {
    *in_block_comment = block_comment;
  }
  return trim_ascii(out);
}

[[nodiscard]] inline bool load_contract_dsl_variables(
    const std::filesystem::path &contract_path,
    std::vector<cuwacunu::camahjucunu::dsl_variable_t> *out_variables,
    std::string *error) {
  if (error)
    error->clear();
  if (!out_variables) {
    if (error)
      *error = "missing contract variable output";
    return false;
  }
  out_variables->clear();

  std::string raw_text{};
  if (!load_text_file(contract_path, &raw_text, error))
    return false;

  bool begin_seen = false;
  bool end_seen = false;
  bool dock_seen = false;
  bool assembly_seen = false;
  bool circuit_seen = false;
  enum class contract_section_e : std::uint8_t {
    None = 0,
    Dock,
    Assembly,
  };
  contract_section_e section = contract_section_e::None;
  std::istringstream input(raw_text);
  std::string raw_line{};
  bool in_block_comment = false;
  while (std::getline(input, raw_line)) {
    std::string line =
        strip_contract_line_comments(raw_line, &in_block_comment);
    if (line.empty())
      continue;
    if (!begin_seen) {
      if (line == "-----BEGIN IITEPI CONTRACT-----") {
        begin_seen = true;
        continue;
      }
      if (error) {
        *error = "expected -----BEGIN IITEPI CONTRACT----- while loading "
                 "contract variables";
      }
      return false;
    }
    if (line == "-----END IITEPI CONTRACT-----") {
      if (section != contract_section_e::None) {
        if (error) {
          *error = "missing closing '}' before -----END IITEPI CONTRACT-----";
        }
        return false;
      }
      end_seen = true;
      break;
    }
    if (section == contract_section_e::None) {
      if (line == "DOCK {") {
        if (dock_seen) {
          if (error)
            *error = "duplicate DOCK section while loading contract variables";
          return false;
        }
        if (assembly_seen) {
          if (error) {
            *error =
                "DOCK section must appear before ASSEMBLY in contract wrapper";
          }
          return false;
        }
        dock_seen = true;
        section = contract_section_e::Dock;
        continue;
      }
      if (line == "ASSEMBLY {") {
        if (!dock_seen) {
          if (error) {
            *error = "ASSEMBLY section requires a preceding DOCK in contract "
                     "wrapper";
          }
          return false;
        }
        if (assembly_seen) {
          if (error) {
            *error =
                "duplicate ASSEMBLY section while loading contract variables";
          }
          return false;
        }
        assembly_seen = true;
        section = contract_section_e::Assembly;
        continue;
      }
      if (error) {
        *error = "expected DOCK { or ASSEMBLY { while loading contract "
                 "variables";
      }
      return false;
    }
    if (line == "}" || line == "};") {
      section = contract_section_e::None;
      continue;
    }

    if (line.back() != ';') {
      if (error) {
        *error = "expected ';' at statement end while loading contract "
                 "variables";
      }
      return false;
    }
    line.pop_back();
    line = trim_ascii(line);
    if (line.empty()) {
      if (error)
        *error = "empty contract statement while loading contract variables";
      return false;
    }

    const std::size_t eq = line.find('=');
    if (eq != std::string::npos) {
      const std::string variable_name = trim_ascii(line.substr(0, eq));
      if (cuwacunu::camahjucunu::is_dsl_variable_name(variable_name)) {
        const std::string variable_value =
            unquote_if_wrapped(trim_ascii(line.substr(eq + 1)));
        if (!cuwacunu::camahjucunu::append_dsl_variable(
                out_variables, variable_name, variable_value, error)) {
          return false;
        }
        continue;
      }
      if (section == contract_section_e::Dock) {
        if (error) {
          *error = "DOCK only accepts __variable assignments while loading "
                   "contract variables";
        }
        return false;
      }
    } else if (section == contract_section_e::Dock) {
      if (error) {
        *error = "DOCK only accepts __variable assignments while loading "
                 "contract variables";
      }
      return false;
    }

    const std::size_t colon = line.find(':');
    if (colon == std::string::npos) {
      if (error) {
        *error = "unknown ASSEMBLY statement while loading contract variables";
      }
      return false;
    }
    const std::string head = trim_ascii(line.substr(0, colon));
    const std::string value = trim_ascii(line.substr(colon + 1));
    if (head == "CIRCUIT_FILE") {
      if (circuit_seen) {
        if (error) {
          *error = "duplicate CIRCUIT_FILE statement while loading contract "
                   "variables";
        }
        return false;
      }
      if (value.empty()) {
        if (error) {
          *error = "empty CIRCUIT_FILE path while loading contract variables";
        }
        return false;
      }
      circuit_seen = true;
      continue;
    }
    if (head == "AKNOWLEDGE") {
      if (value.find('=') == std::string::npos) {
        if (error) {
          *error = "AKNOWLEDGE requires alias = family while loading contract "
                   "variables";
        }
        return false;
      }
      continue;
    }
    if (error) {
      *error = "unknown ASSEMBLY statement while loading contract variables";
    }
    return false;
  }
  if (!begin_seen) {
    if (error) {
      *error = "missing -----BEGIN IITEPI CONTRACT----- while loading contract "
               "variables";
    }
    return false;
  }
  if (!end_seen) {
    if (error) {
      *error = "missing -----END IITEPI CONTRACT----- while loading contract "
               "variables";
    }
    return false;
  }
  if (!dock_seen) {
    if (error)
      *error = "missing DOCK section while loading contract variables";
    return false;
  }
  if (!assembly_seen) {
    if (error)
      *error = "missing ASSEMBLY section while loading contract variables";
    return false;
  }
  if (!circuit_seen) {
    if (error) {
      *error =
          "missing CIRCUIT_FILE statement in ASSEMBLY while loading contract "
          "variables";
    }
    return false;
  }
  return true;
}

[[nodiscard]] inline bool
validate_bind_variables_do_not_shadow_contract_variables(
    const std::vector<cuwacunu::camahjucunu::dsl_variable_t>
        &contract_variables,
    const std::vector<cuwacunu::camahjucunu::dsl_variable_t> &bind_variables,
    std::string *error) {
  if (error)
    error->clear();
  if (contract_variables.empty() || bind_variables.empty())
    return true;

  std::unordered_set<std::string> contract_names{};
  contract_names.reserve(contract_variables.size());
  for (const auto &variable : contract_variables) {
    const std::string name = trim_ascii(variable.name);
    if (!name.empty())
      contract_names.insert(name);
  }

  for (const auto &variable : bind_variables) {
    const std::string name = trim_ascii(variable.name);
    if (name.empty())
      continue;
    if (contract_names.find(name) == contract_names.end())
      continue;
    if (error) {
      *error =
          "bind variable '" + name +
          "' conflicts with a contract DOCK/ASSEMBLY variable; bind-local "
          "variables are wave-scoped only and may not shadow contract-owned "
          "compatibility or realization variables";
    }
    return false;
  }
  return true;
}

[[nodiscard]] inline bool stage_dsl_graph_file(
    const std::filesystem::path &source_path,
    const std::vector<cuwacunu::camahjucunu::dsl_variable_t> &variables,
    const std::filesystem::path &target_path,
    const std::filesystem::path &output_dir, staged_dsl_graph_state_t *state,
    std::string *error);

[[nodiscard]] inline std::string sanitize_identifier(std::string value) {
  for (char &ch : value) {
    const bool alpha_num = (ch >= 'a' && ch <= 'z') ||
                           (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9');
    if (!alpha_num)
      ch = '_';
  }
  std::string out;
  out.reserve(value.size());
  bool last_underscore = false;
  for (const char ch : value) {
    if (ch == '_') {
      if (last_underscore)
        continue;
      last_underscore = true;
      out.push_back(ch);
      continue;
    }
    last_underscore = false;
    out.push_back(ch);
  }
  while (!out.empty() && out.front() == '_')
    out.erase(out.begin());
  while (!out.empty() && out.back() == '_')
    out.pop_back();
  if (out.empty())
    out = "unnamed";
  if (!out.empty() && out.front() >= '0' && out.front() <= '9') {
    out.insert(out.begin(), '_');
  }
  return out;
}

[[nodiscard]] inline std::string derive_contract_id_from_snapshot_filename(
    const std::filesystem::path &filename) {
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
    const cuwacunu::camahjucunu::iitepi_campaign_bind_decl_t &bind,
    std::string_view snapshot_contract_ref,
    std::string_view snapshot_contract_filename,
    std::string_view snapshot_wave_filename) {
  std::ostringstream out;
  out << "CAMPAIGN {\n";
  out << "  IMPORT_CONTRACT \"" << snapshot_contract_filename << "\" AS "
      << snapshot_contract_ref << ";\n\n";
  out << "  FROM \"" << snapshot_wave_filename << "\" IMPORT_WAVE "
      << bind.wave_ref << ";\n\n";
  out << "  BIND " << bind.id << " {\n";
  for (const auto &variable : bind.variables) {
    out << "    " << variable.name << " = " << variable.value << ";\n";
  }
  if (!bind.mounts.empty()) {
    out << "    MOUNT {\n";
    for (const auto &mount : bind.mounts) {
      out << "      " << mount.wave_binding_id << " = "
          << (mount.selector_kind ==
                      cuwacunu::camahjucunu::
                          iitepi_campaign_mount_selector_kind_e::Exact
                  ? "EXACT"
                  : "RANK")
          << " ";
      if (mount.selector_kind ==
          cuwacunu::camahjucunu::iitepi_campaign_mount_selector_kind_e::Exact) {
        out << mount.exact_hashimyei;
      } else {
        out << mount.rank;
      }
      out << ";\n";
    }
    out << "    };\n";
  }
  out << "    CONTRACT = " << snapshot_contract_ref << ";\n";
  out << "    WAVE = " << bind.wave_ref << ";\n";
  out << "  };\n";
  out << "\n";
  out << "  RUN " << bind.id << ";\n";
  out << "}\n";
  return out.str();
}

[[nodiscard]] inline std::string
render_wave_component_identity_line(std::string_view path,
                                    std::string_view family) {
  const std::string trimmed_path = trim_ascii(path);
  const std::string trimmed_family = trim_ascii(family);
  if (!trimmed_path.empty() && trimmed_path != trimmed_family) {
    return "PATH: " + trimmed_path + ";";
  }
  if (!trimmed_family.empty()) {
    return "FAMILY: " + trimmed_family + ";";
  }
  if (!trimmed_path.empty()) {
    return "PATH: " + trimmed_path + ";";
  }
  return {};
}

[[nodiscard]] inline const cuwacunu::camahjucunu::iitepi_campaign_mount_decl_t *
find_mount_decl(const cuwacunu::camahjucunu::iitepi_campaign_bind_decl_t &bind,
                std::string_view wave_binding_id) {
  for (const auto &mount : bind.mounts) {
    if (trim_ascii(mount.wave_binding_id) == trim_ascii(wave_binding_id)) {
      return &mount;
    }
  }
  return nullptr;
}

[[nodiscard]] inline std::string mount_component_canonical_path(
    std::string_view family, std::string_view hashimyei) {
  const std::string family_token = trim_ascii(family);
  const std::string hash_token = trim_ascii(hashimyei);
  if (family_token.empty())
    return hash_token;
  if (hash_token.empty())
    return family_token;
  return family_token + "." + hash_token;
}

[[nodiscard]] inline std::string mount_selector_kind_name(
    cuwacunu::camahjucunu::iitepi_campaign_mount_selector_kind_e kind) {
  switch (kind) {
    case cuwacunu::camahjucunu::iitepi_campaign_mount_selector_kind_e::Exact:
      return "EXACT";
    case cuwacunu::camahjucunu::iitepi_campaign_mount_selector_kind_e::Rank:
      return "RANK";
  }
  return "UNKNOWN";
}

[[nodiscard]] inline bool open_mount_hashimyei_catalog(
    cuwacunu::hero::hashimyei::hashimyei_catalog_store_t *catalog,
    std::string *error) {
  if (error)
    error->clear();
  if (!catalog) {
    if (error)
      *error = "missing hashimyei catalog";
    return false;
  }
  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t::options_t options{};
  options.catalog_path = cuwacunu::hashimyei::catalog_db_path();
  options.encrypted = false;
  if (!catalog->open(options, error)) {
    return false;
  }
  std::string ingest_error{};
  if (!catalog->ingest_filesystem(cuwacunu::hashimyei::store_root(),
                                  &ingest_error)) {
    if (error)
      *error = "cannot ingest hashimyei store: " + ingest_error;
    return false;
  }
  return true;
}

[[nodiscard]] inline bool resolve_mount_exact_hashimyei(
    const cuwacunu::camahjucunu::iitepi_campaign_mount_decl_t &mount,
    std::string_view family, std::string_view expected_dock_hash,
    cuwacunu::hero::hashimyei::hashimyei_catalog_store_t *catalog,
    std::string *out_hashimyei, std::string *error) {
  if (error)
    error->clear();
  if (!out_hashimyei) {
    if (error)
      *error = "missing exact mount output";
    return false;
  }
  out_hashimyei->clear();
  std::string normalized_hash{};
  if (!cuwacunu::hashimyei::normalize_hex_hash_name(mount.exact_hashimyei,
                                                    &normalized_hash)) {
    if (error) {
      *error = "MOUNT EXACT requires canonical 0x<hex> hashimyei: " +
               mount.exact_hashimyei;
    }
    return false;
  }
  if (catalog && catalog->opened()) {
    cuwacunu::hero::hashimyei::component_state_t component{};
    std::string resolve_error{};
    const std::string canonical_path =
        mount_component_canonical_path(family, normalized_hash);
    if (catalog->resolve_component(canonical_path, normalized_hash, &component,
                                   &resolve_error)) {
      const std::string actual_family = trim_ascii(component.manifest.family);
      if (!trim_ascii(family).empty() && !actual_family.empty() &&
          actual_family != trim_ascii(family)) {
        if (error) {
          *error = "MOUNT EXACT " + normalized_hash + " targets family '" +
                   actual_family + "', expected '" + trim_ascii(family) + "'";
        }
        return false;
      }
      const std::string actual_dock =
          trim_ascii(component.manifest.docking_signature_sha256_hex);
      if (!trim_ascii(expected_dock_hash).empty() && !actual_dock.empty() &&
          actual_dock != trim_ascii(expected_dock_hash)) {
        if (error) {
          *error = "MOUNT EXACT " + normalized_hash +
                   " is not dock-compatible with the selected contract";
        }
        return false;
      }
    }
  }
  *out_hashimyei = normalized_hash;
  return true;
}

[[nodiscard]] inline bool resolve_mount_rank_hashimyei(
    const cuwacunu::camahjucunu::iitepi_campaign_mount_decl_t &mount,
    std::string_view family, std::string_view expected_dock_hash,
    cuwacunu::hero::hashimyei::hashimyei_catalog_store_t *catalog,
    std::string *out_hashimyei, std::string *error) {
  if (error)
    error->clear();
  if (!catalog) {
    if (error)
      *error = "missing hashimyei catalog for RANK mount";
    return false;
  }
  if (!out_hashimyei) {
    if (error)
      *error = "missing rank mount output";
    return false;
  }
  out_hashimyei->clear();
  const std::string family_token = trim_ascii(family);
  const std::string dock_hash = trim_ascii(expected_dock_hash);
  if (family_token.empty() || dock_hash.empty()) {
    if (error) {
      *error = "RANK mount requires non-empty family and dock_hash";
    }
    return false;
  }
  if (!catalog->resolve_ranked_hashimyei(family_token, dock_hash, mount.rank,
                                         out_hashimyei, error)) {
    if (error && error->empty()) {
      *error = "cannot resolve dock-compatible rank " +
               std::to_string(mount.rank) + " for family " + family_token;
    }
    return false;
  }
  return true;
}

[[nodiscard]] inline std::string render_iitepi_wave_set_to_dsl(
    const cuwacunu::camahjucunu::iitepi_wave_set_t &wave_set) {
  using namespace cuwacunu::camahjucunu;
  std::ostringstream out;
  for (std::size_t wave_index = 0; wave_index < wave_set.waves.size();
       ++wave_index) {
    const auto &wave = wave_set.waves[wave_index];
    out << "WAVE " << wave.name << " {\n";
    if (!trim_ascii(wave.circuit_name).empty()) {
      out << "  CIRCUIT: " << trim_ascii(wave.circuit_name) << ";\n";
    }
    if (wave.mode_flags != 0) {
      out << "  MODE: " << canonical_iitepi_wave_mode(wave.mode_flags)
          << ";\n";
    } else {
      out << "  MODE: " << trim_ascii(wave.mode) << ";\n";
    }
    if (!trim_ascii(wave.determinism_policy).empty()) {
      out << "  DETERMINISM: " << trim_ascii(wave.determinism_policy)
          << ";\n";
    }
    out << "  SAMPLER: " << trim_ascii(wave.sampler) << ";\n";
    out << "  EPOCHS: " << wave.epochs << ";\n";
    out << "  BATCH_SIZE: " << wave.batch_size << ";\n";
    out << "  MAX_BATCHES_PER_EPOCH: " << wave.max_batches_per_epoch << ";\n";
    out << "\n";

    for (const auto &source : wave.sources) {
      out << "  SOURCE: <" << source.binding_id << "> {\n";
      const std::string identity =
          render_wave_component_identity_line(source.source_path, source.family);
      if (!identity.empty()) {
        out << "    " << identity << "\n";
      }
      out << "    SETTINGS: {\n";
      out << "      WORKERS: " << source.workers << ";\n";
      out << "      FORCE_REBUILD_CACHE: "
          << (source.force_rebuild_cache ? "true" : "false") << ";\n";
      out << "      RANGE_WARN_BATCHES: " << source.range_warn_batches
          << ";\n";
      out << "    };\n";
      out << "    RUNTIME: {\n";
      out << "      SYMBOL: " << trim_ascii(source.symbol) << ";\n";
      out << "      FROM: " << trim_ascii(source.from) << ";\n";
      out << "      TO: " << trim_ascii(source.to) << ";\n";
      out << "    };\n";
      out << "  };\n\n";
    }

    for (const auto &wikimyei : wave.wikimyeis) {
      out << "  WIKIMYEI: <" << wikimyei.binding_id << "> {\n";
      const std::string identity = render_wave_component_identity_line(
          wikimyei.wikimyei_path, wikimyei.family);
      if (!identity.empty()) {
        out << "    " << identity << "\n";
      }
      out << "    JKIMYEI: {\n";
      out << "      HALT_TRAIN: "
          << (wikimyei.halt_train ? "true" : "false") << ";\n";
      if (!trim_ascii(wikimyei.profile_id).empty()) {
        out << "      PROFILE_ID: " << trim_ascii(wikimyei.profile_id)
            << ";\n";
      }
      out << "    };\n";
      out << "  };\n\n";
    }

    for (const auto &probe : wave.probes) {
      out << "  PROBE: <" << probe.binding_id << "> {\n";
      const std::string identity =
          render_wave_component_identity_line(probe.probe_path, probe.family);
      if (!identity.empty()) {
        out << "    " << identity << "\n";
      }
      if (probe.has_runtime) {
        out << "    RUNTIME: {\n";
        out << "      TRAINING_WINDOW: incoming_batch;\n";
        out << "      REPORT_POLICY: epoch_end_log;\n";
        out << "      OBJECTIVE: future_target_dims_nll;\n";
        out << "    };\n";
      }
      out << "  };\n\n";
    }

    for (const auto &sink : wave.sinks) {
      out << "  SINK: <" << sink.binding_id << "> {\n";
      const std::string identity =
          render_wave_component_identity_line(sink.sink_path, sink.family);
      if (!identity.empty()) {
        out << "    " << identity << "\n";
      }
      out << "  };\n\n";
    }

    out << "}\n";
    if (wave_index + 1 != wave_set.waves.size()) {
      out << "\n";
    }
  }
  return out.str();
}

[[nodiscard]] inline bool apply_bind_mounts_to_wave_snapshot(
    const std::filesystem::path &staged_contract_path,
    const cuwacunu::camahjucunu::iitepi_campaign_bind_decl_t &bind,
    const std::filesystem::path &staged_wave_path, std::string *error) {
  if (error)
    error->clear();
  std::string grammar_text{};
  if (!load_text_file(configured_wave_grammar_path(), &grammar_text, error)) {
    return false;
  }
  std::string wave_text{};
  if (!load_text_file(staged_wave_path, &wave_text, error)) {
    return false;
  }

  cuwacunu::camahjucunu::iitepi_wave_set_t wave_set{};
  try {
    wave_set = cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(
        grammar_text, wave_text);
  } catch (const std::exception &e) {
    if (error) {
      *error = "cannot decode staged wave DSL '" + staged_wave_path.string() +
               "': " + e.what();
    }
    return false;
  }

  auto wave_it = std::find_if(wave_set.waves.begin(), wave_set.waves.end(),
                              [&](const auto &wave) {
                                return trim_ascii(wave.name) ==
                                       trim_ascii(bind.wave_ref);
                              });
  if (wave_it == wave_set.waves.end()) {
    if (error) {
      *error = "selected bind references unknown WAVE id in staged wave DSL: " +
               bind.wave_ref;
    }
    return false;
  }

  const auto contract_hash =
      cuwacunu::iitepi::contract_space_t::register_contract_file(
          staged_contract_path.string());
  const auto contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
  if (!contract_snapshot) {
    if (error) {
      *error = "cannot load staged contract snapshot for MOUNT resolution: " +
               staged_contract_path.string();
    }
    return false;
  }
  const std::string dock_hash = trim_ascii(
      contract_snapshot->signature.docking_signature_sha256_hex);
  if (dock_hash.empty()) {
    if (error) {
      *error = "staged contract snapshot is missing docking signature";
    }
    return false;
  }

  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t catalog{};
  bool catalog_open_attempted = false;
  std::string catalog_open_error{};
  const auto try_open_catalog = [&](bool required) -> bool {
    if (catalog.opened())
      return true;
    if (catalog_open_attempted) {
      if (required && error && !catalog_open_error.empty()) {
        *error = catalog_open_error;
      }
      return catalog.opened();
    }
    catalog_open_attempted = true;
    if (!open_mount_hashimyei_catalog(&catalog, &catalog_open_error)) {
      if (required && error) {
        *error = catalog_open_error;
      }
      return false;
    }
    catalog_open_error.clear();
    return true;
  };
  std::unordered_map<std::string, bool> used_mounts{};
  used_mounts.reserve(bind.mounts.size());
  for (const auto &mount : bind.mounts) {
    used_mounts.emplace(trim_ascii(mount.wave_binding_id), false);
  }

  auto apply_mount = [&](std::string_view component_kind,
                         std::string *component_path,
                         const std::string &binding_id,
                         const std::string &family) -> bool {
    const auto *mount = find_mount_decl(bind, binding_id);
    if (!mount) {
      if (error) {
        *error = "BIND '" + bind.id + "' missing required MOUNT for " +
                 std::string(component_kind) + " <" + binding_id + ">";
      }
      return false;
    }
    used_mounts[trim_ascii(binding_id)] = true;
    std::string resolved_hashimyei{};
    bool ok = false;
    if (mount->selector_kind ==
        cuwacunu::camahjucunu::iitepi_campaign_mount_selector_kind_e::Exact) {
      (void)try_open_catalog(false);
      ok = resolve_mount_exact_hashimyei(*mount, family, dock_hash, &catalog,
                                         &resolved_hashimyei, error);
    } else {
      if (!try_open_catalog(true)) {
        return false;
      }
      ok = resolve_mount_rank_hashimyei(*mount, family, dock_hash, &catalog,
                                        &resolved_hashimyei, error);
    }
    if (!ok) {
      if (error && !error->empty()) {
        *error = "BIND '" + bind.id + "' MOUNT <" + binding_id + ">: " +
                 *error;
      }
      return false;
    }
    *component_path = trim_ascii(family) + "." + resolved_hashimyei;
    return true;
  };

  for (auto &wikimyei : wave_it->wikimyeis) {
    if (!apply_mount("WIKIMYEI", &wikimyei.wikimyei_path, wikimyei.binding_id,
                     wikimyei.family)) {
      return false;
    }
  }
  for (auto &probe : wave_it->probes) {
    if (!apply_mount("PROBE", &probe.probe_path, probe.binding_id,
                     probe.family)) {
      return false;
    }
  }

  for (const auto &entry : used_mounts) {
    if (entry.second)
      continue;
    if (error) {
      *error = "BIND '" + bind.id + "' declares MOUNT for unknown wave binding id: " +
               entry.first;
    }
    return false;
  }

  return store_text_file(staged_wave_path, render_iitepi_wave_set_to_dsl(wave_set),
                         error);
}

[[nodiscard]] inline bool
wave_text_contains_wave_name(std::string_view text,
                             std::string_view wave_name) {
  bool in_block_comment = false;
  bool in_line_comment = false;
  bool in_single_quote = false;
  bool in_double_quote = false;
  for (std::size_t i = 0; i < text.size();) {
    const char c = text[i];
    const char next = (i + 1 < text.size()) ? text[i + 1] : '\0';
    if (in_line_comment) {
      if (c == '\n')
        in_line_comment = false;
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
        while (pos < text.size() && is_ident_char(text[pos]))
          ++pos;
        if (pos == name_begin) {
          i += 4;
          continue;
        }
        const std::string found_name =
            std::string(text.substr(name_begin, pos - name_begin));
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
    const std::vector<cuwacunu::camahjucunu::iitepi_campaign_wave_decl_t>
        &waves,
    const std::filesystem::path &source_scope_path, std::string_view wave_ref,
    std::string *out_path, std::string *error) {
  if (error)
    error->clear();
  if (out_path)
    out_path->clear();
  const auto wave_it =
      std::find_if(waves.begin(), waves.end(), [&](const auto &wave_decl) {
        return wave_decl.id == wave_ref;
      });
  if (wave_it == waves.end()) {
    if (error) {
      *error = "binding references unknown WAVE id: " + std::string(wave_ref);
    }
    return false;
  }
  const std::string candidate =
      resolve_path_from_folder(source_scope_path.parent_path(), wave_it->file);
  std::string text{};
  if (!load_text_file(candidate, &text, error))
    return false;
  if (!wave_text_contains_wave_name(text, wave_it->id)) {
    if (error) {
      *error = "imported WAVE id not found in declared wave DSL file: " +
               std::string(wave_it->id);
    }
    return false;
  }
  if (out_path)
    *out_path = candidate;
  return true;
}

template <typename ContractDeclT>
[[nodiscard]] inline const ContractDeclT *
find_contract_decl_by_id(const std::vector<ContractDeclT> &contracts,
                         std::string_view contract_ref) {
  for (const auto &contract : contracts) {
    if (contract.id == contract_ref)
      return &contract;
  }
  return nullptr;
}

template <typename BindDeclT>
[[nodiscard]] inline const BindDeclT *
find_bind_decl_by_id(const std::vector<BindDeclT> &binds,
                     std::string_view binding_id) {
  for (const auto &bind : binds) {
    if (bind.id == binding_id)
      return &bind;
  }
  return nullptr;
}

[[nodiscard]] inline bool stage_dsl_graph_file(
    const std::filesystem::path &source_path,
    const std::vector<cuwacunu::camahjucunu::dsl_variable_t> &variables,
    const std::filesystem::path &target_path,
    const std::filesystem::path &output_dir, staged_dsl_graph_state_t *state,
    std::string *error) {
  if (error)
    error->clear();
  if (!state) {
    if (error)
      *error = "missing staged DSL graph state";
    return false;
  }
  const std::filesystem::path normalized_source =
      source_path.lexically_normal();
  const std::filesystem::path normalized_target =
      target_path.lexically_normal();
  const std::string source_key = normalize_path_key(normalized_source);
  const std::string target_key = normalize_path_key(normalized_target);

  if (const auto it = state->source_to_snapshot.find(source_key);
      it != state->source_to_snapshot.end() &&
      it->second != normalized_target) {
    if (error) {
      *error = "conflicting snapshot targets for DSL source: " + source_key;
    }
    return false;
  }
  if (const auto it = state->snapshot_to_source.find(target_key);
      it != state->snapshot_to_source.end() && it->second != source_key) {
    if (error) {
      *error =
          "conflicting staged DSL destination: " + normalized_target.string();
    }
    return false;
  }

  state->source_to_snapshot[source_key] = normalized_target;
  state->snapshot_to_source[target_key] = source_key;
  if (std::filesystem::exists(normalized_target) &&
      !is_active_source(*state, source_key)) {
    return true;
  }
  if (is_active_source(*state, source_key))
    return true;

  state->active_sources.push_back(source_key);
  const auto cleanup_active = [&]() {
    if (!state->active_sources.empty() &&
        state->active_sources.back() == source_key) {
      state->active_sources.pop_back();
      return;
    }
    for (auto it = state->active_sources.begin();
         it != state->active_sources.end(); ++it) {
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
  if (!cuwacunu::camahjucunu::resolve_dsl_variables_in_text(
          raw_text, variables, &resolved_text, error)) {
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
      const std::filesystem::path child_source(resolve_path_from_folder(
          normalized_source.parent_path(), path_assignment.raw_value));
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

      const std::filesystem::path child_target =
          make_unique_snapshot_path(output_dir, child_source.filename(),
                                    normalize_path_key(child_source), state);
      if (!stage_dsl_graph_file(child_source, variables, child_target,
                                output_dir, state, error)) {
        cleanup_active();
        return false;
      }

      std::filesystem::path relative_child =
          child_target.lexically_relative(normalized_target.parent_path());
      if (relative_child.empty())
        relative_child = child_target.filename();
      const std::string replacement =
          path_assignment.quoted
              ? std::string(1, path_assignment.quote_char) +
                    relative_child.string() +
                    std::string(1, path_assignment.quote_char)
              : relative_child.string();
      line = line.substr(0, path_assignment.replace_begin) + replacement +
             line.substr(path_assignment.replace_end);
    }

    if (!first)
      rebuilt << "\n";
    first = false;
    rebuilt << line;
    in_block_comment = next_block_comment_state(line, in_block_comment);
    if (end == std::string::npos)
      break;
    pos = end + 1;
  }

  std::error_code mkdir_ec{};
  std::filesystem::create_directories(normalized_target.parent_path(),
                                      mkdir_ec);
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
    const std::filesystem::path &source_scope_path,
    const std::filesystem::path &source_contract_path,
    const std::filesystem::path &source_wave_path,
    const cuwacunu::camahjucunu::iitepi_campaign_bind_decl_t &bind,
    const std::filesystem::path &output_dir,
    resolved_wave_contract_binding_snapshot_t *out_snapshot,
    std::string *error) {
  if (error)
    error->clear();
  if (!out_snapshot) {
    if (error)
      *error = "missing binding snapshot output pointer";
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
  std::vector<cuwacunu::camahjucunu::dsl_variable_t> contract_variables{};
  if (!load_contract_dsl_variables(source_contract_path, &contract_variables,
                                   error)) {
    return false;
  }
  if (!validate_bind_variables_do_not_shadow_contract_variables(
          contract_variables, bind.variables, error)) {
    return false;
  }
  if (!stage_dsl_graph_file(source_contract_path, contract_variables,
                            contract_path, instructions_dir, &staged_graph,
                            error)) {
    return false;
  }
  if (!stage_dsl_graph_file(source_wave_path, bind.variables, wave_path,
                            instructions_dir, &staged_graph, error)) {
    return false;
  }
  if (!apply_bind_mounts_to_wave_snapshot(contract_path, bind, wave_path,
                                          error)) {
    return false;
  }
  if (!store_text_file(campaign_path, campaign_text, error))
    return false;

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

} // namespace detail

[[nodiscard]] inline bool prepare_campaign_binding_snapshot(
    const std::filesystem::path &source_campaign_dsl_path,
    std::string_view requested_binding_id,
    const std::filesystem::path &output_dir,
    resolved_wave_contract_binding_snapshot_t *out_snapshot,
    std::string *error = nullptr) {
  if (error)
    error->clear();
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
    instruction = cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
        grammar_text, instruction_text);
  } catch (const std::exception &e) {
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
  const auto *bind =
      detail::find_bind_decl_by_id(instruction.binds, binding_id);
  if (!bind) {
    if (error)
      *error = "unknown campaign binding id: " + binding_id;
    return false;
  }
  const auto *contract_decl = detail::find_contract_decl_by_id(
      instruction.contracts, bind->contract_ref);
  if (!contract_decl) {
    if (error) {
      *error = "binding references unknown CONTRACT id: " + bind->contract_ref;
    }
    return false;
  }

  const std::filesystem::path base_folder =
      source_campaign_dsl_path.parent_path();
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

[[nodiscard]] inline bool explain_campaign_binding_selection(
    const std::filesystem::path &source_campaign_dsl_path,
    std::string_view requested_binding_id,
    resolved_binding_selection_explanation_t *out_explanation,
    std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out_explanation) {
    if (error)
      *error = "missing binding selection explanation output pointer";
    return false;
  }
  *out_explanation = {};
  out_explanation->source_campaign_dsl_path =
      source_campaign_dsl_path.lexically_normal().string();

  const auto fail = [&](std::string summary,
                        std::string details) -> bool {
    out_explanation->resolved = false;
    out_explanation->summary = std::move(summary);
    out_explanation->details = std::move(details);
    if (error) {
      *error = out_explanation->details.empty() ? out_explanation->summary
                                                : out_explanation->details;
    }
    return false;
  };

  const std::string binding_id = detail::trim_ascii(requested_binding_id);
  if (binding_id.empty()) {
    return fail("binding selection explanation requires an explicit binding id",
                "campaign binding selection explanation requires an explicit "
                "binding id");
  }
  out_explanation->binding_id = binding_id;

  std::string campaign_grammar_text{};
  if (!detail::load_text_file(detail::configured_campaign_grammar_path(),
                              &campaign_grammar_text, error)) {
    return fail("cannot load iitepi campaign grammar",
                error ? *error : "cannot load iitepi campaign grammar");
  }
  std::string campaign_text{};
  if (!detail::load_text_file(source_campaign_dsl_path, &campaign_text,
                              error)) {
    return fail("cannot load campaign DSL",
                error ? *error : "cannot load campaign DSL");
  }

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  try {
    instruction = cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
        campaign_grammar_text, campaign_text);
  } catch (const std::exception &e) {
    return fail("cannot decode iitepi campaign DSL",
                "cannot decode iitepi campaign DSL '" +
                    source_campaign_dsl_path.string() + "': " + e.what());
  }

  const auto *bind =
      detail::find_bind_decl_by_id(instruction.binds, binding_id);
  if (!bind) {
    return fail("unknown campaign binding id",
                "unknown campaign binding id: " + binding_id);
  }
  out_explanation->contract_ref = detail::trim_ascii(bind->contract_ref);
  out_explanation->wave_ref = detail::trim_ascii(bind->wave_ref);

  const auto *contract_decl =
      detail::find_contract_decl_by_id(instruction.contracts, bind->contract_ref);
  if (!contract_decl) {
    return fail("binding references unknown CONTRACT id",
                "binding references unknown CONTRACT id: " + bind->contract_ref);
  }

  const std::filesystem::path base_folder =
      source_campaign_dsl_path.parent_path();
  const std::filesystem::path source_contract_path(
      detail::resolve_path_from_folder(base_folder, contract_decl->file));
  out_explanation->source_contract_dsl_path =
      source_contract_path.lexically_normal().string();

  std::string source_wave_path{};
  if (!detail::find_matching_wave_import_path(
          instruction.waves, source_campaign_dsl_path, bind->wave_ref,
          &source_wave_path, error)) {
    return fail("binding references unknown WAVE id",
                error ? *error : "binding references unknown WAVE id");
  }
  out_explanation->source_wave_dsl_path =
      std::filesystem::path(source_wave_path).lexically_normal().string();

  std::vector<cuwacunu::camahjucunu::dsl_variable_t> contract_variables{};
  if (!detail::load_contract_dsl_variables(source_contract_path,
                                           &contract_variables, error)) {
    return fail("cannot load contract variables",
                error ? *error : "cannot load contract variables");
  }
  if (!detail::validate_bind_variables_do_not_shadow_contract_variables(
          contract_variables, bind->variables, error)) {
    return fail("bind variables shadow contract variables",
                error ? *error : "bind variables shadow contract variables");
  }

  std::string wave_grammar_text{};
  if (!detail::load_text_file(detail::configured_wave_grammar_path(),
                              &wave_grammar_text, error)) {
    return fail("cannot load iitepi wave grammar",
                error ? *error : "cannot load iitepi wave grammar");
  }
  std::string wave_text{};
  if (!detail::load_text_file(source_wave_path, &wave_text, error)) {
    return fail("cannot load wave DSL",
                error ? *error : "cannot load wave DSL");
  }
  std::string resolved_wave_text{};
  if (!cuwacunu::camahjucunu::resolve_dsl_variables_in_text(
          wave_text, bind->variables, &resolved_wave_text, error)) {
    return fail("cannot resolve bind-local wave variables",
                error ? *error
                      : "cannot resolve bind-local wave variables");
  }

  cuwacunu::camahjucunu::iitepi_wave_set_t wave_set{};
  try {
    wave_set = cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(
        wave_grammar_text, resolved_wave_text);
  } catch (const std::exception &e) {
    return fail("cannot decode selected wave DSL",
                "cannot decode selected wave DSL '" + source_wave_path +
                    "': " + e.what());
  }

  const auto wave_it = std::find_if(
      wave_set.waves.begin(), wave_set.waves.end(), [&](const auto &wave) {
        return detail::trim_ascii(wave.name) == detail::trim_ascii(bind->wave_ref);
      });
  if (wave_it == wave_set.waves.end()) {
    return fail("selected bind references unknown WAVE id in resolved wave DSL",
                "selected bind references unknown WAVE id in resolved wave DSL: " +
                    bind->wave_ref);
  }

  const auto contract_hash =
      cuwacunu::iitepi::contract_space_t::register_contract_file(
          source_contract_path.string());
  out_explanation->contract_hash = detail::trim_ascii(contract_hash);
  const auto contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
  if (!contract_snapshot) {
    return fail("cannot load contract snapshot for selection explanation",
                "cannot load contract snapshot for selection explanation: " +
                    source_contract_path.string());
  }
  out_explanation->dock_hash = detail::trim_ascii(
      contract_snapshot->signature.docking_signature_sha256_hex);
  if (out_explanation->dock_hash.empty()) {
    return fail("contract snapshot is missing docking signature",
                "contract snapshot is missing docking signature");
  }

  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t catalog{};
  bool catalog_open_attempted = false;
  std::string catalog_open_error{};
  const auto try_open_catalog = [&](bool required) -> bool {
    if (catalog.opened())
      return true;
    if (catalog_open_attempted) {
      return required ? catalog.opened() : false;
    }
    catalog_open_attempted = true;
    if (!detail::open_mount_hashimyei_catalog(&catalog, &catalog_open_error)) {
      return required ? false : false;
    }
    catalog_open_error.clear();
    return true;
  };

  std::unordered_map<std::string, bool> used_mounts{};
  used_mounts.reserve(bind->mounts.size());
  for (const auto &mount : bind->mounts) {
    used_mounts.emplace(detail::trim_ascii(mount.wave_binding_id), false);
  }
  out_explanation->mounts.reserve(wave_it->wikimyeis.size() +
                                  wave_it->probes.size());

  const auto append_unknown_mount_failure =
      [&](std::string_view wave_binding_id) -> bool {
    resolved_mount_selection_explanation_t explanation{};
    explanation.wave_binding_id = detail::trim_ascii(wave_binding_id);
    explanation.summary = "bind declares MOUNT for unknown wave binding id";
    explanation.details =
        "BIND '" + bind->id +
        "' declares MOUNT for unknown wave binding id: " +
        explanation.wave_binding_id;
    out_explanation->mounts.push_back(explanation);
    return fail("bind declares MOUNT for unknown wave binding id",
                explanation.details);
  };

  const auto explain_mount =
      [&](std::string_view component_kind, std::string_view wave_binding_id,
          std::string_view family) -> bool {
    resolved_mount_selection_explanation_t explanation{};
    explanation.wave_binding_id = detail::trim_ascii(wave_binding_id);
    explanation.component_kind = std::string(component_kind);
    explanation.family = detail::trim_ascii(family);

    const auto *mount = detail::find_mount_decl(*bind, wave_binding_id);
    if (!mount) {
      explanation.summary = "missing required MOUNT";
      explanation.details = "BIND '" + bind->id + "' missing required MOUNT for " +
                            std::string(component_kind) + " <" +
                            explanation.wave_binding_id + ">";
      out_explanation->mounts.push_back(explanation);
      return fail("missing required MOUNT", explanation.details);
    }

    used_mounts[detail::trim_ascii(wave_binding_id)] = true;
    explanation.selector_kind =
        detail::mount_selector_kind_name(mount->selector_kind);

    if (mount->selector_kind ==
        cuwacunu::camahjucunu::iitepi_campaign_mount_selector_kind_e::Exact) {
      std::string normalized_hash{};
      if (!cuwacunu::hashimyei::normalize_hex_hash_name(mount->exact_hashimyei,
                                                        &normalized_hash)) {
        explanation.selector_value = detail::trim_ascii(mount->exact_hashimyei);
        explanation.summary = "invalid EXACT selector";
        explanation.details =
            "MOUNT EXACT requires canonical 0x<hex> hashimyei: " +
            mount->exact_hashimyei;
        out_explanation->mounts.push_back(explanation);
        return fail("invalid EXACT selector", explanation.details);
      }
      explanation.selector_value = normalized_hash;
      explanation.resolved_hashimyei = normalized_hash;
      explanation.resolved_canonical_path = detail::mount_component_canonical_path(
          explanation.family, normalized_hash);

      if (try_open_catalog(false) && catalog.opened()) {
        explanation.catalog_checked = true;
        cuwacunu::hero::hashimyei::component_state_t component{};
        std::string resolve_error{};
        const std::string canonical_path = detail::mount_component_canonical_path(
            explanation.family, normalized_hash);
        if (catalog.resolve_component(canonical_path, normalized_hash, &component,
                                      &resolve_error)) {
          explanation.catalog_manifest_found = true;
          const std::string actual_family =
              detail::trim_ascii(component.manifest.family);
          const std::string actual_dock = detail::trim_ascii(
              component.manifest.docking_signature_sha256_hex);
          if (!explanation.family.empty() && !actual_family.empty() &&
              actual_family != explanation.family) {
            explanation.summary = "EXACT selector targets the wrong family";
            explanation.details = "MOUNT EXACT " + normalized_hash +
                                  " targets family '" + actual_family +
                                  "', expected '" + explanation.family + "'";
            out_explanation->mounts.push_back(explanation);
            return fail("EXACT selector targets the wrong family",
                        explanation.details);
          }
          if (!out_explanation->dock_hash.empty() && !actual_dock.empty() &&
              actual_dock != out_explanation->dock_hash) {
            explanation.summary =
                "EXACT selector is not dock-compatible with the selected contract";
            explanation.details = "MOUNT EXACT " + normalized_hash +
                                  " is not dock-compatible with the selected "
                                  "contract";
            out_explanation->mounts.push_back(explanation);
            return fail(
                "EXACT selector is not dock-compatible with the selected contract",
                explanation.details);
          }
          const std::string actual_canonical =
              detail::trim_ascii(component.manifest.canonical_path);
          if (!actual_canonical.empty()) {
            explanation.resolved_canonical_path = actual_canonical;
          }
          explanation.summary =
              "EXACT selector resolved a dock-compatible component";
          explanation.details =
              "MOUNT <" + explanation.wave_binding_id + "> uses EXACT " +
              normalized_hash + " and the catalog manifest matches family '" +
              explanation.family + "' plus dock " + out_explanation->dock_hash +
              ".";
        } else {
          explanation.summary = "EXACT selector accepted explicit hashimyei";
          explanation.details =
              "MOUNT <" + explanation.wave_binding_id + "> uses EXACT " +
              normalized_hash +
              ". Catalog verification is optional for EXACT mounts, so this "
              "explicit selection remains valid even without a stored manifest.";
          if (!detail::trim_ascii(resolve_error).empty()) {
            explanation.details += " Catalog lookup detail: " + resolve_error;
          }
        }
      } else {
        explanation.summary = "EXACT selector accepted explicit hashimyei";
        explanation.details =
            "MOUNT <" + explanation.wave_binding_id + "> uses EXACT " +
            normalized_hash +
            ". Catalog verification is optional for EXACT mounts and was not "
            "required for this explanation path.";
      }
      explanation.resolved = true;
      out_explanation->mounts.push_back(explanation);
      return true;
    }

    explanation.selector_value = std::to_string(mount->rank);
    if (!try_open_catalog(true) || !catalog.opened()) {
      explanation.summary = "cannot open hashimyei catalog for RANK selector";
      explanation.details = catalog_open_error.empty()
                                ? "cannot open hashimyei catalog for RANK selector"
                                : catalog_open_error;
      out_explanation->mounts.push_back(explanation);
      return fail("cannot open hashimyei catalog for RANK selector",
                  explanation.details);
    }
    explanation.catalog_checked = true;

    std::string resolved_hashimyei{};
    std::string resolve_error{};
    if (!detail::resolve_mount_rank_hashimyei(*mount, explanation.family,
                                              out_explanation->dock_hash,
                                              &catalog, &resolved_hashimyei,
                                              &resolve_error)) {
      explanation.summary =
          "RANK selector could not resolve a dock-compatible component";
      explanation.details = resolve_error.empty()
                                ? "RANK selector could not resolve a "
                                  "dock-compatible component"
                                : resolve_error;
      out_explanation->mounts.push_back(explanation);
      return fail("RANK selector could not resolve a dock-compatible component",
                  explanation.details);
    }

    explanation.resolved = true;
    explanation.resolved_hashimyei = resolved_hashimyei;
    explanation.resolved_canonical_path = detail::mount_component_canonical_path(
        explanation.family, resolved_hashimyei);

    cuwacunu::hero::hashimyei::component_state_t component{};
    std::string component_error{};
    if (catalog.resolve_component(explanation.resolved_canonical_path,
                                  resolved_hashimyei, &component,
                                  &component_error)) {
      explanation.catalog_manifest_found = true;
      const std::string actual_canonical =
          detail::trim_ascii(component.manifest.canonical_path);
      if (!actual_canonical.empty()) {
        explanation.resolved_canonical_path = actual_canonical;
      }
    }
    explanation.summary = "RANK selector resolved a dock-compatible component";
    explanation.details =
        "MOUNT <" + explanation.wave_binding_id + "> resolved family '" +
        explanation.family + "' at dock " + out_explanation->dock_hash +
        " to ranked member " + explanation.selector_value + " -> " +
        explanation.resolved_hashimyei + ".";
    out_explanation->mounts.push_back(explanation);
    return true;
  };

  for (const auto &wikimyei : wave_it->wikimyeis) {
    if (!explain_mount("WIKIMYEI", wikimyei.binding_id, wikimyei.family)) {
      return false;
    }
  }
  for (const auto &probe : wave_it->probes) {
    if (!explain_mount("PROBE", probe.binding_id, probe.family)) {
      return false;
    }
  }
  for (const auto &entry : used_mounts) {
    if (!entry.second) {
      return append_unknown_mount_failure(entry.first);
    }
  }

  out_explanation->resolved = true;
  out_explanation->summary =
      "BIND '" + out_explanation->binding_id + "' resolved " +
      std::to_string(out_explanation->mounts.size()) +
      " MOUNT selector(s)";
  out_explanation->details =
      "Selection used contract '" + out_explanation->contract_ref +
      "' (contract_hash=" + out_explanation->contract_hash + ", dock_hash=" +
      out_explanation->dock_hash + ") and wave '" + out_explanation->wave_ref +
      "'. Compatibility is dock-only; ASSEMBLY differences do not affect "
      "MOUNT resolution.";
  return true;
}

} // namespace wave_contract_binding_runtime
} // namespace hero
} // namespace cuwacunu
