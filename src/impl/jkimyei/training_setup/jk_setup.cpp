/* jk_setup.cpp */
#include "jkimyei/training_setup/jk_setup.h"
#include <filesystem>
#include <utility>
#include <string>

namespace cuwacunu {
namespace jkimyei {

/* --------- static definitions (ODR-safe) --------- */
jk_setup_t jk_setup_t::registry{};
jk_setup_t::_init    jk_setup_t::_initializer{};

/* ----------------- lifecycle (optional) ----------------- */
void jk_setup_t::init() {
  log_info("[jk_setup] initialising\n");
}
void jk_setup_t::finit() {
  log_info("[jk_setup] finalising\n");
}

std::string jk_setup_t::make_component_key(
    const cuwacunu::iitepi::contract_hash_t& contract_hash,
    const std::string& runtime_component_name) {
  return contract_hash + ":" + runtime_component_name;
}

/* ------------------- lazy accessor ------------------- */
jk_component_t& jk_setup_t::operator()(
    const std::string& component_name,
    const cuwacunu::iitepi::contract_hash_t& contract_hash) {
  if (contract_hash.empty()) {
    log_fatal("[jk_setup] missing contract hash for component '%s'\n",
              component_name.c_str());
  }
  const std::string component_key = make_component_key(contract_hash, component_name);
  {
    std::lock_guard<std::mutex> lk(mtx);
    auto it = components.find(component_key);
    if (it != components.end()) return it->second;
  }

  std::string component_lookup_name = component_name;
  std::string instr_text;
  {
    std::lock_guard<std::mutex> lk(mtx);
    auto override_it = component_instruction_overrides.find(component_key);
    if (override_it != component_instruction_overrides.end()) {
      component_lookup_name =
          override_it->second.component_lookup_name.empty()
              ? component_name
              : override_it->second.component_lookup_name;
      instr_text = override_it->second.instruction_text;
    }
  }

  const auto contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);

  // Cache miss: decode instruction from override text or contract record payload.
  cuwacunu::camahjucunu::jkimyei_specs_t inst{};
  if (instr_text.empty()) {
    inst = contract_itself->jkimyei.decoded();
  } else {
    inst = cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(
        contract_itself->jkimyei.grammar,
        std::move(instr_text));
  }

  {
    std::lock_guard<std::mutex> lk(mtx);
    // Build (or rebuild) into the map entry.
    auto& comp = components[component_key];
    comp.build_from(inst, component_lookup_name, component_name);
    return comp;
  }
}

void jk_setup_t::set_component_instruction_override(
    const cuwacunu::iitepi::contract_hash_t& contract_hash,
    std::string runtime_component_name,
    std::string component_lookup_name,
    std::string instruction_text) {
  if (contract_hash.empty()) {
    log_fatal("[jk_setup] missing contract hash for override '%s'\n",
              runtime_component_name.c_str());
  }
  if (runtime_component_name.empty()) return;
  if (component_lookup_name.empty()) component_lookup_name = runtime_component_name;

  std::lock_guard<std::mutex> lk(mtx);
  const std::string runtime_key =
      make_component_key(contract_hash, runtime_component_name);
  component_instruction_overrides[runtime_key] =
      component_instruction_override_t{
          .component_lookup_name = std::move(component_lookup_name),
          .instruction_text = std::move(instruction_text),
      };
  components.erase(runtime_key);
}

void jk_setup_t::clear_component_instruction_override(
    const cuwacunu::iitepi::contract_hash_t& contract_hash,
    const std::string& runtime_component_name) {
  if (contract_hash.empty()) {
    log_fatal("[jk_setup] missing contract hash while clearing override '%s'\n",
              runtime_component_name.c_str());
  }
  std::lock_guard<std::mutex> lk(mtx);
  const std::string runtime_key =
      make_component_key(contract_hash, runtime_component_name);
  component_instruction_overrides.erase(runtime_key);
  components.erase(runtime_key);
}

void jk_setup_t::clear_component_instruction_overrides(
    const cuwacunu::iitepi::contract_hash_t& contract_hash) {
  if (contract_hash.empty()) {
    log_fatal("[jk_setup] missing contract hash while clearing all overrides for contract\n");
  }
  std::lock_guard<std::mutex> lk(mtx);
  for (auto comp_it = components.begin(); comp_it != components.end();) {
    if (comp_it->first.rfind(contract_hash + ":", 0) == 0) {
      comp_it = components.erase(comp_it);
    } else {
      ++comp_it;
    }
  }
  for (auto ov_it = component_instruction_overrides.begin();
       ov_it != component_instruction_overrides.end();) {
    if (ov_it->first.rfind(contract_hash + ":", 0) == 0) {
      ov_it = component_instruction_overrides.erase(ov_it);
    } else {
      ++ov_it;
    }
  }
}

void jk_setup_t::clear_all_component_instruction_overrides() {
  std::lock_guard<std::mutex> lk(mtx);
  component_instruction_overrides.clear();
  components.clear();
}

} // namespace jkimyei
} // namespace cuwacunu
