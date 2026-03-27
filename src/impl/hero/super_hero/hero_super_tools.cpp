#include "hero/super_hero/hero_super_tools.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <tuple>
#include <unordered_map>
#include <unistd.h>
#include <vector>

#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"
#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "camahjucunu/dsl/super_objective/super_objective.h"
#include "hero/runtime_hero/runtime_campaign.h"
#include "hero/runtime_hero/runtime_job.h"
#include "hero/human_hero/human_attestation.h"
#include "hero/super_hero/super_loop.h"
#include "hero/super_hero/super_loop_workspace.h"
#include "hero/wave_contract_binding_runtime.h"

namespace cuwacunu {
namespace hero {
namespace super_mcp {

namespace {

constexpr const char* kDefaultCampaignDslKey =
    "default_iitepi_campaign_dsl_filename";
constexpr const char* kDefaultSuperObjectiveDslFilename =
    "default.super.objective.dsl";
constexpr const char* kServerName = "hero_super_mcp";
constexpr const char* kServerVersion = "0.1.0";
constexpr const char* kProtocolVersion = "2025-03-26";
constexpr const char* kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.super.*.";
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;
constexpr std::size_t kRuntimeToolTimeoutSec = 30;
constexpr std::string_view kReviewPacketSchemaV1 = "hero.super.review_packet.v1";
constexpr std::string_view kDecisionSchemaV1 = "hero.super.decision.v1";

bool g_jsonrpc_use_content_length_framing = false;

struct super_review_decision_t {
  std::string control_kind{};
  std::string next_action_kind{};
  std::string target_binding_id{};
  bool reset_runtime_state{false};
  std::string reason{};
  std::string memory_note{};
  std::string human_request{};
};

struct run_manifest_hint_t {
  std::filesystem::path path{};
  std::string run_id{};
  std::string binding_id{};
  std::string contract_hash{};
  std::string wave_hash{};
  std::uint64_t started_at_ms{0};
};

struct super_objective_spec_t {
  std::string campaign_dsl_path{};
  std::string objective_md_path{};
  std::string guidance_md_path{};
  std::string objective_name{};
  std::string loop_id{};
};

using super_tool_handler_t = bool (*)(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error);

struct super_tool_descriptor_t {
  std::string_view name;
  std::string_view description;
  std::string_view input_schema_json;
  super_tool_handler_t handler;
};

[[nodiscard]] bool run_command_with_stdio_and_timeout(
    const std::vector<std::string>& argv, const std::filesystem::path& stdin_path,
    const std::filesystem::path& stdout_path,
    const std::filesystem::path& stderr_path,
    const std::filesystem::path* working_dir,
    const std::vector<std::pair<std::string, std::string>>* env_overrides,
    std::size_t timeout_sec, const std::filesystem::path* pid_path,
    int* out_exit_code, std::string* error);

void append_warning_text(std::string* dst, std::string_view warning);

[[nodiscard]] bool read_super_loop(
    const app_context_t& app, std::string_view loop_id,
    cuwacunu::hero::super::super_loop_record_t* out, std::string* error);

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  return cuwacunu::hero::runtime::trim_ascii(in);
}

[[nodiscard]] std::string lowercase_copy(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] bool looks_like_codex_session_id(std::string_view value) {
  if (value.size() != 36) return false;
  for (std::size_t i = 0; i < value.size(); ++i) {
    const char ch = value[i];
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (ch != '-') return false;
      continue;
    }
    if (!std::isxdigit(static_cast<unsigned char>(ch))) return false;
  }
  return true;
}

[[nodiscard]] std::string extract_codex_session_id_from_log(
    std::string_view log_text) {
  const std::size_t marker = log_text.rfind("session id:");
  if (marker == std::string_view::npos) return {};
  const std::size_t value_begin = marker + std::string_view("session id:").size();
  const std::size_t line_end = log_text.find('\n', value_begin);
  const std::string candidate =
      lowercase_copy(trim_ascii(log_text.substr(
          value_begin, line_end == std::string_view::npos
                           ? std::string_view::npos
                           : line_end - value_begin)));
  if (!looks_like_codex_session_id(candidate)) return {};
  return candidate;
}

[[nodiscard]] std::string strip_inline_comment(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  bool in_single = false;
  bool in_double = false;
  for (const char c : in) {
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

[[nodiscard]] bool path_is_within(const std::filesystem::path& root,
                                  const std::filesystem::path& candidate) {
  if (root.empty() || candidate.empty()) return false;
  const std::filesystem::path normalized_root = root.lexically_normal();
  const std::filesystem::path normalized_candidate =
      candidate.lexically_normal();
  auto root_it = normalized_root.begin();
  auto candidate_it = normalized_candidate.begin();
  for (; root_it != normalized_root.end(); ++root_it, ++candidate_it) {
    if (candidate_it == normalized_candidate.end() || *root_it != *candidate_it) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::string json_escape(std::string_view in) {
  std::ostringstream out;
  for (const unsigned char c : in) {
    switch (c) {
      case '"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (c < 0x20) {
          static constexpr char kHex[] = "0123456789abcdef";
          out << "\\u00" << kHex[(c >> 4) & 0x0F] << kHex[c & 0x0F];
        } else {
          out << static_cast<char>(c);
        }
        break;
    }
  }
  return out.str();
}

[[nodiscard]] std::string json_quote(std::string_view in) {
  return "\"" + json_escape(in) + "\"";
}

[[nodiscard]] std::string bool_json(bool value) {
  return value ? "true" : "false";
}

[[nodiscard]] std::string unquote_if_wrapped(std::string s) {
  s = trim_ascii(std::move(s));
  if (s.size() >= 2) {
    const char a = s.front();
    const char b = s.back();
    if ((a == '"' && b == '"') || (a == '\'' && b == '\'')) {
      return s.substr(1, s.size() - 2);
    }
  }
  return s;
}

[[nodiscard]] std::string resolve_path_from_base_folder(std::string base_folder,
                                                        std::string path) {
  base_folder = trim_ascii(std::move(base_folder));
  path = trim_ascii(std::move(path));
  if (path.empty()) return {};
  const std::filesystem::path p(path);
  if (p.is_absolute()) return p.lexically_normal().string();
  if (base_folder.empty()) return p.lexically_normal().string();
  return (std::filesystem::path(base_folder) / p).lexically_normal().string();
}

[[nodiscard]] std::optional<std::string> read_ini_value(
    const std::filesystem::path& ini_path, const std::string& section,
    const std::string& key) {
  std::ifstream in(ini_path);
  if (!in) return std::nullopt;
  std::string line;
  std::string current_section;
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(strip_inline_comment(line));
    if (trimmed.empty()) continue;
    if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']') {
      current_section = trim_ascii(trimmed.substr(1, trimmed.size() - 2));
      continue;
    }
    if (current_section != section) continue;
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos) continue;
    if (trim_ascii(trimmed.substr(0, eq)) != key) continue;
    return trim_ascii(trimmed.substr(eq + 1));
  }
  return std::nullopt;
}

[[nodiscard]] bool parse_size_token(std::string_view raw, std::size_t* out) {
  if (!out) return false;
  const std::string token = trim_ascii(raw);
  if (token.empty()) return false;
  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(token.data(), token.data() + token.size(), parsed);
  if (ec != std::errc{} || ptr != token.data() + token.size()) return false;
  *out = static_cast<std::size_t>(parsed);
  return true;
}

[[nodiscard]] bool parse_bool_token(std::string_view raw, bool* out) {
  if (!out) return false;
  const std::string lowered = lowercase_copy(trim_ascii(raw));
  if (lowered == "true" || lowered == "1" || lowered == "yes" ||
      lowered == "y" || lowered == "on") {
    *out = true;
    return true;
  }
  if (lowered == "false" || lowered == "0" || lowered == "no" ||
      lowered == "n" || lowered == "off") {
    *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] std::string json_array_from_strings(
    const std::vector<std::string>& values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) out << ",";
    out << json_quote(values[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string json_array_from_paths(
    const std::vector<std::filesystem::path>& values) {
  std::vector<std::string> encoded{};
  encoded.reserve(values.size());
  for (const auto& value : values) encoded.push_back(value.string());
  return json_array_from_strings(encoded);
}

[[nodiscard]] bool is_valid_super_loop_id(std::string_view loop_id) {
  const std::string trimmed = trim_ascii(loop_id);
  if (trimmed.empty()) return false;
  for (const unsigned char ch : trimmed) {
    const bool ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' ||
                    ch == '-';
    if (!ok) return false;
  }
  return true;
}

[[nodiscard]] std::size_t skip_json_whitespace(const std::string& json,
                                               std::size_t pos) {
  while (pos < json.size() &&
         std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
    ++pos;
  }
  return pos;
}

[[nodiscard]] bool parse_json_string_at(const std::string& json, std::size_t pos,
                                        std::string* out,
                                        std::size_t* out_end) {
  if (out) out->clear();
  if (pos >= json.size() || json[pos] != '"') return false;
  ++pos;
  std::string parsed;
  while (pos < json.size()) {
    const char c = json[pos++];
    if (c == '"') {
      if (out) *out = parsed;
      if (out_end) *out_end = pos;
      return true;
    }
    if (c == '\\') {
      if (pos >= json.size()) return false;
      const char esc = json[pos++];
      switch (esc) {
        case '"':
        case '\\':
        case '/':
          parsed.push_back(esc);
          break;
        case 'b':
          parsed.push_back('\b');
          break;
        case 'f':
          parsed.push_back('\f');
          break;
        case 'n':
          parsed.push_back('\n');
          break;
        case 'r':
          parsed.push_back('\r');
          break;
        case 't':
          parsed.push_back('\t');
          break;
        case 'u': {
          if (pos + 4 > json.size()) return false;
          pos += 4;
          parsed.push_back('?');
          break;
        }
        default:
          return false;
      }
      continue;
    }
    parsed.push_back(c);
  }
  return false;
}

[[nodiscard]] bool find_json_value_end(const std::string& json, std::size_t pos,
                                       std::size_t* out_end) {
  pos = skip_json_whitespace(json, pos);
  if (pos >= json.size()) return false;

  const char first = json[pos];
  if (first == '"') {
    return parse_json_string_at(json, pos, nullptr, out_end);
  }
  if (first == '{' || first == '[') {
    int depth = 0;
    bool in_string = false;
    char string_delim = '\0';
    for (std::size_t i = pos; i < json.size(); ++i) {
      const char c = json[i];
      if (in_string) {
        if (c == '\\') {
          ++i;
          continue;
        }
        if (c == string_delim) {
          in_string = false;
        }
        continue;
      }
      if (c == '"' || c == '\'') {
        in_string = true;
        string_delim = c;
        continue;
      }
      if (c == '{' || c == '[') ++depth;
      if (c == '}' || c == ']') {
        --depth;
        if (depth == 0) {
          if (out_end) *out_end = i + 1;
          return true;
        }
      }
    }
    return false;
  }
  std::size_t i = pos;
  while (i < json.size()) {
    const char c = json[i];
    if (c == ',' || c == '}' || c == ']') break;
    ++i;
  }
  while (i > pos &&
         std::isspace(static_cast<unsigned char>(json[i - 1])) != 0) {
    --i;
  }
  if (i == pos) return false;
  if (out_end) *out_end = i;
  return true;
}

[[nodiscard]] bool extract_json_field_raw(const std::string& json,
                                          std::string_view key,
                                          std::string* out_raw) {
  if (out_raw) out_raw->clear();
  const std::string needle = "\"" + std::string(key) + "\"";
  std::size_t pos = 0;
  while (true) {
    pos = json.find(needle, pos);
    if (pos == std::string::npos) return false;
    pos += needle.size();
    pos = skip_json_whitespace(json, pos);
    if (pos >= json.size() || json[pos] != ':') continue;
    pos = skip_json_whitespace(json, pos + 1);
    std::size_t end = 0;
    if (!find_json_value_end(json, pos, &end)) return false;
    if (out_raw) *out_raw = json.substr(pos, end - pos);
    return true;
  }
}

[[nodiscard]] bool extract_json_string_field(const std::string& json,
                                             std::string_view key,
                                             std::string* out) {
  std::string raw;
  if (!extract_json_field_raw(json, key, &raw)) return false;
  std::size_t end = 0;
  return parse_json_string_at(raw, 0, out, &end) &&
         skip_json_whitespace(raw, end) == raw.size();
}

[[nodiscard]] bool extract_json_bool_field(const std::string& json,
                                           std::string_view key, bool* out) {
  std::string raw;
  if (!extract_json_field_raw(json, key, &raw)) return false;
  if (raw == "true") {
    if (out) *out = true;
    return true;
  }
  if (raw == "false") {
    if (out) *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] bool extract_json_size_field(const std::string& json,
                                           std::string_view key,
                                           std::size_t* out) {
  std::string raw;
  if (!extract_json_field_raw(json, key, &raw)) return false;
  return parse_size_token(raw, out);
}

[[nodiscard]] bool extract_json_object_field(const std::string& json,
                                             std::string_view key,
                                             std::string* out) {
  std::string raw;
  if (!extract_json_field_raw(json, key, &raw)) return false;
  raw = trim_ascii(raw);
  if (raw.empty() || raw.front() != '{' || raw.back() != '}') return false;
  if (out) *out = std::move(raw);
  return true;
}

[[nodiscard]] bool extract_json_array_field(const std::string& json,
                                            std::string_view key,
                                            std::string* out) {
  std::string raw;
  if (!extract_json_field_raw(json, key, &raw)) return false;
  raw = trim_ascii(raw);
  if (raw.empty() || raw.front() != '[' || raw.back() != ']') return false;
  if (out) *out = std::move(raw);
  return true;
}

[[nodiscard]] bool extract_json_string_array_field(
    const std::string& json, std::string_view key,
    std::vector<std::string>* out) {
  std::string raw{};
  if (!extract_json_array_field(json, key, &raw)) return false;
  if (!out) return true;
  out->clear();
  std::size_t pos = skip_json_whitespace(raw, 0);
  if (pos >= raw.size() || raw[pos] != '[') return false;
  pos = skip_json_whitespace(raw, pos + 1);
  if (pos < raw.size() && raw[pos] == ']') return true;
  while (pos < raw.size()) {
    std::string value{};
    std::size_t end = 0;
    if (!parse_json_string_at(raw, pos, &value, &end)) return false;
    out->push_back(std::move(value));
    pos = skip_json_whitespace(raw, end);
    if (pos >= raw.size()) return false;
    if (raw[pos] == ']') return true;
    if (raw[pos] != ',') return false;
    pos = skip_json_whitespace(raw, pos + 1);
  }
  return false;
}

[[nodiscard]] std::string make_tool_result_json(std::string_view text,
                                                std::string_view structured_json,
                                                bool is_error) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":" << json_quote(text)
      << "}],\"structuredContent\":" << structured_json
      << ",\"isError\":" << (is_error ? "true" : "false") << "}";
  return out.str();
}

[[nodiscard]] std::string tool_result_json_for_error(std::string_view message) {
  return make_tool_result_json(
      message,
      "{\"loop_id\":\"\",\"error\":" + json_quote(message) + "}", true);
}

[[nodiscard]] bool tool_result_is_error(std::string_view tool_result_json) {
  return tool_result_json.find("\"isError\":true") != std::string_view::npos ||
         tool_result_json.find("\"isError\": true") != std::string_view::npos;
}

[[nodiscard]] bool tool_result_structured_content(std::string_view tool_result_json,
                                                  std::string* out_structured,
                                                  std::string* out_error) {
  if (out_error) out_error->clear();
  if (!out_structured) {
    if (out_error) *out_error = "structuredContent output pointer is null";
    return false;
  }
  out_structured->clear();
  const std::string json(tool_result_json);
  if (tool_result_is_error(json)) {
    std::string structured{};
    if (extract_json_object_field(json, "structuredContent", &structured)) {
      std::string error_value{};
      if (extract_json_string_field(structured, "error", &error_value) &&
          !error_value.empty()) {
        if (out_error) *out_error = error_value;
        return false;
      }
    }
    std::string text{};
    if (extract_json_string_field(json, "text", &text) && !text.empty()) {
      if (out_error) *out_error = text;
      return false;
    }
    if (out_error) *out_error = "tool returned error";
    return false;
  }
  if (!extract_json_object_field(json, "structuredContent", out_structured)) {
    if (out_error) *out_error = "tool result missing structuredContent";
    return false;
  }
  return true;
}

[[nodiscard]] std::filesystem::path resolve_runtime_root_from_global_config(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "GENERAL", "runtime_root");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved);
}

[[nodiscard]] std::filesystem::path resolve_repo_root_from_global_config(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "GENERAL", "repo_root");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved);
}

[[nodiscard]] std::filesystem::path resolve_campaign_grammar_from_global_config(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "BNF",
                     "iitepi_campaign_grammar_filename");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved);
}

[[nodiscard]] std::filesystem::path
resolve_super_objective_grammar_from_global_config(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "BNF",
                     "super_objective_grammar_filename");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved);
}

[[nodiscard]] std::filesystem::path resolve_default_campaign_dsl_path(
    const std::filesystem::path& global_config_path, std::string* error) {
  if (error) error->clear();
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "GENERAL", kDefaultCampaignDslKey);
  if (!configured.has_value()) {
    if (error) {
      *error = "missing GENERAL." + std::string(kDefaultCampaignDslKey) + " in " +
               global_config_path.string();
    }
    return {};
  }
  return std::filesystem::path(resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured));
}

[[nodiscard]] std::filesystem::path resolve_default_super_objective_dsl_path(
    const std::filesystem::path& global_config_path, std::string* error) {
  if (error) error->clear();
  const std::filesystem::path default_campaign_path =
      resolve_default_campaign_dsl_path(global_config_path, error);
  if (default_campaign_path.empty()) return {};
  return (default_campaign_path.parent_path() / kDefaultSuperObjectiveDslFilename)
      .lexically_normal();
}

[[nodiscard]] std::filesystem::path resolve_command_path(
    const std::filesystem::path& base_dir, std::string configured) {
  configured = trim_ascii(std::move(configured));
  if (configured.empty()) return {};
  if (configured.find('/') != std::string::npos) {
    return std::filesystem::path(
        resolve_path_from_base_folder(base_dir.string(), configured));
  }
  const char* env_path = std::getenv("PATH");
  if (env_path != nullptr) {
    std::istringstream in(env_path);
    std::string entry{};
    while (std::getline(in, entry, ':')) {
      const std::filesystem::path candidate =
          (std::filesystem::path(entry) / configured).lexically_normal();
      if (::access(candidate.c_str(), X_OK) == 0) return candidate;
    }
  }
  return std::filesystem::path(configured);
}

[[nodiscard]] std::filesystem::path campaigns_root_for_app(
    const app_context_t& app) {
  return app.defaults.super_root.parent_path() / ".campaigns";
}

