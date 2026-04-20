// SPDX-License-Identifier: MIT
#include "hero/marshal_hero/marshal_session_workspace.h"

#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

#include "hero/runtime_hero/runtime_job.h"

namespace cuwacunu {
namespace hero {
namespace marshal_mcp {

namespace {

[[nodiscard]] std::string resolve_marshal_session_config_scope_root(
    const marshal_session_workspace_context_t &context) {
  std::string scope_root = context.config_scope_root.string();
  if (scope_root.empty()) {
    scope_root = context.global_config_path.parent_path().string();
  }
  if (scope_root.empty() && !context.repo_root.empty()) {
    scope_root = (context.repo_root / "src/config").string();
  }
  return scope_root;
}

[[nodiscard]] std::string
sanitize_marshal_backup_component(std::string_view raw) {
  std::string out;
  out.reserve(raw.size());
  for (const unsigned char ch : raw) {
    const bool keep = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                      (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' ||
                      ch == '-';
    out.push_back(keep ? static_cast<char>(ch) : '_');
  }
  if (out.empty())
    return "unknown_objective";
  return out;
}

[[nodiscard]] std::filesystem::path derive_marshal_session_backup_dir(
    const marshal_session_workspace_context_t &context,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (!context.repo_root.empty()) {
    return (context.repo_root / ".backups" / "hero.marshal" /
            sanitize_marshal_backup_component(session.objective_name))
        .lexically_normal();
  }
  const std::string scope_root =
      resolve_marshal_session_config_scope_root(context);
  if (!scope_root.empty()) {
    return (std::filesystem::path(scope_root) / ".backups" / "hero.marshal" /
            sanitize_marshal_backup_component(session.objective_name))
        .lexically_normal();
  }
  return (std::filesystem::path(session.session_root) / ".hero_config_backups")
      .lexically_normal();
}

[[nodiscard]] std::filesystem::path derive_marshal_session_default_root(
    const marshal_session_workspace_context_t &context) {
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

[[nodiscard]] std::filesystem::path derive_marshal_session_temp_root(
    const marshal_session_workspace_context_t &context) {
  if (!context.config_scope_root.empty()) {
    return (context.config_scope_root / "instructions" / "temp")
        .lexically_normal();
  }
  if (!context.repo_root.empty()) {
    return (context.repo_root / "src/config/instructions/temp")
        .lexically_normal();
  }
  return {};
}

[[nodiscard]] std::string build_marshal_session_config_policy_dsl(
    const marshal_session_workspace_context_t &context,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  const std::filesystem::path backup_dir =
      derive_marshal_session_backup_dir(context, session);
  const std::filesystem::path default_root =
      derive_marshal_session_default_root(context);
  const std::filesystem::path temp_root =
      derive_marshal_session_temp_root(context);
  const bool allow_default_write =
      cuwacunu::hero::runtime::trim_ascii(session.authority_scope) ==
      "objective_plus_defaults";
  std::ostringstream out;
  out << "/* marshal-generated Config Hero policy for a MARSHAL session */\n"
      << "protocol_layer[STDIO|HTTPS/SSE]:str = STDIO\n"
      << "config_scope_root:str = "
      << resolve_marshal_session_config_scope_root(context) << "\n"
      << "allow_local_read:bool = true\n"
      << "allow_local_write:bool = true\n"
      << "default_roots:str = " << default_root.string() << "\n"
      << "objective_roots:str = " << session.objective_root << "\n"
      << "temp_roots:str = " << temp_root.string() << "\n"
      << "allowed_extensions:str = .dsl,.md\n"
      << "write_roots:str = " << session.objective_root << ",";
  if (!temp_root.empty()) {
    out << temp_root.string() << ",";
  }
  if (allow_default_write && !default_root.empty()) {
    out << default_root.string() << ",";
  }
  out << backup_dir.string() << "\n"
      << "backup_enabled:bool = true\n"
      << "backup_dir:str = " << backup_dir.string() << "\n"
      << "backup_max_entries(1,+inf):int = 64\n";
  return out.str();
}

[[nodiscard]] std::string build_marshal_session_hero_marshal_dsl(
    const marshal_session_workspace_context_t &context,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::ostringstream out;
  out << "/*\n"
      << "  marshal-generated hero.marshal.dsl\n"
      << "  Resolved settings governing this Marshal Hero session.\n"
      << "*/\n"
      << "repo_root:path = " << context.repo_root.string() << "\n"
      << "runtime_hero_binary:path = " << context.runtime_hero_binary.string()
      << "\n"
      << "config_hero_binary:path = " << context.config_hero_binary.string()
      << "\n"
      << "lattice_hero_binary:path = " << context.lattice_hero_binary.string()
      << "\n"
      << "human_hero_binary:path = " << context.human_hero_binary.string()
      << "\n"
      << "human_operator_identities:path = "
      << context.human_operator_identities.string() << "\n"
      << "config_scope_root:path = "
      << resolve_marshal_session_config_scope_root(context) << "\n"
      << "marshal_codex_binary:path = " << session.resolved_marshal_codex_binary
      << "\n"
      << "marshal_codex_model:str = " << session.resolved_marshal_codex_model
      << "\n"
      << "marshal_codex_reasoning_effort:str = "
      << session.resolved_marshal_codex_reasoning_effort << "\n"
      << "tail_default_lines(1,+inf):int = " << context.tail_default_lines
      << "\n"
      << "poll_interval_ms(100,+inf):int = " << context.poll_interval_ms << "\n"
      << "marshal_codex_timeout_sec(1,+inf):int = "
      << context.marshal_codex_timeout_sec << "\n"
      << "marshal_max_campaign_launches(1,+inf):int = "
      << session.max_campaign_launches << "\n";
  return out.str();
}

[[nodiscard]] std::string build_default_marshal_briefing(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::ostringstream out;
  const std::string authority_scope =
      cuwacunu::hero::runtime::trim_ascii(session.authority_scope).empty()
          ? "objective_only"
          : cuwacunu::hero::runtime::trim_ascii(session.authority_scope);
  out << "You are the active planner inside a MARSHAL session.\n\n"
      << "Primary files:\n"
      << "- Marshal objective DSL: " << session.marshal_objective_dsl_path
      << "\n"
      << "- Marshal objective markdown: " << session.marshal_objective_md_path
      << "\n"
      << "- Marshal guidance markdown: " << session.marshal_guidance_md_path
      << "\n"
      << "- Marshal session settings DSL: " << session.hero_marshal_dsl_path
      << "\n"
      << "- Config Hero policy: " << session.config_policy_path << "\n"
      << "- Memory: " << session.memory_path << "\n"
      << "- Latest input checkpoint: "
      << cuwacunu::hero::marshal::marshal_session_latest_input_checkpoint_path(
             std::filesystem::path(session.session_root).parent_path(),
             session.marshal_session_id)
             .string()
      << "\n"
      << "- Human request artifact: " << session.human_request_path << "\n"
      << "- Mutable objective root: " << session.objective_root << "\n"
      << "- Current authority scope: " << authority_scope << "\n\n"
      << "The latest input checkpoint includes the current planning context "
         "and the latest "
         "runtime/lattice evidence.\n\n"
      << "Interpret the authored markdown in this order:\n"
      << "- objective markdown = what the session is trying to achieve\n"
      << "- guidance markdown = authored boundaries plus advisory heuristics; "
         "prefer stronger evidence when the guidance is not a hard rule\n\n"
      << "Rules:\n"
      << "1. Work in read-only shell mode. Do not edit files directly.\n"
      << "2. Follow the declared marshal objective DSL, objective markdown, "
         "guidance markdown, and current objective bundle.\n"
      << "3. Use hero.config.objective.list/read/create/replace/delete for "
         "truth-source objective files under objective_root.\n"
      << "4. Use hero.config.temp.list/read/create/replace/delete for "
         "temporary instruction files under temp_roots when scratch authored "
         "artifacts help planning.\n"
      << "5. Shared default writes are not ordinary autonomous authority. If "
         "the objective truly needs a shared default change and the current "
         "authority scope does not allow it, return intent=request_governance "
         "with governance.kind=authority_expansion.\n"
      << "6. Prefer whole-file replace with expected_sha256 from the prior "
         "read.\n"
      << "7. Pass objective_root=" << session.objective_root
      << " to those Config Hero tools.\n"
      << "8. Never mutate files outside the configured objective/default/temp "
         "roots.\n"
      << "9. Prefer the latest checkpoint first, then "
         "hero.lattice.get_view/get_fact "
         "for semantic evidence; use Runtime tails mainly for operational "
         "debugging.\n"
      << "10. Use hero.lattice.list_views/list_facts when you need to discover "
         "selectors; family_evaluation_report requires a family canonical_path "
         "plus contract_hash.\n"
      << "11. Shell exec is unavailable in this environment. Prefer the "
         "embedded "
         "checkpoint context and objective campaign contents when present; use "
         "MCP tools "
         "instead of shell reads.\n"
      << "11. If you change any objective-local .dsl, describe the actual "
         "changes in "
         "memory_note.\n"
      << "12. You may plan autonomously inside the current authority envelope "
         "until the objective is satisfied for now, launch budget is "
         "exhausted, or an input/governance pause is needed.\n"
      << "13. When returning intent=complete or intent=terminate, make "
         "reason a concise report of affairs for the operator: state the "
         "objective outcome, the evidence or campaigns considered, any "
         "truth-source changes made, and the remaining risks or next steps. "
         "Do not make the operator read debug files for the core conclusion.\n"
      << "14. Return only JSON matching the provided output schema. Do not "
         "return "
         "edit payloads or tool transcripts.\n\n"
      << "Return one intent JSON object with:\n"
      << "- intent = launch_campaign | pause_for_clarification | "
         "request_governance | complete | terminate\n"
      << "- intent=complete means the objective is satisfied for now and the "
         "session should park idle until a future continue request\n"
      << "- launch.mode = run_plan | binding when intent=launch_campaign\n"
      << "- launch.binding_id only when launch.mode=binding\n"
      << "- launch.reset_runtime_state = true | false when "
         "intent=launch_campaign\n"
      << "- reason = concise operator-facing explanation; for complete or "
         "terminate, this is the short report of affairs persisted into "
         "human/summary.latest.md\n"
      << "- memory_note = session memory update; use empty string when none\n"
      << "- clarification_request.request = concise operator-facing question "
         "when intent=pause_for_clarification\n"
      << "- governance.kind = authority_expansion | launch_budget_expansion | "
         "policy_decision when intent=request_governance\n"
      << "- governance.request = concise operator-facing governance note when "
         "intent=request_governance\n"
      << "- governance.delta.allow_default_write when requesting authority "
         "expansion\n"
      << "- governance.delta.additional_campaign_launches when requesting "
         "launch budget expansion\n";
  return out.str();
}

[[nodiscard]] std::string build_initial_marshal_session_memory(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return "# Marshal Session Memory\n\n"
         "Marshal ID: " +
         session.marshal_session_id +
         "\n\nObjective: " + session.objective_name +
         "\n\nCurrent stance:\n- Follow the declared marshal objective DSL "
         "plus the authored objective and guidance markdown.\n"
         "- Plan, mutate, launch, observe, and iterate inside the current "
         "authority envelope.\n"
         "- Request governance only for authority expansion, launch budget "
         "expansion, or policy decisions.\n"
         "- Pause for clarification when ordinary clarification is needed.\n";
}

} // namespace

std::string derive_marshal_session_objective_name(
    const std::filesystem::path &source_marshal_objective_dsl_path) {
  const std::string folder =
      source_marshal_objective_dsl_path.parent_path().filename().string();
  if (!folder.empty())
    return folder;
  const std::string stem = source_marshal_objective_dsl_path.stem().string();
  return stem.empty() ? "runtime-marshal-session" : stem;
}

bool write_marshal_session_bootstrap_files(
    const marshal_session_workspace_context_t &context,
    const std::filesystem::path &source_marshal_objective_dsl_path,
    const std::filesystem::path &source_marshal_objective_md_path,
    const std::filesystem::path &source_marshal_guidance_md_path,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string *error) {
  if (error)
    error->clear();

  std::string objective_dsl_text{};
  if (!cuwacunu::hero::runtime::read_text_file(
          source_marshal_objective_dsl_path, &objective_dsl_text, error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          session.marshal_objective_dsl_path, objective_dsl_text, error)) {
    return false;
  }

  std::string objective_md_text{};
  if (!cuwacunu::hero::runtime::read_text_file(source_marshal_objective_md_path,
                                               &objective_md_text, error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          session.marshal_objective_md_path, objective_md_text, error)) {
    return false;
  }
  std::string guidance_md_text{};
  if (!cuwacunu::hero::runtime::read_text_file(source_marshal_guidance_md_path,
                                               &guidance_md_text, error)) {
    return false;
  }
  const std::string hero_marshal_dsl_text =
      build_marshal_session_hero_marshal_dsl(context, session);
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          session.hero_marshal_dsl_path, hero_marshal_dsl_text, error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          session.marshal_guidance_md_path, guidance_md_text, error)) {
    return false;
  }
  if (!refresh_marshal_session_config_policy_dsl(context, session, error))
    return false;

