#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "iinuji/iinuji_cmd/views/common.h"
#include "iinuji/iinuji_cmd/views/tsiemene/commands.h"
#include "tsiemene/tsi.source.dataloader.h"
#include "tsiemene/tsi.wikimyei.representation.vicreg.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {
inline std::vector<std::size_t> collect_tsi_occurrences(
    const CmdState& st,
    std::string_view type_name,
    std::vector<std::vector<std::string>>* aliases_by_circuit = nullptr) {
  std::vector<std::size_t> counts;
  if (!st.board.ok) return counts;

  counts.resize(st.board.board.contracts.size(), 0);
  if (aliases_by_circuit) aliases_by_circuit->assign(st.board.board.contracts.size(), {});

  for (std::size_t ci = 0; ci < st.board.board.contracts.size(); ++ci) {
    const auto& c = st.board.board.contracts[ci];
    for (const auto& inst : c.instances) {
      if (std::string_view(inst.tsi_type) == type_name) {
        ++counts[ci];
        if (aliases_by_circuit) (*aliases_by_circuit)[ci].push_back(inst.alias);
      }
    }
  }
  return counts;
}

inline std::string join_csv(const std::vector<std::string>& values) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) oss << ", ";
    oss << values[i];
  }
  return oss.str();
}

inline void append_tsi_form_row(std::ostringstream& oss,
                                std::size_t row,
                                std::size_t selected_row,
                                bool focus_view,
                                const std::string& text) {
  const bool current = (focus_view && row == selected_row);
  std::ostringstream line;
  line << (current ? " >" : "  ") << "[" << (row + 1) << "] " << text;
  oss << (current ? mark_selected_line(line.str()) : line.str()) << "\n";
}

inline std::vector<std::string> tsi_created_instances_for_family(const TsiNodeDoc& d) {
  std::vector<std::string> out;
  if (d.type_name == "tsi.source.dataloader") {
    const auto items = tsi_source_dataloader_instances();
    out.reserve(items.size());
    for (const auto& item : items) {
      out.push_back(d.type_name + "." + item.init_id);
    }
    return out;
  }
  if (d.type_name == "tsi.wikimyei.representation.vicreg") {
    const auto items = tsiemene::list_wikimyei_representation_vicreg_init_entries();
    out.reserve(items.size());
    for (const auto& item : items) {
      out.push_back(item.canonical_base);
    }
  }
  return out;
}

inline std::size_t selected_source_dataloader_index(const CmdState& st, std::size_t total) {
  if (total == 0) return 0;
  return (st.tsiemene.selected_source_dataloader < total) ? st.tsiemene.selected_source_dataloader : 0;
}

