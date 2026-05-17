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
inline constexpr std::string_view kMarshalSessionSchemaV5 =
    "hero.marshal.session.v5";
inline constexpr std::string_view kMarshalSessionSchemaV6 =
    "hero.marshal.session.v6";
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
inline constexpr std::string_view kMarshalSessionRunnerStdoutFilename =
    "runner.stdout.log";
inline constexpr std::string_view kMarshalSessionRunnerStderrFilename =
    "runner.stderr.log";
inline constexpr std::string_view kMarshalSessionCompatCodexSessionLogFilename =
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
inline constexpr std::string_view kMarshalSessionTurnsFilename =
    "marshal.session.turns.jsonl";
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
  struct operator_message_t {
    std::string message_id{};
    std::string text{};
    std::string delivery_mode{"queued"};
    std::string delivery_status{"received"};
    std::string thread_id_at_delivery{};
    std::optional<std::uint64_t> received_at_ms{};
    std::optional<std::uint64_t> delivered_at_ms{};
    std::optional<std::uint64_t> handled_at_ms{};
    std::uint64_t delivery_attempts{0};
    std::string last_error{};
  };

  // Identity and high-level state.
  std::string schema{std::string(kMarshalSessionSchemaV6)};
  std::string marshal_session_id{};
  std::string lifecycle{"bootstrapping"};
  std::string status_detail{};
  std::string work_gate{"open"};
  std::string finish_reason{"none"};
  std::string activity{"quiet"};
  std::string objective_name{};
  std::string global_config_path{};
  std::string boot_id{};
  std::optional<std::uint64_t> runner_pid{};
  std::optional<std::uint64_t> runner_start_ticks{};

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
  std::string turns_path{};
  std::string codex_stdout_path{};
  std::string codex_stderr_path{};
  std::string current_thread_id{};
  std::string codex_continuity{"attached"};
  std::string last_resume_error{};
  std::string resolved_marshal_codex_binary{};
  std::string resolved_marshal_codex_model{};
  std::string resolved_marshal_codex_reasoning_effort{};
  std::vector<std::string> thread_lineage{};

  // Checkpoint lifecycle.
  std::uint64_t started_at_ms{0};
  std::uint64_t updated_at_ms{0};
  std::optional<std::uint64_t> finished_at_ms{};
  std::uint64_t checkpoint_count{0};
  std::uint64_t launch_count{0};
  std::uint64_t max_campaign_launches{0};
  std::uint64_t remaining_campaign_launches{0};
  std::string authority_scope{"objective_only"};
  std::string campaign_status{"none"};
  std::string campaign_cursor{};
  std::uint64_t next_message_seq{0};
  std::vector<operator_message_t> pending_operator_messages{};
  std::uint64_t last_event_seq{0};

  // Latest execution status and evidence pointers.
  std::string last_codex_action{};
  std::string last_warning{};
  std::string last_warning_code{};
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

[[nodiscard]] inline std::string
normalize_marshal_lifecycle(std::string_view lifecycle) {
  const std::string value = std::string(lifecycle);
  if (value == "live" || value == "terminating" || value == "terminal" ||
      value == "bootstrapping") {
    return value;
  }
  if (value == "active" || value == "running_campaign" || value == "paused" ||
      value == "idle") {
    return "live";
  }
  if (value == "finished") {
    return "terminal";
  }
  return "bootstrapping";
}

[[nodiscard]] inline std::string
normalize_marshal_work_gate(std::string_view work_gate) {
  const std::string value = std::string(work_gate);
  if (value == "open" || value == "operator_pause" ||
      value == "clarification" || value == "governance") {
    return value;
  }
  if (value == "none")
    return "open";
  if (value == "operator")
    return "operator_pause";
  return "open";
}

[[nodiscard]] inline std::string
normalize_marshal_activity(std::string_view activity) {
  const std::string value = std::string(activity);
  if (value == "quiet" || value == "planning" || value == "handling_message" ||
      value == "awaiting_campaign_fact" || value == "resuming_thread" ||
      value == "review" || value == "bootstrapping") {
    return value;
  }
  if (value == "active")
    return "planning";
  if (value == "running_campaign")
    return "awaiting_campaign_fact";
  if (value == "idle")
    return "review";
  if (value == "paused" || value == "finished")
    return "quiet";
  return "quiet";
}

