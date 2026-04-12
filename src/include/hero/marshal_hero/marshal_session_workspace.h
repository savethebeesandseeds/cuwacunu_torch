// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <string>

#include "hero/marshal_hero/marshal_session.h"

namespace cuwacunu {
namespace hero {
namespace marshal_mcp {

// These helpers own the Marshal Hero session's private workspace concerns so the
// main Marshal Hero tool surface can stay focused on orchestration.
struct marshal_session_workspace_context_t {
  std::filesystem::path global_config_path{};
  std::filesystem::path self_binary_path{};
  std::filesystem::path marshal_root{};
  std::filesystem::path repo_root{};
  std::filesystem::path config_scope_root{};
};

[[nodiscard]] std::string derive_marshal_session_objective_name(
    const std::filesystem::path& source_marshal_objective_dsl_path);

[[nodiscard]] bool write_marshal_session_bootstrap_files(
    const marshal_session_workspace_context_t& context,
    const std::filesystem::path& source_marshal_objective_dsl_path,
    const std::filesystem::path& source_marshal_objective_md_path,
    const std::filesystem::path& source_marshal_guidance_md_path,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string* error);

[[nodiscard]] bool refresh_marshal_session_config_policy_dsl(
    const marshal_session_workspace_context_t& context,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string* error);

[[nodiscard]] std::filesystem::path resolve_config_hero_mcp_binary(
    const marshal_session_workspace_context_t& context);

}  // namespace marshal_mcp
}  // namespace hero
}  // namespace cuwacunu