[[nodiscard]] std::filesystem::path loop_runner_lock_path(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  return std::filesystem::path(loop.loop_root) / ".super.runner.lock";
}

[[nodiscard]] std::filesystem::path super_loop_latest_review_packet_path(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  return cuwacunu::hero::super::super_loop_latest_review_packet_path(
      std::filesystem::path(loop.loop_root).parent_path(), loop.loop_id);
}

[[nodiscard]] std::filesystem::path super_loop_latest_decision_path(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  return cuwacunu::hero::super::super_loop_latest_decision_path(
      std::filesystem::path(loop.loop_root).parent_path(), loop.loop_id);
}

[[nodiscard]] std::filesystem::path super_loop_codex_session_log_path(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  return cuwacunu::hero::super::super_loop_codex_session_log_path(
      std::filesystem::path(loop.loop_root).parent_path(), loop.loop_id);
}

[[nodiscard]] std::filesystem::path super_loop_review_pid_path(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  return cuwacunu::hero::super::super_loop_review_pid_path(
      std::filesystem::path(loop.loop_root).parent_path(), loop.loop_id);
}

void remove_file_noexcept(const std::filesystem::path& path) {
  if (path.empty()) return;
  std::error_code ec{};
  (void)std::filesystem::remove(path, ec);
}

[[nodiscard]] bool read_loop_review_pid(
    const cuwacunu::hero::super::super_loop_record_t& loop, pid_t* out_pid) {
  if (!out_pid) return false;
  *out_pid = -1;
  std::string pid_text{};
  std::string ignored{};
  if (!cuwacunu::hero::runtime::read_text_file(super_loop_review_pid_path(loop),
                                               &pid_text, &ignored)) {
    return false;
  }
  pid_text = trim_ascii(pid_text);
  if (pid_text.empty()) return false;
  char* end = nullptr;
  const long parsed = std::strtol(pid_text.c_str(), &end, 10);
  if (end == nullptr || *end != '\0' || parsed <= 0) return false;
  *out_pid = static_cast<pid_t>(parsed);
  return true;
}

void cancel_loop_review_best_effort(
    const cuwacunu::hero::super::super_loop_record_t& loop, bool force,
    std::string* out_warning) {
  pid_t review_pid = -1;
  if (!read_loop_review_pid(loop, &review_pid)) return;
  const int sig = force ? SIGKILL : SIGTERM;
  errno = 0;
  const int rc_group = ::kill(-review_pid, sig);
  const int saved_errno = errno;
  errno = 0;
  const int rc_proc = ::kill(review_pid, sig);
  const int saved_errno_proc = errno;
  if (rc_group != 0 && rc_proc != 0 && saved_errno != ESRCH &&
      saved_errno_proc != ESRCH && out_warning) {
    append_warning_text(out_warning,
                        "review cancel degraded for pid=" +
                            std::to_string(static_cast<long long>(review_pid)));
  }
  remove_file_noexcept(super_loop_review_pid_path(loop));
}

[[nodiscard]] bool reload_terminal_super_loop_if_any(
    const app_context_t& app, std::string_view loop_id,
    cuwacunu::hero::super::super_loop_record_t* out_loop) {
  if (!out_loop) return false;
  std::string ignored{};
  if (!read_super_loop(app, loop_id, out_loop, &ignored)) return false;
  return cuwacunu::hero::super::is_super_loop_terminal_state(out_loop->state);
}

struct scoped_temp_path_t {
  std::filesystem::path path{};

  ~scoped_temp_path_t() {
    if (path.empty()) return;
    std::error_code ec{};
    std::filesystem::remove(path, ec);
  }
};

[[nodiscard]] bool write_temp_super_decision_schema(
    std::string_view schema_json, scoped_temp_path_t* out_path,
    std::string* error) {
  if (error) error->clear();
  if (!out_path) {
    if (error) *error = "temp schema output pointer is null";
    return false;
  }
  out_path->path.clear();
  std::filesystem::path temp_path =
      std::filesystem::temp_directory_path() /
      ("hero_super_decision_schema." +
       std::to_string(cuwacunu::hero::runtime::now_ms_utc()) + "." +
       std::to_string(::getpid()) + ".json");
  if (!cuwacunu::hero::runtime::write_text_file_atomic(temp_path, schema_json,
                                                       error)) {
    return false;
  }
  out_path->path = std::move(temp_path);
  return true;
}

[[nodiscard]] std::string runtime_job_to_json(
    const cuwacunu::hero::runtime::runtime_job_record_t& record,
    const cuwacunu::hero::runtime::runtime_job_observation_t& observation) {
  const std::string observed_state =
      cuwacunu::hero::runtime::stable_state_for_observation(record, observation);
  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(record.schema) << ","
      << "\"job_cursor\":" << json_quote(record.job_cursor) << ","
      << "\"job_kind\":" << json_quote(record.job_kind) << ","
      << "\"boot_id\":" << json_quote(record.boot_id) << ","
      << "\"state\":" << json_quote(record.state) << ","
      << "\"observed_state\":" << json_quote(observed_state) << ","
      << "\"state_detail\":" << json_quote(record.state_detail) << ","
      << "\"worker_binary\":" << json_quote(record.worker_binary) << ","
      << "\"worker_command\":" << json_quote(record.worker_command) << ","
      << "\"global_config_path\":"
      << json_quote(record.global_config_path) << ","
      << "\"source_campaign_dsl_path\":"
      << json_quote(record.source_campaign_dsl_path) << ","
      << "\"source_contract_dsl_path\":"
      << json_quote(record.source_contract_dsl_path) << ","
      << "\"source_wave_dsl_path\":"
      << json_quote(record.source_wave_dsl_path) << ","
      << "\"campaign_dsl_path\":" << json_quote(record.campaign_dsl_path) << ","
      << "\"contract_dsl_path\":" << json_quote(record.contract_dsl_path) << ","
      << "\"wave_dsl_path\":" << json_quote(record.wave_dsl_path) << ","
      << "\"binding_id\":" << json_quote(record.binding_id) << ","
      << "\"reset_runtime_state\":"
      << bool_json(record.reset_runtime_state) << ","
      << "\"stdout_path\":" << json_quote(record.stdout_path) << ","
      << "\"stderr_path\":" << json_quote(record.stderr_path) << ","
      << "\"trace_path\":" << json_quote(record.trace_path) << ","
      << "\"started_at_ms\":" << record.started_at_ms << ","
      << "\"updated_at_ms\":" << record.updated_at_ms << ","
      << "\"finished_at_ms\":"
      << (record.finished_at_ms.has_value()
              ? std::to_string(*record.finished_at_ms)
              : "null")
      << ",\"runner_pid\":"
      << (record.runner_pid.has_value() ? std::to_string(*record.runner_pid)
                                        : "null")
      << ",\"runner_start_ticks\":"
      << (record.runner_start_ticks.has_value()
              ? std::to_string(*record.runner_start_ticks)
              : "null")
      << ",\"target_pid\":"
      << (record.target_pid.has_value() ? std::to_string(*record.target_pid)
                                        : "null")
      << ",\"target_pgid\":"
      << (record.target_pgid.has_value() ? std::to_string(*record.target_pgid)
                                         : "null")
      << ",\"target_start_ticks\":"
      << (record.target_start_ticks.has_value()
              ? std::to_string(*record.target_start_ticks)
              : "null")
      << ",\"exit_code\":"
      << (record.exit_code.has_value() ? std::to_string(*record.exit_code)
                                       : "null")
      << ",\"term_signal\":"
      << (record.term_signal.has_value() ? std::to_string(*record.term_signal)
                                         : "null")
      << ",\"runner_alive\":" << bool_json(observation.runner_alive)
      << ",\"target_alive\":" << bool_json(observation.target_alive) << "}";
  return out.str();
}

[[nodiscard]] std::string runtime_campaign_to_json(
    const cuwacunu::hero::runtime::runtime_campaign_record_t& record) {
  const bool runner_alive =
      record.runner_pid.has_value() &&
      cuwacunu::hero::runtime::process_identity_alive(
          *record.runner_pid, record.runner_start_ticks, record.boot_id);
  std::ostringstream run_bind_ids_json;
  run_bind_ids_json << "[";
  for (std::size_t i = 0; i < record.run_bind_ids.size(); ++i) {
    if (i != 0) run_bind_ids_json << ",";
    run_bind_ids_json << json_quote(record.run_bind_ids[i]);
  }
  run_bind_ids_json << "]";

  std::ostringstream job_cursors_json;
  job_cursors_json << "[";
  for (std::size_t i = 0; i < record.job_cursors.size(); ++i) {
    if (i != 0) job_cursors_json << ",";
    job_cursors_json << json_quote(record.job_cursors[i]);
  }
  job_cursors_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(record.schema) << ","
      << "\"campaign_cursor\":" << json_quote(record.campaign_cursor) << ","
      << "\"boot_id\":" << json_quote(record.boot_id) << ","
      << "\"state\":" << json_quote(record.state) << ","
      << "\"state_detail\":" << json_quote(record.state_detail) << ","
      << "\"super_loop_id\":" << json_quote(record.super_loop_id) << ","
      << "\"global_config_path\":"
      << json_quote(record.global_config_path) << ","
      << "\"source_campaign_dsl_path\":"
      << json_quote(record.source_campaign_dsl_path) << ","
      << "\"campaign_dsl_path\":" << json_quote(record.campaign_dsl_path) << ","
      << "\"reset_runtime_state\":"
      << bool_json(record.reset_runtime_state) << ","
      << "\"stdout_path\":" << json_quote(record.stdout_path) << ","
      << "\"stderr_path\":" << json_quote(record.stderr_path) << ","
      << "\"started_at_ms\":" << record.started_at_ms << ","
      << "\"updated_at_ms\":" << record.updated_at_ms << ","
      << "\"finished_at_ms\":"
      << (record.finished_at_ms.has_value()
              ? std::to_string(*record.finished_at_ms)
              : "null")
      << ",\"runner_pid\":"
      << (record.runner_pid.has_value() ? std::to_string(*record.runner_pid)
                                        : "null")
      << ",\"runner_start_ticks\":"
      << (record.runner_start_ticks.has_value()
              ? std::to_string(*record.runner_start_ticks)
              : "null")
      << ",\"current_run_index\":"
      << (record.current_run_index.has_value()
              ? std::to_string(*record.current_run_index)
              : "null")
      << ",\"runner_alive\":" << bool_json(runner_alive)
      << ",\"run_bind_ids\":" << run_bind_ids_json.str()
      << ",\"job_cursors\":" << job_cursors_json.str() << "}";
  return out.str();
}

[[nodiscard]] std::string super_loop_to_json(
    const cuwacunu::hero::super::super_loop_record_t& record) {
  std::ostringstream campaign_cursors_json;
  campaign_cursors_json << "[";
  for (std::size_t i = 0; i < record.campaign_cursors.size(); ++i) {
    if (i != 0) campaign_cursors_json << ",";
    campaign_cursors_json << json_quote(record.campaign_cursors[i]);
  }
  campaign_cursors_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(record.schema) << ","
      << "\"loop_id\":" << json_quote(record.loop_id) << ","
      << "\"state\":" << json_quote(record.state) << ","
      << "\"state_detail\":" << json_quote(record.state_detail) << ","
      << "\"objective_name\":" << json_quote(record.objective_name) << ","
      << "\"global_config_path\":" << json_quote(record.global_config_path) << ","
      << "\"source_super_objective_dsl_path\":"
      << json_quote(record.source_super_objective_dsl_path) << ","
      << "\"source_campaign_dsl_path\":"
      << json_quote(record.source_campaign_dsl_path) << ","
      << "\"source_super_objective_md_path\":"
      << json_quote(record.source_super_objective_md_path) << ","
      << "\"source_super_guidance_md_path\":"
      << json_quote(record.source_super_guidance_md_path) << ","
      << "\"loop_root\":" << json_quote(record.loop_root) << ","
      << "\"objective_root\":" << json_quote(record.objective_root) << ","
      << "\"campaign_dsl_path\":" << json_quote(record.campaign_dsl_path) << ","
      << "\"super_objective_dsl_path\":"
      << json_quote(record.super_objective_dsl_path) << ","
      << "\"super_objective_md_path\":"
      << json_quote(record.super_objective_md_path) << ","
      << "\"super_guidance_md_path\":"
      << json_quote(record.super_guidance_md_path) << ","
      << "\"config_policy_path\":"
      << json_quote(record.config_policy_path) << ","
      << "\"briefing_path\":" << json_quote(record.briefing_path) << ","
      << "\"memory_path\":" << json_quote(record.memory_path) << ","
      << "\"codex_session_id\":"
      << json_quote(record.codex_session_id) << ","
      << "\"human_request_path\":"
      << json_quote(record.human_request_path) << ","
      << "\"human_response_path\":"
      << json_quote(
             cuwacunu::hero::super::super_loop_human_response_latest_path(
                 std::filesystem::path(record.loop_root).parent_path(),
                 record.loop_id)
                 .string())
      << ",\"human_response_sig_path\":"
      << json_quote(
             cuwacunu::hero::super::super_loop_human_response_latest_sig_path(
                 std::filesystem::path(record.loop_root).parent_path(),
                 record.loop_id)
                 .string())
      << ","
      << "\"events_path\":" << json_quote(record.events_path) << ","
      << "\"started_at_ms\":" << record.started_at_ms << ","
      << "\"updated_at_ms\":" << record.updated_at_ms << ","
      << "\"finished_at_ms\":"
      << (record.finished_at_ms.has_value()
              ? std::to_string(*record.finished_at_ms)
              : "null")
      << ",\"review_count\":" << record.review_count
      << ",\"max_reviews\":" << record.max_reviews
      << ",\"active_campaign_cursor\":"
      << json_quote(record.active_campaign_cursor) << ","
      << "\"last_control_kind\":"
      << json_quote(record.last_control_kind) << ","
      << "\"last_warning\":" << json_quote(record.last_warning) << ","
      << "\"last_review_packet_path\":"
      << json_quote(record.last_review_packet_path) << ","
      << "\"last_decision_path\":"
      << json_quote(record.last_decision_path) << ","
      << "\"campaign_cursors\":" << campaign_cursors_json.str() << "}";
  return out.str();
}

[[nodiscard]] bool append_super_loop_event(
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::string_view event, std::string_view detail, std::string* error) {
  if (error) error->clear();
  const std::string payload =
      "{\"timestamp_ms\":" +
      std::to_string(cuwacunu::hero::runtime::now_ms_utc()) +
      ",\"loop_id\":" + json_quote(loop.loop_id) + ",\"event\":" +
      json_quote(event) + ",\"detail\":" + json_quote(detail) + "}\n";
  return cuwacunu::hero::runtime::append_text_file(loop.events_path, payload,
                                                   error);
}

[[nodiscard]] bool read_super_loop(
    const app_context_t& app, std::string_view loop_id,
    cuwacunu::hero::super::super_loop_record_t* out,
    std::string* error) {
  return cuwacunu::hero::super::read_super_loop_record(
      app.defaults.super_root, loop_id, out, error);
}

[[nodiscard]] bool write_super_loop(
    const app_context_t& app,
    const cuwacunu::hero::super::super_loop_record_t& record,
    std::string* error) {
  if (!cuwacunu::hero::super::write_super_loop_record(app.defaults.super_root,
                                                      record, error)) {
    return false;
  }
  std::string marker_error{};
  if (!cuwacunu::hero::super::sync_human_pending_request_count(
          app.defaults.super_root, &marker_error)) {
    std::cerr << "[hero_super_mcp][warning] failed to refresh Human Hero "
                 "pending marker: "
              << marker_error << std::endl;
  }
  return true;
}

[[nodiscard]] bool list_super_loops(
    const app_context_t& app,
    std::vector<cuwacunu::hero::super::super_loop_record_t>* out,
    std::string* error) {
  return cuwacunu::hero::super::scan_super_loop_records(
      app.defaults.super_root, out, error);
}

[[nodiscard]] bool read_runtime_campaign_direct(
    const app_context_t& app, std::string_view campaign_cursor,
    cuwacunu::hero::runtime::runtime_campaign_record_t* out,
    std::string* error) {
  return cuwacunu::hero::runtime::read_runtime_campaign_record(
      campaigns_root_for_app(app), campaign_cursor, out, error);
}

[[nodiscard]] bool read_runtime_job_direct(
    const app_context_t& app, std::string_view job_cursor,
    cuwacunu::hero::runtime::runtime_job_record_t* out, std::string* error) {
  return cuwacunu::hero::runtime::read_runtime_job_record(
      campaigns_root_for_app(app), job_cursor, out, error);
}

[[nodiscard]] std::filesystem::path runtime_job_trace_path_for_record(
    const app_context_t& app,
    const cuwacunu::hero::runtime::runtime_job_record_t& record) {
  if (!record.trace_path.empty()) return std::filesystem::path(record.trace_path);
  return cuwacunu::hero::runtime::runtime_job_trace_path(
      campaigns_root_for_app(app), record.job_cursor);
}

[[nodiscard]] bool is_runtime_campaign_terminal_state(std::string_view state) {
  return state != "launching" && state != "running" && state != "stopping";
}

[[nodiscard]] bool is_super_loop_terminal_state(std::string_view state) {
  return cuwacunu::hero::super::is_super_loop_terminal_state(state);
}

[[nodiscard]] std::string observed_campaign_state(
    const cuwacunu::hero::runtime::runtime_campaign_record_t& record) {
  const bool runner_alive =
      record.runner_pid.has_value() &&
      cuwacunu::hero::runtime::process_identity_alive(
          *record.runner_pid, record.runner_start_ticks, record.boot_id);
  if (record.state == "launching" || record.state == "running" ||
      record.state == "stopping") {
    if (runner_alive) {
      return (record.state == "launching") ? "running" : record.state;
    }
    return "failed";
  }
  return record.state;
}

[[nodiscard]] bool tail_file_lines(const std::filesystem::path& path,
                                   std::size_t lines, std::string* out,
                                   std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "tail output pointer is null";
    return false;
  }
  *out = "";
  std::error_code ec{};
  if (!std::filesystem::exists(path, ec)) {
    if (ec) {
      if (error) *error = "cannot access log file: " + path.string();
      return false;
    }
    return true;
  }
  std::string text{};
  if (!cuwacunu::hero::runtime::read_text_file(path, &text, error)) return false;
  if (lines == 0) {
    *out = text;
    return true;
  }
  std::size_t count = 0;
  std::size_t pos = text.size();
  while (pos > 0) {
    --pos;
    if (text[pos] == '\n') {
      ++count;
      if (count > lines) {
        ++pos;
        break;
      }
    }
  }
  if (count <= lines) pos = 0;
  *out = text.substr(pos);
  return true;
}

