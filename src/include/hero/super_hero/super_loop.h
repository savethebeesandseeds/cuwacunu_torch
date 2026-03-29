// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "hero/runtime_hero/runtime_job.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"

namespace cuwacunu {
namespace hero {
namespace super {

inline constexpr std::string_view kSuperLoopSchemaV2 =
    "hero.super.loop.v2";
inline constexpr std::string_view kSuperLoopManifestFilename =
    "super.loop.manifest.lls";
inline constexpr std::string_view kLegacySuperLoopManifestFilename = "loop.lls";
inline constexpr std::string_view kSuperLoopMemoryFilename =
    "super.loop.memory.md";
inline constexpr std::string_view kLegacySuperLoopMemoryFilename = "memory.md";
inline constexpr std::string_view kSuperLoopBriefingFilename =
    "super.briefing.md";
inline constexpr std::string_view kSuperLoopCodexSessionLogFilename =
    "codex.session.log";
inline constexpr std::string_view kSuperLoopReviewPidFilename =
    ".codex.review.pid";
inline constexpr std::string_view kSuperLoopObjectiveDslFilename =
    "super.objective.dsl";
inline constexpr std::string_view kSuperLoopObjectiveMdFilename =
    "super.objective.md";
inline constexpr std::string_view kSuperLoopGuidanceMdFilename =
    "super.guidance.md";
inline constexpr std::string_view kSuperLoopConfigHeroPolicyFilename =
    "config.hero.policy.dsl";
inline constexpr std::string_view kSuperLoopHumanEscalationFilename =
    "escalation.latest.md";
inline constexpr std::string_view kSuperLoopHumanReportFilename =
    "report.latest.md";
inline constexpr std::string_view kSuperLoopHumanResolutionLatestFilename =
    "resolution.latest.json";
inline constexpr std::string_view kSuperLoopHumanResolutionLatestSigFilename =
    "resolution.latest.sig";
inline constexpr std::string_view kSuperLoopHumanReportAckLatestFilename =
    "report_ack.latest.json";
inline constexpr std::string_view kSuperLoopHumanReportAckLatestSigFilename =
    "report_ack.latest.sig";
inline constexpr std::string_view kSuperLoopEventsFilename =
    "super.loop.events.jsonl";
inline constexpr std::string_view kLegacySuperLoopEventsFilename =
    "events.jsonl";
inline constexpr std::string_view kSuperLoopTurnsDirname = "turns";
inline constexpr std::string_view kSuperLoopHumanResolutionsDirname =
    "resolutions";
inline constexpr std::string_view kSuperLoopLatestTurnContextFilename =
    "turn_context.latest.json";
inline constexpr std::string_view kSuperLoopLatestTurnOutcomeFilename =
    "turn_outcome.latest.json";
inline constexpr std::string_view kSuperLoopLatestTurnMutationFilename =
    "turn_mutation.latest.json";
inline constexpr std::string_view kSuperLoopLogsDirname = "logs";
inline constexpr std::string_view kSuperLoopHumanDirname = "human";
inline constexpr std::string_view kHumanHeroRuntimeDirname = ".human_hero";
inline constexpr std::string_view kHumanHeroPendingEscalationCountFilename =
    "awaiting_human.pending.count";
inline constexpr std::string_view kHumanHeroPendingReportCountFilename =
    "finished.pending.count";
inline constexpr std::string_view kSuperLoopRunnerLockFilename =
    ".super.loop.runner.lock";

[[nodiscard]] inline std::filesystem::path prefer_canonical_super_loop_file_path(
    const std::filesystem::path& canonical_path,
    const std::filesystem::path& legacy_path);

[[nodiscard]] inline bool is_super_loop_runtime_text_char(char ch) {
  if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
      (ch >= '0' && ch <= '9') || ch == ' ') {
    return true;
  }
  switch (ch) {
    case '.':
    case ',':
    case ':':
    case '-':
    case '_':
    case '\\':
    case '/':
    case '[':
    case ']':
    case '(':
    case ')':
    case '{':
    case '}':
    case '|':
    case '\'':
    case '"':
    case '+':
    case '*':
    case '?':
    case '!':
    case '<':
    case '>':
    case '=':
    case '@':
      return true;
    default:
      return false;
  }
}

[[nodiscard]] inline std::string sanitize_super_loop_runtime_text(
    std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (const char ch : value) {
    char emit = ch;
    if (emit == '\n' || emit == '\r' || emit == '\t' || emit == '#' ||
        emit == ';' || !is_super_loop_runtime_text_char(emit)) {
      emit = ' ';
    }
    if (emit == ' ') {
      if (out.empty() || out.back() == ' ') continue;
      out.push_back(' ');
      continue;
    }
    out.push_back(emit);
  }
  while (!out.empty() && out.front() == ' ') out.erase(out.begin());
  while (!out.empty() && out.back() == ' ') out.pop_back();
  return out;
}

struct super_loop_record_t {
  // Identity and high-level state.
  std::string schema{std::string(kSuperLoopSchemaV2)};
  std::string loop_id{};
  std::string state{"bootstrap"};
  std::string state_detail{};
  std::string objective_name{};
  std::string global_config_path{};

