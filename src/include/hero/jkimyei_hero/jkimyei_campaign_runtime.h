#pragma once

#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "piaabo/latent_lineage_state/runtime_lls.h"
#include "hero/runtime_hero/runtime_job.h"
#include "iitepi/config_space_t.h"

namespace cuwacunu {
namespace hero {
namespace jkimyei {

inline constexpr std::string_view kJkimyeiCampaignSchemaV1 =
    "hero.jkimyei.campaign.v1";
inline constexpr std::string_view kJkimyeiCampaignManifestFilename =
    "campaign.lls";
inline constexpr std::string_view kJkimyeiCampaignDslFilename = "campaign.dsl";

struct jkimyei_campaign_record_t {
  std::string schema{std::string(kJkimyeiCampaignSchemaV1)};
  std::string campaign_cursor{};
  std::string state{"staged"};
  std::string state_detail{};
  std::string config_folder{};
  std::string source_campaign_dsl_path{};
  std::string campaign_dsl_path{};
  std::string requested_campaign_id{};
  std::string active_campaign_id{};
  std::string resolved_campaign_id{};
  std::uint64_t started_at_ms{0};
  std::uint64_t updated_at_ms{0};
  std::optional<std::uint64_t> finished_at_ms{};
  std::vector<std::string> bind_ids{};
};

[[nodiscard]] inline std::filesystem::path jkimyei_campaign_dir(
    const std::filesystem::path& campaigns_root,
    std::string_view campaign_cursor) {
  return campaigns_root / std::string(campaign_cursor);
}

[[nodiscard]] inline std::filesystem::path jkimyei_campaign_manifest_path(
    const std::filesystem::path& campaigns_root,
    std::string_view campaign_cursor) {
  return jkimyei_campaign_dir(campaigns_root, campaign_cursor) /
         std::string(kJkimyeiCampaignManifestFilename);
}

[[nodiscard]] inline std::filesystem::path jkimyei_campaign_dsl_path(
    const std::filesystem::path& campaigns_root,
    std::string_view campaign_cursor) {
  return jkimyei_campaign_dir(campaigns_root, campaign_cursor) /
         std::string(kJkimyeiCampaignDslFilename);
}

[[nodiscard]] inline std::string format_bind_index(std::size_t index) {
  std::ostringstream out;
  out << std::setw(4) << std::setfill('0') << index;
  return out.str();
}

[[nodiscard]] inline std::string make_campaign_cursor(
    const std::filesystem::path& campaigns_root) {
  const std::uint64_t started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  for (std::uint64_t nonce = 1; nonce != 0; ++nonce) {
    const std::uint64_t salt =
        static_cast<std::uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()) ^
        (nonce * 0x9e3779b97f4a7c15ULL);
    const std::string candidate =
        "campaign." + std::to_string(started_at_ms) + "." +
        cuwacunu::hero::runtime::hex_lower_u64(salt).substr(0, 8);
    std::error_code ec{};
    if (!std::filesystem::exists(jkimyei_campaign_dir(campaigns_root, candidate),
                                 ec) ||
        ec) {
      return candidate;
    }
  }
  return "campaign." + std::to_string(started_at_ms) + ".overflow";
}

namespace detail {

[[nodiscard]] inline std::string trim_ascii(std::string_view in) {
  return cuwacunu::hero::runtime::trim_ascii(in);
}

[[nodiscard]] inline std::string resolve_path_from_base_folder(
    std::string base_folder,
    std::string raw_path) {
  base_folder = trim_ascii(base_folder);
  raw_path = trim_ascii(raw_path);
  if (raw_path.empty()) return {};
  const std::filesystem::path p(raw_path);
  if (p.is_absolute()) return p.lexically_normal().string();
  if (base_folder.empty()) return p.lexically_normal().string();
  return (std::filesystem::path(base_folder) / p).lexically_normal().string();
}

[[nodiscard]] inline std::string strip_ini_comment(std::string_view line) {
  std::string out;
  out.reserve(line.size());
  bool in_single = false;
  bool in_double = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (c == '\'' && !in_double) {
      in_single = !in_single;
      out.push_back(c);
      continue;
    }
    if (c == '"' && !in_single) {
      in_double = !in_double;
      out.push_back(c);
      continue;
    }
    if (!in_single && !in_double && (c == '#' || c == ';')) break;
    out.push_back(c);
  }
  return out;
}

[[nodiscard]] inline bool read_ini_value(const std::filesystem::path& path,
                                         std::string_view section,
                                         std::string_view key,
                                         std::string* out_value) {
  if (out_value) out_value->clear();
  std::ifstream in(path);
  if (!in) return false;

  std::string current_section{};
  std::string line{};
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(strip_ini_comment(line));
    if (trimmed.empty()) continue;
    if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']') {
      current_section = trim_ascii(trimmed.substr(1, trimmed.size() - 2));
      continue;
    }
    if (current_section != section) continue;
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos) continue;
    if (trim_ascii(trimmed.substr(0, eq)) != key) continue;
    if (out_value) *out_value = trim_ascii(trimmed.substr(eq + 1));
    return true;
  }
  return false;
}