[[nodiscard]] bool read_kv_file(
    const std::filesystem::path& path,
    std::unordered_map<std::string, std::string>* out, std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "kv output pointer is null";
    return false;
  }
  out->clear();
  std::string text{};
  if (!cuwacunu::hero::runtime::read_text_file(path, &text, error)) return false;
  std::istringstream in(text);
  std::string line{};
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(line);
    if (trimmed.empty()) continue;
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos) continue;
    (*out)[trim_ascii(trimmed.substr(0, eq))] =
        trim_ascii(trimmed.substr(eq + 1));
  }
  return true;
}

[[nodiscard]] std::string extract_logged_hash_value(std::string_view text,
                                                    std::string_view key) {
  const std::size_t pos = text.rfind(key);
  if (pos == std::string::npos) return {};
  const std::size_t start = pos + key.size();
  std::size_t end = start;
  while (end < text.size()) {
    const char c = text[end];
    if (!(std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' ||
          c == '-' || c == '.')) {
      break;
    }
    ++end;
  }
  return std::string(text.substr(start, end - start));
}

[[nodiscard]] std::vector<std::string> unique_nonempty_strings(
    std::vector<std::string> values) {
  for (std::string& value : values) value = trim_ascii(value);
  values.erase(std::remove_if(values.begin(), values.end(),
                              [](const std::string& value) {
                                return value.empty();
                              }),
               values.end());
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  return values;
}

[[nodiscard]] std::vector<run_manifest_hint_t> collect_run_manifest_hints(
    const app_context_t& app, const std::vector<std::string>& binding_ids,
    std::size_t per_binding_limit) {
  const std::filesystem::path runtime_root = app.defaults.super_root.parent_path();
  const std::filesystem::path runs_root =
      runtime_root / ".hashimyei" / "_meta" / "runs";
  std::vector<run_manifest_hint_t> out{};
  std::error_code ec{};
  if (!std::filesystem::exists(runs_root, ec) || ec) return out;

  for (const std::string& binding_id : binding_ids) {
    std::vector<run_manifest_hint_t> binding_rows{};
    for (const auto& it : std::filesystem::directory_iterator(runs_root, ec)) {
      if (ec) break;
      if (!it.is_directory(ec) || ec) continue;
      const std::string dirname = it.path().filename().string();
      if (dirname.find("." + binding_id + ".") == std::string::npos) continue;
      const std::filesystem::path manifest = it.path() / "run.manifest.v2.kv";
      if (!std::filesystem::exists(manifest, ec) || ec) continue;
      std::unordered_map<std::string, std::string> kv{};
      std::string ignored{};
      if (!read_kv_file(manifest, &kv, &ignored)) continue;
      run_manifest_hint_t hint{};
      hint.path = manifest;
      hint.run_id = kv["run_id"];
      hint.binding_id = kv["wave_contract_binding.binding_id"];
      hint.contract_hash = kv["wave_contract_binding.contract.hash_sha256_hex"];
      hint.wave_hash = kv["wave_contract_binding.wave.hash_sha256_hex"];
      (void)cuwacunu::hero::runtime::parse_u64(kv["started_at_ms"],
                                               &hint.started_at_ms);
      binding_rows.push_back(std::move(hint));
    }
    std::sort(binding_rows.begin(), binding_rows.end(),
              [](const auto& lhs, const auto& rhs) {
                if (lhs.started_at_ms != rhs.started_at_ms) {
                  return lhs.started_at_ms > rhs.started_at_ms;
                }
                return lhs.path.string() > rhs.path.string();
              });
    if (binding_rows.size() > per_binding_limit) {
      binding_rows.resize(per_binding_limit);
    }
    out.insert(out.end(), binding_rows.begin(), binding_rows.end());
  }
  return out;
}

[[nodiscard]] std::string build_lattice_recommendations_json(
    const std::vector<std::string>& contract_hash_candidates) {
  std::ostringstream view_queries_json;
  view_queries_json << "[";
  for (std::size_t i = 0; i < contract_hash_candidates.size(); ++i) {
    if (i != 0) view_queries_json << ",";
    view_queries_json << "{"
                      << "\"tool\":\"hero.lattice.get_view\","
                      << "\"arguments\":{"
                      << "\"view_kind\":\"family_evaluation_report\","
                      << "\"canonical_path\":"
                      << json_quote("<family canonical_path>")
                      << ",\"contract_hash\":"
                      << json_quote(contract_hash_candidates[i]) << "},"
                      << "\"when_to_use\":"
                      << json_quote(
                             "Use for semantic family-level evaluation evidence "
                             "once you know the family canonical_path selector.")
                      << ",\"notes\":"
                      << json_quote(
                             "family_evaluation_report requires a family "
                             "canonical_path selector plus contract_hash. If the "
                             "family selector is unknown, discover it with "
                             "hero.lattice.list_facts first.")
                      << "}";
  }
  view_queries_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"preferred_evidence_order\":["
      << json_quote("review_packet")
      << "," << json_quote("hero.lattice.get_view_or_get_fact")
      << "," << json_quote("hero.runtime_operational_debugging")
      << "," << json_quote("direct_file_reads_as_fallback")
      << "],"
      << "\"semantic_preference\":"
      << json_quote(
             "Prefer Lattice facts/views when judging report quality or "
             "comparing semantic evidence. Use Runtime tails primarily for "
             "operational debugging such as launch failures, missing logs, or "
             "abnormal traces.")
      << ",\"fact_discovery_queries\":["
      << "{"
      << "\"tool\":\"hero.lattice.list_views\","
      << "\"arguments\":{},"
      << "\"when_to_use\":"
      << json_quote(
             "Use when you need to confirm available derived view kinds and "
             "their selectors.")
      << "},"
      << "{"
      << "\"tool\":\"hero.lattice.list_facts\","
      << "\"arguments\":{},"
      << "\"when_to_use\":"
      << json_quote(
             "Use when you need to discover canonical_path selectors available "
             "in the current catalog before choosing one concrete fact/view query.")
      << "},"
      << "{"
      << "\"tool\":\"hero.lattice.get_fact\","
      << "\"arguments\":{\"canonical_path\":"
      << json_quote("<component canonical_path>") << "},"
      << "\"when_to_use\":"
      << json_quote(
             "Use after you know a component canonical_path and want one "
             "assembled fact bundle rather than raw report fragments.")
      << "}"
      << "],"
      << "\"family_evaluation_report_queries\":" << view_queries_json.str()
      << ",\"contract_hash_candidates\":"
      << json_array_from_strings(contract_hash_candidates)
      << ",\"selector_guidance\":"
      << json_quote(
             "For family_evaluation_report, canonical_path should be the family "
             "selector without a hashimyei suffix. If you do not already know "
             "that selector from the objective, use hero.lattice.list_facts to "
             "discover it.")
      << "}";
  return out.str();
}

[[nodiscard]] bool rewrite_campaign_imports_absolute(
    const std::filesystem::path& source_campaign_path,
    std::string_view input_text, std::string* out_text, std::string* error) {
  if (error) error->clear();
  if (!out_text) {
    if (error) *error = "campaign snapshot output pointer is null";
    return false;
  }
  *out_text = "";

  bool in_block_comment = false;
  std::ostringstream out;
  std::size_t pos = 0;
  bool first = true;
  while (pos <= input_text.size()) {
    const std::size_t end = input_text.find('\n', pos);
    std::string line = (end == std::string::npos)
                           ? std::string(input_text.substr(pos))
                           : std::string(input_text.substr(pos, end - pos));

    const auto rewrite_import =
        [&](std::string_view token, const std::string& current_line) {
          std::size_t token_pos = current_line.find(token);
          if (token_pos == std::string::npos) return current_line;
          const std::size_t quote_begin =
              current_line.find('"', token_pos + token.size());
          if (quote_begin == std::string::npos) return current_line;
          const std::size_t quote_end = current_line.find('"', quote_begin + 1);
          if (quote_end == std::string::npos) return current_line;
          const std::filesystem::path raw_path(
              current_line.substr(quote_begin + 1,
                                  quote_end - quote_begin - 1));
          if (raw_path.is_absolute()) return current_line;
          const std::filesystem::path rewritten =
              (source_campaign_path.parent_path() / raw_path).lexically_normal();
          return current_line.substr(0, quote_begin + 1) + rewritten.string() +
                 current_line.substr(quote_end);
        };

    if (!in_block_comment) {
      line = rewrite_import("IMPORT_CONTRACT", line);
      line = rewrite_import("FROM", line);
      line = rewrite_import("IMPORT_CONTRACT_FILE", line);
      line = rewrite_import("IMPORT_WAVE_FILE", line);
      line = rewrite_import("SUPER", line);
    }
    if (!first) out << "\n";
    first = false;
    out << line;
    in_block_comment =
        cuwacunu::hero::wave_contract_binding_runtime::detail::
            next_block_comment_state(line, in_block_comment);
    if (end == std::string::npos) break;
    pos = end + 1;
  }
  *out_text = out.str();
  return true;
}