[[nodiscard]] inline std::string
normalize_marshal_campaign_status(std::string_view status) {
  const std::string value = std::string(status);
  if (value == "none" || value == "launching" || value == "running" ||
      value == "stopping") {
    return value;
  }
  return "none";
}

[[nodiscard]] inline std::string
normalize_marshal_message_delivery_mode(std::string_view mode) {
  const std::string value = std::string(mode);
  if (value == "live" || value == "queued" || value == "checkpoint") {
    return value;
  }
  return "queued";
}

[[nodiscard]] inline std::string
normalize_marshal_message_delivery_status(std::string_view status) {
  const std::string value = std::string(status);
  if (value == "received" || value == "delivered" || value == "handled" ||
      value == "failed") {
    return value;
  }
  return "received";
}

inline void canonicalize_marshal_operator_message(
    marshal_session_record_t::operator_message_t *message) {
  if (!message)
    return;
  message->delivery_mode =
      normalize_marshal_message_delivery_mode(message->delivery_mode);
  message->delivery_status =
      normalize_marshal_message_delivery_status(message->delivery_status);
  if (message->message_id.empty() &&
      !cuwacunu::hero::runtime::trim_ascii(message->text).empty()) {
    message->message_id = "legacy";
  }
}

[[nodiscard]] inline std::string
normalize_marshal_codex_continuity(std::string_view continuity);
[[nodiscard]] inline std::string
import_v5_phase_as_activity(std::string_view phase);
[[nodiscard]] inline std::string
import_v5_phase_as_campaign_status(std::string_view phase,
                                   std::string_view v5_campaign_cursor);

inline void
canonicalize_marshal_session_record(marshal_session_record_t *record) {
  if (!record)
    return;
  const std::string raw_lifecycle = record->lifecycle;
  record->lifecycle = normalize_marshal_lifecycle(record->lifecycle);
  record->work_gate = normalize_marshal_work_gate(record->work_gate);
  record->activity = normalize_marshal_activity(record->activity);
  record->codex_continuity =
      normalize_marshal_codex_continuity(record->codex_continuity);
  record->campaign_status =
      normalize_marshal_campaign_status(record->campaign_status);

  if ((raw_lifecycle == "active" || raw_lifecycle == "running_campaign" ||
       raw_lifecycle == "paused" || raw_lifecycle == "idle" ||
       raw_lifecycle == "finished") &&
      record->activity == "quiet") {
    record->activity = import_v5_phase_as_activity(raw_lifecycle);
  }
  if (record->campaign_status == "none") {
    record->campaign_status = import_v5_phase_as_campaign_status(
        raw_lifecycle, record->campaign_cursor);
  }
  for (auto &message : record->pending_operator_messages) {
    canonicalize_marshal_operator_message(&message);
  }
  if (!cuwacunu::hero::runtime::trim_ascii(record->current_thread_id).empty() &&
      record->thread_lineage.empty()) {
    record->thread_lineage.push_back(record->current_thread_id);
  }
  if (record->lifecycle == "terminal") {
    record->activity = "quiet";
    record->work_gate = "open";
    if (record->campaign_status == "running" ||
        record->campaign_status == "launching") {
      record->campaign_status = "stopping";
    }
  }
  const bool has_retained_campaign_cursor =
      !cuwacunu::hero::runtime::trim_ascii(record->campaign_cursor).empty();
  const bool state_cannot_retain_active_campaign =
      record->lifecycle == "terminal" || record->activity == "review" ||
      (record->lifecycle == "live" && record->work_gate != "open");
  if (!has_retained_campaign_cursor && state_cannot_retain_active_campaign) {
    record->campaign_status = "none";
  }
}

[[nodiscard]] inline std::string
normalize_marshal_codex_continuity(std::string_view continuity) {
  const std::string value = std::string(continuity);
  if (value == "attached" || value == "resuming" || value == "resume_failed" ||
      value == "restarted" || value == "unavailable") {
    return value;
  }
  return "attached";
}

