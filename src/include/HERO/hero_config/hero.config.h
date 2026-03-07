#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace cuwacunu {
namespace hero {
namespace config {

enum class profile_mode_e : std::uint8_t {
  Ask,
  Fix,
};

inline constexpr std::string_view kProtocolLayerStdio = "STDIO";
inline constexpr std::string_view kProtocolLayerHttpsSse = "HTTPS/SSE";
inline constexpr std::string_view kProtocolLayerHttpsSseFailFastMessage =
    "HTTPS/SSE protocol layer is not implemented yet; set protocol_layer=STDIO.";

enum class profile_id_e : std::uint8_t {
#define HERO_PROFILE(ID, PROFILE_NAME, MODE_TOKEN, MODEL, SUMMARY) ID,
#include "HERO/hero_config/hero.config.def"
#undef HERO_PROFILE
};

struct profile_descriptor_t {
  profile_id_e id;
  std::string_view profile_name;
  profile_mode_e mode;
  std::string_view model;
  std::string_view summary;
};

inline constexpr std::size_t kProfileDescriptorCount =
0
#define HERO_PROFILE(ID, PROFILE_NAME, MODE_TOKEN, MODEL, SUMMARY) +1
#include "HERO/hero_config/hero.config.def"
#undef HERO_PROFILE
;

inline constexpr std::array<profile_descriptor_t, kProfileDescriptorCount> kProfileDescriptors = {{
#define HERO_PROFILE(ID, PROFILE_NAME, MODE_TOKEN, MODEL, SUMMARY) \
  profile_descriptor_t{profile_id_e::ID, PROFILE_NAME, profile_mode_e::MODE_TOKEN, MODEL, SUMMARY},
#include "HERO/hero_config/hero.config.def"
#undef HERO_PROFILE
}};

struct profile_policy_rule_t {
  profile_id_e profile_id;
  std::string_view key;
  std::string_view value;
  std::string_view summary;
};

inline constexpr std::size_t kProfilePolicyRuleCount =
0
#define HERO_PROFILE(ID, PROFILE_NAME, MODE_TOKEN, MODEL, SUMMARY)
#define HERO_PROFILE_POLICY(PROFILE_ID, KEY, VALUE, SUMMARY) +1
#include "HERO/hero_config/hero.config.def"
#undef HERO_PROFILE_POLICY
#undef HERO_PROFILE
;

inline constexpr std::array<profile_policy_rule_t, kProfilePolicyRuleCount> kProfilePolicyRules = {{
#define HERO_PROFILE(ID, PROFILE_NAME, MODE_TOKEN, MODEL, SUMMARY)
#define HERO_PROFILE_POLICY(PROFILE_ID, KEY, VALUE, SUMMARY) \
  profile_policy_rule_t{profile_id_e::PROFILE_ID, KEY, VALUE, SUMMARY},
#include "HERO/hero_config/hero.config.def"
#undef HERO_PROFILE_POLICY
#undef HERO_PROFILE
}};

struct future_learning_target_t {
  std::string_view include_path;
  std::string_view summary;
};

inline constexpr std::size_t kFutureLearningTargetCount =
0
#define HERO_PROFILE(ID, PROFILE_NAME, MODE_TOKEN, MODEL, SUMMARY)
#define HERO_FUTURE_TARGET(ID, INCLUDE_PATH, SUMMARY) +1
#include "HERO/hero_config/hero.config.def"
#undef HERO_FUTURE_TARGET
#undef HERO_PROFILE
;

inline constexpr std::array<future_learning_target_t, kFutureLearningTargetCount> kFutureLearningTargets = {{
#define HERO_PROFILE(ID, PROFILE_NAME, MODE_TOKEN, MODEL, SUMMARY)
#define HERO_FUTURE_TARGET(ID, INCLUDE_PATH, SUMMARY) \
  future_learning_target_t{INCLUDE_PATH, SUMMARY},
#include "HERO/hero_config/hero.config.def"
#undef HERO_FUTURE_TARGET
#undef HERO_PROFILE
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
#define HERO_PROFILE(ID, PROFILE_NAME, MODE_TOKEN, MODEL, SUMMARY)
#define HERO_RUNTIME_KEY(KEY, TYPE, REQUIRED, DEFAULT_VALUE, RANGE, ALLOWED, SUMMARY) +1
#include "HERO/hero_config/hero.config.def"
#undef HERO_RUNTIME_KEY
#undef HERO_PROFILE
;

inline constexpr std::array<runtime_key_descriptor_t, kRuntimeKeyDescriptorCount> kRuntimeKeyDescriptors = {{
#define HERO_PROFILE(ID, PROFILE_NAME, MODE_TOKEN, MODEL, SUMMARY)
#define HERO_RUNTIME_KEY(KEY, TYPE, REQUIRED, DEFAULT_VALUE, RANGE, ALLOWED, SUMMARY) \
  runtime_key_descriptor_t{KEY, TYPE, ((REQUIRED) != 0), DEFAULT_VALUE, RANGE, ALLOWED, SUMMARY},
#include "HERO/hero_config/hero.config.def"
#undef HERO_RUNTIME_KEY
#undef HERO_PROFILE
}};

struct profile_t {
  std::string_view profile_name;
  profile_mode_e mode;
  std::string_view transport;
  std::string_view config_scope_root;
  std::string_view endpoint;
  std::string_view auth_token_env;
  std::string_view model;
  std::string_view reasoning_effort;
  double temperature;
  double top_p;
  int max_output_tokens;
  bool allow_network;
  bool allow_local_read;
  bool allow_local_write;
  std::string_view write_roots;
};

inline constexpr profile_t kAskProfile{
    "hero.config.ask",
    profile_mode_e::Ask,
    "none",
    "/cuwacunu/src/config",
    "",
    "",
    "deterministic",
    "",
    0.0,
    1.0,
    0,
    false,
    true,
    false,
    "/tmp",
};

inline constexpr profile_t kFixProfile{
    "hero.config.fix",
    profile_mode_e::Fix,
    "none",
    "/cuwacunu/src/config",
    "",
    "",
    "deterministic",
    "",
    0.0,
    1.0,
    0,
    false,
    true,
    true,
    "/cuwacunu/src/config,/tmp",
};

inline constexpr std::array<profile_t, 2> kProfiles{
    kAskProfile,
    kFixProfile,
};

inline constexpr std::string_view kAskProfileConfigText = R"([HERO]
    profile_name                  = hero.config.ask
    mode                          = ask
    enabled                       = true
    transport                     = none
    config_scope_root             = /cuwacunu/src/config

[POLICY]
    deterministic_only            = true
    allow_local_read              = true
    allow_local_write             = false
    write_roots                   = /tmp
    require_config_only_context   = true
)";

inline constexpr std::string_view kFixProfileConfigText = R"([HERO]
    profile_name                  = hero.config.fix
    mode                          = fix
    enabled                       = true
    transport                     = none
    config_scope_root             = /cuwacunu/src/config

[POLICY]
    deterministic_only            = true
    allow_local_read              = true
    allow_local_write             = true
    write_roots                   = /cuwacunu/src/config,/tmp
    require_config_only_context   = true
    require_backup                = true
)";

inline constexpr std::string_view kRuntimeConfigDslTemplateText = R"(
# HERO runtime deterministic config (typed key-value)
protocol_layer:str = STDIO # STDIO | HTTPS/SSE (HTTPS/SSE not implemented yet)
config_scope_root:str = /cuwacunu/src/config
allow_local_read:bool = true
allow_local_write:bool = false
write_roots:str = /tmp
backup_enabled:bool = true
backup_dir:str = /cuwacunu/src/config/backups/hero.config
backup_max_entries:int = 20
)";

}  // namespace config
}  // namespace hero
}  // namespace cuwacunu
