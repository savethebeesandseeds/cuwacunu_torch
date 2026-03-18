// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <chrono>
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
namespace runtime {

inline constexpr std::string_view kRuntimeCampaignSchemaV2 =
    "hero.runtime.campaign.v2";
inline constexpr std::string_view kRuntimeCampaignManifestFilename =
    "campaign.lls";
inline constexpr std::string_view kRuntimeCampaignDslFilename = "campaign.dsl";
inline constexpr std::string_view kRuntimeCampaignStdoutFilename = "stdout.log";
inline constexpr std::string_view kRuntimeCampaignStderrFilename = "stderr.log";

struct runtime_campaign_record_t {
  std::string schema{std::string(kRuntimeCampaignSchemaV2)};
  std::string campaign_cursor{};
  std::string boot_id{};
  std::string state{"launching"};
  std::string state_detail{};
  std::string global_config_path{};
  std::string source_campaign_dsl_path{};
  std::string campaign_dsl_path{};
  bool reset_runtime_state{false};
  std::string stdout_path{};
  std::string stderr_path{};
  std::uint64_t started_at_ms{0};
  std::uint64_t updated_at_ms{0};
  std::optional<std::uint64_t> finished_at_ms{};
  std::optional<std::uint64_t> runner_pid{};
  std::optional<std::uint64_t> runner_start_ticks{};
  std::optional<std::uint64_t> current_run_index{};
  std::vector<std::string> run_bind_ids{};
  std::vector<std::string> job_cursors{};
};

[[nodiscard]] inline std::string format_campaign_index(std::size_t index) {
  std::ostringstream out;
  out << std::setw(4) << std::setfill('0') << index;
  return out.str();
}

[[nodiscard]] inline std::filesystem::path runtime_campaign_dir(
    const std::filesystem::path& campaigns_root,
    std::string_view campaign_cursor) {
  return campaigns_root / std::string(campaign_cursor);
}

[[nodiscard]] inline std::filesystem::path runtime_campaign_manifest_path(
    const std::filesystem::path& campaigns_root,
    std::string_view campaign_cursor) {
  return runtime_campaign_dir(campaigns_root, campaign_cursor) /
         std::string(kRuntimeCampaignManifestFilename);
}

[[nodiscard]] inline std::filesystem::path runtime_campaign_dsl_path(
    const std::filesystem::path& campaigns_root,
    std::string_view campaign_cursor) {
  return runtime_campaign_dir(campaigns_root, campaign_cursor) /
         std::string(kRuntimeCampaignDslFilename);
}

[[nodiscard]] inline std::filesystem::path runtime_campaign_jobs_dir(
    const std::filesystem::path& campaigns_root,
    std::string_view campaign_cursor) {
  return runtime_campaign_dir(campaigns_root, campaign_cursor) /
         std::string(kRuntimeCampaignJobsDirname);
}

[[nodiscard]] inline std::filesystem::path runtime_campaign_stdout_path(
    const std::filesystem::path& campaigns_root,
    std::string_view campaign_cursor) {
  return runtime_campaign_dir(campaigns_root, campaign_cursor) /
         std::string(kRuntimeCampaignStdoutFilename);
}

[[nodiscard]] inline std::filesystem::path runtime_campaign_stderr_path(
    const std::filesystem::path& campaigns_root,
    std::string_view campaign_cursor) {
  return runtime_campaign_dir(campaigns_root, campaign_cursor) /
         std::string(kRuntimeCampaignStderrFilename);
}

[[nodiscard]] inline std::string make_campaign_cursor(
    const std::filesystem::path& campaigns_root) {
  const std::uint64_t started_at_ms = now_ms_utc();
  for (std::uint64_t nonce = 1; nonce != 0; ++nonce) {
    const std::uint64_t salt =
        static_cast<std::uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()) ^
        (nonce * 0x9e3779b97f4a7c15ULL);
    const std::string candidate =
        "campaign." + std::to_string(started_at_ms) + "." +
        hex_lower_u64(salt).substr(0, 8);
    std::error_code ec{};
    if (!std::filesystem::exists(runtime_campaign_dir(campaigns_root, candidate),
                                 ec) ||
        ec) {
      return candidate;
    }
  }
  return "campaign." + std::to_string(started_at_ms) + ".overflow";
}

