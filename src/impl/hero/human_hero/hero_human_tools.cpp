// SPDX-License-Identifier: MIT
#include "hero/human_hero/hero_human_tools.h"

#include <algorithm>
#include <array>
#include <cctype>
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
#include <iostream>
#include <optional>
#include <pwd.h>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"
#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "hero/human_hero/human_attestation.h"
#include "hero/super_hero/super_loop.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"

namespace cuwacunu {
namespace hero {
namespace human_mcp {

namespace {

constexpr const char* kServerName = "hero_human_mcp";
constexpr const char* kServerVersion = "0.1.0";
constexpr const char* kProtocolVersion = "2025-03-26";
constexpr const char* kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.human.*.";
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;
constexpr std::size_t kSuperToolTimeoutSec = 30;
constexpr const char* kNcursesInitErrorPrefix = "ncurses init failed: ";
constexpr std::string_view kHumanReportAckSchemaV2 = "hero.human.report_ack.v2";

using cuwacunu::hero::human::human_resolution_record_t;

using human_tool_handler_t = bool (*)(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error);

struct human_tool_descriptor_t {
  std::string_view name;
  std::string_view description;
  std::string_view input_schema_json;
  human_tool_handler_t handler;
};

struct interactive_resolution_input_t {
  std::string resolution_kind{};
  std::string reason{};
};

[[nodiscard]] bool collect_pending_requests(
    const app_context_t& app,
    std::vector<cuwacunu::hero::super::super_loop_record_t>* out,
    std::string* error);

[[nodiscard]] bool collect_pending_reports(
    const app_context_t& app,
    std::vector<cuwacunu::hero::super::super_loop_record_t>* out,
    std::string* error);

[[nodiscard]] bool build_resolution_and_apply(
    app_context_t* app,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::string resolution_kind, std::string reason,
    std::string* out_structured, std::string* out_error);

[[nodiscard]] bool acknowledge_terminal_report(
    app_context_t* app,
    const cuwacunu::hero::super::super_loop_record_t& loop, std::string note,
    std::string* out_structured, std::string* out_error);

[[nodiscard]] bool dismiss_terminal_report(
    app_context_t* app,
    const cuwacunu::hero::super::super_loop_record_t& loop, std::string note,
    std::string* out_structured, std::string* out_error);

[[nodiscard]] bool list_declared_bind_ids_for_loop(
    const app_context_t& app,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::vector<std::string>* out_bind_ids, std::string* error);

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

[[nodiscard]] std::optional<std::uint64_t> super_loop_elapsed_ms(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  const std::uint64_t end_ms =
      loop.finished_at_ms.has_value() ? *loop.finished_at_ms : loop.updated_at_ms;
  if (loop.started_at_ms == 0 || end_ms < loop.started_at_ms) {
    return std::nullopt;
  }
  return end_ms - loop.started_at_ms;
}

[[nodiscard]] std::string format_compact_duration_ms(
    std::optional<std::uint64_t> duration_ms) {
  if (!duration_ms.has_value()) return "<unknown>";
  if (*duration_ms < 1000) return std::to_string(*duration_ms) + " ms";

  std::uint64_t total_seconds = *duration_ms / 1000;
  const std::uint64_t days = total_seconds / 86400;
  total_seconds %= 86400;
  const std::uint64_t hours = total_seconds / 3600;
  total_seconds %= 3600;
  const std::uint64_t minutes = total_seconds / 60;
  total_seconds %= 60;
  const std::uint64_t seconds = total_seconds;

  std::ostringstream out;
  bool wrote = false;
  const auto append_unit = [&](std::uint64_t value, const char* suffix) {
    if (value == 0) return;
    if (wrote) out << " ";
    out << value << suffix;
    wrote = true;
  };
  append_unit(days, "d");
  append_unit(hours, "h");
  append_unit(minutes, "m");
  if (!wrote || seconds != 0) append_unit(seconds, "s");
  return out.str();
}

[[nodiscard]] std::string format_effort_usage(std::uint64_t used,
                                              std::uint64_t limit,
                                              std::uint64_t remaining) {
  std::ostringstream out;
  out << used;
  if (limit != 0) out << "/" << limit;
  out << " (" << remaining << " rem)";
  return out.str();
}

[[nodiscard]] std::string build_report_effort_summary_text(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  std::ostringstream out;
  out << "elapsed=" << format_compact_duration_ms(super_loop_elapsed_ms(loop))
      << " | review_turns="
      << format_effort_usage(loop.turn_count, loop.max_review_turns,
                             loop.remaining_review_turns)
      << " | launches="
      << format_effort_usage(loop.launch_count, loop.max_campaign_launches,
                             loop.remaining_campaign_launches);
  return out.str();
}

[[nodiscard]] bool terminal_supports_human_ncurses_ui() {
  const char* term_env = std::getenv("TERM");
  const std::string term =
      lowercase_copy(trim_ascii(term_env == nullptr ? "" : term_env));
  if (term.empty() || term == "dumb" || term == "unknown") return false;
  if (::isatty(STDIN_FILENO) == 0 || ::isatty(STDOUT_FILENO) == 0) return false;
  return true;
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

[[nodiscard]] std::string json_escape(std::string_view in) {
  std::ostringstream out;
  for (const unsigned char ch : in) {
    switch (ch) {
      case '\\':
        out << "\\\\";
        break;
      case '"':
        out << "\\\"";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
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
        if (ch < 0x20) {
          out << "\\u00";
          constexpr char kHex[] = "0123456789abcdef";
          out << kHex[(ch >> 4) & 0x0f] << kHex[ch & 0x0f];
        } else {
          out << static_cast<char>(ch);
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

[[nodiscard]] bool operator_id_needs_bootstrap(std::string_view value) {
  const std::string trimmed = trim_ascii(value);
  return trimmed.empty() || trimmed == "CHANGE_ME_OPERATOR";
}

[[nodiscard]] std::string current_user_name() {
  const char* env_user = std::getenv("USER");
  if (env_user != nullptr) {
    const std::string trimmed = trim_ascii(env_user);
    if (!trimmed.empty()) return trimmed;
  }
  const char* env_logname = std::getenv("LOGNAME");
  if (env_logname != nullptr) {
    const std::string trimmed = trim_ascii(env_logname);
    if (!trimmed.empty()) return trimmed;
  }
  errno = 0;
  if (const passwd* pw = ::getpwuid(::geteuid()); pw != nullptr &&
                                                 pw->pw_name != nullptr) {
    const std::string trimmed = trim_ascii(pw->pw_name);
    if (!trimmed.empty()) return trimmed;
  }
  return "operator";
}

[[nodiscard]] std::string current_host_name() {
  std::array<char, 256> host{};
  if (::gethostname(host.data(), host.size() - 1) == 0) {
    host.back() = '\0';
    const std::string trimmed = trim_ascii(host.data());
    if (!trimmed.empty()) return trimmed;
  }
  return "localhost";
}

[[nodiscard]] std::string derive_bootstrap_operator_id() {
  return current_user_name() + "@" + current_host_name();
}

[[nodiscard]] bool persist_bootstrap_operator_id(
    const std::filesystem::path& hero_dsl_path, std::string_view operator_id,
    std::string* error) {
  if (error) error->clear();
  std::ifstream in(hero_dsl_path);
  if (!in) {
    if (error) {
      *error = "cannot open Human HERO defaults DSL for operator bootstrap: " +
               hero_dsl_path.string();
    }
    return false;
  }

  std::vector<std::string> lines{};
  std::string line{};
  bool replaced = false;
  bool in_block_comment = false;
  while (std::getline(in, line)) {
    std::string candidate = line;
    if (in_block_comment) {
      const std::size_t end_block = candidate.find("*/");
      if (end_block == std::string::npos) {
        lines.push_back(line);
        continue;
      }
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
    if (!candidate.empty()) {
      const std::size_t eq = candidate.find('=');
      if (eq != std::string::npos) {
        std::string lhs = trim_ascii(candidate.substr(0, eq));
        lhs = cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(lhs);
        if (lhs == "operator_id") {
          line = "operator_id:str = " + std::string(operator_id) +
                 " # auto-initialized on first Human Hero use";
          replaced = true;
        }
      }
    }
    lines.push_back(line);
  }

  if (!replaced) {
    lines.push_back("operator_id:str = " + std::string(operator_id) +
                    " # auto-initialized on first Human Hero use");
  }

  std::ostringstream out;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    out << lines[i];
    if (i + 1 != lines.size()) out << "\n";
  }
  out << "\n";
  return cuwacunu::hero::runtime::write_text_file_atomic(hero_dsl_path, out.str(),
                                                         error);
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

[[nodiscard]] bool parse_u64_token(std::string_view raw, std::uint64_t* out) {
  if (!out) return false;
  const std::string token = trim_ascii(raw);
  if (token.empty()) return false;
  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(token.data(), token.data() + token.size(), parsed);
  if (ec != std::errc{} || ptr != token.data() + token.size()) return false;
  *out = parsed;
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
        case 'u':
          if (pos + 4 > json.size()) return false;
          pos += 4;
          parsed.push_back('?');
          break;
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
  if (first == '"') return parse_json_string_at(json, pos, nullptr, out_end);
  if (first == '{' || first == '[') {
    int depth = 0;
    bool in_string = false;
    for (std::size_t i = pos; i < json.size(); ++i) {
      const char c = json[i];
      if (in_string) {
        if (c == '\\') {
          ++i;
          continue;
        }
        if (c == '"') in_string = false;
        continue;
      }
      if (c == '"') {
        in_string = true;
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
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw)) return false;
  std::size_t end = 0;
  return parse_json_string_at(raw, 0, out, &end) &&
         skip_json_whitespace(raw, end) == raw.size();
}

[[nodiscard]] bool extract_json_bool_field(const std::string& json,
                                           std::string_view key, bool* out) {
  std::string raw{};
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
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw)) return false;
  return parse_size_token(raw, out);
}

[[nodiscard]] bool extract_json_u64_field(const std::string& json,
                                          std::string_view key,
                                          std::uint64_t* out) {
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw)) return false;
  return parse_u64_token(raw, out);
}

[[nodiscard]] bool extract_json_object_field(const std::string& json,
                                             std::string_view key,
                                             std::string* out) {
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw)) return false;
  raw = trim_ascii(raw);
  if (raw.empty() || raw.front() != '{' || raw.back() != '}') return false;
  if (out) *out = std::move(raw);
  return true;
}

[[nodiscard]] bool tool_result_is_error_impl(std::string_view tool_result_json) {
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
  if (tool_result_is_error_impl(json)) {
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

[[nodiscard]] std::filesystem::path resolve_runtime_root_from_global_config(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "GENERAL", "runtime_root");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved).lexically_normal();
}

[[nodiscard]] std::filesystem::path resolve_campaign_grammar_path(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "BNF",
                     "iitepi_campaign_grammar_filename");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved).lexically_normal();
}

[[nodiscard]] bool list_declared_bind_ids_for_loop(
    const app_context_t& app,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::vector<std::string>* out_bind_ids, std::string* error) {
  if (error) error->clear();
  if (!out_bind_ids) {
    if (error) *error = "bind id output pointer is null";
    return false;
  }
  out_bind_ids->clear();

  const std::filesystem::path campaign_grammar_path =
      resolve_campaign_grammar_path(app.global_config_path);
  if (campaign_grammar_path.empty()) {
    if (error) *error = "cannot resolve BNF.iitepi_campaign_grammar_filename";
    return false;
  }

  std::string grammar_text{};
  if (!cuwacunu::hero::runtime::read_text_file(campaign_grammar_path,
                                               &grammar_text, error)) {
    return false;
  }
  std::string campaign_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.campaign_dsl_path,
                                               &campaign_text, error)) {
    return false;
  }

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  try {
    instruction = cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
        grammar_text, campaign_text);
  } catch (const std::exception& ex) {
    if (error) {
      *error = "failed to decode campaign DSL snapshot '" + loop.campaign_dsl_path +
               "': " + ex.what();
    }
    return false;
  }