[[nodiscard]] inline bool next_block_comment_state(
    std::string_view line,
    bool in_block_comment) {
  bool in_single_quote = false;
  bool in_double_quote = false;
  for (std::size_t i = 0; i < line.size();) {
    const char c = line[i];
    const char next = (i + 1 < line.size()) ? line[i + 1] : '\0';
    if (in_block_comment) {
      if (c == '*' && next == '/') {
        in_block_comment = false;
        i += 2;
      } else {
        ++i;
      }
      continue;
    }
    if (!in_double_quote && c == '\'') {
      in_single_quote = !in_single_quote;
      ++i;
      continue;
    }
    if (!in_single_quote && c == '"') {
      in_double_quote = !in_double_quote;
      ++i;
      continue;
    }
    if (!in_single_quote && !in_double_quote && c == '/' && next == '*') {
      in_block_comment = true;
      i += 2;
      continue;
    }
    ++i;
  }
  return in_block_comment;
}

[[nodiscard]] inline std::string rewrite_campaign_imports_absolute(
    std::string_view text,
    const std::filesystem::path& source_campaign_path) {
  const auto find_token_outside_comments =
      [](std::string_view line, std::string_view token,
         bool in_block_comment) -> std::size_t {
        bool in_single_quote = false;
        bool in_double_quote = false;
        for (std::size_t i = 0; i < line.size();) {
          const char c = line[i];
          const char next = (i + 1 < line.size()) ? line[i + 1] : '\0';
          if (in_block_comment) {
            if (c == '*' && next == '/') {
              in_block_comment = false;
              i += 2;
              continue;
            }
            ++i;
            continue;
          }
          if (!in_double_quote && c == '\'') {
            in_single_quote = !in_single_quote;
            ++i;
            continue;
          }
          if (!in_single_quote && c == '"') {
            in_double_quote = !in_double_quote;
            ++i;
            continue;
          }
          if (!in_single_quote && !in_double_quote) {
            if (c == '/' && next == '*') {
              in_block_comment = true;
              i += 2;
              continue;
            }
            if (c == '/' && next == '/') break;
            if (c == '#' || c == ';') break;
            if (i + token.size() <= line.size() &&
                line.compare(i, token.size(), token) == 0) {
              return i;
            }
          }
          ++i;
        }
        return std::string::npos;
      };

  const auto rewrite_line =
      [&](const std::string& line,
          std::string_view token,
          bool in_block_comment) -> std::string {
        const std::size_t token_pos =
            find_token_outside_comments(line, token, in_block_comment);
        if (token_pos == std::string::npos) return line;
        const std::size_t quote_begin = line.find('"', token_pos + token.size());
        if (quote_begin == std::string::npos) return line;
        const std::size_t quote_end = line.find('"', quote_begin + 1);
        if (quote_end == std::string::npos) return line;
        const std::string import_path =
            line.substr(quote_begin + 1, quote_end - quote_begin - 1);
        if (import_path.empty()) return line;
        const std::filesystem::path raw(import_path);
        if (raw.is_absolute()) return line;
        const std::filesystem::path rewritten =
            (source_campaign_path.parent_path() / raw).lexically_normal();
        return line.substr(0, quote_begin + 1) + rewritten.string() +
               line.substr(quote_end);
      };

  std::ostringstream out;
  std::size_t pos = 0;
  bool first = true;
  bool in_block_comment = false;
  while (pos <= text.size()) {
    const std::size_t end = text.find('\n', pos);
    std::string line = (end == std::string::npos)
                           ? std::string(text.substr(pos))
                           : std::string(text.substr(pos, end - pos));
    const std::string trimmed = trim_ascii(line);
    const bool line_is_comment_only =
        !in_block_comment &&
        (trimmed.rfind("//", 0) == 0 || trimmed.rfind("#", 0) == 0 ||
         trimmed.rfind(";", 0) == 0);
    if (!line_is_comment_only) {
      line = rewrite_line(line, "IMPORT_CONTRACT_FILE", in_block_comment);
      line = rewrite_line(line, "IMPORT_WAVE_FILE", in_block_comment);
    }
    if (!first) out << "\n";
    first = false;
    out << line;
    in_block_comment = next_block_comment_state(line, in_block_comment);
    if (end == std::string::npos) break;
    pos = end + 1;
  }
  return out.str();
}

}  // namespace detail

