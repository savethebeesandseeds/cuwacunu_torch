#pragma once

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/views/board/completion.h"
#include "iinuji/iinuji_cmd/views/board/contract.section.registry.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string render_board_circuit_instruction_text(
    const cuwacunu::camahjucunu::tsiemene_circuit_decl_t& c) {
  std::ostringstream oss;
  oss << c.name << " = {\n";
  for (const auto& inst : c.instances) {
    oss << "  " << inst.alias << " = " << inst.tsi_type << "\n";
  }
  for (const auto& h : c.hops) {
    oss << "  " << h.from.instance << "@" << h.from.directive;
    if (!h.from.kind.empty()) oss << ":" << h.from.kind;
    oss << " -> " << h.to.instance << "@" << h.to.directive;
    if (!h.to.kind.empty()) oss << ":" << h.to.kind;
    oss << "\n";
  }
  oss << "}\n";
  oss << c.invoke_name << "(" << c.invoke_payload << ");\n";
  return oss.str();
}

inline std::string render_board_instruction_text(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t& board) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < board.contracts.size(); ++i) {
    if (i > 0) oss << "\n";
    oss << render_board_circuit_instruction_text(board.contracts[i]);
  }
  return oss.str();
}

inline void append_board_contract_segment_block(std::ostringstream& oss,
                                                std::string_view key,
                                                const std::string& text) {
  oss << "BEGIN " << key << "\n";
  if (text.empty()) {
    oss << "# missing DSL text\n";
  } else {
    oss << text;
    if (text.back() != '\n') oss << "\n";
  }
  oss << "END " << key << "\n";
}

inline std::string render_board_contract_text_for_selected_contract(const CmdState& st,
                                                                    std::size_t contract_index) {
  return render_board_contract_text_by_sections(st, contract_index);
}

inline std::string board_contract_section_editor_path(const std::string& instruction_path,
                                                      BoardContractSection section) {
  std::string out = instruction_path;
  out += "#section:";
  out += board_contract_section_key(section);
  return out;
}

inline std::string board_contract_section_instruction_path(BoardContractSection section,
                                                           const std::string& circuit_fallback_path,
                                                           const std::string& contract_hash) {
  if (section == BoardContractSection::Circuit) return circuit_fallback_path;
  std::string key;
  switch (section) {
    case BoardContractSection::ObservationSources:
      key = "observation_sources_dsl_filename";
      break;
    case BoardContractSection::ObservationChannels:
      key = "observation_channels_dsl_filename";
      break;
    case BoardContractSection::JkimyeiSpecs:
      key = "jkimyei_specs_dsl_filename";
      break;
    case BoardContractSection::Circuit:
      break;
  }
  std::string path;
  if (lookup_contract_config_value("DSL", key, contract_hash, &path) && !path.empty()) {
    return path;
  }
  return {};
}

inline bool write_text_file(const std::string& path,
                            const std::string& text,
                            std::string* error_out = nullptr) {
  if (error_out) error_out->clear();
  if (path.empty()) {
    if (error_out) *error_out = "instruction path is empty";
    return false;
  }
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error_out) *error_out = "cannot write file: " + path;
    return false;
  }
  out << text;
  if (!out.good()) {
    if (error_out) *error_out = "write failed: " + path;
    return false;
  }
  return true;
}

