#pragma once

#include <string>
#include <vector>

#include "iinuji/iinuji_cmd/commands/iinuji.path.tokens.h"
#include "iinuji/iinuji_cmd/views/common.h"
#include "tsiemene/tsi.source.dataloader.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class TsiSourceDataloaderFormRow : std::size_t {
  Create = 0,
  SelectPrev = 1,
  SelectNext = 2,
  EditSelected = 3,
  DeleteSelected = 4,
  DslInstruments = 5,
  DslInputs = 6,
  StoreRoot = 7,
};

inline constexpr std::size_t tsi_source_dataloader_form_row_count() {
  return 8;
}

inline std::vector<tsiemene::source_dataloader_init_entry_t> tsi_source_dataloader_instances() {
  return tsiemene::list_source_dataloader_init_entries();
}

inline void clamp_tsi_source_dataloader_selection(CmdState& st) {
  const auto items = tsi_source_dataloader_instances();
  if (items.empty()) {
    st.tsiemene.selected_source_dataloader = 0;
    return;
  }
  if (st.tsiemene.selected_source_dataloader >= items.size()) {
    st.tsiemene.selected_source_dataloader = items.size() - 1;
  }
}

inline std::string selected_tsi_source_dataloader_id(const CmdState& st) {
  const auto items = tsi_source_dataloader_instances();
  if (items.empty()) return {};
  const std::size_t idx =
      (st.tsiemene.selected_source_dataloader < items.size()) ? st.tsiemene.selected_source_dataloader : 0;
  return items[idx].init_id;
}

inline bool select_tsi_source_dataloader_by_id(CmdState& st, std::string_view init_id) {
  const auto items = tsi_source_dataloader_instances();
  for (std::size_t i = 0; i < items.size(); ++i) {
    if (items[i].init_id == init_id) {
      st.tsiemene.selected_source_dataloader = i;
      return true;
    }
  }
  if (items.empty()) st.tsiemene.selected_source_dataloader = 0;
  return false;
}

inline bool select_prev_tsi_source_dataloader(CmdState& st) {
  const auto items = tsi_source_dataloader_instances();
  if (items.empty()) {
    st.tsiemene.selected_source_dataloader = 0;
    return false;
  }
  st.tsiemene.selected_source_dataloader =
      (st.tsiemene.selected_source_dataloader + items.size() - 1u) % items.size();
  return true;
}

inline bool select_next_tsi_source_dataloader(CmdState& st) {
  const auto items = tsi_source_dataloader_instances();
  if (items.empty()) {
    st.tsiemene.selected_source_dataloader = 0;
    return false;
  }
  st.tsiemene.selected_source_dataloader =
      (st.tsiemene.selected_source_dataloader + 1u) % items.size();
  return true;
}

inline bool tsi_tab_supports_form_rows(const CmdState& st, std::size_t tab) {
  const auto& docs = tsi_node_docs();
  if (docs.empty()) return false;
  const std::size_t idx = clamp_tsi_tab_index(tab);
  return docs[idx].type_name == "tsi.source.dataloader";
}

inline bool tsi_selected_tab_supports_form_rows(const CmdState& st) {
  return tsi_tab_supports_form_rows(st, st.tsiemene.selected_tab);
}

inline std::size_t tsi_form_row_count_for_selected_tab(const CmdState& st) {
  if (!tsi_selected_tab_supports_form_rows(st)) return 0;
  return tsi_source_dataloader_form_row_count();
}

inline void clamp_tsi_view_cursor(CmdState& st) {
  const std::size_t n = tsi_form_row_count_for_selected_tab(st);
  if (n == 0) {
    st.tsiemene.view_cursor = 0;
    return;
  }
  if (st.tsiemene.view_cursor >= n) st.tsiemene.view_cursor = n - 1;
}

inline void clamp_tsi_navigation_state(CmdState& st) {
  const std::size_t n = tsi_tab_count();
  if (n == 0) {
    st.tsiemene.selected_tab = 0;
    st.tsiemene.panel_focus = TsiPanelFocus::Context;
    st.tsiemene.view_cursor = 0;
    st.tsiemene.selected_source_dataloader = 0;
    return;
  }

  if (st.tsiemene.selected_tab >= n) st.tsiemene.selected_tab = 0;
  clamp_tsi_view_cursor(st);
  clamp_tsi_source_dataloader_selection(st);
}

inline void select_next_tsi_tab(CmdState& st) {
  const std::size_t n = tsi_tab_count();
  if (n == 0) {
    st.tsiemene.selected_tab = 0;
    st.tsiemene.panel_focus = TsiPanelFocus::Context;
    st.tsiemene.view_cursor = 0;
    st.tsiemene.selected_source_dataloader = 0;
    return;
  }
  st.tsiemene.selected_tab = (st.tsiemene.selected_tab + 1) % n;
  clamp_tsi_navigation_state(st);
}

inline void select_prev_tsi_tab(CmdState& st) {
  const std::size_t n = tsi_tab_count();
  if (n == 0) {
    st.tsiemene.selected_tab = 0;
    st.tsiemene.panel_focus = TsiPanelFocus::Context;
    st.tsiemene.view_cursor = 0;
    st.tsiemene.selected_source_dataloader = 0;
    return;
  }
  st.tsiemene.selected_tab = (st.tsiemene.selected_tab + n - 1) % n;
  clamp_tsi_navigation_state(st);
}

inline bool select_tsi_tab_by_token(CmdState& st, const std::string& token) {
  const std::size_t n = tsi_tab_count();
  if (n == 0) return false;

  std::size_t idx1 = 0;
  if (parse_positive_index(token, &idx1)) {
    if (idx1 == 0 || idx1 > n) return false;
    st.tsiemene.selected_tab = idx1 - 1;
    clamp_tsi_navigation_state(st);
    return true;
  }

  const std::string needle = to_lower_copy(token);
  const auto& docs = tsi_node_docs();
  for (std::size_t i = 0; i < docs.size(); ++i) {
    if (to_lower_copy(docs[i].id) == needle ||
        to_lower_copy(docs[i].title) == needle ||
        to_lower_copy(docs[i].type_name) == needle ||
        canonical_path_tokens::token_matches(docs[i].id, token) ||
        canonical_path_tokens::token_matches(docs[i].title, token) ||
        canonical_path_tokens::token_matches(docs[i].type_name, token)) {
      st.tsiemene.selected_tab = i;
      clamp_tsi_navigation_state(st);
      return true;
    }
  }
  return false;
}

template <class PushWarn, class AppendLog>
inline bool handle_tsi_show(CmdState& st, PushWarn&& push_warn, AppendLog&& append_log) {
  const auto& docs = tsi_node_docs();
  if (docs.empty()) {
    push_warn("no tsi tabs");
    return true;
  }
  const std::size_t tab = clamp_tsi_tab_index(st.tsiemene.selected_tab);
  const auto& d = docs[tab];
  append_log("tsi.tab=" + d.id, "show", "#d8d8ff");
  append_log("type=" + d.type_name, "show", "#d8d8ff");
  append_log("directives=" + std::to_string(d.directives.size()), "show", "#d8d8ff");
  if (d.type_name == "tsi.source.dataloader") {
    append_log(
        "created=" + std::to_string(tsi_source_dataloader_instances().size()),
        "show",
        "#d8d8ff");
  }
  return true;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
