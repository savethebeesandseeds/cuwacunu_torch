/* tsiemene_circuit_runtime_validate.cpp */
#include "tsiemene_circuit_runtime_internal.h"

#include <functional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace cuwacunu {
namespace camahjucunu {

bool validate_circuit_decl(const tsiemene_circuit_decl_t& circuit,
                           std::string* error) {
  const std::string circuit_name =
      tsiemene_circuit_runtime_internal::trim_ascii_ws_copy(circuit.name);
  if (circuit_name.empty()) {
    if (error) *error = "empty circuit name";
    return false;
  }
  if (circuit.instances.empty()) {
    if (error) *error = "circuit has no instance declarations";
    return false;
  }
  if (circuit.hops.empty()) {
    if (error) *error = "circuit has no hop declarations";
    return false;
  }

  std::unordered_map<std::string, tsiemene::TsiTypeId> alias_to_type;
  if (!tsiemene_circuit_runtime_internal::build_alias_type_map(
          circuit, &alias_to_type, error)) {
    return false;
  }

  std::vector<tsiemene_resolved_hop_t> resolved_hops;
  if (!resolve_hops(circuit, &resolved_hops, error)) return false;

  std::unordered_map<std::string, std::vector<std::string>> adj;
  std::unordered_map<std::string, std::size_t> in_degree;
  std::unordered_map<std::string, std::size_t> out_degree;
  std::unordered_set<std::string> referenced;
  referenced.reserve(circuit.instances.size());

  for (const auto& h : resolved_hops) {
    if (alias_to_type.find(h.from.instance) == alias_to_type.end()) {
      if (error) *error = "hop references unknown instance alias: " + h.from.instance;
      return false;
    }
    if (alias_to_type.find(h.to.instance) == alias_to_type.end()) {
      if (error) *error = "hop references unknown instance alias: " + h.to.instance;
      return false;
    }
    referenced.insert(h.from.instance);
    referenced.insert(h.to.instance);
    adj[h.from.instance].push_back(h.to.instance);
    (void)adj[h.to.instance];
    ++in_degree[h.to.instance];
    (void)in_degree[h.from.instance];
    ++out_degree[h.from.instance];
    (void)out_degree[h.to.instance];
  }

  if (referenced.empty()) {
    if (error) *error = "no valid hop endpoints";
    return false;
  }

  for (const auto& [alias, _] : alias_to_type) {
    if (!referenced.count(alias)) {
      if (error) *error = "orphan instance not referenced by any hop: " + alias;
      return false;
    }
  }

  std::vector<std::string> roots;
  roots.reserve(referenced.size());
  for (const auto& alias : referenced) {
    const auto in_it = in_degree.find(alias);
    const std::size_t id = (in_it == in_degree.end()) ? 0 : in_it->second;
    if (id == 0) roots.push_back(alias);
  }

  if (roots.empty()) {
    if (error) *error = "circuit has no root instance";
    return false;
  }
  if (roots.size() != 1) {
    if (error) *error = "circuit must have exactly one root instance";
    return false;
  }

  std::unordered_map<std::string, int> color;
  std::unordered_set<std::string> reachable;
  bool cycle = false;

  std::function<void(const std::string&)> dfs = [&](const std::string& u) {
    if (cycle) return;
    color[u] = 1;
    reachable.insert(u);
    const auto it = adj.find(u);
    if (it != adj.end()) {
      for (const std::string& v : it->second) {
        const int state = color[v];
        if (state == 1) {
          cycle = true;
          return;
        }
        if (state == 0) dfs(v);
        if (cycle) return;
      }
    }
    color[u] = 2;
  };

  dfs(roots[0]);
  if (cycle) {
    if (error) *error = "cycle detected in circuit hops";
    return false;
  }

  if (reachable.size() != referenced.size()) {
    if (error) *error = "unreachable instance from circuit root";
    return false;
  }

  for (const auto& alias : referenced) {
    const auto od_it = out_degree.find(alias);
    const std::size_t od = (od_it == out_degree.end()) ? 0 : od_it->second;
    if (od != 0) continue;
    const auto type_it = alias_to_type.find(alias);
    if (type_it == alias_to_type.end()) {
      if (error) {
        *error = "internal semantic error resolving type for alias: " + alias;
      }
      return false;
    }
    if (!tsiemene::is_sink_type(type_it->second)) {
      if (error) {
        *error = "terminal instance must be sink type: " + alias + "=" +
                 std::string(tsiemene::tsi_type_token(type_it->second));
      }
      return false;
    }
  }

  return true;
}

bool validate_circuit_instruction(
    const tsiemene_circuit_instruction_t& circuit_instruction,
    std::string* error) {
  if (circuit_instruction.circuits.empty()) {
    if (error) *error = "circuit instruction has no circuits";
    return false;
  }

  std::unordered_set<std::string> circuit_names;
  circuit_names.reserve(circuit_instruction.circuits.size());

  for (std::size_t i = 0; i < circuit_instruction.circuits.size(); ++i) {
    const auto& c = circuit_instruction.circuits[i];
    const std::string cname =
        tsiemene_circuit_runtime_internal::trim_ascii_ws_copy(c.name);
    if (!circuit_names.insert(cname).second) {
      if (error) *error = "duplicated circuit name: " + cname;
      return false;
    }

    std::string local_error;
    if (!validate_circuit_decl(c, &local_error)) {
      if (error) *error = "circuit[" + std::to_string(i) + "] " + local_error;
      return false;
    }
  }

  return true;
}

std::string tsiemene_circuit_instruction_t::str() const {
  std::ostringstream oss;
  oss << "tsiemene_circuit_instruction_t: circuits=" << circuits.size() << "\n";
  for (std::size_t i = 0; i < circuits.size(); ++i) {
    const auto& c = circuits[i];
    oss << "  [" << i << "] " << c.name
        << " instances=" << c.instances.size()
        << " hops=" << c.hops.size()
        << " invoke=" << c.invoke_name
        << "(\"" << c.invoke_payload << "\")\n";
  }
  return oss.str();
}

} /* namespace camahjucunu */
} /* namespace cuwacunu */