[[nodiscard]] inline cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t
runtime_campaign_record_to_document(const runtime_campaign_record_t& record) {
  using namespace cuwacunu::piaabo::latent_lineage_state;
  runtime_lls_document_t document{};
  document.entries.push_back(make_runtime_lls_string_entry("schema", record.schema));
  document.entries.push_back(
      make_runtime_lls_string_entry("campaign_cursor", record.campaign_cursor));
  document.entries.push_back(make_runtime_lls_string_entry("boot_id", record.boot_id));
  document.entries.push_back(make_runtime_lls_string_entry("state", record.state));
  document.entries.push_back(
      make_runtime_lls_string_entry("state_detail", record.state_detail));
  document.entries.push_back(
      make_runtime_lls_string_entry("global_config_path",
                                    record.global_config_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "source_campaign_dsl_path", record.source_campaign_dsl_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "campaign_dsl_path", record.campaign_dsl_path));
  document.entries.push_back(make_runtime_lls_bool_entry(
      "reset_runtime_state", record.reset_runtime_state));
  document.entries.push_back(
      make_runtime_lls_string_entry("stdout_path", record.stdout_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("stderr_path", record.stderr_path));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "started_at_ms", record.started_at_ms, "(0,+inf)"));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "updated_at_ms", record.updated_at_ms, "(0,+inf)"));
  if (record.finished_at_ms.has_value()) {
    document.entries.push_back(make_runtime_lls_uint_entry(
        "finished_at_ms", *record.finished_at_ms, "(0,+inf)"));
  }
  if (record.runner_pid.has_value()) {
    document.entries.push_back(make_runtime_lls_uint_entry(
        "runner_pid", *record.runner_pid, "(0,+inf)"));
  }
  if (record.runner_start_ticks.has_value()) {
    document.entries.push_back(make_runtime_lls_uint_entry(
        "runner_start_ticks", *record.runner_start_ticks, "(0,+inf)"));
  }
  if (record.current_run_index.has_value()) {
    document.entries.push_back(make_runtime_lls_uint_entry(
        "current_run_index", *record.current_run_index, "(0,+inf)"));
  }
  for (std::size_t i = 0; i < record.run_bind_ids.size(); ++i) {
    document.entries.push_back(make_runtime_lls_string_entry(
        "run_bind_id." + format_campaign_index(i), record.run_bind_ids[i]));
  }
  for (std::size_t i = 0; i < record.job_cursors.size(); ++i) {
    document.entries.push_back(make_runtime_lls_string_entry(
        "job_cursor." + format_campaign_index(i), record.job_cursors[i]));
  }
  return document;
}

[[nodiscard]] inline bool parse_runtime_campaign_record_document(
    const cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t& document,
    runtime_campaign_record_t* out,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for runtime campaign record";
    return false;
  }

  std::unordered_map<std::string, std::string> kv{};
  if (!cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          document, &kv, error)) {
    return false;
  }

  runtime_campaign_record_t parsed{};
  parsed.schema = kv["schema"];
  if (parsed.schema != kRuntimeCampaignSchemaV2) {
    if (error) *error = "unexpected runtime campaign schema: " + parsed.schema;
    return false;
  }
  parsed.campaign_cursor = kv["campaign_cursor"];
  parsed.boot_id = kv["boot_id"];
  parsed.state = kv["state"];
  parsed.state_detail = kv["state_detail"];
  parsed.global_config_path = kv["global_config_path"];
  parsed.source_campaign_dsl_path = kv["source_campaign_dsl_path"];
  parsed.campaign_dsl_path = kv["campaign_dsl_path"];
  (void)parse_bool(kv["reset_runtime_state"], &parsed.reset_runtime_state);
  parsed.stdout_path = kv["stdout_path"];
  parsed.stderr_path = kv["stderr_path"];
  if (!parse_u64(kv["started_at_ms"], &parsed.started_at_ms)) {
    if (error) *error = "runtime campaign record missing started_at_ms";
    return false;
  }
  if (!parse_u64(kv["updated_at_ms"], &parsed.updated_at_ms)) {
    if (error) *error = "runtime campaign record missing updated_at_ms";
    return false;
  }
  std::uint64_t value = 0;
  if (parse_u64(kv["finished_at_ms"], &value)) parsed.finished_at_ms = value;
  if (parse_u64(kv["runner_pid"], &value)) parsed.runner_pid = value;
  if (parse_u64(kv["runner_start_ticks"], &value)) {
    parsed.runner_start_ticks = value;
  }
  if (parse_u64(kv["current_run_index"], &value)) {
    parsed.current_run_index = value;
  }

  if (parsed.campaign_cursor.empty()) {
    if (error) *error = "runtime campaign record missing campaign_cursor";
    return false;
  }
  if (parsed.state.empty()) {
    if (error) *error = "runtime campaign record missing state";
    return false;
  }
  if (parsed.global_config_path.empty()) {
    if (error) *error = "runtime campaign record missing global_config_path";
    return false;
  }

  std::map<std::string, std::string, std::less<>> ordered(kv.begin(), kv.end());
  for (const auto& [key, entry] : ordered) {
    if (key.rfind("run_bind_id.", 0) == 0) parsed.run_bind_ids.push_back(entry);
    if (key.rfind("job_cursor.", 0) == 0) parsed.job_cursors.push_back(entry);
  }
  *out = std::move(parsed);
  return true;
}

