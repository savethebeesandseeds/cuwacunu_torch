#pragma once

#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "hero/config_hero/hero_config_tools.h"
#include "hero/runtime_dev_loop.h"

namespace cuwacunu::hero::mcp::detail {

inline constexpr const char* kMcpServerName = "hero_config_mcp";
inline constexpr const char* kMcpServerVersion = "0.5.0";
inline constexpr const char* kDefaultMcpProtocolVersion = "2025-03-26";
inline constexpr const char* kMcpInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.config.*.";
inline constexpr const char* kConfigDslScopeErrorTag = "E_CONFIG_DSL_SCOPE";
inline constexpr int kConfigDslScopeErrorCode = -32041;
inline constexpr const char* kConfigWritePolicyErrorTag =
    "E_CONFIG_WRITE_POLICY";
inline constexpr int kConfigWritePolicyErrorCode = -32042;
inline constexpr const char* kDefaultGlobalConfigPath =
    "/cuwacunu/src/config/.config";

enum class dsl_path_resolution_error_t {
  kNone = 0,
  kOutputPointerNull,
  kEmptyPath,
  kEscapesScope,
  kHashimyeiPath,
  kNotDefaultInstructionDsl,
  kEmptyObjectiveRoot,
  kObjectiveRootMissing,
  kObjectiveRootEscapesAllowedScopes,
  kNotObjectiveInstructionRoot,
  kEscapesObjectiveRoot,
  kNotObjectiveInstructionDsl,
};

enum class instruction_dsl_validation_family_e : std::uint8_t;

using hero_config_tool_handler_t = bool (*)(
    std::string_view tool_name, const std::string& request_json,
    hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message);

struct hero_config_tool_descriptor_t {
  std::string_view name;
  std::string_view description;
  std::string_view input_schema_json;
  hero_config_tool_handler_t handler;
};

[[nodiscard]] bool write_all_fd(int fd, const void* bytes, std::size_t size);
[[nodiscard]] std::string trim_ascii(std::string_view in);
[[nodiscard]] std::string lowercase_copy(std::string_view in);
[[nodiscard]] bool parse_bool_ascii(std::string_view raw, bool* out);
[[nodiscard]] bool read_text_file(std::string_view path, std::string* out,
                                  std::string* err);
[[nodiscard]] bool write_text_file_atomic(std::string_view path,
                                          std::string_view content,
                                          std::string* err);
[[nodiscard]] bool read_ini_general_key(std::string_view ini_path,
                                        std::string_view key,
                                        std::string* out_value);
[[nodiscard]] std::string path_vector_json(
    const std::vector<std::filesystem::path>& paths);
[[nodiscard]] std::string json_quote(std::string_view in);
[[nodiscard]] std::string bool_json(bool v);
[[nodiscard]] std::filesystem::path resolve_path_near_config(
    std::string_view raw_path, std::string_view config_path);
[[nodiscard]] std::string normalized_path_key(
    const std::filesystem::path& path);
[[nodiscard]] bool sha256_hex_file(const std::filesystem::path& path,
                                   std::string* out_hex, std::string* err);
[[nodiscard]] std::filesystem::path resolve_config_root_from_global_config(
    const hero_config_store_t& store);
[[nodiscard]] instruction_dsl_validation_family_e
detect_instruction_dsl_validation_family(const std::filesystem::path& dsl_path);
[[nodiscard]] std::string instruction_dsl_validation_family_name(
    instruction_dsl_validation_family_e family);
[[nodiscard]] std::filesystem::path find_associated_man_path_with_fallback(
    const std::filesystem::path& primary_config_root,
    const std::filesystem::path& dsl_path,
    instruction_dsl_validation_family_e family);
[[nodiscard]] bool append_associated_man_fields(
    std::ostringstream* out, const std::filesystem::path& man_path,
    bool include_content, std::string_view warning, std::string* out_error);
[[nodiscard]] std::string missing_associated_man_warning(
    const std::filesystem::path& dsl_path);
void log_config_warning(std::string_view warning);
[[nodiscard]] bool validate_instruction_dsl_replacement(
    const hero_config_store_t& store, const std::filesystem::path& dsl_path,
    std::string_view replacement_text, std::string* out_family_name,
    std::filesystem::path* out_grammar_path, std::string* out_error);
[[nodiscard]] bool validate_default_dsl_replacement_semantics(
    const hero_config_store_t& store, const std::filesystem::path& dsl_path,
    std::string_view replacement_text, std::string* out_error);

[[nodiscard]] bool extract_json_raw_field(const std::string& object_json,
                                          const std::string& key,
                                          std::string* out_raw_json);
[[nodiscard]] bool extract_json_object_field(const std::string& object_json,
                                             const std::string& key,
                                             std::string* out_object_json);
[[nodiscard]] bool extract_json_string_field(const std::string& object_json,
                                             const std::string& key,
                                             std::string* out_value);
[[nodiscard]] bool extract_json_bool_field(const std::string& object_json,
                                           const std::string& key,
                                           bool* out_value);

[[nodiscard]] bool path_is_within(std::filesystem::path base,
                                  std::filesystem::path candidate);
[[nodiscard]] bool is_scope_violation(dsl_path_resolution_error_t err);
[[nodiscard]] bool path_has_component(const std::filesystem::path& path,
                                      std::string_view component);
[[nodiscard]] bool is_campaign_dsl_path(const std::filesystem::path& path);
[[nodiscard]] bool is_super_objective_dsl_path(
    const std::filesystem::path& path);
[[nodiscard]] bool split_csv_tokens(std::string_view raw,
                                    std::vector<std::string>* out_tokens);
[[nodiscard]] bool collect_configured_root_paths(
    const hero_config_store_t& store, std::string_view key,
    std::vector<std::filesystem::path>* out_roots, std::string* out_error);
[[nodiscard]] bool collect_allowed_extensions(
    const hero_config_store_t& store,
    std::vector<std::string>* out_extensions, std::string* out_error);
[[nodiscard]] bool path_has_allowed_extension(
    const std::filesystem::path& path,
    const std::vector<std::string>& allowed_extensions);
[[nodiscard]] bool path_is_within_any_root(
    const std::vector<std::filesystem::path>& roots,
    const std::filesystem::path& candidate);
[[nodiscard]] bool resolve_default_dsl_path_with_scope(
    const hero_config_store_t& store, std::string_view request_path,
    std::filesystem::path* out_path, std::string* out_error,
    dsl_path_resolution_error_t* out_reason);
[[nodiscard]] bool resolve_objective_root_with_scope(
    const hero_config_store_t& store, std::string_view request_root,
    std::filesystem::path* out_root, std::string* out_error,
    dsl_path_resolution_error_t* out_reason);
[[nodiscard]] bool resolve_objective_path_with_scope(
    const hero_config_store_t& store, std::string_view request_root,
    std::string_view raw_path, bool allow_missing_target,
    std::filesystem::path* out_path, std::string* out_error,
    dsl_path_resolution_error_t* out_reason);
[[nodiscard]] std::string make_write_policy_error(std::string_view detail);
[[nodiscard]] bool read_bool_config_key_or_default(
    const hero_config_store_t& store, std::string_view key, bool default_value,
    bool* out_value, std::string* out_error);
[[nodiscard]] bool collect_allowed_write_roots(
    const hero_config_store_t& store,
    std::vector<std::filesystem::path>* out_roots, std::string* out_error);
[[nodiscard]] bool list_instruction_files_under_root(
    const std::filesystem::path& root,
    const std::vector<std::string>& allowed_extensions,
    std::vector<std::filesystem::path>* out_paths, std::string* out_error);
[[nodiscard]] bool enforce_write_target_allowed(
    const hero_config_store_t& store, const std::filesystem::path& raw_target,
    std::string* out_error);
[[nodiscard]] bool read_int64_config_key_or_default(
    const hero_config_store_t& store, std::string_view key,
    std::int64_t default_value, std::int64_t* out_value,
    std::string* out_error);
[[nodiscard]] bool backup_previous_write_target_with_cap(
    const hero_config_store_t& store, const std::filesystem::path& source_path,
    std::string* out_error);
[[nodiscard]] bool enforce_runtime_config_write_policy(
    const hero_config_store_t& store, std::string* out_error);
[[nodiscard]] bool enforce_runtime_reset_targets_write_policy(
    const hero_config_store_t& store,
    const cuwacunu::hero::runtime_dev::runtime_reset_targets_t& targets,
    std::string* out_error);

[[nodiscard]] bool handle_tool_status(std::string_view tool_name,
                                      const std::string& request_json,
                                      hero_config_store_t* store,
                                      std::string* out_result_json,
                                      int* out_error_code,
                                      std::string* out_error_message);
[[nodiscard]] bool handle_tool_schema(std::string_view tool_name,
                                      const std::string& request_json,
                                      hero_config_store_t* store,
                                      std::string* out_result_json,
                                      int* out_error_code,
                                      std::string* out_error_message);
[[nodiscard]] bool handle_tool_show(std::string_view tool_name,
                                    const std::string& request_json,
                                    hero_config_store_t* store,
                                    std::string* out_result_json,
                                    int* out_error_code,
                                    std::string* out_error_message);
[[nodiscard]] bool handle_tool_get(std::string_view tool_name,
                                   const std::string& request_json,
                                   hero_config_store_t* store,
                                   std::string* out_result_json,
                                   int* out_error_code,
                                   std::string* out_error_message);
[[nodiscard]] bool handle_tool_set(std::string_view tool_name,
                                   const std::string& request_json,
                                   hero_config_store_t* store,
                                   std::string* out_result_json,
                                   int* out_error_code,
                                   std::string* out_error_message);
[[nodiscard]] bool handle_tool_default_read(std::string_view tool_name,
                                            const std::string& request_json,
                                            hero_config_store_t* store,
                                            std::string* out_result_json,
                                            int* out_error_code,
                                            std::string* out_error_message);
[[nodiscard]] bool handle_tool_default_list(std::string_view tool_name,
                                            const std::string& request_json,
                                            hero_config_store_t* store,
                                            std::string* out_result_json,
                                            int* out_error_code,
                                            std::string* out_error_message);
[[nodiscard]] bool handle_tool_default_create(std::string_view tool_name,
                                              const std::string& request_json,
                                              hero_config_store_t* store,
                                              std::string* out_result_json,
                                              int* out_error_code,
                                              std::string* out_error_message);
[[nodiscard]] bool handle_tool_default_replace(std::string_view tool_name,
                                               const std::string& request_json,
                                               hero_config_store_t* store,
                                               std::string* out_result_json,
                                               int* out_error_code,
                                               std::string* out_error_message);
[[nodiscard]] bool handle_tool_default_delete(std::string_view tool_name,
                                              const std::string& request_json,
                                              hero_config_store_t* store,
                                              std::string* out_result_json,
                                              int* out_error_code,
                                              std::string* out_error_message);
[[nodiscard]] bool handle_tool_objective_read(std::string_view tool_name,
                                              const std::string& request_json,
                                              hero_config_store_t* store,
                                              std::string* out_result_json,
                                              int* out_error_code,
                                              std::string* out_error_message);
[[nodiscard]] bool handle_tool_objective_list(std::string_view tool_name,
                                              const std::string& request_json,
                                              hero_config_store_t* store,
                                              std::string* out_result_json,
                                              int* out_error_code,
                                              std::string* out_error_message);
[[nodiscard]] bool handle_tool_objective_create(std::string_view tool_name,
                                                const std::string& request_json,
                                                hero_config_store_t* store,
                                                std::string* out_result_json,
                                                int* out_error_code,
                                                std::string* out_error_message);
[[nodiscard]] bool handle_tool_objective_replace(
    std::string_view tool_name, const std::string& request_json,
    hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message);
[[nodiscard]] bool handle_tool_objective_delete(std::string_view tool_name,
                                                const std::string& request_json,
                                                hero_config_store_t* store,
                                                std::string* out_result_json,
                                                int* out_error_code,
                                                std::string* out_error_message);
[[nodiscard]] bool handle_tool_optim_read(std::string_view tool_name,
                                          const std::string& request_json,
                                          hero_config_store_t* store,
                                          std::string* out_result_json,
                                          int* out_error_code,
                                          std::string* out_error_message);
[[nodiscard]] bool handle_tool_optim_list(std::string_view tool_name,
                                          const std::string& request_json,
                                          hero_config_store_t* store,
                                          std::string* out_result_json,
                                          int* out_error_code,
                                          std::string* out_error_message);
[[nodiscard]] bool handle_tool_optim_create(std::string_view tool_name,
                                            const std::string& request_json,
                                            hero_config_store_t* store,
                                            std::string* out_result_json,
                                            int* out_error_code,
                                            std::string* out_error_message);
[[nodiscard]] bool handle_tool_optim_replace(std::string_view tool_name,
                                             const std::string& request_json,
                                             hero_config_store_t* store,
                                             std::string* out_result_json,
                                             int* out_error_code,
                                             std::string* out_error_message);
[[nodiscard]] bool handle_tool_optim_delete(std::string_view tool_name,
                                            const std::string& request_json,
                                            hero_config_store_t* store,
                                            std::string* out_result_json,
                                            int* out_error_code,
                                            std::string* out_error_message);
[[nodiscard]] bool handle_tool_optim_backups(std::string_view tool_name,
                                             const std::string& request_json,
                                             hero_config_store_t* store,
                                             std::string* out_result_json,
                                             int* out_error_code,
                                             std::string* out_error_message);
[[nodiscard]] bool handle_tool_optim_rollback(std::string_view tool_name,
                                              const std::string& request_json,
                                              hero_config_store_t* store,
                                              std::string* out_result_json,
                                              int* out_error_code,
                                              std::string* out_error_message);
[[nodiscard]] bool handle_tool_validate(std::string_view tool_name,
                                        const std::string& request_json,
                                        hero_config_store_t* store,
                                        std::string* out_result_json,
                                        int* out_error_code,
                                        std::string* out_error_message);
[[nodiscard]] bool handle_tool_diff(std::string_view tool_name,
                                    const std::string& request_json,
                                    hero_config_store_t* store,
                                    std::string* out_result_json,
                                    int* out_error_code,
                                    std::string* out_error_message);
[[nodiscard]] bool handle_tool_backups(std::string_view tool_name,
                                       const std::string& request_json,
                                       hero_config_store_t* store,
                                       std::string* out_result_json,
                                       int* out_error_code,
                                       std::string* out_error_message);
[[nodiscard]] bool handle_tool_rollback(std::string_view tool_name,
                                        const std::string& request_json,
                                        hero_config_store_t* store,
                                        std::string* out_result_json,
                                        int* out_error_code,
                                        std::string* out_error_message);
[[nodiscard]] bool handle_tool_save(std::string_view tool_name,
                                    const std::string& request_json,
                                    hero_config_store_t* store,
                                    std::string* out_result_json,
                                    int* out_error_code,
                                    std::string* out_error_message);
[[nodiscard]] bool handle_tool_reload(std::string_view tool_name,
                                      const std::string& request_json,
                                      hero_config_store_t* store,
                                      std::string* out_result_json,
                                      int* out_error_code,
                                      std::string* out_error_message);
[[nodiscard]] bool handle_tool_dev_nuke_reset(std::string_view tool_name,
                                              const std::string& request_json,
                                              hero_config_store_t* store,
                                              std::string* out_result_json,
                                              int* out_error_code,
                                              std::string* out_error_message);

}  // namespace cuwacunu::hero::mcp::detail
