#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "iinuji/iinuji_cmd/views/board/commands.h"
#include "tsiemene/tsi.type.registry.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline const std::vector<std::string>& board_completion_tokens() {
  static const std::vector<std::string> tokens = [] {
    std::vector<std::string> out;
    out.reserve(64);

    out.emplace_back("->");
    out.emplace_back("{");
    out.emplace_back("}");
    out.emplace_back("=");
    out.emplace_back("(");
    out.emplace_back(")");
    out.emplace_back(":tensor");
    out.emplace_back(":str");

    for (const auto& item : tsiemene::kTsiTypeRegistry) {
      out.emplace_back(std::string(item.canonical));
    }

#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY) out.emplace_back(TOKEN);
#define TSI_PATH_METHOD(ID, TOKEN, SUMMARY) out.emplace_back(TOKEN);
#define TSI_PATH_COMPONENT(TYPE_ID, CANONICAL, DOMAIN, INSTANCE_POLICY, SUMMARY)
#define TSI_PATH_LANE(TYPE_ID, DIR, DIRECTIVE_ID, KIND, SUMMARY)
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_LANE
#undef TSI_PATH_COMPONENT
#undef TSI_PATH_METHOD
#undef TSI_PATH_DIRECTIVE

#define BOARD_PATH_DIRECTIVE(ID, TOKEN, SUMMARY) out.emplace_back(TOKEN);
#define BOARD_PATH_METHOD(ID, TOKEN, SUMMARY) out.emplace_back(TOKEN);
#define BOARD_PATH_ACTION(ID, TOKEN, SUMMARY)
#define BOARD_PATH_DSL_SEGMENT(ID, KEY, SUMMARY)
#include "iitepi/board/board.paths.def"
#undef BOARD_PATH_DSL_SEGMENT
#undef BOARD_PATH_ACTION
#undef BOARD_PATH_METHOD
#undef BOARD_PATH_DIRECTIVE

    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
  }();
  return tokens;
}

inline void clear_board_completion(CmdState& st) {
  st.board.completion_active = false;
  st.board.completion_items.clear();
  st.board.completion_index = 0;
  st.board.completion_line = -1;
  st.board.completion_start_col = -1;
}

inline std::string_view trim_completion_line_view(std::string_view s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.remove_prefix(1);
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.remove_suffix(1);
  return s;
}

inline bool board_line_looks_like_circuit_header(std::string_view line) {
  line = trim_completion_line_view(line);
  return line.find("= {") != std::string_view::npos ||
         line.find("={") != std::string_view::npos;
}

inline bool board_line_looks_like_circuit_close(std::string_view line) {
  line = trim_completion_line_view(line);
  return !line.empty() && line.front() == '}';
}

inline bool board_find_contract_region(const cuwacunu::iinuji::editorBox_data_t& ed,
                                       std::size_t contract_index,
                                       int* out_begin,
                                       int* out_end) {
  if (!out_begin || !out_end) return false;
  *out_begin = -1;
  *out_end = -1;
  if (ed.lines.empty()) return false;

  std::size_t seen = 0;
  for (int i = 0; i < static_cast<int>(ed.lines.size()); ++i) {
    if (!board_line_looks_like_circuit_header(ed.lines[static_cast<std::size_t>(i)])) continue;
    if (seen == contract_index) {
      *out_begin = i;
      break;
    }
    ++seen;
  }
  if (*out_begin < 0) return false;

  int end = static_cast<int>(ed.lines.size()) - 1;
  for (int i = *out_begin + 1; i < static_cast<int>(ed.lines.size()); ++i) {
    if (!board_line_looks_like_circuit_close(ed.lines[static_cast<std::size_t>(i)])) continue;
    end = i;
    break;
  }
  int invoke_line = end;
  for (int i = end + 1; i < static_cast<int>(ed.lines.size()); ++i) {
    const std::string_view line = trim_completion_line_view(ed.lines[static_cast<std::size_t>(i)]);
    if (line.empty()) continue;
    invoke_line = i;
    break;
  }
  *out_end = invoke_line;
  return true;
}

inline bool board_completion_allowed_at_cursor(const CmdState& st,
                                               const cuwacunu::iinuji::editorBox_data_t& ed) {
  if (ed.cursor_line < 0 || ed.cursor_line >= static_cast<int>(ed.lines.size())) return false;
  if (st.board.editor_scope == BoardEditorScope::ContractVirtual) {
    return true;
  }
  if (st.board.editor_scope == BoardEditorScope::FullInstruction ||
      st.board.editor_scope == BoardEditorScope::ContractSection) {
    return false;
  }

  int begin = -1;
  int end = -1;
  if (!board_find_contract_region(ed, st.board.selected_circuit, &begin, &end)) {
    return false;
  }
  return ed.cursor_line >= begin && ed.cursor_line <= end;
}

inline bool starts_with_token(std::string_view s, std::string_view p) {
  return s.size() >= p.size() && s.substr(0, p.size()) == p;
}

inline std::unordered_map<std::string, tsiemene::TsiTypeId> board_alias_type_map(const CmdState& st) {
  std::unordered_map<std::string, tsiemene::TsiTypeId> out;
  if (!st.board.editor) return out;
  for (const auto& line_raw : st.board.editor->lines) {
    std::string_view line = trim_completion_line_view(line_raw);
    if (line.empty()) continue;
    if (line.find("->") != std::string_view::npos) continue;
    const std::size_t eq = line.find('=');
    if (eq == std::string_view::npos) continue;
    const std::string alias(trim_completion_line_view(line.substr(0, eq)));
    const std::string type(trim_completion_line_view(line.substr(eq + 1)));
    if (alias.empty() || type.empty()) continue;
    const auto type_id = tsiemene::parse_tsi_type_id(type);
    if (!type_id.has_value()) continue;
    out.emplace(alias, *type_id);
  }
  return out;
}