  // Source-of-truth inputs chosen at loop creation time.
  std::string source_super_objective_dsl_path{};
  std::string source_campaign_dsl_path{};
  std::string source_super_objective_md_path{};
  std::string source_super_guidance_md_path{};

  // Live loop ledger plus truth-source objective references.
  std::string loop_root{};
  std::string objective_root{};
  std::string campaign_dsl_path{};
  std::string super_objective_dsl_path{};
  std::string super_objective_md_path{};
  std::string super_guidance_md_path{};

  // Generated operational artifacts used during Super Hero review.
  std::string config_policy_path{};
  std::string briefing_path{};
  std::string memory_path{};
  std::string human_escalation_path{};
  std::string events_path{};
  std::string codex_session_id{};

  // Turn lifecycle.
  std::uint64_t started_at_ms{0};
  std::uint64_t updated_at_ms{0};
  std::optional<std::uint64_t> finished_at_ms{};
  std::uint64_t turn_count{0};
  std::uint64_t launch_count{0};
  std::uint64_t max_review_turns{0};
  std::uint64_t max_campaign_launches{0};
  std::uint64_t remaining_review_turns{0};
  std::uint64_t remaining_campaign_launches{0};
  std::string authority_scope{"objective_only"};
  std::string latest_escalation_kind{};

  // Latest execution status and evidence pointers.
  std::string active_campaign_cursor{};
  std::string last_outcome_kind{};
  std::string last_warning{};
  std::string last_turn_context_path{};
  std::string last_turn_outcome_path{};
  std::string last_turn_mutation_path{};

  // Historical campaign trail for this loop.
  std::vector<std::string> campaign_cursors{};
};

[[nodiscard]] inline std::filesystem::path super_root(
    const std::filesystem::path& runtime_root) {
  if (runtime_root.empty()) return {};
  return (runtime_root / ".super_hero").lexically_normal();
}

[[nodiscard]] inline std::filesystem::path human_hero_runtime_dir(
    const std::filesystem::path& super_root) {
  if (super_root.empty()) return {};
  return (super_root.parent_path() / std::string(kHumanHeroRuntimeDirname))
      .lexically_normal();
}

[[nodiscard]] inline std::filesystem::path human_hero_pending_count_path(
    const std::filesystem::path& super_root) {
  return human_hero_runtime_dir(super_root) /
         std::string(kHumanHeroPendingEscalationCountFilename);
}

[[nodiscard]] inline std::filesystem::path human_hero_pending_report_count_path(
    const std::filesystem::path& super_root) {
  return human_hero_runtime_dir(super_root) /
         std::string(kHumanHeroPendingReportCountFilename);
}

[[nodiscard]] inline std::string format_super_index(std::size_t index) {
  std::ostringstream out;
  out << std::setw(4) << std::setfill('0') << index;
  return out.str();
}

[[nodiscard]] inline std::filesystem::path super_loop_dir(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_root / std::string(loop_id);
}

[[nodiscard]] inline std::filesystem::path super_loop_manifest_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kSuperLoopManifestFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_legacy_manifest_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kLegacySuperLoopManifestFilename);
}

