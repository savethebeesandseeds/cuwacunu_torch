/* jk_setup.cpp */
#include "jkimyei/training_setup/jk_setup.h"
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

/* ------------------- lazy accessor ------------------- */
jk_component_t& jk_setup_t::operator()(const std::string& component_name) {
  {
    std::lock_guard<std::mutex> lk(mtx);
    auto it = components.find(component_name);
    if (it != components.end()) return it->second;
  }

  std::string component_lookup_name = component_name;
  std::string instr_text;
  {
    std::lock_guard<std::mutex> lk(mtx);
    auto override_it = component_instruction_overrides.find(component_name);
    if (override_it != component_instruction_overrides.end()) {
      component_lookup_name =
          override_it->second.component_lookup_name.empty()
              ? component_name
              : override_it->second.component_lookup_name;
      instr_text = override_it->second.instruction_text;
    }
  }

  // Cache miss: decode instruction from override text or contract config fallback.
  if (instr_text.empty()) {
    instr_text = cuwacunu::piaabo::dconfig::contract_space_t::jkimyei_specs_dsl();
  }
  auto inst = cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(
      cuwacunu::piaabo::dconfig::contract_space_t::jkimyei_specs_grammar(),
      std::move(instr_text));

  {
    std::lock_guard<std::mutex> lk(mtx);
    // Build (or rebuild) into the map entry.
    auto& comp = components[component_name];
    comp.build_from(inst, component_lookup_name, component_name);
    return comp;
  }
}

void jk_setup_t::set_component_instruction_override(std::string runtime_component_name,
                                                    std::string component_lookup_name,
                                                    std::string instruction_text) {
  if (runtime_component_name.empty()) return;
  if (component_lookup_name.empty()) component_lookup_name = runtime_component_name;

  std::lock_guard<std::mutex> lk(mtx);
  const std::string runtime_key = runtime_component_name;
  component_instruction_overrides[runtime_key] =
      component_instruction_override_t{
          .component_lookup_name = std::move(component_lookup_name),
          .instruction_text = std::move(instruction_text),
      };
  components.erase(runtime_key);
}

void jk_setup_t::clear_component_instruction_override(const std::string& runtime_component_name) {
  std::lock_guard<std::mutex> lk(mtx);
  component_instruction_overrides.erase(runtime_component_name);
  components.erase(runtime_component_name);
}

void jk_setup_t::clear_component_instruction_overrides() {
  std::lock_guard<std::mutex> lk(mtx);
  component_instruction_overrides.clear();
  components.clear();
}

} // namespace jkimyei
} // namespace cuwacunu