inline std::vector<std::string> board_candidates_for_context(const CmdState& st,
                                                             const cuwacunu::iinuji::editorBox_data_t& ed,
                                                             std::string_view prefix,
                                                             int token_start_col) {
  std::vector<std::string> out;
  const auto alias_types = board_alias_type_map(st);
  if (ed.cursor_line < 0 || ed.cursor_line >= static_cast<int>(ed.lines.size())) return out;

  const std::string& line_raw = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
  const int cursor_col = std::clamp(ed.cursor_col, 0, static_cast<int>(line_raw.size()));
  std::string_view line = line_raw;
  const std::size_t arrow = line.find("->");
  const bool in_hop = (arrow != std::string_view::npos);
  const bool completing_directive =
      token_start_col >= 0 &&
      token_start_col < static_cast<int>(line.size()) &&
      line[static_cast<std::size_t>(token_start_col)] == '@';

  auto append_aliases = [&]() {
    for (const auto& kv : alias_types) out.push_back(kv.first);
  };
  auto append_type_tokens = [&]() {
    for (const auto& item : tsiemene::kTsiTypeRegistry) out.push_back(std::string(item.canonical));
  };
  auto append_all_directives = [&]() {
#define TSI_PATH_DIRECTIVE(ID, TOKEN, SUMMARY) out.emplace_back(TOKEN);
#define TSI_PATH_METHOD(ID, TOKEN, SUMMARY)
#define TSI_PATH_COMPONENT(TYPE_ID, CANONICAL, DOMAIN, INSTANCE_POLICY, SUMMARY)
#define TSI_PATH_LANE(TYPE_ID, DIR, DIRECTIVE_ID, KIND, SUMMARY)
#include "tsiemene/tsi.paths.def"
#undef TSI_PATH_LANE
#undef TSI_PATH_COMPONENT
#undef TSI_PATH_METHOD
#undef TSI_PATH_DIRECTIVE

#define BOARD_PATH_DIRECTIVE(ID, TOKEN, SUMMARY) out.emplace_back(TOKEN);
#define BOARD_PATH_METHOD(ID, TOKEN, SUMMARY)
#define BOARD_PATH_ACTION(ID, TOKEN, SUMMARY)
#define BOARD_PATH_DSL_SEGMENT(ID, KEY, SUMMARY)
#include "iitepi/board/board.paths.def"
#undef BOARD_PATH_DSL_SEGMENT
#undef BOARD_PATH_ACTION
#undef BOARD_PATH_METHOD
#undef BOARD_PATH_DIRECTIVE
  };
  auto append_output_directives_of = [&](std::string_view alias) {
    const auto it = alias_types.find(std::string(alias));
    if (it == alias_types.end()) {
      append_all_directives();
      return;
    }
    for (const auto& d : tsiemene::tsi_type_outputs(it->second)) out.emplace_back(std::string(d.id));
  };
  auto append_input_directives_of = [&](std::string_view alias) {
    const auto it = alias_types.find(std::string(alias));
    if (it == alias_types.end()) {
      append_all_directives();
      return;
    }
    for (const auto& d : tsiemene::tsi_type_inputs(it->second)) out.emplace_back(std::string(d.id));
  };

  if (!in_hop) {
    const std::size_t eq = line.find('=');
    if (eq != std::string_view::npos && cursor_col > static_cast<int>(eq)) {
      append_type_tokens();
    } else {
      append_aliases();
    }
  } else {
    const bool rhs = cursor_col > static_cast<int>(arrow + 2);
    if (!rhs) {
      const std::size_t at = line.find('@');
      const std::size_t colon = (at == std::string_view::npos) ? std::string_view::npos : line.find(':', at + 1);
      if (token_start_col >= 0 && token_start_col < static_cast<int>(line.size()) &&
          line[static_cast<std::size_t>(token_start_col)] == '@') {
        const std::string_view alias = trim_completion_line_view(line.substr(0, at));
        append_output_directives_of(alias);
      } else if (token_start_col >= 0 && token_start_col < static_cast<int>(line.size()) &&
                 line[static_cast<std::size_t>(token_start_col)] == ':') {
        out.emplace_back(":tensor");
        out.emplace_back(":str");
      } else if (at != std::string_view::npos && colon != std::string_view::npos &&
                 cursor_col > static_cast<int>(colon)) {
        out.emplace_back(":tensor");
        out.emplace_back(":str");
      } else {
        append_aliases();
      }
    } else {
      std::string_view rhs_text = line.substr(arrow + 2);
      const std::size_t at_rhs_local = rhs_text.find('@');
      if (token_start_col >= 0 && token_start_col < static_cast<int>(line.size()) &&
          line[static_cast<std::size_t>(token_start_col)] == '@') {
        std::string_view rhs_alias = rhs_text;
        if (at_rhs_local != std::string_view::npos) rhs_alias = rhs_text.substr(0, at_rhs_local);
        append_input_directives_of(trim_completion_line_view(rhs_alias));
      } else {
        append_aliases();
      }
    }
  }

  for (const auto& tok : board_completion_tokens()) {
    if (tok == ":tensor" || tok == ":str") continue;
    if (completing_directive && !tok.empty() && tok.front() == '@') continue;
    out.push_back(tok);
  }
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());

  std::vector<std::string> filtered;
  filtered.reserve(out.size());
  for (const auto& cand : out) {
    if (starts_with_token(cand, prefix)) filtered.push_back(cand);
  }
  return filtered;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