[[nodiscard]] inline std::filesystem::path
resolve_existing_super_loop_manifest_path(const std::filesystem::path& super_root,
                                          std::string_view loop_id) {
  const std::filesystem::path manifest_path =
      super_loop_manifest_path(super_root, loop_id);
  std::error_code ec{};
  if (std::filesystem::exists(manifest_path, ec) &&
      std::filesystem::is_regular_file(manifest_path, ec)) {
    return manifest_path;
  }
  ec.clear();
  const std::filesystem::path legacy_manifest_path =
      super_loop_legacy_manifest_path(super_root, loop_id);
  if (std::filesystem::exists(legacy_manifest_path, ec) &&
      std::filesystem::is_regular_file(legacy_manifest_path, ec)) {
    return legacy_manifest_path;
  }
  return manifest_path;
}

[[nodiscard]] inline std::filesystem::path prefer_canonical_super_loop_file_path(
    const std::filesystem::path& canonical_path,
    const std::filesystem::path& legacy_path) {
  std::error_code ec{};
  if (std::filesystem::exists(canonical_path, ec) &&
      std::filesystem::is_regular_file(canonical_path, ec)) {
    ec.clear();
    (void)std::filesystem::remove(legacy_path, ec);
    return canonical_path;
  }
  ec.clear();
  if (std::filesystem::exists(legacy_path, ec) &&
      std::filesystem::is_regular_file(legacy_path, ec)) {
    std::filesystem::rename(legacy_path, canonical_path, ec);
    if (!ec) return canonical_path;
    ec.clear();
    (void)std::filesystem::copy_file(
        legacy_path, canonical_path,
        std::filesystem::copy_options::overwrite_existing, ec);
    if (!ec) {
      std::error_code remove_ec{};
      (void)std::filesystem::remove(legacy_path, remove_ec);
      return canonical_path;
    }
    return legacy_path;
  }
  return canonical_path;
}

[[nodiscard]] inline std::filesystem::path super_loop_runner_lock_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kSuperLoopRunnerLockFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_memory_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kSuperLoopMemoryFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_legacy_memory_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kLegacySuperLoopMemoryFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_briefing_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kSuperLoopBriefingFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_logs_dir(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kSuperLoopLogsDirname);
}

[[nodiscard]] inline std::filesystem::path super_loop_codex_session_log_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_logs_dir(super_root, loop_id) /
         std::string(kSuperLoopCodexSessionLogFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_review_pid_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kSuperLoopReviewPidFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_objective_dsl_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kSuperLoopObjectiveDslFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_objective_md_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kSuperLoopObjectiveMdFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_guidance_md_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kSuperLoopGuidanceMdFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_config_policy_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kSuperLoopConfigHeroPolicyFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_human_dir(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kSuperLoopHumanDirname);
}

[[nodiscard]] inline std::filesystem::path super_loop_human_escalation_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_human_dir(super_root, loop_id) /
         std::string(kSuperLoopHumanEscalationFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_human_report_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_human_dir(super_root, loop_id) /
         std::string(kSuperLoopHumanReportFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_events_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kSuperLoopEventsFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_legacy_events_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kLegacySuperLoopEventsFilename);
}

[[nodiscard]] inline std::filesystem::path
super_loop_human_resolution_latest_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_human_dir(super_root, loop_id) /
         std::string(kSuperLoopHumanResolutionLatestFilename);
}

[[nodiscard]] inline std::filesystem::path
super_loop_human_resolution_latest_sig_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_human_dir(super_root, loop_id) /
         std::string(kSuperLoopHumanResolutionLatestSigFilename);
}

[[nodiscard]] inline std::filesystem::path
super_loop_human_report_ack_latest_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_human_dir(super_root, loop_id) /
         std::string(kSuperLoopHumanReportAckLatestFilename);
}