inline void append_tsi_dataloader_form(const CmdState& st, std::ostringstream& oss) {
  const std::string& contract_hash = st.board.contract_hash;
  if (contract_hash.empty()) {
    oss << "\nFamily form: tsi.source.dataloader\n";
    oss << "  contract: unavailable\n";
    return;
  }
  std::string sources_path =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)
          ->get<std::string>("DSL", "observation_sources_dsl_filename");
  std::string channels_path =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)
          ->get<std::string>("DSL", "observation_channels_dsl_filename");

  const std::string next_id = tsiemene::next_source_dataloader_init_id({});
  const auto items = tsi_source_dataloader_instances();
  const std::size_t selected_idx = selected_source_dataloader_index(st, items.size());
  const std::string selected_id = items.empty() ? std::string() : items[selected_idx].init_id;
  const std::string selected_canonical = selected_id.empty()
      ? std::string("<none>")
      : std::string("tsi.source.dataloader.") + selected_id;

  const std::size_t row_count = tsi_form_row_count_for_selected_tab(st);
  const bool view_focus = (st.tsiemene.panel_focus == TsiPanelFocus::View);
  const std::size_t selected_row =
      (row_count == 0 || st.tsiemene.view_cursor >= row_count)
      ? 0
      : st.tsiemene.view_cursor;

  oss << "\nFamily form: tsi.source.dataloader\n";
  oss << "  panel.focus: " << (view_focus ? "view" : "context")
      << "  row=" << ((row_count == 0) ? 0 : (selected_row + 1))
      << "/" << row_count << "\n";
  oss << "  selected.instance: " << selected_canonical << "\n";
  oss << "  state.source: contract_space_t"
      << "  created=" << items.size()
      << "  active.id=" << next_id << "\n";

  append_tsi_form_row(
      oss,
      static_cast<std::size_t>(TsiSourceDataloaderFormRow::Create),
      selected_row,
      view_focus,
      "refresh contract-backed dataloader state (" + next_id + ")  -> " +
          std::string(canonical_paths::kTsiDataloaderCreate));
  append_tsi_form_row(
      oss,
      static_cast<std::size_t>(TsiSourceDataloaderFormRow::SelectPrev),
      selected_row,
      view_focus,
      "select previous created instance");
  append_tsi_form_row(
      oss,
      static_cast<std::size_t>(TsiSourceDataloaderFormRow::SelectNext),
      selected_row,
      view_focus,
      "select next created instance");
  append_tsi_form_row(
      oss,
      static_cast<std::size_t>(TsiSourceDataloaderFormRow::EditSelected),
      selected_row,
      view_focus,
      "edit selected -> " +
          (selected_id.empty() ? std::string("<none>") : canonical_paths::build_tsi_dataloader_edit(selected_id)));
  append_tsi_form_row(
      oss,
      static_cast<std::size_t>(TsiSourceDataloaderFormRow::DeleteSelected),
      selected_row,
      view_focus,
      "delete selected (contract-backed no-op) -> " +
          (selected_id.empty() ? std::string("<none>")
                               : canonical_paths::build_tsi_dataloader_delete(selected_id)));
  append_tsi_form_row(
      oss,
      static_cast<std::size_t>(TsiSourceDataloaderFormRow::DslInstruments),
      selected_row,
      view_focus,
      "dsl.observation_sources: " + format_file_status(sources_path));
  append_tsi_form_row(
      oss,
      static_cast<std::size_t>(TsiSourceDataloaderFormRow::DslInputs),
      selected_row,
      view_focus,
      "dsl.observation_channels: " + format_file_status(channels_path));
  append_tsi_form_row(
      oss,
      static_cast<std::size_t>(TsiSourceDataloaderFormRow::StoreRoot),
      selected_row,
      view_focus,
      "persistence: disabled (contract-backed)");
}

inline std::string make_tsi_left(const CmdState& st) {
  const auto& docs = tsi_node_docs();
  if (docs.empty()) return "No TSI families registered.";

  const std::size_t tab = clamp_tsi_tab_index(st.tsiemene.selected_tab);
  const auto& d = docs[tab];
  const auto created = tsi_created_instances_for_family(d);

  std::ostringstream oss;
  oss << "TSI Family " << (tab + 1) << "/" << docs.size() << "\n";
  oss << "canonical:   " << d.type_name << "\n";
  oss << "family.id:   " << d.id << "\n";
  oss << "determinism: " << d.determinism << "\n";
  oss << "panel.focus: " << ((st.tsiemene.panel_focus == TsiPanelFocus::View) ? "view" : "context")
      << "  (Enter with empty cmd -> view, Esc -> context)\n";

  oss << "\nFamily summary\n";
  oss << "  role: " << d.role << "\n";
  oss << "  notes: " << d.notes << "\n";
  oss << "  directives: " << d.directives.size() << "\n";

  oss << "\nCreated instances (" << created.size() << ")\n";
  if (created.empty()) {
    oss << "  <empty>\n";
  } else {
    for (std::size_t i = 0; i < created.size(); ++i) {
      const bool selected =
          (d.type_name == "tsi.source.dataloader" &&
           i == selected_source_dataloader_index(st, created.size()));
      oss << "  " << (selected ? "*" : " ") << "[" << (i + 1) << "] " << created[i] << "\n";
    }
  }

  oss << "\nDirectives\n";
  for (const auto& dd : d.directives) {
    oss << "  - " << dir_token(dd.dir) << " "
        << dd.directive << dd.kind
        << "  " << dd.description << "\n";
  }

  if (tsi_selected_tab_supports_form_rows(st)) {
    append_tsi_dataloader_form(st, oss);
  }

  if (!st.board.ok) {
    oss << "\nBoard occurrences: board invalid (" << st.board.error << ")\n";
    return oss.str();
  }

  std::vector<std::vector<std::string>> aliases;
  const auto counts = collect_tsi_occurrences(st, d.type_name, &aliases);
  std::size_t total = 0;
  for (const auto v : counts) total += v;
  oss << "\nBoard occurrences: total=" << total << "\n";

  bool any = false;
  for (std::size_t ci = 0; ci < counts.size(); ++ci) {
    if (counts[ci] == 0) continue;
    any = true;
    const auto& c = st.board.board.contracts[ci];
    oss << "  - circuit[" << (ci + 1) << "] " << c.name
        << " count=" << counts[ci];
    if (!aliases[ci].empty()) oss << " aliases={" << join_csv(aliases[ci]) << "}";
    oss << "\n";
  }
  if (!any) oss << "  - none\n";

  return oss.str();
}