[[nodiscard]] inline bool prepare_campaign_dsl_snapshot(
    const std::filesystem::path& campaigns_root,
    std::string_view campaign_cursor,
    const std::filesystem::path& config_folder,
    std::string* out_source_campaign_path,
    std::string* out_campaign_snapshot_path,
    std::string* error) {
  if (error) error->clear();
  if (out_source_campaign_path) out_source_campaign_path->clear();
  if (out_campaign_snapshot_path) out_campaign_snapshot_path->clear();

  const std::filesystem::path config_path =
      config_folder / DEFAULT_CONFIG_FILE;
  std::string configured = DEFAULT_JKIMYEI_CAMPAIGN_DSL_FILE;
  std::string from_ini{};
  if (detail::read_ini_value(config_path, "GENERAL",
                             GENERAL_DEFAULT_JKIMYEI_CAMPAIGN_DSL_KEY,
                             &from_ini) &&
      !from_ini.empty()) {
    configured = from_ini;
  }

  const std::filesystem::path source_campaign_path(
      detail::resolve_path_from_base_folder(config_folder.string(), configured));
  std::error_code ec{};
  if (!std::filesystem::exists(source_campaign_path, ec) ||
      !std::filesystem::is_regular_file(source_campaign_path, ec)) {
    if (error) {
      *error = "configured default jkimyei campaign DSL does not exist: " +
               source_campaign_path.string();
    }
    return false;
  }

  std::string campaign_text{};
  if (!cuwacunu::hero::runtime::read_text_file(source_campaign_path,
                                               &campaign_text, error)) {
    return false;
  }
  const std::string snapshot_text =
      detail::rewrite_campaign_imports_absolute(campaign_text,
                                                source_campaign_path);
  const std::filesystem::path snapshot_path =
      jkimyei_campaign_dsl_path(campaigns_root, campaign_cursor);
  if (!cuwacunu::hero::runtime::write_text_file_atomic(snapshot_path,
                                                       snapshot_text, error)) {
    return false;
  }

  if (out_source_campaign_path) {
    *out_source_campaign_path = source_campaign_path.string();
  }
  if (out_campaign_snapshot_path) {
    *out_campaign_snapshot_path = snapshot_path.string();
  }
  return true;
}

[[nodiscard]] inline cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t
jkimyei_campaign_record_to_document(const jkimyei_campaign_record_t& record) {
  using namespace cuwacunu::piaabo::latent_lineage_state;
  runtime_lls_document_t document{};
  document.entries.push_back(make_runtime_lls_string_entry("schema", record.schema));
  document.entries.push_back(
      make_runtime_lls_string_entry("campaign_cursor", record.campaign_cursor));
  document.entries.push_back(make_runtime_lls_string_entry("state", record.state));
  document.entries.push_back(
      make_runtime_lls_string_entry("state_detail", record.state_detail));
  document.entries.push_back(
      make_runtime_lls_string_entry("config_folder", record.config_folder));
  document.entries.push_back(make_runtime_lls_string_entry(
      "source_campaign_dsl_path", record.source_campaign_dsl_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "campaign_dsl_path", record.campaign_dsl_path));
  document.entries.push_back(make_runtime_lls_string_entry(
      "requested_campaign_id", record.requested_campaign_id));
  document.entries.push_back(make_runtime_lls_string_entry(
      "active_campaign_id", record.active_campaign_id));
  document.entries.push_back(make_runtime_lls_string_entry(
      "resolved_campaign_id", record.resolved_campaign_id));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "started_at_ms", record.started_at_ms, "(0,+inf)"));
  document.entries.push_back(make_runtime_lls_uint_entry(
      "updated_at_ms", record.updated_at_ms, "(0,+inf)"));
  if (record.finished_at_ms.has_value()) {
    document.entries.push_back(make_runtime_lls_uint_entry(
        "finished_at_ms", *record.finished_at_ms, "(0,+inf)"));
  }
  for (std::size_t i = 0; i < record.bind_ids.size(); ++i) {
    document.entries.push_back(make_runtime_lls_string_entry(
        "bind_id." + format_bind_index(i), record.bind_ids[i]));
  }
  return document;
}