[[nodiscard]] inline std::filesystem::path
super_loop_human_report_ack_latest_sig_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_human_dir(super_root, loop_id) /
         std::string(kSuperLoopHumanReportAckLatestSigFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_turns_dir(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_dir(super_root, loop_id) /
         std::string(kSuperLoopTurnsDirname);
}

[[nodiscard]] inline std::filesystem::path super_loop_human_resolutions_dir(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_human_dir(super_root, loop_id) /
         std::string(kSuperLoopHumanResolutionsDirname);
}

[[nodiscard]] inline std::filesystem::path super_loop_latest_turn_context_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_turns_dir(super_root, loop_id) /
         std::string(kSuperLoopLatestTurnContextFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_latest_turn_outcome_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_turns_dir(super_root, loop_id) /
         std::string(kSuperLoopLatestTurnOutcomeFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_latest_turn_mutation_path(
    const std::filesystem::path& super_root, std::string_view loop_id) {
  return super_loop_turns_dir(super_root, loop_id) /
         std::string(kSuperLoopLatestTurnMutationFilename);
}

[[nodiscard]] inline std::filesystem::path super_loop_turn_context_path(
    const std::filesystem::path& super_root, std::string_view loop_id,
    std::uint64_t turn_index) {
  return super_loop_turns_dir(super_root, loop_id) /
         ("turn_context." +
          format_super_index(static_cast<std::size_t>(turn_index)) + ".json");
}

[[nodiscard]] inline std::filesystem::path super_loop_turn_outcome_path(
    const std::filesystem::path& super_root, std::string_view loop_id,
    std::uint64_t turn_index) {
  return super_loop_turns_dir(super_root, loop_id) /
         ("turn_outcome." +
          format_super_index(static_cast<std::size_t>(turn_index)) + ".json");
}

[[nodiscard]] inline std::filesystem::path super_loop_turn_mutation_path(
    const std::filesystem::path& super_root, std::string_view loop_id,
    std::uint64_t turn_index) {
  return super_loop_turns_dir(super_root, loop_id) /
         ("turn_mutation." +
          format_super_index(static_cast<std::size_t>(turn_index)) + ".json");
}

[[nodiscard]] inline std::filesystem::path super_loop_human_resolution_path(
    const std::filesystem::path& super_root, std::string_view loop_id,
    std::uint64_t turn_index) {
  return super_loop_human_resolutions_dir(super_root, loop_id) /
         ("resolution." +
          format_super_index(static_cast<std::size_t>(turn_index)) + ".json");
}

[[nodiscard]] inline std::filesystem::path super_loop_human_resolution_sig_path(
    const std::filesystem::path& super_root, std::string_view loop_id,
    std::uint64_t turn_index) {
  return super_loop_human_resolutions_dir(super_root, loop_id) /
         ("resolution." +
          format_super_index(static_cast<std::size_t>(turn_index)) + ".sig");
}

[[nodiscard]] inline std::string make_super_loop_id(
    const std::filesystem::path& super_root) {
  const std::uint64_t started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  for (std::uint64_t nonce = 1; nonce != 0; ++nonce) {
    const std::uint64_t salt =
        static_cast<std::uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()) ^
        (nonce * 0x9e3779b97f4a7c15ULL);
    const std::string candidate =
        "loop." + std::to_string(started_at_ms) + "." +
        cuwacunu::hero::runtime::hex_lower_u64(salt).substr(0, 8);
    std::error_code ec{};
    if (!std::filesystem::exists(super_loop_dir(super_root, candidate),
                                 ec) ||
        ec) {
      return candidate;
    }
  }
  return "loop." + std::to_string(started_at_ms) + ".overflow";
}

[[nodiscard]] inline cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t
super_loop_record_to_document(const super_loop_record_t& record) {
  using namespace cuwacunu::piaabo::latent_lineage_state;
  runtime_lls_document_t document{};
  document.entries.push_back(make_runtime_lls_string_entry("schema", record.schema));
  document.entries.push_back(make_runtime_lls_string_entry("loop_id", record.loop_id));
  document.entries.push_back(make_runtime_lls_string_entry("state", record.state));
  document.entries.push_back(
      make_runtime_lls_string_entry("state_detail",
                                    sanitize_super_loop_runtime_text(
                                        record.state_detail)));
  document.entries.push_back(
      make_runtime_lls_string_entry("objective_name", record.objective_name));
  document.entries.push_back(make_runtime_lls_string_entry(
      "global_config_path", record.global_config_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "source_super_objective_dsl_path",
      record.source_super_objective_dsl_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "source_campaign_dsl_path", record.source_campaign_dsl_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "source_super_objective_md_path", record.source_super_objective_md_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "source_super_guidance_md_path", record.source_super_guidance_md_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("loop_root", record.loop_root));
  document.entries.push_back(
      make_runtime_lls_string_entry("objective_root", record.objective_root));
  document.entries.push_back(make_runtime_lls_string_entry(
      "campaign_dsl_path", record.campaign_dsl_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "super_objective_dsl_path", record.super_objective_dsl_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "super_objective_md_path", record.super_objective_md_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "super_guidance_md_path", record.super_guidance_md_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "config_policy_path", record.config_policy_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("briefing_path", record.briefing_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("memory_path", record.memory_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "human_escalation_path", record.human_escalation_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("events_path", record.events_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "codex_session_id", record.codex_session_id));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "started_at_ms", record.started_at_ms, "(0,+inf)"));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "updated_at_ms", record.updated_at_ms, "(0,+inf)"));
  if (record.finished_at_ms.has_value()) {
    document.entries.push_back(make_runtime_lls_uint_entry(
        "finished_at_ms", *record.finished_at_ms, "(0,+inf)"));
  }
  document.entries.push_back(make_runtime_lls_uint_entry(
      "turn_count", record.turn_count, "[0,+inf)"));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "launch_count", record.launch_count, "[0,+inf)"));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "max_review_turns", record.max_review_turns, "[0,+inf)"));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "max_campaign_launches", record.max_campaign_launches, "[0,+inf)"));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "remaining_review_turns", record.remaining_review_turns, "[0,+inf)"));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "remaining_campaign_launches", record.remaining_campaign_launches,
      "[0,+inf)"));
  document.entries.push_back(make_runtime_lls_string_entry(
      "authority_scope", record.authority_scope));
  document.entries.push_back(make_runtime_lls_string_entry(
      "latest_escalation_kind", record.latest_escalation_kind));
  document.entries.push_back(make_runtime_lls_string_entry(
      "active_campaign_cursor", record.active_campaign_cursor));
  document.entries.push_back(make_runtime_lls_string_entry(
      "last_outcome_kind", record.last_outcome_kind));
  document.entries.push_back(
      make_runtime_lls_string_entry("last_warning",
                                    sanitize_super_loop_runtime_text(
                                        record.last_warning)));
  document.entries.push_back(make_runtime_lls_string_entry(
      "last_turn_context_path", record.last_turn_context_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "last_turn_outcome_path", record.last_turn_outcome_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "last_turn_mutation_path", record.last_turn_mutation_path));
  for (std::size_t i = 0; i < record.campaign_cursors.size(); ++i) {
    document.entries.push_back(make_runtime_lls_string_entry(
        "campaign_cursor." + format_super_index(i), record.campaign_cursors[i]));
  }
  return document;
}