inline bool build_merged_board_from_virtual_contract_text(
    const CmdState& st,
    const std::string& contract_text,
    cuwacunu::camahjucunu::tsiemene_circuit_instruction_t* out_board,
    std::vector<std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t>>* out_resolved_hops,
    std::string* out_board_text,
    std::string* error_out = nullptr) {
  if (error_out) error_out->clear();
  if (!board_has_circuits(st)) {
    if (error_out) *error_out = "cannot merge contract: board has no contracts";
    return false;
  }
  if (!out_board || !out_resolved_hops || !out_board_text) {
    if (error_out) *error_out = "internal error: null output in virtual contract merge";
    return false;
  }

  cuwacunu::camahjucunu::tsiemene_circuit_instruction_t edited_board{};
  std::vector<std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t>> edited_resolved{};
  std::string parse_error;
  if (!decode_board_instruction_text(
          contract_text,
          st.board.contract_hash,
          &edited_board,
          &edited_resolved,
          &parse_error)) {
    if (error_out) *error_out = "invalid contract text: " + parse_error;
    return false;
  }
  if (edited_board.contracts.size() != 1) {
    if (error_out) {
      *error_out = "contract editor expects exactly one contract, got " +
                   std::to_string(edited_board.contracts.size());
    }
    return false;
  }

  const std::size_t merge_index = std::min(
      st.board.editing_contract_index,
      st.board.board.contracts.empty() ? std::size_t(0) : (st.board.board.contracts.size() - 1));

  auto merged = st.board.board;
  if (merge_index >= merged.contracts.size()) {
    if (error_out) *error_out = "merge index out of range";
    return false;
  }
  merged.contracts[merge_index] = std::move(edited_board.contracts.front());

  std::string semantic_error;
  if (!cuwacunu::camahjucunu::validate_circuit_instruction(merged, &semantic_error)) {
    if (error_out) *error_out = "merged board invalid: " + semantic_error;
    return false;
  }

  std::vector<std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t>> resolved{};
  resolved.reserve(merged.contracts.size());
  for (std::size_t i = 0; i < merged.contracts.size(); ++i) {
    std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t> rh;
    std::string resolve_error;
    if (!cuwacunu::camahjucunu::resolve_hops(merged.contracts[i], &rh, &resolve_error)) {
      if (error_out) *error_out = "merged board resolve error circuit[" + std::to_string(i) + "]: " + resolve_error;
      return false;
    }
    resolved.push_back(std::move(rh));
  }

  *out_board_text = render_board_instruction_text(merged);
  *out_board = std::move(merged);
  *out_resolved_hops = std::move(resolved);
  return true;
}

// Circuit-only editor path retained for targeted circuit merge workflows.
inline bool enter_selected_contract_virtual_editor(CmdState& st) {
  if (!board_has_circuits(st)) return false;
  clamp_board_navigation_state(st);
  const std::size_t idx = st.board.selected_circuit;
  if (idx >= st.board.board.contracts.size()) return false;

  if (!st.board.editor) {
    st.board.editor = std::make_shared<cuwacunu::iinuji::editorBox_data_t>(st.board.instruction_path);
  }
  auto& ed = *st.board.editor;
  ed.path = st.board.instruction_path;
  configure_board_editor_highlighting(ed);
  cuwacunu::iinuji::primitives::editor_set_text(
      ed,
      render_board_circuit_instruction_text(st.board.board.contracts[idx]));
  ed.dirty = false;
  ed.status = "contract circuit edit mode";

  st.board.display_mode = BoardDisplayMode::ContractTextEdit;
  st.board.panel_focus = BoardPanelFocus::ViewOptions;
  st.board.editor_focus = true;
  st.board.editor_scope = BoardEditorScope::ContractVirtual;
  st.board.editing_contract_index = idx;
  st.board.exit_prompt = BoardState::ExitPrompt::None;
  st.board.exit_prompt_index = 0;
  clear_board_completion(st);
  return true;
}

inline bool enter_selected_contract_full_editor(CmdState& st) {
  if (!board_has_circuits(st)) return false;
  clamp_board_navigation_state(st);
  const std::size_t idx = st.board.selected_circuit;
  if (idx >= st.board.board.contracts.size()) return false;

  if (!st.board.editor) {
    st.board.editor = std::make_shared<cuwacunu::iinuji::editorBox_data_t>(st.board.instruction_path);
  }
  auto& ed = *st.board.editor;
  ed.path = st.board.instruction_path;
  configure_board_editor_highlighting(ed);
  cuwacunu::iinuji::primitives::editor_set_text(
      ed,
      render_board_contract_text_for_selected_contract(st, idx));
  ed.dirty = false;
  ed.status = "contract full edit mode (completion/validation disabled)";

  st.board.display_mode = BoardDisplayMode::ContractTextEdit;
  st.board.panel_focus = BoardPanelFocus::ViewOptions;
  st.board.editor_focus = true;
  st.board.editor_scope = BoardEditorScope::FullInstruction;
  st.board.editing_contract_index = idx;
  st.board.exit_prompt = BoardState::ExitPrompt::None;
  st.board.exit_prompt_index = 0;
  clear_board_completion(st);
  return true;
}

