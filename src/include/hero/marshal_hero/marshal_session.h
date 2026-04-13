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

#include "hero/human_hero/human_attestation.h"
#include "hero/runtime_hero/runtime_job.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"

namespace cuwacunu {
namespace hero {
namespace marshal {

inline constexpr std::string_view kMarshalSessionSchemaV4 =
    "hero.marshal.session.v4";
inline constexpr std::string_view kMarshalSessionManifestFilename =
    "marshal.session.manifest.lls";
inline constexpr std::string_view kMarshalSessionMemoryFilename =
    "marshal.session.memory.md";
inline constexpr std::string_view kMarshalSessionBriefingFilename =
    "marshal.briefing.md";
inline constexpr std::string_view kMarshalSessionCodexStdoutFilename =
    "codex.session.stdout.jsonl";
inline constexpr std::string_view kMarshalSessionCodexStderrFilename =
    "codex.session.stderr.jsonl";
inline constexpr std::string_view kMarshalSessionLegacyCodexSessionLogFilename =
    "codex.session.log";
inline constexpr std::string_view kMarshalSessionCheckpointPidFilename =
    ".codex.checkpoint.pid";
inline constexpr std::string_view kMarshalSessionObjectiveDslFilename =
    "marshal.objective.dsl";
inline constexpr std::string_view kMarshalSessionObjectiveMdFilename =
    "marshal.objective.md";
inline constexpr std::string_view kMarshalSessionGuidanceMdFilename =
    "marshal.guidance.md";
inline constexpr std::string_view kMarshalSessionHeroMarshalDslFilename =
    "hero.marshal.dsl";
inline constexpr std::string_view kMarshalSessionConfigHeroPolicyFilename =
    "config.hero.policy.dsl";
inline constexpr std::string_view kMarshalSessionHumanRequestFilename =
    "request.latest.md";
inline constexpr std::string_view kMarshalSessionHumanSummaryFilename =
    "summary.latest.md";
inline constexpr std::string_view
    kMarshalSessionHumanGovernanceResolutionLatestFilename =
        "governance_resolution.latest.json";
inline constexpr std::string_view
    kMarshalSessionHumanGovernanceResolutionLatestSigFilename =
        "governance_resolution.latest.sig";
inline constexpr std::string_view
    kMarshalSessionHumanClarificationAnswerLatestFilename =
        "clarification_answer.latest.json";
inline constexpr std::string_view kMarshalSessionHumanSummaryAckLatestFilename =
    "summary_ack.latest.json";
inline constexpr std::string_view
    kMarshalSessionHumanSummaryAckLatestSigFilename = "summary_ack.latest.sig";
inline constexpr std::string_view kMarshalSessionEventsFilename =
    "marshal.session.events.jsonl";
inline constexpr std::string_view kMarshalSessionCheckpointsDirname =
    "checkpoints";
inline constexpr std::string_view
    kMarshalSessionHumanGovernanceResolutionsDirname = "governance_resolutions";
inline constexpr std::string_view kMarshalSessionLatestInputCheckpointFilename =
    "input.latest.json";
inline constexpr std::string_view
    kMarshalSessionLatestIntentCheckpointFilename = "intent.latest.json";
inline constexpr std::string_view
    kMarshalSessionLatestMutationCheckpointFilename = "mutation.latest.json";
inline constexpr std::string_view kMarshalSessionLogsDirname = "logs";
inline constexpr std::string_view kMarshalSessionHumanDirname = "human";
inline constexpr std::string_view kHumanHeroRuntimeDirname = ".human_hero";
inline constexpr std::string_view kHumanHeroPendingRequestCountFilename =
    "request.pending.count";
inline constexpr std::string_view kHumanHeroPendingSummaryCountFilename =
    "summary.pending.count";
inline constexpr std::string_view kMarshalSessionRunnerLockFilename =
    ".marshal.session.runner.lock";

[[nodiscard]] inline bool is_marshal_session_runtime_text_char(char ch) {
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

[[nodiscard]] inline std::string
sanitize_marshal_session_runtime_text(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (const char ch : value) {
    char emit = ch;
    if (emit == '\n' || emit == '\r' || emit == '\t' || emit == '#' ||
        emit == ';' || !is_marshal_session_runtime_text_char(emit)) {
      emit = ' ';
    }
    if (emit == ' ') {
      if (out.empty() || out.back() == ' ')
        continue;
      out.push_back(' ');
      continue;
    }
    out.push_back(emit);
  }
  while (!out.empty() && out.front() == ' ')
    out.erase(out.begin());
  while (!out.empty() && out.back() == ' ')
    out.pop_back();
  return out;
}

struct marshal_session_record_t {
  // Identity and high-level state.
  std::string schema{std::string(kMarshalSessionSchemaV4)};
  std::string marshal_session_id{};
  std::string phase{"active"};
  std::string phase_detail{};
  std::string pause_kind{"none"};
  std::string finish_reason{"none"};
  std::string objective_name{};
  std::string global_config_path{};

  // Source-of-truth inputs chosen at session creation time.
  std::string source_marshal_objective_dsl_path{};
  std::string source_campaign_dsl_path{};
  std::string source_marshal_objective_md_path{};
  std::string source_marshal_guidance_md_path{};

  // Live session ledger plus truth-source objective references.
  std::string session_root{};
  std::string objective_root{};
  std::string campaign_dsl_path{};
  std::string marshal_objective_dsl_path{};
  std::string marshal_objective_md_path{};
  std::string marshal_guidance_md_path{};

  // Generated operational artifacts used during Marshal Hero checkpoint
  // execution.
  std::string hero_marshal_dsl_path{};
  std::string config_policy_path{};
  std::string briefing_path{};
  std::string memory_path{};
  std::string human_request_path{};
  std::string events_path{};
  std::string codex_stdout_path{};
  std::string codex_stderr_path{};
  std::string codex_session_id{};
  std::string resolved_marshal_codex_binary{};
  std::string resolved_marshal_codex_model{};
  std::string resolved_marshal_codex_reasoning_effort{};

  // Checkpoint lifecycle.
  std::uint64_t started_at_ms{0};
  std::uint64_t updated_at_ms{0};
  std::optional<std::uint64_t> finished_at_ms{};
  std::uint64_t checkpoint_count{0};
  std::uint64_t launch_count{0};
  std::uint64_t max_campaign_launches{0};
  std::uint64_t remaining_campaign_launches{0};
  std::string authority_scope{"objective_only"};
  std::string latest_request_kind{};

  // Latest execution status and evidence pointers.
  std::string active_campaign_cursor{};
  std::string last_intent_kind{};
  std::string last_warning{};
  std::string last_input_checkpoint_path{};
  std::string last_intent_checkpoint_path{};
  std::string last_mutation_checkpoint_path{};

  // Historical campaign trail for this session.
  std::vector<std::string> campaign_cursors{};
};

[[nodiscard]] inline std::filesystem::path
marshal_root(const std::filesystem::path &runtime_root) {
  if (runtime_root.empty())
    return {};
  return (runtime_root / ".marshal_hero").lexically_normal();
}

[[nodiscard]] inline std::filesystem::path
human_hero_runtime_dir(const std::filesystem::path &marshal_root) {
  if (marshal_root.empty())
    return {};
  return (marshal_root.parent_path() / std::string(kHumanHeroRuntimeDirname))
      .lexically_normal();
}

[[nodiscard]] inline std::filesystem::path
human_hero_pending_count_path(const std::filesystem::path &marshal_root) {
  return human_hero_runtime_dir(marshal_root) /
         std::string(kHumanHeroPendingRequestCountFilename);
}

[[nodiscard]] inline std::filesystem::path
human_hero_pending_summary_count_path(
    const std::filesystem::path &marshal_root) {
  return human_hero_runtime_dir(marshal_root) /
         std::string(kHumanHeroPendingSummaryCountFilename);
}

[[nodiscard]] inline std::string format_marshal_index(std::size_t index) {
  std::ostringstream out;
  out << std::setw(4) << std::setfill('0') << index;
  return out.str();
}

[[nodiscard]] inline std::filesystem::path
marshal_session_dir(const std::filesystem::path &marshal_root,
                    std::string_view marshal_session_id) {
  return marshal_root / std::string(marshal_session_id);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_manifest_path(const std::filesystem::path &marshal_root,
                              std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionManifestFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_runner_lock_path(const std::filesystem::path &marshal_root,
                                 std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionRunnerLockFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_memory_path(const std::filesystem::path &marshal_root,
                            std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionMemoryFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_briefing_path(const std::filesystem::path &marshal_root,
                              std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionBriefingFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_logs_dir(const std::filesystem::path &marshal_root,
                         std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionLogsDirname);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_codex_stdout_path(const std::filesystem::path &marshal_root,
                                  std::string_view marshal_session_id) {
  return marshal_session_logs_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionCodexStdoutFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_codex_stderr_path(const std::filesystem::path &marshal_root,
                                  std::string_view marshal_session_id) {
  return marshal_session_logs_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionCodexStderrFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_legacy_codex_session_log_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id) {
  return marshal_session_logs_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionLegacyCodexSessionLogFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_checkpoint_pid_path(const std::filesystem::path &marshal_root,
                                    std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionCheckpointPidFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_objective_dsl_path(const std::filesystem::path &marshal_root,
                                   std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionObjectiveDslFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_objective_md_path(const std::filesystem::path &marshal_root,
                                  std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionObjectiveMdFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_guidance_md_path(const std::filesystem::path &marshal_root,
                                 std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionGuidanceMdFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_hero_marshal_dsl_path(const std::filesystem::path &marshal_root,
                                      std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionHeroMarshalDslFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_config_policy_path(const std::filesystem::path &marshal_root,
                                   std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionConfigHeroPolicyFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_human_dir(const std::filesystem::path &marshal_root,
                          std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionHumanDirname);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_human_request_path(const std::filesystem::path &marshal_root,
                                   std::string_view marshal_session_id) {
  return marshal_session_human_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionHumanRequestFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_human_summary_path(const std::filesystem::path &marshal_root,
                                   std::string_view marshal_session_id) {
  return marshal_session_human_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionHumanSummaryFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_events_path(const std::filesystem::path &marshal_root,
                            std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionEventsFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_human_governance_resolution_latest_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id) {
  return marshal_session_human_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionHumanGovernanceResolutionLatestFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_human_governance_resolution_latest_sig_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id) {
  return marshal_session_human_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionHumanGovernanceResolutionLatestSigFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_human_clarification_answer_latest_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id) {
  return marshal_session_human_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionHumanClarificationAnswerLatestFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_human_summary_ack_latest_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id) {
  return marshal_session_human_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionHumanSummaryAckLatestFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_human_summary_ack_latest_sig_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id) {
  return marshal_session_human_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionHumanSummaryAckLatestSigFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_checkpoints_dir(const std::filesystem::path &marshal_root,
                                std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionCheckpointsDirname);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_human_governance_resolutions_dir(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id) {
  return marshal_session_human_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionHumanGovernanceResolutionsDirname);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_latest_input_checkpoint_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id) {
  return marshal_session_checkpoints_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionLatestInputCheckpointFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_latest_intent_checkpoint_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id) {
  return marshal_session_checkpoints_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionLatestIntentCheckpointFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_latest_mutation_checkpoint_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id) {
  return marshal_session_checkpoints_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionLatestMutationCheckpointFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_input_checkpoint_path(const std::filesystem::path &marshal_root,
                                      std::string_view marshal_session_id,
                                      std::uint64_t checkpoint_index) {
  return marshal_session_checkpoints_dir(marshal_root, marshal_session_id) /
         ("input." +
          format_marshal_index(static_cast<std::size_t>(checkpoint_index)) +
          ".json");
}

[[nodiscard]] inline std::filesystem::path
marshal_session_intent_checkpoint_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id, std::uint64_t checkpoint_index) {
  return marshal_session_checkpoints_dir(marshal_root, marshal_session_id) /
         ("intent." +
          format_marshal_index(static_cast<std::size_t>(checkpoint_index)) +
          ".json");
}

[[nodiscard]] inline std::filesystem::path
marshal_session_mutation_checkpoint_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id, std::uint64_t checkpoint_index) {
  return marshal_session_checkpoints_dir(marshal_root, marshal_session_id) /
         ("mutation." +
          format_marshal_index(static_cast<std::size_t>(checkpoint_index)) +
          ".json");
}

[[nodiscard]] inline std::filesystem::path
marshal_session_human_governance_resolution_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id, std::uint64_t checkpoint_index) {
  return marshal_session_human_governance_resolutions_dir(marshal_root,
                                                          marshal_session_id) /
         ("governance_resolution." +
          format_marshal_index(static_cast<std::size_t>(checkpoint_index)) +
          ".json");
}

[[nodiscard]] inline std::filesystem::path
marshal_session_human_governance_resolution_sig_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id, std::uint64_t checkpoint_index) {
  return marshal_session_human_governance_resolutions_dir(marshal_root,
                                                          marshal_session_id) /
         ("governance_resolution." +
          format_marshal_index(static_cast<std::size_t>(checkpoint_index)) +
          ".sig");
}

[[nodiscard]] inline std::filesystem::path
marshal_session_human_clarification_answer_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id, std::uint64_t checkpoint_index) {
  return marshal_session_human_dir(marshal_root, marshal_session_id) /
         ("clarification_answer." +
          format_marshal_index(static_cast<std::size_t>(checkpoint_index)) +
          ".json");
}

[[nodiscard]] inline std::string
make_marshal_session_id(const std::filesystem::path &marshal_root) {
  const std::uint64_t started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  const std::string stem =
      "marshal." + cuwacunu::hero::runtime::base36_lower_u64(started_at_ms);
  for (std::uint64_t collision = 0; collision != 0xffffffffffffffffULL;
       ++collision) {
    std::string candidate = stem;
    if (collision != 0) {
      candidate += "." + cuwacunu::hero::runtime::base36_lower_u64(collision);
    }
    std::error_code ec{};
    if (!std::filesystem::exists(marshal_session_dir(marshal_root, candidate),
                                 ec) ||
        ec) {
      return candidate;
    }
  }
  return stem + ".overflow";
}

[[nodiscard]] inline cuwacunu::piaabo::latent_lineage_state::
    runtime_lls_document_t
    marshal_session_record_to_document(const marshal_session_record_t &record) {
  using namespace cuwacunu::piaabo::latent_lineage_state;
  runtime_lls_document_t document{};
  document.entries.push_back(
      make_runtime_lls_string_entry("schema", record.schema));
  document.entries.push_back(make_runtime_lls_string_entry(
      "marshal_session_id", record.marshal_session_id));
  document.entries.push_back(
      make_runtime_lls_string_entry("phase", record.phase));
  document.entries.push_back(make_runtime_lls_string_entry(
      "phase_detail",
      sanitize_marshal_session_runtime_text(record.phase_detail)));
  document.entries.push_back(
      make_runtime_lls_string_entry("pause_kind", record.pause_kind));
  document.entries.push_back(
      make_runtime_lls_string_entry("finish_reason", record.finish_reason));
  document.entries.push_back(
      make_runtime_lls_string_entry("objective_name", record.objective_name));
  document.entries.push_back(make_runtime_lls_string_entry(
      "global_config_path", record.global_config_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("source_marshal_objective_dsl_path",
                                    record.source_marshal_objective_dsl_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "source_campaign_dsl_path", record.source_campaign_dsl_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("source_marshal_objective_md_path",
                                    record.source_marshal_objective_md_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("source_marshal_guidance_md_path",
                                    record.source_marshal_guidance_md_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("session_root", record.session_root));
  document.entries.push_back(
      make_runtime_lls_string_entry("objective_root", record.objective_root));
  document.entries.push_back(make_runtime_lls_string_entry(
      "campaign_dsl_path", record.campaign_dsl_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "marshal_objective_dsl_path", record.marshal_objective_dsl_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "marshal_objective_md_path", record.marshal_objective_md_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "marshal_guidance_md_path", record.marshal_guidance_md_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "hero_marshal_dsl_path", record.hero_marshal_dsl_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "config_policy_path", record.config_policy_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("briefing_path", record.briefing_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("memory_path", record.memory_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "human_request_path", record.human_request_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("events_path", record.events_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "codex_stdout_path", record.codex_stdout_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "codex_stderr_path", record.codex_stderr_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "codex_session_id", record.codex_session_id));
  document.entries.push_back(make_runtime_lls_string_entry(
      "resolved_marshal_codex_binary", record.resolved_marshal_codex_binary));
  document.entries.push_back(make_runtime_lls_string_entry(
      "resolved_marshal_codex_model", record.resolved_marshal_codex_model));
  document.entries.push_back(make_runtime_lls_string_entry(
      "resolved_marshal_codex_reasoning_effort",
      record.resolved_marshal_codex_reasoning_effort));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "started_at_ms", record.started_at_ms, "(0,+inf)"));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "updated_at_ms", record.updated_at_ms, "(0,+inf)"));
  if (record.finished_at_ms.has_value()) {
    document.entries.push_back(make_runtime_lls_uint_entry(
        "finished_at_ms", *record.finished_at_ms, "(0,+inf)"));
  }
  document.entries.push_back(make_runtime_lls_uint_entry(
      "checkpoint_count", record.checkpoint_count, "[0,+inf)"));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "launch_count", record.launch_count, "[0,+inf)"));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "max_campaign_launches", record.max_campaign_launches, "[0,+inf)"));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "remaining_campaign_launches", record.remaining_campaign_launches,
      "[0,+inf)"));
  document.entries.push_back(
      make_runtime_lls_string_entry("authority_scope", record.authority_scope));
  document.entries.push_back(make_runtime_lls_string_entry(
      "latest_request_kind", record.latest_request_kind));
  document.entries.push_back(make_runtime_lls_string_entry(
      "active_campaign_cursor", record.active_campaign_cursor));
  document.entries.push_back(make_runtime_lls_string_entry(
      "last_intent_kind", record.last_intent_kind));
  document.entries.push_back(make_runtime_lls_string_entry(
      "last_warning",
      sanitize_marshal_session_runtime_text(record.last_warning)));
  document.entries.push_back(make_runtime_lls_string_entry(
      "last_input_checkpoint_path", record.last_input_checkpoint_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "last_intent_checkpoint_path", record.last_intent_checkpoint_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "last_mutation_checkpoint_path", record.last_mutation_checkpoint_path));
  for (std::size_t i = 0; i < record.campaign_cursors.size(); ++i) {
    document.entries.push_back(make_runtime_lls_string_entry(
        "campaign_cursor." + format_marshal_index(i),
        record.campaign_cursors[i]));
  }
  return document;
}

[[nodiscard]] inline bool parse_marshal_session_record_document(
    const cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t
        &document,
    marshal_session_record_t *out, std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "missing destination for marshal session record";
    return false;
  }
  std::unordered_map<std::string, std::string> kv{};
  if (!cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          document, &kv, error)) {
    return false;
  }

  marshal_session_record_t parsed{};
  parsed.schema = kv["schema"];
  if (parsed.schema != kMarshalSessionSchemaV4) {
    if (error)
      *error = "unexpected marshal session schema: " + parsed.schema;
    return false;
  }
  parsed.marshal_session_id = kv["marshal_session_id"];
  parsed.phase = kv["phase"];
  parsed.phase_detail = kv["phase_detail"];
  parsed.pause_kind = kv["pause_kind"];
  parsed.finish_reason = kv["finish_reason"];
  parsed.objective_name = kv["objective_name"];
  parsed.global_config_path = kv["global_config_path"];
  parsed.source_marshal_objective_dsl_path =
      kv["source_marshal_objective_dsl_path"];
  parsed.source_campaign_dsl_path = kv["source_campaign_dsl_path"];
  parsed.source_marshal_objective_md_path =
      kv["source_marshal_objective_md_path"];
  parsed.source_marshal_guidance_md_path =
      kv["source_marshal_guidance_md_path"];
  parsed.session_root = kv["session_root"];
  parsed.objective_root = kv["objective_root"];
  parsed.campaign_dsl_path = kv["campaign_dsl_path"];
  parsed.marshal_objective_dsl_path = kv["marshal_objective_dsl_path"];
  parsed.marshal_objective_md_path = kv["marshal_objective_md_path"];
  parsed.marshal_guidance_md_path = kv["marshal_guidance_md_path"];
  parsed.hero_marshal_dsl_path = kv["hero_marshal_dsl_path"];
  parsed.config_policy_path = kv["config_policy_path"];
  parsed.briefing_path = kv["briefing_path"];
  parsed.memory_path = kv["memory_path"];
  parsed.human_request_path = kv["human_request_path"];
  parsed.events_path = kv["events_path"];
  parsed.codex_stdout_path = kv["codex_stdout_path"];
  parsed.codex_stderr_path = kv["codex_stderr_path"];
  parsed.codex_session_id = kv["codex_session_id"];
  parsed.resolved_marshal_codex_binary = kv["resolved_marshal_codex_binary"];
  parsed.resolved_marshal_codex_model = kv["resolved_marshal_codex_model"];
  parsed.resolved_marshal_codex_reasoning_effort =
      kv["resolved_marshal_codex_reasoning_effort"];
  parsed.active_campaign_cursor = kv["active_campaign_cursor"];
  parsed.last_intent_kind = kv["last_intent_kind"];
  parsed.last_warning = kv["last_warning"];
  parsed.last_input_checkpoint_path = kv["last_input_checkpoint_path"];
  parsed.last_intent_checkpoint_path = kv["last_intent_checkpoint_path"];
  parsed.last_mutation_checkpoint_path = kv["last_mutation_checkpoint_path"];
  parsed.authority_scope = kv["authority_scope"];
  parsed.latest_request_kind = kv["latest_request_kind"];
  if (!cuwacunu::hero::runtime::parse_u64(kv["started_at_ms"],
                                          &parsed.started_at_ms)) {
    if (error)
      *error = "marshal session record missing started_at_ms";
    return false;
  }
  if (!cuwacunu::hero::runtime::parse_u64(kv["updated_at_ms"],
                                          &parsed.updated_at_ms)) {
    if (error)
      *error = "marshal session record missing updated_at_ms";
    return false;
  }
  std::uint64_t value = 0;
  if (cuwacunu::hero::runtime::parse_u64(kv["finished_at_ms"], &value)) {
    parsed.finished_at_ms = value;
  }
  if (!cuwacunu::hero::runtime::parse_u64(kv["checkpoint_count"],
                                          &parsed.checkpoint_count)) {
    if (error)
      *error = "marshal session record missing checkpoint_count";
    return false;
  }
  if (!cuwacunu::hero::runtime::parse_u64(kv["launch_count"],
                                          &parsed.launch_count)) {
    if (error)
      *error = "marshal session record missing launch_count";
    return false;
  }
  if (!cuwacunu::hero::runtime::parse_u64(kv["max_campaign_launches"],
                                          &parsed.max_campaign_launches)) {
    if (error)
      *error = "marshal session record missing max_campaign_launches";
    return false;
  }
  if (!cuwacunu::hero::runtime::parse_u64(
          kv["remaining_campaign_launches"],
          &parsed.remaining_campaign_launches)) {
    if (error)
      *error = "marshal session record missing remaining_campaign_launches";
    return false;
  }
  if (parsed.marshal_session_id.empty()) {
    if (error)
      *error = "marshal session record missing marshal_session_id";
    return false;
  }
  if (parsed.phase.empty()) {
    if (error)
      *error = "marshal session record missing phase";
    return false;
  }
  if (parsed.pause_kind.empty()) {
    if (error)
      *error = "marshal session record missing pause_kind";
    return false;
  }
  if (parsed.finish_reason.empty()) {
    if (error)
      *error = "marshal session record missing finish_reason";
    return false;
  }
  if (parsed.global_config_path.empty()) {
    if (error)
      *error = "marshal session record missing global_config_path";
    return false;
  }
  if (parsed.source_marshal_objective_md_path.empty()) {
    if (error)
      *error =
          "marshal session record missing source_marshal_objective_md_path";
    return false;
  }
  if (parsed.source_marshal_guidance_md_path.empty()) {
    if (error)
      *error = "marshal session record missing source_marshal_guidance_md_path";
    return false;
  }
  if (parsed.marshal_objective_md_path.empty()) {
    if (error)
      *error = "marshal session record missing marshal_objective_md_path";
    return false;
  }
  if (parsed.marshal_guidance_md_path.empty()) {
    if (error)
      *error = "marshal session record missing marshal_guidance_md_path";
    return false;
  }
  if (parsed.hero_marshal_dsl_path.empty()) {
    if (error)
      *error = "marshal session record missing hero_marshal_dsl_path";
    return false;
  }
  if (parsed.config_policy_path.empty()) {
    if (error)
      *error = "marshal session record missing config_policy_path";
    return false;
  }
  if (parsed.human_request_path.empty()) {
    if (error)
      *error = "marshal session record missing human_request_path";
    return false;
  }
  std::map<std::string, std::string, std::less<>> ordered(kv.begin(), kv.end());
  for (const auto &[key, entry] : ordered) {
    if (key.rfind("campaign_cursor.", 0) == 0) {
      parsed.campaign_cursors.push_back(entry);
    }
  }
  *out = std::move(parsed);
  return true;
}

[[nodiscard]] inline bool
write_marshal_session_record(const std::filesystem::path &marshal_root,
                             const marshal_session_record_t &record,
                             std::string *error = nullptr) {
  if (error)
    error->clear();
  if (record.marshal_session_id.empty()) {
    if (error)
      *error = "marshal session record missing marshal_session_id";
    return false;
  }
  const auto manifest_path =
      marshal_session_manifest_path(marshal_root, record.marshal_session_id);
  const std::string text =
      cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
          marshal_session_record_to_document(record));
  if (!cuwacunu::hero::runtime::write_text_file_atomic(manifest_path, text,
                                                       error)) {
    return false;
  }
  return true;
}

[[nodiscard]] inline bool
read_marshal_session_record(const std::filesystem::path &marshal_root,
                            std::string_view marshal_session_id,
                            marshal_session_record_t *out,
                            std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "missing destination for marshal session record";
    return false;
  }
  std::string text{};
  const std::filesystem::path manifest_path =
      marshal_session_manifest_path(marshal_root, marshal_session_id);
  if (!cuwacunu::hero::runtime::read_text_file(manifest_path, &text, error)) {
    return false;
  }
  cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t document{};
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          text, &document, error)) {
    return false;
  }
  return parse_marshal_session_record_document(document, out, error);
}

[[nodiscard]] inline bool
scan_marshal_session_records(const std::filesystem::path &marshal_root,
                             std::vector<marshal_session_record_t> *out,
                             std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "missing destination for marshal session list";
    return false;
  }
  out->clear();

  std::error_code ec{};
  if (!std::filesystem::exists(marshal_root, ec))
    return true;
  if (ec) {
    if (error)
      *error = "cannot inspect marshal_root: " + marshal_root.string();
    return false;
  }

  for (const auto &it : std::filesystem::directory_iterator(marshal_root, ec)) {
    if (ec) {
      if (error)
        *error = "cannot iterate marshal_root: " + marshal_root.string();
      return false;
    }
    if (!it.is_directory(ec) || ec)
      continue;
    const std::string marshal_session_id = it.path().filename().string();
    if (marshal_session_id.empty() || marshal_session_id.front() == '.')
      continue;
    const std::filesystem::path manifest_path =
        marshal_session_manifest_path(marshal_root, marshal_session_id);
    if (!std::filesystem::exists(manifest_path, ec) ||
        !std::filesystem::is_regular_file(manifest_path, ec)) {
      ec.clear();
      continue;
    }
    marshal_session_record_t record{};
    std::string record_error{};
    if (!read_marshal_session_record(marshal_root, marshal_session_id, &record,
                                     &record_error)) {
      if (error) {
        *error = "failed reading marshal session manifest " +
                 manifest_path.string() + ": " + record_error;
      }
      return false;
    }
    out->push_back(std::move(record));
  }
  std::sort(out->begin(), out->end(), [](const auto &lhs, const auto &rhs) {
    if (lhs.started_at_ms != rhs.started_at_ms) {
      return lhs.started_at_ms < rhs.started_at_ms;
    }
    return lhs.marshal_session_id < rhs.marshal_session_id;
  });
  return true;
}

[[nodiscard]] inline bool
is_marshal_session_terminal_state(const marshal_session_record_t &record) {
  return record.phase == "finished";
}

[[nodiscard]] inline bool
is_marshal_session_terminal_state(std::string_view phase) {
  return phase == "finished";
}

[[nodiscard]] inline bool
is_marshal_session_summary_state(const marshal_session_record_t &record) {
  return record.phase == "idle" || record.phase == "finished";
}

[[nodiscard]] inline bool
is_marshal_session_summary_state(std::string_view phase) {
  return phase == "idle" || phase == "finished";
}

[[nodiscard]] inline bool extract_marshal_session_json_string_field(
    std::string_view json, std::string_view key, std::string *out) {
  if (!out)
    return false;
  out->clear();
  const std::string needle = "\"" + std::string(key) + "\"";
  const std::size_t key_pos = json.find(needle);
  if (key_pos == std::string_view::npos)
    return false;
  const std::size_t colon_pos = json.find(':', key_pos + needle.size());
  if (colon_pos == std::string_view::npos)
    return false;
  std::size_t value_pos = colon_pos + 1;
  while (value_pos < json.size() &&
         (json[value_pos] == ' ' || json[value_pos] == '\t' ||
          json[value_pos] == '\r' || json[value_pos] == '\n')) {
    ++value_pos;
  }
  if (value_pos >= json.size() || json[value_pos] != '"')
    return false;
  ++value_pos;
  std::string value{};
  while (value_pos < json.size()) {
    const char ch = json[value_pos++];
    if (ch == '\\') {
      if (value_pos >= json.size())
        return false;
      value.push_back(static_cast<char>(json[value_pos++]));
      continue;
    }
    if (ch == '"') {
      *out = std::move(value);
      return true;
    }
    value.push_back(ch);
  }
  return false;
}

[[nodiscard]] inline bool marshal_session_summary_ack_matches_current_summary(
    const std::filesystem::path &marshal_root,
    const marshal_session_record_t &session, std::string *error = nullptr) {
  if (error)
    error->clear();
  const std::filesystem::path summary_path = marshal_session_human_summary_path(
      marshal_root, session.marshal_session_id);
  const std::filesystem::path ack_path =
      marshal_session_human_summary_ack_latest_path(marshal_root,
                                                    session.marshal_session_id);
  if (!std::filesystem::exists(summary_path) ||
      !std::filesystem::is_regular_file(summary_path)) {
    if (error)
      *error = "session summary is missing";
    return false;
  }
  if (!std::filesystem::exists(ack_path) ||
      !std::filesystem::is_regular_file(ack_path)) {
    return false;
  }
  std::string summary_sha256_hex{};
  if (!cuwacunu::hero::human::sha256_hex_file(summary_path, &summary_sha256_hex,
                                              error)) {
    return false;
  }
  std::string ack_json{};
  if (!cuwacunu::hero::runtime::read_text_file(ack_path, &ack_json, error)) {
    return false;
  }
  std::string acknowledged_summary_sha256_hex{};
  if (!extract_marshal_session_json_string_field(
          ack_json, "summary_sha256_hex", &acknowledged_summary_sha256_hex)) {
    if (error)
      *error = "summary ack is missing summary_sha256_hex";
    return false;
  }
  return acknowledged_summary_sha256_hex == summary_sha256_hex;
}

[[nodiscard]] inline bool marshal_session_summary_ack_matches_current_summary(
    const marshal_session_record_t &session, std::string *error = nullptr) {
  return marshal_session_summary_ack_matches_current_summary(
      std::filesystem::path(session.session_root).parent_path(), session,
      error);
}

[[nodiscard]] inline bool
sync_human_pending_request_count(const std::filesystem::path &marshal_root,
                                 std::string *error = nullptr) {
  if (error)
    error->clear();
  std::vector<marshal_session_record_t> sessions{};
  if (!scan_marshal_session_records(marshal_root, &sessions, error))
    return false;

  std::size_t pending = 0;
  for (const auto &session : sessions) {
    if (session.phase == "paused" && ((session.pause_kind == "clarification") ||
                                      session.pause_kind == "governance")) {
      ++pending;
    }
  }

  const std::filesystem::path marker_dir = human_hero_runtime_dir(marshal_root);
  std::error_code ec{};
  std::filesystem::create_directories(marker_dir, ec);
  if (ec) {
    if (error) {
      *error = "cannot create Human Hero runtime dir: " + marker_dir.string();
    }
    return false;
  }

  return cuwacunu::hero::runtime::write_text_file_atomic(
      human_hero_pending_count_path(marshal_root),
      std::to_string(pending) + "\n", error);
}

[[nodiscard]] inline bool
sync_human_pending_summary_count(const std::filesystem::path &marshal_root,
                                 std::string *error = nullptr) {
  if (error)
    error->clear();
  std::vector<marshal_session_record_t> sessions{};
  if (!scan_marshal_session_records(marshal_root, &sessions, error))
    return false;

  std::size_t pending = 0;
  for (const auto &session : sessions) {
    if (!is_marshal_session_summary_state(session))
      continue;
    const std::filesystem::path summary_path =
        marshal_session_human_summary_path(marshal_root,
                                           session.marshal_session_id);
    std::error_code ec{};
    const bool summary_exists =
        std::filesystem::exists(summary_path, ec) &&
        std::filesystem::is_regular_file(summary_path, ec);
    if (!summary_exists) {
      continue;
    }
    std::string ack_error{};
    if (!marshal_session_summary_ack_matches_current_summary(
            marshal_root, session, &ack_error)) {
      ++pending;
    }
  }

  const std::filesystem::path marker_dir = human_hero_runtime_dir(marshal_root);
  std::error_code ec{};
  std::filesystem::create_directories(marker_dir, ec);
  if (ec) {
    if (error) {
      *error = "cannot create Human Hero runtime dir: " + marker_dir.string();
    }
    return false;
  }

  return cuwacunu::hero::runtime::write_text_file_atomic(
      human_hero_pending_summary_count_path(marshal_root),
      std::to_string(pending) + "\n", error);
}

} // namespace marshal
} // namespace hero
} // namespace cuwacunu
