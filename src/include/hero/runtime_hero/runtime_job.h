// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

#include <unistd.h>

#include "piaabo/latent_lineage_state/runtime_lls.h"

namespace cuwacunu {
namespace hero {
namespace runtime {

inline constexpr std::string_view kRuntimeJobSchemaV3 = "hero.runtime.job.v3";
inline constexpr std::string_view kRuntimeJobManifestFilename = "job.lls";
inline constexpr std::string_view kRuntimeJobCampaignDslFilename = "campaign.dsl";
inline constexpr std::string_view kRuntimeJobContractDslFilename =
    "binding.contract.dsl";
inline constexpr std::string_view kRuntimeJobWaveDslFilename = "binding.wave.dsl";
inline constexpr std::string_view kRuntimeJobInstructionsDirname = "instructions";
inline constexpr std::string_view kRuntimeJobStdoutFilename = "stdout.log";
inline constexpr std::string_view kRuntimeJobStderrFilename = "stderr.log";
inline constexpr std::string_view kRuntimeJobTraceFilename = "job.trace.jsonl";
inline constexpr std::string_view kRuntimeCampaignJobsDirname = "jobs";
inline constexpr std::string_view kRuntimeLegacyJobsDirname = ".jobs";

struct runtime_job_record_t {
  std::string schema{std::string(kRuntimeJobSchemaV3)};
  std::string job_cursor{};
  std::string job_kind{"campaign_run"};
  std::string boot_id{};
  std::string state{"launching"};
  std::string state_detail{};
  std::string worker_binary{};
  std::string worker_command{};
  std::string global_config_path{};
  std::string source_campaign_dsl_path{};
  std::string source_contract_dsl_path{};
  std::string source_wave_dsl_path{};
  std::string campaign_dsl_path{};
  std::string contract_dsl_path{};
  std::string wave_dsl_path{};
  std::string binding_id{};
  bool reset_runtime_state{false};
  std::string stdout_path{};
  std::string stderr_path{};
  std::string trace_path{};
  std::uint64_t started_at_ms{0};
  std::uint64_t updated_at_ms{0};
  std::optional<std::uint64_t> finished_at_ms{};
  std::optional<std::uint64_t> runner_pid{};
  std::optional<std::uint64_t> runner_start_ticks{};
  std::optional<std::uint64_t> target_pid{};
  std::optional<std::uint64_t> target_pgid{};
  std::optional<std::uint64_t> target_start_ticks{};
  std::optional<std::int64_t> exit_code{};
  std::optional<std::int64_t> term_signal{};
};

struct runtime_job_observation_t {
  bool runner_alive{false};
  bool target_alive{false};
};

[[nodiscard]] inline std::string trim_ascii(std::string_view in) {
  std::size_t b = 0;
  while (b < in.size() &&
         std::isspace(static_cast<unsigned char>(in[b])) != 0) {
    ++b;
  }
  std::size_t e = in.size();
  while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1])) != 0) {
    --e;
  }
  return std::string(in.substr(b, e - b));
}

[[nodiscard]] inline std::uint64_t now_ms_utc() {
  const auto now = std::chrono::system_clock::now();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now.time_since_epoch())
          .count());
}

[[nodiscard]] inline std::string current_boot_id() {
  std::ifstream in("/proc/sys/kernel/random/boot_id");
  if (!in) return {};
  std::string boot_id;
  std::getline(in, boot_id);
  return trim_ascii(boot_id);
}

[[nodiscard]] inline std::string hex_lower_u64(std::uint64_t value) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string out(16, '0');
  for (std::size_t i = 0; i < out.size(); ++i) {
    const unsigned shift =
        static_cast<unsigned>((out.size() - 1 - i) * 4u);
    out[i] = kHex[(value >> shift) & 0x0Fu];
  }
  return out;
}

[[nodiscard]] inline std::filesystem::path runtime_job_dir(
    const std::filesystem::path& campaigns_root, std::string_view job_cursor) {
  const std::string trimmed = trim_ascii(job_cursor);
  const std::size_t marker = trimmed.find(".job.");
  if (marker != std::string::npos && marker != 0) {
    const std::string campaign_cursor = trimmed.substr(0, marker);
    return campaigns_root / campaign_cursor /
           std::string(kRuntimeCampaignJobsDirname) / trimmed;
  }
  return campaigns_root / std::string(kRuntimeLegacyJobsDirname) / trimmed;
}

