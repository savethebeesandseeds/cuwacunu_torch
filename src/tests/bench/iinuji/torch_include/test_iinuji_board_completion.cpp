#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "iinuji/iinuji_cmd/views/board/completion.h"

namespace {

bool require(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "[FAIL] " << msg << "\n";
    return false;
  }
  return true;
}

bool contains(const std::vector<std::string>& values, const std::string& item) {
  for (const auto& v : values) {
    if (v == item) return true;
  }
  return false;
}

}  // namespace

int main() {
  using namespace cuwacunu::iinuji::iinuji_cmd;

  CmdState st{};
  st.board.selected_circuit = 0;
  st.board.editor = std::make_shared<cuwacunu::iinuji::editorBox_data_t>();
  auto& ed = *st.board.editor;
  ed.lines = {
      "circuit_1 = {",
      "  w_source = tsi.source.dataloader",
      "  w_rep = tsi.wikimyei.representation.vicreg",
      "  w_log = tsi.sink.log.sys",
      "  w_source@pay -> w_rep",
      "  w_source@payload:tensor -> w_rep@",
      "  w_log = tsi.s",
      "}",
      "circuit_1(\"BTCUSDT[01.01.2009,31.12.2009]\");",
      "",
      "circuit_2 = {",
      "  s_source = tsi.source.dataloader",
      "  s_sink = tsi.sink.null",
      "  s_source@payload:tensor -> s_sink@step",
      "}",
      "circuit_2(\"ETHUSDT[01.01.2010,31.12.2010]\");",
  };

  bool ok = true;

  ed.cursor_line = 4;
  ed.cursor_col = static_cast<int>(ed.lines[4].find("@pay")) + 4;
  const int lhs_token_start = static_cast<int>(ed.lines[4].find("@pay"));
  const auto lhs_matches = board_candidates_for_context(st, ed, "@pay", lhs_token_start);
  ok = ok && require(contains(lhs_matches, "@payload"), "lhs directive completion should include @payload");
  ok = ok && require(!contains(lhs_matches, "@step"), "lhs directive completion should not include @step for source outputs");

  ed.cursor_line = 5;
  ed.cursor_col = static_cast<int>(ed.lines[5].size());
  const int rhs_token_start = static_cast<int>(ed.lines[5].rfind('@'));
  const auto rhs_matches = board_candidates_for_context(st, ed, "@", rhs_token_start);
  ok = ok && require(contains(rhs_matches, "@step"), "rhs directive completion should include w_rep input @step");
  ok = ok && require(!contains(rhs_matches, "@payload"), "rhs directive completion should not suggest @payload for w_rep input");

  ed.cursor_line = 6;
  ed.cursor_col = static_cast<int>(ed.lines[6].size());
  const int type_token_start = static_cast<int>(ed.lines[6].find("tsi.s"));
  const auto type_matches = board_candidates_for_context(st, ed, "tsi.s", type_token_start);
  ok = ok && require(contains(type_matches, "tsi.sink.log.sys"), "type completion should include tsi.sink.log.sys");

  ed.cursor_line = 5;
  ok = ok && require(board_completion_allowed_at_cursor(st, ed),
                     "completion should be enabled inside selected contract DSL");
  ed.cursor_line = 10;
  ok = ok && require(!board_completion_allowed_at_cursor(st, ed),
                     "completion should be disabled outside selected contract DSL");
  st.board.editor_scope = BoardEditorScope::ContractVirtual;
  ok = ok && require(board_completion_allowed_at_cursor(st, ed),
                     "virtual contract editor should allow completion in its DSL buffer");
  st.board.editor_scope = BoardEditorScope::FullInstruction;
  ok = ok && require(!board_completion_allowed_at_cursor(st, ed),
                     "full contract editor should disable completion");
  st.board.editor_scope = BoardEditorScope::ContractSection;
  ok = ok && require(!board_completion_allowed_at_cursor(st, ed),
                     "contract section editor should disable completion by default");
  st.board.editor_scope = BoardEditorScope::None;

  st.board.completion_active = true;
  st.board.completion_items = {"one", "two"};
  st.board.completion_index = 1;
  st.board.completion_line = 5;
  st.board.completion_start_col = 10;
  clear_board_completion(st);
  ok = ok && require(!st.board.completion_active, "clear_board_completion should disable completion");
  ok = ok && require(st.board.completion_items.empty(), "clear_board_completion should clear items");
  ok = ok && require(st.board.completion_index == 0, "clear_board_completion should reset completion index");
  ok = ok && require(st.board.completion_line == -1, "clear_board_completion should reset completion line");
  ok = ok && require(st.board.completion_start_col == -1, "clear_board_completion should reset completion start");

  if (!ok) return 1;
  std::cout << "[ok] board completion context test passed\n";
  return 0;
}