[[nodiscard]] inline bool parse_jkimyei_campaign_record_document(
    const cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t& document,
    jkimyei_campaign_record_t* out,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for jkimyei campaign record";
    return false;
  }

  std::unordered_map<std::string, std::string> kv{};
  if (!cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          document, &kv, error)) {
    return false;
  }

  jkimyei_campaign_record_t parsed{};
  parsed.schema = kv["schema"];
  parsed.campaign_cursor = kv["campaign_cursor"];
  parsed.state = kv["state"];
  parsed.state_detail = kv["state_detail"];
  parsed.config_folder = kv["config_folder"];
  parsed.source_campaign_dsl_path = kv["source_campaign_dsl_path"];
  parsed.campaign_dsl_path = kv["campaign_dsl_path"];
  parsed.requested_campaign_id = kv["requested_campaign_id"];
  parsed.active_campaign_id = kv["active_campaign_id"];
  parsed.resolved_campaign_id = kv["resolved_campaign_id"];
  if (!cuwacunu::hero::runtime::parse_u64(kv["started_at_ms"],
                                          &parsed.started_at_ms)) {
    if (error) *error = "jkimyei campaign record missing started_at_ms";
    return false;
  }
  if (!cuwacunu::hero::runtime::parse_u64(kv["updated_at_ms"],
                                          &parsed.updated_at_ms)) {
    if (error) *error = "jkimyei campaign record missing updated_at_ms";
    return false;
  }
  std::uint64_t finished_at_ms = 0;
  if (cuwacunu::hero::runtime::parse_u64(kv["finished_at_ms"], &finished_at_ms)) {
    parsed.finished_at_ms = finished_at_ms;
  }
  if (parsed.campaign_cursor.empty()) {
    if (error) *error = "jkimyei campaign record missing campaign_cursor";
    return false;
  }
  if (parsed.state.empty()) {
    if (error) *error = "jkimyei campaign record missing state";
    return false;
  }

  std::map<std::string, std::string, std::less<>> ordered(kv.begin(), kv.end());
  for (const auto& [key, value] : ordered) {
    if (key.rfind("bind_id.", 0) == 0) parsed.bind_ids.push_back(value);
  }

  *out = std::move(parsed);
  return true;
}

[[nodiscard]] inline bool write_jkimyei_campaign_record(
    const std::filesystem::path& campaigns_root,
    const jkimyei_campaign_record_t& record,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (record.campaign_cursor.empty()) {
    if (error) *error = "jkimyei campaign record missing campaign_cursor";
    return false;
  }
  const auto manifest_path =
      jkimyei_campaign_manifest_path(campaigns_root, record.campaign_cursor);
  const std::string text =
      cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
          jkimyei_campaign_record_to_document(record));
  return cuwacunu::hero::runtime::write_text_file_atomic(manifest_path,
                                                         text, error);
}

[[nodiscard]] inline bool read_jkimyei_campaign_record(
    const std::filesystem::path& campaigns_root,
    std::string_view campaign_cursor,
    jkimyei_campaign_record_t* out,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for jkimyei campaign record";
    return false;
  }
  std::string text{};
  if (!cuwacunu::hero::runtime::read_text_file(
          jkimyei_campaign_manifest_path(campaigns_root, campaign_cursor),
          &text, error)) {
    return false;
  }
  cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t document{};
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          text, &document, error)) {
    return false;
  }
  return parse_jkimyei_campaign_record_document(document, out, error);
}

[[nodiscard]] inline bool list_jkimyei_campaign_records(
    const std::filesystem::path& campaigns_root,
    std::vector<jkimyei_campaign_record_t>* out,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for jkimyei campaign list";
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
    jkimyei_campaign_record_t record{};
    std::string record_error{};
    if (!read_jkimyei_campaign_record(campaigns_root,
                                      it.path().filename().string(),
                                      &record, &record_error)) {
      continue;
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

}  // namespace jkimyei
}  // namespace hero
}  // namespace cuwacunu