[[nodiscard]] inline std::filesystem::path runtime_job_manifest_path(
    const std::filesystem::path& campaigns_root, std::string_view job_cursor) {
  return runtime_job_dir(campaigns_root, job_cursor) /
         std::string(kRuntimeJobManifestFilename);
}

[[nodiscard]] inline std::filesystem::path runtime_job_stdout_path(
    const std::filesystem::path& campaigns_root, std::string_view job_cursor) {
  return runtime_job_dir(campaigns_root, job_cursor) /
         std::string(kRuntimeJobStdoutFilename);
}

[[nodiscard]] inline std::filesystem::path runtime_job_trace_path(
    const std::filesystem::path& campaigns_root, std::string_view job_cursor) {
  return runtime_job_dir(campaigns_root, job_cursor) /
         std::string(kRuntimeJobTraceFilename);
}

[[nodiscard]] inline std::filesystem::path runtime_job_campaign_dsl_path(
    const std::filesystem::path& campaigns_root, std::string_view job_cursor) {
  return runtime_job_dir(campaigns_root, job_cursor) /
         std::string(kRuntimeJobInstructionsDirname) /
         std::string(kRuntimeJobCampaignDslFilename);
}

[[nodiscard]] inline std::filesystem::path runtime_job_contract_dsl_path(
    const std::filesystem::path& campaigns_root, std::string_view job_cursor) {
  return runtime_job_dir(campaigns_root, job_cursor) /
         std::string(kRuntimeJobInstructionsDirname) /
         std::string(kRuntimeJobContractDslFilename);
}

[[nodiscard]] inline std::filesystem::path runtime_job_wave_dsl_path(
    const std::filesystem::path& campaigns_root, std::string_view job_cursor) {
  return runtime_job_dir(campaigns_root, job_cursor) /
         std::string(kRuntimeJobInstructionsDirname) /
         std::string(kRuntimeJobWaveDslFilename);
}

[[nodiscard]] inline std::filesystem::path runtime_job_stderr_path(
    const std::filesystem::path& campaigns_root, std::string_view job_cursor) {
  return runtime_job_dir(campaigns_root, job_cursor) /
         std::string(kRuntimeJobStderrFilename);
}

[[nodiscard]] inline bool write_text_file_atomic(
    const std::filesystem::path& path, std::string_view content,
    std::string* error = nullptr) {
  if (error) error->clear();
  std::error_code ec{};
  const auto parent = path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      if (error) *error = "cannot create parent directory: " + parent.string();
      return false;
    }
  }

  const auto stamp =
      std::chrono::steady_clock::now().time_since_epoch().count();
  const auto tmp =
      parent / (path.filename().string() + ".tmp." +
                std::to_string(static_cast<long long>(stamp)));
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) {
      if (error) *error = "cannot open temp file: " + tmp.string();
      return false;
    }
    out << content;
    out.flush();
    if (!out.good()) {
      if (error) *error = "cannot write temp file: " + tmp.string();
      return false;
    }
  }

  std::filesystem::rename(tmp, path, ec);
  if (ec) {
    std::error_code rm_ec{};
    std::filesystem::remove(tmp, rm_ec);
    if (error) *error = "cannot replace target file: " + path.string();
    return false;
  }
  return true;
}

[[nodiscard]] inline bool read_text_file(const std::filesystem::path& path,
                                         std::string* out,
                                         std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "output pointer is null";
    return false;
  }
  out->clear();

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error) *error = "cannot open file: " + path.string();
    return false;
  }
  out->assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  if (!in.eof() && in.fail()) {
    if (error) *error = "cannot read file: " + path.string();
    return false;
  }
  return true;
}

[[nodiscard]] inline bool append_text_file(const std::filesystem::path& path,
                                          std::string_view content,
                                          std::string* error = nullptr) {
  if (error) error->clear();
  const auto parent = path.parent_path();
  std::error_code ec{};
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      if (error) *error = "cannot create parent directory: " + parent.string();
      return false;
    }
  }
  std::ofstream out(path, std::ios::binary | std::ios::app);
  if (!out) {
    if (error) *error = "cannot open file for append: " + path.string();
    return false;
  }
  out << content;
  out.flush();
  if (!out.good()) {
    if (error) *error = "cannot append file: " + path.string();
    return false;
  }
  return true;
}

