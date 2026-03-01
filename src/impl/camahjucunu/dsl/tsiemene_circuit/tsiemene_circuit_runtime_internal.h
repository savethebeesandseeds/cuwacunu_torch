/* tsiemene_circuit_runtime_internal.h */
#pragma once

#include <string>
#include <unordered_map>

#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"
#include "tsiemene/tsi.type.registry.h"

namespace cuwacunu {
namespace camahjucunu {
namespace tsiemene_circuit_runtime_internal {

std::string trim_ascii_ws_copy(std::string s);

bool build_alias_type_map(
    const tsiemene_circuit_decl_t& circuit,
    std::unordered_map<std::string, tsiemene::TsiTypeId>* out,
    std::string* error);

bool resolve_hop_decl_with_types(
    const tsiemene_hop_decl_t& hop,
    const std::unordered_map<std::string, tsiemene::TsiTypeId>& alias_to_type,
    tsiemene_resolved_hop_t* out,
    std::string* error);

} /* namespace tsiemene_circuit_runtime_internal */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