  out_bind_ids->reserve(instruction.binds.size());
  for (const auto& bind : instruction.binds) {
    const std::string bind_id = trim_ascii(bind.id);
    if (!bind_id.empty()) out_bind_ids->push_back(bind_id);
  }
  return true;
}

[[nodiscard]] std::filesystem::path resolve_command_path(
    const std::filesystem::path& base_dir, std::string_view configured_value) {
  const std::string configured = trim_ascii(configured_value);
  if (configured.empty()) return {};
  const std::filesystem::path raw(configured);
  if (configured.find('/') != std::string::npos ||
      configured.find('\\') != std::string::npos) {
    if (raw.is_absolute()) return raw.lexically_normal();
    return (base_dir / raw).lexically_normal();
  }
  if (const char* env_path = std::getenv("PATH"); env_path != nullptr) {
    std::stringstream in(env_path);
    std::string entry{};
    while (std::getline(in, entry, ':')) {
      const std::filesystem::path candidate =
          (std::filesystem::path(entry) / configured).lexically_normal();
      if (::access(candidate.c_str(), X_OK) == 0) return candidate;
    }
  }
  return std::filesystem::path(configured);
}

[[nodiscard]] bool run_command_with_stdio_and_timeout(
    const std::vector<std::string>& argv, const std::filesystem::path& stdin_path,
    const std::filesystem::path& stdout_path,
    const std::filesystem::path& stderr_path, std::size_t timeout_sec,
    int* out_exit_code, std::string* error) {
  if (error) error->clear();
  if (argv.empty()) {
    if (error) *error = "command argv is empty";
    return false;
  }
  if (out_exit_code) *out_exit_code = -1;

  std::error_code ec{};
  if (!std::filesystem::exists(stdin_path, ec) ||
      !std::filesystem::is_regular_file(stdin_path, ec)) {
    if (error) *error = "command stdin path does not exist: " + stdin_path.string();
    return false;
  }
  const int stdout_probe =
      ::open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (stdout_probe < 0) {
    if (error) *error = "cannot open stdout path for command: " + stdout_path.string();
    return false;
  }
  (void)::close(stdout_probe);
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
    const int stdin_fd = ::open(stdin_path.c_str(), O_RDONLY);
    if (stdin_fd < 0) _exit(126);
    (void)::dup2(stdin_fd, STDIN_FILENO);
    if (stdin_fd > STDERR_FILENO) (void)::close(stdin_fd);
    const int stdout_fd =
        ::open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
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
      return false;
    }
    if (std::chrono::steady_clock::now() >= deadline) {
      (void)::kill(-child, SIGKILL);
      (void)::kill(child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      if (error) *error = "command timed out";
      return false;
    }
    ::usleep(100 * 1000);
  }

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

[[nodiscard]] std::filesystem::path human_tool_io_dir(const app_context_t& app) {
  return app.defaults.super_root / ".human_tool_io";
}

[[nodiscard]] std::filesystem::path make_human_tool_io_path(
    const app_context_t& app, std::string_view stem) {
  const std::uint64_t salt =
      static_cast<std::uint64_t>(
          std::chrono::steady_clock::now().time_since_epoch().count());
  return human_tool_io_dir(app) /
         ("human_tool." + std::to_string(cuwacunu::hero::runtime::now_ms_utc()) +
          "." + std::to_string(static_cast<unsigned long long>(::getpid())) + "." +
          cuwacunu::hero::runtime::hex_lower_u64(salt).substr(0, 8) + "." +
          std::string(stem));
}

void cleanup_human_tool_io(const std::filesystem::path& stdin_path,
                           const std::filesystem::path& stdout_path,
                           const std::filesystem::path& stderr_path) {
  std::error_code ec{};
  (void)std::filesystem::remove(stdin_path, ec);
  (void)std::filesystem::remove(stdout_path, ec);
  (void)std::filesystem::remove(stderr_path, ec);
}

[[nodiscard]] bool call_super_tool(const app_context_t& app,
                                   const std::string& tool_name,
                                   std::string arguments_json,
                                   std::string* out_structured,
                                   std::string* error) {
  if (error) error->clear();
  if (!out_structured) {
    if (error) *error = "super structured output pointer is null";
    return false;
  }
  *out_structured = "";

  std::error_code ec{};
  std::filesystem::create_directories(human_tool_io_dir(app), ec);
  if (ec) {
    if (error) {
      *error = "cannot create human tool io dir: " +
               human_tool_io_dir(app).string();
    }
    return false;
  }

  const std::filesystem::path stdin_path =
      make_human_tool_io_path(app, "stdin.json");
  const std::filesystem::path stdout_path =
      make_human_tool_io_path(app, "stdout.json");
  const std::filesystem::path stderr_path =
      make_human_tool_io_path(app, "stderr.log");
  if (!cuwacunu::hero::runtime::write_text_file_atomic(stdin_path, "", error)) {
    return false;
  }

  const std::vector<std::string> argv{
      app.defaults.super_hero_binary.string(), "--global-config",
      app.global_config_path.string(), "--tool", tool_name, "--args-json",
      arguments_json};
  int exit_code = -1;
  std::string invoke_error{};
  const bool invoked = run_command_with_stdio_and_timeout(
      argv, stdin_path, stdout_path, stderr_path, kSuperToolTimeoutSec,
      &exit_code, &invoke_error);

  std::string stdout_text{};
  std::string stderr_text{};
  std::string ignored{};
  (void)cuwacunu::hero::runtime::read_text_file(stdout_path, &stdout_text, &ignored);
  (void)cuwacunu::hero::runtime::read_text_file(stderr_path, &stderr_text, &ignored);
  cleanup_human_tool_io(stdin_path, stdout_path, stderr_path);

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
                   : ("super tool call produced no stdout: " + tool_name);
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
                   : ("super tool exited non-zero: " + tool_name);
    }
    return false;
  }
  return true;
}

[[nodiscard]] std::string request_excerpt(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  std::string text{};
  std::string ignored{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.human_escalation_path, &text,
                                               &ignored)) {
    return {};
  }
  std::istringstream in(text);
  std::ostringstream out;
  std::string line{};
  std::size_t count = 0;
  while (count < 8 && std::getline(in, line)) {
    if (count != 0) out << "\n";
    out << line;
    ++count;
  }
  return out.str();
}

[[nodiscard]] std::filesystem::path report_path_for_loop(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  return cuwacunu::hero::super::super_loop_human_report_path(
      std::filesystem::path(loop.loop_root).parent_path(), loop.loop_id);
}

[[nodiscard]] std::filesystem::path report_ack_path_for_loop(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  return cuwacunu::hero::super::super_loop_human_report_ack_latest_path(
      std::filesystem::path(loop.loop_root).parent_path(), loop.loop_id);
}

[[nodiscard]] std::filesystem::path report_ack_sig_path_for_loop(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  return cuwacunu::hero::super::super_loop_human_report_ack_latest_sig_path(
      std::filesystem::path(loop.loop_root).parent_path(), loop.loop_id);
}

[[nodiscard]] bool read_report_text_for_loop(
    const cuwacunu::hero::super::super_loop_record_t& loop, std::string* out,
    std::string* error) {
  if (!out) {
    if (error) *error = "report text output pointer is null";
    return false;
  }
  return cuwacunu::hero::runtime::read_text_file(report_path_for_loop(loop), out,
                                                 error);
}

[[nodiscard]] std::string report_excerpt(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  std::string text{};
  std::string ignored{};
  if (!read_report_text_for_loop(loop, &text, &ignored)) return {};
  std::istringstream in(text);
  std::ostringstream out;
  std::string line{};
  std::size_t count = 0;
  while (count < 8 && std::getline(in, line)) {
    if (count != 0) out << "\n";
    out << line;
    ++count;
  }
  return out.str();
}

[[nodiscard]] bool report_ack_matches_current_report(
    const cuwacunu::hero::super::super_loop_record_t& loop, std::string* error) {
  if (error) error->clear();
  const std::filesystem::path report_path = report_path_for_loop(loop);
  const std::filesystem::path ack_path = report_ack_path_for_loop(loop);
  if (!std::filesystem::exists(report_path) ||
      !std::filesystem::is_regular_file(report_path)) {
    if (error) *error = "loop summary report is missing";
    return false;
  }
  if (!std::filesystem::exists(ack_path) || !std::filesystem::is_regular_file(ack_path)) {
    return false;
  }
  std::string report_sha256_hex{};
  if (!cuwacunu::hero::human::sha256_hex_file(report_path, &report_sha256_hex,
                                              error)) {
    return false;
  }
  std::string ack_json{};
  if (!cuwacunu::hero::runtime::read_text_file(ack_path, &ack_json, error)) {
    return false;
  }
  std::string acknowledged_report_sha256_hex{};
  if (!extract_json_string_field(ack_json, "report_sha256_hex",
                                 &acknowledged_report_sha256_hex)) {
    if (error) *error = "report ack is missing report_sha256_hex";
    return false;
  }
  return trim_ascii(acknowledged_report_sha256_hex) ==
         trim_ascii(report_sha256_hex);
}

void sync_human_pending_markers_best_effort(const app_context_t& app) {
  std::string marker_error{};
  if (!cuwacunu::hero::super::sync_human_pending_request_count(
          app.defaults.super_root, &marker_error)) {
    std::cerr << "[hero_human_mcp][warning] failed to refresh Human Hero "
                 "request marker: "
              << marker_error << std::endl;
  }
  marker_error.clear();
  if (!cuwacunu::hero::super::sync_human_pending_report_count(
          app.defaults.super_root, &marker_error)) {
    std::cerr << "[hero_human_mcp][warning] failed to refresh Human Hero "
                 "report marker: "
              << marker_error << std::endl;
  }
}

[[nodiscard]] std::string ellipsize_text(std::string_view in, int width) {
  if (width <= 0) return {};
  if (static_cast<int>(in.size()) <= width) return std::string(in);
  if (width <= 3) return std::string(in.substr(0, static_cast<std::size_t>(width)));
  return std::string(in.substr(0, static_cast<std::size_t>(width - 3))) + "...";
}

[[nodiscard]] std::vector<std::string> wrap_text_lines(std::string_view text,
                                                       int width) {
  std::vector<std::string> lines{};
  if (width <= 1) {
    lines.emplace_back();
    return lines;
  }
  std::istringstream input{std::string(text)};
  std::string raw{};
  while (std::getline(input, raw)) {
    if (raw.empty()) {
      lines.emplace_back();
      continue;
    }
    std::istringstream words(raw);
    std::string word{};
    std::string current{};
    while (words >> word) {
      while (static_cast<int>(word.size()) > width) {
        if (!current.empty()) {
          lines.push_back(current);
          current.clear();
        }
        lines.push_back(word.substr(0, static_cast<std::size_t>(width)));
        word.erase(0, static_cast<std::size_t>(width));
      }
      if (current.empty()) {
        current = word;
        continue;
      }
      if (static_cast<int>(current.size() + 1 + word.size()) <= width) {
        current.append(" ").append(word);
      } else {
        lines.push_back(current);
        current = word;
      }
    }
    if (!current.empty()) lines.push_back(current);
  }
  if (lines.empty()) lines.emplace_back();
  return lines;
}

[[nodiscard]] std::vector<std::string> wrap_input_lines(std::string_view text,
                                                        int width) {
  std::vector<std::string> lines{};
  if (width <= 0) {
    lines.emplace_back();
    return lines;
  }
  lines.emplace_back();
  for (char ch : text) {
    if (ch == '\n') {
      lines.emplace_back();
      continue;
    }
    if (static_cast<int>(lines.back().size()) >= width) {
      lines.emplace_back();
    }
    lines.back().push_back(ch);
  }
  if (lines.empty()) lines.emplace_back();
  return lines;
}

