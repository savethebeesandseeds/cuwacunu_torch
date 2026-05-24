// SPDX-License-Identifier: MIT
#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "wikimyei/assembly.h"

namespace cuwacunu::kikijyeba::topology {

namespace wikimyei_assembly = cuwacunu::wikimyei::assembly;

struct wikimyei_registry_t {
  std::vector<wikimyei_assembly::wikimyei_assembly_t> assemblies{};

  void add(wikimyei_assembly::wikimyei_assembly_t assembly) {
    wikimyei_assembly::validate_wikimyei_assembly(assembly);
    for (const auto &existing : assemblies) {
      if (existing.component_assembly_id == assembly.component_assembly_id) {
        throw std::runtime_error(
            "[wikimyei_registry] duplicate component_assembly_id: " +
            assembly.component_assembly_id);
      }
    }
    assemblies.push_back(std::move(assembly));
  }

  [[nodiscard]] const wikimyei_assembly::wikimyei_assembly_t &
  require_component(const std::string &component_assembly_id) const {
    for (const auto &assembly : assemblies) {
      if (assembly.component_assembly_id == component_assembly_id) {
        return assembly;
      }
    }
    throw std::runtime_error("[wikimyei_registry] unknown component_assembly_id: " +
                             component_assembly_id);
  }

  [[nodiscard]] std::unordered_map<std::string, std::string>
  component_fingerprints() const {
    std::unordered_map<std::string, std::string> out;
    for (const auto &assembly : assemblies) {
      out.emplace(assembly.component_assembly_id,
                  wikimyei_assembly::assembly_fingerprint(assembly));
    }
    return out;
  }
};

} // namespace cuwacunu::kikijyeba::topology