[[nodiscard]] bool decode_campaign_snapshot(
    const app_context_t& app, const std::string& campaign_dsl_path,
    cuwacunu::camahjucunu::iitepi_campaign_instruction_t* out_instruction,
    std::string* error) {
  if (error) error->clear();
  if (!out_instruction) {
    if (error) *error = "campaign decode output pointer is null";
    return false;
  }
  std::string grammar_text{};
  if (!cuwacunu::hero::runtime::read_text_file(app.defaults.campaign_grammar_path,
                                               &grammar_text, error)) {
    return false;
  }
  std::string campaign_text{};
  if (!cuwacunu::hero::runtime::read_text_file(campaign_dsl_path, &campaign_text,
                                               error)) {
    return false;
  }
  try {
    *out_instruction =
        cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
            grammar_text, campaign_text);
  } catch (const std::exception& e) {
    if (error) {
      *error = "failed to decode campaign DSL snapshot '" + campaign_dsl_path +
               "': " + e.what();
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool decode_super_objective_snapshot(
    const app_context_t& app, const std::string& super_objective_dsl_path,
    super_objective_spec_t* out_spec, std::string* error) {
  if (error) error->clear();
  if (!out_spec) {
    if (error) *error = "super objective decode output pointer is null";
    return false;
  }
  *out_spec = super_objective_spec_t{};

  std::string grammar_text{};
  if (!cuwacunu::hero::runtime::read_text_file(
          app.defaults.super_objective_grammar_path, &grammar_text, error)) {
    return false;
  }
  std::string objective_text{};
  if (!cuwacunu::hero::runtime::read_text_file(super_objective_dsl_path,
                                               &objective_text, error)) {
    return false;
  }

  try {
    const auto decoded =
        cuwacunu::camahjucunu::dsl::decode_super_objective_from_dsl(
            grammar_text, objective_text);
    out_spec->campaign_dsl_path = trim_ascii(decoded.campaign_dsl_path);
    out_spec->objective_md_path = trim_ascii(decoded.objective_md_path);
    out_spec->guidance_md_path = trim_ascii(decoded.guidance_md_path);
    out_spec->objective_name = trim_ascii(decoded.objective_name);
    out_spec->loop_id = trim_ascii(decoded.loop_id);
  } catch (const std::exception& e) {
    if (error) {
      *error = "failed to decode super objective DSL '" +
               super_objective_dsl_path + "': " + e.what();
    }
    return false;
  }

  if (out_spec->campaign_dsl_path.empty()) {
    if (error) {
      *error = "super objective '" + super_objective_dsl_path +
               "' is missing required campaign_dsl_path";
    }
    return false;
  }
  if (out_spec->objective_md_path.empty()) {
    if (error) {
      *error = "super objective '" + super_objective_dsl_path +
               "' is missing required objective_md_path";
    }
    return false;
  }
  if (out_spec->guidance_md_path.empty()) {
    if (error) {
      *error = "super objective '" + super_objective_dsl_path +
               "' is missing required guidance_md_path";
    }
    return false;
  }
  return true;
}

[[nodiscard]] std::filesystem::path
resolve_requested_super_objective_source_path(
    const app_context_t& app, std::string requested_super_objective_dsl_path,
    std::string* error) {
  if (error) error->clear();
  requested_super_objective_dsl_path =
      trim_ascii(std::move(requested_super_objective_dsl_path));
  const std::filesystem::path source_super_objective_dsl_path =
      requested_super_objective_dsl_path.empty()
          ? resolve_default_super_objective_dsl_path(app.global_config_path, error)
          : std::filesystem::path(resolve_path_from_base_folder(
                app.global_config_path.parent_path().string(),
                requested_super_objective_dsl_path));
  if (source_super_objective_dsl_path.empty()) return {};

  std::error_code ec{};
  if (!std::filesystem::exists(source_super_objective_dsl_path, ec) ||
      !std::filesystem::is_regular_file(source_super_objective_dsl_path, ec)) {
    if (error) {
      *error = "super objective DSL does not exist: " +
               source_super_objective_dsl_path.string();
    }
    return {};
  }
  if (lowercase_copy(source_super_objective_dsl_path.extension().string()) !=
      ".dsl") {
    if (error) {
      *error = "super objective path must target a .dsl file: " +
               source_super_objective_dsl_path.string();
    }
    return {};
  }
  return source_super_objective_dsl_path;
}

[[nodiscard]] bool resolve_super_objective_member_source_path(
    const std::filesystem::path& source_super_objective_dsl_path,
    std::string_view raw_path, std::string_view field_name,
    std::filesystem::path* out_path, std::string* error) {
  if (error) error->clear();
  if (!out_path) {
    if (error) *error = "super objective member output pointer is null";
    return false;
  }
  out_path->clear();
  const std::string trimmed = trim_ascii(raw_path);
  if (trimmed.empty()) {
    if (error) {
      *error = "super objective '" + source_super_objective_dsl_path.string() +
               "' is missing required " + std::string(field_name);
    }
    return false;
  }

  std::filesystem::path resolved(trimmed);
  if (!resolved.is_absolute()) {
    resolved =
        (source_super_objective_dsl_path.parent_path() / resolved).lexically_normal();
  }
  const bool inside_objective_bundle =
      path_is_within(source_super_objective_dsl_path.parent_path(), resolved);
  const bool allow_shared_guidance =
      field_name == "guidance_md_path" &&
      path_is_within(source_super_objective_dsl_path.parent_path().parent_path().parent_path(),
                     resolved);
  if (!inside_objective_bundle && !allow_shared_guidance) {
    if (error) {
      *error = "super objective field " + std::string(field_name) +
               " escapes its objective bundle: " + resolved.string();
    }
    return false;
  }
  std::error_code ec{};
  if (!std::filesystem::exists(resolved, ec) ||
      !std::filesystem::is_regular_file(resolved, ec)) {
    if (error) {
      *error = "super objective field " + std::string(field_name) +
               " does not exist: " + resolved.string();
    }
    return false;
  }
  *out_path = resolved;
  return true;
}

[[nodiscard]] super_loop_workspace_context_t
make_super_loop_workspace_context(const app_context_t& app) {
  super_loop_workspace_context_t context{};
  context.global_config_path = app.global_config_path;
  context.self_binary_path = app.self_binary_path;
  context.super_root = app.defaults.super_root;
  context.repo_root = app.defaults.repo_root;
  context.config_scope_root = app.defaults.config_scope_root;
  return context;
}

[[nodiscard]] std::filesystem::path runtime_tool_io_dir(const app_context_t& app) {
  return app.defaults.super_root / ".tool_io";
}

[[nodiscard]] std::filesystem::path make_runtime_tool_io_path(
    const app_context_t& app, std::string_view stem) {
  const std::uint64_t salt =
      static_cast<std::uint64_t>(
          std::chrono::steady_clock::now().time_since_epoch().count());
  return runtime_tool_io_dir(app) /
         ("runtime_tool." + std::to_string(cuwacunu::hero::runtime::now_ms_utc()) +
          "." + std::to_string(static_cast<unsigned long long>(::getpid())) + "." +
          cuwacunu::hero::runtime::hex_lower_u64(salt).substr(0, 8) + "." +
          std::string(stem));
}

void cleanup_runtime_tool_io(const std::filesystem::path& stdin_path,
                             const std::filesystem::path& stdout_path,
                             const std::filesystem::path& stderr_path) {
  std::error_code ec{};
  (void)std::filesystem::remove(stdin_path, ec);
  (void)std::filesystem::remove(stdout_path, ec);
  (void)std::filesystem::remove(stderr_path, ec);
}

[[nodiscard]] bool call_runtime_tool(const app_context_t& app,
                                     const std::string& tool_name,
                                     std::string arguments_json,
                                     std::string* out_structured,
                                     std::string* error) {
  if (error) error->clear();
  if (!out_structured) {
    if (error) *error = "runtime structured output pointer is null";
    return false;
  }
  *out_structured = "";

  std::error_code ec{};
  std::filesystem::create_directories(runtime_tool_io_dir(app), ec);
  if (ec) {
    if (error) {
      *error = "cannot create runtime tool io dir: " +
               runtime_tool_io_dir(app).string();
    }
    return false;
  }

  const std::filesystem::path stdin_path =
      make_runtime_tool_io_path(app, "stdin.json");
  const std::filesystem::path stdout_path =
      make_runtime_tool_io_path(app, "stdout.json");
  const std::filesystem::path stderr_path =
      make_runtime_tool_io_path(app, "stderr.log");
  if (!cuwacunu::hero::runtime::write_text_file_atomic(stdin_path, "", error)) {
    return false;
  }

  const std::vector<std::string> argv{
      app.defaults.runtime_hero_binary.string(), "--global-config",
      app.global_config_path.string(), "--tool", tool_name, "--args-json",
      arguments_json};
  int exit_code = -1;
  std::string invoke_error{};
  const bool invoked = run_command_with_stdio_and_timeout(
      argv, stdin_path, stdout_path, stderr_path, nullptr, nullptr,
      kRuntimeToolTimeoutSec, nullptr, &exit_code, &invoke_error);

  std::string stdout_text{};
  std::string stderr_text{};
  std::string ignored{};
  (void)cuwacunu::hero::runtime::read_text_file(stdout_path, &stdout_text, &ignored);
  (void)cuwacunu::hero::runtime::read_text_file(stderr_path, &stderr_text, &ignored);
  cleanup_runtime_tool_io(stdin_path, stdout_path, stderr_path);

  if (!invoked && stdout_text.empty()) {
    if (error) {
      *error = !trim_ascii(stderr_text).empty() ? trim_ascii(stderr_text)
                                                : invoke_error;
    }
    return false;
  }
  if (stdout_text.empty()) {
    if (error) {
      *error = !trim_ascii(stderr_text).empty()
                   ? trim_ascii(stderr_text)
                   : ("runtime tool call produced no stdout: " + tool_name);
    }
    return false;
  }
  if (!tool_result_structured_content(stdout_text, out_structured, error)) {
    return false;
  }
  if (exit_code != 0) {
    if (error) {
      *error = !trim_ascii(stderr_text).empty()
                   ? trim_ascii(stderr_text)
                   : ("runtime tool exited non-zero: " + tool_name);
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool initialize_super_loop_record(
    const app_context_t& app,
    const std::filesystem::path& source_super_objective_dsl_path,
    cuwacunu::hero::super::super_loop_record_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "super loop output pointer is null";
    return false;
  }
  *out = cuwacunu::hero::super::super_loop_record_t{};

  std::error_code ec{};
  std::filesystem::create_directories(app.defaults.super_root, ec);
  if (ec) {
    if (error) {
      *error = "cannot create .super_hero root: " +
               app.defaults.super_root.string();
    }
    return false;
  }

  super_objective_spec_t super_objective_spec{};
  if (!decode_super_objective_snapshot(app, source_super_objective_dsl_path.string(),
                                       &super_objective_spec, error)) {
    return false;
  }
  const std::string loop_id =
      trim_ascii(super_objective_spec.loop_id).empty()
          ? cuwacunu::hero::super::make_super_loop_id(app.defaults.super_root)
          : trim_ascii(super_objective_spec.loop_id);
  if (!is_valid_super_loop_id(loop_id)) {
    if (error) {
      *error = "super objective loop_id is invalid; use only [A-Za-z0-9._-]: " +
               loop_id;
    }
    return false;
  }
  std::filesystem::path source_campaign_path{};
  if (!resolve_super_objective_member_source_path(
          source_super_objective_dsl_path, super_objective_spec.campaign_dsl_path,
          "campaign_dsl_path", &source_campaign_path, error)) {
    return false;
  }
  std::filesystem::path source_super_objective_md_path{};
  if (!resolve_super_objective_member_source_path(
          source_super_objective_dsl_path, super_objective_spec.objective_md_path,
          "objective_md_path", &source_super_objective_md_path, error)) {
    return false;
  }
  std::filesystem::path source_super_guidance_md_path{};
  if (!resolve_super_objective_member_source_path(
          source_super_objective_dsl_path, super_objective_spec.guidance_md_path,
          "guidance_md_path", &source_super_guidance_md_path, error)) {
    return false;
  }
  cuwacunu::camahjucunu::iitepi_campaign_instruction_t ignored_campaign{};
  if (!decode_campaign_snapshot(app, source_campaign_path.string(),
                                &ignored_campaign, error)) {
    return false;
  }
  {
    std::vector<cuwacunu::hero::super::super_loop_record_t> existing_loops{};
    std::string scan_error{};
    if (!cuwacunu::hero::super::scan_super_loop_records(
            app.defaults.super_root, &existing_loops, &scan_error)) {
      if (error) *error = scan_error;
      return false;
    }
    const std::filesystem::path normalized_source_super_objective =
        source_super_objective_dsl_path.lexically_normal();
    for (const auto& existing : existing_loops) {
      if (cuwacunu::hero::super::is_super_loop_terminal_state(existing.state)) {
        continue;
      }
      if (std::filesystem::path(existing.source_super_objective_dsl_path)
              .lexically_normal() == normalized_source_super_objective) {
        if (error) {
          *error =
              "another active super loop already owns this super objective: " +
              existing.loop_id;
        }
        return false;
      }
    }
  }
  const std::filesystem::path loop_root =
      cuwacunu::hero::super::super_loop_dir(app.defaults.super_root,
                                                      loop_id);
  if (std::filesystem::exists(loop_root, ec) && !ec) {
    if (error) *error = "super loop already exists: " + loop_id;
    return false;
  }
  std::filesystem::create_directories(loop_root, ec);
  if (ec) {
    if (error) *error = "cannot create super loop dir: " + loop_root.string();
    return false;
  }

  const super_loop_workspace_context_t workspace_context =
      make_super_loop_workspace_context(app);
  const std::filesystem::path objective_root =
      source_super_objective_dsl_path.parent_path().lexically_normal();

  cuwacunu::hero::super::super_loop_record_t loop{};
  loop.loop_id = loop_id;
  loop.state = "launching";
  loop.state_detail = "initializing super loop";
  loop.objective_name =
      trim_ascii(super_objective_spec.objective_name).empty()
          ? derive_super_loop_objective_name(source_super_objective_dsl_path)
          : trim_ascii(super_objective_spec.objective_name);
  loop.global_config_path = app.global_config_path.string();
  loop.source_super_objective_dsl_path = source_super_objective_dsl_path.string();
  loop.source_campaign_dsl_path = source_campaign_path.string();
  loop.source_super_objective_md_path = source_super_objective_md_path.string();
  loop.source_super_guidance_md_path = source_super_guidance_md_path.string();
  loop.loop_root = loop_root.string();
  loop.objective_root = objective_root.string();
  loop.campaign_dsl_path = source_campaign_path.lexically_normal().string();
  loop.super_objective_dsl_path =
      cuwacunu::hero::super::super_loop_objective_dsl_path(
          app.defaults.super_root, loop_id)
          .string();
  loop.super_objective_md_path =
      cuwacunu::hero::super::super_loop_objective_md_path(
          app.defaults.super_root, loop_id)
          .string();
  loop.super_guidance_md_path =
      cuwacunu::hero::super::super_loop_guidance_md_path(
          app.defaults.super_root, loop_id)
          .string();
  loop.config_policy_path =
      cuwacunu::hero::super::super_loop_config_policy_path(
          app.defaults.super_root, loop_id)
          .string();
  loop.briefing_path =
      cuwacunu::hero::super::super_loop_briefing_path(
          app.defaults.super_root, loop_id)
          .string();
  loop.memory_path =
      cuwacunu::hero::super::super_loop_memory_path(app.defaults.super_root,
                                                              loop_id)
          .string();
  loop.human_request_path =
      cuwacunu::hero::super::super_loop_human_request_path(
          app.defaults.super_root, loop_id)
          .string();
  loop.events_path =
      cuwacunu::hero::super::super_loop_events_path(app.defaults.super_root,
                                                              loop_id)
          .string();
  loop.started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop.updated_at_ms = loop.started_at_ms;
  loop.max_reviews = app.defaults.super_max_reviews;

  if (!write_super_loop_bootstrap_files(
          workspace_context, source_super_objective_dsl_path,
          source_super_objective_md_path, source_super_guidance_md_path, loop,
          error)) {
    return false;
  }
  if (!write_super_loop(app, loop, error)) return false;
  if (!append_super_loop_event(loop, "loop_created",
                               "super loop initialized from super objective source",
                               error)) {
    return false;
  }
  *out = std::move(loop);
  return true;
}

void append_memory_note(
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::uint64_t review_index, std::string_view note) {
  const std::string trimmed = trim_ascii(note);
  if (trimmed.empty()) return;
  std::ostringstream out;
  out << "\n\n## Review " << review_index << "\n\n" << trimmed << "\n";
  std::string ignored{};
  (void)cuwacunu::hero::runtime::append_text_file(loop.memory_path, out.str(),
                                                  &ignored);
}

void append_warning_text(std::string* dst, std::string_view warning) {
  if (!dst) return;
  const std::string trimmed = trim_ascii(warning);
  if (trimmed.empty()) return;
  if (!dst->empty()) dst->append("; ");
  dst->append(trimmed);
}

void append_structured_warnings(const std::string& structured_json,
                                std::string_view prefix,
                                std::vector<std::string>* out) {
  if (!out) return;
  std::vector<std::string> warnings{};
  if (!extract_json_string_array_field(structured_json, "warnings", &warnings)) {
    return;
  }
  for (const std::string& warning : warnings) {
    const std::string trimmed = trim_ascii(warning);
    if (trimmed.empty()) continue;
    out->push_back(trim_ascii(std::string(prefix) + trimmed));
  }
}

void persist_super_loop_warning_best_effort(
    const app_context_t& app,
    cuwacunu::hero::super::super_loop_record_t* loop,
    std::string_view warning) {
  if (!loop) return;
  const std::string trimmed = trim_ascii(warning);
  if (trimmed.empty()) return;
  loop->last_warning = trimmed;
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  std::string ignored{};
  (void)write_super_loop(app, *loop, &ignored);
  std::ostringstream note;
  note << "\n\n## Runtime Warning\n\n" << trimmed << "\n";
  (void)cuwacunu::hero::runtime::append_text_file(loop->memory_path, note.str(),
                                                  &ignored);
}

[[nodiscard]] bool write_human_request_note(
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::uint64_t review_index, const super_review_decision_t& decision,
    std::string* error) {
  if (error) error->clear();
  const std::string request = trim_ascii(
      decision.human_request.empty() ? decision.reason : decision.human_request);
  std::ostringstream out;
  out << "# Human Review Requested\n\n"
      << "Loop ID: " << loop.loop_id << "\n"
      << "Review: " << review_index << "\n"
      << "Control: " << decision.control_kind << "\n\n"
      << "Reason:\n" << decision.reason << "\n\n"
      << "Operator request:\n" << request << "\n\n"
      << "Key files:\n"
      << "- Loop manifest: " << loop.loop_root << "/loop.lls\n"
      << "- Review packet: " << super_loop_latest_review_packet_path(loop).string()
      << "\n"
      << "- Decision: " << super_loop_latest_decision_path(loop).string() << "\n"
      << "- Memory: " << loop.memory_path << "\n"
      << "- Events: " << loop.events_path << "\n";
  return cuwacunu::hero::runtime::write_text_file_atomic(loop.human_request_path,
                                                         out.str(), error);
}

[[nodiscard]] super_review_decision_t human_response_to_super_decision(
    const cuwacunu::hero::human::human_response_record_t& response) {
  super_review_decision_t decision{};
  decision.control_kind = response.control_kind;
  decision.next_action_kind = response.next_action_kind;
  decision.target_binding_id = response.target_binding_id;
  decision.reset_runtime_state = response.reset_runtime_state;
  decision.reason = response.reason;
  decision.memory_note = response.memory_note;
  decision.human_request.clear();
  return decision;
}

[[nodiscard]] bool load_verified_human_response(
    const app_context_t& app,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    const std::filesystem::path& response_path,
    const std::filesystem::path& signature_path,
    cuwacunu::hero::human::human_response_record_t* out_response,
    std::string* out_signature_hex, std::string* error) {
  if (error) error->clear();
  if (!out_response || !out_signature_hex) {
    if (error) *error = "human response outputs are null";
    return false;
  }
  *out_response = cuwacunu::hero::human::human_response_record_t{};
  out_signature_hex->clear();

  if (app.defaults.human_operator_identities.empty()) {
    if (error) {
      *error = "Super Hero defaults missing human_operator_identities; cannot verify human response";
    }
    return false;
  }

  std::string response_text{};
  if (!cuwacunu::hero::runtime::read_text_file(response_path, &response_text,
                                               error)) {
    return false;
  }
  std::string signature_hex{};
  if (!cuwacunu::hero::runtime::read_text_file(signature_path, &signature_hex,
                                               error)) {
    return false;
  }
  signature_hex = trim_ascii(signature_hex);

  cuwacunu::hero::human::human_response_record_t response{};
  if (!cuwacunu::hero::human::parse_human_response_json(response_text, &response,
                                                        error)) {
    return false;
  }

  std::string verified_fingerprint{};
  if (!cuwacunu::hero::human::verify_human_response_json_signature(
          app.defaults.human_operator_identities, response.operator_id,
          response_text, signature_hex, &verified_fingerprint, error)) {
    return false;
  }
  if (response.signer_public_key_fingerprint_sha256_hex != verified_fingerprint) {
    if (error) {
      *error =
          "human response fingerprint does not match verified public key fingerprint";
    }
    return false;
  }
  if (response.loop_id != loop.loop_id) {
    if (error) *error = "human response loop_id does not match target loop";
    return false;
  }
  if (response.review_index != loop.review_count) {
    if (error) *error = "human response review_index does not match pending review";
    return false;
  }
  std::string request_sha256_hex{};
  if (!cuwacunu::hero::human::sha256_hex_file(loop.human_request_path,
                                              &request_sha256_hex, error)) {
    return false;
  }
  if (response.request_sha256_hex != request_sha256_hex) {
    if (error) {
      *error =
          "human response request_sha256_hex does not match current human request artifact";
    }
    return false;
  }
  *out_response = std::move(response);
  *out_signature_hex = std::move(signature_hex);
  return true;
}

[[nodiscard]] std::string super_decision_schema_json() {
  return "{\"type\":\"object\",\"properties\":{"
         "\"control_kind\":{\"type\":\"string\",\"enum\":[\"continue\",\"stop\",\"need_human\"]},"
         "\"next_action\":{\"type\":\"object\",\"properties\":{"
         "\"kind\":{\"type\":\"string\",\"enum\":[\"none\",\"default_plan\",\"binding\"]},"
         "\"target_binding_id\":{\"type\":\"string\"},"
         "\"reset_runtime_state\":{\"type\":\"boolean\"}"
         "},\"required\":[\"kind\",\"target_binding_id\",\"reset_runtime_state\"],\"additionalProperties\":false},"
         "\"reason\":{\"type\":\"string\"},"
         "\"memory_note\":{\"type\":\"string\"},"
         "\"human_request\":{\"type\":\"string\"}"
         "},\"required\":[\"control_kind\",\"next_action\",\"reason\",\"memory_note\",\"human_request\"],\"additionalProperties\":false}";
}

[[nodiscard]] bool validate_super_review_decision_action(
    const app_context_t& app,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    const super_review_decision_t& decision, std::string* error) {
  if (error) error->clear();
  if (decision.control_kind == "continue") {
    if (decision.next_action_kind == "default_plan") {
      return true;
    }
    if (decision.next_action_kind == "binding") {
      cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
      if (!decode_campaign_snapshot(app, loop.campaign_dsl_path,
                                    &instruction, error)) {
        return false;
      }
      const auto bind_it = std::find_if(
          instruction.binds.begin(), instruction.binds.end(),
          [&](const auto& bind) { return bind.id == decision.target_binding_id; });
      if (bind_it == instruction.binds.end()) {
        if (error) {
          *error =
              "continue decision target_binding_id is not declared in campaign: " +
              decision.target_binding_id;
        }
        return false;
      }
      return true;
    }
    if (error) {
      *error = "unsupported next_action.kind: " + decision.next_action_kind;
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool parse_super_review_decision(const std::string& json,
                                               super_review_decision_t* out,
                                               std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "decision output pointer is null";
    return false;
  }
  *out = super_review_decision_t{};
  if (!extract_json_string_field(json, "reason", &out->reason)) {
    if (error) *error = "decision JSON missing required reason";
    return false;
  }
  if (!extract_json_string_field(json, "control_kind", &out->control_kind)) {
    if (error) *error = "decision JSON missing required control_kind";
    return false;
  }
  std::string next_action_json{};
  if (extract_json_object_field(json, "next_action", &next_action_json)) {
    (void)extract_json_string_field(next_action_json, "kind",
                                    &out->next_action_kind);
    (void)extract_json_string_field(next_action_json, "target_binding_id",
                                    &out->target_binding_id);
    if (extract_json_field_raw(next_action_json, "reset_runtime_state",
                               nullptr) &&
        !extract_json_bool_field(next_action_json, "reset_runtime_state",
                                 &out->reset_runtime_state)) {
      if (error) {
        *error = "next_action.reset_runtime_state must be boolean when present";
      }
      return false;
    }
  }
  (void)extract_json_string_field(json, "memory_note", &out->memory_note);
  (void)extract_json_string_field(json, "human_request", &out->human_request);
  out->control_kind = trim_ascii(out->control_kind);
  out->next_action_kind = trim_ascii(out->next_action_kind);
  out->target_binding_id = trim_ascii(out->target_binding_id);
  out->memory_note = trim_ascii(out->memory_note);
  out->human_request = trim_ascii(out->human_request);
  if (out->control_kind != "continue" && out->control_kind != "stop" &&
      out->control_kind != "need_human") {
    if (error) *error = "unsupported control_kind: " + out->control_kind;
    return false;
  }
  if (out->next_action_kind != "none" && out->next_action_kind != "default_plan" &&
      out->next_action_kind != "binding") {
    if (error) *error = "unsupported next_action.kind: " + out->next_action_kind;
    return false;
  }
  if (out->control_kind == "continue" && out->next_action_kind == "none") {
    if (error) *error = "continue decision requires actionable next_action.kind";
    return false;
  }
  if (out->control_kind != "continue") {
    if (out->next_action_kind != "none") {
      if (error) {
        *error =
            "stop/need_human decisions must use next_action.kind=none";
      }
      return false;
    }
    out->next_action_kind.clear();
    out->target_binding_id.clear();
    out->reset_runtime_state = false;
  }
  if (out->next_action_kind == "default_plan") out->target_binding_id.clear();
  if (out->next_action_kind == "binding" && out->target_binding_id.empty()) {
    if (error) *error = "next_action.kind=binding requires target_binding_id";
    return false;
  }
  if (out->control_kind == "need_human" && out->human_request.empty()) {
    if (error) *error = "need_human decision requires human_request";
    return false;
  }
  return true;
}

[[nodiscard]] bool rewrite_super_loop_briefing(
    const app_context_t& app,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::string* error) {
  if (error) error->clear();
  std::string objective_dsl_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.super_objective_dsl_path,
                                               &objective_dsl_text, error)) {
    return false;
  }
  std::string objective_md_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.super_objective_md_path,
                                               &objective_md_text, error)) {
    return false;
  }
  std::string guidance_md_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.super_guidance_md_path,
                                               &guidance_md_text, error)) {
    return false;
  }
  std::string memory_text{};
  {
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(loop.memory_path, &memory_text,
                                                  &ignored);
  }
  std::string review_packet_text{};
  {
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(
        super_loop_latest_review_packet_path(loop), &review_packet_text, &ignored);
  }
  std::string campaign_text{};
  {
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(loop.campaign_dsl_path,
                                                  &campaign_text, &ignored);
  }
  std::ostringstream out;
  out << "You are reviewing a Super Hero loop.\n\n"
      << "Sovereignty:\n"
      << "- Super Hero owns the loop ledger and the continue/stop decision.\n"
      << "- Runtime Hero executes campaigns only.\n"
      << "- Config Hero is the only writer for objective/default file changes.\n"
      << "- Hashimyei and Lattice are read-only evidence surfaces in this session.\n\n"
      << "Primary files:\n"
      << "- Super objective DSL: " << loop.super_objective_dsl_path << "\n"
      << "- Super objective markdown: " << loop.super_objective_md_path << "\n"
      << "- Super guidance markdown: " << loop.super_guidance_md_path << "\n"
      << "- Super Hero loop manifest: " << loop.loop_root << "/loop.lls\n"
      << "- Config Hero policy: " << loop.config_policy_path << "\n"
      << "- Memory: " << loop.memory_path << "\n"
      << "- Review packet: "
      << super_loop_latest_review_packet_path(loop).string() << "\n"
      << "- Human request artifact: " << loop.human_request_path << "\n"
      << "- Mutable objective root: " << loop.objective_root << "\n\n"
      << "The review packet includes phase = prelaunch or postcampaign.\n\n"
      << "Interpret the authored markdown in this order:\n"
      << "- objective markdown = what the loop is trying to achieve\n"
      << "- guidance markdown = authored boundaries plus advisory heuristics; prefer stronger evidence when the guidance is not a hard rule\n\n"
      << "Available MCP tools in this review session:\n"
      << "- hero.config.default.list\n"
      << "- hero.config.default.read\n"
      << "- hero.config.default.create\n"
      << "- hero.config.default.replace\n"
      << "- hero.config.default.delete\n"
      << "- hero.config.objective.list\n"
      << "- hero.config.objective.read\n"
      << "- hero.config.objective.create\n"
      << "- hero.config.objective.replace\n"
      << "- hero.config.objective.delete\n"
      << "- hero.runtime.get_campaign\n"
      << "- hero.runtime.get_job\n"
      << "- hero.runtime.list_jobs\n"
      << "- hero.runtime.tail_log\n"
      << "- hero.runtime.tail_trace\n"
      << "- hero.lattice.list_facts\n"
      << "- hero.lattice.get_fact\n"
      << "- hero.lattice.list_views\n"
      << "- hero.lattice.get_view\n\n"
      << "Rules:\n"
      << "1. Work in read-only shell mode. Do not edit files directly.\n"
      << "2. Do not use hero.hashimyei.* tools in this review session; prefer review_packet evidence plus hero.lattice.* queries.\n"
      << "3. Use Config Hero objective.read/create/replace/delete for truth-source objective files under objective_root.\n"
      << "4. Use Config Hero default.read/create/replace/delete only when a shared default truly needs to change.\n"
      << "5. Pass objective_root=" << loop.objective_root
      << " to those Config Hero tools.\n"
      << "6. Prefer whole-file replace with expected_sha256 from the prior read.\n"
      << "7. Never mutate files outside the configured objective/default roots.\n"
      << "8. Prefer the review packet first, then hero.lattice.get_view/get_fact for semantic evidence.\n"
      << "9. Use hero.lattice.list_views/list_facts to discover selectors; family_evaluation_report requires a family canonical_path plus contract_hash.\n"
      << "10. Use Runtime get/tail tools mainly for operational debugging such as launch failures, missing logs, or abnormal traces.\n"
      << "11. Shell exec is unavailable in this environment. Prefer the embedded review packet, memory, and objective campaign contents below; use MCP tools instead of shell reads.\n"
      << "12. If you change any objective or campaign DSL, describe the actual changes in memory_note.\n"
      << "13. Prefer stopping or need_human when evidence is weak or authority would need to widen.\n"
      << "14. Return only JSON matching the provided output schema.\n\n"
      << "Decision contract:\n"
      << "- control_kind = continue | stop | need_human\n"
      << "- Always include next_action with kind = none | default_plan | binding\n"
      << "- continue requires next_action.kind = default_plan | binding\n"
      << "- stop and need_human must use next_action.kind = none\n"
      << "- binding requires next_action.target_binding_id\n"
      << "- Always include next_action.reset_runtime_state as true or false\n"
      << "- Always include memory_note and human_request as strings; use empty string when not applicable\n"
      << "- need_human requires a non-empty human_request\n\n"
      << "Repo root: " << app.defaults.repo_root.string() << "\n";
  if (!trim_ascii(objective_dsl_text).empty()) {
    out << "\nSuper objective DSL contents:\n" << objective_dsl_text;
    if (objective_dsl_text.back() != '\n') out << "\n";
  }
  if (!trim_ascii(objective_md_text).empty()) {
    out << "\nSuper objective markdown contents:\n" << objective_md_text;
    if (objective_md_text.back() != '\n') out << "\n";
  }
  if (!trim_ascii(guidance_md_text).empty()) {
    out << "\nSuper guidance markdown contents:\n" << guidance_md_text;
    if (guidance_md_text.back() != '\n') out << "\n";
  }
  if (!trim_ascii(memory_text).empty()) {
    out << "\nMemory contents:\n" << memory_text;
    if (memory_text.back() != '\n') out << "\n";
  }
  if (!trim_ascii(review_packet_text).empty()) {
    out << "\nReview packet contents:\n" << review_packet_text;
    if (review_packet_text.back() != '\n') out << "\n";
  }
  if (!trim_ascii(campaign_text).empty()) {
    out << "\nObjective campaign DSL contents:\n" << campaign_text;
    if (campaign_text.back() != '\n') out << "\n";
  }
  return cuwacunu::hero::runtime::write_text_file_atomic(loop.briefing_path,
                                                         out.str(), error);
}