void init_human_ncurses_theme() {
  if (!has_colors()) return;
  init_pair(1, COLOR_CYAN, COLOR_BLACK);
  init_pair(2, COLOR_BLACK, COLOR_CYAN);
  init_pair(3, COLOR_YELLOW, COLOR_BLACK);
  init_pair(4, COLOR_RED, COLOR_BLACK);
  init_pair(5, COLOR_GREEN, COLOR_BLACK);
  init_pair(6, COLOR_WHITE, COLOR_BLACK);
  bkgd(COLOR_PAIR(6) | ' ');
  attrset(COLOR_PAIR(6));
}

void draw_boxed_window(WINDOW* win, std::string_view title) {
  if (!win) return;
  if (has_colors()) {
    wbkgdset(win, COLOR_PAIR(6) | ' ');
    wattrset(win, COLOR_PAIR(6));
  }
  werase(win);
  box(win, 0, 0);
  if (!title.empty()) {
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    mvwaddnstr(win, 0, 2, title.data(), static_cast<int>(title.size()));
    wattroff(win, COLOR_PAIR(1) | A_BOLD);
  }
}

void draw_boxed_region(int y, int x, int height, int width,
                       std::string_view title) {
  if (height < 2 || width < 2) return;
  mvaddch(y, x, ACS_ULCORNER);
  mvhline(y, x + 1, ACS_HLINE, width - 2);
  mvaddch(y, x + width - 1, ACS_URCORNER);
  for (int row = 1; row < height - 1; ++row) {
    mvaddch(y + row, x, ACS_VLINE);
    mvaddch(y + row, x + width - 1, ACS_VLINE);
  }
  mvaddch(y + height - 1, x, ACS_LLCORNER);
  mvhline(y + height - 1, x + 1, ACS_HLINE, width - 2);
  mvaddch(y + height - 1, x + width - 1, ACS_LRCORNER);
  if (!title.empty() && width > 4) {
    attron(COLOR_PAIR(1) | A_BOLD);
    mvaddnstr(y, x + 2, title.data(), width - 4);
    attroff(COLOR_PAIR(1) | A_BOLD);
  }
}

void clear_region_line(int y, int x, int width) {
  if (width <= 0) return;
  mvaddnstr(y, x, std::string(static_cast<std::size_t>(width), ' ').c_str(),
            width);
}

void draw_wrapped_region_block(int y, int x, int width, int height,
                               std::string_view text) {
  if (width <= 0 || height <= 0) return;
  const auto lines = wrap_text_lines(text, width);
  for (int i = 0; i < height; ++i) clear_region_line(y + i, x, width);
  for (int i = 0; i < height && i < static_cast<int>(lines.size()); ++i) {
    mvaddnstr(y + i, x, lines[static_cast<std::size_t>(i)].c_str(), width);
  }
}

void draw_centered_region_line(int y, int x, int width, std::string_view text,
                               int color_pair = 0, bool bold = false) {
  if (width <= 0) return;
  const std::string shown = ellipsize_text(text, std::max(1, width - 2));
  const int line_x = x + std::max(1, (width - static_cast<int>(shown.size())) / 2);
  if (color_pair > 0) attron(COLOR_PAIR(color_pair));
  if (bold) attron(A_BOLD);
  mvaddnstr(y, line_x, shown.c_str(), width - 2);
  if (bold) attroff(A_BOLD);
  if (color_pair > 0) attroff(COLOR_PAIR(color_pair));
}

void draw_wrapped_block(WINDOW* win, int y, int x, int width, int height,
                        std::string_view text) {
  if (!win || width <= 0 || height <= 0) return;
  const auto lines = wrap_text_lines(text, width);
  for (int i = 0; i < height; ++i) {
    mvwaddnstr(win, y + i, x, std::string(width, ' ').c_str(), width);
  }
  for (int i = 0; i < height && i < static_cast<int>(lines.size()); ++i) {
    mvwaddnstr(win, y + i, x, lines[static_cast<std::size_t>(i)].c_str(), width);
  }
}

void draw_centered_line(WINDOW* win, int y, int width, std::string_view text,
                        int color_pair = 0, bool bold = false) {
  if (!win || width <= 0 || y < 0) return;
  const std::string shown = ellipsize_text(text, std::max(1, width - 2));
  const int x = std::max(1, (width - static_cast<int>(shown.size())) / 2);
  if (color_pair > 0) wattron(win, COLOR_PAIR(color_pair));
  if (bold) wattron(win, A_BOLD);
  mvwaddnstr(win, y, x, shown.c_str(), width - 2);
  if (bold) wattroff(win, A_BOLD);
  if (color_pair > 0) wattroff(win, COLOR_PAIR(color_pair));
}

[[nodiscard]] bool prompt_text_dialog(const std::string& title,
                                      const std::string& label,
                                      std::string* out_value, bool secret,
                                      bool allow_empty, bool* cancelled) {
  if (cancelled) *cancelled = false;
  if (!out_value) return false;
  std::string buffer = *out_value;
  const int width = std::max(24, std::min(std::max(24, COLS - 2), 96));
  const int body_width = std::max(8, width - 4);
  const auto label_lines = wrap_text_lines(label, body_width);
  const int label_h =
      std::min<int>(3, std::max<int>(1, static_cast<int>(label_lines.size())));
  const int footer_h = 2;
  const int input_h = std::max(3, std::min(8, LINES - label_h - footer_h - 4));
  const int height = std::max(10, label_h + input_h + footer_h + 3);
  const int starty = std::max(0, (LINES - height) / 2);
  const int startx = std::max(0, (COLS - width) / 2);
  WINDOW* win = newwin(height, width, starty, startx);
  if (!win) return false;
  keypad(win, TRUE);
  curs_set(1);

  for (;;) {
    draw_boxed_window(win, title);
    for (int i = 0; i < label_h; ++i) {
      mvwaddnstr(win, 2 + i, 2, std::string(body_width, ' ').c_str(), body_width);
      if (i < static_cast<int>(label_lines.size())) {
        mvwaddnstr(win, 2 + i, 2, label_lines[static_cast<std::size_t>(i)].c_str(),
                   body_width);
      }
    }
    mvwaddnstr(win, height - 2, 2,
               "Enter=accept  Esc=cancel  Backspace=edit  Text wraps automatically",
               width - 4);

    const int field_width = body_width;
    const int field_y = 2 + label_h + 1;
    const std::string shown = secret ? std::string(buffer.size(), '*') : buffer;
    const auto wrapped_input = wrap_input_lines(shown, std::max(1, field_width - 1));
    const int visible_input_h =
        std::max(1, std::min(input_h, height - field_y - footer_h - 1));
    const int top_line = std::max(
        0, static_cast<int>(wrapped_input.size()) - visible_input_h);
    for (int i = 0; i < visible_input_h; ++i) {
      mvwaddnstr(win, field_y + i, 2, std::string(field_width, ' ').c_str(),
                 field_width);
      const int src = top_line + i;
      if (src >= 0 && src < static_cast<int>(wrapped_input.size())) {
        mvwaddnstr(win, field_y + i, 2,
                   wrapped_input[static_cast<std::size_t>(src)].c_str(),
                   field_width - 1);
      }
    }
    const int cursor_line =
        std::max(0, static_cast<int>(wrapped_input.size()) - 1) - top_line;
    const int cursor_col = wrapped_input.empty()
                               ? 0
                               : std::min<int>(
                                     static_cast<int>(wrapped_input.back().size()),
                                     std::max(0, field_width - 1));
    wmove(win, field_y + std::clamp(cursor_line, 0, visible_input_h - 1),
          2 + cursor_col);
    wrefresh(win);

    const int ch = wgetch(win);
    if (ch == 27) {
      if (cancelled) *cancelled = true;
      delwin(win);
      curs_set(0);
      return true;
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      if (!allow_empty && trim_ascii(buffer).empty()) continue;
      *out_value = trim_ascii(buffer);
      delwin(win);
      curs_set(0);
      return true;
    }
    if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
      if (!buffer.empty()) buffer.pop_back();
      continue;
    }
    if (ch >= 32 && ch <= 126) {
      buffer.push_back(static_cast<char>(ch));
    }
  }
}

[[nodiscard]] bool prompt_choice_dialog(const std::string& title,
                                        const std::string& prompt,
                                        const std::vector<std::string>& options,
                                        std::size_t* out_index,
                                        bool* cancelled) {
  if (cancelled) *cancelled = false;
  if (!out_index || options.empty()) return false;
  std::size_t selected = std::min<std::size_t>(*out_index, options.size() - 1);
  const int height = std::max(8, static_cast<int>(options.size()) + 6);
  const int width = std::max(24, std::min(std::max(24, COLS - 2), 84));
  const int starty = std::max(0, (LINES - height) / 2);
  const int startx = std::max(0, (COLS - width) / 2);
  WINDOW* win = newwin(height, width, starty, startx);
  if (!win) return false;
  keypad(win, TRUE);

  for (;;) {
    draw_boxed_window(win, title);
    draw_wrapped_block(win, 2, 2, width - 4, 2, prompt);
    for (std::size_t i = 0; i < options.size(); ++i) {
      const int row = 4 + static_cast<int>(i);
      if (i == selected) wattron(win, COLOR_PAIR(2) | A_BOLD);
      mvwaddnstr(win, row, 2, std::string(width - 4, ' ').c_str(), width - 4);
      const std::string line = (i == selected ? "> " : "  ") + options[i];
      mvwaddnstr(win, row, 2, line.c_str(), width - 4);
      if (i == selected) wattroff(win, COLOR_PAIR(2) | A_BOLD);
    }
    mvwaddnstr(win, height - 2, 2,
               "Up/Down=move  Enter=accept  Esc=cancel",
               width - 4);
    wrefresh(win);

    const int ch = wgetch(win);
    if (ch == 27) {
      if (cancelled) *cancelled = true;
      delwin(win);
      return true;
    }
    if (ch == KEY_UP || ch == 'k') {
      if (selected > 0) --selected;
      continue;
    }
    if (ch == KEY_DOWN || ch == 'j') {
      if (selected + 1 < options.size()) ++selected;
      continue;
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      *out_index = selected;
      delwin(win);
      return true;
    }
  }
}

[[nodiscard]] bool prompt_yes_no_dialog(const std::string& title,
                                        const std::string& prompt,
                                        bool default_yes, bool* out_value,
                                        bool* cancelled) {
  if (!out_value) return false;
  std::size_t idx = default_yes ? 1u : 0u;
  if (!prompt_choice_dialog(title, prompt, {"No", "Yes"}, &idx, cancelled)) {
    return false;
  }
  if (cancelled && *cancelled) return true;
  *out_value = (idx == 1u);
  return true;
}

[[nodiscard]] bool read_request_text_for_loop(
    const cuwacunu::hero::super::super_loop_record_t& loop, std::string* out,
    std::string* error) {
  if (!out) {
    if (error) *error = "request text output pointer is null";
    return false;
  }
  return cuwacunu::hero::runtime::read_text_file(loop.human_escalation_path, out,
                                                 error);
}

