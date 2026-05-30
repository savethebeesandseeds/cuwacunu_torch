#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace cuwacunu::hero::marshal {

inline constexpr std::string_view kProtocolLayerStdio = "STDIO";
inline constexpr std::string_view kProtocolLayerHttpsSse = "HTTPS/SSE";
inline constexpr std::string_view kProtocolLayerHttpsSseFailFastMessage =
    "HTTPS/SSE protocol layer is not implemented yet; set "
    "protocol_layer=STDIO.";

struct marshal_tool_descriptor_t {
  std::string_view name;
  std::string_view description;
  std::string_view input_schema_json;
};

inline constexpr std::size_t kMarshalToolDescriptorCount = 0
#define HERO_MARSHAL_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON) +1
#include "hero/marshal_hero/hero_marshal.def"
#undef HERO_MARSHAL_TOOL
    ;

inline constexpr std::array<marshal_tool_descriptor_t,
                            kMarshalToolDescriptorCount>
    kMarshalToolDescriptors = {{
#define HERO_MARSHAL_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON)                \
  marshal_tool_descriptor_t{NAME, DESCRIPTION, INPUT_SCHEMA_JSON},
#include "hero/marshal_hero/hero_marshal.def"
#undef HERO_MARSHAL_TOOL
    }};

inline constexpr std::string_view kMarshalHeroBoundaryText =
    "Marshal prepares bounded handoffs and explains state. Runtime executes "
    "and writes evidence. Lattice proves target satisfaction.";

inline constexpr std::string_view kMarshalPolicyTemplateText = R"(
# Marshal Hero v0 policy
protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO
)";

} // namespace cuwacunu::hero::marshal
