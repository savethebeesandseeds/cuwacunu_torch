// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>

namespace cuwacunu::hero::environment {

struct environment_tool_descriptor_t {
  const char *name;
  const char *description;
  const char *input_schema_json;
};

inline constexpr std::size_t kEnvironmentToolDescriptorCount = 0
#define HERO_ENVIRONMENT_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON) +1
#include "hero/environment_hero/hero_environment.def"
#undef HERO_ENVIRONMENT_TOOL
    ;

} // namespace cuwacunu::hero::environment