[[nodiscard]] inline bool parse_super_loop_record_document(
    const cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t& document,
    super_loop_record_t* out, std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for super loop record";
    return false;
  }
  std::unordered_map<std::string, std::string> kv{};
  if (!cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          document, &kv, error)) {
    return false;
  }

  super_loop_record_t parsed{};
  parsed.schema = kv["schema"];
  if (parsed.schema != kSuperLoopSchemaV2) {
    if (error) *error = "unexpected super loop schema: " + parsed.schema;
    return false;
  }
  parsed.loop_id = kv["loop_id"];
  parsed.state = kv["state"];
  parsed.state_detail = kv["state_detail"];
  parsed.objective_name = kv["objective_name"];
  parsed.global_config_path = kv["global_config_path"];
  parsed.source_super_objective_dsl_path = kv["source_super_objective_dsl_path"];
  parsed.source_campaign_dsl_path = kv["source_campaign_dsl_path"];
  parsed.source_super_objective_md_path = kv["source_super_objective_md_path"];
  parsed.source_super_guidance_md_path = kv["source_super_guidance_md_path"];
  parsed.loop_root = kv["loop_root"];
  parsed.objective_root = kv["objective_root"];
  parsed.campaign_dsl_path = kv["campaign_dsl_path"];
  parsed.super_objective_dsl_path = kv["super_objective_dsl_path"];
  parsed.super_objective_md_path = kv["super_objective_md_path"];
  parsed.super_guidance_md_path = kv["super_guidance_md_path"];
  parsed.config_policy_path = kv["config_policy_path"];
  parsed.briefing_path = kv["briefing_path"];
  parsed.memory_path = kv["memory_path"];
  parsed.human_escalation_path = kv["human_escalation_path"];
  parsed.events_path = kv["events_path"];
  parsed.codex_session_id = kv["codex_session_id"];
  parsed.active_campaign_cursor = kv["active_campaign_cursor"];
  parsed.last_outcome_kind = kv["last_outcome_kind"];
  parsed.last_warning = kv["last_warning"];
  parsed.last_turn_context_path = kv["last_turn_context_path"];
  parsed.last_turn_outcome_path = kv["last_turn_outcome_path"];
  parsed.last_turn_mutation_path = kv["last_turn_mutation_path"];
  parsed.authority_scope = kv["authority_scope"];
  parsed.latest_escalation_kind = kv["latest_escalation_kind"];
  if (!parsed.loop_root.empty()) {
    const std::filesystem::path loop_root_path(parsed.loop_root);
    const std::filesystem::path canonical_memory_path =
        loop_root_path / std::string(kSuperLoopMemoryFilename);
    const std::filesystem::path legacy_memory_path =
        loop_root_path / std::string(kLegacySuperLoopMemoryFilename);
    const std::filesystem::path raw_memory_path(parsed.memory_path);
    if (parsed.memory_path.empty() ||
        raw_memory_path.lexically_normal() == canonical_memory_path ||
        raw_memory_path.lexically_normal() == legacy_memory_path) {
      parsed.memory_path =
          prefer_canonical_super_loop_file_path(canonical_memory_path,
                                                legacy_memory_path)
              .string();
    }

    const std::filesystem::path canonical_events_path =
        loop_root_path / std::string(kSuperLoopEventsFilename);
    const std::filesystem::path legacy_events_path =
        loop_root_path / std::string(kLegacySuperLoopEventsFilename);
    const std::filesystem::path raw_events_path(parsed.events_path);
    if (parsed.events_path.empty() ||
        raw_events_path.lexically_normal() == canonical_events_path ||
        raw_events_path.lexically_normal() == legacy_events_path) {
      parsed.events_path =
          prefer_canonical_super_loop_file_path(canonical_events_path,
                                                legacy_events_path)
              .string();
    }
  }
  if (!cuwacunu::hero::runtime::parse_u64(kv["started_at_ms"],
                                          &parsed.started_at_ms)) {
    if (error) *error = "super loop record missing started_at_ms";
    return false;
  }
  if (!cuwacunu::hero::runtime::parse_u64(kv["updated_at_ms"],
                                          &parsed.updated_at_ms)) {
    if (error) *error = "super loop record missing updated_at_ms";
    return false;
  }
  std::uint64_t value = 0;
  if (cuwacunu::hero::runtime::parse_u64(kv["finished_at_ms"], &value)) {
    parsed.finished_at_ms = value;
  }
  if (!cuwacunu::hero::runtime::parse_u64(kv["turn_count"],
                                          &parsed.turn_count)) {
    if (error) *error = "super loop record missing turn_count";
    return false;
  }
  if (!cuwacunu::hero::runtime::parse_u64(kv["launch_count"],
                                          &parsed.launch_count)) {
    if (error) *error = "super loop record missing launch_count";
    return false;
  }
  if (!cuwacunu::hero::runtime::parse_u64(kv["max_review_turns"],
                                          &parsed.max_review_turns)) {
    if (error) *error = "super loop record missing max_review_turns";
    return false;
  }
  if (!cuwacunu::hero::runtime::parse_u64(kv["max_campaign_launches"],
                                          &parsed.max_campaign_launches)) {
    if (error) *error = "super loop record missing max_campaign_launches";
    return false;
  }
  if (!cuwacunu::hero::runtime::parse_u64(kv["remaining_review_turns"],
                                          &parsed.remaining_review_turns)) {
    if (error) *error = "super loop record missing remaining_review_turns";
    return false;
  }
  if (!cuwacunu::hero::runtime::parse_u64(kv["remaining_campaign_launches"],
                                          &parsed.remaining_campaign_launches)) {
    if (error) *error = "super loop record missing remaining_campaign_launches";
    return false;
  }
  if (parsed.loop_id.empty()) {
    if (error) *error = "super loop record missing loop_id";
    return false;
  }
  if (parsed.state.empty()) {
    if (error) *error = "super loop record missing state";
    return false;
  }
  if (parsed.global_config_path.empty()) {
    if (error) *error = "super loop record missing global_config_path";
    return false;
  }
  if (parsed.source_super_objective_md_path.empty()) {
    if (error) *error = "super loop record missing source_super_objective_md_path";
    return false;
  }
  if (parsed.source_super_guidance_md_path.empty()) {
    if (error) *error = "super loop record missing source_super_guidance_md_path";
    return false;
  }
  if (parsed.super_objective_md_path.empty()) {
    if (error) *error = "super loop record missing super_objective_md_path";
    return false;
  }
  if (parsed.super_guidance_md_path.empty()) {
    if (error) *error = "super loop record missing super_guidance_md_path";
    return false;
  }
  if (parsed.config_policy_path.empty()) {
    if (error) *error = "super loop record missing config_policy_path";
    return false;
  }
  if (parsed.human_escalation_path.empty()) {
    if (error) *error = "super loop record missing human_escalation_path";
    return false;
  }
  std::map<std::string, std::string, std::less<>> ordered(kv.begin(), kv.end());
  for (const auto& [key, entry] : ordered) {
    if (key.rfind("campaign_cursor.", 0) == 0) {
      parsed.campaign_cursors.push_back(entry);
    }
  }
  *out = std::move(parsed);
  return true;
}