  std::string briefing_text = build_default_marshal_briefing(session);
  if (!cuwacunu::hero::runtime::trim_ascii(hero_marshal_dsl_text).empty()) {
    briefing_text.append("\n\nMarshal session settings DSL contents:\n");
    briefing_text.append(hero_marshal_dsl_text);
    if (briefing_text.back() != '\n')
      briefing_text.push_back('\n');
  }
  if (!cuwacunu::hero::runtime::trim_ascii(objective_dsl_text).empty()) {
    briefing_text.append("\n\nMarshal objective DSL contents:\n");
    briefing_text.append(objective_dsl_text);
    if (briefing_text.back() != '\n')
      briefing_text.push_back('\n');
  }
  if (!cuwacunu::hero::runtime::trim_ascii(objective_md_text).empty()) {
    briefing_text.append("\n\nMarshal objective markdown contents:\n");
    briefing_text.append(objective_md_text);
    if (briefing_text.back() != '\n')
      briefing_text.push_back('\n');
  }
  if (!cuwacunu::hero::runtime::trim_ascii(guidance_md_text).empty()) {
    briefing_text.append("\n\nMarshal guidance markdown contents:\n");
    briefing_text.append(guidance_md_text);
    if (briefing_text.back() != '\n')
      briefing_text.push_back('\n');
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(session.briefing_path,
                                                       briefing_text, error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          session.memory_path, build_initial_marshal_session_memory(session),
          error)) {
    return false;
  }

