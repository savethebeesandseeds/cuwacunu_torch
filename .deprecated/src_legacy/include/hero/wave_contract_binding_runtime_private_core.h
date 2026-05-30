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
#include "iitepi/config_space_t.h"
#include "iitepi/contract_space_t.h"

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

struct resolved_component_selection_explanation_t {
  std::string wave_binding_id{};
  std::string component_kind{};
  std::string family{};
  std::string component_compatibility_sha256_hex{};
  std::string component_tag{};
  bool resolved{false};
  std::string derived_hashimyei{};
  std::string derived_canonical_path{};
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
  std::string docking_signature_sha256_hex{};
  bool resolved{false};
  std::string summary{};
  std::string details{};
  std::vector<resolved_component_selection_explanation_t> components{};
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
