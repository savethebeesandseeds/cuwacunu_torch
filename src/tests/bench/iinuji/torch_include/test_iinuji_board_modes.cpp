// test_iinuji_board_modes.cpp
#include <iostream>
#include <string>

#include "iinuji/iinuji_cmd/views/board/app.h"

namespace {

bool require(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "[FAIL] " << msg << "\n";
    return false;
  }
  return true;
}

}  // namespace

int main() {
  using namespace cuwacunu::iinuji::iinuji_cmd;

  CmdState st{};
  st.screen = ScreenMode::Board;
  st.board.instruction_path = "/tmp/test_iinuji_board_modes.dsl";
  st.board.editor = std::make_shared<cuwacunu::iinuji::editorBox_data_t>();

  const std::string board_text =
      "circuit_1 = {\n"
      "  w_source = tsi.source.dataloader\n"
      "  w_rep = tsi.wikimyei.representation.vicreg\n"
      "  w_sink = tsi.sink.null\n"
      "  w_log = tsi.sink.log.sys\n"
      "  w_source@payload:tensor -> w_rep@step\n"
      "  w_rep@payload:tensor -> w_sink@step\n"
      "  w_rep@loss:tensor -> w_log@info\n"
      "}\n"
      "circuit_1(BTCUSDT[01.01.2009,31.12.2009]);\n"
      "\n"
      "circuit_2 = {\n"
      "  s_source = tsi.source.dataloader\n"
      "  s_sink = tsi.sink.null\n"
      "  s_log = tsi.sink.log.sys\n"
      "  s_source@payload:tensor -> s_sink@step\n"
      "  s_source@meta:str -> s_log@warn\n"
      "}\n"
      "circuit_2(ETHUSDT[01.01.2010,31.12.2010]);\n";

  std::string load_error;
  bool ok = apply_board_instruction_text(st, board_text, &load_error);
  ok = ok && require(ok, "board text should decode");
  ok = ok && require(st.board.board.circuits.size() == 2, "expected two contracts");
  const std::string initial_payload = st.board.board.circuits[0].invoke_payload;
  clamp_board_navigation_state(st);

  ok = ok && require(st.board.panel_focus == BoardPanelFocus::Context,
                     "board should start in context focus");

  ok = ok && require(handle_board_navigation_key(st, '\n', true),
                     "Enter should transition context -> view options");
  ok = ok && require(st.board.panel_focus == BoardPanelFocus::ViewOptions,
                     "panel focus should be view options");

  st.board.selected_view_option = 0;
  ok = ok && require(handle_board_navigation_key(st, KEY_DOWN, true),
                     "Down should move to next option");
  ok = ok && require(st.board.selected_view_option == 1, "selected option should be row 2");
  ok = ok && require(handle_board_navigation_key(st, KEY_DOWN, true),
                     "Down should wrap to first option");
  ok = ok && require(st.board.selected_view_option == 0, "selected option should wrap to row 1");

  st.board.selected_view_option = 0;
  ok = ok && require(handle_board_navigation_key(st, '\n', true),
                     "Enter should select diagram mode");
  ok = ok && require(st.board.display_mode == BoardDisplayMode::Diagram,
                     "display mode should be diagram");
  ok = ok && require(!st.board.editor_focus, "diagram mode should not open editor");

  st.board.selected_view_option = 1;
  ok = ok && require(handle_board_navigation_key(st, '\n', true),
                     "Enter should select contract edit mode");
  ok = ok && require(st.board.display_mode == BoardDisplayMode::ContractTextEdit,
                     "display mode should be contract edit");
  ok = ok && require(!st.board.editor_focus, "contract edit mode should start in section navigation");
  ok = ok && require(st.board.panel_focus == BoardPanelFocus::ContractSections,
                     "contract edit mode should focus contract sections");
  ok = ok && require(st.board.selected_contract_section == 0,
                     "default selected contract section should be circuit");

  ok = ok && require(handle_board_navigation_key(st, '\n', true),
                     "Enter in section navigation should open selected section editor");
  ok = ok && require(st.board.editor_focus, "section editor should be opened");
  ok = ok && require(st.board.editor_scope == BoardEditorScope::ContractSection,
                     "editor scope should be contract section");
  ok = ok && require(st.board.editing_contract_index == st.board.selected_circuit,
                     "editing contract index should match selected contract");
  ok = ok && require(st.board.editing_contract_section == BoardContractSection::Circuit,
                     "default edited section should be circuit");

  auto& ed = *st.board.editor;
  ok = ok && require(
      cuwacunu::iinuji::primitives::editor_text(ed).find("BEGIN board.contract.circuit@DSL:str") ==
          std::string::npos,
      "section editor should load section DSL text only");
  bool replaced_symbol = false;
  for (std::string& line : ed.lines) {
    const std::size_t at = line.find("BTCUSDT");
    if (at == std::string::npos) continue;
    line.replace(at, std::string("BTCUSDT").size(), "SOLUSDT");
    replaced_symbol = true;
    break;
  }
  ok = ok && require(replaced_symbol, "expected to find BTCUSDT in contract editor text");
  ed.dirty = true;
  ok = ok && require(handle_board_editor_key(st, 18), "Ctrl+R should be handled");
  ok = ok && require(ed.status.find("disabled") != std::string::npos,
                     "Ctrl+R should be disabled in contract section edit mode");
  ok = ok && require(handle_board_editor_key(st, 19), "Ctrl+S should be handled");
  std::string saved_text;
  std::string saved_error;
  ok = ok && require(
      read_text_file_safe(st.board.instruction_path, &saved_text, &saved_error),
      "saved section should be persisted to the instruction file path");
  ok = ok && require(saved_text.find("SOLUSDT") != std::string::npos,
                     "saved section text should contain SOLUSDT");
  ok = ok && require(st.board.board.circuits[0].invoke_payload == initial_payload,
                     "contract section edit should not recompile/merge runtime board");

  const std::string board_before_invalid_save = saved_text;
  cuwacunu::iinuji::primitives::editor_set_text(ed, "not a contract");
  ed.dirty = true;
  std::string save_error;
  const bool saved_invalid = persist_board_editor(st, &save_error);
  ok = ok && require(saved_invalid, "section save should bypass validation");
  std::string saved_invalid_text;
  std::string saved_invalid_error;
  ok = ok && require(
      read_text_file_safe(st.board.instruction_path, &saved_invalid_text, &saved_invalid_error),
      "invalid section save should still write text");
  ok = ok && require(saved_invalid_text != board_before_invalid_save,
                     "invalid section save should mutate persisted section text");
  ok = ok && require(st.board.board.circuits[0].invoke_payload == initial_payload,
                     "invalid section save should keep previously compiled runtime board");

  ok = ok && require(!st.board.board.circuits.empty(),
                     "board should still contain contracts");
  cuwacunu::iinuji::primitives::editor_set_text(
      ed, saved_invalid_text);
  ed.dirty = false;
  ok = ok && require(handle_board_editor_key(st, 27), "Esc should close editor when clean");
  ok = ok && require(!st.board.editor_focus, "editor should be closed");
  ok = ok && require(st.board.panel_focus == BoardPanelFocus::ContractSections,
                     "closing section editor should return to contract sections");

  ok = ok && require(handle_board_navigation_key(st, 27, true),
                     "Esc in contract sections should return to view options");
  ok = ok && require(st.board.panel_focus == BoardPanelFocus::ViewOptions,
                     "panel focus should be view options");

  ok = ok && require(handle_board_navigation_key(st, 27, true),
                     "Esc in view options should return to context");
  ok = ok && require(st.board.panel_focus == BoardPanelFocus::Context,
                     "panel focus should be context");

  if (!ok) return 1;
  std::cout << "[ok] board modes/navigation test passed\n";
  return 0;
}