[[nodiscard]] inline std::string
import_v5_phase_as_lifecycle(std::string_view phase) {
  if (phase == "finished")
    return "terminal";
  return "live";
}

[[nodiscard]] inline std::string
import_v5_phase_as_activity(std::string_view phase) {
  if (phase == "active")
    return "planning";
  if (phase == "running_campaign")
    return "awaiting_campaign_fact";
  if (phase == "idle")
    return "review";
  return "quiet";
}

[[nodiscard]] inline std::string
import_v5_pause_kind_as_work_gate(std::string_view v5_pause_kind) {
  if (v5_pause_kind == "operator")
    return "operator_pause";
  if (v5_pause_kind == "clarification")
    return "clarification";
  if (v5_pause_kind == "governance")
    return "governance";
  return "open";
}

[[nodiscard]] inline std::string
import_v5_phase_as_campaign_status(std::string_view phase,
                                   std::string_view v5_campaign_cursor) {
  if (phase == "running_campaign")
    return "running";
  if (!std::string(v5_campaign_cursor).empty() &&
      (phase == "paused" || phase == "finished")) {
    return "stopping";
  }
  return "none";
}

[[nodiscard]] inline bool
marshal_session_work_blocked(const marshal_session_record_t &record) {
  return record.lifecycle == "live" && record.work_gate != "open";
}