void render_human_requests_screen(
    const std::vector<cuwacunu::hero::super::super_loop_record_t>& pending,
    std::string_view operator_id, std::string_view super_root,
    std::size_t selected, std::string_view request_text,
    std::string_view status_line, bool status_is_error) {
  const int W = COLS;
  const int H = LINES;
  if (W < 56 || H < 10) {
    erase();
    mvaddnstr(0, 0, "Human Hero: terminal too small. Resize or use hero.human.* tools.",
              std::max(0, W - 1));
    refresh();
    return;
  }
  const int header_h = 3;
  const int footer_h = 3;
  const int body_h = std::max(5, H - header_h - footer_h);
  const int left_w = std::clamp(W / 3, 28, std::max(28, W - 24));
  const int right_w = W - left_w;

  if (has_colors()) attrset(COLOR_PAIR(6));
  erase();

  draw_boxed_region(0, 0, header_h, W, " Human Hero ");
  attron(COLOR_PAIR(1) | A_BOLD);
  const std::string header_line =
      "Operator console | pending escalations: " +
      std::to_string(pending.size()) + " | operator: " +
      std::string(operator_id.empty() ? "<unset>" : operator_id);
  mvaddnstr(1, 2, ellipsize_text(header_line, W - 4).c_str(), W - 4);
  attroff(COLOR_PAIR(1) | A_BOLD);

  draw_boxed_region(header_h, 0, body_h, left_w,
                    pending.empty() ? " Inbox " : " Escalations ");
  if (pending.empty()) {
    draw_centered_region_line(header_h + 2, 0, left_w, "No Pending Escalations",
                              5, true);
    draw_centered_region_line(header_h + 4, 0, left_w,
                              "Super Hero has not asked for human input.");
    draw_centered_region_line(header_h + 6, 0, left_w,
                              "Press r to refresh the queue.");
    draw_centered_region_line(header_h + 7, 0, left_w,
                              "Press q to leave the console.");
  } else {
    const int max_rows = body_h - 2;
    std::size_t top = 0;
    if (selected >= static_cast<std::size_t>(max_rows)) {
      top = selected - static_cast<std::size_t>(max_rows) + 1;
    }
    for (int row = 0; row < max_rows; ++row) {
      const std::size_t idx = top + static_cast<std::size_t>(row);
      if (idx >= pending.size()) break;
      const auto& loop = pending[idx];
      const std::string label = loop.loop_id + " | " + loop.objective_name;
      const int row_y = header_h + 1 + row;
      clear_region_line(row_y, 1, left_w - 2);
      if (idx == selected) attron(COLOR_PAIR(2) | A_BOLD);
      mvaddnstr(row_y, 2, ellipsize_text(label, left_w - 4).c_str(),
                left_w - 4);
      if (idx == selected) attroff(COLOR_PAIR(2) | A_BOLD);
    }
  }

  draw_boxed_region(header_h, left_w, body_h, right_w,
                    pending.empty() ? " Overview " : " Request ");
  if (!pending.empty()) {
    const auto& loop = pending[selected];
    std::string meta = "loop=" + loop.loop_id + "  turn=" +
                       std::to_string(loop.turn_count);
    mvaddnstr(header_h + 1, left_w + 2, ellipsize_text(meta, right_w - 4).c_str(),
              right_w - 4);
    mvaddnstr(header_h + 2, left_w + 2,
              ellipsize_text(loop.state_detail.empty() ? loop.objective_name
                                                       : loop.state_detail,
                             right_w - 4)
                  .c_str(),
              right_w - 4);
    draw_wrapped_region_block(header_h + 4, left_w + 2, right_w - 4, body_h - 6,
                              request_text);
  } else {
    std::ostringstream out;
    out << "This console becomes active when a Super Hero loop enters "
           "\"awaiting_human\".\n\n"
        << "Operator: "
        << (operator_id.empty() ? "<unset>" : std::string(operator_id)) << "\n"
        << "Super root: " << super_root << "\n\n"
        << "What to do here:\n"
        << "- wait for a pending loop to appear\n"
        << "- press r to refresh\n"
        << "- press q to exit\n\n"
        << "Non-interactive alternative:\n"
        << "hero.human.list_escalations";
    draw_wrapped_region_block(header_h + 2, left_w + 2, right_w - 4, body_h - 4,
                              out.str());
  }

  draw_boxed_region(header_h + body_h, 0, footer_h, W, " Controls ");
  const std::string controls = pending.empty()
                                   ? "r refresh  q quit"
                                   : "Up/Down select  Enter/g resolve escalation"
                                     "  s stop loop  r refresh  q quit";
  mvaddnstr(header_h + body_h + 1, 2, controls.c_str(), W - 4);
  if (!status_line.empty()) {
    attron(COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);
    mvaddnstr(header_h + body_h + 1, std::max(2, W / 2),
              ellipsize_text(status_line, std::max(8, W / 2 - 3)).c_str(),
              std::max(8, W / 2 - 3));
    attroff(COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);
  }

  refresh();
}

void render_human_reports_screen(
    const std::vector<cuwacunu::hero::super::super_loop_record_t>& pending,
    std::string_view operator_id, std::string_view super_root,
    std::size_t selected, std::string_view report_text,
    std::string_view status_line, bool status_is_error) {
  const int W = COLS;
  const int H = LINES;
  if (W < 56 || H < 10) {
    erase();
    mvaddnstr(0, 0,
              "Human Hero: terminal too small. Resize or use hero.human.* tools.",
              std::max(0, W - 1));
    refresh();
    return;
  }
  const int header_h = 3;
  const int footer_h = 3;
  const int body_h = std::max(5, H - header_h - footer_h);
  const int left_w = std::clamp(W / 3, 28, std::max(28, W - 24));
  const int right_w = W - left_w;

  if (has_colors()) attrset(COLOR_PAIR(6));
  erase();

  draw_boxed_region(0, 0, header_h, W, " Human Hero ");
  attron(COLOR_PAIR(1) | A_BOLD);
  const std::string header_line =
      "Operator console | informational terminal reports: " +
      std::to_string(pending.size()) + " | operator: " +
      std::string(operator_id.empty() ? "<unset>" : operator_id);
  mvaddnstr(1, 2, ellipsize_text(header_line, W - 4).c_str(), W - 4);
  attroff(COLOR_PAIR(1) | A_BOLD);

  draw_boxed_region(header_h, 0, body_h, left_w,
                    pending.empty() ? " Inbox " : " Reports ");
  if (pending.empty()) {
    draw_centered_region_line(header_h + 2, 0, left_w, "No Pending Reports", 5,
                              true);
    draw_centered_region_line(header_h + 4, 0, left_w,
                              "Finished Super Hero loop summaries appear here.");
    draw_centered_region_line(header_h + 6, 0, left_w,
                              "Press r to refresh the queue.");
    draw_centered_region_line(header_h + 7, 0, left_w,
                              "Press q to leave the console.");
  } else {
    const int max_rows = body_h - 2;
    std::size_t top = 0;
    if (selected >= static_cast<std::size_t>(max_rows)) {
      top = selected - static_cast<std::size_t>(max_rows) + 1;
    }
    for (int row = 0; row < max_rows; ++row) {
      const std::size_t idx = top + static_cast<std::size_t>(row);
      if (idx >= pending.size()) break;
      const auto& loop = pending[idx];
      const std::string label = "REPORT | " + loop.loop_id + " | " +
                                loop.objective_name + " | " + loop.state;
      const int row_y = header_h + 1 + row;
      clear_region_line(row_y, 1, left_w - 2);
      if (idx == selected) attron(COLOR_PAIR(2) | A_BOLD);
      mvaddnstr(row_y, 2, ellipsize_text(label, left_w - 4).c_str(),
                left_w - 4);
      if (idx == selected) attroff(COLOR_PAIR(2) | A_BOLD);
    }
  }

  draw_boxed_region(header_h, left_w, body_h, right_w,
                    pending.empty() ? " Overview " : " Report ");
  if (!pending.empty()) {
    const auto& loop = pending[selected];
    std::string meta = "terminal report | loop=" + loop.loop_id +
                       "  state=" + loop.state;
    mvaddnstr(header_h + 1, left_w + 2, ellipsize_text(meta, right_w - 4).c_str(),
              right_w - 4);
    const std::string subtitle = build_report_effort_summary_text(loop);
    mvaddnstr(header_h + 2, left_w + 2,
              ellipsize_text(subtitle, right_w - 4).c_str(), right_w - 4);
    draw_wrapped_region_block(header_h + 4, left_w + 2, right_w - 4, body_h - 6,
                              report_text);
  } else {
    std::ostringstream out;
    out << "This console also tracks finished Super Hero loops.\n\n"
        << "Operator: "
        << (operator_id.empty() ? "<unset>" : std::string(operator_id)) << "\n"
        << "Super root: " << super_root << "\n\n"
        << "What to do here:\n"
        << "- review the finished-loop summary report\n"
        << "- acknowledge it after inspection, or dismiss it to clear the inbox\n"
        << "- stop does not apply here because these loops are already terminal\n"
        << "- press r to refresh\n"
        << "- press q to exit\n\n"
        << "Non-interactive alternative:\n"
        << "hero.human.list_reports";
    draw_wrapped_region_block(header_h + 2, left_w + 2, right_w - 4, body_h - 4,
                              out.str());
  }

  draw_boxed_region(header_h + body_h, 0, footer_h, W, " Controls ");
  const std::string controls =
      pending.empty() ? "r refresh  q quit"
                      : "Up/Down select  Enter inspect  a acknowledge report  d dismiss report  r refresh  q quit";
  mvaddnstr(header_h + body_h + 1, 2, controls.c_str(), W - 4);
  if (!status_line.empty()) {
    attron(COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);
    mvaddnstr(header_h + body_h + 1, std::max(2, W / 2),
              ellipsize_text(status_line, std::max(8, W / 2 - 3)).c_str(),
              std::max(8, W / 2 - 3));
    attroff(COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);
  }

  refresh();
}

void render_human_requests_bootstrap_screen(std::string_view operator_id,
                                            std::string_view super_root,
                                            std::string_view status_line,
                                            bool status_is_error) {
  const int W = COLS;
  const int H = LINES;
  if (W < 56 || H < 10) {
    erase();
    mvaddnstr(0, 0,
              "Human Hero: terminal too small. Resize or use hero.human.* tools.",
              std::max(0, W - 1));
    refresh();
    return;
  }

  const int header_h = 3;
  const int footer_h = 3;
  const int body_h = std::max(5, H - header_h - footer_h);

  if (has_colors()) attrset(COLOR_PAIR(6));
  erase();
  draw_boxed_region(0, 0, header_h, W, " Human Hero ");
  attron(COLOR_PAIR(1) | A_BOLD);
  mvaddnstr(1, 2, "Operator console bootstrap", W - 4);
  attroff(COLOR_PAIR(1) | A_BOLD);

  draw_boxed_region(header_h, 0, body_h, W, " Boot ");
  draw_wrapped_region_block(
      header_h + 2, 2, W - 4, body_h - 4,
      std::string("Initializing operator console...\n\n"
                  "[work] scanning Super Hero escalation queue\n\n") +
          "operator: " +
          std::string(operator_id.empty() ? "<unset>" : operator_id) + "\n" +
          "super_root: " + std::string(super_root) + "\n\n"
          "The escalation inbox will appear when the scan completes.");

  draw_boxed_region(header_h + body_h, 0, footer_h, W, " Status ");
  attron(COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);
  mvaddnstr(header_h + body_h + 1, 2, ellipsize_text(status_line, W - 4).c_str(),
            W - 4);
  attroff(COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);

  refresh();
}

[[nodiscard]] bool collect_interactive_resolution_inputs(
    app_context_t* app,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    bool stop_direct, interactive_resolution_input_t* out, bool* cancelled,
    std::string* error) {
  if (cancelled) *cancelled = false;
  if (error) error->clear();
  if (!app || !out) {
    if (error) *error = "interactive resolution output pointer is null";
    return false;
  }
  *out = interactive_resolution_input_t{};

  if (stop_direct) {
    out->resolution_kind = "stop";
  } else {
    std::size_t resolution_idx = 0;
    if (!prompt_choice_dialog(
            " Resolution ",
            "Choose how to answer this pending escalation.",
            {"Grant requested escalation",
             "Clarify objective or policy without widening authority",
             "Deny requested escalation",
             "Stop the loop"},
            &resolution_idx, cancelled)) {
      return false;
    }
    if (cancelled && *cancelled) return true;
    if (resolution_idx == 0) {
      out->resolution_kind = "grant";
    } else if (resolution_idx == 1) {
      out->resolution_kind = "clarify";
    } else if (resolution_idx == 2) {
      out->resolution_kind = "deny";
    } else {
      out->resolution_kind = "stop";
    }
  }

  const std::string prompt_title =
      (out->resolution_kind == "grant")
          ? " Grant rationale "
          : (out->resolution_kind == "clarify")
                ? " Clarification "
                : (out->resolution_kind == "deny") ? " Denial rationale "
                                                    : " Stop rationale ";
  const std::string prompt_body =
      (out->resolution_kind == "grant")
          ? "Why grant this escalation? This reason is shown to Super Hero."
          : (out->resolution_kind == "clarify")
                ? "What clarification should Super Hero incorporate on the next planning turn?"
                : (out->resolution_kind == "deny")
                      ? "Why deny this escalation? This reason is shown to Super Hero."
                      : "Why stop this loop? This reason is shown to Super Hero.";
  if (!prompt_text_dialog(prompt_title, prompt_body, &out->reason, false, false,
                          cancelled)) {
    return false;
  }
  if (cancelled && *cancelled) return true;

  (void)loop;
  return true;
}

