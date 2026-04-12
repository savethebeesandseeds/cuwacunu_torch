#include "iitepi/runtime_binding/runtime_binding_space_t.h"

#include "camahjucunu/dsl/runtime_binding_instruction/runtime_binding_instruction.h"
#include "iitepi/contract_space_t.h"

#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_set>

namespace cuwacunu {
namespace iitepi {

namespace {

[[nodiscard]] std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] std::string resolve_path_from_folder(const std::string& folder,
                                                   std::string path) {
  path = trim_ascii_ws_copy(std::move(path));
  if (path.empty()) return {};
  const std::filesystem::path p(path);
  if (p.is_absolute()) return path;
  if (folder.empty()) return path;
  return (std::filesystem::path(folder) / p).string();
}

[[nodiscard]] std::string parent_folder_from_path(const std::string& path) {
  const std::filesystem::path p(path);
  if (!p.has_parent_path()) return {};
  return p.parent_path().string();
}

}  // namespace

void runtime_binding_space_t::network_topology_analytics(std::ostream* out,
                                                         bool beautify) {
  std::ostream& os = (out != nullptr) ? *out : std::cout;

  const auto runtime_binding_hash = locked_runtime_binding_hash();
  const auto binding_id = locked_binding_id();
  const auto runtime_binding =
      runtime_binding_space_t::runtime_binding_itself(runtime_binding_hash);
  if (!runtime_binding) {
    os << "[iitepi::runtime_binding_space_t::network_topology_analytics] error=null runtime binding record\n";
    return;
  }
  const auto& instruction = runtime_binding->runtime_binding.decoded();
  os << "[iitepi::runtime_binding_space_t::network_topology_analytics] runtime_binding_hash="
     << runtime_binding_hash << " binding_id=" << binding_id
     << " contracts=" << instruction.contracts.size() << "\n";

  if (instruction.contracts.empty()) {
    os << "[iitepi::runtime_binding_space_t::network_topology_analytics] no contracts declared in runtime binding\n";
    return;
  }

  std::unordered_set<std::string> reported_contract_hashes{};
  for (const auto& contract_decl : instruction.contracts) {
    const std::string contract_path =
        resolve_path_from_folder(
            parent_folder_from_path(runtime_binding->config_file_path),
            contract_decl.file);
    const std::string contract_hash =
        contract_space_t::register_contract_file(contract_path);

    if (!reported_contract_hashes.insert(contract_hash).second) {
      os << "[iitepi::runtime_binding_space_t::network_topology_analytics] contract_id="
         << contract_decl.id << " hash=" << contract_hash
         << " skipped=duplicate_hash\n";
      continue;
    }

    os << "[iitepi::runtime_binding_space_t::network_topology_analytics] contract_id="
       << contract_decl.id << " hash=" << contract_hash << "\n";
    contract_space_t::network_topology_analytics(contract_hash, &os, beautify);
  }
}

void runtime_binding_space_t::network_parameter_analytics(std::ostream* out,
                                                          bool beautify) {
  std::ostream& os = (out != nullptr) ? *out : std::cout;

  const auto runtime_binding_hash = locked_runtime_binding_hash();
  const auto binding_id = locked_binding_id();
  const auto runtime_binding =
      runtime_binding_space_t::runtime_binding_itself(runtime_binding_hash);
  if (!runtime_binding) {
    os << "[iitepi::runtime_binding_space_t::network_parameter_analytics] error=null runtime binding record\n";
    return;
  }
  const auto& instruction = runtime_binding->runtime_binding.decoded();
  os << "[iitepi::runtime_binding_space_t::network_parameter_analytics] runtime_binding_hash="
     << runtime_binding_hash << " binding_id=" << binding_id
     << " contracts=" << instruction.contracts.size() << "\n";

  if (instruction.contracts.empty()) {
    os << "[iitepi::runtime_binding_space_t::network_parameter_analytics] no contracts declared in runtime binding\n";
    return;
  }

  std::unordered_set<std::string> reported_contract_hashes{};
  for (const auto& contract_decl : instruction.contracts) {
    const std::string contract_path =
        resolve_path_from_folder(
            parent_folder_from_path(runtime_binding->config_file_path),
            contract_decl.file);
    const std::string contract_hash =
        contract_space_t::register_contract_file(contract_path);

    if (!reported_contract_hashes.insert(contract_hash).second) {
      os << "[iitepi::runtime_binding_space_t::network_parameter_analytics] contract_id="
         << contract_decl.id << " hash=" << contract_hash
         << " skipped=duplicate_hash\n";
      continue;
    }

    os << "[iitepi::runtime_binding_space_t::network_parameter_analytics] contract_id="
       << contract_decl.id << " hash=" << contract_hash << "\n";
    contract_space_t::network_parameter_analytics(contract_hash, &os, beautify);
  }
}

void runtime_binding_space_t::network_analytics(std::ostream* out,
                                                bool beautify,
                                                network_analytics_mode_e mode) {
  std::ostream& os = (out != nullptr) ? *out : std::cout;
  switch (mode) {
    case network_analytics_mode_e::Topology:
      network_topology_analytics(&os, beautify);
      return;
    case network_analytics_mode_e::Parameters:
      network_parameter_analytics(&os, beautify);
      return;
    case network_analytics_mode_e::Both:
      network_topology_analytics(&os, beautify);
      if (beautify) {
        os << "\x1b[1;94mRuntime Binding Parameter Analytics\x1b[0m\n";
      }
      network_parameter_analytics(&os, beautify);
      return;
  }
}

}  // namespace iitepi
}  // namespace cuwacunu