inline bool enter_selected_contract_section_editor(CmdState& st) {
  if (!board_has_circuits(st)) return false;
  clamp_board_navigation_state(st);

  const std::size_t idx = st.board.selected_circuit;
  if (idx >= st.board.board.contracts.size()) return false;
  const BoardContractSection section =
      board_contract_section_from_index(st.board.selected_contract_section);
  const std::string text = board_contract_section_get_text(st, idx, section);

  if (!st.board.editor) {
    st.board.editor = std::make_shared<cuwacunu::iinuji::editorBox_data_t>(st.board.instruction_path);
  }
  auto& ed = *st.board.editor;
  ed.path = board_contract_section_editor_path(st.board.instruction_path, section);
  configure_board_editor_highlighting(ed);
  cuwacunu::iinuji::primitives::editor_set_text(ed, text);
  ed.dirty = false;
  ed.status = std::string("contract section edit: ") + std::string(board_contract_section_title(section));

  st.board.display_mode = BoardDisplayMode::ContractTextEdit;
  st.board.panel_focus = BoardPanelFocus::ContractSections;
  st.board.editor_focus = true;
  st.board.editor_scope = BoardEditorScope::ContractSection;
  st.board.editing_contract_index = idx;
  st.board.editing_contract_section = section;
  st.board.exit_prompt = BoardState::ExitPrompt::None;
  st.board.exit_prompt_index = 0;
  clear_board_completion(st);
  return true;
}

inline bool apply_board_instruction_text(CmdState& st,
                                         const std::string& text,
                                         std::string* error_out = nullptr) {
  if (error_out) error_out->clear();
  cuwacunu::camahjucunu::tsiemene_circuit_instruction_t board{};
  std::vector<std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t>> resolved{};
  std::string error;
  const bool ok = decode_board_instruction_text(
      text, st.board.contract_hash, &board, &resolved, &error);

  st.board.raw_instruction = text;
  st.board.board = std::move(board);
  st.board.resolved_hops = std::move(resolved);
  st.board.ok = ok;
  st.board.error = ok ? std::string{} : error;
  if (ok) {
    board_contract_sections_sync_from_runtime_board(st);
  } else {
    st.board.contract_circuit_dsl_sections.clear();
  }
  clamp_board_navigation_state(st);

  if (error_out && !ok) *error_out = error;
  return ok;
}

inline bool sync_board_from_editor(CmdState& st, std::string* error_out = nullptr) {
  if (!st.board.editor) {
    if (error_out) *error_out = "board editor is not available";
    return false;
  }
  if (st.board.editor->path.empty()) {
    st.board.editor->path = st.board.instruction_path;
    configure_board_editor_highlighting(*st.board.editor);
  }
  const std::string text = cuwacunu::iinuji::primitives::editor_text(*st.board.editor);
  if (st.board.editor_scope == BoardEditorScope::ContractVirtual) {
    cuwacunu::camahjucunu::tsiemene_circuit_instruction_t merged{};
    std::vector<std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t>> resolved{};
    std::string merged_text;
    std::string error;
    if (!build_merged_board_from_virtual_contract_text(
            st, text, &merged, &resolved, &merged_text, &error)) {
      if (error_out) *error_out = error;
      return false;
    }

    st.board.raw_instruction = std::move(merged_text);
    st.board.board = std::move(merged);
    st.board.resolved_hops = std::move(resolved);
    st.board.ok = true;
    st.board.error.clear();
    clamp_board_navigation_state(st);
    if (error_out) error_out->clear();
    return true;
  }

  if (st.board.editor_scope == BoardEditorScope::FullInstruction) {
    st.board.raw_instruction = text;
    if (error_out) error_out->clear();
    return true;
  }
  if (st.board.editor_scope == BoardEditorScope::ContractSection) {
    board_contract_section_set_text(
        st,
        st.board.editing_contract_index,
        st.board.editing_contract_section,
        text);
    if (error_out) error_out->clear();
    return true;
  }

  return apply_board_instruction_text(st, text, error_out);
}