[[nodiscard]] inline bool write_super_loop_record(
    const std::filesystem::path& super_root,
    const super_loop_record_t& record, std::string* error = nullptr) {
  if (error) error->clear();
  if (record.loop_id.empty()) {
    if (error) *error = "super loop record missing loop_id";
    return false;
  }
  const auto manifest_path =
      super_loop_manifest_path(super_root, record.loop_id);
  const std::string text =
      cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
          super_loop_record_to_document(record));
  if (!cuwacunu::hero::runtime::write_text_file_atomic(manifest_path, text,
                                                       error)) {
    return false;
  }
  const auto legacy_manifest_path =
      super_loop_legacy_manifest_path(super_root, record.loop_id);
  if (legacy_manifest_path != manifest_path) {
    std::error_code ec{};
    (void)std::filesystem::remove(legacy_manifest_path, ec);
  }
  return true;
}

[[nodiscard]] inline bool read_super_loop_record(
    const std::filesystem::path& super_root, std::string_view loop_id,
    super_loop_record_t* out, std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for super loop record";
    return false;
  }
  std::string text{};
  const std::filesystem::path manifest_path =
      resolve_existing_super_loop_manifest_path(super_root, loop_id);
  if (!cuwacunu::hero::runtime::read_text_file(
          manifest_path, &text, error)) {
    return false;
  }
  cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t document{};
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          text, &document, error)) {
    return false;
  }
  return parse_super_loop_record_document(document, out, error);
}

