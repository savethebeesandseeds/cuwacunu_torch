#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace cuwacunu::hero::runtime {

inline constexpr std::string_view kProtocolLayerStdio = "STDIO";
inline constexpr std::string_view kProtocolLayerHttpsSse = "HTTPS/SSE";
inline constexpr std::string_view kProtocolLayerHttpsSseFailFastMessage =
    "HTTPS/SSE protocol layer is not implemented yet; set "
    "protocol_layer=STDIO.";

struct runtime_policy_descriptor_t {
  std::string_view key;
  std::string_view type;
  bool required;
  std::string_view default_value;
  std::string_view range;
  std::string_view allowed;
  std::string_view summary;
};

inline constexpr std::size_t kRuntimePolicyDescriptorCount = 0
#define HERO_RUNTIME_POLICY_KEY(KEY, TYPE, REQUIRED, DEFAULT_VALUE, RANGE,     \
                                ALLOWED, SUMMARY)                              \
  +1
#include "hero/runtime_hero/hero_runtime.def"
#undef HERO_RUNTIME_POLICY_KEY
    ;

inline constexpr std::array<runtime_policy_descriptor_t,
                            kRuntimePolicyDescriptorCount>
    kRuntimePolicyDescriptors = {{
#define HERO_RUNTIME_POLICY_KEY(KEY, TYPE, REQUIRED, DEFAULT_VALUE, RANGE,     \
                                ALLOWED, SUMMARY)                              \
  runtime_policy_descriptor_t{                                                 \
      KEY, TYPE, ((REQUIRED) != 0), DEFAULT_VALUE, RANGE, ALLOWED, SUMMARY},
#include "hero/runtime_hero/hero_runtime.def"
#undef HERO_RUNTIME_POLICY_KEY
    }};

inline constexpr std::string_view kRuntimePolicyTemplateText = R"(
# Runtime Hero v2 policy
protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO
runtime_profile:enum = locked_default
runtime_exec_path:path = /cuwacunu/.build/exec/cuwacunu_exec
default_config_path:path = /cuwacunu/src/config/.config
runtime_root:path = /cuwacunu/.runtime/cuwacunu_exec
allowed_job_roots:path_list = /cuwacunu/.runtime/cuwacunu_exec,/tmp
allow_force_rebuild_cache:bool = false
require_confirm_execute:bool = true
require_confirm_dev_nuke:bool = true
dev_nuke_backup_enabled:bool = false
dev_nuke_backup_root:path = /tmp/cuwacunu_runtime_dev_nuke_backups/hero.runtime_dev_nuke
allowed_dev_nuke_roots:path_list = /cuwacunu/.runtime
max_capture_bytes:int = 65536
max_runtime_seconds:int = 600

RUNTIME_PROFILE locked_default {
  default_dry_run:bool = true
  allow_execute:bool = false
  allow_train_execute:bool = false
  allow_dev_nuke:bool = false
  max_runtime_seconds:int = 600
}
)";

} // namespace cuwacunu::hero::runtime
