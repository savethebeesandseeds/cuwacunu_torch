// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <string>

#include "hero/super_hero/super_loop.h"

namespace cuwacunu {
namespace hero {
namespace super_mcp {

// These helpers own the Super Hero loop's private workspace concerns so the
// main Super Hero tool surface can stay focused on orchestration.
struct super_loop_workspace_context_t {
  std::filesystem::path global_config_path{};
  std::filesystem::path self_binary_path{};
  std::filesystem::path super_root{};
  std::filesystem::path repo_root{};
  std::filesystem::path config_scope_root{};
};

[[nodiscard]] std::string derive_super_loop_objective_name(
    const std::filesystem::path& source_super_objective_dsl_path);

[[nodiscard]] bool write_super_loop_bootstrap_files(
    const super_loop_workspace_context_t& context,
    const std::filesystem::path& source_super_objective_dsl_path,
    const std::filesystem::path& source_super_objective_md_path,
    const std::filesystem::path& source_super_guidance_md_path,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::string* error);

[[nodiscard]] bool refresh_super_loop_config_policy_dsl(
    const super_loop_workspace_context_t& context,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::string* error);

[[nodiscard]] std::filesystem::path resolve_config_hero_mcp_binary(
    const super_loop_workspace_context_t& context);

}  // namespace super_mcp
}  // namespace hero
}  // namespace cuwacunu