inline bool persist_board_editor(CmdState& st, std::string* error_out = nullptr) {
  if (error_out) error_out->clear();
  if (!st.board.editor) {
    if (error_out) *error_out = "board editor is not available";
    return false;
  }
  auto& ed = *st.board.editor;

  if (st.board.editor_scope == BoardEditorScope::ContractVirtual) {
    const std::string text = cuwacunu::iinuji::primitives::editor_text(ed);
    cuwacunu::camahjucunu::tsiemene_circuit_instruction_t merged{};
    std::vector<std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t>> resolved{};
    std::string merged_text;
    std::string error;
    if (!build_merged_board_from_virtual_contract_text(
            st, text, &merged, &resolved, &merged_text, &error)) {
      if (error_out) *error_out = error;
      return false;
    }
    if (!write_text_file(st.board.instruction_path, merged_text, &error)) {
      if (error_out) *error_out = error;
      return false;
    }

    st.board.raw_instruction = std::move(merged_text);
    st.board.board = std::move(merged);
    st.board.resolved_hops = std::move(resolved);
    st.board.ok = true;
    st.board.error.clear();
    board_contract_sections_sync_from_runtime_board(st);
    clamp_board_navigation_state(st);

    const std::size_t idx = std::min(
        st.board.editing_contract_index,
        st.board.board.contracts.empty() ? std::size_t(0) : (st.board.board.contracts.size() - 1));
    cuwacunu::iinuji::primitives::editor_set_text(
        ed,
        st.board.board.contracts.empty()
            ? std::string()
            : render_board_circuit_instruction_text(st.board.board.contracts[idx]));
    ed.dirty = false;
    return true;
  }

  if (st.board.editor_scope == BoardEditorScope::FullInstruction) {
    const std::string text = cuwacunu::iinuji::primitives::editor_text(ed);
    std::string error;
    if (!write_text_file(st.board.instruction_path, text, &error)) {
      if (error_out) *error_out = error;
      return false;
    }
    st.board.raw_instruction = text;
    ed.dirty = false;
    return true;
  }
  if (st.board.editor_scope == BoardEditorScope::ContractSection) {
    const std::string text = cuwacunu::iinuji::primitives::editor_text(ed);
    board_contract_section_set_text(
        st,
        st.board.editing_contract_index,
        st.board.editing_contract_section,
        text);

    const std::string section_path = board_contract_section_instruction_path(
        st.board.editing_contract_section,
        st.board.instruction_path,
        st.board.contract_hash);
    std::string error;
    if (!write_text_file(section_path, text, &error)) {
      if (error_out) *error_out = error;
      return false;
    }
    ed.dirty = false;
    if (error_out) error_out->clear();
    return true;
  }

  if (ed.path.empty()) ed.path = st.board.instruction_path;
  std::string save_error;
  if (!cuwacunu::iinuji::primitives::editor_save_file(ed, {}, &save_error)) {
    if (error_out) *error_out = save_error;
    return false;
  }
  std::string parse_error;
  if (!sync_board_from_editor(st, &parse_error)) {
    if (error_out) *error_out = parse_error;
    return false;
  }
  return true;
}

