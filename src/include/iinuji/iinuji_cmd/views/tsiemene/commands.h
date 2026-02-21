#pragma once

#include <sstream>
#include <string>

#include "iinuji/iinuji_cmd/views/common.h"

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
        to_lower_copy(docs[i].type_name) == needle) {
      st.tsiemene.selected_tab = i;
      return true;
    }
  }
  return false;
}

template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
inline bool handle_tsi_command(CmdState& st,
                               const std::string& command,
                               std::istringstream& iss,
                               PushInfo&& push_info,
                               PushWarn&& push_warn,
                               PushErr&& push_err,
                               AppendLog&& append_log) {
  if (!(command == "tsi" || command == "f4")) return false;

  std::string arg0;
  iss >> arg0;
  arg0 = to_lower_copy(arg0);
  if (arg0.empty()) {
    st.screen = ScreenMode::Tsiemene;
    push_info("screen=tsi");
    return true;
  }
  if (arg0 == "tabs") {
    const auto& docs = tsi_node_docs();
    if (docs.empty()) {
      push_warn("no tsi tabs");
      return true;
    }
    for (std::size_t i = 0; i < docs.size(); ++i) {
      append_log("[" + std::to_string(i + 1) + "] " + docs[i].id, "tsi.tabs", "#d0d0d0");
    }
    st.screen = ScreenMode::Tsiemene;
    return true;
  }
  if (arg0 == "tab") {
    std::string arg1;
    iss >> arg1;
    arg1 = to_lower_copy(arg1);
    if (arg1.empty()) {
      push_err("usage: tsi tab next|prev|N|<id>");
      return true;
    }
    if (arg1 == "next") {
      select_next_tsi_tab(st);
      st.screen = ScreenMode::Tsiemene;
      push_info("selected tsi tab=" + std::to_string(st.tsiemene.selected_tab + 1));
      return true;
    }
    if (arg1 == "prev") {
      select_prev_tsi_tab(st);
      st.screen = ScreenMode::Tsiemene;
      push_info("selected tsi tab=" + std::to_string(st.tsiemene.selected_tab + 1));
      return true;
    }
    if (!select_tsi_tab_by_token(st, arg1)) {
      push_err("tsi tab not found");
      return true;
    }
    st.screen = ScreenMode::Tsiemene;
    push_info("selected tsi tab=" + std::to_string(st.tsiemene.selected_tab + 1));
    return true;
  }
  push_err("usage: tsi | tsi tabs | tsi tab next|prev|N|<id>");
  return true;
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