[[nodiscard]] bool run_ncurses_request_responder(app_context_t* app,
                                                 std::string* error) {
  if (error) error->clear();
  if (!app) {
    if (error) *error = "human app pointer is null";
    return false;
  }

  try {
    cuwacunu::iinuji::NcursesAppOpts opts{};
    opts.input_timeout_ms = -1;
    opts.clear_on_start = false;
    cuwacunu::iinuji::NcursesApp curses_app(opts);
    init_human_ncurses_theme();

    std::vector<cuwacunu::hero::super::super_loop_record_t> pending_requests{};
    std::vector<cuwacunu::hero::super::super_loop_record_t> pending_reports{};
    auto sort_pending = [&](std::vector<cuwacunu::hero::super::super_loop_record_t>* rows) {
      if (!rows) return;
      std::sort(rows->begin(), rows->end(), [](const auto& a, const auto& b) {
        if (a.updated_at_ms != b.updated_at_ms) return a.updated_at_ms > b.updated_at_ms;
        return a.loop_id > b.loop_id;
      });
    };

    std::size_t selected = 0;
    std::string detail_text{};
    std::string status = "Scanning pending escalations...";
    bool status_is_error = false;
    bool dirty = true;
    bool queue_refresh_pending = true;
    bool queue_refresh_needs_paint = true;
    bool has_loaded_once = false;

    for (;;) {
      if (queue_refresh_pending && queue_refresh_needs_paint) dirty = true;

      if (dirty) {
        if (!has_loaded_once && queue_refresh_pending) {
          render_human_requests_bootstrap_screen(
              app->defaults.operator_id, app->defaults.super_root.string(),
              status, status_is_error);
        } else {
          const bool showing_requests = !pending_requests.empty();
          auto& visible = showing_requests ? pending_requests : pending_reports;
          if (!visible.empty()) selected = std::min(selected, visible.size() - 1);
          else selected = 0;
          detail_text.clear();
          if (!visible.empty()) {
            std::string detail_error{};
            const bool read_ok =
                showing_requests
                    ? read_request_text_for_loop(visible[selected], &detail_text,
                                                 &detail_error)
                    : read_report_text_for_loop(visible[selected], &detail_text,
                                                &detail_error);
            if (!read_ok) {
              detail_text = std::string("<failed to read ") +
                            (showing_requests ? "escalation" : "report") + ": " +
                            detail_error + ">";
            }
          }
          if (showing_requests) {
            render_human_requests_screen(
                pending_requests, app->defaults.operator_id,
                app->defaults.super_root.string(), selected, detail_text, status,
                status_is_error);
          } else {
            render_human_reports_screen(
                pending_reports, app->defaults.operator_id,
                app->defaults.super_root.string(), selected, detail_text, status,
                status_is_error);
          }
        }
        dirty = false;
      }

      if (queue_refresh_pending) {
        if (queue_refresh_needs_paint) {
          queue_refresh_needs_paint = false;
          continue;
        }

        std::string refresh_error{};
        std::vector<cuwacunu::hero::super::super_loop_record_t> refreshed_requests{};
        std::vector<cuwacunu::hero::super::super_loop_record_t> refreshed_reports{};
        if (!collect_pending_requests(*app, &refreshed_requests, &refresh_error)) {
          status = refresh_error;
          status_is_error = true;
        } else if (!collect_pending_reports(*app, &refreshed_reports,
                                            &refresh_error)) {
          status = refresh_error;
          status_is_error = true;
        } else {
          pending_requests = std::move(refreshed_requests);
          pending_reports = std::move(refreshed_reports);
          sort_pending(&pending_requests);
          sort_pending(&pending_reports);
          const auto& visible =
              !pending_requests.empty() ? pending_requests : pending_reports;
          if (!visible.empty()) selected = std::min(selected, visible.size() - 1);
          else selected = 0;
          const bool has_requests = !pending_requests.empty();
          const bool has_reports = !pending_reports.empty();
          status = has_loaded_once
                       ? (has_requests
                              ? "Refreshed pending escalations."
                              : (has_reports ? "Refreshed pending reports."
                                             : "No pending escalations or reports."))
                       : (has_requests
                              ? "Ready."
                              : (has_reports ? "Ready with finished-loop reports."
                                             : "No pending escalations or reports. Waiting for Super Hero."));
          status_is_error = false;
        }
        has_loaded_once = true;
        queue_refresh_pending = false;
        dirty = true;
        continue;
      }

      const int ch = getch();
      if (ch == 'q' || ch == 'Q' || ch == 27) return true;
      if (ch == KEY_RESIZE) {
        dirty = true;
        continue;
      }
      if (ch == 'r' || ch == 'R') {
        status = "Refreshing escalation queue...";
        status_is_error = false;
        queue_refresh_pending = true;
        queue_refresh_needs_paint = true;
        dirty = true;
        continue;
      }
      const bool showing_requests = !pending_requests.empty();
      auto& visible = showing_requests ? pending_requests : pending_reports;
      if (visible.empty()) continue;
      if (ch == KEY_UP || ch == 'k') {
        if (selected > 0) --selected;
        dirty = true;
        continue;
      }
      if (ch == KEY_DOWN || ch == 'j') {
        if (selected + 1 < visible.size()) ++selected;
        dirty = true;
        continue;
      }
      if (showing_requests &&
          (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == 'c' || ch == 'C' ||
           ch == 's' || ch == 'S')) {
        const bool stop_direct = (ch == 's' || ch == 'S');
        interactive_resolution_input_t input{};
        bool cancelled = false;
        std::string input_error{};
        if (!collect_interactive_resolution_inputs(app, visible[selected],
                                                   stop_direct, &input,
                                                   &cancelled, &input_error)) {
          status = input_error;
          status_is_error = true;
          dirty = true;
          continue;
        }
        if (cancelled) {
          status = "Cancelled.";
          status_is_error = false;
          dirty = true;
          continue;
        }
        std::string structured{};
        std::string response_error{};
        if (!build_resolution_and_apply(app, visible[selected],
                                        input.resolution_kind, input.reason,
                                        &structured, &response_error)) {
          status = response_error;
          status_is_error = true;
          dirty = true;
          continue;
        }
        status = "Applied signed human resolution. Refreshing queue...";
        status_is_error = false;
        queue_refresh_pending = true;
        queue_refresh_needs_paint = true;
        dirty = true;
        continue;
      }
      if (!showing_requests &&
          (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == 'a' ||
           ch == 'A' || ch == 'd' || ch == 'D')) {
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
          status =
              "Informational terminal report. Super is not waiting; use 'a' to acknowledge or 'd' to dismiss.";
          status_is_error = false;
          dirty = true;
          continue;
        }
        const bool dismiss = (ch == 'd' || ch == 'D');
        std::string note{};
        bool cancelled = false;
        if (!prompt_text_dialog(
                dismiss ? " Report dismissal " : " Report acknowledgment ",
                dismiss ? "Optional note saved with the signed report dismissal."
                        : "Optional note saved with the signed report acknowledgment.",
                &note, false, true, &cancelled)) {
          status = dismiss ? "failed to collect report dismissal note"
                           : "failed to collect report acknowledgment note";
          status_is_error = true;
          dirty = true;
          continue;
        }
        if (cancelled) {
          status = "Cancelled.";
          status_is_error = false;
          dirty = true;
          continue;
        }
        std::string structured{};
        std::string ack_error{};
        if (!(dismiss ? dismiss_terminal_report(app, visible[selected], note,
                                                &structured, &ack_error)
                      : acknowledge_terminal_report(app, visible[selected], note,
                                                    &structured, &ack_error))) {
          status = ack_error;
          status_is_error = true;
          dirty = true;
          continue;
        }
        status = dismiss ? "Dismissed summary report. Refreshing queue..."
                         : "Acknowledged summary report. Refreshing queue...";
        status_is_error = false;
        queue_refresh_pending = true;
        queue_refresh_needs_paint = true;
        dirty = true;
        continue;
      }
    }
  } catch (const std::exception& ex) {
    if (error) *error = std::string(kNcursesInitErrorPrefix) + ex.what();
    return false;
  }
}

[[nodiscard]] std::string pending_request_row_to_json(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  std::ostringstream out;
  out << "{"
      << "\"loop_id\":" << json_quote(loop.loop_id) << ","
      << "\"objective_name\":" << json_quote(loop.objective_name) << ","
      << "\"state\":" << json_quote(loop.state) << ","
      << "\"state_detail\":" << json_quote(loop.state_detail) << ","
      << "\"turn_count\":" << loop.turn_count << ","
      << "\"started_at_ms\":" << loop.started_at_ms << ","
      << "\"updated_at_ms\":" << loop.updated_at_ms << ","
      << "\"human_escalation_path\":"
      << json_quote(loop.human_escalation_path) << ","
      << "\"human_resolution_path\":"
      << json_quote(
             cuwacunu::hero::super::super_loop_human_resolution_latest_path(
                 std::filesystem::path(loop.loop_root).parent_path(),
                 loop.loop_id)
                 .string())
      << ",\"human_resolution_sig_path\":"
      << json_quote(
             cuwacunu::hero::super::super_loop_human_resolution_latest_sig_path(
                 std::filesystem::path(loop.loop_root).parent_path(),
                 loop.loop_id)
                 .string())
      << ",\"escalation_excerpt\":" << json_quote(request_excerpt(loop))
      << "}";
  return out.str();
}

[[nodiscard]] std::string pending_report_row_to_json(
    const cuwacunu::hero::super::super_loop_record_t& loop) {
  const std::optional<std::uint64_t> elapsed_ms = super_loop_elapsed_ms(loop);
  std::ostringstream out;
  out << "{"
      << "\"loop_id\":" << json_quote(loop.loop_id) << ","
      << "\"objective_name\":" << json_quote(loop.objective_name) << ","
      << "\"state\":" << json_quote(loop.state) << ","
      << "\"state_detail\":" << json_quote(loop.state_detail) << ","
      << "\"turn_count\":" << loop.turn_count << ","
      << "\"launch_count\":" << loop.launch_count << ","
      << "\"started_at_ms\":" << loop.started_at_ms << ","
      << "\"updated_at_ms\":" << loop.updated_at_ms << ","
      << "\"finished_at_ms\":"
      << (loop.finished_at_ms.has_value() ? std::to_string(*loop.finished_at_ms)
                                          : "null")
      << ","
      << "\"duration_ms\":"
      << (elapsed_ms.has_value() ? std::to_string(*elapsed_ms) : "null") << ","
      << "\"max_review_turns\":" << loop.max_review_turns << ","
      << "\"max_campaign_launches\":" << loop.max_campaign_launches << ","
      << "\"remaining_review_turns\":" << loop.remaining_review_turns << ","
      << "\"remaining_campaign_launches\":"
      << loop.remaining_campaign_launches << ","
      << "\"effort_summary_text\":"
      << json_quote(build_report_effort_summary_text(loop)) << ","
      << "\"report_path\":" << json_quote(report_path_for_loop(loop).string()) << ","
      << "\"report_ack_path\":"
      << json_quote(report_ack_path_for_loop(loop).string()) << ","
      << "\"report_ack_sig_path\":"
      << json_quote(report_ack_sig_path_for_loop(loop).string()) << ","
      << "\"report_excerpt\":" << json_quote(report_excerpt(loop)) << "}";
  return out.str();
}