[[nodiscard]] inline std::string shell_join_command(
    const std::vector<std::string>& argv) {
  std::ostringstream out;
  for (std::size_t i = 0; i < argv.size(); ++i) {
    if (i != 0) out << " ";
    const std::string& arg = argv[i];
    const bool safe =
        !arg.empty() &&
        arg.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789._/:=-")
            == std::string::npos;
    if (safe) {
      out << arg;
      continue;
    }
    out << "'";
    for (const char c : arg) {
      if (c == '\'') {
        out << "'\\''";
      } else {
        out << c;
      }
    }
    out << "'";
  }
  return out.str();
}

[[nodiscard]] inline std::string make_job_cursor(
    const std::filesystem::path& campaigns_root, std::string_view campaign_cursor,
    std::size_t job_index) {
  const std::string parent = trim_ascii(campaign_cursor);
  auto format_index = [](std::size_t value) {
    std::ostringstream out;
    out << std::setw(4) << std::setfill('0') << value;
    return out.str();
  };
  if (!parent.empty()) {
    const std::string base =
        parent + ".job." + format_index(job_index);
    std::error_code ec{};
    if (!std::filesystem::exists(runtime_job_dir(campaigns_root, base), ec) ||
        ec) {
      return base;
    }
    for (std::size_t retry = 1; retry != 0; ++retry) {
      const std::string candidate =
          base + ".retry." + format_index(retry);
      std::error_code retry_ec{};
      if (!std::filesystem::exists(runtime_job_dir(campaigns_root, candidate),
                                   retry_ec) ||
          retry_ec) {
        return candidate;
      }
    }
  }
  const std::uint64_t started_at_ms = now_ms_utc();
  for (std::uint64_t nonce = 1; nonce != 0; ++nonce) {
    const std::uint64_t salt =
        static_cast<std::uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()) ^
        (nonce * 0x9e3779b97f4a7c15ULL);
    const std::string candidate =
        "job." + std::to_string(started_at_ms) + "." +
        hex_lower_u64(salt).substr(0, 8);
    std::error_code ec{};
    if (!std::filesystem::exists(runtime_job_dir(campaigns_root, candidate),
                                 ec) ||
        ec) {
      return candidate;
    }
  }
  return "job." + std::to_string(started_at_ms) + ".overflow";
}

[[nodiscard]] inline bool parse_u64(std::string_view text, std::uint64_t* out) {
  if (!out) return false;
  const std::string token = trim_ascii(text);
  if (token.empty()) return false;
  std::uint64_t value = 0;
  const auto [ptr, ec] =
      std::from_chars(token.data(), token.data() + token.size(), value, 10);
  if (ec != std::errc{} || ptr != token.data() + token.size()) return false;
  *out = value;
  return true;
}

[[nodiscard]] inline bool parse_i64(std::string_view text, std::int64_t* out) {
  if (!out) return false;
  const std::string token = trim_ascii(text);
  if (token.empty()) return false;
  std::int64_t value = 0;
  const auto [ptr, ec] =
      std::from_chars(token.data(), token.data() + token.size(), value, 10);
  if (ec != std::errc{} || ptr != token.data() + token.size()) return false;
  *out = value;
  return true;
}

