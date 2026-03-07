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

enum class backend_mode_e : std::uint8_t {
  Openai,
  Selfhosted,
};

inline constexpr std::string_view kBackendModeOpenai = "openai";
inline constexpr std::string_view kBackendModeSelfhosted = "selfhosted";
inline constexpr std::string_view kSelfHostedModeFailFastMessage =
    "isufficient founds for self hosted model deployment, please change mode to openai.";
inline constexpr std::string_view kProtocolLayerStdio = "STDIO";
inline constexpr std::string_view kProtocolLayerHttpsSse = "HTTPS/SSE";
inline constexpr std::string_view kProtocolLayerHttpsSseFailFastMessage =
    "HTTPS/SSE protocol layer is not implemented yet; set protocol_layer=STDIO.";

[[nodiscard]] inline backend_mode_e parse_backend_mode_or_throw(
    std::string_view mode_token) {
  if (mode_token == kBackendModeOpenai) {
    return backend_mode_e::Openai;
  }
  if (mode_token == kBackendModeSelfhosted) {
    return backend_mode_e::Selfhosted;
  }
  throw std::runtime_error(
      "Unsupported HERO mode. Allowed values: openai|selfhosted.");
}

inline void validate_backend_mode_or_throw(std::string_view mode_token) {
  const backend_mode_e parsed = parse_backend_mode_or_throw(mode_token);
  if (parsed == backend_mode_e::Selfhosted) {
    throw std::runtime_error(kSelfHostedModeFailFastMessage.data());
  }
}

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
    "curl",
    "/cuwacunu/src/config",
    "https://api.openai.com/v1/responses",
    "OPENAI_API_KEY",
    "gpt-5-mini",
    "medium",
    0.20,
    1.00,
    1400,
    true,
    true,
    false,
    "/tmp",
};

inline constexpr profile_t kFixProfile{
    "hero.config.fix",
    profile_mode_e::Fix,
    "curl",
    "/cuwacunu/src/config",
    "https://api.openai.com/v1/responses",
    "OPENAI_API_KEY",
    "gpt-5",
    "medium",
    0.00,
    1.00,
    2200,
    true,
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
    transport                     = curl
    config_scope_root             = /cuwacunu/src/config

[HTTP]
    method                        = POST
    endpoint                      = https://api.openai.com/v1/responses
    auth_header_name              = Authorization
    auth_header_prefix            = Bearer
    auth_token_env                = OPENAI_API_KEY
    content_type                  = application/json
    timeout_ms                    = 30000
    connect_timeout_ms            = 10000
    retry_max_attempts            = 3
    retry_backoff_ms              = 700
    verify_tls                    = true
    user_agent                    = cuwacunu-hero-ask/0.1

[MODEL]
    provider                      = openai
    model                         = gpt-5-mini
    reasoning_effort              = medium
    temperature                   = 0.20
    top_p                         = 1.00
    max_output_tokens             = 1400

[POLICY]
    allow_network                 = true
    allow_local_read              = true
    allow_local_write             = false
    write_roots                   = /tmp
    require_curl_only             = true
    require_config_only_context   = true
    require_sources               = true

[PROMPT]
    system_contract               = hero.ask.v1
    task_style                    = analyze_and_propose
    response_contract             = concise_findings_then_next_actions
    forbid_full_file_rewrites     = true
)";

inline constexpr std::string_view kFixProfileConfigText = R"([HERO]
    profile_name                  = hero.config.fix
    mode                          = fix
    enabled                       = true
    transport                     = curl
    config_scope_root             = /cuwacunu/src/config

[HTTP]
    method                        = POST
    endpoint                      = https://api.openai.com/v1/responses
    auth_header_name              = Authorization
    auth_header_prefix            = Bearer
    auth_token_env                = OPENAI_API_KEY
    content_type                  = application/json
    timeout_ms                    = 45000
    connect_timeout_ms            = 12000
    retry_max_attempts            = 4
    retry_backoff_ms              = 900
    verify_tls                    = true
    user_agent                    = cuwacunu-hero-fix/0.1

[MODEL]
    provider                      = openai
    model                         = gpt-5
    reasoning_effort              = medium
    temperature                   = 0.00
    top_p                         = 1.00
    max_output_tokens             = 2200

[POLICY]
    allow_network                 = true
    allow_local_read              = true
    allow_local_write             = true
    write_roots                   = /cuwacunu/src/config,/tmp
    require_curl_only             = true
    require_config_only_context   = true
    require_patch_plan            = true
    require_atomic_edits          = true

[PROMPT]
    system_contract               = hero.fix.v1
    task_style                    = patch_with_validation
    response_contract             = patch_summary_plus_risks
    forbid_destructive_commands   = true
)";

inline constexpr std::string_view kRuntimeConfigDslTemplateText = R"(
# HERO runtime model backend config (typed key-value)
mode:str = openai # openai | selfhosted (selfhosted currently fails fast)
protocol_layer:str = STDIO # STDIO | HTTPS/SSE (HTTPS/SSE not implemented yet)
transport:str = curl
endpoint:str = https://api.openai.com/v1/responses
auth_token_env:str = OPENAI_API_KEY
model:str = gpt-5-mini
reasoning_effort:str = medium # low | medium | high
temperature:float = 0.20
top_p:float = 1.00
max_output_tokens:int = 1400
timeout_ms:int = 30000
connect_timeout_ms:int = 10000
retry_max_attempts:int = 3
retry_backoff_ms:int = 700
verify_tls:bool = true
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
