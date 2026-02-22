#pragma once

#include <string>

#include "iinuji/iinuji_cmd/views/common.h"
#include "iinuji/iinuji_cmd/commands/iinuji.path.tokens.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void select_next_tsi_tab(CmdState& st) {
  const std::size_t n = tsi_tab_count();
  if (n == 0) {
    st.tsiemene.selected_tab = 0;
    return;
  }
  st.tsiemene.selected_tab = (st.tsiemene.selected_tab + 1) % n;
}

inline void select_prev_tsi_tab(CmdState& st) {
  const std::size_t n = tsi_tab_count();
  if (n == 0) {
    st.tsiemene.selected_tab = 0;
    return;
  }
  st.tsiemene.selected_tab = (st.tsiemene.selected_tab + n - 1) % n;
}

inline bool select_tsi_tab_by_token(CmdState& st, const std::string& token) {
  const std::size_t n = tsi_tab_count();
  if (n == 0) return false;

  std::size_t idx1 = 0;
  if (parse_positive_index(token, &idx1)) {
    if (idx1 == 0 || idx1 > n) return false;
    st.tsiemene.selected_tab = idx1 - 1;
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
  return true;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