[[nodiscard]] inline bool scan_super_loop_records(
    const std::filesystem::path& super_root,
    std::vector<super_loop_record_t>* out, std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for super loop list";
    return false;
  }
  out->clear();

  std::error_code ec{};
  if (!std::filesystem::exists(super_root, ec)) return true;
  if (ec) {
    if (error) *error = "cannot inspect super_root: " + super_root.string();
    return false;
  }

  for (const auto& it : std::filesystem::directory_iterator(super_root, ec)) {
    if (ec) {
      if (error) *error = "cannot iterate super_root: " + super_root.string();
      return false;
    }
    if (!it.is_directory(ec) || ec) continue;
    const std::string loop_id = it.path().filename().string();
    if (loop_id.empty() || loop_id.front() == '.') continue;
    const std::filesystem::path manifest_path =
        resolve_existing_super_loop_manifest_path(super_root, loop_id);
    if (!std::filesystem::exists(manifest_path, ec) ||
        !std::filesystem::is_regular_file(manifest_path, ec)) {
      ec.clear();
      continue;
    }
    super_loop_record_t record{};
    std::string record_error{};
    if (!read_super_loop_record(super_root, loop_id, &record, &record_error)) {
      if (error) {
        *error = "failed reading super loop manifest " +
                 manifest_path.string() +
                 ": " + record_error;
      }
      return false;
    }
    out->push_back(std::move(record));
  }
  std::sort(out->begin(), out->end(), [](const auto& lhs, const auto& rhs) {
    if (lhs.started_at_ms != rhs.started_at_ms) {
      return lhs.started_at_ms < rhs.started_at_ms;
    }
    return lhs.loop_id < rhs.loop_id;
  });
  return true;
}