[[nodiscard]] bool collect_pending_requests(
    const app_context_t& app,
    std::vector<cuwacunu::hero::super::super_loop_record_t>* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "pending requests output pointer is null";
    return false;
  }
  out->clear();
  sync_human_pending_markers_best_effort(app);
  std::vector<cuwacunu::hero::super::super_loop_record_t> loops{};
  if (!cuwacunu::hero::super::scan_super_loop_records(app.defaults.super_root,
                                                      &loops, error)) {
    return false;
  }
  for (const auto& loop : loops) {
    if (loop.state == "awaiting_human") out->push_back(loop);
  }
  return true;
}

[[nodiscard]] bool collect_pending_reports(
    const app_context_t& app,
    std::vector<cuwacunu::hero::super::super_loop_record_t>* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "pending reports output pointer is null";
    return false;
  }
  out->clear();
  sync_human_pending_markers_best_effort(app);
  std::vector<cuwacunu::hero::super::super_loop_record_t> loops{};
  if (!cuwacunu::hero::super::scan_super_loop_records(app.defaults.super_root,
                                                      &loops, error)) {
    return false;
  }
  for (const auto& loop : loops) {
    if (!cuwacunu::hero::super::is_super_loop_finished_report_state(loop.state)) {
      continue;
    }
    const std::filesystem::path report_path = report_path_for_loop(loop);
    if (!std::filesystem::exists(report_path) ||
        !std::filesystem::is_regular_file(report_path)) {
      continue;
    }
    std::string ack_error{};
    if (!report_ack_matches_current_report(loop, &ack_error)) out->push_back(loop);
  }
  return true;
}

[[nodiscard]] bool build_resolution_and_apply(
    app_context_t* app,
    const cuwacunu::hero::super::super_loop_record_t& loop,
    std::string resolution_kind, std::string reason,
    std::string* out_structured, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !out_structured || !out_error) return false;

  resolution_kind = trim_ascii(resolution_kind);
  reason = trim_ascii(reason);
  if (resolution_kind != "grant" && resolution_kind != "deny" &&
      resolution_kind != "clarify" && resolution_kind != "stop") {
    *out_error = "human resolution_kind must be grant, deny, clarify, or stop";
    return false;
  }
  if (reason.empty()) {
    *out_error = "human resolution requires a non-empty reason";
    return false;
  }
  if (app->defaults.operator_id.empty() ||
      app->defaults.operator_id == "CHANGE_ME_OPERATOR") {
    *out_error =
        "Human Hero operator_id is not configured; update default.hero.human.dsl";
    return false;
  }
  if (app->defaults.operator_signing_ssh_identity.empty()) {
    *out_error =
        "Human Hero operator_signing_ssh_identity is not configured";
    return false;
  }

  std::string escalation_sha256_hex{};
  if (!cuwacunu::hero::human::sha256_hex_file(loop.human_escalation_path,
                                              &escalation_sha256_hex,
                                              out_error)) {
    return false;
  }

  const std::filesystem::path latest_turn_outcome_path =
      cuwacunu::hero::super::super_loop_latest_turn_outcome_path(
          app->defaults.super_root, loop.loop_id);
  std::string latest_turn_outcome_json{};
  if (!cuwacunu::hero::runtime::read_text_file(latest_turn_outcome_path,
                                               &latest_turn_outcome_json,
                                               out_error)) {
    return false;
  }
  std::string outcome{};
  (void)extract_json_string_field(latest_turn_outcome_json, "outcome", &outcome);
  outcome = trim_ascii(outcome);
  if (outcome != "escalate") {
    *out_error = "latest pending turn outcome is not an escalation";
    return false;
  }
  std::string escalation_json{};
  if (!extract_json_object_field(latest_turn_outcome_json, "escalation",
                                 &escalation_json)) {
    *out_error = "latest turn outcome is missing escalation object";
    return false;
  }
  std::string escalation_kind{};
  (void)extract_json_string_field(escalation_json, "kind", &escalation_kind);
  escalation_kind = trim_ascii(escalation_kind);
  if (escalation_kind.empty()) {
    *out_error = "latest turn outcome escalation is missing kind";
    return false;
  }

  std::string delta_json{};
  (void)extract_json_object_field(escalation_json, "delta", &delta_json);
  human_resolution_record_t resolution{};
  resolution.loop_id = loop.loop_id;
  resolution.turn_index = loop.turn_count;
  resolution.escalation_sha256_hex = escalation_sha256_hex;
  resolution.operator_id = app->defaults.operator_id;
  resolution.resolved_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  resolution.resolution_kind = resolution_kind;
  resolution.escalation_kind = escalation_kind;
  resolution.reason = reason;
  resolution.signer_public_key_fingerprint_sha256_hex =
      std::string(64, '0');
  if (resolution_kind == "grant") {
    (void)extract_json_bool_field(delta_json, "allow_default_write",
                                  &resolution.grant_allow_default_write);
    (void)extract_json_u64_field(delta_json, "additional_review_turns",
                                 &resolution.grant_additional_review_turns);
    (void)extract_json_u64_field(delta_json, "additional_campaign_launches",
                                 &resolution.grant_additional_campaign_launches);
  }

  std::string signature_hex{};
  std::string fingerprint_hex{};
  std::string provisional_json =
      cuwacunu::hero::human::human_resolution_to_json(resolution);
  if (!cuwacunu::hero::human::sign_human_attested_json(
          app->defaults.operator_signing_ssh_identity, provisional_json,
          &signature_hex, &fingerprint_hex, out_error)) {
    return false;
  }
  resolution.signer_public_key_fingerprint_sha256_hex = fingerprint_hex;
  const std::string resolution_json =
      cuwacunu::hero::human::human_resolution_to_json(resolution);
  if (!cuwacunu::hero::human::sign_human_attested_json(
          app->defaults.operator_signing_ssh_identity, resolution_json,
          &signature_hex, &fingerprint_hex, out_error)) {
    return false;
  }
  if (fingerprint_hex !=
      resolution.signer_public_key_fingerprint_sha256_hex) {
    *out_error = "human resolution fingerprint changed during signing";
    return false;
  }

  std::error_code ec{};
  const auto response_dir =
      cuwacunu::hero::super::super_loop_human_resolutions_dir(
      app->defaults.super_root, loop.loop_id);
  std::filesystem::create_directories(response_dir, ec);
  if (ec) {
    *out_error =
        "cannot create human resolution directory: " + response_dir.string();
    return false;
  }
  const auto response_path =
      cuwacunu::hero::super::super_loop_human_resolution_path(
          app->defaults.super_root, loop.loop_id, loop.turn_count);
  const auto response_sig_path =
      cuwacunu::hero::super::super_loop_human_resolution_sig_path(
          app->defaults.super_root, loop.loop_id, loop.turn_count);
  const auto latest_path =
      cuwacunu::hero::super::super_loop_human_resolution_latest_path(
          app->defaults.super_root, loop.loop_id);
  const auto latest_sig_path =
      cuwacunu::hero::super::super_loop_human_resolution_latest_sig_path(
          app->defaults.super_root, loop.loop_id);

  if (!cuwacunu::hero::runtime::write_text_file_atomic(response_path,
                                                       resolution_json,
                                                       out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(response_sig_path,
                                                       signature_hex + "\n",
                                                       out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(latest_path,
                                                       resolution_json,
                                                       out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(latest_sig_path,
                                                       signature_hex + "\n",
                                                       out_error)) {
    return false;
  }

  std::string super_structured{};
  std::string resume_args =
      "{\"loop_id\":" + json_quote(loop.loop_id) +
      ",\"human_resolution_path\":" + json_quote(response_path.string()) +
      ",\"human_resolution_sig_path\":" +
      json_quote(response_sig_path.string()) + "}";
  if (!call_super_tool(*app, "hero.super.apply_human_resolution", resume_args,
                       &super_structured, out_error)) {
    return false;
  }

  *out_structured = "{\"loop_id\":" + json_quote(loop.loop_id) +
                    ",\"resolution_path\":" +
                    json_quote(response_path.string()) +
                    ",\"resolution_sig_path\":" +
                    json_quote(response_sig_path.string()) +
                    ",\"fingerprint_sha256_hex\":" + json_quote(fingerprint_hex) +
                    ",\"super\":" + super_structured + "}";
  return true;
}

[[nodiscard]] bool finalize_terminal_report(
    app_context_t* app,
    const cuwacunu::hero::super::super_loop_record_t& loop, std::string note,
    std::string disposition, std::string* out_structured,
    std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !out_structured || !out_error) return false;
  note = trim_ascii(note);
  disposition = lowercase_copy(trim_ascii(disposition));
  if (disposition != "acknowledged" && disposition != "dismissed") {
    *out_error = "invalid report disposition";
    return false;
  }
  if (disposition == "dismissed" && note.empty()) {
    note = "dismissed by operator";
  }

  if (!cuwacunu::hero::super::is_super_loop_finished_report_state(loop.state)) {
    *out_error = "loop does not currently expose a finished summary report";
    return false;
  }
  if (app->defaults.operator_id.empty() ||
      app->defaults.operator_id == "CHANGE_ME_OPERATOR") {
    *out_error =
        "Human Hero operator_id is not configured; update default.hero.human.dsl";
    return false;
  }
  if (app->defaults.operator_signing_ssh_identity.empty()) {
    *out_error =
        "Human Hero operator_signing_ssh_identity is not configured";
    return false;
  }

  const std::filesystem::path report_path = report_path_for_loop(loop);
  std::string report_sha256_hex{};
  if (!cuwacunu::hero::human::sha256_hex_file(report_path, &report_sha256_hex,
                                              out_error)) {
    return false;
  }

  const std::uint64_t acknowledged_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  std::string ack_json =
      std::string("{\"schema\":") + json_quote(kHumanReportAckSchemaV2) +
      ",\"loop_id\":" + json_quote(loop.loop_id) + ",\"state\":" +
      json_quote(loop.state) + ",\"report_sha256_hex\":" +
      json_quote(report_sha256_hex) + ",\"operator_id\":" +
      json_quote(app->defaults.operator_id) + ",\"acknowledged_at_ms\":" +
      std::to_string(acknowledged_at_ms) + ",\"disposition\":" +
      json_quote(disposition) + ",\"note\":" + json_quote(note) +
      ",\"signer_public_key_fingerprint_sha256_hex\":" +
      json_quote(std::string(64, '0')) + "}";

  std::string signature_hex{};
  std::string fingerprint_hex{};
  if (!cuwacunu::hero::human::sign_human_attested_json(
          app->defaults.operator_signing_ssh_identity, ack_json, &signature_hex,
          &fingerprint_hex, out_error)) {
    return false;
  }
  if (fingerprint_hex.size() != 64) {
    *out_error = "report ack signing returned invalid fingerprint length";
    return false;
  }
  ack_json =
      std::string("{\"schema\":") + json_quote(kHumanReportAckSchemaV2) +
      ",\"loop_id\":" + json_quote(loop.loop_id) + ",\"state\":" +
      json_quote(loop.state) + ",\"report_sha256_hex\":" +
      json_quote(report_sha256_hex) + ",\"operator_id\":" +
      json_quote(app->defaults.operator_id) + ",\"acknowledged_at_ms\":" +
      std::to_string(acknowledged_at_ms) + ",\"disposition\":" +
      json_quote(disposition) + ",\"note\":" + json_quote(note) +
      ",\"signer_public_key_fingerprint_sha256_hex\":" +
      json_quote(fingerprint_hex) + "}";

  const std::filesystem::path ack_path = report_ack_path_for_loop(loop);
  const std::filesystem::path ack_sig_path = report_ack_sig_path_for_loop(loop);
  if (!cuwacunu::hero::runtime::write_text_file_atomic(ack_path, ack_json + "\n",
                                                       out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(ack_sig_path,
                                                       signature_hex + "\n",
                                                       out_error)) {
    return false;
  }
  sync_human_pending_markers_best_effort(*app);

  *out_structured =
      "{\"loop_id\":" + json_quote(loop.loop_id) + ",\"report_path\":" +
      json_quote(report_path.string()) + ",\"report_ack_path\":" +
      json_quote(ack_path.string()) + ",\"report_ack_sig_path\":" +
      json_quote(ack_sig_path.string()) + ",\"disposition\":" +
      json_quote(disposition) + ",\"note\":" + json_quote(note) +
      ",\"fingerprint_sha256_hex\":" + json_quote(fingerprint_hex) + "}";
  return true;
}

[[nodiscard]] bool acknowledge_terminal_report(
    app_context_t* app,
    const cuwacunu::hero::super::super_loop_record_t& loop, std::string note,
    std::string* out_structured, std::string* out_error) {
  return finalize_terminal_report(app, loop, std::move(note), "acknowledged",
                                  out_structured, out_error);
}

[[nodiscard]] bool dismiss_terminal_report(
    app_context_t* app,
    const cuwacunu::hero::super::super_loop_record_t& loop, std::string note,
    std::string* out_structured, std::string* out_error) {
  return finalize_terminal_report(app, loop, std::move(note), "dismissed",
                                  out_structured, out_error);
}

[[nodiscard]] bool handle_tool_list_escalations(app_context_t* app,
                                                const std::string& arguments_json,
                                                std::string* out_structured,
                                                std::string* out_error);
[[nodiscard]] bool handle_tool_list_reports(app_context_t* app,
                                            const std::string& arguments_json,
                                            std::string* out_structured,
                                            std::string* out_error);
[[nodiscard]] bool handle_tool_get_escalation(app_context_t* app,
                                              const std::string& arguments_json,
                                              std::string* out_structured,
                                              std::string* out_error);
[[nodiscard]] bool handle_tool_get_report(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error);
[[nodiscard]] bool handle_tool_resolve_escalation(app_context_t* app,
                                                  const std::string& arguments_json,
                                                  std::string* out_structured,
                                                  std::string* out_error);
[[nodiscard]] bool handle_tool_ack_report(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error);
[[nodiscard]] bool handle_tool_dismiss_report(app_context_t* app,
                                              const std::string& arguments_json,
                                              std::string* out_structured,
                                              std::string* out_error);

#define HERO_HUMAN_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER) \
  {NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
constexpr human_tool_descriptor_t kHumanTools[] = {
#include "hero/human_hero/hero_human_tools.def"
};
#undef HERO_HUMAN_TOOL

[[nodiscard]] const human_tool_descriptor_t* find_human_tool_descriptor(
    std::string_view tool_name) {
  for (const auto& tool : kHumanTools) {
    if (tool.name == tool_name) return &tool;
  }
  return nullptr;
}

[[nodiscard]] bool handle_tool_list_escalations(app_context_t* app,
                                                const std::string& arguments_json,
                                                std::string* out_structured,
                                                std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::size_t limit = 0;
  std::size_t offset = 0;
  bool newest_first = true;
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);
  (void)extract_json_bool_field(arguments_json, "newest_first", &newest_first);

  std::vector<cuwacunu::hero::super::super_loop_record_t> loops{};
  if (!collect_pending_requests(*app, &loops, out_error)) return false;
  std::sort(loops.begin(), loops.end(), [newest_first](const auto& a,
                                                       const auto& b) {
    if (a.updated_at_ms != b.updated_at_ms) {
      return newest_first ? (a.updated_at_ms > b.updated_at_ms)
                          : (a.updated_at_ms < b.updated_at_ms);
    }
    return newest_first ? (a.loop_id > b.loop_id) : (a.loop_id < b.loop_id);
  });
  const std::size_t total = loops.size();
  const std::size_t off = std::min(offset, loops.size());
  std::size_t count = limit;
  if (count == 0) count = loops.size() - off;
  count = std::min(count, loops.size() - off);

  std::ostringstream rows;
  rows << "[";
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0) rows << ",";
    rows << pending_request_row_to_json(loops[off + i]);
  }
  rows << "]";

  *out_structured =
      "{\"loop_id\":\"\",\"count\":" + std::to_string(count) +
      ",\"total\":" + std::to_string(total) + ",\"escalations\":" +
      rows.str() +
      "}";
  return true;
}

