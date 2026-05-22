#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace cuwacunu::hero::lattice {

inline constexpr std::string_view kProtocolLayerStdio = "STDIO";
inline constexpr std::string_view kProtocolLayerHttpsSse = "HTTPS/SSE";
inline constexpr std::string_view kProtocolLayerHttpsSseFailFastMessage =
    "HTTPS/SSE protocol layer is not implemented yet; set "
    "protocol_layer=STDIO.";

struct lattice_policy_descriptor_t {
  std::string_view key;
  std::string_view type;
  bool required;
  std::string_view default_value;
  std::string_view range;
  std::string_view allowed;
  std::string_view summary;
};

inline constexpr std::size_t kLatticePolicyDescriptorCount = 0
#define HERO_LATTICE_POLICY_KEY(KEY, TYPE, REQUIRED, DEFAULT_VALUE, RANGE,     \
                                ALLOWED, SUMMARY)                              \
  +1
#include "hero/lattice_hero/hero_lattice.def"
#undef HERO_LATTICE_POLICY_KEY
    ;

inline constexpr std::array<lattice_policy_descriptor_t,
                            kLatticePolicyDescriptorCount>
    kLatticePolicyDescriptors = {{
#define HERO_LATTICE_POLICY_KEY(KEY, TYPE, REQUIRED, DEFAULT_VALUE, RANGE,     \
                                ALLOWED, SUMMARY)                              \
  lattice_policy_descriptor_t{                                                 \
      KEY, TYPE, ((REQUIRED) != 0), DEFAULT_VALUE, RANGE, ALLOWED, SUMMARY},
#include "hero/lattice_hero/hero_lattice.def"
#undef HERO_LATTICE_POLICY_KEY
    }};

inline constexpr std::string_view kLatticePolicyTemplateText = R"(
# Lattice Hero v0 policy
protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO
default_config_path:path = /cuwacunu/src/config/.config
runtime_root:path = /cuwacunu/.runtime/cuwacunu_exec
allowed_runtime_roots:path_list = /cuwacunu/.runtime/cuwacunu_exec,/tmp
auto_build_exposure_ledger:bool = true
max_fact_preview:int = 64
max_closure_facts:int = 256
)";

} // namespace cuwacunu::hero::lattice