[[nodiscard]] inline bool write_runtime_campaign_record(
    const std::filesystem::path& campaigns_root,
    const runtime_campaign_record_t& record,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (record.campaign_cursor.empty()) {
    if (error) *error = "runtime campaign record missing campaign_cursor";
    return false;
  }
  const auto manifest_path =
      runtime_campaign_manifest_path(campaigns_root, record.campaign_cursor);
  const std::string text =
      cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
          runtime_campaign_record_to_document(record));
  return write_text_file_atomic(manifest_path, text, error);
}

[[nodiscard]] inline bool read_runtime_campaign_record(
    const std::filesystem::path& campaigns_root,
    std::string_view campaign_cursor,
    runtime_campaign_record_t* out,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for runtime campaign record";
    return false;
  }
  std::string text{};
  if (!read_text_file(runtime_campaign_manifest_path(campaigns_root, campaign_cursor),
                      &text, error)) {
    return false;
  }
  cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t document{};
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          text, &document, error)) {
    return false;
  }
  return parse_runtime_campaign_record_document(document, out, error);
}

[[nodiscard]] inline bool is_legacy_runtime_campaign_schema_error(
    std::string_view error) {
  constexpr std::string_view kPrefix = "unexpected runtime campaign schema: ";
  if (error.rfind(kPrefix, 0) != 0) return false;
  const std::string_view schema = error.substr(kPrefix.size());
  return !schema.empty() && schema != kRuntimeCampaignSchemaV2 &&
         schema.rfind("hero.runtime.campaign.v", 0) == 0;
}

[[nodiscard]] inline bool scan_runtime_campaign_records(
    const std::filesystem::path& campaigns_root,
    std::vector<runtime_campaign_record_t>* out,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for runtime campaign list";
    return false;
  }
  out->clear();

  std::error_code ec{};
  if (!std::filesystem::exists(campaigns_root, ec)) return true;
  if (ec) {
    if (error) *error = "cannot inspect campaigns_root: " + campaigns_root.string();
    return false;
  }

  for (const auto& it : std::filesystem::directory_iterator(campaigns_root, ec)) {
    if (ec) {
      if (error) *error = "cannot iterate campaigns_root: " + campaigns_root.string();
      return false;
    }
    if (!it.is_directory(ec) || ec) continue;
    runtime_campaign_record_t record{};
    std::string record_error{};
    if (!read_runtime_campaign_record(campaigns_root,
                                      it.path().filename().string(),
                                      &record, &record_error)) {
      if (is_legacy_runtime_campaign_schema_error(record_error)) continue;
      if (error) {
        *error = "failed reading runtime campaign manifest " +
                 runtime_campaign_manifest_path(campaigns_root,
                                                it.path().filename().string())
                     .string() +
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
    return lhs.campaign_cursor < rhs.campaign_cursor;
  });
  return true;
}

}  // namespace runtime
}  // namespace hero
}  // namespace cuwacunu