[[nodiscard]] inline bool
marshal_session_has_active_campaign(const marshal_session_record_t &record) {
  return record.campaign_status == "launching" ||
         record.campaign_status == "running" ||
         record.campaign_status == "stopping";
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
marshal_session_runner_stdout_path(const std::filesystem::path &marshal_root,
                                   std::string_view marshal_session_id) {
  return marshal_session_logs_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionRunnerStdoutFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_runner_stderr_path(const std::filesystem::path &marshal_root,
                                   std::string_view marshal_session_id) {
  return marshal_session_logs_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionRunnerStderrFilename);
}

[[nodiscard]] inline std::filesystem::path
marshal_session_compat_codex_session_log_path(
    const std::filesystem::path &marshal_root,
    std::string_view marshal_session_id) {
  return marshal_session_logs_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionCompatCodexSessionLogFilename);
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
marshal_session_turns_path(const std::filesystem::path &marshal_root,
                           std::string_view marshal_session_id) {
  return marshal_session_dir(marshal_root, marshal_session_id) /
         std::string(kMarshalSessionTurnsFilename);
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
  const std::string normalized_schema =
      (record.schema.empty() || record.schema == kMarshalSessionSchemaV4 ||
       record.schema == kMarshalSessionSchemaV5)
          ? std::string(kMarshalSessionSchemaV6)
          : record.schema;
  document.entries.push_back(
      make_runtime_lls_string_entry("schema", normalized_schema));
  document.entries.push_back(make_runtime_lls_string_entry(
      "marshal_session_id", record.marshal_session_id));
  document.entries.push_back(
      make_runtime_lls_string_entry("lifecycle", record.lifecycle));
  document.entries.push_back(make_runtime_lls_string_entry(
      "status_detail",
      sanitize_marshal_session_runtime_text(record.status_detail)));
  document.entries.push_back(
      make_runtime_lls_string_entry("work_gate", record.work_gate));
  document.entries.push_back(
      make_runtime_lls_string_entry("finish_reason", record.finish_reason));
  document.entries.push_back(
      make_runtime_lls_string_entry("activity", record.activity));
  document.entries.push_back(
      make_runtime_lls_string_entry("objective_name", record.objective_name));
  document.entries.push_back(make_runtime_lls_string_entry(
      "global_config_path", record.global_config_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("boot_id", record.boot_id));
  if (record.runner_pid.has_value()) {
    document.entries.push_back(make_runtime_lls_uint_entry(
        "runner_pid", *record.runner_pid, "(0,+inf)"));
  }
  if (record.runner_start_ticks.has_value()) {
    document.entries.push_back(make_runtime_lls_uint_entry(
        "runner_start_ticks", *record.runner_start_ticks, "(0,+inf)"));
  }
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
  document.entries.push_back(
      make_runtime_lls_string_entry("turns_path", record.turns_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "codex_stdout_path", record.codex_stdout_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "codex_stderr_path", record.codex_stderr_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "current_thread_id", record.current_thread_id));
  document.entries.push_back(make_runtime_lls_string_entry(
      "codex_continuity", record.codex_continuity));
  document.entries.push_back(make_runtime_lls_string_entry(
      "last_resume_error",
      sanitize_marshal_session_runtime_text(record.last_resume_error)));
  document.entries.push_back(make_runtime_lls_string_entry(
      "resolved_marshal_codex_binary", record.resolved_marshal_codex_binary));
  document.entries.push_back(make_runtime_lls_string_entry(
      "resolved_marshal_codex_model", record.resolved_marshal_codex_model));
  document.entries.push_back(make_runtime_lls_string_entry(
      "resolved_marshal_codex_reasoning_effort",
      record.resolved_marshal_codex_reasoning_effort));
  for (std::size_t i = 0; i < record.thread_lineage.size(); ++i) {
    document.entries.push_back(make_runtime_lls_string_entry(
        "thread_lineage." + format_marshal_index(i), record.thread_lineage[i]));
  }
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
  document.entries.push_back(
      make_runtime_lls_string_entry("campaign_status", record.campaign_status));
  document.entries.push_back(
      make_runtime_lls_string_entry("campaign_cursor", record.campaign_cursor));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "next_message_seq", record.next_message_seq, "[0,+inf)"));
  for (std::size_t i = 0; i < record.pending_operator_messages.size(); ++i) {
    const auto &message = record.pending_operator_messages[i];
    const std::string prefix =
        "pending_operator_message." + format_marshal_index(i) + ".";
    document.entries.push_back(make_runtime_lls_string_entry(
        prefix + "message_id", message.message_id));
    document.entries.push_back(
        make_runtime_lls_string_entry(prefix + "text", message.text));
    document.entries.push_back(make_runtime_lls_string_entry(
        prefix + "delivery_mode", message.delivery_mode));
    document.entries.push_back(make_runtime_lls_string_entry(
        prefix + "delivery_status", message.delivery_status));
    document.entries.push_back(make_runtime_lls_string_entry(
        prefix + "thread_id_at_delivery", message.thread_id_at_delivery));
    if (message.received_at_ms.has_value()) {
      document.entries.push_back(make_runtime_lls_uint_entry(
          prefix + "received_at_ms", *message.received_at_ms, "(0,+inf)"));
    }
    if (message.delivered_at_ms.has_value()) {
      document.entries.push_back(make_runtime_lls_uint_entry(
          prefix + "delivered_at_ms", *message.delivered_at_ms, "(0,+inf)"));
    }
    if (message.handled_at_ms.has_value()) {
      document.entries.push_back(make_runtime_lls_uint_entry(
          prefix + "handled_at_ms", *message.handled_at_ms, "(0,+inf)"));
    }
    document.entries.push_back(make_runtime_lls_uint_entry(
        prefix + "delivery_attempts", message.delivery_attempts, "[0,+inf)"));
    document.entries.push_back(make_runtime_lls_string_entry(
        prefix + "last_error",
        sanitize_marshal_session_runtime_text(message.last_error)));
  }
  document.entries.push_back(make_runtime_lls_uint_entry(
      "last_event_seq", record.last_event_seq, "[0,+inf)"));
  document.entries.push_back(make_runtime_lls_string_entry(
      "last_codex_action", record.last_codex_action));
  document.entries.push_back(make_runtime_lls_string_entry(
      "last_warning",
      sanitize_marshal_session_runtime_text(record.last_warning)));
  document.entries.push_back(make_runtime_lls_string_entry(
      "last_warning_code", record.last_warning_code));
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
  if (parsed.schema != kMarshalSessionSchemaV4 &&
      parsed.schema != kMarshalSessionSchemaV5 &&
      parsed.schema != kMarshalSessionSchemaV6) {
    if (error)
      *error = "unexpected marshal session schema: " + parsed.schema;
    return false;
  }
  parsed.marshal_session_id = kv["marshal_session_id"];
  if (parsed.schema == kMarshalSessionSchemaV6) {
    parsed.lifecycle = normalize_marshal_lifecycle(kv["lifecycle"]);
    parsed.status_detail = kv["status_detail"];
    parsed.work_gate = normalize_marshal_work_gate(kv["work_gate"]);
    parsed.finish_reason = kv["finish_reason"];
    parsed.activity = normalize_marshal_activity(kv["activity"]);
  } else {
    parsed.lifecycle = import_v5_phase_as_lifecycle(kv["phase"]);
    parsed.status_detail = kv["phase_detail"];
    parsed.work_gate = import_v5_pause_kind_as_work_gate(kv["pause_kind"]);
    parsed.finish_reason = kv["finish_reason"];
    parsed.activity = import_v5_phase_as_activity(kv["phase"]);
  }
  parsed.objective_name = kv["objective_name"];
  parsed.global_config_path = kv["global_config_path"];
  parsed.boot_id = kv["boot_id"];
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
  parsed.turns_path = kv["turns_path"];
  parsed.codex_stdout_path = kv["codex_stdout_path"];
  parsed.codex_stderr_path = kv["codex_stderr_path"];
  if (parsed.schema == kMarshalSessionSchemaV6) {
    parsed.current_thread_id = kv["current_thread_id"];
    parsed.codex_continuity =
        normalize_marshal_codex_continuity(kv["codex_continuity"]);
    parsed.last_resume_error = kv["last_resume_error"];
  } else {
    parsed.current_thread_id = kv["codex_session_id"];
    parsed.codex_continuity =
        cuwacunu::hero::runtime::trim_ascii(parsed.current_thread_id).empty()
            ? "restarted"
            : "attached";
  }
  parsed.resolved_marshal_codex_binary = kv["resolved_marshal_codex_binary"];
  parsed.resolved_marshal_codex_model = kv["resolved_marshal_codex_model"];
  parsed.resolved_marshal_codex_reasoning_effort =
      kv["resolved_marshal_codex_reasoning_effort"];
  if (parsed.schema == kMarshalSessionSchemaV6) {
    parsed.campaign_status =
        normalize_marshal_campaign_status(kv["campaign_status"]);
    parsed.campaign_cursor = kv["campaign_cursor"];
    parsed.last_codex_action = kv["last_codex_action"];
    parsed.last_warning_code = kv["last_warning_code"];
    (void)cuwacunu::hero::runtime::parse_u64(kv["next_message_seq"],
                                             &parsed.next_message_seq);
  } else {
    parsed.campaign_cursor = kv["active_campaign_cursor"];
    parsed.campaign_status =
        import_v5_phase_as_campaign_status(kv["phase"], parsed.campaign_cursor);
    parsed.last_codex_action = kv["last_intent_kind"];
  }
  parsed.last_warning = kv["last_warning"];
  parsed.last_input_checkpoint_path = kv["last_input_checkpoint_path"];
  parsed.last_intent_checkpoint_path = kv["last_intent_checkpoint_path"];
  parsed.last_mutation_checkpoint_path = kv["last_mutation_checkpoint_path"];
  parsed.authority_scope = kv["authority_scope"];
  if (parsed.turns_path.empty() && !parsed.session_root.empty()) {
    parsed.turns_path = (std::filesystem::path(parsed.session_root) /
                         std::string(kMarshalSessionTurnsFilename))
                            .string();
  }
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
  if (cuwacunu::hero::runtime::parse_u64(kv["runner_pid"], &value)) {
    parsed.runner_pid = value;
  }
  if (cuwacunu::hero::runtime::parse_u64(kv["runner_start_ticks"], &value)) {
    parsed.runner_start_ticks = value;
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
  (void)cuwacunu::hero::runtime::parse_u64(kv["last_event_seq"],
                                           &parsed.last_event_seq);
  if (parsed.marshal_session_id.empty()) {
    if (error)
      *error = "marshal session record missing marshal_session_id";
    return false;
  }
  if (parsed.lifecycle.empty()) {
    if (error)
      *error = "marshal session record missing lifecycle";
    return false;
  }
  if (parsed.work_gate.empty()) {
    if (error)
      *error = "marshal session record missing work_gate";
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
  std::map<std::string, marshal_session_record_t::operator_message_t,
           std::less<>>
      pending_messages{};
  for (const auto &[key, entry] : ordered) {
    if (key.rfind("thread_lineage.", 0) == 0) {
      parsed.thread_lineage.push_back(entry);
      continue;
    }
    if (key.rfind("pending_operator_message.", 0) == 0) {
      const std::size_t prefix_size =
          std::string("pending_operator_message.").size();
      const std::size_t dot_pos = key.find('.', prefix_size);
      if (dot_pos == std::string::npos) {
        marshal_session_record_t::operator_message_t legacy{};
        legacy.message_id = "legacy." + key.substr(prefix_size);
        legacy.text = entry;
        canonicalize_marshal_operator_message(&legacy);
        pending_messages[legacy.message_id] = legacy;
        continue;
      }
      const std::string index_key =
          key.substr(prefix_size, dot_pos - prefix_size);
      const std::string field = key.substr(dot_pos + 1);
      auto &message = pending_messages[index_key];
      if (field == "message_id") {
        message.message_id = entry;
      } else if (field == "text") {
        message.text = entry;
      } else if (field == "delivery_mode") {
        message.delivery_mode = entry;
      } else if (field == "delivery_status") {
        message.delivery_status = entry;
      } else if (field == "thread_id_at_delivery") {
        message.thread_id_at_delivery = entry;
      } else if (field == "last_error") {
        message.last_error = entry;
      } else if (field == "received_at_ms") {
        if (cuwacunu::hero::runtime::parse_u64(entry, &value))
          message.received_at_ms = value;
      } else if (field == "delivered_at_ms") {
        if (cuwacunu::hero::runtime::parse_u64(entry, &value))
          message.delivered_at_ms = value;
      } else if (field == "handled_at_ms") {
        if (cuwacunu::hero::runtime::parse_u64(entry, &value))
          message.handled_at_ms = value;
      } else if (field == "delivery_attempts") {
        (void)cuwacunu::hero::runtime::parse_u64(entry,
                                                 &message.delivery_attempts);
      }
      continue;
    }
    if (key.rfind("campaign_cursor.", 0) == 0) {
      parsed.campaign_cursors.push_back(entry);
    }
  }
  if (parsed.thread_lineage.empty() &&
      !cuwacunu::hero::runtime::trim_ascii(parsed.current_thread_id).empty()) {
    parsed.thread_lineage.push_back(parsed.current_thread_id);
  }
  for (auto &[index_key, message] : pending_messages) {
    if (message.message_id.empty())
      message.message_id = "legacy." + index_key;
    canonicalize_marshal_operator_message(&message);
    if (!cuwacunu::hero::runtime::trim_ascii(message.text).empty()) {
      parsed.pending_operator_messages.push_back(std::move(message));
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
  return record.lifecycle == "terminal";
}

[[nodiscard]] inline bool
is_marshal_session_terminal_state(std::string_view lifecycle) {
  return lifecycle == "terminal" || lifecycle == "finished";
}

[[nodiscard]] inline bool
marshal_session_is_review_ready(const marshal_session_record_t &record) {
  return record.lifecycle == "live" && record.activity == "review";
}

[[nodiscard]] inline bool
is_marshal_session_summary_state(const marshal_session_record_t &record) {
  return record.lifecycle == "terminal" ||
         marshal_session_is_review_ready(record);
}

[[nodiscard]] inline bool
is_marshal_session_summary_state(std::string_view lifecycle,
                                 std::string_view activity) {
  return lifecycle == "terminal" ||
         (lifecycle == "live" && activity == "review");
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
    if (session.lifecycle == "live" && (session.work_gate == "clarification" ||
                                        session.work_gate == "governance")) {
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
