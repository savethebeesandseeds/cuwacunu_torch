/* tsiemene_circuit_runtime_resolve.cpp */
#include "tsiemene_circuit_runtime_internal.h"

#include <array>
#include <unordered_map>
#include <utility>

#include "camahjucunu/dsl/canonical_path/canonical_path.h"

namespace cuwacunu {
namespace camahjucunu {

namespace tsiemene_circuit_runtime_internal {

bool build_alias_type_map(
    const tsiemene_circuit_decl_t& circuit,
    std::unordered_map<std::string, tsiemene::TsiTypeId>* out,
    std::string* error) {
  if (!out) return false;
  out->clear();
  out->reserve(circuit.instances.size());
  std::array<std::size_t, tsiemene::kTsiTypeRegistry.size()> instance_counts{};

  for (const auto& inst : circuit.instances) {
    const std::string alias = trim_ascii_ws_copy(inst.alias);
    const std::string type = trim_ascii_ws_copy(inst.tsi_type);
    if (alias.empty()) {
      if (error) *error = "empty instance alias";
      return false;
    }
    if (type.empty()) {
      if (error) *error = "empty tsi_type for alias: " + alias;
      return false;
    }

    const auto type_path = cuwacunu::camahjucunu::decode_canonical_path(type);
    if (!type_path.ok) {
      if (error) {
        *error = "invalid tsi_type canonical path for alias " + alias + ": " +
                 type_path.error;
      }
      return false;
    }
    if (type_path.path_kind !=
        cuwacunu::camahjucunu::canonical_path_kind_e::Node) {
      if (error) {
        *error = "tsi_type must be canonical node path for alias " + alias +
                 ": " + type_path.canonical;
      }
      return false;
    }

    const auto type_id = tsiemene::parse_tsi_type_id(type_path.canonical_identity);
    if (!type_id) {
      if (error) {
        *error = "unsupported tsi_type for alias " + alias + ": " +
                 type_path.canonical_identity;
      }
      return false;
    }
    const std::size_t type_index = tsiemene::tsi_type_index(*type_id);
    ++instance_counts[type_index];
    if (tsiemene::is_unique_instance_type(*type_id) &&
        instance_counts[type_index] > 1) {
      if (error) {
        *error = "tsi_type must be unique per circuit: " +
                 std::string(tsiemene::tsi_type_token(*type_id)) +
                 " (alias: " + alias + ")";
      }
      return false;
    }
    if (!out->emplace(alias, *type_id).second) {
      if (error) *error = "duplicated instance alias: " + alias;
      return false;
    }
  }
  return true;
}

bool resolve_hop_decl_with_types(
    const tsiemene_hop_decl_t& hop,
    const std::unordered_map<std::string, tsiemene::TsiTypeId>& alias_to_type,
    tsiemene_resolved_hop_t* out,
    std::string* error) {
  if (!out) return false;

  const std::string from_instance = trim_ascii_ws_copy(hop.from.instance);
  const std::string to_instance = trim_ascii_ws_copy(hop.to.instance);
  const std::string from_dir_text = trim_ascii_ws_copy(hop.from.directive);
  const std::string from_kind_text = trim_ascii_ws_copy(hop.from.kind);
  const std::string to_dir_text = trim_ascii_ws_copy(hop.to.directive);
  const std::string to_kind_text = trim_ascii_ws_copy(hop.to.kind);

  const auto from_it = alias_to_type.find(from_instance);
  if (from_it == alias_to_type.end()) {
    if (error) *error = "hop references unknown instance alias: " + from_instance;
    return false;
  }
  const auto to_it = alias_to_type.find(to_instance);
  if (to_it == alias_to_type.end()) {
    if (error) *error = "hop references unknown instance alias: " + to_instance;
    return false;
  }

  const auto from_dir = parse_directive_ref(from_dir_text);
  const auto from_kind = parse_kind_ref(from_kind_text);
  if (!from_dir || !from_kind) {
    if (error) {
      *error = "invalid directive/kind in hop: " + from_instance + "@" +
               from_dir_text + ":" + from_kind_text + " -> " + to_instance;
    }
    return false;
  }

  if (!tsiemene::type_emits_output(from_it->second, *from_dir, *from_kind)) {
    if (error) {
      *error = "hop source endpoint is not an output of source tsi type: " +
               from_instance + std::string(*from_dir) +
               std::string(tsiemene::kind_token(*from_kind)) + " for type " +
               std::string(tsiemene::tsi_type_token(from_it->second));
    }
    return false;
  }

  if (to_dir_text.empty()) {
    if (error) {
      *error = "missing target input directive in hop: " + from_instance +
               std::string(*from_dir) +
               std::string(tsiemene::kind_token(*from_kind)) + " -> " +
               to_instance;
    }
    return false;
  }
  if (!to_kind_text.empty()) {
    if (error) {
      *error = "target kind cast is not allowed in hop: " + from_instance +
               std::string(*from_dir) +
               std::string(tsiemene::kind_token(*from_kind)) + " -> " +
               to_instance + "@" + to_dir_text + ":" + to_kind_text +
               " (use target inbound directive only; kind is inferred from source)";
    }
    return false;
  }
  const auto to_dir = parse_directive_ref(to_dir_text);
  if (!to_dir) {
    if (error) {
      *error = "invalid target directive in hop: " + to_instance + "@" + to_dir_text;
    }
    return false;
  }

  if (!tsiemene::type_is_compatible(to_it->second, *to_dir, *from_kind)) {
    if (error) {
      *error = "hop target endpoint is not an input of target tsi type: " +
               to_instance + std::string(*to_dir) +
               std::string(tsiemene::kind_token(*from_kind)) + " for type " +
               std::string(tsiemene::tsi_type_token(to_it->second));
    }
    return false;
  }

  out->from.instance = from_instance;
  out->from.directive = *from_dir;
  out->from.kind = *from_kind;

  out->to.instance = to_instance;
  out->to.directive = *to_dir;
  out->to.kind = *from_kind;
  return true;
}

} /* namespace tsiemene_circuit_runtime_internal */

bool resolve_hops(
    const tsiemene_circuit_decl_t& circuit,
    std::vector<tsiemene_resolved_hop_t>* out_hops,
    std::string* error) {
  if (!out_hops) return false;
  std::unordered_map<std::string, tsiemene::TsiTypeId> alias_to_type;
  if (!tsiemene_circuit_runtime_internal::build_alias_type_map(
          circuit, &alias_to_type, error)) {
    return false;
  }

  out_hops->clear();
  out_hops->reserve(circuit.hops.size());

  for (const auto& h : circuit.hops) {
    tsiemene_resolved_hop_t resolved{};
    if (!tsiemene_circuit_runtime_internal::resolve_hop_decl_with_types(
            h, alias_to_type, &resolved, error)) {
      return false;
    }
    out_hops->push_back(std::move(resolved));
  }

  return true;
}

bool resolve_hop_decl(const tsiemene_hop_decl_t& hop,
                      tsiemene_resolved_hop_t* out,
                      std::string* error) {
  if (!out) return false;

  const std::string from_dir_text =
      tsiemene_circuit_runtime_internal::trim_ascii_ws_copy(hop.from.directive);
  const std::string from_kind_text =
      tsiemene_circuit_runtime_internal::trim_ascii_ws_copy(hop.from.kind);
  const std::string to_dir_text =
      tsiemene_circuit_runtime_internal::trim_ascii_ws_copy(hop.to.directive);
  const std::string to_kind_text =
      tsiemene_circuit_runtime_internal::trim_ascii_ws_copy(hop.to.kind);

  const auto from_dir = parse_directive_ref(hop.from.directive);
  const auto from_kind = parse_kind_ref(hop.from.kind);
  if (!from_dir || !from_kind) {
    if (error) {
      *error = "invalid directive/kind in hop: " + hop.from.instance + "@" +
               from_dir_text + ":" + from_kind_text + " -> " + hop.to.instance;
    }
    return false;
  }

  if (to_dir_text.empty()) {
    if (error) {
      *error = "missing target input directive in hop: " + hop.from.instance +
               "@" + from_dir_text + ":" + from_kind_text + " -> " +
               hop.to.instance;
    }
    return false;
  }
  if (!to_kind_text.empty()) {
    if (error) {
      *error = "target kind cast is not allowed in hop: " + hop.from.instance +
               "@" + from_dir_text + ":" + from_kind_text + " -> " +
               hop.to.instance + "@" + to_dir_text + ":" + to_kind_text;
    }
    return false;
  }
  const auto to_dir = parse_directive_ref(to_dir_text);
  if (!to_dir) {
    if (error) {
      *error = "invalid target directive in hop: " + hop.to.instance + "@" + to_dir_text;
    }
    return false;
  }

  out->from.instance = hop.from.instance;
  out->from.directive = *from_dir;
  out->from.kind = *from_kind;

  out->to.instance = hop.to.instance;
  out->to.directive = *to_dir;
  out->to.kind = *from_kind;
  return true;
}

} /* namespace camahjucunu */
} /* namespace cuwacunu */
