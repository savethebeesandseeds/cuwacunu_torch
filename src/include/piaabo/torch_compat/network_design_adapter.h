/* network_design_adapter.h */
#pragma once

#include "wikimyei/representation/VICReg/vicreg_4d_network_design.h"
#include "wikimyei/representation/VICReg/vicreg_4d_projector.h"

#include <string>

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {

struct vicreg_torch_adapter_spec_t {
  std::string projector_mlp_spec{};
  cuwacunu::wikimyei::vicreg_4d::ProjectorOptions projector_options{
      .norm_kind = cuwacunu::wikimyei::vicreg_4d::NormKind::LayerNorm,
      .act_kind = cuwacunu::wikimyei::vicreg_4d::ActKind::SiLU,
      .use_hidden_bias = false,
      .use_last_bias = false,
      .bn_in_fp32 = true,
  };
};

[[nodiscard]] bool build_vicreg_torch_adapter_spec(
    const cuwacunu::wikimyei::vicreg_4d::vicreg_network_design_spec_t& semantic_spec,
    vicreg_torch_adapter_spec_t* out,
    std::string* error = nullptr);

} /* namespace torch_compat */
} /* namespace piaabo */
} /* namespace cuwacunu */