inline bool handle_board_editor_key(CmdState& st, int ch) {
  if (st.screen != ScreenMode::Board || !st.board.editor_focus || !st.board.editor) return false;
  auto& ed = *st.board.editor;

  auto set_status = [&](std::string s) {
    ed.status = std::move(s);
    return true;
  };

  auto close_editor = [&]() {
    st.board.exit_prompt = BoardState::ExitPrompt::None;
    st.board.exit_prompt_index = 0;
    clear_board_completion(st);
    st.board.editor_focus = false;
    st.board.editor_scope = BoardEditorScope::None;
    st.board.panel_focus =
        (st.board.display_mode == BoardDisplayMode::ContractTextEdit)
            ? BoardPanelFocus::ContractSections
            : BoardPanelFocus::ViewOptions;
    return true;
  };

  auto reload_editor_from_disk_or_state = [&]() {
    if (st.board.editor_scope == BoardEditorScope::ContractVirtual) {
      if (!board_has_circuits(st)) {
        cuwacunu::iinuji::primitives::editor_set_text(ed, std::string());
        ed.dirty = false;
        return std::string("discarded + contract unavailable");
      }
      clamp_board_navigation_state(st);
      const std::size_t idx = std::min(
          st.board.editing_contract_index,
          st.board.board.contracts.empty() ? std::size_t(0) : (st.board.board.contracts.size() - 1));
      cuwacunu::iinuji::primitives::editor_set_text(
          ed,
          render_board_circuit_instruction_text(st.board.board.contracts[idx]));
      ed.dirty = false;
      return std::string("discarded + reloaded contract");
    }

    if (st.board.editor_scope == BoardEditorScope::FullInstruction) {
      if (ed.path.empty()) ed.path = st.board.instruction_path;
      std::string load_error;
      if (cuwacunu::iinuji::primitives::editor_load_file(ed, {}, &load_error)) {
        st.board.raw_instruction = cuwacunu::iinuji::primitives::editor_text(ed);
        return std::string("discarded + reloaded (validation disabled)");
      }
      cuwacunu::iinuji::primitives::editor_set_text(ed, st.board.raw_instruction);
      ed.dirty = false;
      return std::string("discarded (kept in-memory contract text)");
    }
    if (st.board.editor_scope == BoardEditorScope::ContractSection) {
      const std::string section_path = board_contract_section_instruction_path(
          st.board.editing_contract_section,
          st.board.instruction_path,
          st.board.contract_hash);
      std::string loaded{};
      std::string load_error{};
      if (read_text_file_safe(section_path, &loaded, &load_error)) {
        board_contract_section_set_text(
            st,
            st.board.editing_contract_index,
            st.board.editing_contract_section,
            loaded);
        cuwacunu::iinuji::primitives::editor_set_text(ed, loaded);
        ed.dirty = false;
        return std::string("discarded + reloaded section");
      }
      cuwacunu::iinuji::primitives::editor_set_text(
          ed,
          board_contract_section_get_text(
              st,
              st.board.editing_contract_index,
              st.board.editing_contract_section));
      ed.dirty = false;
      return std::string("discarded (kept in-memory section)");
    }

    if (ed.path.empty()) ed.path = st.board.instruction_path;
    std::string load_error;
    if (cuwacunu::iinuji::primitives::editor_load_file(ed, {}, &load_error)) {
      std::string parse_error;
      const bool ok = sync_board_from_editor(st, &parse_error);
      return ok ? std::string("discarded + reloaded")
                : ("discarded + reloaded invalid: " + parse_error);
    }

    cuwacunu::iinuji::primitives::editor_set_text(ed, st.board.raw_instruction);
    ed.dirty = false;
    std::string parse_error;
    const bool ok = sync_board_from_editor(st, &parse_error);
    return ok ? std::string("discarded (kept last applied board)")
              : ("discarded fallback invalid: " + parse_error);
  };

  auto accept_completion = [&]() -> bool {
    if (!st.board.completion_active || st.board.completion_items.empty()) return false;
    const int selected =
        std::clamp(static_cast<int>(st.board.completion_index), 0,
                   static_cast<int>(st.board.completion_items.size()) - 1);
    int start = st.board.completion_start_col;
    if (start < 0 || start > ed.cursor_col) {
      const auto pref = cuwacunu::iinuji::primitives::editor_token_prefix_at_cursor(ed);
      if (!pref.has_value()) return false;
      start = pref->first;
    }
    std::string& line = ed.lines[static_cast<std::size_t>(ed.cursor_line)];
    const int end = std::clamp(ed.cursor_col, start, static_cast<int>(line.size()));
    line.replace(static_cast<std::size_t>(start),
                 static_cast<std::size_t>(end - start),
                 st.board.completion_items[static_cast<std::size_t>(selected)]);
    ed.cursor_col =
        start + static_cast<int>(st.board.completion_items[static_cast<std::size_t>(selected)].size());
    ed.preferred_col = ed.cursor_col;
    ed.dirty = true;
    cuwacunu::iinuji::primitives::editor_ensure_cursor_visible(ed);
    clear_board_completion(st);
    return true;
  };

  if (st.board.exit_prompt == BoardState::ExitPrompt::SaveDiscardCancel) {
    auto apply_exit_choice = [&](int idx) {
      switch (std::clamp(idx, 0, 2)) {
        case 0: {
          std::string save_error;
          if (!persist_board_editor(st, &save_error)) {
            st.board.exit_prompt = BoardState::ExitPrompt::None;
            st.board.exit_prompt_index = 0;
            return set_status("save failed: " + save_error);
          }
          close_editor();
          return set_status("saved + exited");
        }
        case 1: {
          const std::string status = reload_editor_from_disk_or_state();
          close_editor();
          return set_status(status);
        }
        case 2:
        default:
          st.board.exit_prompt = BoardState::ExitPrompt::None;
          st.board.exit_prompt_index = 0;
          return set_status("continue editing");
      }
    };

    switch (ch) {
      case KEY_LEFT:
      case KEY_UP:
        st.board.exit_prompt_index = (st.board.exit_prompt_index + 2) % 3;
        return set_status("save prompt");
      case KEY_RIGHT:
      case KEY_DOWN:
        st.board.exit_prompt_index = (st.board.exit_prompt_index + 1) % 3;
        return set_status("save prompt");
      case KEY_ENTER:
      case '\n':
      case '\r':
        return apply_exit_choice(st.board.exit_prompt_index);
      case 27:
        return apply_exit_choice(2);
      default:
        return true;
    }
  }

  const bool keep_completion_key =
      (ch == '\t' || ch == KEY_ENTER || ch == '\n' || ch == '\r' || ch == 27);
  if (!keep_completion_key && st.board.completion_active) {
    clear_board_completion(st);
  }

  switch (ch) {
    case 27:
      clear_board_completion(st);
      if (ed.dirty) {
        st.board.exit_prompt = BoardState::ExitPrompt::SaveDiscardCancel;
        st.board.exit_prompt_index = 0;
        ed.status = "unsaved changes";
        return true;
      }
      st.board.exit_prompt = BoardState::ExitPrompt::None;
      st.board.exit_prompt_index = 0;
      st.board.editor_focus = false;
      st.board.editor_scope = BoardEditorScope::None;
      st.board.panel_focus =
          (st.board.display_mode == BoardDisplayMode::ContractTextEdit)
              ? BoardPanelFocus::ContractSections
              : BoardPanelFocus::ViewOptions;
      ed.status = "command mode";
      return true;
    case KEY_UP:
      cuwacunu::iinuji::primitives::editor_move_up(ed);
      return true;
    case KEY_DOWN:
      cuwacunu::iinuji::primitives::editor_move_down(ed);
      return true;
    case KEY_LEFT:
      cuwacunu::iinuji::primitives::editor_move_left(ed);
      return true;
    case KEY_RIGHT:
      cuwacunu::iinuji::primitives::editor_move_right(ed);
      return true;
    case KEY_HOME:
      cuwacunu::iinuji::primitives::editor_move_home(ed);
      return true;
    case KEY_END:
      cuwacunu::iinuji::primitives::editor_move_end(ed);
      return true;
    case KEY_PPAGE:
      cuwacunu::iinuji::primitives::editor_page_up(ed);
      return true;
    case KEY_NPAGE:
      cuwacunu::iinuji::primitives::editor_page_down(ed);
      return true;
    case KEY_BACKSPACE:
    case 127:
    case 8:
      cuwacunu::iinuji::primitives::editor_backspace(ed);
      return true;
    case KEY_DC:
      cuwacunu::iinuji::primitives::editor_delete(ed);
      return true;
    case KEY_ENTER:
    case '\n':
    case '\r':
      if (accept_completion()) return set_status("completion accepted");
      cuwacunu::iinuji::primitives::editor_insert_newline(ed);
      clear_board_completion(st);
      return true;
    case '\t': {
      if (st.board.completion_active && !st.board.completion_items.empty()) {
        const std::size_t n = st.board.completion_items.size();
        st.board.completion_index = (st.board.completion_index + 1u) % n;
        st.board.completion_active = true;
        return set_status(
            "completion " +
            std::to_string(st.board.completion_index + 1u) +
            "/" +
            std::to_string(n) +
            " (Enter=accept)");
      }

      const auto pref = cuwacunu::iinuji::primitives::editor_token_prefix_at_cursor(ed);
      if (!pref.has_value()) {
        const int tw = std::max(1, ed.tab_width);
        cuwacunu::iinuji::primitives::editor_insert_text(ed, std::string(static_cast<std::size_t>(tw), ' '));
        clear_board_completion(st);
        return set_status("indent");
      }

      if (!board_completion_allowed_at_cursor(st, ed)) {
        clear_board_completion(st);
        if (st.board.editor_scope == BoardEditorScope::FullInstruction ||
            st.board.editor_scope == BoardEditorScope::ContractSection) {
          const int tw = std::max(1, ed.tab_width);
          cuwacunu::iinuji::primitives::editor_insert_text(ed, std::string(static_cast<std::size_t>(tw), ' '));
          return set_status("completion disabled in contract edit mode");
        }
        return set_status("completion disabled outside selected contract DSL");
      }

      const auto matches = board_candidates_for_context(st, ed, pref->second, pref->first);
      if (matches.empty()) {
        const int tw = std::max(1, ed.tab_width);
        cuwacunu::iinuji::primitives::editor_insert_text(ed, std::string(static_cast<std::size_t>(tw), ' '));
        clear_board_completion(st);
        return set_status("no completion");
      }

      st.board.completion_items = matches;
      st.board.completion_active = true;
      st.board.completion_index = 0;
      st.board.completion_line = ed.cursor_line;
      st.board.completion_start_col = pref->first;
      return set_status(
          "completion 1/" + std::to_string(matches.size()) + " (Tab=next, Enter=accept)");
    }
    case 1:
      cuwacunu::iinuji::primitives::editor_move_home(ed);
      return true;
    case 5:
      cuwacunu::iinuji::primitives::editor_move_end(ed);
      return true;
    case 4:
      cuwacunu::iinuji::primitives::editor_delete(ed);
      return true;
    case 11:
      cuwacunu::iinuji::primitives::editor_delete_to_eol(ed);
      return true;
    case 23:
      cuwacunu::iinuji::primitives::editor_delete_prev_word(ed);
      return true;
    case 12: {
      const std::string status = reload_editor_from_disk_or_state();
      clear_board_completion(st);
      st.board.exit_prompt = BoardState::ExitPrompt::None;
      return set_status(status);
    }
    case 18: {
      if (st.board.editor_scope == BoardEditorScope::FullInstruction ||
          st.board.editor_scope == BoardEditorScope::ContractSection) {
        st.board.exit_prompt = BoardState::ExitPrompt::None;
        return set_status("validation disabled in contract edit mode");
      }
      std::string error;
      const bool ok = sync_board_from_editor(st, &error);
      st.board.exit_prompt = BoardState::ExitPrompt::None;
      if (ok && st.board.editor_scope == BoardEditorScope::ContractVirtual) {
        return set_status("valid (merged in memory, Ctrl+S to persist)");
      }
      return set_status(ok ? "valid" : ("invalid: " + error));
    }
    case 19: {
      std::string error;
      if (!persist_board_editor(st, &error)) {
        return set_status("save failed: " + error);
      }
      clear_board_completion(st);
      st.board.exit_prompt = BoardState::ExitPrompt::None;
      if (st.board.editor_scope == BoardEditorScope::FullInstruction ||
          st.board.editor_scope == BoardEditorScope::ContractSection) {
        return set_status("saved (validation disabled)");
      }
      return set_status("saved + valid");
    }
    default:
      break;
  }

  if (ch >= 32 && ch <= 126) {
    cuwacunu::iinuji::primitives::editor_insert_char(ed, static_cast<char>(ch));
    clear_board_completion(st);
    ed.status = "editing";
    return true;
  }

  return true;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