  std::error_code ec{};
  std::filesystem::create_directories(
      cuwacunu::hero::marshal::marshal_session_checkpoints_dir(
          context.marshal_root, session.marshal_session_id),
      ec);
  if (ec) {
    if (error)
      *error = "cannot create marshal session checkpoints dir";
    return false;
  }
  std::filesystem::create_directories(
      cuwacunu::hero::marshal::marshal_session_logs_dir(
          context.marshal_root, session.marshal_session_id),
      ec);
  if (ec) {
    if (error)
      *error = "cannot create marshal session logs dir";
    return false;
  }
  std::filesystem::create_directories(
      cuwacunu::hero::marshal::marshal_session_human_dir(
          context.marshal_root, session.marshal_session_id),
      ec);
  if (ec) {
    if (error)
      *error = "cannot create marshal session human dir";
    return false;
  }
  std::filesystem::create_directories(
      cuwacunu::hero::marshal::marshal_session_human_governance_resolutions_dir(
          context.marshal_root, session.marshal_session_id),
      ec);
  if (ec) {
    if (error)
      *error = "cannot create marshal session governance dir";
    return false;
  }
  return true;
}

bool refresh_marshal_session_config_policy_dsl(
    const marshal_session_workspace_context_t &context,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string *error) {
  if (error)
    error->clear();
  const std::filesystem::path config_policy_path =
      session.config_policy_path.empty()
          ? cuwacunu::hero::marshal::marshal_session_config_policy_path(
                context.marshal_root, session.marshal_session_id)
          : std::filesystem::path(session.config_policy_path);
  return cuwacunu::hero::runtime::write_text_file_atomic(
      config_policy_path,
      build_marshal_session_config_policy_dsl(context, session), error);
}

bool refresh_marshal_session_hero_marshal_dsl(
    const marshal_session_workspace_context_t &context,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string *error) {
  if (error)
    error->clear();
  const std::filesystem::path hero_marshal_dsl_path =
      session.hero_marshal_dsl_path.empty()
          ? cuwacunu::hero::marshal::marshal_session_hero_marshal_dsl_path(
                context.marshal_root, session.marshal_session_id)
          : std::filesystem::path(session.hero_marshal_dsl_path);
  return cuwacunu::hero::runtime::write_text_file_atomic(
      hero_marshal_dsl_path,
      build_marshal_session_hero_marshal_dsl(context, session), error);
}

std::filesystem::path resolve_config_hero_mcp_binary(
    const marshal_session_workspace_context_t &context) {
  std::error_code ec{};
  if (!context.self_binary_path.empty()) {
    const std::filesystem::path sibling =
        context.self_binary_path.parent_path() / "hero_config.mcp";
    if (std::filesystem::exists(sibling, ec) &&
        std::filesystem::is_regular_file(sibling, ec)) {
      return sibling;
    }
  }
  if (!context.repo_root.empty()) {
    return (context.repo_root / ".build/hero/hero_config.mcp")
        .lexically_normal();
  }
  return {};
}

} // namespace marshal_mcp
} // namespace hero
} // namespace cuwacunu
