#pragma once

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {
inline std::vector<std::size_t> collect_tsi_occurrences(const CmdState& st,
                                                        std::string_view type_name,
                                                        std::vector<std::vector<std::string>>* aliases_by_circuit = nullptr) {
  std::vector<std::size_t> counts;
  if (!st.board.ok) return counts;

  counts.resize(st.board.board.circuits.size(), 0);
  if (aliases_by_circuit) aliases_by_circuit->assign(st.board.board.circuits.size(), {});

  for (std::size_t ci = 0; ci < st.board.board.circuits.size(); ++ci) {
    const auto& c = st.board.board.circuits[ci];
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

inline std::string make_tsi_left(const CmdState& st) {
  const auto& docs = tsi_node_docs();
  if (docs.empty()) return "No TSI nodes registered.";

  const std::size_t tab = clamp_tsi_tab_index(st.tsiemene.selected_tab);
  const auto& d = docs[tab];

  std::ostringstream oss;
  oss << "TSI Node " << (tab + 1) << "/" << docs.size() << "\n";
  oss << "id:          " << d.id << "\n";
  oss << "type:        " << d.type_name << "\n";
  oss << "role:        " << d.role << "\n";
  oss << "determinism: " << d.determinism << "\n";
  oss << "notes:       " << d.notes << "\n";

  oss << "\nDirectives (" << d.directives.size() << ")\n";
  for (const auto& dd : d.directives) {
    oss << "  - " << dir_token(dd.dir) << " "
        << dd.directive << dd.kind
        << "  " << dd.description << "\n";
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
    const auto& c = st.board.board.circuits[ci];
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
  std::ostringstream oss;
  oss << "TSI Tabs\n";
  if (docs.empty()) {
    oss << "  (none)\n";
  } else {
    for (std::size_t i = 0; i < docs.size(); ++i) {
      const bool active = (i == clamp_tsi_tab_index(st.tsiemene.selected_tab));
      std::size_t total = 0;
      if (st.board.ok) {
        const auto counts = collect_tsi_occurrences(st, docs[i].type_name);
        for (const auto v : counts) total += v;
      }
      std::ostringstream row;
      row << "  " << (active ? ">" : " ") << "[" << (i + 1) << "] "
          << docs[i].id << "  occ=" << total;
      if (active) oss << mark_selected_line(row.str()) << "\n";
      else oss << row.str() << "\n";
    }
  }

  oss << "\nCanonical directives\n";
  oss << "  @payload :str/:tensor\n";
  oss << "  @loss    :tensor\n";
  oss << "  @meta    :str\n";

  oss << "\nCommands\n";
  oss << "  iinuji.screen.tsi()\n";
  oss << "  iinuji.tsi.tabs()\n";
  oss << "  iinuji.tsi.tab.next()\n";
  oss << "  iinuji.tsi.tab.prev()\n";
  oss << "  iinuji.tsi.tab.index.n1()\n";
  oss << "  iinuji.tsi.tab.id.<token>()\n";
  oss << "  iinuji.show.tsi()\n";
  oss << "  aliases: tsi, f4\n";
  oss << "  primitive translation: disabled\n";

  oss << "\nKeys\n";
  oss << "  F4 : open tsiemene screen\n";
  oss << "  Left/Right : previous/next tsi tab\n";
  oss << "  wheel : vertical scroll both panels\n";
  oss << "  Shift/Ctrl/Alt+wheel : horizontal scroll both panels\n";
  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