[[nodiscard]] bool run_command_with_stdio_and_timeout(
    const std::vector<std::string>& argv, const std::filesystem::path& stdin_path,
    const std::filesystem::path& stdout_path,
    const std::filesystem::path& stderr_path,
    const std::filesystem::path* working_dir,
    const std::vector<std::pair<std::string, std::string>>* env_overrides,
    std::size_t timeout_sec, const std::filesystem::path* pid_path,
    int* out_exit_code, std::string* error) {
  if (error) error->clear();
  if (argv.empty()) {
    if (error) *error = "command argv is empty";
    return false;
  }
  if (out_exit_code) *out_exit_code = -1;
  if (pid_path != nullptr && !pid_path->empty()) remove_file_noexcept(*pid_path);

  std::error_code ec{};
  if (!std::filesystem::exists(stdin_path, ec) ||
      !std::filesystem::is_regular_file(stdin_path, ec)) {
    if (error) *error = "command stdin path does not exist: " + stdin_path.string();
    return false;
  }
  if (!stdout_path.empty()) {
    const int stdout_probe =
        ::open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stdout_probe < 0) {
      if (error) {
        *error = "cannot open stdout path for command: " + stdout_path.string();
      }
      return false;
    }
    (void)::close(stdout_probe);
  }
  if (stderr_path.empty()) {
    if (error) *error = "command stderr path is empty";
    return false;
  }
  const int stderr_probe =
      ::open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (stderr_probe < 0) {
    if (error) *error = "cannot open stderr path for command: " + stderr_path.string();
    return false;
  }
  (void)::close(stderr_probe);

  const pid_t child = ::fork();
  if (child < 0) {
    if (error) *error = "fork failed for command execution";
    return false;
  }
  if (child == 0) {
    (void)::setpgid(0, 0);
    if (working_dir != nullptr && !working_dir->empty() &&
        ::chdir(working_dir->c_str()) != 0) {
      _exit(125);
    }
    if (env_overrides != nullptr) {
      for (const auto& [key, value] : *env_overrides) {
        if (key.empty()) continue;
        if (::setenv(key.c_str(), value.c_str(), 1) != 0) _exit(125);
      }
    }
    const int stdin_fd = ::open(stdin_path.c_str(), O_RDONLY);
    if (stdin_fd < 0) _exit(126);
    (void)::dup2(stdin_fd, STDIN_FILENO);
    if (stdin_fd > STDERR_FILENO) (void)::close(stdin_fd);
    const char* stdout_target =
        stdout_path.empty() ? "/dev/null" : stdout_path.c_str();
    const int stdout_fd =
        ::open(stdout_target, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stdout_fd < 0) _exit(126);
    (void)::dup2(stdout_fd, STDOUT_FILENO);
    if (stdout_fd > STDERR_FILENO) (void)::close(stdout_fd);
    const int stderr_fd =
        ::open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stderr_fd < 0) _exit(126);
    (void)::dup2(stderr_fd, STDERR_FILENO);
    if (stderr_fd > STDERR_FILENO) (void)::close(stderr_fd);
    std::vector<char*> exec_argv{};
    exec_argv.reserve(argv.size() + 1);
    for (const std::string& arg : argv) {
      exec_argv.push_back(const_cast<char*>(arg.c_str()));
    }
    exec_argv.push_back(nullptr);
    ::execvp(exec_argv[0], exec_argv.data());
    _exit(127);
  }

  if (pid_path != nullptr && !pid_path->empty()) {
    std::string pid_error{};
    if (!cuwacunu::hero::runtime::write_text_file_atomic(
            *pid_path, std::to_string(static_cast<long long>(child)),
            &pid_error)) {
      (void)::kill(-child, SIGKILL);
      (void)::kill(child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      if (error) {
        *error = "cannot persist review pid path " + pid_path->string() +
                 ": " + pid_error;
      }
      return false;
    }
  }

  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);
  int status = 0;
  for (;;) {
    const pid_t waited = ::waitpid(child, &status, WNOHANG);
    if (waited == child) break;
    if (waited < 0 && errno != EINTR) {
      if (error) *error = "waitpid failed for command execution";
      (void)::kill(-child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      if (pid_path != nullptr && !pid_path->empty()) remove_file_noexcept(*pid_path);
      return false;
    }
    if (std::chrono::steady_clock::now() >= deadline) {
      (void)::kill(-child, SIGKILL);
      (void)::kill(child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      if (error) *error = "command timed out";
      if (pid_path != nullptr && !pid_path->empty()) remove_file_noexcept(*pid_path);
      return false;
    }
    ::usleep(100 * 1000);
  }

  if (pid_path != nullptr && !pid_path->empty()) remove_file_noexcept(*pid_path);

  if (WIFEXITED(status)) {
    if (out_exit_code) *out_exit_code = WEXITSTATUS(status);
    return true;
  }
  if (WIFSIGNALED(status)) {
    if (out_exit_code) *out_exit_code = 128 + WTERMSIG(status);
    if (error) *error = "command terminated by signal";
    return false;
  }
  if (error) *error = "command ended in unknown state";
  return false;
}

[[nodiscard]] bool build_super_review_packet_json(
    const app_context_t& app,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    const cuwacunu::hero::runtime::runtime_campaign_record_t& campaign,
    std::uint64_t review_index, std::string* out_json, std::string* error) {
  if (error) error->clear();
  if (!out_json) {
    if (error) *error = "review packet output pointer is null";
    return false;
  }
  out_json->clear();

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  if (!decode_campaign_snapshot(app, loop.campaign_dsl_path,
                                &instruction, error)) {
    return false;
  }

  std::vector<std::string> declared_bind_ids{};
  declared_bind_ids.reserve(instruction.binds.size());
  for (const auto& bind : instruction.binds) declared_bind_ids.push_back(bind.id);
  std::vector<std::string> default_run_bind_ids{};
  default_run_bind_ids.reserve(instruction.runs.size());
  for (const auto& run : instruction.runs) {
    default_run_bind_ids.push_back(run.bind_ref);
  }

  struct job_review_row_t {
    cuwacunu::hero::runtime::runtime_job_record_t record{};
    cuwacunu::hero::runtime::runtime_job_observation_t observation{};
    std::string stdout_tail{};
    std::string stderr_tail{};
    std::string trace_tail{};
    std::string contract_hash{};
    std::string wave_hash{};
  };
  std::vector<job_review_row_t> jobs{};
  jobs.reserve(campaign.job_cursors.size());
  for (const std::string& job_cursor : campaign.job_cursors) {
    cuwacunu::hero::runtime::runtime_job_record_t job{};
    if (!read_runtime_job_direct(app, job_cursor, &job, error)) return false;
    job_review_row_t row{};
    row.record = job;
    row.observation = cuwacunu::hero::runtime::observe_runtime_job(job);
    (void)tail_file_lines(job.stdout_path,
                          std::min<std::size_t>(40, app.defaults.tail_default_lines),
                          &row.stdout_tail, nullptr);
    (void)tail_file_lines(job.stderr_path,
                          std::min<std::size_t>(20, app.defaults.tail_default_lines),
                          &row.stderr_tail, nullptr);
    (void)tail_file_lines(runtime_job_trace_path_for_record(app, job),
                          std::min<std::size_t>(40, app.defaults.tail_default_lines),
                          &row.trace_tail, nullptr);
    row.contract_hash =
        extract_logged_hash_value(row.stdout_tail, "contract_hash=");
    row.wave_hash = extract_logged_hash_value(row.stdout_tail, "wave_hash=");
    jobs.push_back(std::move(row));
  }

  const auto run_manifest_hints =
      collect_run_manifest_hints(app, declared_bind_ids, 2);
  std::vector<std::string> contract_hash_candidates{};
  for (const auto& row : jobs) contract_hash_candidates.push_back(row.contract_hash);
  for (const auto& hint : run_manifest_hints) {
    contract_hash_candidates.push_back(hint.contract_hash);
  }
  contract_hash_candidates =
      unique_nonempty_strings(std::move(contract_hash_candidates));

  std::ostringstream jobs_json;
  jobs_json << "[";
  for (std::size_t i = 0; i < jobs.size(); ++i) {
    if (i != 0) jobs_json << ",";
    jobs_json << "{"
              << "\"job\":"
              << runtime_job_to_json(jobs[i].record, jobs[i].observation)
              << ",\"stdout_tail\":" << json_quote(jobs[i].stdout_tail)
              << ",\"stderr_tail\":" << json_quote(jobs[i].stderr_tail)
              << ",\"trace_tail\":" << json_quote(jobs[i].trace_tail)
              << ",\"contract_hash\":" << json_quote(jobs[i].contract_hash)
              << ",\"wave_hash\":" << json_quote(jobs[i].wave_hash) << "}";
  }
  jobs_json << "]";

  std::ostringstream manifests_json;
  manifests_json << "[";
  for (std::size_t i = 0; i < run_manifest_hints.size(); ++i) {
    if (i != 0) manifests_json << ",";
    manifests_json << "{"
                   << "\"path\":"
                   << json_quote(run_manifest_hints[i].path.string())
                   << ",\"run_id\":"
                   << json_quote(run_manifest_hints[i].run_id)
                   << ",\"binding_id\":"
                   << json_quote(run_manifest_hints[i].binding_id)
                   << ",\"contract_hash\":"
                   << json_quote(run_manifest_hints[i].contract_hash)
                   << ",\"wave_hash\":"
                   << json_quote(run_manifest_hints[i].wave_hash)
                   << ",\"started_at_ms\":"
                   << run_manifest_hints[i].started_at_ms << "}";
  }
  manifests_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(kReviewPacketSchemaV1) << ","
      << "\"phase\":\"postcampaign\","
      << "\"review_index\":" << review_index
      << ",\"loop\":" << super_loop_to_json(loop)
      << ",\"campaign\":" << runtime_campaign_to_json(campaign)
      << ",\"declared_bind_ids\":" << json_array_from_strings(declared_bind_ids)
      << ",\"default_run_bind_ids\":"
      << json_array_from_strings(default_run_bind_ids)
      << ",\"jobs\":" << jobs_json.str()
      << ",\"run_manifest_hints\":" << manifests_json.str()
      << ",\"lattice_recommendations\":"
      << build_lattice_recommendations_json(contract_hash_candidates)
      << ",\"memory_path\":" << json_quote(loop.memory_path)
      << ",\"human_request_path\":" << json_quote(loop.human_request_path)
      << ",\"super_objective_dsl_path\":"
      << json_quote(loop.super_objective_dsl_path)
      << ",\"super_objective_md_path\":"
      << json_quote(loop.super_objective_md_path)
      << ",\"super_guidance_md_path\":"
      << json_quote(loop.super_guidance_md_path)
      << ",\"config_policy_path\":"
      << json_quote(loop.config_policy_path)
      << ",\"briefing_path\":" << json_quote(loop.briefing_path)
      << ",\"mutable_objective_root\":"
      << json_quote(loop.objective_root)
      << ",\"objective_campaign_dsl_path\":"
      << json_quote(loop.campaign_dsl_path)
      << ",\"objective_root\":" << json_quote(loop.objective_root)
      << ",\"campaigns_root\":"
      << json_quote(campaigns_root_for_app(app).string())
      << ",\"super_root\":" << json_quote(app.defaults.super_root.string())
      << "}";
  *out_json = out.str();
  return true;
}

[[nodiscard]] bool build_super_prelaunch_review_packet_json(
    const app_context_t& app,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::uint64_t review_index, std::string* out_json, std::string* error) {
  if (error) error->clear();
  if (!out_json) {
    if (error) *error = "prelaunch review packet output pointer is null";
    return false;
  }
  out_json->clear();

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  if (!decode_campaign_snapshot(app, loop.campaign_dsl_path, &instruction, error)) {
    return false;
  }

  std::vector<std::string> declared_bind_ids{};
  declared_bind_ids.reserve(instruction.binds.size());
  for (const auto& bind : instruction.binds) declared_bind_ids.push_back(bind.id);
  std::vector<std::string> default_run_bind_ids{};
  default_run_bind_ids.reserve(instruction.runs.size());
  for (const auto& run : instruction.runs) {
    default_run_bind_ids.push_back(run.bind_ref);
  }

  const auto run_manifest_hints =
      collect_run_manifest_hints(app, declared_bind_ids, 2);
  std::vector<std::string> contract_hash_candidates{};
  for (const auto& hint : run_manifest_hints) {
    contract_hash_candidates.push_back(hint.contract_hash);
  }
  contract_hash_candidates =
      unique_nonempty_strings(std::move(contract_hash_candidates));

  std::ostringstream manifests_json;
  manifests_json << "[";
  for (std::size_t i = 0; i < run_manifest_hints.size(); ++i) {
    if (i != 0) manifests_json << ",";
    manifests_json << "{"
                   << "\"path\":"
                   << json_quote(run_manifest_hints[i].path.string())
                   << ",\"run_id\":"
                   << json_quote(run_manifest_hints[i].run_id)
                   << ",\"binding_id\":"
                   << json_quote(run_manifest_hints[i].binding_id)
                   << ",\"contract_hash\":"
                   << json_quote(run_manifest_hints[i].contract_hash)
                   << ",\"wave_hash\":"
                   << json_quote(run_manifest_hints[i].wave_hash)
                   << ",\"started_at_ms\":"
                   << run_manifest_hints[i].started_at_ms << "}";
  }
  manifests_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(kReviewPacketSchemaV1) << ","
      << "\"phase\":\"prelaunch\","
      << "\"review_index\":" << review_index
      << ",\"loop\":" << super_loop_to_json(loop)
      << ",\"campaign\":null"
      << ",\"declared_bind_ids\":" << json_array_from_strings(declared_bind_ids)
      << ",\"default_run_bind_ids\":"
      << json_array_from_strings(default_run_bind_ids)
      << ",\"jobs\":[]"
      << ",\"run_manifest_hints\":" << manifests_json.str()
      << ",\"lattice_recommendations\":"
      << build_lattice_recommendations_json(contract_hash_candidates)
      << ",\"memory_path\":" << json_quote(loop.memory_path)
      << ",\"human_request_path\":" << json_quote(loop.human_request_path)
      << ",\"super_objective_dsl_path\":"
      << json_quote(loop.super_objective_dsl_path)
      << ",\"super_objective_md_path\":"
      << json_quote(loop.super_objective_md_path)
      << ",\"super_guidance_md_path\":"
      << json_quote(loop.super_guidance_md_path)
      << ",\"config_policy_path\":"
      << json_quote(loop.config_policy_path)
      << ",\"briefing_path\":" << json_quote(loop.briefing_path)
      << ",\"mutable_objective_root\":"
      << json_quote(loop.objective_root)
      << ",\"objective_campaign_dsl_path\":"
      << json_quote(loop.campaign_dsl_path)
      << ",\"objective_root\":" << json_quote(loop.objective_root)
      << ",\"campaigns_root\":"
      << json_quote(campaigns_root_for_app(app).string())
      << ",\"super_root\":" << json_quote(app.defaults.super_root.string())
      << ",\"launch_context\":{\"initial_launch\":true}"
      << "}";
  *out_json = out.str();
  return true;
}

[[nodiscard]] bool launch_runtime_campaign_for_decision(
    const app_context_t& app,
    cuwacunu::hero::super::super_loop_record_t* loop,
    const super_review_decision_t& decision, std::string_view state_detail,
    std::string_view event_name, std::string* out_campaign_cursor,
    std::string* out_campaign_json, std::string* out_warning,
    std::string* error) {
  if (error) error->clear();
  if (out_campaign_cursor) out_campaign_cursor->clear();
  if (out_campaign_json) out_campaign_json->clear();
  if (out_warning) out_warning->clear();
  if (!loop) {
    if (error) *error = "super loop pointer is null";
    return false;
  }

  std::string start_args =
      "{\"campaign_dsl_path\":" + json_quote(loop->campaign_dsl_path);
  start_args += ",\"super_loop_id\":" + json_quote(loop->loop_id);
  if (decision.next_action_kind == "binding") {
    start_args += ",\"binding_id\":" + json_quote(decision.target_binding_id);
  }
  if (decision.reset_runtime_state) {
    start_args += ",\"reset_runtime_state\":true";
  }
  start_args += "}";

  std::string start_structured{};
  if (!call_runtime_tool(app, "hero.runtime.start_campaign", start_args,
                         &start_structured, error)) {
    return false;
  }

  std::vector<std::string> launch_warnings{};
  append_structured_warnings(start_structured, "runtime launch warning: ",
                             &launch_warnings);
  std::string runtime_warning{};
  for (const std::string& item : launch_warnings) {
    append_warning_text(&runtime_warning, item);
  }

  std::string campaign_cursor{};
  if (!extract_json_string_field(start_structured, "campaign_cursor",
                                 &campaign_cursor) ||
      campaign_cursor.empty()) {
    if (error) *error = "Runtime Hero start_campaign did not return campaign_cursor";
    return false;
  }
  std::string campaign_json{};
  if (!extract_json_object_field(start_structured, "campaign", &campaign_json)) {
    if (error) *error = "Runtime Hero start_campaign did not return campaign object";
    return false;
  }

  loop->state = "running";
  loop->state_detail = std::string(state_detail);
  loop->active_campaign_cursor = campaign_cursor;
  loop->campaign_cursors.push_back(campaign_cursor);
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop->last_warning.clear();
  std::string bookkeeping_error{};
  if (!write_super_loop(app, *loop, &bookkeeping_error)) {
    append_warning_text(&runtime_warning,
                        "post-launch loop manifest degraded: " + bookkeeping_error);
  } else if (!append_super_loop_event(*loop, std::string(event_name),
                                      campaign_cursor, &bookkeeping_error)) {
    append_warning_text(&runtime_warning,
                        "post-launch super event degraded: " + bookkeeping_error);
  }
  if (!runtime_warning.empty()) {
    persist_super_loop_warning_best_effort(app, loop, runtime_warning);
    if (out_warning) *out_warning = runtime_warning;
  }
  if (out_campaign_cursor) *out_campaign_cursor = std::move(campaign_cursor);
  if (out_campaign_json) *out_campaign_json = std::move(campaign_json);
  return true;
}

[[nodiscard]] bool run_super_review_with_codex(
    const app_context_t& app,
    cuwacunu::hero::super::super_loop_record_t* loop,
    std::uint64_t review_index, super_review_decision_t* out_decision,
    std::string* out_warning, std::string* error) {
  if (error) error->clear();
  if (out_warning) out_warning->clear();
  if (!loop) {
    if (error) *error = "super loop pointer is null";
    return false;
  }
  if (!out_decision) {
    if (error) *error = "super decision output pointer is null";
    return false;
  }
  scoped_temp_path_t decision_schema_path{};
  if (!write_temp_super_decision_schema(super_decision_schema_json(),
                                        &decision_schema_path, error)) {
    return false;
  }
  const super_loop_workspace_context_t workspace_context =
      make_super_loop_workspace_context(app);
  if (!refresh_super_loop_config_policy_dsl(workspace_context, *loop, error)) {
    return false;
  }
  if (!rewrite_super_loop_briefing(app, *loop, error)) return false;

  const std::filesystem::path decision_path =
      cuwacunu::hero::super::super_loop_decision_path(
          app.defaults.super_root, loop->loop_id, review_index);
  const std::string config_args = json_array_from_strings(
      {"--global-config", app.global_config_path.string(), "--config",
       loop->config_policy_path});
  const std::string runtime_args = json_array_from_strings(
      {"--global-config", app.global_config_path.string()});
  const std::string lattice_args = runtime_args;
  const std::string enabled_config_tools = json_array_from_strings(
      {"hero.config.default.list", "hero.config.default.read",
       "hero.config.default.create", "hero.config.default.replace",
       "hero.config.default.delete", "hero.config.objective.list",
       "hero.config.objective.read", "hero.config.objective.create",
       "hero.config.objective.replace", "hero.config.objective.delete"});
  const std::string enabled_runtime_tools = json_array_from_strings(
      {"hero.runtime.get_campaign", "hero.runtime.get_job",
       "hero.runtime.list_jobs", "hero.runtime.tail_log",
       "hero.runtime.tail_trace"});
  const std::string enabled_lattice_tools = json_array_from_strings(
      {"hero.lattice.list_facts", "hero.lattice.get_fact",
       "hero.lattice.list_views", "hero.lattice.get_view"});

  auto append_common_codex_mcp_args = [&](std::vector<std::string>* argv) {
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-config.enabled=true");
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-config.command=" +
                    json_quote(app.defaults.config_hero_binary.string()));
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-config.args=" + config_args);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-config.enabled_tools=" +
                    enabled_config_tools);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-config.startup_timeout_sec=30");

    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-runtime.enabled=true");
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-runtime.command=" +
                    json_quote(app.defaults.runtime_hero_binary.string()));
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-runtime.args=" + runtime_args);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-runtime.enabled_tools=" +
                    enabled_runtime_tools);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-runtime.startup_timeout_sec=30");

    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.enabled=true");
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.command=" +
                    json_quote(app.defaults.lattice_hero_binary.string()));
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.args=" + lattice_args);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.enabled_tools=" +
                    enabled_lattice_tools);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.startup_timeout_sec=30");
  };

  auto load_decision_text = [&](std::string* out_text,
                                std::string* out_err) -> bool {
    if (!out_text) {
      if (out_err) *out_err = "decision output pointer is null";
      return false;
    }
    out_text->clear();
    return cuwacunu::hero::runtime::read_text_file(decision_path, out_text,
                                                   out_err);
  };

  auto load_session_id_from_stderr = [&](std::string* out_session_id) -> bool {
    if (!out_session_id) return false;
    out_session_id->clear();
    std::string stderr_text{};
    std::string ignored{};
    if (!cuwacunu::hero::runtime::read_text_file(
            super_loop_codex_session_log_path(*loop), &stderr_text, &ignored)) {
      return false;
    }
    *out_session_id = extract_codex_session_id_from_log(stderr_text);
    return !out_session_id->empty();
  };

  auto run_codex_attempt = [&](bool resume_mode, std::string* out_decision_text,
                               std::string* out_session_id,
                               std::string* out_attempt_error) -> bool {
    if (out_attempt_error) out_attempt_error->clear();
    if (out_decision_text) out_decision_text->clear();
    if (out_session_id) out_session_id->clear();

    std::vector<std::string> argv{};
    argv.reserve(64);
    argv.push_back(app.defaults.super_codex_binary.string());
    argv.push_back("exec");
    if (resume_mode) {
      argv.push_back("resume");
    } else {
      argv.push_back("-s");
      argv.push_back("read-only");
      argv.push_back("--color");
      argv.push_back("never");
    }
    append_common_codex_mcp_args(&argv);
    if (!resume_mode) {
      argv.push_back("--output-schema");
      argv.push_back(decision_schema_path.path.string());
    }
    argv.push_back("-o");
    argv.push_back(decision_path.string());
    if (resume_mode) {
      argv.push_back(loop->codex_session_id);
    }
    argv.push_back("-");

    int exit_code = -1;
    std::string invoke_error{};
    const std::filesystem::path review_pid_path =
        super_loop_review_pid_path(*loop);
    if (!run_command_with_stdio_and_timeout(
            argv, loop->briefing_path, {},
            super_loop_codex_session_log_path(*loop), &app.defaults.repo_root,
            nullptr, app.defaults.super_codex_timeout_sec, &review_pid_path,
            &exit_code,
            &invoke_error)) {
      if (out_attempt_error) {
        *out_attempt_error = resume_mode
                                 ? "codex exec resume failed: " + invoke_error
                                 : "codex exec failed: " + invoke_error;
      }
      return false;
    }
    if (exit_code != 0) {
      if (out_attempt_error) {
        *out_attempt_error =
            std::string(resume_mode ? "codex exec resume failed with exit_code="
                                    : "codex exec failed with exit_code=") +
            std::to_string(exit_code);
      }
      return false;
    }
    if (!load_decision_text(out_decision_text, out_attempt_error)) return false;
    (void)load_session_id_from_stderr(out_session_id);
    return true;
  };

  std::string decision_text{};
  std::string parsed_session_id{};
  bool decision_ready = false;
  const bool had_prior_session = !trim_ascii(loop->codex_session_id).empty();
  if (had_prior_session) {
    std::string resume_error{};
    std::string resume_decision_text{};
    std::string resumed_session_id{};
    if (run_codex_attempt(true, &resume_decision_text, &resumed_session_id,
                          &resume_error)) {
      if (parse_super_review_decision(resume_decision_text, out_decision,
                                      &resume_error)) {
        decision_text = std::move(resume_decision_text);
        if (!resumed_session_id.empty()) {
          loop->codex_session_id = resumed_session_id;
        }
        decision_ready = true;
      } else if (out_warning) {
        append_warning_text(
            out_warning,
            "codex resume degraded: decision parse failed; retrying fresh review (" +
                resume_error + ")");
      }
    } else if (out_warning) {
      append_warning_text(out_warning,
                          "codex resume degraded: " + resume_error +
                              "; retrying fresh review");
    }
  }

  if (!decision_ready) {
    std::string fresh_error{};
    if (!run_codex_attempt(false, &decision_text, &parsed_session_id,
                           &fresh_error)) {
      if (error) *error = fresh_error;
      return false;
    }
    if (!parse_super_review_decision(decision_text, out_decision, error)) {
      return false;
    }
    if (!parsed_session_id.empty()) {
      loop->codex_session_id = parsed_session_id;
    } else {
      loop->codex_session_id.clear();
      if (out_warning) {
        append_warning_text(
            out_warning,
            "fresh codex review completed without a persisted session id; future reviews will start fresh");
      }
    }
  }

  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          super_loop_latest_decision_path(*loop), decision_text, error)) {
    return false;
  }
  return true;
}