[[nodiscard]] bool handle_tool_get_escalation(app_context_t* app,
                                              const std::string& arguments_json,
                                              std::string* out_structured,
                                              std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string loop_id{};
  (void)extract_json_string_field(arguments_json, "loop_id", &loop_id);
  loop_id = trim_ascii(loop_id);
  if (loop_id.empty()) {
    *out_error = "get_escalation requires arguments.loop_id";
    return false;
  }

  cuwacunu::hero::super::super_loop_record_t loop{};
  if (!cuwacunu::hero::super::read_super_loop_record(app->defaults.super_root, loop_id,
                                                     &loop, out_error)) {
    return false;
  }
  if (loop.state != "awaiting_human") {
    *out_error = "loop is not currently waiting for a human escalation resolution";
    return false;
  }

  std::string request_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.human_escalation_path,
                                               &request_text, out_error)) {
    return false;
  }
  *out_structured = "{\"loop_id\":" + json_quote(loop_id) + ",\"loop\":" +
                    pending_request_row_to_json(loop) + ",\"escalation_text\":" +
                    json_quote(request_text) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_list_reports(app_context_t* app,
                                            const std::string& arguments_json,
                                            std::string* out_structured,
                                            std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::size_t limit = 0;
  std::size_t offset = 0;
  bool newest_first = true;
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);
  (void)extract_json_bool_field(arguments_json, "newest_first", &newest_first);

  std::vector<cuwacunu::hero::super::super_loop_record_t> loops{};
  if (!collect_pending_reports(*app, &loops, out_error)) return false;
  std::sort(loops.begin(), loops.end(), [newest_first](const auto& a,
                                                       const auto& b) {
    if (a.updated_at_ms != b.updated_at_ms) {
      return newest_first ? (a.updated_at_ms > b.updated_at_ms)
                          : (a.updated_at_ms < b.updated_at_ms);
    }
    return newest_first ? (a.loop_id > b.loop_id) : (a.loop_id < b.loop_id);
  });
  const std::size_t total = loops.size();
  const std::size_t off = std::min(offset, loops.size());
  std::size_t count = limit;
  if (count == 0) count = loops.size() - off;
  count = std::min(count, loops.size() - off);

  std::ostringstream rows;
  rows << "[";
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0) rows << ",";
    rows << pending_report_row_to_json(loops[off + i]);
  }
  rows << "]";

  *out_structured =
      "{\"loop_id\":\"\",\"count\":" + std::to_string(count) +
      ",\"total\":" + std::to_string(total) + ",\"reports\":" + rows.str() +
      "}";
  return true;
}

[[nodiscard]] bool handle_tool_get_report(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string loop_id{};
  (void)extract_json_string_field(arguments_json, "loop_id", &loop_id);
  loop_id = trim_ascii(loop_id);
  if (loop_id.empty()) {
    *out_error = "get_report requires arguments.loop_id";
    return false;
  }

  cuwacunu::hero::super::super_loop_record_t loop{};
  if (!cuwacunu::hero::super::read_super_loop_record(app->defaults.super_root,
                                                     loop_id, &loop,
                                                     out_error)) {
    return false;
  }
  if (!cuwacunu::hero::super::is_super_loop_finished_report_state(loop.state)) {
    *out_error = "loop does not currently expose a finished summary report";
    return false;
  }

  std::string report_text{};
  if (!read_report_text_for_loop(loop, &report_text, out_error)) return false;
  *out_structured = "{\"loop_id\":" + json_quote(loop_id) + ",\"loop\":" +
                    pending_report_row_to_json(loop) + ",\"report_text\":" +
                    json_quote(report_text) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_resolve_escalation(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string loop_id{};
  std::string resolution_kind{};
  std::string reason{};
  (void)extract_json_string_field(arguments_json, "loop_id", &loop_id);
  (void)extract_json_string_field(arguments_json, "resolution_kind",
                                  &resolution_kind);
  (void)extract_json_string_field(arguments_json, "reason", &reason);
  loop_id = trim_ascii(loop_id);
  if (loop_id.empty()) {
    *out_error = "resolve_escalation requires arguments.loop_id";
    return false;
  }
  if (trim_ascii(resolution_kind).empty()) {
    *out_error = "resolve_escalation requires arguments.resolution_kind";
    return false;
  }

  cuwacunu::hero::super::super_loop_record_t loop{};
  if (!cuwacunu::hero::super::read_super_loop_record(app->defaults.super_root, loop_id,
                                                     &loop, out_error)) {
    return false;
  }
  if (loop.state != "awaiting_human") {
    *out_error = "loop is not currently waiting for a human escalation resolution";
    return false;
  }
  return build_resolution_and_apply(app, loop, resolution_kind, reason,
                                    out_structured, out_error);
}

[[nodiscard]] bool handle_tool_ack_report(app_context_t* app,
                                          const std::string& arguments_json,
                                          std::string* out_structured,
                                          std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string loop_id{};
  std::string note{};
  (void)extract_json_string_field(arguments_json, "loop_id", &loop_id);
  (void)extract_json_string_field(arguments_json, "note", &note);
  loop_id = trim_ascii(loop_id);
  if (loop_id.empty()) {
    *out_error = "ack_report requires arguments.loop_id";
    return false;
  }

  cuwacunu::hero::super::super_loop_record_t loop{};
  if (!cuwacunu::hero::super::read_super_loop_record(app->defaults.super_root,
                                                     loop_id, &loop,
                                                     out_error)) {
    return false;
  }
  return acknowledge_terminal_report(app, loop, note, out_structured, out_error);
}

[[nodiscard]] bool handle_tool_dismiss_report(app_context_t* app,
                                              const std::string& arguments_json,
                                              std::string* out_structured,
                                              std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string loop_id{};
  std::string note{};
  (void)extract_json_string_field(arguments_json, "loop_id", &loop_id);
  (void)extract_json_string_field(arguments_json, "note", &note);
  loop_id = trim_ascii(loop_id);
  if (loop_id.empty()) {
    *out_error = "dismiss_report requires arguments.loop_id";
    return false;
  }

  cuwacunu::hero::super::super_loop_record_t loop{};
  if (!cuwacunu::hero::super::read_super_loop_record(app->defaults.super_root,
                                                     loop_id, &loop,
                                                     out_error)) {
    return false;
  }
  return dismiss_terminal_report(app, loop, note, out_structured, out_error);
}

}  // namespace

std::filesystem::path resolve_human_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "REAL_HERO", "human_hero_dsl_filename");
  if (!configured.has_value()) return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) return {};
  return std::filesystem::path(resolved);
}

