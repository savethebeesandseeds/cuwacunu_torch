#pragma once

#include <utility>
#include <vector>

#include "hashimyei/hashimyei_artifacts.h"
#include "iinuji/iinuji_cmd/views/common.h"
#include "tsiemene/tsi.wikimyei.representation.vicreg.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

[[nodiscard]] inline std::vector<cuwacunu::hashimyei::artifact_identity_t>
training_artifacts_for_tab_index(const CmdState& st, std::size_t tab_index) {
  const auto& docs = training_wikimyei_docs();
  if (docs.empty()) return {};
  const std::size_t tab = (tab_index < docs.size()) ? tab_index : 0;
  if (docs[tab].type_name == "tsi.wikimyei.representation.vicreg") {
    return tsiemene::list_wikimyei_representation_vicreg_artifacts();
  }
  return cuwacunu::hashimyei::discover_created_artifacts_for_type(docs[tab].type_name);
}

[[nodiscard]] inline std::vector<cuwacunu::hashimyei::artifact_identity_t>
training_artifacts_for_selected_tab(const CmdState& st) {
  return training_artifacts_for_tab_index(st, clamp_training_wikimyei_index(st.training.selected_tab));
}

[[nodiscard]] inline std::vector<std::string> training_hashes_for_selected_tab(const CmdState& st) {
  std::vector<std::string> out;
  const auto artifacts = training_artifacts_for_selected_tab(st);
  out.reserve(artifacts.size());
  for (const auto& item : artifacts) out.push_back(item.hashimyei);
  return out;
}

[[nodiscard]] inline std::vector<std::size_t> training_circuit_indices_for_type(const CmdState& st,
                                                                                 std::string_view type_name) {
  std::vector<std::size_t> out;
  if (!st.board.ok) return out;
  for (std::size_t ci = 0; ci < st.board.board.contracts.size(); ++ci) {
    const auto& c = st.board.board.contracts[ci];
    for (const auto& inst : c.instances) {
      if (std::string_view(inst.tsi_type) == type_name) {
        out.push_back(ci);
        break;
      }
    }
  }
  return out;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
