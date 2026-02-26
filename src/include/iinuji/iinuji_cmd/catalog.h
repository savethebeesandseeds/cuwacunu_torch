#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "tsiemene/tsi.type.registry.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct TsiDirectiveDoc {
  tsiemene::DirectiveDir dir{};
  std::string directive{};
  std::string kind{};
  std::string description{};
};

struct TsiNodeDoc {
  std::string id{};
  std::string title{};
  std::string type_name{};
  std::string role{};
  std::string determinism{};
  std::string notes{};
  std::vector<TsiDirectiveDoc> directives{};
};

[[nodiscard]] inline std::string canonical_to_tsi_tab_id(std::string_view canonical) {
  constexpr std::string_view kWikimyeiPrefix = "tsi.wikimyei.";
  constexpr std::string_view kPrefix = "tsi.";
  if (canonical.size() >= kWikimyeiPrefix.size() &&
      canonical.substr(0, kWikimyeiPrefix.size()) == kWikimyeiPrefix) {
    canonical.remove_prefix(kWikimyeiPrefix.size());
  } else if (canonical.size() >= kPrefix.size() &&
             canonical.substr(0, kPrefix.size()) == kPrefix) {
    canonical.remove_prefix(kPrefix.size());
  }
  return std::string(canonical);
}

[[nodiscard]] inline std::string determinism_hint_for_type(tsiemene::TsiTypeId type_id) {
  // Keep this lightweight and centralized in one place; directive/type metadata
  // comes from tsi.paths.def via tsi.type.registry.h.
  if (type_id == tsiemene::TsiTypeId::SourceDataloader) return "SeededStochastic";
  return "Deterministic";
}

[[nodiscard]] inline std::vector<TsiDirectiveDoc> lane_docs_for_type(tsiemene::TsiTypeId type_id) {
  std::vector<TsiDirectiveDoc> out;
  for (const auto& lane : tsiemene::kTsiTypeLanes) {
    if (lane.type_id != type_id) continue;
    out.push_back(TsiDirectiveDoc{
        .dir = lane.lane.dir,
        .directive = std::string(lane.lane.id),
        .kind = std::string(tsiemene::kind_token(lane.lane.kind.kind)),
        .description = std::string(lane.lane.doc),
    });
  }
  return out;
}

[[nodiscard]] inline std::string notes_for_type(const tsiemene::TsiTypeDescriptor& d) {
  std::string notes = "domain=";
  notes += std::string(tsiemene::domain_token(d.domain));
  notes += ", instances=";
  notes += std::string(tsiemene::instance_policy_token(d.instance_policy));
  notes += " (from tsi.paths.def)";
  return notes;
}

inline const std::vector<TsiNodeDoc>& tsi_node_docs() {
  static const std::vector<TsiNodeDoc> docs = [] {
    std::vector<TsiNodeDoc> out;
    out.reserve(tsiemene::kTsiTypeRegistry.size());

    for (const auto& d : tsiemene::kTsiTypeRegistry) {
      const std::string tab_id = canonical_to_tsi_tab_id(d.canonical);
      out.push_back(TsiNodeDoc{
          .id = tab_id,
          .title = tab_id,
          .type_name = std::string(d.canonical),
          .role = std::string(d.summary),
          .determinism = determinism_hint_for_type(d.id),
          .notes = notes_for_type(d),
          .directives = lane_docs_for_type(d.id),
      });
    }
    return out;
  }();
  return docs;
}

inline std::size_t tsi_tab_count() {
  return tsi_node_docs().size();
}

inline std::size_t clamp_tsi_tab_index(std::size_t idx) {
  const std::size_t n = tsi_tab_count();
  if (n == 0) return 0;
  return (idx < n) ? idx : 0;
}

struct TrainingWikimyeiDoc {
  std::string id{};
  std::string type_name{};
  std::string role{};
  std::string payload_kind{};
  bool trainable_jkimyei{false};
  std::string notes{};
};

inline const std::vector<TrainingWikimyeiDoc>& training_wikimyei_docs() {
  static const std::vector<TrainingWikimyeiDoc> docs = {
      {
          "representation.vicreg",
          "tsi.wikimyei.representation.vicreg",
          "encodes batches into latent space and emits train-time loss",
          ":tensor",
          true,
          "Trainable wikimyei. Supports canonical @jkimyei and @weights endpoints.",
      },
  };
  return docs;
}

inline std::size_t training_wikimyei_count() {
  return training_wikimyei_docs().size();
}

inline std::size_t clamp_training_wikimyei_index(std::size_t idx) {
  const std::size_t n = training_wikimyei_count();
  if (n == 0) return 0;
  return (idx < n) ? idx : 0;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