inline std::string make_tsi_right(const CmdState& st) {
  const auto& docs = tsi_node_docs();
  const std::size_t active_tab = clamp_tsi_tab_index(st.tsiemene.selected_tab);
  std::ostringstream oss;
  oss << "TSI Families (canonical)\n";
  oss << "  panel.focus="
      << ((st.tsiemene.panel_focus == TsiPanelFocus::View) ? "view" : "context")
      << "\n";
  if (docs.empty()) {
    oss << "  <empty>\n";
  } else {
    for (std::size_t i = 0; i < docs.size(); ++i) {
      const bool active = (i == active_tab);
      const bool context_focus = active && (st.tsiemene.panel_focus == TsiPanelFocus::Context);
      const auto created = tsi_created_instances_for_family(docs[i]);
      std::ostringstream row;
      row << "  " << (context_focus ? ">" : " ") << "[" << (i + 1) << "] "
          << docs[i].type_name << "  created=" << created.size();
      if (active) oss << mark_selected_line(row.str()) << "\n";
      else oss << row.str() << "\n";
    }
  }

  if (!docs.empty()) {
    const auto& active = docs[active_tab];
    const auto created = tsi_created_instances_for_family(active);
    oss << "\nSelected family summary\n";
    oss << "  canonical: " << active.type_name << "\n";
    oss << "  role: " << active.role << "\n";
    oss << "  created: " << created.size() << "\n";
    if (created.empty()) oss << "  instances: <empty>\n";
    else oss << "  instances: " << created.front()
             << ((created.size() > 1) ? " ..." : "") << "\n";
  }

  if (st.tsiemene.panel_focus == TsiPanelFocus::View) {
    const std::size_t row_count = tsi_form_row_count_for_selected_tab(st);
    const std::size_t selected_row =
        (row_count == 0 || st.tsiemene.view_cursor >= row_count)
        ? 0
        : st.tsiemene.view_cursor;
    oss << "\nView selection\n";
    if (row_count == 0) {
      oss << "  row: n/a (selected family has no editable form)\n";
    } else {
      oss << "  row: " << (selected_row + 1) << "/" << row_count << "\n";
    }
  }

  oss << "\nCanonical directives\n";
  oss << "  @payload :str/:tensor\n";
  oss << "  @loss    :tensor\n";
  oss << "  @meta    :str\n";

  oss << "\nCommands\n";
  static const auto kTsiCallCommands = [] {
    auto out = canonical_paths::call_texts_by_prefix({"iinuji.tsi."});
    const auto screen = canonical_paths::call_texts_by_prefix({"iinuji.screen.tsi("});
    const auto show = canonical_paths::call_texts_by_prefix({"iinuji.show.tsi("});
    out.insert(out.end(), screen.begin(), screen.end());
    out.insert(out.end(), show.begin(), show.end());
    return out;
  }();
  static const auto kTsiPatternCommands = canonical_paths::pattern_texts_by_prefix({"iinuji.tsi."});
  for (const auto cmd : kTsiCallCommands) oss << "  " << cmd << "\n";
  for (const auto cmd : kTsiPatternCommands) oss << "  " << cmd << "\n";
  oss << "  aliases: tsi, f4\n";
  oss << "  primitive translation: disabled\n";

  oss << "\nKeys\n";
  oss << "  F4 : open tsiemene screen\n";
  oss << "  F3 : switch to training-only wikimyei view\n";
  oss << "  Enter (empty cmd) : context -> view focus, then execute selected row\n";
  oss << "  Esc (empty cmd)   : view -> context focus\n";
  oss << "  Up/Down (context) : previous/next tsi family\n";
  oss << "  Up/Down (view)    : previous/next form row\n";
  oss << "  wheel : vertical scroll both panels\n";
  oss << "  Shift/Ctrl/Alt+wheel : horizontal scroll both panels\n";
  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