[[nodiscard]] bool continue_loop_after_terminal_campaign(
    app_context_t* app, std::string_view campaign_cursor,
    std::string* out_warning, std::string* error) {
  if (error) error->clear();
  if (out_warning) out_warning->clear();
  if (!app) {
    if (error) *error = "super app pointer is null";
    return false;
  }

  cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
  if (!read_runtime_campaign_direct(*app, campaign_cursor, &campaign, error)) {
    return false;
  }

  cuwacunu::hero::super::super_loop_record_t loop{};
  const std::string loop_id = trim_ascii(campaign.super_loop_id);
  if (loop_id.empty()) {
    if (error) *error = "campaign is not linked to a super loop";
    return false;
  }
  if (!read_super_loop(*app, loop_id, &loop, error)) return false;
  if (is_super_loop_terminal_state(loop.state)) return true;
  if (loop.review_count >= loop.max_reviews) {
    loop.state = "exhausted";
    loop.state_detail = "super review budget exhausted";
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    loop.active_campaign_cursor.clear();
    if (!write_super_loop(*app, loop, error)) return false;
    if (!append_super_loop_event(loop, "loop_exhausted", loop.state_detail,
                                 error)) {
      return false;
    }
    return true;
  }

  const std::uint64_t review_index = loop.review_count + 1;
  std::string review_packet_json{};
  if (!build_super_review_packet_json(*app, loop, campaign, review_index,
                                      &review_packet_json, error)) {
    loop.state = "failed";
    loop.state_detail = *error;
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_super_loop(*app, loop, &ignored);
    (void)append_super_loop_event(loop, "review_failed", *error, &ignored);
    return false;
  }
  const std::filesystem::path review_packet_path =
      cuwacunu::hero::super::super_loop_review_packet_path(
          app->defaults.super_root, loop.loop_id, review_index);
  if (!cuwacunu::hero::runtime::write_text_file_atomic(review_packet_path,
                                                       review_packet_json,
                                                       error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          super_loop_latest_review_packet_path(loop), review_packet_json, error)) {
    return false;
  }
  loop.state = "reviewing";
  loop.state_detail = "waiting for codex review";
  loop.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop.last_review_packet_path = review_packet_path.string();
  if (!write_super_loop(*app, loop, error)) return false;
  if (!append_super_loop_event(loop, "review_started", review_packet_path.string(),
                               error)) {
    return false;
  }

  super_review_decision_t decision{};
  std::string review_warning{};
  if (!run_super_review_with_codex(*app, &loop, review_index, &decision,
                                   &review_warning, error)) {
    cuwacunu::hero::super::super_loop_record_t terminal_loop{};
    if (reload_terminal_super_loop_if_any(*app, loop.loop_id, &terminal_loop)) {
      if (out_warning) {
        append_warning_text(out_warning,
                            "review interrupted after loop became terminal");
      }
      return true;
    }
    loop.state = "failed";
    loop.state_detail = *error;
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_super_loop(*app, loop, &ignored);
    (void)append_super_loop_event(loop, "review_failed", *error, &ignored);
    return false;
  }
  if (!validate_super_review_decision_action(*app, loop, decision, error)) {
    cuwacunu::hero::super::super_loop_record_t terminal_loop{};
    if (reload_terminal_super_loop_if_any(*app, loop.loop_id, &terminal_loop)) {
      if (out_warning) {
        append_warning_text(out_warning,
                            "review validation interrupted after loop became terminal");
      }
      return true;
    }
    loop.state = "failed";
    loop.state_detail = *error;
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_super_loop(*app, loop, &ignored);
    (void)append_super_loop_event(loop, "review_failed", *error, &ignored);
    return false;
  }

  append_memory_note(loop, review_index, decision.memory_note);
  loop.review_count = review_index;
  loop.last_control_kind = decision.control_kind;
  loop.last_warning.clear();
  if (!review_warning.empty()) append_warning_text(&loop.last_warning, review_warning);
  loop.last_decision_path =
      cuwacunu::hero::super::super_loop_decision_path(
          app->defaults.super_root, loop.loop_id, review_index)
          .string();
  loop.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();

  if (decision.control_kind == "stop" ||
      decision.control_kind == "need_human") {
    loop.state = (decision.control_kind == "stop") ? "stopped" : "need_human";
    loop.state_detail = decision.reason;
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    loop.active_campaign_cursor.clear();
    if (!write_super_loop(*app, loop, error)) return false;
    std::string warning{};
    std::string human_request_error{};
    if (decision.control_kind == "need_human" &&
        !write_human_request_note(loop, review_index, decision,
                                  &human_request_error)) {
      append_warning_text(&warning,
                          "human_request artifact write degraded: " +
                              human_request_error);
    }
    std::string event_error{};
    if (!append_super_loop_event(loop, "review_finished", decision.reason,
                                 &event_error)) {
      append_warning_text(&warning,
                          "final review event append degraded: " + event_error);
    }
    if (!warning.empty()) {
      persist_super_loop_warning_best_effort(*app, &loop, warning);
      if (out_warning) *out_warning = warning;
    }
    if (!review_warning.empty()) {
      if (out_warning) append_warning_text(out_warning, review_warning);
    }
    return true;
  }

  std::string next_campaign_cursor{};
  std::string ignored_campaign_json{};
  std::string runtime_warning{};
  if (!launch_runtime_campaign_for_decision(
          *app, &loop, decision, "campaign launched from super review",
          "campaign_launched_from_review", &next_campaign_cursor,
          &ignored_campaign_json, &runtime_warning, error)) {
    loop.state = "failed";
    loop.state_detail = *error;
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_super_loop(*app, loop, &ignored);
    (void)append_super_loop_event(loop, "launch_failed", *error, &ignored);
    return false;
  }
  if (!runtime_warning.empty()) {
    if (out_warning) append_warning_text(out_warning, runtime_warning);
  }
  if (!review_warning.empty()) {
    persist_super_loop_warning_best_effort(*app, &loop, review_warning);
    if (out_warning) append_warning_text(out_warning, review_warning);
  }
  return true;
}

struct loop_runner_lock_guard_t {
  int fd{-1};

  ~loop_runner_lock_guard_t() {
    if (fd >= 0) {
      (void)::flock(fd, LOCK_UN);
      (void)::close(fd);
    }
  }
};

[[nodiscard]] bool acquire_loop_runner_lock(
    const cuwacunu::hero::super::super_loop_record_t& loop,
    loop_runner_lock_guard_t* out_lock, bool* out_already_locked,
    std::string* error) {
  if (error) error->clear();
  if (out_already_locked) *out_already_locked = false;
  if (!out_lock) {
    if (error) *error = "loop runner lock output pointer is null";
    return false;
  }
  out_lock->fd = -1;
  const std::filesystem::path lock_path = loop_runner_lock_path(loop);
  const int fd = ::open(lock_path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
  if (fd < 0) {
    if (error) *error = "cannot open loop runner lock: " + lock_path.string();
    return false;
  }
  if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
    const int err = errno;
    (void)::close(fd);
    if (err == EWOULDBLOCK || err == EAGAIN) {
      if (out_already_locked) *out_already_locked = true;
      return true;
    }
    if (error) *error = "cannot acquire loop runner lock: " + loop.loop_id;
    return false;
  }
  out_lock->fd = fd;
  return true;
}

void write_child_errno_noexcept(int fd, int err) {
  if (fd < 0) return;
  const ssize_t ignored = ::write(fd, &err, sizeof(err));
  (void)ignored;
}

