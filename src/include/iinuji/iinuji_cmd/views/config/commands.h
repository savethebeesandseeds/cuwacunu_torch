#pragma once

#include <string>

#include "iinuji/iinuji_cmd/views/common.h"
#include "iinuji/iinuji_cmd/commands/iinuji.path.tokens.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void select_next_tab(CmdState& st) {
  if (!config_has_tabs(st)) {
    st.config.selected_tab = 0;
    return;
  }
  st.config.selected_tab = (st.config.selected_tab + 1) % st.config.tabs.size();
}

inline void select_prev_tab(CmdState& st) {
  if (!config_has_tabs(st)) {
    st.config.selected_tab = 0;
    return;
  }
  st.config.selected_tab =
      (st.config.selected_tab + st.config.tabs.size() - 1) % st.config.tabs.size();
}

inline bool select_tab_by_token(CmdState& st, const std::string& token) {
  if (!config_has_tabs(st)) return false;
  std::size_t idx1 = 0;
  if (parse_positive_index(token, &idx1)) {
    if (idx1 == 0 || idx1 > st.config.tabs.size()) return false;
    st.config.selected_tab = idx1 - 1;
    return true;
  }

  const std::string needle = to_lower_copy(token);
  for (std::size_t i = 0; i < st.config.tabs.size(); ++i) {
    const auto& t = st.config.tabs[i];
    if (to_lower_copy(t.id) == needle ||
        to_lower_copy(t.title) == needle ||
        canonical_path_tokens::token_matches(t.id, token) ||
        canonical_path_tokens::token_matches(t.title, token)) {
      st.config.selected_tab = i;
      return true;
    }
  }
  return false;
}

template <class PushWarn, class AppendLog>
inline bool handle_config_show(CmdState& st, PushWarn&& push_warn, AppendLog&& append_log) {
  if (!config_has_tabs(st)) {
    push_warn("no config tabs");
    return true;
  }
  const auto& tab = st.config.tabs[st.config.selected_tab];
  append_log("tab=" + tab.id, "show", "#d8d8ff");
  append_log("path=" + (tab.path.empty() ? "<none>" : tab.path), "show", "#d8d8ff");
  append_log("status=" + std::string(tab.ok ? "ok" : ("error: " + tab.error)), "show", "#d8d8ff");
  return true;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
