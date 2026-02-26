#pragma once

#include <string>

#include "iinuji/iinuji_cmd/commands/iinuji.path.tokens.h"
#include "iinuji/iinuji_cmd/views/training/catalog.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void select_next_training_tab(CmdState& st) {
  const std::size_t n = training_wikimyei_count();
  if (n == 0) {
    st.training.selected_tab = 0;
    return;
  }
  st.training.selected_tab = (st.training.selected_tab + 1) % n;
}

inline void select_prev_training_tab(CmdState& st) {
  const std::size_t n = training_wikimyei_count();
  if (n == 0) {
    st.training.selected_tab = 0;
    return;
  }
  st.training.selected_tab = (st.training.selected_tab + n - 1) % n;
}

inline bool select_training_tab_by_token(CmdState& st, const std::string& token) {
  const std::size_t n = training_wikimyei_count();
  if (n == 0) return false;

  std::size_t idx1 = 0;
  if (parse_positive_index(token, &idx1)) {
    if (idx1 == 0 || idx1 > n) return false;
    st.training.selected_tab = idx1 - 1;
    return true;
  }

  const std::string needle = to_lower_copy(token);
  const auto& docs = training_wikimyei_docs();
  for (std::size_t i = 0; i < docs.size(); ++i) {
    if (to_lower_copy(docs[i].id) == needle ||
        to_lower_copy(docs[i].type_name) == needle ||
        canonical_path_tokens::token_matches(docs[i].id, token) ||
        canonical_path_tokens::token_matches(docs[i].type_name, token)) {
      st.training.selected_tab = i;
      return true;
    }
  }
  return false;
}

inline void select_next_training_hash(CmdState& st) {
  const auto hashes = training_hashes_for_selected_tab(st);
  const std::size_t n = hashes.size();
  if (n == 0) {
    st.training.selected_hash = 0;
    return;
  }
  st.training.selected_hash = (st.training.selected_hash + 1) % n;
}

inline void select_prev_training_hash(CmdState& st) {
  const auto hashes = training_hashes_for_selected_tab(st);
  const std::size_t n = hashes.size();
  if (n == 0) {
    st.training.selected_hash = 0;
    return;
  }
  st.training.selected_hash = (st.training.selected_hash + n - 1) % n;
}

inline bool select_training_hash_by_token(CmdState& st, const std::string& token) {
  const auto hashes = training_hashes_for_selected_tab(st);
  if (hashes.empty()) return false;

  std::size_t idx1 = 0;
  if (parse_positive_index(token, &idx1)) {
    if (idx1 == 0 || idx1 > hashes.size()) return false;
    st.training.selected_hash = idx1 - 1;
    return true;
  }

  const std::string needle = to_lower_copy(token);
  for (std::size_t i = 0; i < hashes.size(); ++i) {
    if (to_lower_copy(hashes[i]) == needle ||
        canonical_path_tokens::token_matches(hashes[i], token)) {
      st.training.selected_hash = i;
      return true;
    }
  }
  return false;
}

template <class PushWarn, class AppendLog>
inline bool handle_training_show(CmdState& st, PushWarn&& push_warn, AppendLog&& append_log) {
  const auto& docs = training_wikimyei_docs();
  const auto artifacts = training_artifacts_for_selected_tab(st);
  if (docs.empty()) {
    push_warn("no training wikimyei entries");
    return true;
  }
  if (artifacts.empty()) {
    push_warn("no created hashimyei artifacts for selected wikimyei");
    return true;
  }

  const std::size_t tab = clamp_training_wikimyei_index(st.training.selected_tab);
  const std::size_t hx = (st.training.selected_hash < artifacts.size()) ? st.training.selected_hash : 0;
  const auto& d = docs[tab];
  const auto& item = artifacts[hx];
  const auto& h = item.hashimyei;

  const std::string base = item.canonical_base;
  append_log("training.wikimyei=" + d.id, "show", "#d8d8ff");
  append_log("training.hashimyei=" + h, "show", "#d8d8ff");
  append_log("canonical=" + base, "show", "#d8d8ff");
  append_log("weights.files=" + std::to_string(item.weight_files.size()), "show", "#d8d8ff");
  if (item.metadata.present) {
    append_log(
        std::string("metadata=") + (item.metadata.decrypted ? "encrypted+decrypted" : "encrypted"),
        "show",
        "#d8d8ff");
  } else {
    append_log("metadata=none", "show", "#d8d8ff");
  }
  if (d.trainable_jkimyei) {
    append_log("jkimyei=" + base + "@jkimyei:tensor", "show", "#d8d8ff");
    append_log("weights=" + base + "@weights:tensor", "show", "#d8d8ff");
  }
  return true;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
