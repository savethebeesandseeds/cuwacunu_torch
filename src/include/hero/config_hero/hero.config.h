#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace cuwacunu {
namespace hero {
namespace config {

inline constexpr std::string_view kProtocolLayerStdio = "STDIO";
inline constexpr std::string_view kProtocolLayerHttpsSse = "HTTPS/SSE";
inline constexpr std::string_view kProtocolLayerHttpsSseFailFastMessage =
    "HTTPS/SSE protocol layer is not implemented yet; set protocol_layer=STDIO.";

struct future_learning_target_t {
  std::string_view include_path;
  std::string_view summary;
};

inline constexpr std::size_t kFutureLearningTargetCount =
0
#define HERO_FUTURE_TARGET(ID, INCLUDE_PATH, SUMMARY) +1
#include "hero/config_hero/hero.config.def"
#undef HERO_FUTURE_TARGET
;

inline constexpr std::array<future_learning_target_t, kFutureLearningTargetCount> kFutureLearningTargets = {{
#define HERO_FUTURE_TARGET(ID, INCLUDE_PATH, SUMMARY) \
  future_learning_target_t{INCLUDE_PATH, SUMMARY},
#include "hero/config_hero/hero.config.def"
#undef HERO_FUTURE_TARGET
}};

struct runtime_key_descriptor_t {
  std::string_view key;
  std::string_view type;
  bool required;
  std::string_view default_value;
  std::string_view range;
  std::string_view allowed;
  std::string_view summary;
};

inline constexpr std::size_t kRuntimeKeyDescriptorCount =
0
#define HERO_RUNTIME_KEY(KEY, TYPE, REQUIRED, DEFAULT_VALUE, RANGE, ALLOWED, SUMMARY) +1
#include "hero/config_hero/hero.config.def"
#undef HERO_RUNTIME_KEY
;

inline constexpr std::array<runtime_key_descriptor_t, kRuntimeKeyDescriptorCount> kRuntimeKeyDescriptors = {{
#define HERO_RUNTIME_KEY(KEY, TYPE, REQUIRED, DEFAULT_VALUE, RANGE, ALLOWED, SUMMARY) \
  runtime_key_descriptor_t{KEY, TYPE, ((REQUIRED) != 0), DEFAULT_VALUE, RANGE, ALLOWED, SUMMARY},
#include "hero/config_hero/hero.config.def"
#undef HERO_RUNTIME_KEY
}};

inline constexpr std::string_view kRuntimeConfigDslTemplateText = R"(
# HERO runtime deterministic config (typed key-value with domain metadata)
protocol_layer[STDIO|HTTPS/SSE]:str = STDIO # STDIO | HTTPS/SSE (HTTPS/SSE not implemented yet)
config_scope_root:str = /cuwacunu/src/config
allow_local_read:bool = true
allow_local_write:bool = false
write_roots:str = /tmp
backup_enabled:bool = true
backup_dir:str = /cuwacunu/.backups/hero.config
backup_max_entries(1,+inf):int = 20
)";

}  // namespace config
}  // namespace hero
}  // namespace cuwacunu
