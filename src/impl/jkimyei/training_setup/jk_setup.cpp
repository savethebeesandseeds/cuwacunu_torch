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

  // Cache miss: decode instruction from CONFIG and build just this component.
  auto instr_text = cuwacunu::piaabo::dconfig::config_space_t::training_components_instruction();
  cuwacunu::camahjucunu::BNF::trainingPipeline decoder;
  auto inst = decoder.decode(std::move(instr_text));

  {
    std::lock_guard<std::mutex> lk(mtx);
    // Build (or rebuild) into the map entry.
    auto& comp = components[component_name];
    comp.build_from(inst, component_name);
    return comp;
  }
}

} // namespace jkimyei
} // namespace cuwacunu
