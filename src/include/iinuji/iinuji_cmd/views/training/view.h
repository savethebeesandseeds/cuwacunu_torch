#pragma once

#include "iinuji/iinuji_cmd/views/common.h"
#include "iinuji/iinuji_cmd/views/training/catalog.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

// UI-only helper: render a short comma-separated alias list for display.
inline std::string join_aliases_for_display(const std::vector<std::string>& values) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) oss << ", ";
    oss << values[i];
  }
  return oss.str();
}

inline std::vector<std::size_t> collect_training_occurrences(
    const CmdState& st,
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

inline std::string metadata_status_for_display(const cuwacunu::hashimyei::artifact_metadata_t& meta) {
  if (!meta.present) return "none";
  if (meta.decrypted) return "encrypted+decrypted";
  if (!meta.error.empty()) return "encrypted(error)";
  return "encrypted";
}

inline void split_training_doc_id(std::string_view id, std::string* out_family, std::string* out_model) {
  if (out_family) *out_family = "family";
  if (out_model) *out_model = "model";
  const std::size_t dot = id.find('.');
  if (dot == std::string::npos || dot + 1 >= id.size()) return;
  if (out_family) *out_family = std::string(id.substr(0, dot));
  if (out_model) *out_model = std::string(id.substr(dot + 1));
}

inline std::string make_training_left(const CmdState& st) {
  const auto& docs = training_wikimyei_docs();
  if (docs.empty()) return "No training wikimyei entries registered.";

  const std::size_t tab = clamp_training_wikimyei_index(st.training.selected_tab);
  const auto& d = docs[tab];
  const auto artifacts = training_artifacts_for_selected_tab(st);

  std::string family = d.id;
  std::string model = "model";
  split_training_doc_id(d.id, &family, &model);

  std::ostringstream oss;
  oss << "Training Wikimyei " << (tab + 1) << "/" << docs.size() << "\n";
  oss << "id:          " << d.id << "\n";
  oss << "type:        " << d.type_name << "\n";
  oss << "role:        " << d.role << "\n";
  oss << "notes:       " << d.notes << "\n";

  if (artifacts.empty()) {
    oss << "created hashimyei: none\n";
    oss << "store root:       " << cuwacunu::hashimyei::store_root().string() << "\n";
    oss << "\nCanonical template\n";
    const std::string base = d.type_name + ".<hashimyei>";
    oss << "  " << base << "\n";
    oss << "  " << base << "@payload" << d.payload_kind << "\n";
    oss << "  " << base << "@meta:str\n";
    if (d.trainable_jkimyei) {
      oss << "  " << base << "@jkimyei:tensor\n";
      oss << "  " << base << "@weights:tensor\n";
    } else {
      oss << "  jkimyei: n/a (non-trainable wikimyei)\n";
    }
  } else {
    const std::size_t hx = (st.training.selected_hash < artifacts.size()) ? st.training.selected_hash : 0;
    const auto& item = artifacts[hx];
    const std::string base = item.canonical_base;
    const std::string fused = "tsi.wikimyei." + family + "." + model + "_" + item.hashimyei;

    oss << "hashimyei:   " << item.hashimyei << "  [" << (hx + 1) << "/" << artifacts.size() << "]\n";
    oss << "\nCanonical identities\n";
    oss << "  " << base << "\n";
    oss << "  " << base << "@payload" << d.payload_kind << "\n";
    oss << "  " << base << "@meta:str\n";
    oss << "  " << fused << "@payload" << d.payload_kind << "\n";
    if (d.trainable_jkimyei) {
      oss << "  " << base << "@jkimyei:tensor\n";
      oss << "  " << base << "@weights:tensor\n";
      oss << "  " << fused << "@jkimyei:tensor\n";
      oss << "  " << fused << "@weights:tensor\n";
    } else {
      oss << "  jkimyei: n/a (non-trainable wikimyei)\n";
    }

    oss << "\nArtifact storage\n";
    oss << "  dir: " << item.directory.string() << "\n";
    oss << "  weights: " << item.weight_files.size() << "\n";
    for (const auto& wf : item.weight_files) {
      oss << "    - " << wf.filename().string() << "\n";
    }
    oss << "  metadata: " << metadata_status_for_display(item.metadata);
    if (!item.metadata.error.empty()) oss << " (" << item.metadata.error << ")";
    oss << "\n";
    if (item.metadata.decrypted && !item.metadata.text.empty()) {
      std::string preview = item.metadata.text;
      const std::size_t nl = preview.find('\n');
      if (nl != std::string::npos) preview = preview.substr(0, nl);
      if (preview.size() > 120) preview = preview.substr(0, 117) + "...";
      oss << "  metadata.preview: " << preview << "\n";
    }
  }

  if (!st.board.ok) {
    oss << "\nCircuit contexts: board invalid (" << st.board.error << ")\n";
    return oss.str();
  }

  std::vector<std::vector<std::string>> aliases;
  const auto counts = collect_training_occurrences(st, d.type_name, &aliases);
  const auto circuit_indices = training_circuit_indices_for_type(st, d.type_name);
  std::size_t total = 0;
  for (const auto v : counts) total += v;
  oss << "\nCircuit contexts: total occurrences=" << total
      << " circuits=" << circuit_indices.size() << "\n";
  if (circuit_indices.empty()) {
    oss << "  - none\n";
    return oss.str();
  }
  for (const std::size_t ci : circuit_indices) {
    const auto& c = st.board.board.circuits[ci];
    oss << "  - circuit[" << (ci + 1) << "] " << c.name
        << " count=" << counts[ci];
    if (!aliases[ci].empty()) oss << " aliases={" << join_aliases_for_display(aliases[ci]) << "}";
    oss << "\n";
  }
  return oss.str();
}

inline std::string make_training_right(const CmdState& st) {
  const auto& docs = training_wikimyei_docs();

  const std::size_t active_tab = clamp_training_wikimyei_index(st.training.selected_tab);
  const auto active_artifacts = training_artifacts_for_tab_index(st, active_tab);
  const std::size_t active_hash =
      active_artifacts.empty() ? 0 : ((st.training.selected_hash < active_artifacts.size()) ? st.training.selected_hash : 0);

  std::ostringstream oss;
  oss << "Training Wikimyei Tabs\n";
  if (docs.empty()) {
    oss << "  (none)\n";
  } else {
    for (std::size_t i = 0; i < docs.size(); ++i) {
      const bool active = (i == active_tab);
      std::size_t total = 0;
      if (st.board.ok) {
        const auto counts = collect_training_occurrences(st, docs[i].type_name);
        for (const auto v : counts) total += v;
      }
      const auto artifacts = training_artifacts_for_tab_index(st, i);
      std::ostringstream row;
      row << "  " << (active ? ">" : " ") << "[" << (i + 1) << "] "
          << docs[i].id << "  occ=" << total
          << " created=" << artifacts.size();
      if (active) oss << mark_selected_line(row.str()) << "\n";
      else oss << row.str() << "\n";
    }
  }

  oss << "\nCreated Hashimyei Artifacts\n";
  if (active_artifacts.empty()) {
    oss << "  (none created)\n";
    oss << "  root=" << cuwacunu::hashimyei::store_root().string() << "\n";
  } else {
    const std::size_t window = 11;
    std::size_t start = (active_hash > (window / 2)) ? (active_hash - (window / 2)) : 0;
    std::size_t end = std::min(active_artifacts.size(), start + window);
    if (end - start < window && end > window) start = end - window;

    for (std::size_t i = start; i < end; ++i) {
      const bool active = (i == active_hash);
      const auto& item = active_artifacts[i];
      std::ostringstream row;
      row << "  " << (active ? ">" : " ") << "[" << (i + 1) << "] " << item.hashimyei
          << " w=" << item.weight_files.size()
          << " meta=" << metadata_status_for_display(item.metadata);
      if (active) oss << mark_selected_line(row.str()) << "\n";
      else oss << row.str() << "\n";
    }
    if (active_artifacts.size() > window) {
      oss << "  ... total=" << active_artifacts.size()
          << " selected=" << (active_hash + 1) << "\n";
    }
  }

  oss << "\nTraining Circuits\n";
  if (docs.empty()) {
    oss << "  (none)\n";
  } else if (!st.board.ok) {
    oss << "  board invalid (" << st.board.error << ")\n";
  } else {
    const auto circuit_indices = training_circuit_indices_for_type(st, docs[active_tab].type_name);
    if (circuit_indices.empty()) {
      oss << "  (none)\n";
    } else {
      for (const std::size_t ci : circuit_indices) {
        const auto& c = st.board.board.circuits[ci];
        oss << "  [" << (ci + 1) << "] " << c.name << "\n";
      }
    }
  }

  oss << "\nCommands\n";
  static const auto kTrainingCallCommands = [] {
    auto out = canonical_paths::call_texts_by_prefix({"iinuji.training."});
    const auto screen = canonical_paths::call_texts_by_prefix({"iinuji.screen.training("});
    const auto show = canonical_paths::call_texts_by_prefix({"iinuji.show.training("});
    out.insert(out.end(), screen.begin(), screen.end());
    out.insert(out.end(), show.begin(), show.end());
    return out;
  }();
  static const auto kTrainingPatternCommands = canonical_paths::pattern_texts_by_prefix({"iinuji.training."});
  for (const auto cmd : kTrainingCallCommands) oss << "  " << cmd << "\n";
  for (const auto cmd : kTrainingPatternCommands) oss << "  " << cmd << "\n";
  oss << "  aliases: training, f3\n";

  oss << "\nKeys\n";
  oss << "  F3 : open training screen\n";
  oss << "  Left/Right : previous/next wikimyei\n";
  oss << "  Up/Down : previous/next created hashimyei\n";
  oss << "  F4 : switch to full tsi elements (waves/sources/sinks)\n";
  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
