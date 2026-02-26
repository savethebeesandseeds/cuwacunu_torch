#pragma once

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "iinuji/iinuji_cmd/views/board/commands.h"
#include "iinuji/iinuji_cmd/views/board/contract.section.registry.h"
#include "iinuji/iinuji_cmd/views/board/view.circuit.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string board_display_mode_label(BoardDisplayMode mode) {
  switch (mode) {
    case BoardDisplayMode::Diagram:
      return "Contract Circuit Diagram";
    case BoardDisplayMode::ContractTextEdit:
      return "Contract Text (edit)";
  }
  return "Contract Circuit Diagram";
}

inline std::size_t board_text_line_count(std::string_view text) {
  if (text.empty()) return 0;
  std::size_t lines = 1;
  for (const char c : text) {
    if (c == '\n') ++lines;
  }
  return lines;
}

inline std::string make_contract_edit_section_picker(const CmdState& st,
                                                     std::size_t ci,
                                                     std::size_t total) {
  std::ostringstream oss;
  oss << "Contract " << (ci + 1) << "/" << total << "\n";
  oss << "display: Contract Text (edit)\n";
  oss << "mode: section navigation\n";
  oss << "Enter opens selected section editor at line 1.\n\n";
  oss << "Sections:\n";
  for (std::size_t i = 0; i < board_contract_section_row_count(); ++i) {
    const BoardContractSection section = board_contract_section_from_index(i);
    const bool selected = (i == st.board.selected_contract_section);
    const std::string text = board_contract_section_get_text(st, ci, section);
    oss << (selected ? " > " : "   ")
        << board_contract_section_key(section)
        << "  lines=" << board_text_line_count(text)
        << "\n";
  }
  return oss.str();
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_contract_edit_section_picker_styled_lines(const CmdState& st,
                                               std::size_t ci,
                                               std::size_t total) {
  using Emph = cuwacunu::iinuji::text_line_emphasis_t;
  std::vector<cuwacunu::iinuji::styled_text_line_t> lines{};
  auto push = [&](std::string text, Emph emphasis = Emph::None) {
    lines.push_back(cuwacunu::iinuji::styled_text_line_t{
        .text = std::move(text),
        .emphasis = emphasis,
    });
  };

  push("Contract " + std::to_string(ci + 1) + "/" + std::to_string(total), Emph::Success);
  push("display: Contract Text (edit)", Emph::Success);
  push("mode: section navigation", Emph::Success);
  push("Enter opens selected section editor at line 1.", Emph::Success);
  push("");

  push("Sections", Emph::Success);
  const bool focus_sections = (st.board.panel_focus == BoardPanelFocus::ContractSections);
  for (std::size_t i = 0; i < board_contract_section_row_count(); ++i) {
    const BoardContractSection section = board_contract_section_from_index(i);
    const bool selected = (i == st.board.selected_contract_section);
    const std::string text = board_contract_section_get_text(st, ci, section);
    std::string row = std::string(selected ? " > " : "   ") +
                      std::string(board_contract_section_key(section)) +
                      "  lines=" + std::to_string(board_text_line_count(text));
    if (focus_sections && selected) row = mark_selected_line(row);

    Emph emphasis = Emph::Success;
    if (selected) emphasis = focus_sections ? Emph::Accent : Emph::Success;
    push(std::move(row), emphasis);
  }
  return lines;
}

inline std::string make_board_left(const CmdState& st) {
  if (!st.board.ok) {
    std::ostringstream oss;
    oss << "Board instruction invalid.\n\n";
    oss << "error: " << st.board.error << "\n\n";
    oss << "raw instruction:\n" << st.board.raw_instruction << "\n";
    return oss.str();
  }
  if (st.board.board.circuits.empty()) {
    return "Board has no contracts.";
  }
  const std::size_t ci = std::min(st.board.selected_circuit, st.board.board.circuits.size() - 1);
  const auto& c = st.board.board.circuits[ci];
  static const std::vector<tsiemene_resolved_hop_t> kEmptyResolvedHops{};
  const auto& hops = (ci < st.board.resolved_hops.size()) ? st.board.resolved_hops[ci] : kEmptyResolvedHops;

  switch (st.board.display_mode) {
    case BoardDisplayMode::Diagram:
      return make_circuit_canvas(c, hops);
    case BoardDisplayMode::ContractTextEdit: {
      if (st.board.editor_focus) {
        return "contract edit mode";
      }
      return make_contract_edit_section_picker(st, ci, st.board.board.circuits.size());
    }
  }
  return make_circuit_canvas(c, hops);
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_board_left_contract_edit_styled_lines(const CmdState& st) {
  using Emph = cuwacunu::iinuji::text_line_emphasis_t;
  if (!st.board.ok) {
    return {
        cuwacunu::iinuji::styled_text_line_t{"Board instruction invalid.", Emph::Error},
        cuwacunu::iinuji::styled_text_line_t{"error: " + st.board.error, Emph::Error},
    };
  }
  if (st.board.board.circuits.empty()) {
    return {cuwacunu::iinuji::styled_text_line_t{"Board has no contracts.", Emph::Warning}};
  }
  const std::size_t ci = std::min(st.board.selected_circuit, st.board.board.circuits.size() - 1);
  return make_contract_edit_section_picker_styled_lines(st, ci, st.board.board.circuits.size());
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t> make_board_right_styled_lines(const CmdState& st) {
  using Emph = cuwacunu::iinuji::text_line_emphasis_t;
  std::vector<cuwacunu::iinuji::styled_text_line_t> lines{};
  auto push = [&](std::string text, Emph emphasis = Emph::None) {
    lines.push_back(cuwacunu::iinuji::styled_text_line_t{
        .text = std::move(text),
        .emphasis = emphasis,
    });
  };

  if (!st.board.ok) {
    push("Board instruction invalid", Emph::Error);
    push("error: " + st.board.error, Emph::Error);
    push("");
    push("Fix src/config/instructions/tsiemene_circuit.dsl then run: reload", Emph::Warning);
    return lines;
  }
  if (st.board.board.circuits.empty()) {
    push("No contracts.", Emph::Warning);
    return lines;
  }

  const std::size_t total = st.board.board.circuits.size();
  const std::size_t selected = std::min(st.board.selected_circuit, total - 1);
  const bool focus_context = (st.board.panel_focus == BoardPanelFocus::Context);
  const bool focus_view_options = (st.board.panel_focus == BoardPanelFocus::ViewOptions);
  const std::string focus_label = focus_context
                                      ? "context"
                                      : (focus_view_options ? "view-options" : "contract-sections");

  push("Board Contracts", Emph::Accent);
  push("focus: " + focus_label, Emph::Info);
  push("display: " + board_display_mode_label(st.board.display_mode), Emph::Info);
  push("");

  push("Contracts (" + std::to_string(total) + ")", Emph::Accent);
  for (std::size_t i = 0; i < total; ++i) {
    const auto& c = st.board.board.circuits[i];
    std::ostringstream row;
    row << (i == selected ? " > " : "   ")
        << "[" << (i + 1) << "] "
        << c.name
        << "  instances=" << c.instances.size()
        << " hops=" << c.hops.size();
    std::string text = row.str();
    if (focus_context && i == selected) {
      text = mark_selected_line(text);
      push(std::move(text), Emph::Accent);
    } else {
      push(std::move(text), Emph::None);
    }
  }
  push("");

  push("View Options", Emph::Accent);
  for (std::size_t i = 0; i < board_view_option_row_count(); ++i) {
    const bool selected_row = (i == st.board.selected_view_option);
    std::string row = std::string(selected_row ? " > " : "   ") + board_view_option_label(i);
    if (focus_view_options && selected_row) {
      row = mark_selected_line(row);
    }
    push(row, (focus_view_options && selected_row) ? Emph::Accent : Emph::Debug);
  }
  push("");

  const auto& c = st.board.board.circuits[selected];
  push("Selected Contract", Emph::Accent);
  push("name: " + c.name, Emph::Info);
  push("invoke: " + c.invoke_name + "(" + c.invoke_payload + ")", Emph::Debug);
  cuwacunu::camahjucunu::tsiemene_wave_invoke_t w{};
  std::string werr;
  if (cuwacunu::camahjucunu::parse_circuit_invoke_wave(c, &w, &werr)) {
    push("symbol: " + w.source_symbol, Emph::Info);
    push("source command: " + w.source_command, Emph::Debug);
    std::ostringstream wave_line;
    wave_line << "wave: episode=" << w.episode << " batch=" << w.batch << " i=" << w.wave_i;
    if (w.has_time_span) {
      wave_line << " span_ms=[" << w.span_begin_ms << "," << w.span_end_ms << "]";
    }
    push(wave_line.str(), Emph::Debug);
  } else if (!werr.empty()) {
    push("wave parse error: " + werr, Emph::Warning);
  }
  push("contract segments", Emph::Accent);
  push(" - board.contract.circuit@DSL:str : local/derived", Emph::Debug);
  push(
      std::string(" - board.contract.observation_sources@DSL:str : ") +
          (st.board.contract_observation_sources_dsl.empty() ? "missing" : "loaded"),
      st.board.contract_observation_sources_dsl.empty() ? Emph::Warning : Emph::Debug);
  push(
      std::string(" - board.contract.observation_channels@DSL:str : ") +
          (st.board.contract_observation_channels_dsl.empty() ? "missing" : "loaded"),
      st.board.contract_observation_channels_dsl.empty() ? Emph::Warning : Emph::Debug);
  push(
      std::string(" - board.contract.jkimyei_specs@DSL:str : ") +
          (st.board.contract_jkimyei_specs_dsl.empty() ? "missing" : "loaded"),
      st.board.contract_jkimyei_specs_dsl.empty() ? Emph::Warning : Emph::Debug);
  if (focus_context) {
    push("hint: Up/Down contract | Enter view options", Emph::Warning);
  } else if (focus_view_options) {
    push("hint: Up/Down option | Enter open | Esc context", Emph::Warning);
  } else {
    push("hint: Up/Down section | Enter edit section | Esc view options", Emph::Warning);
  }
  return lines;
}

inline std::string make_board_right(const CmdState& st) {
  const auto lines = make_board_right_styled_lines(st);
  std::ostringstream oss;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i > 0) oss << "\n";
    oss << lines[i].text;
  }
  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
