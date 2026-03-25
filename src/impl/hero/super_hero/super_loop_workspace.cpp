// SPDX-License-Identifier: MIT
#include "hero/super_hero/super_loop_workspace.h"

#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

#include "hero/runtime_hero/runtime_job.h"

namespace cuwacunu {
namespace hero {
namespace super_mcp {

namespace {

[[nodiscard]] bool find_instructions_root(
    const std::filesystem::path& source_super_objective_dsl_path,
    std::filesystem::path* out_root, std::string* error) {
  if (error) error->clear();
  if (!out_root) {
    if (error) *error = "instructions root output pointer is null";
    return false;
  }
  out_root->clear();

  std::filesystem::path current = source_super_objective_dsl_path.parent_path();
  while (!current.empty()) {
    if (current.filename() == "instructions") {
      *out_root = current;
      return true;
    }
    const std::filesystem::path parent = current.parent_path();
    if (parent == current) break;
    current = parent;
  }
  if (error) {
    *error = "cannot derive instructions root from super objective path: " +
             source_super_objective_dsl_path.string();
  }
  return false;
}

[[nodiscard]] bool copy_tree_recursive(const std::filesystem::path& source,
                                       const std::filesystem::path& target,
                                       std::string* error) {
  if (error) error->clear();
  std::error_code ec{};
  if (!std::filesystem::exists(source, ec) || ec) {
    if (error) *error = "copy source does not exist: " + source.string();
    return false;
  }
  std::filesystem::create_directories(target.parent_path(), ec);
  if (ec) {
    if (error) *error = "cannot create copy target parent: " + target.string();
    return false;
  }
  std::filesystem::copy(
      source, target,
      std::filesystem::copy_options::recursive |
          std::filesystem::copy_options::overwrite_existing,
      ec);
  if (ec) {
    if (error) {
      *error = "cannot copy tree from " + source.string() + " to " +
               target.string();
    }
    return false;
  }
  return true;
}

[[nodiscard]] std::string resolve_super_loop_config_scope_root(
    const super_loop_workspace_context_t& context) {
  std::string scope_root = context.config_scope_root.string();
  if (scope_root.empty()) {
    scope_root = context.global_config_path.parent_path().string();
  }
  if (scope_root.empty() && !context.repo_root.empty()) {
    scope_root = (context.repo_root / "src/config").string();
  }
  return scope_root;
}

[[nodiscard]] std::string build_super_loop_config_hero_dsl(
    const super_loop_workspace_context_t& context,
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  const std::filesystem::path backup_dir =
      std::filesystem::path(loop.loop_root) / ".hero_config_backups";
  std::ostringstream out;
  out << "/* super-generated Config Hero policy for a SUPER loop */\n"
      << "protocol_layer[STDIO|HTTPS/SSE]:str = STDIO\n"
      << "config_scope_root:str = "
      << resolve_super_loop_config_scope_root(context) << "\n"
      << "allow_local_read:bool = true\n"
      << "allow_local_write:bool = true\n"
      << "write_roots:str = " << loop.objective_root << "\n"
      << "backup_enabled:bool = false\n"
      << "backup_dir:str = " << backup_dir.string() << "\n"
      << "backup_max_entries(1,+inf):int = 1\n";
  return out.str();
}

[[nodiscard]] std::string build_default_super_briefing(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  std::ostringstream out;
  out << "You are reviewing a SUPER loop.\n\n"
      << "Primary files:\n"
      << "- Super objective DSL: " << loop.super_objective_dsl_path << "\n"
      << "- Super objective prompt: " << loop.super_objective_prompt_path << "\n"
      << "- Config Hero policy: " << loop.config_hero_dsl_path << "\n"
      << "- Memory: " << loop.memory_path << "\n"
      << "- Review packet: "
      << (std::filesystem::path(loop.loop_root) / "review_packet.latest.json").string()
      << "\n"
      << "- Human request artifact: " << loop.human_request_path << "\n"
      << "- Copied instructions root: " << loop.instructions_root << "\n"
      << "- Active objective root: " << loop.objective_root << "\n"
      << "- Mutable objective root: " << loop.objective_root << "\n\n"
      << "The review packet includes phase = prelaunch or postcampaign.\n\n"
      << "Rules:\n"
      << "1. Work in read-only shell mode. Do not edit files directly.\n"
      << "2. Follow the declared super objective DSL, prompt, and copied campaign bundle.\n"
      << "3. When a loop-local objective .dsl change is justified, use the "
         "hero.config.objective_dsl.read/replace MCP tools directly.\n"
      << "4. When the execution plan itself must change, use the "
         "hero.config.objective_campaign.read/replace MCP tools directly.\n"
      << "5. Prefer whole-file replace with expected_sha256 from the prior read.\n"
      << "6. Pass objective_root=" << loop.objective_root
      << " to those Config Hero tools.\n"
      << "7. Never mutate the copied super.objective.dsl constitution or files "
         "outside the mutable objective root.\n"
      << "8. Prefer the review packet first, then hero.lattice.get_view/get_fact "
         "for semantic evidence; use Runtime tails mainly for operational "
         "debugging.\n"
      << "9. Use hero.lattice.list_views/list_facts when you need to discover "
         "selectors; family_evaluation_report requires a family canonical_path "
         "plus contract_hash.\n"
      << "10. Shell exec is unavailable in this environment. Prefer the embedded "
         "review packet and copied campaign contents when present; use MCP tools "
         "instead of shell reads.\n"
      << "11. If you change any objective-local .dsl, describe the actual changes in "
         "memory_note.\n"
      << "12. Prefer stopping when evidence is weak or human judgment is needed.\n"
      << "13. Return only JSON matching the provided output schema. Do not return "
         "edit payloads or tool transcripts.\n\n"
      << "Allowed control_kind values:\n"
      << "- continue\n"
      << "- stop\n"
      << "- need_human\n\n"
      << "Always include next_action with kind as one of:\n"
      << "- none\n"
      << "- default_plan\n"
      << "- binding\n"
      << "Use next_action.kind = none for stop and need_human decisions.\n"
      << "When next_action.kind is binding, include next_action.target_binding_id.\n"
      << "Always include next_action.reset_runtime_state as true or false.\n"
      << "Always include memory_note and human_request as strings; use empty string "
         "when not applicable.\n"
      << "When control_kind is need_human, include human_request with a concise "
         "operator-facing note.\n";
  return out.str();
}

[[nodiscard]] std::string build_initial_super_loop_memory(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  return "# Super Loop Memory\n\n"
         "Loop ID: " +
         loop.loop_id + "\n\nObjective: " + loop.objective_name +
         "\n\nCurrent stance:\n- Follow the declared super objective DSL and prompt.\n"
         "- Prefer a clean stop or need_human when the evidence is ambiguous.\n";
}

}  // namespace

bool copy_super_loop_instruction_bundle(
    const std::filesystem::path& source_super_objective_dsl_path,
    const std::filesystem::path& target_instructions_root,
    std::filesystem::path* out_objective_root, std::string* error) {
  if (error) error->clear();
  if (!out_objective_root) {
    if (error) *error = "objective root output pointer is null";
    return false;
  }
  out_objective_root->clear();

  std::filesystem::path instructions_root{};
  if (!find_instructions_root(source_super_objective_dsl_path, &instructions_root,
                              error)) {
    return false;
  }
  const std::filesystem::path source_defaults_root = instructions_root / "defaults";
  const std::filesystem::path target_defaults_root =
      target_instructions_root / "defaults";
  if (!copy_tree_recursive(source_defaults_root, target_defaults_root, error)) {
    return false;
  }

  std::error_code rel_ec{};
  const std::filesystem::path objective_rel_dir = std::filesystem::relative(
      source_super_objective_dsl_path.parent_path(), instructions_root, rel_ec);
  if (rel_ec || objective_rel_dir.empty()) {
    if (error) {
      *error = "cannot derive objective bundle relative path from " +
               source_super_objective_dsl_path.string();
    }
    return false;
  }
  const std::filesystem::path target_objective_root =
      target_instructions_root / objective_rel_dir;
  if (!copy_tree_recursive(source_super_objective_dsl_path.parent_path(),
                           target_objective_root, error)) {
    return false;
  }
  *out_objective_root = target_objective_root.lexically_normal();
  return true;
}

std::string derive_super_loop_objective_name(
    const std::filesystem::path& source_super_objective_dsl_path) {
  const std::string folder =
      source_super_objective_dsl_path.parent_path().filename().string();
  if (!folder.empty()) return folder;
  const std::string stem = source_super_objective_dsl_path.stem().string();
  return stem.empty() ? "runtime-super-loop" : stem;
}

bool write_super_loop_bootstrap_files(
    const super_loop_workspace_context_t& context,
    const std::filesystem::path& source_super_objective_dsl_path,
    const std::filesystem::path& source_super_objective_prompt_path,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::string* error) {
  if (error) error->clear();

  std::string objective_dsl_text{};
  if (!cuwacunu::hero::runtime::read_text_file(source_super_objective_dsl_path,
                                               &objective_dsl_text, error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          loop.super_objective_dsl_path, objective_dsl_text, error)) {
    return false;
  }

  std::string objective_prompt_text{};
  if (!cuwacunu::hero::runtime::read_text_file(source_super_objective_prompt_path,
                                               &objective_prompt_text, error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          loop.super_objective_prompt_path, objective_prompt_text, error)) {
    return false;
  }
  if (!refresh_super_loop_config_hero_dsl(context, loop, error)) return false;

  std::string briefing_text = build_default_super_briefing(loop);
  if (!cuwacunu::hero::runtime::trim_ascii(objective_dsl_text).empty()) {
    briefing_text.append("\n\nSuper objective DSL contents:\n");
    briefing_text.append(objective_dsl_text);
    if (briefing_text.back() != '\n') briefing_text.push_back('\n');
  }
  if (!cuwacunu::hero::runtime::trim_ascii(objective_prompt_text).empty()) {
    briefing_text.append("\n\nSuper objective prompt contents:\n");
    briefing_text.append(objective_prompt_text);
    if (briefing_text.back() != '\n') briefing_text.push_back('\n');
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(loop.briefing_path,
                                                       briefing_text, error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          loop.memory_path, build_initial_super_loop_memory(loop), error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(loop.human_request_path, "",
                                                       error)) {
    return false;
  }

  std::error_code ec{};
  std::filesystem::create_directories(
      cuwacunu::hero::super::super_loop_reviews_dir(context.super_root,
                                                              loop.loop_id),
      ec);
  if (ec) {
    if (error) *error = "cannot create super loop reviews dir";
    return false;
  }
  std::filesystem::create_directories(
      cuwacunu::hero::super::super_loop_decisions_dir(context.super_root,
                                                                loop.loop_id),
      ec);
  if (ec) {
    if (error) *error = "cannot create super loop decisions dir";
    return false;
  }
  return true;
}

bool refresh_super_loop_config_hero_dsl(
    const super_loop_workspace_context_t& context,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::string* error) {
  if (error) error->clear();
  const std::filesystem::path config_hero_dsl_path =
      loop.config_hero_dsl_path.empty()
          ? cuwacunu::hero::super::super_loop_config_hero_path(
                context.super_root, loop.loop_id)
          : std::filesystem::path(loop.config_hero_dsl_path);
  return cuwacunu::hero::runtime::write_text_file_atomic(
      config_hero_dsl_path, build_super_loop_config_hero_dsl(context, loop), error);
}

std::filesystem::path resolve_config_hero_mcp_binary(
    const super_loop_workspace_context_t& context) {
  std::error_code ec{};
  if (!context.self_binary_path.empty()) {
    const std::filesystem::path sibling =
        context.self_binary_path.parent_path() / "hero_config_mcp";
    if (std::filesystem::exists(sibling, ec) &&
        std::filesystem::is_regular_file(sibling, ec)) {
      return sibling;
    }
  }
  if (!context.repo_root.empty()) {
    return (context.repo_root / ".build/hero/hero_config_mcp").lexically_normal();
  }
  return {};
}

}  // namespace super_mcp
}  // namespace hero
}  // namespace cuwacunu