[[nodiscard]] inline bool is_super_loop_terminal_state(
    std::string_view state) {
  return state == "stopped" || state == "failed" || state == "success" ||
         state == "exhausted";
}

[[nodiscard]] inline bool is_super_loop_finished_report_state(
    std::string_view state) {
  return state == "stopped" || state == "failed" || state == "success" ||
         state == "exhausted";
}

[[nodiscard]] inline bool sync_human_pending_request_count(
    const std::filesystem::path& super_root, std::string* error = nullptr) {
  if (error) error->clear();
  std::vector<super_loop_record_t> loops{};
  if (!scan_super_loop_records(super_root, &loops, error)) return false;

  std::size_t pending = 0;
  for (const auto& loop : loops) {
    if (loop.state == "awaiting_human") ++pending;
  }

  const std::filesystem::path marker_dir = human_hero_runtime_dir(super_root);
  std::error_code ec{};
  std::filesystem::create_directories(marker_dir, ec);
  if (ec) {
    if (error) {
      *error = "cannot create Human Hero runtime dir: " + marker_dir.string();
    }
    return false;
  }

  return cuwacunu::hero::runtime::write_text_file_atomic(
      human_hero_pending_count_path(super_root),
      std::to_string(pending) + "\n", error);
}

[[nodiscard]] inline bool sync_human_pending_report_count(
    const std::filesystem::path& super_root, std::string* error = nullptr) {
  if (error) error->clear();
  std::vector<super_loop_record_t> loops{};
  if (!scan_super_loop_records(super_root, &loops, error)) return false;

  std::size_t pending = 0;
  for (const auto& loop : loops) {
    if (!is_super_loop_finished_report_state(loop.state)) continue;
    const std::filesystem::path report_path =
        super_loop_human_report_path(super_root, loop.loop_id);
    const std::filesystem::path ack_path =
        super_loop_human_report_ack_latest_path(super_root, loop.loop_id);
    std::error_code ec{};
    const bool report_exists =
        std::filesystem::exists(report_path, ec) &&
        std::filesystem::is_regular_file(report_path, ec);
    ec.clear();
    const bool ack_exists =
        std::filesystem::exists(ack_path, ec) &&
        std::filesystem::is_regular_file(ack_path, ec);
    if (!report_exists) {
      continue;
    }
    if (!ack_exists) {
      ++pending;
      continue;
    }
    ec.clear();
    const auto report_time = std::filesystem::last_write_time(report_path, ec);
    if (ec) {
      ++pending;
      continue;
    }
    ec.clear();
    const auto ack_time = std::filesystem::last_write_time(ack_path, ec);
    if (ec || ack_time < report_time) {
      ++pending;
      continue;
    }
  }

  const std::filesystem::path marker_dir = human_hero_runtime_dir(super_root);
  std::error_code ec{};
  std::filesystem::create_directories(marker_dir, ec);
  if (ec) {
    if (error) {
      *error = "cannot create Human Hero runtime dir: " + marker_dir.string();
    }
    return false;
  }

  return cuwacunu::hero::runtime::write_text_file_atomic(
      human_hero_pending_report_count_path(super_root),
      std::to_string(pending) + "\n", error);
}

}  // namespace runtime
}  // namespace hero
}  // namespace cuwacunu