bool load_human_defaults(const std::filesystem::path& hero_dsl_path,
                         const std::filesystem::path& global_config_path,
                         human_defaults_t* out, std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "human defaults output pointer is null";
    return false;
  }
  *out = human_defaults_t{};

  std::ifstream in(hero_dsl_path);
  if (!in) {
    if (error) {
      *error = "cannot open Human HERO defaults DSL: " + hero_dsl_path.string();
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
  const auto resolve_exec = [&](const char* key, std::filesystem::path* dst) {
    const auto it = values.find(key);
    if (it == values.end()) return false;
    *dst = resolve_command_path(hero_dsl_path.parent_path(), it->second);
    return !dst->empty();
  };
  if (!resolve_exec("super_hero_binary", &out->super_hero_binary)) {
    if (error) *error = "missing/invalid super_hero_binary in " + hero_dsl_path.string();
    return false;
  }
  out->operator_id = trim_ascii(values["operator_id"]);
  if (operator_id_needs_bootstrap(out->operator_id)) {
    const std::string bootstrapped_operator_id = derive_bootstrap_operator_id();
    std::string bootstrap_error{};
    if (!persist_bootstrap_operator_id(hero_dsl_path, bootstrapped_operator_id,
                                       &bootstrap_error)) {
      std::cerr << "[hero_human_mcp][warning] failed to persist bootstrapped "
                   "operator_id to "
                << hero_dsl_path.string() << ": " << bootstrap_error
                << std::endl;
    } else {
      std::cerr << "[hero_human_mcp] auto-initialized operator_id to "
                << bootstrapped_operator_id << " in " << hero_dsl_path.string()
                << std::endl;
    }
    out->operator_id = bootstrapped_operator_id;
  }
  const auto ssh_identity_it = values.find("operator_signing_ssh_identity");
  if (ssh_identity_it != values.end() &&
      !trim_ascii(ssh_identity_it->second).empty()) {
    out->operator_signing_ssh_identity = std::filesystem::path(
        resolve_path_from_base_folder(hero_dsl_path.parent_path().string(),
                                      ssh_identity_it->second));
  }
  if (out->super_root.empty()) {
    if (error) {
      *error = "cannot derive .super_hero root from GENERAL.runtime_root in " +
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

[[nodiscard]] bool human_tool_is_read_only(std::string_view name) {
  return name == "hero.human.list_escalations" ||
         name == "hero.human.list_reports" ||
         name == "hero.human.get_escalation" ||
         name == "hero.human.get_report";
}

[[nodiscard]] bool human_tool_is_destructive(std::string_view name) {
  return name == "hero.human.dismiss_report";
}

std::string build_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < std::size(kHumanTools); ++i) {
    const auto& tool = kHumanTools[i];
    if (i != 0) out << ",";
    out << "{\"name\":" << json_quote(tool.name)
        << ",\"description\":" << json_quote(tool.description)
        << ",\"inputSchema\":" << tool.input_schema_json
        << ",\"annotations\":{\"readOnlyHint\":"
        << (human_tool_is_read_only(tool.name) ? "true" : "false")
        << ",\"destructiveHint\":"
        << (human_tool_is_destructive(tool.name) ? "true" : "false")
        << ",\"openWorldHint\":false}}";
  }
  out << "]}";
  return out.str();
}

std::string build_tools_list_human_text() {
  std::ostringstream out;
  for (const auto& tool : kHumanTools) {
    out << tool.name << "\t" << tool.description << "\n";
  }
  return out.str();
}

bool tool_result_is_error(std::string_view tool_result_json) {
  return tool_result_json.find("\"isError\":true") != std::string_view::npos ||
         tool_result_json.find("\"isError\": true") != std::string_view::npos;
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

  const auto* descriptor = find_human_tool_descriptor(tool_name);
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

[[nodiscard]] bool run_line_prompt_request_responder(app_context_t* app,
                                                     std::string* error) {
  if (error) error->clear();
  if (!app) {
    if (error) *error = "human app pointer is null";
    return false;
  }

  std::vector<cuwacunu::hero::super::super_loop_record_t> pending{};
  std::vector<cuwacunu::hero::super::super_loop_record_t> reports{};
  if (!collect_pending_requests(*app, &pending, error)) return false;
  if (!collect_pending_reports(*app, &reports, error)) return false;
  std::cout << "== Human Hero ==\n"
            << "operator: "
            << (app->defaults.operator_id.empty() ? "<unset>"
                                                  : app->defaults.operator_id)
            << "\n"
            << "super_root: " << app->defaults.super_root.string() << "\n"
            << "pending_escalations: " << pending.size() << "\n"
            << "pending_terminal_reports: " << reports.size() << "\n";
  if (pending.empty() && reports.empty()) {
    std::cout << "status: no pending human escalations or finished reports\n"
              << "hint: rerun this command after a Super Hero loop enters "
                 "\"awaiting_human\", finishes, or use "
                 "hero.human.list_escalations / hero.human.list_reports.\n";
    return true;
  }
  if (!pending.empty()) {
    std::cout << "status: pending human escalation resolution required\n";

    std::sort(pending.begin(), pending.end(), [](const auto& a, const auto& b) {
      if (a.updated_at_ms != b.updated_at_ms) return a.updated_at_ms > b.updated_at_ms;
      return a.loop_id > b.loop_id;
    });

    std::size_t selected = 0;
    if (pending.size() > 1) {
      std::cout << "Pending escalations:\n";
      for (std::size_t i = 0; i < pending.size(); ++i) {
        std::cout << "  [" << i + 1 << "] " << pending[i].loop_id << "  "
                  << pending[i].objective_name << "  "
                  << pending[i].state_detail << "\n";
      }
      std::cout << "Select escalation number (blank to cancel): " << std::flush;
      std::string line{};
      if (!std::getline(std::cin, line)) return false;
      line = trim_ascii(line);
      if (line.empty()) return true;
      std::size_t choice = 0;
      if (!parse_size_token(line, &choice) || choice == 0 || choice > pending.size()) {
        if (error) *error = "invalid escalation selection";
        return false;
      }
      selected = choice - 1;
    }
    const auto& loop = pending[selected];
    std::string escalation_text{};
    if (!cuwacunu::hero::runtime::read_text_file(loop.human_escalation_path,
                                                 &escalation_text, error)) {
      return false;
    }
    std::cout << "\n=== Human Escalation ===\n" << escalation_text << "\n";

    std::cout << "Resolve with [g]rant, [c]larify, [d]eny, [s]top loop, or blank to cancel: "
              << std::flush;
    std::string resolution{};
    if (!std::getline(std::cin, resolution)) return false;
    resolution = lowercase_copy(trim_ascii(resolution));
    if (resolution.empty()) return true;

    std::string resolution_kind{};
    if (resolution == "g" || resolution == "grant") {
      resolution_kind = "grant";
    } else if (resolution == "c" || resolution == "clarify" ||
               resolution == "clarification") {
      resolution_kind = "clarify";
    } else if (resolution == "d" || resolution == "deny") {
      resolution_kind = "deny";
    } else if (resolution == "s" || resolution == "stop") {
      resolution_kind = "stop";
    } else {
      if (error) *error = "invalid resolution selection";
      return false;
    }

    std::cout << "Reason shown to Super Hero: " << std::flush;
    std::string reason{};
    if (!std::getline(std::cin, reason)) return false;
    reason = trim_ascii(reason);
    if (reason.empty()) {
      if (error) *error = "reason is required";
      return false;
    }

    std::string structured{};
    if (!build_resolution_and_apply(app, loop, resolution_kind, reason,
                                    &structured, error)) {
      return false;
    }
    std::cout << "\nHuman escalation resolution applied.\n"
              << structured << "\n";
    return true;
  }

  std::cout << "status: informational terminal report only; Super is not waiting on you\n";
  std::sort(reports.begin(), reports.end(), [](const auto& a, const auto& b) {
    if (a.updated_at_ms != b.updated_at_ms) return a.updated_at_ms > b.updated_at_ms;
    return a.loop_id > b.loop_id;
  });

  std::size_t selected = 0;
  if (reports.size() > 1) {
    std::cout << "Pending finished-loop reports:\n";
    for (std::size_t i = 0; i < reports.size(); ++i) {
      std::cout << "  [" << i + 1 << "] " << reports[i].loop_id << "  "
                << reports[i].objective_name << "  " << reports[i].state
                << "  " << build_report_effort_summary_text(reports[i]) << "\n";
    }
    std::cout << "Select report number (blank to cancel): " << std::flush;
    std::string line{};
    if (!std::getline(std::cin, line)) return false;
    line = trim_ascii(line);
    if (line.empty()) return true;
    std::size_t choice = 0;
    if (!parse_size_token(line, &choice) || choice == 0 || choice > reports.size()) {
      if (error) *error = "invalid report selection";
      return false;
    }
    selected = choice - 1;
  }
  const auto& loop = reports[selected];
  std::string report_text{};
  if (!read_report_text_for_loop(loop, &report_text, error)) return false;
  std::cout << "\n=== Human Summary Report ===\n"
            << "[informational terminal report; not an awaiting_human case]\n\n"
            << report_text << "\n";

  std::cout << "Resolve this report? [a]cknowledge, [d]ismiss, or blank to cancel: "
            << std::flush;
  std::string control{};
  if (!std::getline(std::cin, control)) return false;
  control = lowercase_copy(trim_ascii(control));
  if (control.empty()) return true;
  if (control != "a" && control != "ack" && control != "acknowledge" &&
      control != "d" && control != "dismiss") {
    if (error) *error = "invalid report action selection";
    return false;
  }

  const bool dismiss = (control == "d" || control == "dismiss");
  std::cout << (dismiss ? "Dismissal note (optional): "
                        : "Validation note (optional): ")
            << std::flush;
  std::string note{};
  if (!std::getline(std::cin, note)) return false;
  note = trim_ascii(note);

  std::string structured{};
  if (!(dismiss ? dismiss_terminal_report(app, loop, note, &structured, error)
                : acknowledge_terminal_report(app, loop, note, &structured,
                                              error))) {
    return false;
  }
  std::cout << "\nHuman report "
            << (dismiss ? "dismissal" : "acknowledgment")
            << " applied.\n"
            << structured << "\n";
  return true;
}

bool run_interactive_request_responder(app_context_t* app, std::string* error) {
  if (error) error->clear();
  if (!terminal_supports_human_ncurses_ui()) {
    std::cerr << "[hero_human_mcp] terminal does not support ncurses UI"
                 " cleanly (TERM="
              << trim_ascii(std::getenv("TERM") == nullptr ? "" : std::getenv("TERM"))
              << "); using line prompt responder instead.\n";
    return run_line_prompt_request_responder(app, error);
  }
  std::string ncurses_error{};
  if (run_ncurses_request_responder(app, &ncurses_error)) return true;
  if (!ncurses_error.empty() &&
      ncurses_error.rfind(kNcursesInitErrorPrefix, 0) == 0) {
    return run_line_prompt_request_responder(app, error);
  }
  if (error) *error = ncurses_error.empty() ? "interactive human responder failed"
                                            : ncurses_error;
  return false;
}

void run_jsonrpc_stdio_loop(app_context_t* app) {
  std::string line;
  while (std::getline(std::cin, line)) {
    const std::string request_json = trim_ascii(line);
    if (request_json.empty()) continue;
    if (request_json.size() > kMaxJsonRpcPayloadBytes) continue;

    std::string id_raw{"null"};
    std::string method{};
    std::string params_json{"{}"};
    if (extract_json_field_raw(request_json, "id", &id_raw)) {
      id_raw = trim_ascii(id_raw);
      if (id_raw.empty()) id_raw = "null";
    }
    if (!extract_json_string_field(request_json, "method", &method) ||
        method.empty()) {
      std::cout
          << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
          << ",\"error\":{\"code\":-32600,\"message\":\"invalid request\"}}\n";
      std::cout.flush();
      continue;
    }
    (void)extract_json_object_field(request_json, "params", &params_json);

    if (method == "initialize") {
      std::cout
          << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
          << ",\"result\":{\"protocolVersion\":" << json_quote(kProtocolVersion)
          << ",\"serverInfo\":{\"name\":" << json_quote(kServerName)
          << ",\"version\":" << json_quote(kServerVersion)
          << "},\"instructions\":" << json_quote(kInitializeInstructions)
          << "}}\n";
      std::cout.flush();
      continue;
    }
    if (method == "notifications/initialized") continue;
    if (method == "ping") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":{}}\n";
      std::cout.flush();
      continue;
    }
    if (method == "tools/list") {
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":" << build_tools_list_result_json() << "}\n";
      std::cout.flush();
      continue;
    }
    if (method == "tools/call") {
      std::string name{};
      std::string arguments{"{}"};
      if (!extract_json_string_field(params_json, "name", &name) ||
          name.empty()) {
        std::cout
            << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
            << ",\"error\":{\"code\":-32602,\"message\":\"missing tool name\"}}\n";
        std::cout.flush();
        continue;
      }
      std::string args_object{};
      if (extract_json_object_field(params_json, "arguments", &args_object)) {
        arguments = std::move(args_object);
      }
      std::string tool_result{};
      std::string tool_error{};
      if (!execute_tool_json(name, arguments, app, &tool_result, &tool_error)) {
        std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                  << ",\"error\":{\"code\":-32603,\"message\":"
                  << json_quote(tool_error) << "}}\n";
        std::cout.flush();
        continue;
      }
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":" << tool_result << "}\n";
      std::cout.flush();
      continue;
    }
    std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
              << ",\"error\":{\"code\":-32601,\"message\":\"method not found\"}}\n";
    std::cout.flush();
  }
}

}  // namespace human_mcp
}  // namespace hero
}  // namespace cuwacunu