[[nodiscard]] inline bool parse_bool(std::string_view text, bool* out) {
  if (!out) return false;
  std::string token = trim_ascii(text);
  std::transform(token.begin(), token.end(), token.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  if (token == "true" || token == "1" || token == "yes" || token == "on") {
    *out = true;
    return true;
  }
  if (token == "false" || token == "0" || token == "no" || token == "off") {
    *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] inline cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t
runtime_job_record_to_document(const runtime_job_record_t& record) {
  using namespace cuwacunu::piaabo::latent_lineage_state;
  runtime_lls_document_t document{};
  document.entries.push_back(
      make_runtime_lls_string_entry("schema", record.schema));
  document.entries.push_back(
      make_runtime_lls_string_entry("job_cursor", record.job_cursor));
  document.entries.push_back(
      make_runtime_lls_string_entry("job_kind", record.job_kind));
  document.entries.push_back(
      make_runtime_lls_string_entry("boot_id", record.boot_id));
  document.entries.push_back(make_runtime_lls_string_entry("state", record.state));
  document.entries.push_back(
      make_runtime_lls_string_entry("state_detail", record.state_detail));
  document.entries.push_back(
      make_runtime_lls_string_entry("worker_binary", record.worker_binary));
  document.entries.push_back(
      make_runtime_lls_string_entry("worker_command", record.worker_command));
  document.entries.push_back(
      make_runtime_lls_string_entry("global_config_path",
                                    record.global_config_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "source_campaign_dsl_path", record.source_campaign_dsl_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "source_contract_dsl_path", record.source_contract_dsl_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "source_wave_dsl_path", record.source_wave_dsl_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("campaign_dsl_path", record.campaign_dsl_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("contract_dsl_path", record.contract_dsl_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("wave_dsl_path", record.wave_dsl_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("binding_id", record.binding_id));
  document.entries.push_back(make_runtime_lls_bool_entry(
      "reset_runtime_state", record.reset_runtime_state));
  document.entries.push_back(
      make_runtime_lls_string_entry("stdout_path", record.stdout_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("stderr_path", record.stderr_path));
  document.entries.push_back(
      make_runtime_lls_string_entry("trace_path", record.trace_path));
  document.entries.push_back(
      make_runtime_lls_uint_entry("started_at_ms", record.started_at_ms,
                                  "(0,+inf)"));
  document.entries.push_back(
      make_runtime_lls_uint_entry("updated_at_ms", record.updated_at_ms,
                                  "(0,+inf)"));
  if (record.finished_at_ms.has_value()) {
    document.entries.push_back(make_runtime_lls_uint_entry(
        "finished_at_ms", *record.finished_at_ms, "(0,+inf)"));
  }
  if (record.runner_pid.has_value()) {
    document.entries.push_back(
        make_runtime_lls_uint_entry("runner_pid", *record.runner_pid,
                                    "(0,+inf)"));
  }
  if (record.runner_start_ticks.has_value()) {
    document.entries.push_back(make_runtime_lls_uint_entry(
        "runner_start_ticks", *record.runner_start_ticks, "(0,+inf)"));
  }
  if (record.target_pid.has_value()) {
    document.entries.push_back(
        make_runtime_lls_uint_entry("target_pid", *record.target_pid,
                                    "(0,+inf)"));
  }
  if (record.target_pgid.has_value()) {
    document.entries.push_back(
        make_runtime_lls_uint_entry("target_pgid", *record.target_pgid,
                                    "(0,+inf)"));
  }
  if (record.target_start_ticks.has_value()) {
    document.entries.push_back(make_runtime_lls_uint_entry(
        "target_start_ticks", *record.target_start_ticks, "(0,+inf)"));
  }
  if (record.exit_code.has_value()) {
    document.entries.push_back(
        make_runtime_lls_int_entry("exit_code", *record.exit_code,
                                   "(-inf,+inf)"));
  }
  if (record.term_signal.has_value()) {
    document.entries.push_back(
        make_runtime_lls_int_entry("term_signal", *record.term_signal,
                                   "(0,+inf)"));
  }
  return document;
}

[[nodiscard]] inline bool parse_runtime_job_record(
    const cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t& document,
    runtime_job_record_t* out, std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "job record output pointer is null";
    return false;
  }
  std::unordered_map<std::string, std::string> kv{};
  if (!cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          document, &kv, error)) {
    return false;
  }

  runtime_job_record_t parsed{};
  parsed.schema = kv["schema"];
  if (parsed.schema != kRuntimeJobSchemaV3) {
    if (error) *error = "unexpected runtime job schema: " + parsed.schema;
    return false;
  }
  parsed.job_cursor = kv["job_cursor"];
  parsed.job_kind = kv["job_kind"];
  parsed.boot_id = kv["boot_id"];
  parsed.state = kv["state"];
  parsed.state_detail = kv["state_detail"];
  parsed.worker_binary = kv["worker_binary"];
  parsed.worker_command = kv["worker_command"];
  parsed.global_config_path = kv["global_config_path"];
  parsed.source_campaign_dsl_path = kv["source_campaign_dsl_path"];
  parsed.source_contract_dsl_path = kv["source_contract_dsl_path"];
  parsed.source_wave_dsl_path = kv["source_wave_dsl_path"];
  parsed.campaign_dsl_path = kv["campaign_dsl_path"];
  parsed.contract_dsl_path = kv["contract_dsl_path"];
  parsed.wave_dsl_path = kv["wave_dsl_path"];
  parsed.binding_id = kv["binding_id"];
  parsed.stdout_path = kv["stdout_path"];
  parsed.stderr_path = kv["stderr_path"];
  parsed.trace_path = kv["trace_path"];
  if (parsed.job_cursor.empty()) {
    if (error) *error = "runtime job record missing job_cursor";
    return false;
  }
  if (parsed.global_config_path.empty()) {
    if (error) *error = "runtime job record missing global_config_path";
    return false;
  }
  if (!parse_u64(kv["started_at_ms"], &parsed.started_at_ms)) {
    if (error) *error = "runtime job record missing/invalid started_at_ms";
    return false;
  }
  if (!parse_u64(kv["updated_at_ms"], &parsed.updated_at_ms)) {
    if (error) *error = "runtime job record missing/invalid updated_at_ms";
    return false;
  }
  bool reset = false;
  if (!kv["reset_runtime_state"].empty() &&
      !parse_bool(kv["reset_runtime_state"], &reset)) {
    if (error) *error = "runtime job record has invalid reset_runtime_state";
    return false;
  }
  parsed.reset_runtime_state = reset;

  std::uint64_t u64 = 0;
  std::int64_t i64 = 0;
  if (!kv["finished_at_ms"].empty()) {
    if (!parse_u64(kv["finished_at_ms"], &u64)) {
      if (error) *error = "runtime job record has invalid finished_at_ms";
      return false;
    }
    parsed.finished_at_ms = u64;
  }
  if (!kv["runner_pid"].empty()) {
    if (!parse_u64(kv["runner_pid"], &u64)) {
      if (error) *error = "runtime job record has invalid runner_pid";
      return false;
    }
    parsed.runner_pid = u64;
  }
  if (!kv["runner_start_ticks"].empty()) {
    if (!parse_u64(kv["runner_start_ticks"], &u64)) {
      if (error) *error = "runtime job record has invalid runner_start_ticks";
      return false;
    }
    parsed.runner_start_ticks = u64;
  }
  if (!kv["target_pid"].empty()) {
    if (!parse_u64(kv["target_pid"], &u64)) {
      if (error) *error = "runtime job record has invalid target_pid";
      return false;
    }
    parsed.target_pid = u64;
  }
  if (!kv["target_pgid"].empty()) {
    if (!parse_u64(kv["target_pgid"], &u64)) {
      if (error) *error = "runtime job record has invalid target_pgid";
      return false;
    }
    parsed.target_pgid = u64;
  }
  if (!kv["target_start_ticks"].empty()) {
    if (!parse_u64(kv["target_start_ticks"], &u64)) {
      if (error) *error = "runtime job record has invalid target_start_ticks";
      return false;
    }
    parsed.target_start_ticks = u64;
  }
  if (!kv["exit_code"].empty()) {
    if (!parse_i64(kv["exit_code"], &i64)) {
      if (error) *error = "runtime job record has invalid exit_code";
      return false;
    }
    parsed.exit_code = i64;
  }
  if (!kv["term_signal"].empty()) {
    if (!parse_i64(kv["term_signal"], &i64)) {
      if (error) *error = "runtime job record has invalid term_signal";
      return false;
    }
    parsed.term_signal = i64;
  }

  *out = std::move(parsed);
  return true;
}

[[nodiscard]] inline bool write_runtime_job_record(
    const std::filesystem::path& campaigns_root,
    const runtime_job_record_t& record,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (record.job_cursor.empty()) {
    if (error) *error = "runtime job record missing job_cursor";
    return false;
  }

  const auto job_dir = runtime_job_dir(campaigns_root, record.job_cursor);
  std::error_code ec{};
  std::filesystem::create_directories(job_dir, ec);
  if (ec) {
    if (error) *error = "cannot create runtime job directory: " + job_dir.string();
    return false;
  }

  const auto document = runtime_job_record_to_document(record);
  std::string validate_error{};
  if (!cuwacunu::piaabo::latent_lineage_state::validate_runtime_lls_document(
          document, &validate_error)) {
    if (error) *error = validate_error;
    return false;
  }
  return write_text_file_atomic(
      runtime_job_manifest_path(campaigns_root, record.job_cursor),
                                cuwacunu::piaabo::latent_lineage_state::
                                    emit_runtime_lls_canonical(document),
                                error);
}

[[nodiscard]] inline bool read_runtime_job_record(
    const std::filesystem::path& campaigns_root, std::string_view job_cursor,
    runtime_job_record_t* out, std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "job record output pointer is null";
    return false;
  }

  std::string text{};
  if (!read_text_file(runtime_job_manifest_path(campaigns_root, job_cursor),
                      &text,
                      error)) {
    return false;
  }
  cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t document{};
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          text, &document, error)) {
    return false;
  }
  return parse_runtime_job_record(document, out, error);
}

[[nodiscard]] inline bool is_legacy_runtime_job_schema_error(
    std::string_view error) {
  constexpr std::string_view kPrefix = "unexpected runtime job schema: ";
  if (error.rfind(kPrefix, 0) != 0) return false;
  const std::string_view schema = error.substr(kPrefix.size());
  return !schema.empty() && schema != kRuntimeJobSchemaV3 &&
         schema.rfind("hero.runtime.job.v", 0) == 0;
}

[[nodiscard]] inline bool scan_runtime_job_records(
    const std::filesystem::path& campaigns_root,
    std::vector<runtime_job_record_t>* out,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "runtime job list output pointer is null";
    return false;
  }
  out->clear();

  std::error_code ec{};
  if (!std::filesystem::exists(campaigns_root, ec)) return true;
  if (ec) {
    if (error) *error = "cannot access campaigns_root: " + campaigns_root.string();
    return false;
  }

  const auto scan_job_leaf_dir =
      [&](const std::filesystem::path& jobs_dir) -> bool {
    if (!std::filesystem::exists(jobs_dir, ec)) return true;
    if (ec) {
      if (error) *error = "cannot access runtime jobs directory: " + jobs_dir.string();
      return false;
    }
    for (std::filesystem::directory_iterator job_it(jobs_dir, ec), job_end;
         job_it != job_end; job_it.increment(ec)) {
      if (ec) {
        if (error) *error = "failed scanning runtime jobs directory: " + jobs_dir.string();
        return false;
      }
      if (!job_it->is_directory(ec)) {
        if (ec) {
          if (error) *error = "failed reading runtime job entry type";
          return false;
        }
        continue;
      }
      const auto manifest_path =
          job_it->path() / std::string(kRuntimeJobManifestFilename);
      if (!std::filesystem::exists(manifest_path, ec) || ec) continue;
      runtime_job_record_t record{};
      std::string record_error{};
      if (!read_runtime_job_record(campaigns_root,
                                   job_it->path().filename().string(),
                                   &record, &record_error)) {
        if (is_legacy_runtime_job_schema_error(record_error)) continue;
        if (error) {
          *error = "failed reading runtime job manifest " +
                   runtime_job_manifest_path(campaigns_root,
                                             job_it->path().filename().string())
                       .string() +
                   ": " + record_error;
        }
        return false;
      }
      out->push_back(std::move(record));
    }
    return true;
  };

  for (std::filesystem::directory_iterator it(campaigns_root, ec), end; it != end;
       it.increment(ec)) {
    if (ec) {
      if (error) {
        *error = "failed scanning campaigns_root for runtime jobs: " +
                 campaigns_root.string();
      }
      return false;
    }
    if (!it->is_directory(ec)) {
      if (ec) {
        if (error) *error = "failed reading campaigns_root entry type";
        return false;
      }
      continue;
    }
    const std::string entry_name = it->path().filename().string();
    if (entry_name == kRuntimeLegacyJobsDirname) {
      if (!scan_job_leaf_dir(it->path())) return false;
      continue;
    }
    if (!scan_job_leaf_dir(it->path() / std::string(kRuntimeCampaignJobsDirname))) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool pid_alive(std::uint64_t pid_value) {
  if (pid_value == 0) return false;
  const pid_t pid = static_cast<pid_t>(pid_value);
  if (pid <= 0) return false;
  if (::kill(pid, 0) == 0) return true;
  return errno == EPERM;
}

[[nodiscard]] inline bool read_process_start_ticks(std::uint64_t pid_value,
                                                   std::uint64_t* out) {
  if (!out) return false;
  *out = 0;
  if (pid_value == 0) return false;
  const pid_t pid = static_cast<pid_t>(pid_value);
  if (pid <= 0) return false;

  std::ifstream in("/proc/" + std::to_string(pid) + "/stat");
  if (!in) return false;
  std::string stat_line;
  std::getline(in, stat_line);
  if (stat_line.empty()) return false;

  const std::size_t close_paren = stat_line.rfind(')');
  if (close_paren == std::string::npos || close_paren + 2 >= stat_line.size()) {
    return false;
  }
  std::istringstream rest(stat_line.substr(close_paren + 2));
  std::string token;
  for (int index = 0; index <= 19; ++index) {
    if (!(rest >> token)) return false;
  }
  return parse_u64(token, out);
}

[[nodiscard]] inline bool process_identity_alive(
    std::uint64_t pid_value, const std::optional<std::uint64_t>& start_ticks,
    std::string_view boot_id) {
  if (pid_value == 0) return false;
  if (::kill(static_cast<pid_t>(pid_value), 0) != 0 && errno != EPERM) {
    return false;
  }
  const std::string current_boot = current_boot_id();
  if (!boot_id.empty() && !current_boot.empty() && current_boot != boot_id) {
    return false;
  }
  if (!start_ticks.has_value()) return false;
  std::uint64_t current_start_ticks = 0;
  if (!read_process_start_ticks(pid_value, &current_start_ticks)) return false;
  return current_start_ticks == *start_ticks;
}

[[nodiscard]] inline bool process_group_alive(
    std::uint64_t pgid_value, const std::optional<std::uint64_t>& leader_start_ticks,
    std::string_view boot_id) {
  if (pgid_value == 0) return false;
  const pid_t pgid = static_cast<pid_t>(pgid_value);
  if (pgid <= 0) return false;
  if (::kill(-pgid, 0) != 0 && errno != EPERM) return false;
  return process_identity_alive(pgid_value, leader_start_ticks, boot_id);
}

[[nodiscard]] inline runtime_job_observation_t observe_runtime_job(
    const runtime_job_record_t& record) {
  runtime_job_observation_t observation{};
  if (record.runner_pid.has_value()) {
    observation.runner_alive = process_identity_alive(
        *record.runner_pid, record.runner_start_ticks, record.boot_id);
  }
  if (record.target_pgid.has_value()) {
    observation.target_alive = process_group_alive(
        *record.target_pgid, record.target_start_ticks, record.boot_id);
  } else if (record.target_pid.has_value()) {
    observation.target_alive = process_identity_alive(
        *record.target_pid, record.target_start_ticks, record.boot_id);
  }
  return observation;
}

[[nodiscard]] inline std::string stable_state_for_observation(
    const runtime_job_record_t& record,
    const runtime_job_observation_t& observation) {
  const bool active_state =
      record.state == "launching" || record.state == "running" ||
      record.state == "stopping";
  if (!active_state) return record.state;
  if (observation.target_alive && observation.runner_alive) {
    return record.state == "launching" ? std::string("running") : record.state;
  }
  if (observation.target_alive && !observation.runner_alive) return "orphaned";
  if (!observation.target_alive && !observation.runner_alive) return "failed";
  return record.state;
}

[[nodiscard]] inline bool list_active_runtime_job_cursors(
    const std::filesystem::path& campaigns_root, std::vector<std::string>* out,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for active runtime jobs";
    return false;
  }
  out->clear();

  std::vector<runtime_job_record_t> records{};
  if (!scan_runtime_job_records(campaigns_root, &records, error)) return false;
  for (const auto& record : records) {
    const runtime_job_observation_t observation = observe_runtime_job(record);
    const std::string stable_state =
        stable_state_for_observation(record, observation);
    if (stable_state == "launching" || stable_state == "running" ||
        stable_state == "stopping" || stable_state == "orphaned") {
      out->push_back(record.job_cursor);
    }
  }
  return true;
}

}  // namespace runtime
}  // namespace hero
}  // namespace cuwacunu
