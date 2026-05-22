#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace cuwacunu::hero::config {

inline constexpr std::string_view kProtocolLayerStdio = "STDIO";
inline constexpr std::string_view kProtocolLayerHttpsSse = "HTTPS/SSE";
inline constexpr std::string_view kProtocolLayerHttpsSseFailFastMessage =
    "HTTPS/SSE protocol layer is not implemented yet; set "
    "protocol_layer=STDIO.";

struct runtime_key_descriptor_t {
  std::string_view key;
  std::string_view type;
  bool required;
  std::string_view default_value;
  std::string_view range;
  std::string_view allowed;
  std::string_view summary;
};

inline constexpr std::size_t kRuntimeKeyDescriptorCount = 0
#define HERO_CONFIG_RUNTIME_KEY(KEY, TYPE, REQUIRED, DEFAULT_VALUE, RANGE,     \
                                ALLOWED, SUMMARY)                              \
  +1
#include "hero/config_hero/hero_config.def"
#undef HERO_CONFIG_RUNTIME_KEY
    ;

inline constexpr std::array<runtime_key_descriptor_t,
                            kRuntimeKeyDescriptorCount>
    kRuntimeKeyDescriptors = {{
#define HERO_CONFIG_RUNTIME_KEY(KEY, TYPE, REQUIRED, DEFAULT_VALUE, RANGE,     \
                                ALLOWED, SUMMARY)                              \
  runtime_key_descriptor_t{                                                    \
      KEY, TYPE, ((REQUIRED) != 0), DEFAULT_VALUE, RANGE, ALLOWED, SUMMARY},
#include "hero/config_hero/hero_config.def"
#undef HERO_CONFIG_RUNTIME_KEY
    }};

inline constexpr std::string_view kRuntimeConfigDslTemplateText = R"(
# Config Hero v2 policy
protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO
config_root:path = /cuwacunu/src/config
managed_roots:path_list = /cuwacunu/src/config
allow_read:bool = true
allow_write:bool = false
write_roots:path_list = /tmp
allowed_extensions:string_list = .config,.dsl,.bnf,.man,.md,.net,.jkimyei
backup_enabled:bool = true
backup_dir:path = /cuwacunu/.backups/hero.config
backup_max_entries:int = 20
require_expected_sha256_for_replace:bool = true
require_expected_sha256_for_delete:bool = true
)";

} // namespace cuwacunu::hero::config