[[nodiscard]] bool launch_loop_runner_process(
    const app_context_t& app, std::string_view loop_id, std::string* error) {
  if (error) error->clear();

  int pipe_fds[2]{-1, -1};
#ifdef O_CLOEXEC
  if (::pipe2(pipe_fds, O_CLOEXEC) != 0) {
    if (error) *error = "pipe2 failed";
    return false;
  }
#else
  if (::pipe(pipe_fds) != 0) {
    if (error) *error = "pipe failed";
    return false;
  }
  (void)::fcntl(pipe_fds[0], F_SETFD, FD_CLOEXEC);
  (void)::fcntl(pipe_fds[1], F_SETFD, FD_CLOEXEC);
#endif

  const pid_t child = ::fork();
  if (child < 0) {
    const std::string msg = "fork failed";
    (void)::close(pipe_fds[0]);
    (void)::close(pipe_fds[1]);
    if (error) *error = msg;
    return false;
  }
  if (child == 0) {
    (void)::close(pipe_fds[0]);
    if (::setsid() < 0) {
      write_child_errno_noexcept(pipe_fds[1], errno);
      _exit(127);
    }

    const pid_t grandchild = ::fork();
    if (grandchild < 0) {
      write_child_errno_noexcept(pipe_fds[1], errno);
      _exit(127);
    }
    if (grandchild > 0) _exit(0);

    const int devnull = ::open("/dev/null", O_RDWR);
    if (devnull >= 0) {
      (void)::dup2(devnull, STDIN_FILENO);
      (void)::dup2(devnull, STDOUT_FILENO);
      (void)::dup2(devnull, STDERR_FILENO);
      if (devnull > STDERR_FILENO) (void)::close(devnull);
    }

    std::vector<std::string> args{};
    args.push_back(app.self_binary_path.string());
    args.push_back("--loop-runner");
    args.push_back("--loop-id");
    args.push_back(std::string(loop_id));
    args.push_back("--global-config");
    args.push_back(app.global_config_path.string());
    args.push_back("--hero-config");
    args.push_back(app.hero_config_path.string());

    std::vector<char*> argv{};
    argv.reserve(args.size() + 1);
    for (auto& arg : args) argv.push_back(arg.data());
    argv.push_back(nullptr);
    ::execv(argv[0], argv.data());

    write_child_errno_noexcept(pipe_fds[1], errno);
    _exit(127);
  }

  (void)::close(pipe_fds[1]);
  int exec_errno = 0;
  const ssize_t n = ::read(pipe_fds[0], &exec_errno, sizeof(exec_errno));
  (void)::close(pipe_fds[0]);
  int ignored_status = 0;
  while (::waitpid(child, &ignored_status, 0) < 0 && errno == EINTR) {
  }
  if (n > 0) {
    if (error) {
      *error = "cannot exec detached super loop runner: errno=" +
               std::to_string(exec_errno);
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool handle_tool_start_loop(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error);
[[nodiscard]] bool handle_tool_list_loops(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error);
[[nodiscard]] bool handle_tool_get_loop(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error);
[[nodiscard]] bool handle_tool_stop_loop(app_context_t* app,
                                         const std::string& arguments_json,
                                         std::string* out_structured,
                                         std::string* out_error);
[[nodiscard]] bool handle_tool_resume_loop(app_context_t* app,
                                           const std::string& arguments_json,
                                           std::string* out_structured,
                                           std::string* out_error);

#define HERO_SUPER_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER) \
  {NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
constexpr super_tool_descriptor_t kSuperTools[] = {
#include "hero/super_hero/hero_super_tools.def"
};
#undef HERO_SUPER_TOOL

[[nodiscard]] const super_tool_descriptor_t* find_super_tool_descriptor(
    std::string_view tool_name) {
  for (const auto& tool : kSuperTools) {
    if (tool.name == tool_name) return &tool;
  }
  return nullptr;
}

[[nodiscard]] bool handle_tool_start_loop(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string super_objective_dsl_path{};
  (void)extract_json_string_field(arguments_json, "super_objective_dsl_path",
                                  &super_objective_dsl_path);
  super_objective_dsl_path = trim_ascii(super_objective_dsl_path);

  const std::filesystem::path source_super_objective_dsl_path =
      resolve_requested_super_objective_source_path(*app, super_objective_dsl_path,
                                                   out_error);
  if (source_super_objective_dsl_path.empty()) return false;

  cuwacunu::hero::super::super_loop_record_t loop{};
  if (!initialize_super_loop_record(*app, source_super_objective_dsl_path, &loop,
                                    out_error)) {
    return false;
  }

  std::vector<std::string> warnings{};
  std::string campaign_cursor{};
  std::string campaign_json{"null"};
  const std::uint64_t review_index = 1;
  std::string review_packet_json{};
  if (!build_super_prelaunch_review_packet_json(*app, loop, review_index,
                                                &review_packet_json,
                                                out_error)) {
    loop.state = "failed";
    loop.state_detail = *out_error;
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_super_loop(*app, loop, &ignored);
    (void)append_super_loop_event(loop, "review_failed", *out_error, &ignored);
    return false;
  }
  const std::filesystem::path review_packet_path =
      cuwacunu::hero::super::super_loop_review_packet_path(
          app->defaults.super_root, loop.loop_id, review_index);
  if (!cuwacunu::hero::runtime::write_text_file_atomic(review_packet_path,
                                                       review_packet_json,
                                                       out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          super_loop_latest_review_packet_path(loop), review_packet_json,
          out_error)) {
    return false;
  }

  loop.state = "reviewing";
  loop.state_detail = "waiting for codex review before first launch";
  loop.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop.last_review_packet_path = review_packet_path.string();
  if (!write_super_loop(*app, loop, out_error)) return false;
  if (!append_super_loop_event(loop, "review_started", review_packet_path.string(),
                               out_error)) {
    return false;
  }

  super_review_decision_t decision{};
  std::string review_warning{};
  if (!run_super_review_with_codex(*app, &loop, review_index, &decision,
                                   &review_warning, out_error)) {
    cuwacunu::hero::super::super_loop_record_t terminal_loop{};
    if (reload_terminal_super_loop_if_any(*app, loop.loop_id, &terminal_loop)) {
      loop = std::move(terminal_loop);
      warnings.push_back("review interrupted after loop became terminal");
      *out_structured =
          "{\"loop_id\":" + json_quote(loop.loop_id) + ",\"loop\":" +
          super_loop_to_json(loop) +
          ",\"campaign_cursor\":\"\",\"campaign\":null,\"warnings\":" +
          json_array_from_strings(warnings) + "}";
      return true;
    }
    loop.state = "failed";
    loop.state_detail = *out_error;
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_super_loop(*app, loop, &ignored);
    (void)append_super_loop_event(loop, "review_failed", *out_error, &ignored);
    return false;
  }
  if (!validate_super_review_decision_action(*app, loop, decision, out_error)) {
    cuwacunu::hero::super::super_loop_record_t terminal_loop{};
    if (reload_terminal_super_loop_if_any(*app, loop.loop_id, &terminal_loop)) {
      loop = std::move(terminal_loop);
      warnings.push_back("review validation interrupted after loop became terminal");
      *out_structured =
          "{\"loop_id\":" + json_quote(loop.loop_id) + ",\"loop\":" +
          super_loop_to_json(loop) +
          ",\"campaign_cursor\":\"\",\"campaign\":null,\"warnings\":" +
          json_array_from_strings(warnings) + "}";
      return true;
    }
    loop.state = "failed";
    loop.state_detail = *out_error;
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_super_loop(*app, loop, &ignored);
    (void)append_super_loop_event(loop, "review_failed", *out_error, &ignored);
    return false;
  }

  append_memory_note(loop, review_index, decision.memory_note);
  loop.review_count = review_index;
  loop.last_control_kind = decision.control_kind;
  loop.last_warning.clear();
  if (!review_warning.empty()) append_warning_text(&loop.last_warning, review_warning);
  loop.last_decision_path =
      cuwacunu::hero::super::super_loop_decision_path(
          app->defaults.super_root, loop.loop_id, review_index)
          .string();
  loop.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();

  if (decision.control_kind == "stop" ||
      decision.control_kind == "need_human") {
    loop.state = (decision.control_kind == "stop") ? "stopped" : "need_human";
    loop.state_detail = decision.reason;
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    loop.active_campaign_cursor.clear();
    if (!write_super_loop(*app, loop, out_error)) return false;
    std::string warning{};
    std::string artifact_error{};
    if (decision.control_kind == "need_human" &&
        !write_human_request_note(loop, review_index, decision,
                                  &artifact_error)) {
      append_warning_text(&warning,
                          "human_request artifact write degraded: " +
                              artifact_error);
    }
    std::string event_error{};
    if (!append_super_loop_event(loop, "review_finished", decision.reason,
                                 &event_error)) {
      append_warning_text(&warning,
                          "final review event append degraded: " + event_error);
    }
    if (!warning.empty()) {
      warnings.push_back(warning);
      persist_super_loop_warning_best_effort(*app, &loop, warning);
    }
    if (!review_warning.empty()) warnings.push_back(review_warning);
    *out_structured =
        "{\"loop_id\":" + json_quote(loop.loop_id) + ",\"loop\":" +
        super_loop_to_json(loop) + ",\"campaign_cursor\":\"\",\"campaign\":null"
        + ",\"warnings\":" + json_array_from_strings(warnings) + "}";
    return true;
  }

  std::string launch_warning{};
  if (!launch_runtime_campaign_for_decision(
          *app, &loop, decision, "campaign launched under Super Hero",
          "campaign_launched", &campaign_cursor, &campaign_json, &launch_warning,
          out_error)) {
    loop.state = "failed";
    loop.state_detail = *out_error;
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_super_loop(*app, loop, &ignored);
    (void)append_super_loop_event(loop, "launch_failed", *out_error, &ignored);
    return false;
  }
  if (!launch_warning.empty()) warnings.push_back(launch_warning);
  if (!review_warning.empty()) warnings.push_back(review_warning);

  std::string bookkeeping_error{};
  if (!launch_loop_runner_process(*app, loop.loop_id, &bookkeeping_error)) {
    warnings.push_back("super loop runner launch failed: " + bookkeeping_error);
    std::string ignored{};
    (void)call_runtime_tool(
        *app, "hero.runtime.stop_campaign",
        "{\"campaign_cursor\":" + json_quote(campaign_cursor) + ",\"force\":true}",
        &ignored, nullptr);
    loop.state = "failed";
    loop.state_detail = "loop runner launch failed: " + bookkeeping_error;
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    (void)write_super_loop(*app, loop, &ignored);
    (void)append_super_loop_event(loop, "loop_runner_failed", bookkeeping_error,
                                  &ignored);
    *out_error = bookkeeping_error;
    return false;
  }
  (void)append_super_loop_event(loop, "loop_runner_started",
                                "detached super runner launched", nullptr);
  if (!warnings.empty()) {
    std::string combined_warning{};
    for (const std::string& warning : warnings) {
      append_warning_text(&combined_warning, warning);
    }
    persist_super_loop_warning_best_effort(*app, &loop, combined_warning);
  }

  *out_structured =
      "{\"loop_id\":" + json_quote(loop.loop_id) + ",\"loop\":" +
      super_loop_to_json(loop) + ",\"campaign_cursor\":" +
      json_quote(campaign_cursor) + ",\"campaign\":" + campaign_json +
      ",\"warnings\":" + json_array_from_strings(warnings) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_list_loops(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string state_filter{};
  std::size_t limit = 0;
  std::size_t offset = 0;
  bool newest_first = true;
  (void)extract_json_string_field(arguments_json, "state", &state_filter);
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);
  (void)extract_json_bool_field(arguments_json, "newest_first", &newest_first);
  state_filter = trim_ascii(state_filter);

  std::vector<cuwacunu::hero::super::super_loop_record_t> loops{};
  if (!list_super_loops(*app, &loops, out_error)) return false;
  if (!state_filter.empty()) {
    loops.erase(std::remove_if(loops.begin(), loops.end(),
                               [&](const auto& loop) {
                                 return loop.state != state_filter;
                               }),
                loops.end());
  }
  std::sort(loops.begin(), loops.end(), [newest_first](const auto& a,
                                                       const auto& b) {
    if (a.started_at_ms != b.started_at_ms) {
      return newest_first ? (a.started_at_ms > b.started_at_ms)
                          : (a.started_at_ms < b.started_at_ms);
    }
    return newest_first ? (a.loop_id > b.loop_id) : (a.loop_id < b.loop_id);
  });

  const std::size_t total = loops.size();
  const std::size_t off = std::min(offset, loops.size());
  std::size_t count = limit;
  if (count == 0) count = loops.size() - off;
  count = std::min(count, loops.size() - off);

  std::ostringstream loops_json;
  loops_json << "[";
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0) loops_json << ",";
    loops_json << super_loop_to_json(loops[off + i]);
  }
  loops_json << "]";

  *out_structured =
      "{\"loop_id\":\"\",\"count\":" + std::to_string(count) +
      ",\"total\":" + std::to_string(total) + ",\"state\":" +
      json_quote(state_filter) + ",\"loops\":" + loops_json.str() + "}";
  return true;
}

[[nodiscard]] bool handle_tool_get_loop(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string loop_id{};
  (void)extract_json_string_field(arguments_json, "loop_id", &loop_id);
  loop_id = trim_ascii(loop_id);
  if (loop_id.empty()) {
    *out_error = "get_loop requires arguments.loop_id";
    return false;
  }
  cuwacunu::hero::super::super_loop_record_t loop{};
  if (!read_super_loop(*app, loop_id, &loop, out_error)) return false;
  *out_structured = "{\"loop_id\":" + json_quote(loop_id) + ",\"loop\":" +
                    super_loop_to_json(loop) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_stop_loop(app_context_t* app,
                                         const std::string& arguments_json,
                                         std::string* out_structured,
                                         std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string loop_id{};
  bool force = false;
  (void)extract_json_string_field(arguments_json, "loop_id", &loop_id);
  (void)extract_json_bool_field(arguments_json, "force", &force);
  loop_id = trim_ascii(loop_id);
  if (loop_id.empty()) {
    *out_error = "stop_loop requires arguments.loop_id";
    return false;
  }

  cuwacunu::hero::super::super_loop_record_t loop{};
  if (!read_super_loop(*app, loop_id, &loop, out_error)) return false;
  if (is_super_loop_terminal_state(loop.state)) {
    *out_structured = "{\"loop_id\":" + json_quote(loop_id) + ",\"loop\":" +
                      super_loop_to_json(loop) + "}";
    return true;
  }

  const std::string active_campaign_cursor = trim_ascii(loop.active_campaign_cursor);
  loop.state = "stopped";
  loop.state_detail = "stop_loop requested by operator";
  loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop.updated_at_ms = *loop.finished_at_ms;
  loop.active_campaign_cursor.clear();
  if (!write_super_loop(*app, loop, out_error)) return false;
  if (!append_super_loop_event(loop, "loop_stopped", loop.state_detail,
                               out_error)) {
    return false;
  }

  std::string runtime_warning{};
  cancel_loop_review_best_effort(loop, force, &runtime_warning);
  if (!active_campaign_cursor.empty()) {
    std::string ignored{};
    std::string runtime_error{};
    if (!call_runtime_tool(
            *app, "hero.runtime.stop_campaign",
            "{\"campaign_cursor\":" + json_quote(active_campaign_cursor) +
                ",\"force\":" + bool_json(force) + "}",
            &ignored, &runtime_error)) {
      runtime_warning = "active campaign stop degraded: " + runtime_error;
      persist_super_loop_warning_best_effort(*app, &loop, runtime_warning);
    }
  }

  *out_structured = "{\"loop_id\":" + json_quote(loop_id) + ",\"loop\":" +
                    super_loop_to_json(loop) + ",\"warning\":" +
                    json_quote(runtime_warning) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_resume_loop(app_context_t* app,
                                           const std::string& arguments_json,
                                           std::string* out_structured,
                                           std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string loop_id{};
  std::string response_path_arg{};
  std::string response_sig_path_arg{};
  (void)extract_json_string_field(arguments_json, "loop_id", &loop_id);
  (void)extract_json_string_field(arguments_json, "human_response_path",
                                  &response_path_arg);
  (void)extract_json_string_field(arguments_json, "human_response_sig_path",
                                  &response_sig_path_arg);
  loop_id = trim_ascii(loop_id);
  if (loop_id.empty()) {
    *out_error = "resume_loop requires arguments.loop_id";
    return false;
  }

  cuwacunu::hero::super::super_loop_record_t loop{};
  if (!read_super_loop(*app, loop_id, &loop, out_error)) return false;
  if (loop.state != "need_human") {
    *out_error = "resume_loop requires loop.state=need_human";
    return false;
  }

  const std::filesystem::path response_path =
      trim_ascii(response_path_arg).empty()
          ? cuwacunu::hero::super::super_loop_human_response_latest_path(
                app->defaults.super_root, loop.loop_id)
          : std::filesystem::path(resolve_path_from_base_folder(
                app->global_config_path.parent_path().string(),
                trim_ascii(response_path_arg)));
  const std::filesystem::path response_sig_path =
      trim_ascii(response_sig_path_arg).empty()
          ? cuwacunu::hero::super::super_loop_human_response_latest_sig_path(
                app->defaults.super_root, loop.loop_id)
          : std::filesystem::path(resolve_path_from_base_folder(
                app->global_config_path.parent_path().string(),
                trim_ascii(response_sig_path_arg)));

  cuwacunu::hero::human::human_response_record_t response{};
  std::string signature_hex{};
  if (!load_verified_human_response(*app, loop, response_path, response_sig_path,
                                    &response, &signature_hex, out_error)) {
    return false;
  }

  const super_review_decision_t decision =
      human_response_to_super_decision(response);
  if (!validate_super_review_decision_action(*app, loop, decision, out_error)) {
    return false;
  }

  if (!trim_ascii(decision.memory_note).empty()) {
    append_memory_note(loop, loop.review_count,
                       "Human response (" + response.operator_id + "):\n" +
                           decision.memory_note);
  }
  loop.last_control_kind = decision.control_kind;
  loop.last_warning.clear();
  loop.finished_at_ms.reset();
  loop.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();

  std::string event_detail = response_path.string();
  if (!response.operator_id.empty()) {
    event_detail.append(" by ");
    event_detail.append(response.operator_id);
  }
  if (!append_super_loop_event(loop, "human_response_verified", event_detail,
                               out_error)) {
    return false;
  }

  if (decision.control_kind == "stop") {
    loop.state = "stopped";
    loop.state_detail = "stopped by human response: " + decision.reason;
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    loop.active_campaign_cursor.clear();
    if (!write_super_loop(*app, loop, out_error)) return false;
    if (!append_super_loop_event(loop, "loop_stopped_by_human", decision.reason,
                                 out_error)) {
      return false;
    }
    *out_structured =
        "{\"loop_id\":" + json_quote(loop_id) + ",\"loop\":" +
        super_loop_to_json(loop) + ",\"campaign_cursor\":\"\",\"campaign\":null"
        + ",\"warning\":\"\"}";
    return true;
  }

  std::string campaign_cursor{};
  std::string campaign_json{"null"};
  std::string launch_warning{};
  if (!launch_runtime_campaign_for_decision(
          *app, &loop, decision, "campaign launched from human response",
          "campaign_launched_from_human_response", &campaign_cursor,
          &campaign_json, &launch_warning, out_error)) {
    loop.state = "failed";
    loop.state_detail = *out_error;
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_super_loop(*app, loop, &ignored);
    (void)append_super_loop_event(loop, "human_resume_failed", *out_error,
                                  &ignored);
    return false;
  }

  std::vector<std::string> warnings{};
  if (!launch_warning.empty()) warnings.push_back(launch_warning);
  std::string runner_error{};
  if (!launch_loop_runner_process(*app, loop.loop_id, &runner_error)) {
    warnings.push_back("super loop runner launch failed: " + runner_error);
    std::string ignored{};
    (void)call_runtime_tool(
        *app, "hero.runtime.stop_campaign",
        "{\"campaign_cursor\":" + json_quote(campaign_cursor) + ",\"force\":true}",
        &ignored, nullptr);
    loop.state = "failed";
    loop.state_detail = "loop runner launch failed after human response: " +
                        runner_error;
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    (void)write_super_loop(*app, loop, &ignored);
    (void)append_super_loop_event(loop, "loop_runner_failed", runner_error,
                                  &ignored);
    *out_error = runner_error;
    return false;
  }
  (void)append_super_loop_event(loop, "loop_runner_started",
                                "detached super runner relaunched from human response",
                                nullptr);
  if (!warnings.empty()) {
    std::string combined_warning{};
    for (const std::string& warning : warnings) {
      append_warning_text(&combined_warning, warning);
    }
    persist_super_loop_warning_best_effort(*app, &loop, combined_warning);
  }

  *out_structured =
      "{\"loop_id\":" + json_quote(loop_id) + ",\"loop\":" +
      super_loop_to_json(loop) + ",\"campaign_cursor\":" +
      json_quote(campaign_cursor) + ",\"campaign\":" + campaign_json +
      ",\"warnings\":" + json_array_from_strings(warnings) + "}";
  return true;
}

}  // namespace

std::filesystem::path resolve_super_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "REAL_HERO", "super_hero_dsl_filename");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved);
}

