// SPDX-License-Identifier: MIT
#pragma once

#include <string>

#include "wikimyei/assembly.h"

namespace cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg {

[[nodiscard]] inline cuwacunu::wikimyei::assembly::wikimyei_assembly_t
make_mtf_jepa_mae_vicreg_assembly(
    std::string component_assembly_id = "mtf_jepa_mae_vicreg_v1",
    std::string version_token =
        "wikimyei.representation.mtf_jepa_mae_vicreg.v1") {
  namespace wa = cuwacunu::wikimyei::assembly;

  wa::wikimyei_assembly_t out{};
  out.family = "wikimyei.representation.encoding.mtf_jepa_mae_vicreg";
  out.component_assembly_id = std::move(component_assembly_id);
  out.version_token = std::move(version_token);
  out.trainability = wa::assembly_trainability_t::trainable;
  out.docks.push_back(wa::make_dock(
      "channel_time_feature_input", wa::dock_direction_t::consumes,
      wa::dock_role_t::conditioning, wa::dock_domain_t::node_lifted_state,
      "standalone.channel_time_feature.v1", "[B,C,Hx,F]", "[B,C,Hx,F]", true,
      false, {"B", "C", "Hx", "F"}));
  out.docks.push_back(wa::make_dock(
      "pooled_representation", wa::dock_direction_t::produces,
      wa::dock_role_t::output, wa::dock_domain_t::node_representation,
      "standalone.mtf_jepa_mae_vicreg.v1", "[B,De]", "[B]", true, false,
      {"B", "De"}));
  out.docks.push_back(wa::make_dock(
      "channel_representation", wa::dock_direction_t::produces,
      wa::dock_role_t::output, wa::dock_domain_t::node_representation,
      "standalone.mtf_jepa_mae_vicreg.channel.v1", "[B,C,De]", "[B,C]", true,
      false, {"B", "C", "De"}));
  out.constraints.push_back(
      "separate representation family; does not replace VICReg");
  out.constraints.push_back(
      "JEPA latent prediction is primary; MAE reconstruction is auxiliary");
  out.constraints.push_back(
      "VICReg-style stabilization is local to this representation family");
  wa::validate_wikimyei_assembly(out);
  return out;
}

} // namespace cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg
