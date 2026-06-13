/* jkimyei_setup.cpp */
#include "jkimyei/api/jkimyei_setup.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace cuwacunu {
namespace jkimyei {

/* --------- static definitions (ODR-safe) --------- */
jkimyei_setup_t jkimyei_setup_t::registry{};
jkimyei_setup_t::_init jkimyei_setup_t::_initializer{};

/* ----------------- lifecycle (optional) ----------------- */
void jkimyei_setup_t::init() { log_dbg("[jkimyei_setup] initialising\n"); }
void jkimyei_setup_t::finit() { log_dbg("[jkimyei_setup] finalising\n"); }

std::string jkimyei_setup_t::make_component_key(
    const contract_fingerprint_t &contract_fingerprint,
    const std::string &runtime_component_name) {
  return contract_fingerprint + ":" + runtime_component_name;
}

/* ------------------- lazy accessor ------------------- */
jkimyei_component_t &jkimyei_setup_t::operator()(
    const std::string &component_name,
    const contract_fingerprint_t &contract_fingerprint) {
  if (contract_fingerprint.empty()) {
    log_fatal(
        "[jkimyei_setup] missing contract fingerprint for component '%s'\n",
        component_name.c_str());
  }
  const std::string component_key =
      make_component_key(contract_fingerprint, component_name);
  {
    std::lock_guard<std::mutex> lk(mtx);
    auto it = components.find(component_key);
    if (it != components.end())
      return it->second;
  }

  std::string component_lookup_name = component_name;
  std::string instr_text;
  {
    std::lock_guard<std::mutex> lk(mtx);
    auto override_it = component_instruction_overrides.find(component_key);
    if (override_it != component_instruction_overrides.end()) {
      component_lookup_name = override_it->second.component_lookup_name.empty()
                                  ? component_name
                                  : override_it->second.component_lookup_name;
      instr_text = override_it->second.instruction_text;
    }
  }

  // Cache miss: decode instruction from explicit DSL text.
  cuwacunu::jkimyei::specs::jkimyei_specs_t inst{};
  if (instr_text.empty()) {
    throw std::runtime_error(
        "[jkimyei_setup] no explicit jkimyei DSL bound for component '" +
        component_name + "'");
  } else {
    inst = cuwacunu::jkimyei::specs::dsl::decode_jkimyei_specs_from_dsl(
        "", std::move(instr_text));
  }

  {
    std::lock_guard<std::mutex> lk(mtx);
    // Build (or rebuild) into the map entry.
    auto &comp = components[component_key];
    comp.build_from(inst, component_lookup_name, component_name);
    return comp;
  }
}

void jkimyei_setup_t::set_component_instruction_override(
    const contract_fingerprint_t &contract_fingerprint,
    std::string runtime_component_name, std::string component_lookup_name,
    std::string instruction_text) {
  if (contract_fingerprint.empty()) {
    log_fatal(
        "[jkimyei_setup] missing contract fingerprint for override '%s'\n",
        runtime_component_name.c_str());
  }
  if (runtime_component_name.empty())
    return;
  if (component_lookup_name.empty())
    component_lookup_name = runtime_component_name;

  std::lock_guard<std::mutex> lk(mtx);
  const std::string runtime_key =
      make_component_key(contract_fingerprint, runtime_component_name);
  component_instruction_overrides[runtime_key] =
      component_instruction_override_t{
          .component_lookup_name = std::move(component_lookup_name),
          .instruction_text = std::move(instruction_text),
      };
  components.erase(runtime_key);
}

void jkimyei_setup_t::clear_component_instruction_override(
    const contract_fingerprint_t &contract_fingerprint,
    const std::string &runtime_component_name) {
  if (contract_fingerprint.empty()) {
    log_fatal("[jkimyei_setup] missing contract fingerprint while clearing "
              "override '%s'\n",
              runtime_component_name.c_str());
  }
  std::lock_guard<std::mutex> lk(mtx);
  const std::string runtime_key =
      make_component_key(contract_fingerprint, runtime_component_name);
  component_instruction_overrides.erase(runtime_key);
  components.erase(runtime_key);
}

void jkimyei_setup_t::clear_component_instruction_overrides(
    const contract_fingerprint_t &contract_fingerprint) {
  if (contract_fingerprint.empty()) {
    log_fatal("[jkimyei_setup] missing contract fingerprint while clearing all "
              "overrides for contract\n");
  }
  std::lock_guard<std::mutex> lk(mtx);
  for (auto comp_it = components.begin(); comp_it != components.end();) {
    if (comp_it->first.rfind(contract_fingerprint + ":", 0) == 0) {
      comp_it = components.erase(comp_it);
    } else {
      ++comp_it;
    }
  }
  for (auto ov_it = component_instruction_overrides.begin();
       ov_it != component_instruction_overrides.end();) {
    if (ov_it->first.rfind(contract_fingerprint + ":", 0) == 0) {
      ov_it = component_instruction_overrides.erase(ov_it);
    } else {
      ++ov_it;
    }
  }
}

void jkimyei_setup_t::clear_all_component_instruction_overrides() {
  std::lock_guard<std::mutex> lk(mtx);
  component_instruction_overrides.clear();
  components.clear();
}

} // namespace jkimyei
} // namespace cuwacunu