bool load_super_defaults(const std::filesystem::path& hero_dsl_path,
                         const std::filesystem::path& global_config_path,
                         super_defaults_t* out, std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "super defaults output pointer is null";
    return false;
  }
  *out = super_defaults_t{};

  std::ifstream in(hero_dsl_path);
  if (!in) {
    if (error) {
      *error = "cannot open Super HERO defaults DSL: " +
               hero_dsl_path.string();
    }
    return false;
  }

  std::unordered_map<std::string, std::string> values{};
  std::string line{};
  bool in_block_comment = false;
  while (std::getline(in, line)) {
    std::string candidate = line;
    if (in_block_comment) {
      const std::size_t end_block = candidate.find("*/");
      if (end_block == std::string::npos) continue;
      candidate = candidate.substr(end_block + 2);
      in_block_comment = false;
    }
    const std::size_t start_block = candidate.find("/*");
    if (start_block != std::string::npos) {
      const std::size_t end_block = candidate.find("*/", start_block + 2);
      if (end_block == std::string::npos) {
        candidate = candidate.substr(0, start_block);
        in_block_comment = true;
      } else {
        candidate.erase(start_block, end_block - start_block + 2);
      }
    }
    candidate = trim_ascii(strip_inline_comment(candidate));
    if (candidate.empty()) continue;
    const std::size_t eq = candidate.find('=');
    if (eq == std::string::npos) continue;
    std::string lhs = trim_ascii(candidate.substr(0, eq));
    std::string rhs = unquote_if_wrapped(candidate.substr(eq + 1));
    lhs = cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(lhs);
    if (lhs.empty()) continue;
    values[lhs] = trim_ascii(rhs);
  }

  out->super_root = cuwacunu::hero::super::super_root(
      resolve_runtime_root_from_global_config(global_config_path));
  out->repo_root = resolve_repo_root_from_global_config(global_config_path);
  out->campaign_grammar_path =
      resolve_campaign_grammar_from_global_config(global_config_path);
  out->super_objective_grammar_path =
      resolve_super_objective_grammar_from_global_config(global_config_path);
  const auto config_scope_it = values.find("config_scope_root");
  if (config_scope_it != values.end() && !trim_ascii(config_scope_it->second).empty()) {
    out->config_scope_root = std::filesystem::path(resolve_path_from_base_folder(
        hero_dsl_path.parent_path().string(), config_scope_it->second));
  } else {
    out->config_scope_root = global_config_path.parent_path();
  }

  const auto resolve_exec = [&](const char* key, std::filesystem::path* dst) {
    const auto it = values.find(key);
    if (it == values.end()) return false;
    *dst = resolve_command_path(hero_dsl_path.parent_path(), it->second);
    return !dst->empty();
  };

  bool ok = true;
  ok = resolve_exec("runtime_hero_binary", &out->runtime_hero_binary) && ok;
  ok = resolve_exec("config_hero_binary", &out->config_hero_binary) && ok;
  ok = resolve_exec("lattice_hero_binary", &out->lattice_hero_binary) && ok;
  ok = resolve_exec("super_codex_binary", &out->super_codex_binary) && ok;
  if (!ok) {
    if (error) *error = "missing one or more binary fields in " + hero_dsl_path.string();
    return false;
  }
  const auto human_identities_it = values.find("human_operator_identities");
  if (human_identities_it != values.end() &&
      !trim_ascii(human_identities_it->second).empty()) {
    out->human_operator_identities = std::filesystem::path(
        resolve_path_from_base_folder(hero_dsl_path.parent_path().string(),
                                      human_identities_it->second));
  }
  if (!parse_size_token(values["tail_default_lines"], &out->tail_default_lines) ||
      out->tail_default_lines == 0) {
    if (error) *error = "missing/invalid tail_default_lines in " + hero_dsl_path.string();
    return false;
  }
  if (!parse_size_token(values["poll_interval_ms"], &out->poll_interval_ms) ||
      out->poll_interval_ms < 100) {
    if (error) *error = "missing/invalid poll_interval_ms in " + hero_dsl_path.string();
    return false;
  }
  if (!parse_size_token(values["super_codex_timeout_sec"],
                        &out->super_codex_timeout_sec) ||
      out->super_codex_timeout_sec == 0) {
    if (error) {
      *error = "missing/invalid super_codex_timeout_sec in " +
               hero_dsl_path.string();
    }
    return false;
  }
  if (!parse_size_token(values["super_max_reviews"], &out->super_max_reviews) ||
      out->super_max_reviews == 0) {
    if (error) *error = "missing/invalid super_max_reviews in " + hero_dsl_path.string();
    return false;
  }
  if (out->super_root.empty()) {
    if (error) {
      *error = "cannot derive .super_hero root from GENERAL.runtime_root in " +
               global_config_path.string();
    }
    return false;
  }
  if (out->repo_root.empty()) {
    if (error) {
      *error = "missing/invalid GENERAL.repo_root in " +
               global_config_path.string();
    }
    return false;
  }
  if (out->config_scope_root.empty()) {
    if (error) {
      *error = "missing/invalid config_scope_root in " + hero_dsl_path.string();
    }
    return false;
  }
  if (out->campaign_grammar_path.empty()) {
    if (error) {
      *error =
          "missing [BNF].iitepi_campaign_grammar_filename in " +
          global_config_path.string();
    }
    return false;
  }
  if (out->super_objective_grammar_path.empty()) {
    if (error) {
      *error =
          "missing [BNF].super_objective_grammar_filename in " +
          global_config_path.string();
    }
    return false;
  }
  return true;
}

std::filesystem::path current_executable_path() {
  std::array<char, 4096> buf{};
  const ssize_t n = ::readlink("/proc/self/exe", buf.data(), buf.size() - 1);
  if (n <= 0) return {};
  buf[static_cast<std::size_t>(n)] = '\0';
  return std::filesystem::path(buf.data()).lexically_normal();
}

bool tool_result_is_error(std::string_view tool_result_json) {
  return tool_result_json.find("\"isError\":true") != std::string_view::npos ||
         tool_result_json.find("\"isError\": true") != std::string_view::npos;
}

std::string build_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < std::size(kSuperTools); ++i) {
    const auto& tool = kSuperTools[i];
    if (i != 0) out << ",";
    out << "{\"name\":" << json_quote(tool.name)
        << ",\"description\":" << json_quote(tool.description)
        << ",\"inputSchema\":" << tool.input_schema_json << "}";
  }
  out << "]}";
  return out.str();
}

std::string build_tools_list_human_text() {
  std::ostringstream out;
  for (const auto& tool : kSuperTools) {
    out << tool.name << "\t" << tool.description << "\n";
  }
  return out.str();
}

bool execute_tool_json(const std::string& tool_name, std::string arguments_json,
                       app_context_t* app, std::string* out_tool_result_json,
                       std::string* out_error_message) {
  if (out_tool_result_json) out_tool_result_json->clear();
  if (out_error_message) out_error_message->clear();
  if (!app) {
    if (out_error_message) *out_error_message = "app context pointer is null";
    return false;
  }
  if (!out_tool_result_json || !out_error_message) return false;

  arguments_json = trim_ascii(arguments_json);
  if (arguments_json.empty()) arguments_json = "{}";

  const auto* descriptor = find_super_tool_descriptor(tool_name);
  if (descriptor == nullptr) {
    *out_error_message = "unknown tool: " + tool_name;
    return false;
  }

  std::string structured{};
  std::string err{};
  const bool ok = descriptor->handler(app, arguments_json, &structured, &err);
  if (!ok) {
    *out_tool_result_json = tool_result_json_for_error(err);
    return true;
  }
  *out_tool_result_json = make_tool_result_json("ok", structured, false);
  return true;
}

bool run_loop_runner(app_context_t* app, std::string_view loop_id,
                     std::string* error) {
  if (error) error->clear();
  if (!app) {
    if (error) *error = "super app pointer is null";
    return false;
  }

  cuwacunu::hero::super::super_loop_record_t loop{};
  if (!read_super_loop(*app, loop_id, &loop, error)) return false;
  loop_runner_lock_guard_t runner_lock{};
  bool already_locked = false;
  if (!acquire_loop_runner_lock(loop, &runner_lock, &already_locked, error)) {
    return false;
  }
  if (already_locked) return true;

  std::string ignored{};
  (void)append_super_loop_event(loop, "loop_runner_active",
                                "runner lock acquired", &ignored);
  for (;;) {
    if (!read_super_loop(*app, loop_id, &loop, error)) return false;
    if (is_super_loop_terminal_state(loop.state)) return true;

    const std::string active_campaign_cursor =
        trim_ascii(loop.active_campaign_cursor);
    if (active_campaign_cursor.empty()) {
      loop.state = "failed";
      loop.state_detail = "running loop has no active campaign cursor";
      loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      loop.updated_at_ms = *loop.finished_at_ms;
      if (!write_super_loop(*app, loop, error)) return false;
      (void)append_super_loop_event(loop, "loop_failed", loop.state_detail,
                                    &ignored);
      return false;
    }

    cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
    if (!read_runtime_campaign_direct(*app, active_campaign_cursor, &campaign,
                                      error)) {
      loop.state = "failed";
      loop.state_detail = *error;
      loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      loop.updated_at_ms = *loop.finished_at_ms;
      std::string ignored_local{};
      (void)write_super_loop(*app, loop, &ignored_local);
      (void)append_super_loop_event(loop, "loop_failed", loop.state_detail,
                                    &ignored_local);
      return false;
    }
    const std::string state = observed_campaign_state(campaign);
    if (!is_runtime_campaign_terminal_state(state)) {
      ::usleep(static_cast<useconds_t>(app->defaults.poll_interval_ms * 1000));
      continue;
    }

    std::string warning{};
    if (!continue_loop_after_terminal_campaign(app, active_campaign_cursor,
                                               &warning, error)) {
      return false;
    }
    if (!warning.empty()) {
      persist_super_loop_warning_best_effort(*app, &loop, warning);
    }
  }
}

[[nodiscard]] bool starts_with_case_insensitive(std::string_view value,
                                                std::string_view prefix) {
  if (value.size() < prefix.size()) return false;
  for (std::size_t i = 0; i < prefix.size(); ++i) {
    const char a = static_cast<char>(
        std::tolower(static_cast<unsigned char>(value[i])));
    const char b = static_cast<char>(
        std::tolower(static_cast<unsigned char>(prefix[i])));
    if (a != b) return false;
  }
  return true;
}

[[nodiscard]] bool parse_content_length_header(std::string_view header_line,
                                               std::size_t* out_length) {
  constexpr std::string_view kPrefix = "content-length:";
  const std::string line = trim_ascii(header_line);
  if (!starts_with_case_insensitive(line, kPrefix)) return false;
  const std::string number = trim_ascii(line.substr(kPrefix.size()));
  return parse_size_token(number, out_length);
}

[[nodiscard]] bool read_next_jsonrpc_message(std::istream* in,
                                             std::string* out_payload,
                                             bool* out_used_content_length) {
  if (out_used_content_length) *out_used_content_length = false;

  std::string first_line;
  while (std::getline(*in, first_line)) {
    const std::string trimmed = trim_ascii(first_line);
    if (trimmed.empty()) continue;

    std::size_t content_length = 0;
    bool saw_header_block = false;
    if (parse_content_length_header(trimmed, &content_length)) {
      saw_header_block = true;
    } else if (trimmed.front() != '{' && trimmed.front() != '[' &&
               trimmed.find(':') != std::string::npos) {
      saw_header_block = true;
    }

    if (saw_header_block) {
      std::string header_line;
      while (std::getline(*in, header_line)) {
        const std::string header_trimmed = trim_ascii(header_line);
        if (header_trimmed.empty()) break;
        std::size_t parsed_length = 0;
        if (parse_content_length_header(header_trimmed, &parsed_length)) {
          content_length = parsed_length;
        }
      }
      if (content_length == 0) continue;
      if (content_length > kMaxJsonRpcPayloadBytes) return false;

      std::string payload(content_length, '\0');
      in->read(payload.data(), static_cast<std::streamsize>(content_length));
      if (in->gcount() != static_cast<std::streamsize>(content_length)) {
        return false;
      }
      if (out_payload) *out_payload = std::move(payload);
      if (out_used_content_length) *out_used_content_length = true;
      return true;
    }

    if (out_payload) *out_payload = trimmed;
    return true;
  }
  return false;
}

void emit_jsonrpc_payload(std::string_view payload_json) {
  if (g_jsonrpc_use_content_length_framing) {
    std::cout << "Content-Length: " << payload_json.size() << "\r\n\r\n"
              << payload_json;
  } else {
    std::cout << payload_json << "\n";
  }
  std::cout.flush();
}

void write_jsonrpc_result(std::string_view id_json, std::string_view result_json) {
  emit_jsonrpc_payload("{\"jsonrpc\":\"2.0\",\"id\":" + std::string(id_json) +
                       ",\"result\":" + std::string(result_json) + "}");
}

void write_jsonrpc_error(std::string_view id_json, int code,
                         std::string_view message) {
  emit_jsonrpc_payload(
      "{\"jsonrpc\":\"2.0\",\"id\":" + std::string(id_json) +
      ",\"error\":{\"code\":" + std::to_string(code) +
      ",\"message\":" + json_quote(message) + "}}");
}

[[nodiscard]] bool parse_jsonrpc_id(const std::string& json,
                                    std::string* out_id_json) {
  std::string raw_id{};
  if (!extract_json_field_raw(json, "id", &raw_id)) return false;
  raw_id = trim_ascii(raw_id);
  if (raw_id.empty()) return false;
  if (raw_id.front() == '"') {
    std::string parsed;
    std::size_t end = 0;
    if (!parse_json_string_at(raw_id, 0, &parsed, &end)) return false;
    end = skip_json_whitespace(raw_id, end);
    if (end != raw_id.size()) return false;
    if (out_id_json) *out_id_json = json_quote(parsed);
    return true;
  }
  if (out_id_json) *out_id_json = raw_id;
  return true;
}

void run_jsonrpc_stdio_loop(app_context_t* app) {
  if (!app) return;

  for (;;) {
    std::string request_json{};
    bool used_content_length = false;
    if (!read_next_jsonrpc_message(&std::cin, &request_json,
                                   &used_content_length)) {
      break;
    }
    g_jsonrpc_use_content_length_framing = used_content_length;
    request_json = trim_ascii(request_json);
    if (request_json.empty()) continue;

    std::string id_json = "null";
    (void)parse_jsonrpc_id(request_json, &id_json);

    std::string method{};
    if (!extract_json_string_field(request_json, "method", &method) ||
        method.empty()) {
      write_jsonrpc_error(id_json, -32600, "invalid request: missing method");
      continue;
    }

    if (method == "initialize") {
      write_jsonrpc_result(
          id_json,
          "{\"protocolVersion\":" + json_quote(kProtocolVersion) +
              ",\"serverInfo\":{\"name\":" + json_quote(kServerName) +
              ",\"version\":" + json_quote(kServerVersion) +
              "},\"capabilities\":{\"tools\":{}},\"instructions\":" +
              json_quote(kInitializeInstructions) + "}");
      continue;
    }
    if (method == "notifications/initialized") continue;
    if (method == "tools/list") {
      write_jsonrpc_result(id_json, build_tools_list_result_json());
      continue;
    }
    if (method == "tools/call") {
      std::string params_json{};
      if (!extract_json_object_field(request_json, "params", &params_json)) {
        write_jsonrpc_error(id_json, -32602, "invalid params");
        continue;
      }
      std::string name{};
      if (!extract_json_string_field(params_json, "name", &name) ||
          name.empty()) {
        write_jsonrpc_error(id_json, -32602, "missing params.name");
        continue;
      }
      std::string arguments = "{}";
      (void)extract_json_object_field(params_json, "arguments", &arguments);

      std::string tool_result{};
      std::string tool_error{};
      if (!execute_tool_json(name, arguments, app, &tool_result, &tool_error)) {
        write_jsonrpc_error(id_json, -32001,
                            tool_error.empty() ? "tool execution failed"
                                              : tool_error);
        continue;
      }
      write_jsonrpc_result(id_json, tool_result);
      continue;
    }
    write_jsonrpc_error(id_json, -32601, "method not found");
  }
}

}  // namespace super_mcp
}  // namespace hero
}  // namespace cuwacunu
