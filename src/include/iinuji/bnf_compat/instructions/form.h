#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>

#include "iinuji/iinuji_utils.h"
RUNTIME_WARNING("(iinuji/form.h, validation.h)[] grammar allows arbitrary __form local_name but validator restricts to {str,vec,num}; document as semantic rule or enforce in grammar.\n");

namespace cuwacunu {
namespace iinuji {

/* __form parsing (NO traversal):
   - data slots: .strN / .vecN / .numN
   - system slots: .sys.stdout / .sys.stderr   (string payload sources)
*/
enum class bind_kind_e { Str, Vec, Num, Unknown };

enum class sys_ref_e  { Stdout, Stderr, Invalid };
enum class data_kind_e { Str, Vec, Num, System, Invalid };

struct data_ref_t {
  data_kind_e kind = data_kind_e::Invalid;
  int index = -1;              // only used for Str/Vec/Num
  sys_ref_e sys = sys_ref_e::Invalid; // only used for System
  std::string raw;
};

inline bind_kind_e parse_bind_kind(const std::string& local_name) {
  const std::string t = to_lower(local_name);
  if (t == "str") return bind_kind_e::Str;
  if (t == "vec") return bind_kind_e::Vec;
  if (t == "num") return bind_kind_e::Num;
  return bind_kind_e::Unknown;
}

inline data_ref_t parse_data_path(const std::string& path) {
  data_ref_t out; out.raw = path;
  if (path.size() < 2 || path[0] != '.') return out;

  // allow only [a-zA-Z0-9._] and forbid ".."
  for (char c : path) {
    if (!(std::isalnum((unsigned char)c) || c=='.' || c=='_')) return out;
  }
  if (path.find("..") != std::string::npos) return out;

  const std::string name = path.substr(1);

  // system tokens (be robust to decoder dropping dots)
  if (name == "sys.stdout" || name == "sysstdout") {
    out.kind = data_kind_e::System;
    out.sys  = sys_ref_e::Stdout;
    out.index = -1;
    return out;
  }
  if (name == "sys.stderr" || name == "sysstderr") {
    out.kind = data_kind_e::System;
    out.sys  = sys_ref_e::Stderr;
    out.index = -1;
    return out;
  }


  auto parse_indexed = [&](const std::string& prefix, data_kind_e k)->bool {
    if (name.rfind(prefix, 0) != 0) return false;
    std::string rest = name.substr(prefix.size());
    if (rest.empty()) return false;
    for (char c : rest) if (!std::isdigit((unsigned char)c)) return false;
    int idx = -1;
    try { idx = std::stoi(rest); } catch (...) { return false; }
    out.kind = k;
    out.index = idx;
    out.sys = sys_ref_e::Invalid;
    return true;
  };

  if (parse_indexed("str", data_kind_e::Str)) return out;
  if (parse_indexed("vec", data_kind_e::Vec)) return out;
  if (parse_indexed("num", data_kind_e::Num)) return out;

  return out;
}

/* Type compatibility:
   - str binding can target .strN OR a system string source (.sys.*)
*/
inline bool kind_ok(bind_kind_e bk, data_kind_e dk) {
  if (bk==bind_kind_e::Str) return (dk==data_kind_e::Str || dk==data_kind_e::System);
  if (bk==bind_kind_e::Vec) return (dk==data_kind_e::Vec);
  if (bk==bind_kind_e::Num) return (dk==data_kind_e::Num);
  return false;
}

struct resolved_binding_t {
  bind_kind_e bind_kind = bind_kind_e::Unknown;
  data_ref_t  ref;
};

struct resolved_event_t {
  std::string kind_raw;  // "_update" or "_action"
  std::string name;
  std::vector<resolved_binding_t> bindings;
};

using resolved_event_map_t = std::unordered_map<std::string, resolved_event_t>;

inline bind_kind_e required_bind_kind_for_figure(const std::string& fig_kind_raw) {
  if (fig_kind_raw == "_horizontal_plot") return bind_kind_e::Vec;
  if (fig_kind_raw == "_buffer")          return bind_kind_e::Str;
  return bind_kind_e::Str; // label + input_box
}

inline std::string required_event_kind_for_figure(const std::string& fig_kind_raw) {
  if (fig_kind_raw == "_input_box") return "_action";
  if (fig_kind_raw == "_buffer")    return "_update";
  return "_update";
}

inline const resolved_binding_t* first_binding_of_kind(const resolved_event_t& E, bind_kind_e want) {
  for (const auto& b : E.bindings) if (b.bind_kind == want) return &b;
  return nullptr;
}

inline bool event_has_system_binding(const resolved_event_t& E) {
  for (const auto& b : E.bindings) if (b.ref.kind == data_kind_e::System) return true;
  return false;
}

} // namespace iinuji
} // namespace cuwacunu
