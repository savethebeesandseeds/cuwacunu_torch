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

[[nodiscard]] std::string sanitize_super_backup_component(
    std::string_view raw) {
  std::string out;
  out.reserve(raw.size());
  for (const unsigned char ch : raw) {
    const bool keep = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                      (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' ||
                      ch == '-';
    out.push_back(keep ? static_cast<char>(ch) : '_');
  }
  if (out.empty()) return "unknown_objective";
  return out;
}

[[nodiscard]] std::filesystem::path derive_super_loop_backup_dir(
    const super_loop_workspace_context_t& context,
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  if (!context.repo_root.empty()) {
    return (context.repo_root / ".backups" / "hero.super" /
            sanitize_super_backup_component(loop.objective_name))
        .lexically_normal();
  }
  const std::string scope_root = resolve_super_loop_config_scope_root(context);
  if (!scope_root.empty()) {
    return (std::filesystem::path(scope_root) / ".backups" / "hero.super" /
            sanitize_super_backup_component(loop.objective_name))
        .lexically_normal();
  }
  return (std::filesystem::path(loop.loop_root) / ".hero_config_backups")
      .lexically_normal();
}

[[nodiscard]] std::filesystem::path derive_super_loop_default_root(
    const super_loop_workspace_context_t& context) {
  if (!context.config_scope_root.empty()) {
    return (context.config_scope_root / "instructions" / "defaults")
        .lexically_normal();
  }
  if (!context.repo_root.empty()) {
    return (context.repo_root / "src/config/instructions/defaults")
        .lexically_normal();
  }
  return {};
}

[[nodiscard]] std::string build_super_loop_config_policy_dsl(
    const super_loop_workspace_context_t& context,
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  const std::filesystem::path backup_dir =
      derive_super_loop_backup_dir(context, loop);
  const std::filesystem::path default_root =
      derive_super_loop_default_root(context);
  const bool allow_default_write =
      cuwacunu::hero::runtime::trim_ascii(loop.authority_scope) ==
      "objective_plus_defaults";
  std::ostringstream out;
  out << "/* super-generated Config Hero policy for a SUPER loop */\n"
      << "protocol_layer[STDIO|HTTPS/SSE]:str = STDIO\n"
      << "config_scope_root:str = "
      << resolve_super_loop_config_scope_root(context) << "\n"
      << "allow_local_read:bool = true\n"
      << "allow_local_write:bool = true\n"
      << "default_roots:str = " << default_root.string() << "\n"
      << "objective_roots:str = " << loop.objective_root << "\n"
      << "allowed_extensions:str = .dsl\n"
      << "write_roots:str = " << loop.objective_root << ",";
  if (allow_default_write && !default_root.empty()) {
    out << default_root.string() << ",";
  }
  out << backup_dir.string() << "\n"
      << "backup_enabled:bool = true\n"
      << "backup_dir:str = " << backup_dir.string() << "\n"
      << "backup_max_entries(1,+inf):int = 64\n";
  return out.str();
}

[[nodiscard]] std::string build_default_super_briefing(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  std::ostringstream out;
  const std::string authority_scope =
      cuwacunu::hero::runtime::trim_ascii(loop.authority_scope).empty()
          ? "objective_only"
          : cuwacunu::hero::runtime::trim_ascii(loop.authority_scope);
  out << "You are the active planner inside a SUPER loop.\n\n"
      << "Primary files:\n"
      << "- Super objective DSL: " << loop.super_objective_dsl_path << "\n"
      << "- Super objective markdown: " << loop.super_objective_md_path << "\n"
      << "- Super guidance markdown: " << loop.super_guidance_md_path << "\n"
      << "- Config Hero policy: " << loop.config_policy_path << "\n"
      << "- Memory: " << loop.memory_path << "\n"
      << "- Latest turn context: "
      << cuwacunu::hero::super::super_loop_latest_turn_context_path(
             std::filesystem::path(loop.loop_root).parent_path(), loop.loop_id)
             .string()
      << "\n"
      << "- Human escalation artifact: " << loop.human_escalation_path << "\n"
      << "- Mutable objective root: " << loop.objective_root << "\n"
      << "- Current authority scope: " << authority_scope << "\n\n"
      << "The turn context includes the current planning phase and the latest "
         "runtime/lattice evidence.\n\n"
      << "Interpret the authored markdown in this order:\n"
      << "- objective markdown = what the loop is trying to achieve\n"
      << "- guidance markdown = authored boundaries plus advisory heuristics; prefer stronger evidence when the guidance is not a hard rule\n\n"
      << "Rules:\n"
      << "1. Work in read-only shell mode. Do not edit files directly.\n"
      << "2. Follow the declared super objective DSL, objective markdown, guidance markdown, and current objective bundle.\n"
      << "3. Use hero.config.objective.list/read/create/replace/delete for truth-source objective files under objective_root.\n"
      << "4. Shared default writes are not ordinary autonomous authority. If the objective truly needs a shared default change and the current authority scope does not allow it, return outcome=escalate with escalation.kind=authority_expansion.\n"
      << "5. Prefer whole-file replace with expected_sha256 from the prior read.\n"
      << "6. Pass objective_root=" << loop.objective_root
      << " to those Config Hero tools.\n"
      << "7. Never mutate files outside the configured objective/default roots.\n"
      << "8. Prefer the turn context first, then hero.lattice.get_view/get_fact "
         "for semantic evidence; use Runtime tails mainly for operational "
         "debugging.\n"
      << "9. Use hero.lattice.list_views/list_facts when you need to discover "
         "selectors; family_evaluation_report requires a family canonical_path "
         "plus contract_hash.\n"
      << "10. Shell exec is unavailable in this environment. Prefer the embedded "
         "turn context and objective campaign contents when present; use MCP tools "
         "instead of shell reads.\n"
      << "11. If you change any objective-local .dsl, describe the actual changes in "
         "memory_note.\n"
      << "12. You may plan autonomously inside the current authority envelope until the objective is solved, budget is exhausted, or a typed escalation is needed.\n"
      << "13. Return only JSON matching the provided output schema. Do not return "
         "edit payloads or tool transcripts.\n\n"
      << "Return one outcome JSON object with:\n"
      << "- outcome = launch | escalate | success | stop | fail\n"
      << "- launch.mode = run_plan | binding when outcome=launch\n"
      << "- launch.binding_id only when launch.mode=binding\n"
      << "- launch.reset_runtime_state = true | false when outcome=launch\n"
      << "- reason = concise operator-facing explanation\n"
      << "- memory_note = loop memory update; use empty string when none\n"
      << "- escalation.kind = authority_expansion | budget_expansion | objective_clarification when outcome=escalate\n"
      << "- escalation.request = concise operator-facing escalation note when outcome=escalate\n"
      << "- escalation.delta.allow_default_write when requesting authority expansion\n"
      << "- escalation.delta.additional_review_turns and escalation.delta.additional_campaign_launches when requesting budget expansion\n";
  return out.str();
}

[[nodiscard]] std::string build_initial_super_loop_memory(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  return "# Super Loop Memory\n\n"
         "Loop ID: " +
         loop.loop_id + "\n\nObjective: " + loop.objective_name +
         "\n\nCurrent stance:\n- Follow the declared super objective DSL plus the authored objective and guidance markdown.\n"
         "- Plan, mutate, launch, observe, and iterate inside the current authority envelope.\n"
         "- Escalate only for authority expansion, budget expansion, or objective clarification.\n";
}

}  // namespace

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
    const std::filesystem::path& source_super_objective_md_path,
    const std::filesystem::path& source_super_guidance_md_path,
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

  std::string objective_md_text{};
  if (!cuwacunu::hero::runtime::read_text_file(source_super_objective_md_path,
                                               &objective_md_text, error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          loop.super_objective_md_path, objective_md_text, error)) {
    return false;
  }
  std::string guidance_md_text{};
  if (!cuwacunu::hero::runtime::read_text_file(source_super_guidance_md_path,
                                               &guidance_md_text, error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          loop.super_guidance_md_path, guidance_md_text, error)) {
    return false;
  }
  if (!refresh_super_loop_config_policy_dsl(context, loop, error)) return false;

  std::string briefing_text = build_default_super_briefing(loop);
  if (!cuwacunu::hero::runtime::trim_ascii(objective_dsl_text).empty()) {
    briefing_text.append("\n\nSuper objective DSL contents:\n");
    briefing_text.append(objective_dsl_text);
    if (briefing_text.back() != '\n') briefing_text.push_back('\n');
  }
  if (!cuwacunu::hero::runtime::trim_ascii(objective_md_text).empty()) {
    briefing_text.append("\n\nSuper objective markdown contents:\n");
    briefing_text.append(objective_md_text);
    if (briefing_text.back() != '\n') briefing_text.push_back('\n');
  }
  if (!cuwacunu::hero::runtime::trim_ascii(guidance_md_text).empty()) {
    briefing_text.append("\n\nSuper guidance markdown contents:\n");
    briefing_text.append(guidance_md_text);
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

  std::error_code ec{};
  std::filesystem::create_directories(
      cuwacunu::hero::super::super_loop_turns_dir(context.super_root,
                                                  loop.loop_id),
      ec);
  if (ec) {
    if (error) *error = "cannot create super loop turns dir";
    return false;
  }
  std::filesystem::create_directories(
      cuwacunu::hero::super::super_loop_logs_dir(context.super_root, loop.loop_id),
      ec);
  if (ec) {
    if (error) *error = "cannot create super loop logs dir";
    return false;
  }
  std::filesystem::create_directories(
      cuwacunu::hero::super::super_loop_human_dir(context.super_root, loop.loop_id),
      ec);
  if (ec) {
    if (error) *error = "cannot create super loop human dir";
    return false;
  }
  std::filesystem::create_directories(
      cuwacunu::hero::super::super_loop_human_resolutions_dir(
          context.super_root, loop.loop_id),
      ec);
  if (ec) {
    if (error) *error = "cannot create super loop human resolutions dir";
    return false;
  }
  return true;
}

bool refresh_super_loop_config_policy_dsl(
    const super_loop_workspace_context_t& context,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::string* error) {
  if (error) error->clear();
  const std::filesystem::path config_policy_path =
      loop.config_policy_path.empty()
          ? cuwacunu::hero::super::super_loop_config_policy_path(
                context.super_root, loop.loop_id)
          : std::filesystem::path(loop.config_policy_path);
  return cuwacunu::hero::runtime::write_text_file_atomic(
      config_policy_path, build_super_loop_config_policy_dsl(context, loop),
      error);
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
