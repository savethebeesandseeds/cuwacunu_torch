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
#include "hero/marshal_hero/marshal_session.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"

namespace cuwacunu {
namespace hero {
namespace human_mcp {

constexpr const char* kServerName = "hero_human_mcp";
constexpr const char* kServerVersion = "0.1.0";
constexpr const char* kProtocolVersion = "2025-03-26";
constexpr const char* kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.human.*.";
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;
constexpr std::size_t kMarshalToolTimeoutSec = 30;
constexpr const char* kNcursesInitErrorPrefix = "ncurses init failed: ";
constexpr std::string_view kHumanClarificationAnswerSchemaV3 =
    "hero.human.clarification_answer.v3";
constexpr std::string_view kHumanSummaryAckSchemaV3 =
    "hero.human.summary_ack.v3";

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

struct detail_viewport_t {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
  int scrollbar_x = 0;
  std::size_t top_line = 0;
  std::size_t page_lines = 0;
  std::size_t total_lines = 0;
};

[[nodiscard]] bool collect_pending_requests(
    const app_context_t& app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t>* out,
    std::string* error);

[[nodiscard]] bool collect_all_sessions(
    const app_context_t& app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t>* out,
    std::string* error);

[[nodiscard]] bool collect_pending_reviews(
    const app_context_t& app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t>* out,
    std::string* error);

[[nodiscard]] bool collect_human_operator_inbox(
    const app_context_t& app, human_operator_inbox_t* out, bool sync_markers,
    std::string* error);

[[nodiscard]] bool build_resolution_and_apply(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string resolution_kind, std::string reason,
    std::string* out_structured, std::string* out_error);

[[nodiscard]] bool build_request_answer_and_resume(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string answer,
    std::string* out_structured, std::string* out_error);

[[nodiscard]] bool acknowledge_session_summary(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string note,
    std::string* out_structured, std::string* out_error);

[[nodiscard]] bool pause_marshal_session(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session, bool force,
    std::string* out_structured, std::string* out_error);

[[nodiscard]] bool resume_marshal_session(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string* out_structured, std::string* out_error);

[[nodiscard]] bool continue_marshal_session(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string instruction,
    std::string* out_structured, std::string* out_error);

[[nodiscard]] bool terminate_marshal_session(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session, bool force,
    std::string* out_structured, std::string* out_error);

[[nodiscard]] bool list_declared_bind_ids_for_session(
    const app_context_t& app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
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

[[nodiscard]] bool is_clarification_pause_kind(std::string_view pause_kind) {
  return pause_kind == "clarification";
}

[[nodiscard]] bool is_human_request_pause_kind(std::string_view pause_kind) {
  return is_clarification_pause_kind(pause_kind) ||
         pause_kind == "governance";
}

[[nodiscard]] std::string_view human_request_kind_label(
    std::string_view pause_kind) {
  if (is_clarification_pause_kind(pause_kind)) return "clarification";
  if (pause_kind == "governance") return "governance";
  if (pause_kind == "operator") return "operator";
  if (pause_kind == "none") return "none";
  return "unknown";
}

[[nodiscard]] std::optional<std::uint64_t> marshal_session_elapsed_ms(
    const cuwacunu::hero::marshal::marshal_session_record_t& session) {
  const std::uint64_t end_ms =
      session.finished_at_ms.has_value() ? *session.finished_at_ms
                                         : session.updated_at_ms;
  if (session.started_at_ms == 0 || end_ms < session.started_at_ms) {
    return std::nullopt;
  }
  return end_ms - session.started_at_ms;
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

[[nodiscard]] std::string build_summary_effort_text(
    const cuwacunu::hero::marshal::marshal_session_record_t& session) {
  std::ostringstream out;
  out << "elapsed="
      << format_compact_duration_ms(marshal_session_elapsed_ms(session))
      << " | checkpoints=" << session.checkpoint_count
      << " | launches="
      << format_effort_usage(session.launch_count, session.max_campaign_launches,
                             session.remaining_campaign_launches);
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

void sort_sessions_newest_first(
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t>* rows) {
  if (!rows) return;
  std::sort(rows->begin(), rows->end(), [](const auto& a, const auto& b) {
    if (a.updated_at_ms != b.updated_at_ms) return a.updated_at_ms > b.updated_at_ms;
    return a.marshal_session_id > b.marshal_session_id;
  });
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
      "{\"marshal_session_id\":\"\",\"error\":" + json_quote(message) + "}", true);
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

[[nodiscard]] bool list_declared_bind_ids_for_session(
    const app_context_t& app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
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
  if (!cuwacunu::hero::runtime::read_text_file(session.campaign_dsl_path,
                                               &campaign_text, error)) {
    return false;
  }

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  try {
    instruction = cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
        grammar_text, campaign_text);
  } catch (const std::exception& ex) {
    if (error) {
      *error = "failed to decode campaign DSL snapshot '" +
               session.campaign_dsl_path + "': " + ex.what();
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
  return app.defaults.marshal_root / ".human_io";
}

[[nodiscard]] std::string make_human_tool_io_basename_(
    std::string_view stem) {
  const std::uint64_t started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  const std::uint64_t salt = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  return "h." + cuwacunu::hero::runtime::base36_lower_u64(started_at_ms) +
         "." +
         cuwacunu::hero::runtime::base36_lower_u64(
             static_cast<std::uint64_t>(::getpid())) +
         "." +
         cuwacunu::hero::runtime::base36_lower_u64(salt).substr(0, 6) + "." +
         std::string(stem);
}

[[nodiscard]] std::filesystem::path make_human_tool_io_path(
    const app_context_t& app, std::string_view stem) {
  return human_tool_io_dir(app) / make_human_tool_io_basename_(stem);
}

void cleanup_human_tool_io(const std::filesystem::path& stdin_path,
                           const std::filesystem::path& stdout_path,
                           const std::filesystem::path& stderr_path) {
  std::error_code ec{};
  (void)std::filesystem::remove(stdin_path, ec);
  (void)std::filesystem::remove(stdout_path, ec);
  (void)std::filesystem::remove(stderr_path, ec);
}

[[nodiscard]] bool call_marshal_tool(const app_context_t& app,
                                   const std::string& tool_name,
                                   std::string arguments_json,
                                   std::string* out_structured,
                                   std::string* error) {
  if (error) error->clear();
  if (!out_structured) {
    if (error) *error = "marshal structured output pointer is null";
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
      app.defaults.marshal_hero_binary.string(), "--global-config",
      app.global_config_path.string(), "--tool", tool_name, "--args-json",
      arguments_json};
  int exit_code = -1;
  std::string invoke_error{};
  const bool invoked = run_command_with_stdio_and_timeout(
      argv, stdin_path, stdout_path, stderr_path, kMarshalToolTimeoutSec,
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
                   : ("marshal tool call produced no stdout: " + tool_name);
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
                   : ("marshal tool exited non-zero: " + tool_name);
    }
    return false;
  }
  return true;
}

[[nodiscard]] std::string request_excerpt(
    const cuwacunu::hero::marshal::marshal_session_record_t& session) {
  std::string text{};
  std::string ignored{};
  if (!cuwacunu::hero::runtime::read_text_file(session.human_request_path, &text,
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

[[nodiscard]] std::filesystem::path summary_path_for_session(
    const cuwacunu::hero::marshal::marshal_session_record_t& session) {
  return cuwacunu::hero::marshal::marshal_session_human_summary_path(
      std::filesystem::path(session.session_root).parent_path(), session.marshal_session_id);
}

[[nodiscard]] std::filesystem::path summary_ack_path_for_session(
    const cuwacunu::hero::marshal::marshal_session_record_t& session) {
  return cuwacunu::hero::marshal::marshal_session_human_summary_ack_latest_path(
      std::filesystem::path(session.session_root).parent_path(), session.marshal_session_id);
}

[[nodiscard]] std::filesystem::path summary_ack_sig_path_for_session(
    const cuwacunu::hero::marshal::marshal_session_record_t& session) {
  return cuwacunu::hero::marshal::marshal_session_human_summary_ack_latest_sig_path(
      std::filesystem::path(session.session_root).parent_path(), session.marshal_session_id);
}

[[nodiscard]] bool read_summary_text_for_session(
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string* out,
    std::string* error) {
  if (!out) {
    if (error) *error = "summary text output pointer is null";
    return false;
  }
  return cuwacunu::hero::runtime::read_text_file(
      summary_path_for_session(session), out, error);
}

[[nodiscard]] std::string summary_excerpt(
    const cuwacunu::hero::marshal::marshal_session_record_t& session) {
  std::string text{};
  std::string ignored{};
  if (!read_summary_text_for_session(session, &text, &ignored)) return {};
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

[[nodiscard]] bool summary_ack_matches_current_summary(
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string* error) {
  return cuwacunu::hero::marshal::marshal_session_summary_ack_matches_current_summary(
      session, error);
}

[[nodiscard]] bool write_human_pending_marker_counts(
    const std::filesystem::path& marshal_root, std::size_t pending_requests,
    std::size_t pending_reviews, std::string* error) {
  if (error) error->clear();
  const std::filesystem::path marker_dir =
      cuwacunu::hero::marshal::human_hero_runtime_dir(marshal_root);
  std::error_code ec{};
  std::filesystem::create_directories(marker_dir, ec);
  if (ec) {
    if (error) {
      *error = "cannot create Human Hero runtime dir: " + marker_dir.string();
    }
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          cuwacunu::hero::marshal::human_hero_pending_count_path(marshal_root),
          std::to_string(pending_requests) + "\n", error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          cuwacunu::hero::marshal::human_hero_pending_summary_count_path(marshal_root),
          std::to_string(pending_reviews) + "\n", error)) {
    return false;
  }
  return true;
}

void sync_human_pending_markers_best_effort(const app_context_t& app) {
  human_operator_inbox_t inbox{};
  std::string marker_error{};
  if (!collect_human_operator_inbox(app, &inbox, true, &marker_error)) {
    std::cerr << "[hero_human_mcp][warning] failed to refresh Human Hero "
                 "inbox markers: "
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
  use_default_colors();
  init_pair(1, COLOR_CYAN, -1);
  init_pair(2, COLOR_BLACK, COLOR_CYAN);
  init_pair(3, COLOR_YELLOW, -1);
  init_pair(4, COLOR_RED, -1);
  init_pair(5, COLOR_GREEN, -1);
  init_pair(6, COLOR_WHITE, -1);
  init_pair(7, COLOR_CYAN, -1);
  init_pair(8, COLOR_YELLOW, -1);
  init_pair(9, COLOR_GREEN, -1);
  init_pair(10, COLOR_MAGENTA, -1);
  init_pair(11, COLOR_BLUE, -1);
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

[[nodiscard]] std::size_t draw_wrapped_region_block_scrolled(
    int y, int x, int width, int height, std::string_view text,
    std::size_t top_line, std::size_t* out_actual_top_line = nullptr) {
  if (out_actual_top_line) *out_actual_top_line = 0;
  if (width <= 0 || height <= 0) return 0;
  const auto lines = wrap_text_lines(text, width);
  const std::size_t total_lines = lines.size();
  const std::size_t max_top_line =
      total_lines > static_cast<std::size_t>(height)
          ? total_lines - static_cast<std::size_t>(height)
          : 0u;
  const std::size_t actual_top_line = std::min(top_line, max_top_line);
  if (out_actual_top_line) *out_actual_top_line = actual_top_line;
  for (int i = 0; i < height; ++i) clear_region_line(y + i, x, width);
  for (int i = 0; i < height; ++i) {
    const std::size_t src = actual_top_line + static_cast<std::size_t>(i);
    if (src >= total_lines) break;
    mvaddnstr(y + i, x, lines[src].c_str(), width);
  }
  return total_lines;
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

[[nodiscard]] int session_accent_color(
    const cuwacunu::hero::marshal::marshal_session_record_t& session) {
  if (session.phase == "paused") {
    return session.pause_kind == "governance" ? 8 : 7;
  }
  if (session.phase == "idle") return 9;
  if (session.phase == "finished") {
    if (session.finish_reason == "failed" ||
        session.finish_reason == "terminated") {
      return 4;
    }
    if (session.finish_reason == "exhausted") return 3;
    return 11;
  }
  if (session.phase == "running_campaign") return 1;
  return 6;
}

[[nodiscard]] operator_session_state_t operator_session_state(
    const cuwacunu::hero::marshal::marshal_session_record_t& session) {
  if (session.phase == "paused") {
    if (is_clarification_pause_kind(session.pause_kind)) {
      return operator_session_state_t::waiting_clarification;
    }
    if (session.pause_kind == "governance") {
      return operator_session_state_t::waiting_governance;
    }
    if (session.pause_kind == "operator") {
      return operator_session_state_t::operator_paused;
    }
    return operator_session_state_t::unknown;
  }
  if (session.phase == "idle") return operator_session_state_t::review;
  if (session.phase == "finished") return operator_session_state_t::done;
  if (session.phase == "active" || session.phase == "running_campaign") {
    return operator_session_state_t::running;
  }
  return operator_session_state_t::unknown;
}

[[nodiscard]] std::string_view operator_session_state_label(
    operator_session_state_t state) {
  switch (state) {
    case operator_session_state_t::running:
      return "Running";
    case operator_session_state_t::waiting_clarification:
      return "Waiting: Clarification";
    case operator_session_state_t::waiting_governance:
      return "Waiting: Governance";
    case operator_session_state_t::operator_paused:
      return "Operator Paused";
    case operator_session_state_t::review:
      return "Review";
    case operator_session_state_t::done:
      return "Done";
    case operator_session_state_t::unknown:
      break;
  }
  return "Unknown";
}

[[nodiscard]] std::string operator_session_state_detail(
    const cuwacunu::hero::marshal::marshal_session_record_t& session) {
  std::ostringstream out;
  out << operator_session_state_label(operator_session_state(session));
  if (session.phase == "active") {
    out << " (planning)";
  } else if (session.phase == "running_campaign") {
    out << " (campaign active)";
  } else if (session.phase == "idle") {
    if (session.finish_reason == "failed") {
      out << " (review after failure)";
    } else {
      out << " (review pending)";
    }
  } else if (session.phase == "finished" && !session.finish_reason.empty() &&
             session.finish_reason != "none") {
    out << " (" << session.finish_reason << ")";
  }
  return out.str();
}

[[nodiscard]] std::string operator_session_action_hint(
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    bool has_pending_review) {
  const auto state = operator_session_state(session);
  if ((state == operator_session_state_t::review ||
       state == operator_session_state_t::done) &&
      !has_pending_review) {
    return "<none>";
  }
  switch (state) {
    case operator_session_state_t::running:
      return "Pause | Terminate";
    case operator_session_state_t::waiting_clarification:
      return "Answer clarification";
    case operator_session_state_t::waiting_governance:
      return "Grant | Deny | Clarify | Terminate";
    case operator_session_state_t::operator_paused:
      return "Resume | Terminate";
    case operator_session_state_t::review:
      return "Continue | Acknowledge";
    case operator_session_state_t::done:
      return "Acknowledge";
    case operator_session_state_t::unknown:
      break;
  }
  return "<none>";
}

[[nodiscard]] std::string session_state_badge(
    const cuwacunu::hero::marshal::marshal_session_record_t& session) {
  switch (operator_session_state(session)) {
    case operator_session_state_t::waiting_clarification:
      return "[CLARIFY]";
    case operator_session_state_t::waiting_governance:
      return "[GOV]";
    case operator_session_state_t::operator_paused:
      return "[PAUSED]";
    case operator_session_state_t::review:
      return "[REVIEW]";
    case operator_session_state_t::done:
      if (session.finish_reason == "success") return "[DONE]";
      if (session.finish_reason == "exhausted") return "[EXHAUSTED]";
      if (session.finish_reason == "failed") return "[FAILED]";
      if (session.finish_reason == "terminated") return "[TERMINATED]";
      return "[DONE]";
    case operator_session_state_t::running:
      return "[RUNNING]";
    case operator_session_state_t::unknown:
      break;
  }
  return "[SESSION]";
}

[[nodiscard]] std::string_view operator_console_view_label(
    operator_console_view_t view) {
  switch (view) {
    case operator_console_view_t::sessions:
      return "overview";
    case operator_console_view_t::requests:
      return "requests";
    case operator_console_view_t::summaries:
      return "reviews";
  }
  return "overview";
}

[[nodiscard]] std::string_view operator_console_phase_filter_label(
    operator_console_phase_filter_t filter) {
  switch (filter) {
    case operator_console_phase_filter_t::all:
      return "all";
    case operator_console_phase_filter_t::active:
      return "planning";
    case operator_console_phase_filter_t::running_campaign:
      return "campaign";
    case operator_console_phase_filter_t::paused:
      return "waiting/paused";
    case operator_console_phase_filter_t::idle:
      return "review";
    case operator_console_phase_filter_t::finished:
      return "done";
  }
  return "all";
}

[[nodiscard]] bool session_matches_phase_filter(
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    operator_console_phase_filter_t filter) {
  switch (filter) {
    case operator_console_phase_filter_t::all:
      return true;
    case operator_console_phase_filter_t::active:
      return session.phase == "active";
    case operator_console_phase_filter_t::running_campaign:
      return session.phase == "running_campaign";
    case operator_console_phase_filter_t::paused:
      return session.phase == "paused";
    case operator_console_phase_filter_t::idle:
      return session.phase == "idle";
    case operator_console_phase_filter_t::finished:
      return session.phase == "finished";
  }
  return true;
}

void filter_sessions_for_console(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>& all_sessions,
    operator_console_phase_filter_t filter,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t>* out) {
  if (!out) return;
  out->clear();
  out->reserve(all_sessions.size());
  for (const auto& session : all_sessions) {
    if (session_matches_phase_filter(session, filter)) out->push_back(session);
  }
}

[[nodiscard]] std::size_t count_sessions_in_operator_state(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>& sessions,
    operator_session_state_t state) {
  return static_cast<std::size_t>(std::count_if(
      sessions.begin(), sessions.end(), [state](const auto& session) {
        return operator_session_state(session) == state;
      }));
}

[[nodiscard]] std::optional<std::size_t> find_session_index_by_id(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>& sessions,
    std::string_view marshal_session_id) {
  for (std::size_t i = 0; i < sessions.size(); ++i) {
    if (sessions[i].marshal_session_id == marshal_session_id) return i;
  }
  return std::nullopt;
}

void select_session_by_id(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>& sessions,
    std::string_view marshal_session_id, std::size_t* selected) {
  if (!selected) return;
  const auto found = find_session_index_by_id(sessions, marshal_session_id);
  if (found.has_value()) *selected = *found;
}

void cycle_operator_console_view(operator_console_view_t* view) {
  if (!view) return;
  switch (*view) {
    case operator_console_view_t::sessions:
      *view = operator_console_view_t::requests;
      return;
    case operator_console_view_t::requests:
      *view = operator_console_view_t::summaries;
      return;
    case operator_console_view_t::summaries:
      *view = operator_console_view_t::sessions;
      return;
  }
}

void cycle_operator_console_phase_filter(operator_console_phase_filter_t* filter) {
  if (!filter) return;
  switch (*filter) {
    case operator_console_phase_filter_t::all:
      *filter = operator_console_phase_filter_t::active;
      return;
    case operator_console_phase_filter_t::active:
      *filter = operator_console_phase_filter_t::running_campaign;
      return;
    case operator_console_phase_filter_t::running_campaign:
      *filter = operator_console_phase_filter_t::paused;
      return;
    case operator_console_phase_filter_t::paused:
      *filter = operator_console_phase_filter_t::idle;
      return;
    case operator_console_phase_filter_t::idle:
      *filter = operator_console_phase_filter_t::finished;
      return;
    case operator_console_phase_filter_t::finished:
      *filter = operator_console_phase_filter_t::all;
      return;
  }
}

void draw_detail_scroll_indicator(int y, int x, int width, std::size_t top_line,
                                  std::size_t page_lines,
                                  std::size_t total_lines) {
  clear_region_line(y, x, width);
  if (width <= 0 || page_lines == 0 || total_lines == 0) return;
  const std::size_t shown_from = top_line + 1;
  const std::size_t shown_to = std::min(total_lines, top_line + page_lines);
  std::ostringstream out;
  if (total_lines > page_lines) {
    out << "Scroll Wheel/Bar/PgUp/PgDn/Home/End | lines " << shown_from << "-"
        << shown_to << "/" << total_lines;
  } else {
    out << "Lines " << shown_from << "-" << shown_to << "/" << total_lines;
  }
  attron(COLOR_PAIR(3));
  mvaddnstr(y, x, ellipsize_text(out.str(), width).c_str(), width);
  attroff(COLOR_PAIR(3));
}

void draw_status_line(int y, int x, int width, std::string_view status_line,
                      bool status_is_error) {
  clear_region_line(y, x, width);
  if (width <= 0 || status_line.empty()) return;
  const std::string shown =
      "status: " + ellipsize_text(status_line, std::max(1, width - 8));
  attron(COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);
  mvaddnstr(y, x, shown.c_str(), width);
  attroff(COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);
}

void draw_vertical_scrollbar(int y, int x, int height, std::size_t top_line,
                             std::size_t page_lines,
                             std::size_t total_lines) {
  if (height <= 0) return;
  for (int row = 0; row < height; ++row) {
    mvaddch(y + row, x, ' ');
  }
  if (page_lines == 0 || total_lines == 0) return;
  attron(COLOR_PAIR(11));
  for (int row = 0; row < height; ++row) {
    mvaddch(y + row, x, ACS_VLINE);
  }
  attroff(COLOR_PAIR(11));
  if (total_lines <= page_lines) return;
  const std::size_t max_top_line = total_lines - page_lines;
  const int thumb_h =
      std::clamp(static_cast<int>((page_lines * static_cast<std::size_t>(height) +
                                   total_lines - 1) /
                                  total_lines),
                 1, height);
  const int travel = std::max(0, height - thumb_h);
  const int thumb_top =
      max_top_line == 0
          ? 0
          : static_cast<int>((top_line * static_cast<std::size_t>(travel) +
                              max_top_line / 2) /
                             max_top_line);
  attron(COLOR_PAIR(1) | A_BOLD);
  for (int row = 0; row < thumb_h; ++row) {
    mvaddch(y + thumb_top + row, x, ACS_CKBOARD);
  }
  attroff(COLOR_PAIR(1) | A_BOLD);
}

[[nodiscard]] bool mouse_hits_detail_viewport(const detail_viewport_t& viewport,
                                              int mouse_x, int mouse_y) {
  if (viewport.width <= 0 || viewport.height <= 0) return false;
  const int right_edge =
      viewport.scrollbar_x > 0 ? viewport.scrollbar_x : viewport.x + viewport.width - 1;
  return mouse_x >= viewport.x && mouse_x <= right_edge &&
         mouse_y >= viewport.y && mouse_y < viewport.y + viewport.height;
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
               "Enter=accept  Esc=cancel  Backspace=edit",
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

[[nodiscard]] bool read_request_text_for_session(
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string* out,
    std::string* error) {
  if (!out) {
    if (error) *error = "request text output pointer is null";
    return false;
  }
  return cuwacunu::hero::runtime::read_text_file(session.human_request_path, out,
                                                 error);
}

[[nodiscard]] std::string build_session_console_detail_text(
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    bool has_pending_review) {
  std::ostringstream out;
  out << "objective: " << session.objective_name << "\n"
      << "marshal_session_id: " << session.marshal_session_id << "\n"
      << "operator_state: " << operator_session_state_detail(session) << "\n"
      << "next_action: "
      << operator_session_action_hint(session, has_pending_review) << "\n"
      << "phase: " << session.phase;
  if (session.phase == "paused") {
    out << " (" << session.pause_kind << ")";
  } else if (session.phase == "finished") {
    out << " (" << session.finish_reason << ")";
  }
  out << "\n"
      << "effort: " << build_summary_effort_text(session) << "\n"
      << "authority_scope: " << session.authority_scope << "\n";
  if (!session.phase_detail.empty()) {
    out << "phase_detail: " << session.phase_detail << "\n";
  }
  if (!session.last_intent_kind.empty()) {
    out << "last_intent: " << session.last_intent_kind << "\n";
  }
  if (!session.active_campaign_cursor.empty()) {
    out << "active_campaign: " << session.active_campaign_cursor << "\n";
  }
  if (!session.last_warning.empty()) {
    out << "last_warning: " << session.last_warning << "\n";
  }
  if (!session.codex_session_id.empty()) {
    out << "codex_session_id: " << session.codex_session_id << "\n";
  }
  if (!session.campaign_dsl_path.empty()) {
    out << "campaign_dsl: " << session.campaign_dsl_path << "\n";
  }
  if (!session.campaign_cursors.empty()) {
    out << "campaign_history: ";
    for (std::size_t i = 0; i < session.campaign_cursors.size(); ++i) {
      if (i != 0) out << ", ";
      out << session.campaign_cursors[i];
    }
    out << "\n";
  }

  if (session.phase == "paused" &&
      is_human_request_pause_kind(session.pause_kind)) {
    std::string request_text{};
    std::string request_error{};
    out << "\n--- Human Request ---\n";
    if (read_request_text_for_session(session, &request_text, &request_error)) {
      out << request_text;
    } else {
      out << "<failed to read request: " << request_error << ">";
    }
  }

  if (cuwacunu::hero::marshal::is_marshal_session_summary_state(session.phase)) {
    std::string summary_text{};
    std::string summary_error{};
    out << "\n\n--- Session Summary ---\n";
    if (read_summary_text_for_session(session, &summary_text, &summary_error)) {
      out << summary_text;
    } else {
      out << "<failed to read summary: " << summary_error << ">";
    }
  }
  return out.str();
}

void render_human_sessions_screen(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>& all_sessions,
    std::size_t request_count, std::size_t summary_count,
    operator_console_phase_filter_t phase_filter, std::string_view operator_id,
    std::string_view marshal_root, std::size_t selected,
    std::string_view detail_text, std::size_t detail_scroll,
    std::size_t* out_detail_total_lines, std::size_t* out_detail_page_lines,
    std::size_t* out_actual_detail_scroll, detail_viewport_t* out_detail_viewport,
    std::string_view status_line, bool status_is_error) {
  if (out_detail_total_lines) *out_detail_total_lines = 0;
  if (out_detail_page_lines) *out_detail_page_lines = 0;
  if (out_actual_detail_scroll) *out_actual_detail_scroll = 0;
  if (out_detail_viewport) *out_detail_viewport = detail_viewport_t{};

  const int W = COLS;
  const int H = LINES;
  if (W < 56 || H < 12) {
    erase();
    mvaddnstr(0, 0,
              "Human Hero: terminal too small. Resize or use hero.human.* tools.",
              std::max(0, W - 1));
    refresh();
    return;
  }

  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> sessions{};
  filter_sessions_for_console(all_sessions, phase_filter, &sessions);

  const int header_h = 4;
  const int footer_h = 3;
  const int body_h = H - header_h - footer_h;
  const int left_w = std::clamp(W / 3, 28, std::max(28, W - 24));
  const int right_w = W - left_w;

  if (has_colors()) attrset(COLOR_PAIR(6));
  erase();

  draw_boxed_region(0, 0, header_h, W, " Human Hero ");
  attron(COLOR_PAIR(1) | A_BOLD);
  const std::string header_line =
      "Operator session console | tracked: " +
      std::to_string(all_sessions.size()) + " | requests: " +
      std::to_string(request_count) + " | reviews: " +
      std::to_string(summary_count) + " | lane: " +
      std::string(operator_console_view_label(operator_console_view_t::sessions)) +
      " | overview filter: " +
      std::string(operator_console_phase_filter_label(phase_filter)) +
      " | operator: " +
      std::string(operator_id.empty() ? "<unset>" : operator_id);
  mvaddnstr(1, 2, ellipsize_text(header_line, W - 4).c_str(), W - 4);
  attroff(COLOR_PAIR(1) | A_BOLD);
  draw_status_line(2, 2, W - 4, status_line, status_is_error);

  draw_boxed_region(header_h, 0, body_h, left_w,
                    sessions.empty() ? " Overview " : " Tracked Sessions ");
  if (all_sessions.empty()) {
    draw_centered_region_line(header_h + 2, 0, left_w, "No Sessions", 5, true);
    draw_centered_region_line(header_h + 4, 0, left_w,
                              "Start a Marshal Hero session to populate this console.");
    draw_centered_region_line(header_h + 6, 0, left_w,
                              "Press r to refresh.");
    draw_centered_region_line(header_h + 7, 0, left_w,
                              "Press q to leave the console.");
  } else if (sessions.empty()) {
    draw_centered_region_line(header_h + 2, 0, left_w, "No Matching Sessions", 3,
                              true);
    draw_centered_region_line(
        header_h + 4, 0, left_w,
        "The current state filter hides all known sessions.");
    draw_centered_region_line(header_h + 6, 0, left_w,
                              "Press f to cycle the overview filter.");
    draw_centered_region_line(header_h + 7, 0, left_w,
                              "Press Tab to cycle lanes.");
  } else {
    const int max_rows = body_h - 2;
    std::size_t top = 0;
    if (selected >= static_cast<std::size_t>(max_rows)) {
      top = selected - static_cast<std::size_t>(max_rows) + 1;
    }
    for (int row = 0; row < max_rows; ++row) {
      const std::size_t idx = top + static_cast<std::size_t>(row);
      if (idx >= sessions.size()) break;
      const auto& session = sessions[idx];
      const std::string label = session_state_badge(session) + " " +
                                session.objective_name + " | " + session.marshal_session_id;
      const int row_y = header_h + 1 + row;
      clear_region_line(row_y, 1, left_w - 2);
      const int accent_color = session_accent_color(session);
      if (idx == selected) {
        attron(COLOR_PAIR(2) | A_BOLD);
      } else if (accent_color > 0) {
        attron(COLOR_PAIR(accent_color) | A_BOLD);
      }
      mvaddnstr(row_y, 2, ellipsize_text(label, left_w - 4).c_str(), left_w - 4);
      if (idx == selected) {
        attroff(COLOR_PAIR(2) | A_BOLD);
      } else if (accent_color > 0) {
        attroff(COLOR_PAIR(accent_color) | A_BOLD);
      }
    }
  }

  draw_boxed_region(header_h, left_w, body_h, right_w,
                    sessions.empty() ? " Overview " : " Session Detail ");
  if (!sessions.empty()) {
    const auto& session = sessions[selected];
    const int accent_color = session_accent_color(session);
    std::ostringstream meta;
    meta << "session=" << session.marshal_session_id
         << "  state=" << operator_session_state_detail(session);
    meta << "  raw_phase=" << session.phase;
    if (session.phase == "paused") meta << "(" << session.pause_kind << ")";
    if (session.phase == "finished") meta << "(" << session.finish_reason << ")";
    attron(COLOR_PAIR(accent_color) | A_BOLD);
    mvaddnstr(header_h + 1, left_w + 2,
              ellipsize_text(meta.str(), right_w - 4).c_str(), right_w - 4);
    attroff(COLOR_PAIR(accent_color) | A_BOLD);
    const std::string subtitle =
        "running=" +
        std::to_string(count_sessions_in_operator_state(
            all_sessions, operator_session_state_t::running)) +
        " waiting_clarification=" +
        std::to_string(count_sessions_in_operator_state(
            all_sessions, operator_session_state_t::waiting_clarification)) +
        " waiting_governance=" +
        std::to_string(count_sessions_in_operator_state(
            all_sessions, operator_session_state_t::waiting_governance)) +
        " review=" +
        std::to_string(count_sessions_in_operator_state(
            all_sessions, operator_session_state_t::review)) +
        " done=" + std::to_string(count_sessions_in_operator_state(
                      all_sessions, operator_session_state_t::done));
    mvaddnstr(header_h + 2, left_w + 2,
              ellipsize_text(subtitle, right_w - 4).c_str(), right_w - 4);
    const int detail_x = left_w + 2;
    const int detail_y = header_h + 4;
    const int detail_width = std::max(1, right_w - 5);
    const int detail_height = std::max(1, body_h - 7);
    const int scrollbar_x = detail_x + detail_width + 1;
    std::size_t actual_detail_scroll = 0;
    const std::size_t detail_total_lines = draw_wrapped_region_block_scrolled(
        detail_y, detail_x, detail_width, detail_height, detail_text,
        detail_scroll, &actual_detail_scroll);
    draw_vertical_scrollbar(detail_y, scrollbar_x, detail_height,
                            actual_detail_scroll,
                            static_cast<std::size_t>(detail_height),
                            detail_total_lines);
    draw_detail_scroll_indicator(header_h + body_h - 2, left_w + 2, right_w - 4,
                                 actual_detail_scroll,
                                 static_cast<std::size_t>(detail_height),
                                 detail_total_lines);
    if (out_detail_total_lines) *out_detail_total_lines = detail_total_lines;
    if (out_detail_page_lines) {
      *out_detail_page_lines = static_cast<std::size_t>(detail_height);
    }
    if (out_actual_detail_scroll) *out_actual_detail_scroll = actual_detail_scroll;
    if (out_detail_viewport) {
      *out_detail_viewport = detail_viewport_t{
          .x = detail_x,
          .y = detail_y,
          .width = detail_width,
          .height = detail_height,
          .scrollbar_x = scrollbar_x,
          .top_line = actual_detail_scroll,
          .page_lines = static_cast<std::size_t>(detail_height),
          .total_lines = detail_total_lines};
    }
  } else {
    std::ostringstream out;
    out << "This is the overview lane.\n\n"
        << "Operator: "
        << (operator_id.empty() ? "<unset>" : std::string(operator_id)) << "\n"
        << "Marshal root: " << marshal_root << "\n\n"
        << "What to do here:\n"
        << "- inspect every known session, not only pending operator work\n"
        << "- use f to cycle overview filters\n"
        << "- use Tab to cycle between overview, requests, and reviews\n"
        << "- use pause/resume/continue/terminate controls from the cockpit\n\n"
        << "Non-interactive alternative:\n"
        << "hero.marshal.list_sessions";
    draw_wrapped_region_block(header_h + 2, left_w + 2, right_w - 4, body_h - 4,
                              out.str());
  }

  draw_boxed_region(header_h + body_h, 0, footer_h, W, " Controls ");
  std::string controls =
      sessions.empty()
          ? "Tab cycle lanes  f cycle filter  r refresh  q quit"
          : "Tab cycle lanes  f cycle filter  Up/Down select  Wheel/PgUp/PgDn scroll detail  Home/End jump  Enter open request/review  p pause  u resume  c continue  t terminate  r refresh  q quit";
  mvaddnstr(header_h + body_h + 1, 2, ellipsize_text(controls, W - 4).c_str(),
            W - 4);

  refresh();
}

void render_human_requests_screen(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>& pending,
    std::size_t summary_count, std::string_view operator_id,
    std::string_view marshal_root, std::size_t selected,
    std::string_view request_text, std::size_t detail_scroll,
    std::size_t* out_detail_total_lines, std::size_t* out_detail_page_lines,
    std::size_t* out_actual_detail_scroll, detail_viewport_t* out_detail_viewport,
    std::string_view status_line, bool status_is_error) {
  if (out_detail_total_lines) *out_detail_total_lines = 0;
  if (out_detail_page_lines) *out_detail_page_lines = 0;
  if (out_actual_detail_scroll) *out_actual_detail_scroll = 0;
  if (out_detail_viewport) *out_detail_viewport = detail_viewport_t{};
  const int W = COLS;
  const int H = LINES;
  if (W < 56 || H < 12) {
    erase();
    mvaddnstr(0, 0, "Human Hero: terminal too small. Resize or use hero.human.* tools.",
              std::max(0, W - 1));
    refresh();
    return;
  }
  const int header_h = 4;
  const int footer_h = 3;
  const int body_h = H - header_h - footer_h;
  const int left_w = std::clamp(W / 3, 28, std::max(28, W - 24));
  const int right_w = W - left_w;

  if (has_colors()) attrset(COLOR_PAIR(6));
  erase();

  draw_boxed_region(0, 0, header_h, W, " Human Hero ");
  attron(COLOR_PAIR(1) | A_BOLD);
  const std::string header_line =
      "Operator session console | requests: " +
      std::to_string(pending.size()) + " | reviews: " +
      std::to_string(summary_count) + " | lane: " +
      std::string(operator_console_view_label(operator_console_view_t::requests)) +
      " | operator: " +
      std::string(operator_id.empty() ? "<unset>" : operator_id);
  mvaddnstr(1, 2, ellipsize_text(header_line, W - 4).c_str(), W - 4);
  attroff(COLOR_PAIR(1) | A_BOLD);
  draw_status_line(2, 2, W - 4, status_line, status_is_error);

  draw_boxed_region(header_h, 0, body_h, left_w,
                    pending.empty() ? " Requests " : " Pending Requests ");
  if (pending.empty()) {
    draw_centered_region_line(header_h + 2, 0, left_w, "No Pending Requests",
                              5, true);
    draw_centered_region_line(header_h + 4, 0, left_w,
                              "Marshal Hero has no pending clarification or governance request.");
    draw_centered_region_line(
        header_h + 6, 0, left_w,
        summary_count == 0 ? "Press r to refresh the lane."
                           : "Press Tab to cycle lanes.");
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
      const auto& session = pending[idx];
      const std::string label = session_state_badge(session) + " " +
                                session.objective_name + " | " + session.marshal_session_id;
      const int row_y = header_h + 1 + row;
      clear_region_line(row_y, 1, left_w - 2);
      const int accent_color = session_accent_color(session);
      if (idx == selected) {
        attron(COLOR_PAIR(2) | A_BOLD);
      } else if (accent_color > 0) {
        attron(COLOR_PAIR(accent_color) | A_BOLD);
      }
      mvaddnstr(row_y, 2, ellipsize_text(label, left_w - 4).c_str(),
                left_w - 4);
      if (idx == selected) {
        attroff(COLOR_PAIR(2) | A_BOLD);
      } else if (accent_color > 0) {
        attroff(COLOR_PAIR(accent_color) | A_BOLD);
      }
    }
  }

  draw_boxed_region(header_h, left_w, body_h, right_w,
                    pending.empty() ? " Overview " : " Session Request ");
  if (!pending.empty()) {
    const auto& session = pending[selected];
    const int accent_color = session_accent_color(session);
    std::string meta = "session=" + session.marshal_session_id + "  checkpoint=" +
                       std::to_string(session.checkpoint_count);
    attron(COLOR_PAIR(accent_color) | A_BOLD);
    mvaddnstr(header_h + 1, left_w + 2,
              ellipsize_text(meta, right_w - 4).c_str(), right_w - 4);
    attroff(COLOR_PAIR(accent_color) | A_BOLD);
    mvaddnstr(header_h + 2, left_w + 2,
              ellipsize_text(operator_session_state_detail(session) + " | " +
                                 (session.phase_detail.empty()
                                      ? session.objective_name
                                      : session.phase_detail),
                             right_w - 4)
                  .c_str(),
              right_w - 4);
    const int detail_x = left_w + 2;
    const int detail_y = header_h + 4;
    const int detail_width = std::max(1, right_w - 5);
    const int detail_height = std::max(1, body_h - 7);
    const int scrollbar_x = detail_x + detail_width + 1;
    std::size_t actual_detail_scroll = 0;
    const std::size_t detail_total_lines = draw_wrapped_region_block_scrolled(
        detail_y, detail_x, detail_width, detail_height, request_text,
        detail_scroll, &actual_detail_scroll);
    draw_vertical_scrollbar(detail_y, scrollbar_x, detail_height,
                            actual_detail_scroll,
                            static_cast<std::size_t>(detail_height),
                            detail_total_lines);
    draw_detail_scroll_indicator(header_h + body_h - 2, left_w + 2, right_w - 4,
                                 actual_detail_scroll,
                                 static_cast<std::size_t>(detail_height),
                                 detail_total_lines);
    if (out_detail_total_lines) *out_detail_total_lines = detail_total_lines;
    if (out_detail_page_lines) {
      *out_detail_page_lines = static_cast<std::size_t>(detail_height);
    }
    if (out_actual_detail_scroll) *out_actual_detail_scroll = actual_detail_scroll;
    if (out_detail_viewport) {
      *out_detail_viewport = detail_viewport_t{
          .x = detail_x,
          .y = detail_y,
          .width = detail_width,
          .height = detail_height,
          .scrollbar_x = scrollbar_x,
          .top_line = actual_detail_scroll,
          .page_lines = static_cast<std::size_t>(detail_height),
          .total_lines = detail_total_lines};
    }
  } else {
    std::ostringstream out;
    out << "This lane becomes active when a Marshal Hero session pauses for "
           "clarification or governance.\n\n"
        << "Operator: "
        << (operator_id.empty() ? "<unset>" : std::string(operator_id)) << "\n"
        << "Marshal root: " << marshal_root << "\n\n"
        << "What to do here:\n"
        << "- wait for a paused session to appear\n"
        << "- press Tab to cycle lanes when overview or reviews are relevant\n"
        << "- press r to refresh\n"
        << "- press q to exit\n\n"
        << "Non-interactive alternative:\n"
        << "hero.human.list_requests";
    draw_wrapped_region_block(header_h + 2, left_w + 2, right_w - 4, body_h - 4,
                              out.str());
  }

  draw_boxed_region(header_h + body_h, 0, footer_h, W, " Controls ");
  std::string controls =
      pending.empty()
          ? "r refresh  q quit"
          : "Up/Down select  Wheel/PgUp/PgDn scroll detail  Home/End jump  Enter inspect/respond  r refresh  q quit";
  if (summary_count > 0) controls = "Tab cycle lanes  " + controls;
  mvaddnstr(header_h + body_h + 1, 2,
            ellipsize_text(controls, W - 4).c_str(), W - 4);

  refresh();
}

void render_human_reviews_screen(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>& pending,
    std::size_t request_count, std::string_view operator_id,
    std::string_view marshal_root, std::size_t selected,
    std::string_view summary_text, std::size_t detail_scroll,
    std::size_t* out_detail_total_lines, std::size_t* out_detail_page_lines,
    std::size_t* out_actual_detail_scroll, detail_viewport_t* out_detail_viewport,
    std::string_view status_line, bool status_is_error) {
  if (out_detail_total_lines) *out_detail_total_lines = 0;
  if (out_detail_page_lines) *out_detail_page_lines = 0;
  if (out_actual_detail_scroll) *out_actual_detail_scroll = 0;
  if (out_detail_viewport) *out_detail_viewport = detail_viewport_t{};
  const int W = COLS;
  const int H = LINES;
  if (W < 56 || H < 12) {
    erase();
    mvaddnstr(0, 0,
              "Human Hero: terminal too small. Resize or use hero.human.* tools.",
              std::max(0, W - 1));
    refresh();
    return;
  }
  const int header_h = 4;
  const int footer_h = 3;
  const int body_h = H - header_h - footer_h;
  const int left_w = std::clamp(W / 3, 28, std::max(28, W - 24));
  const int right_w = W - left_w;

  if (has_colors()) attrset(COLOR_PAIR(6));
  erase();

  draw_boxed_region(0, 0, header_h, W, " Human Hero ");
  attron(COLOR_PAIR(1) | A_BOLD);
  const std::string header_line =
      "Operator session console | requests: " +
      std::to_string(request_count) + " | reviews: " +
      std::to_string(pending.size()) + " | lane: " +
      std::string(
          operator_console_view_label(operator_console_view_t::summaries)) +
      " | operator: " +
      std::string(operator_id.empty() ? "<unset>" : operator_id);
  mvaddnstr(1, 2, ellipsize_text(header_line, W - 4).c_str(), W - 4);
  attroff(COLOR_PAIR(1) | A_BOLD);
  draw_status_line(2, 2, W - 4, status_line, status_is_error);

  draw_boxed_region(header_h, 0, body_h, left_w,
                    pending.empty() ? " Reviews " : " Pending Reviews ");
  if (pending.empty()) {
    draw_centered_region_line(header_h + 2, 0, left_w, "No Pending Reviews", 5,
                              true);
    draw_centered_region_line(
        header_h + 4, 0, left_w,
        "Idle and finished Marshal Hero review reports appear here.");
    draw_centered_region_line(
        header_h + 6, 0, left_w,
        request_count == 0 ? "Press r to refresh the lane."
                           : "Press Tab to cycle lanes.");
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
      const auto& session = pending[idx];
      const std::string label = session_state_badge(session) + " " +
                                session.objective_name + " | " + session.marshal_session_id;
      const int row_y = header_h + 1 + row;
      clear_region_line(row_y, 1, left_w - 2);
      const int accent_color = session_accent_color(session);
      if (idx == selected) {
        attron(COLOR_PAIR(2) | A_BOLD);
      } else if (accent_color > 0) {
        attron(COLOR_PAIR(accent_color) | A_BOLD);
      }
      mvaddnstr(row_y, 2, ellipsize_text(label, left_w - 4).c_str(),
                left_w - 4);
      if (idx == selected) {
        attroff(COLOR_PAIR(2) | A_BOLD);
      } else if (accent_color > 0) {
        attroff(COLOR_PAIR(accent_color) | A_BOLD);
      }
    }
  }

  draw_boxed_region(header_h, left_w, body_h, right_w,
                    pending.empty() ? " Overview " : " Review Report ");
  if (!pending.empty()) {
    const auto& session = pending[selected];
    const int accent_color = session_accent_color(session);
    std::string meta = "review report | session=" + session.marshal_session_id +
                       "  finish_reason=" + session.finish_reason;
    attron(COLOR_PAIR(accent_color) | A_BOLD);
    mvaddnstr(header_h + 1, left_w + 2,
              ellipsize_text(meta, right_w - 4).c_str(), right_w - 4);
    attroff(COLOR_PAIR(accent_color) | A_BOLD);
    const std::string subtitle = operator_session_state_detail(session) + " | " +
                                 build_summary_effort_text(session);
    mvaddnstr(header_h + 2, left_w + 2,
              ellipsize_text(subtitle, right_w - 4).c_str(), right_w - 4);
    const int detail_x = left_w + 2;
    const int detail_y = header_h + 4;
    const int detail_width = std::max(1, right_w - 5);
    const int detail_height = std::max(1, body_h - 7);
    const int scrollbar_x = detail_x + detail_width + 1;
    std::size_t actual_detail_scroll = 0;
    const std::size_t detail_total_lines = draw_wrapped_region_block_scrolled(
        detail_y, detail_x, detail_width, detail_height, summary_text,
        detail_scroll, &actual_detail_scroll);
    draw_vertical_scrollbar(detail_y, scrollbar_x, detail_height,
                            actual_detail_scroll,
                            static_cast<std::size_t>(detail_height),
                            detail_total_lines);
    draw_detail_scroll_indicator(header_h + body_h - 2, left_w + 2, right_w - 4,
                                 actual_detail_scroll,
                                 static_cast<std::size_t>(detail_height),
                                 detail_total_lines);
    if (out_detail_total_lines) *out_detail_total_lines = detail_total_lines;
    if (out_detail_page_lines) {
      *out_detail_page_lines = static_cast<std::size_t>(detail_height);
    }
    if (out_actual_detail_scroll) *out_actual_detail_scroll = actual_detail_scroll;
    if (out_detail_viewport) {
      *out_detail_viewport = detail_viewport_t{
          .x = detail_x,
          .y = detail_y,
          .width = detail_width,
          .height = detail_height,
          .scrollbar_x = scrollbar_x,
          .top_line = actual_detail_scroll,
          .page_lines = static_cast<std::size_t>(detail_height),
          .total_lines = detail_total_lines};
    }
  } else {
    std::ostringstream out;
    out << "This lane tracks idle and finished Marshal Hero review reports.\n\n"
        << "Operator: "
        << (operator_id.empty() ? "<unset>" : std::string(operator_id)) << "\n"
        << "Marshal root: " << marshal_root << "\n\n"
        << "What to do here:\n"
        << "- inspect the review report\n"
        << "- continue idle sessions with a fresh operator instruction when you want more work to launch\n"
        << "- acknowledge a report only after review; that signs it off with a required message and clears it from the queue\n"
        << "- finished sessions are informational; idle sessions are resumable\n"
        << "- press Tab to cycle lanes when overview or requests are relevant\n"
        << "- press r to refresh\n"
        << "- press q to exit\n\n"
        << "Non-interactive alternative:\n"
        << "hero.human.list_summaries";
    draw_wrapped_region_block(header_h + 2, left_w + 2, right_w - 4, body_h - 4,
                              out.str());
  }

  draw_boxed_region(header_h + body_h, 0, footer_h, W, " Controls ");
  std::string controls =
      pending.empty()
          ? "r refresh  q quit"
          : "Up/Down select  Wheel/PgUp/PgDn scroll detail  Home/End jump  Enter inspect  c continue session  a acknowledge review  r refresh  q quit";
  if (request_count > 0) controls = "Tab cycle lanes  " + controls;
  mvaddnstr(header_h + body_h + 1, 2,
            ellipsize_text(controls, W - 4).c_str(), W - 4);

  refresh();
}

void render_human_requests_bootstrap_screen(std::string_view operator_id,
                                            std::string_view marshal_root,
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
                  "[work] scanning Marshal Hero session console\n\n") +
          "operator: " +
          std::string(operator_id.empty() ? "<unset>" : operator_id) + "\n" +
          "marshal_root: " + std::string(marshal_root) + "\n\n"
          "The operator console will appear when the scan completes.");

  draw_boxed_region(header_h + body_h, 0, footer_h, W, " Status ");
  attron(COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);
  mvaddnstr(header_h + body_h + 1, 2, ellipsize_text(status_line, W - 4).c_str(),
            W - 4);
  attroff(COLOR_PAIR(status_is_error ? 4 : 5) | A_BOLD);

  refresh();
}

[[nodiscard]] bool collect_interactive_resolution_inputs(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
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
    out->resolution_kind = "terminate";
  } else {
    std::size_t resolution_idx = 0;
    if (!prompt_choice_dialog(
            " Resolution ",
            "Choose how to answer this pending governance request.",
            {"Grant requested governance",
             "Clarify objective or policy without widening authority",
             "Deny requested governance",
             "Terminate the session"},
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
      out->resolution_kind = "terminate";
    }
  }

  const std::string prompt_title =
      (out->resolution_kind == "grant")
          ? " Grant rationale "
          : (out->resolution_kind == "clarify")
                ? " Clarification "
                : (out->resolution_kind == "deny") ? " Denial rationale "
                                                    : " Termination rationale ";
  const std::string prompt_body =
      (out->resolution_kind == "grant")
          ? "Why grant this governance request? This reason is shown to Marshal Hero."
          : (out->resolution_kind == "clarify")
                ? "What clarification should Marshal Hero incorporate on the next planning checkpoint?"
                : (out->resolution_kind == "deny")
                      ? "Why deny this governance request? This reason is shown to Marshal Hero."
                      : "Why terminate this session? This reason is shown to Marshal Hero.";
  if (!prompt_text_dialog(prompt_title, prompt_body, &out->reason, false, false,
                          cancelled)) {
    return false;
  }
  if (cancelled && *cancelled) return true;

  (void)session;
  return true;
}

[[nodiscard]] bool run_ncurses_operator_console(app_context_t* app,
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
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);
    mouseinterval(0);

    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> all_sessions{};
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> pending_requests{};
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> pending_reviews{};
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> session_rows{};
    operator_console_view_t current_view = operator_console_view_t::sessions;
    operator_console_phase_filter_t session_phase_filter =
        operator_console_phase_filter_t::all;
    std::size_t selected_session = 0;
    std::size_t selected_request = 0;
    std::size_t selected_review = 0;
    std::size_t detail_scroll = 0;
    std::size_t detail_total_lines = 0;
    std::size_t detail_page_lines = 0;
    detail_viewport_t detail_viewport{};
    bool detail_dirty = true;
    std::string detail_text{};
    std::string detail_session_id{};
    operator_console_view_t detail_view = operator_console_view_t::sessions;
    std::string status = "Scanning operator console...";
    bool status_is_error = false;
    bool dirty = true;
    bool inbox_refresh_pending = true;
    bool inbox_refresh_needs_paint = true;
    bool has_loaded_once = false;

    for (;;) {
      if (inbox_refresh_pending && inbox_refresh_needs_paint) dirty = true;

      if (dirty) {
        if (!has_loaded_once && inbox_refresh_pending) {
          render_human_requests_bootstrap_screen(
              app->defaults.operator_id, app->defaults.marshal_root.string(),
              status, status_is_error);
        } else {
          filter_sessions_for_console(all_sessions, session_phase_filter,
                                      &session_rows);

          auto& visible =
              current_view == operator_console_view_t::sessions
                  ? session_rows
                  : (current_view == operator_console_view_t::requests
                         ? pending_requests
                         : pending_reviews);
          auto& selected =
              current_view == operator_console_view_t::sessions
                  ? selected_session
                  : (current_view == operator_console_view_t::requests
                         ? selected_request
                         : selected_review);
          if (!visible.empty()) {
            selected = std::min(selected, visible.size() - 1);
          } else {
            selected = 0;
          }

          if (visible.empty()) {
            detail_text.clear();
            detail_session_id.clear();
            detail_scroll = 0;
            detail_total_lines = 0;
            detail_page_lines = 0;
            detail_viewport = detail_viewport_t{};
            detail_dirty = false;
          } else if (detail_dirty || detail_session_id != visible[selected].marshal_session_id ||
                     detail_view != current_view) {
            if (current_view == operator_console_view_t::sessions) {
              const bool has_pending_review =
                  find_session_index_by_id(pending_reviews,
                                           visible[selected].marshal_session_id)
                      .has_value();
              detail_text = build_session_console_detail_text(
                  visible[selected], has_pending_review);
            } else {
              detail_text.clear();
              std::string detail_error{};
              const bool read_ok =
                  current_view == operator_console_view_t::requests
                      ? read_request_text_for_session(visible[selected], &detail_text,
                                                      &detail_error)
                      : read_summary_text_for_session(visible[selected], &detail_text,
                                                      &detail_error);
              if (!read_ok) {
                detail_text = std::string("<failed to read ") +
                              (current_view == operator_console_view_t::requests
                                   ? "request"
                                   : "review report") +
                              ": " + detail_error + ">";
              }
            }
            detail_session_id = visible[selected].marshal_session_id;
            detail_view = current_view;
            detail_dirty = false;
          }

          if (current_view == operator_console_view_t::sessions) {
            render_human_sessions_screen(
                all_sessions, pending_requests.size(), pending_reviews.size(),
                session_phase_filter, app->defaults.operator_id,
                app->defaults.marshal_root.string(), selected, detail_text,
                detail_scroll, &detail_total_lines, &detail_page_lines,
                &detail_scroll, &detail_viewport, status, status_is_error);
          } else if (current_view == operator_console_view_t::requests) {
            render_human_requests_screen(
                pending_requests, pending_reviews.size(),
                app->defaults.operator_id, app->defaults.marshal_root.string(),
                selected, detail_text, detail_scroll, &detail_total_lines,
                &detail_page_lines, &detail_scroll, &detail_viewport, status,
                status_is_error);
          } else {
            render_human_reviews_screen(
                pending_reviews, pending_requests.size(),
                app->defaults.operator_id, app->defaults.marshal_root.string(),
                selected, detail_text, detail_scroll, &detail_total_lines,
                &detail_page_lines, &detail_scroll, &detail_viewport, status,
                status_is_error);
          }
        }
        dirty = false;
      }

      if (inbox_refresh_pending) {
        if (inbox_refresh_needs_paint) {
          inbox_refresh_needs_paint = false;
          continue;
        }

        std::string refresh_error{};
        human_operator_inbox_t inbox{};
        if (!collect_human_operator_inbox(*app, &inbox, true, &refresh_error)) {
          status = refresh_error;
          status_is_error = true;
        } else {
          all_sessions = std::move(inbox.all_sessions);
          pending_requests = std::move(inbox.actionable_requests);
          pending_reviews = std::move(inbox.unacknowledged_summaries);
          sort_sessions_newest_first(&all_sessions);
          sort_sessions_newest_first(&pending_requests);
          sort_sessions_newest_first(&pending_reviews);
          if (!pending_requests.empty()) {
            selected_request =
                std::min(selected_request, pending_requests.size() - 1);
          } else {
            selected_request = 0;
          }
          if (!pending_reviews.empty()) {
            selected_review =
                std::min(selected_review, pending_reviews.size() - 1);
          } else {
            selected_review = 0;
          }
          filter_sessions_for_console(all_sessions, session_phase_filter,
                                      &session_rows);
          if (!session_rows.empty()) {
            selected_session = std::min(selected_session, session_rows.size() - 1);
          } else {
            selected_session = 0;
          }
          const bool has_sessions = !all_sessions.empty();
          const bool has_requests = !pending_requests.empty();
          const bool has_reviews = !pending_reviews.empty();
          status = has_loaded_once
                       ? (has_sessions
                              ? (has_requests
                                     ? (has_reviews
                                            ? "Refreshed sessions, requests, and reviews."
                                            : "Refreshed sessions and pending requests.")
                                     : (has_reviews
                                            ? "Refreshed sessions and reviews."
                                            : "Refreshed session inventory."))
                              : "No sessions or pending items.")
                       : (has_sessions
                              ? (has_requests
                                     ? (has_reviews
                                            ? "Ready with sessions, requests, and reviews."
                                            : "Ready with sessions and requests.")
                                     : (has_reviews
                                            ? "Ready with sessions and reviews."
                                            : "Ready with session inventory."))
                              : "No sessions or pending items. Waiting for Marshal Hero.");
          status_is_error = false;
        }
        has_loaded_once = true;
        inbox_refresh_pending = false;
        detail_dirty = true;
        detail_scroll = 0;
        detail_viewport = detail_viewport_t{};
        dirty = true;
        continue;
      }

      const int ch = getch();
      if (ch == 'q' || ch == 'Q' || ch == 27) return true;
      if (ch == KEY_RESIZE) {
        dirty = true;
        continue;
      }
      if (ch == KEY_MOUSE) {
        MEVENT me{};
        if (getmouse(&me) == OK && mouse_hits_detail_viewport(detail_viewport, me.x, me.y)) {
          const std::size_t max_scroll =
              detail_viewport.total_lines > detail_viewport.page_lines
                  ? detail_viewport.total_lines - detail_viewport.page_lines
                  : 0u;
          bool handled_mouse = false;
          if (me.bstate & BUTTON4_PRESSED) {
            detail_scroll = detail_scroll > 3 ? detail_scroll - 3 : 0u;
            handled_mouse = true;
          } else if (me.bstate & BUTTON5_PRESSED) {
            detail_scroll = std::min(detail_scroll + 3, max_scroll);
            handled_mouse = true;
          } else if ((me.bstate & BUTTON1_PRESSED) ||
                     (me.bstate & BUTTON1_CLICKED)) {
            if (detail_viewport.scrollbar_x > 0 &&
                me.x == detail_viewport.scrollbar_x && max_scroll > 0) {
              const int relative_y =
                  std::clamp(me.y - detail_viewport.y, 0, detail_viewport.height - 1);
              detail_scroll =
                  detail_viewport.height <= 1
                      ? max_scroll
                      : static_cast<std::size_t>(
                            (static_cast<unsigned long long>(relative_y) * max_scroll) /
                            static_cast<unsigned long long>(detail_viewport.height - 1));
              handled_mouse = true;
            }
          }
          if (handled_mouse) {
            dirty = true;
            continue;
          }
        }
        continue;
      }
      if (ch == 'r' || ch == 'R') {
        status = "Refreshing operator inbox...";
        status_is_error = false;
        inbox_refresh_pending = true;
        inbox_refresh_needs_paint = true;
        detail_dirty = true;
        detail_scroll = 0;
        detail_viewport = detail_viewport_t{};
        dirty = true;
        continue;
      }
      if (ch == '\t' || ch == KEY_BTAB) {
        std::string active_session_id{};
        if (current_view == operator_console_view_t::sessions) {
          filter_sessions_for_console(all_sessions, session_phase_filter,
                                      &session_rows);
          if (!session_rows.empty() && selected_session < session_rows.size()) {
            active_session_id = session_rows[selected_session].marshal_session_id;
          }
        } else if (current_view == operator_console_view_t::requests) {
          if (!pending_requests.empty() && selected_request < pending_requests.size()) {
            active_session_id = pending_requests[selected_request].marshal_session_id;
          }
        } else if (!pending_reviews.empty() &&
                   selected_review < pending_reviews.size()) {
          active_session_id = pending_reviews[selected_review].marshal_session_id;
        }
        cycle_operator_console_view(&current_view);
        if (!active_session_id.empty()) {
          filter_sessions_for_console(all_sessions, session_phase_filter,
                                      &session_rows);
          if (current_view == operator_console_view_t::sessions) {
            select_session_by_id(session_rows, active_session_id, &selected_session);
          } else if (current_view == operator_console_view_t::requests) {
            select_session_by_id(pending_requests, active_session_id,
                                 &selected_request);
          } else {
            select_session_by_id(pending_reviews, active_session_id,
                                 &selected_review);
          }
        }
        detail_dirty = true;
        detail_scroll = 0;
        detail_viewport = detail_viewport_t{};
        dirty = true;
        continue;
      }
      filter_sessions_for_console(all_sessions, session_phase_filter, &session_rows);
      auto& visible =
          current_view == operator_console_view_t::sessions
              ? session_rows
              : (current_view == operator_console_view_t::requests
                     ? pending_requests
                     : pending_reviews);
      auto& selected =
          current_view == operator_console_view_t::sessions
              ? selected_session
              : (current_view == operator_console_view_t::requests
                     ? selected_request
                     : selected_review);
      if (current_view == operator_console_view_t::sessions &&
          (ch == 'f' || ch == 'F')) {
        cycle_operator_console_phase_filter(&session_phase_filter);
        selected_session = 0;
        detail_dirty = true;
        detail_scroll = 0;
        detail_viewport = detail_viewport_t{};
        dirty = true;
        continue;
      }
      if (!visible.empty()) {
        selected = std::min(selected, visible.size() - 1);
      }
      if (visible.empty()) {
        dirty = true;
        continue;
      }
      if (ch == KEY_UP || ch == 'k') {
        if (selected > 0) --selected;
        detail_dirty = true;
        detail_scroll = 0;
        detail_viewport = detail_viewport_t{};
        dirty = true;
        continue;
      }
      if (ch == KEY_DOWN || ch == 'j') {
        if (selected + 1 < visible.size()) ++selected;
        detail_dirty = true;
        detail_scroll = 0;
        detail_viewport = detail_viewport_t{};
        dirty = true;
        continue;
      }
      if (ch == KEY_PPAGE) {
        if (detail_page_lines != 0) {
          detail_scroll = detail_scroll > detail_page_lines
                              ? detail_scroll - detail_page_lines
                              : 0u;
          dirty = true;
        }
        continue;
      }
      if (ch == KEY_NPAGE) {
        if (detail_page_lines != 0 && detail_total_lines > detail_page_lines) {
          const std::size_t max_scroll = detail_total_lines - detail_page_lines;
          detail_scroll = std::min(detail_scroll + detail_page_lines, max_scroll);
          dirty = true;
        }
        continue;
      }
      if (ch == KEY_HOME) {
        if (detail_scroll != 0) {
          detail_scroll = 0;
          dirty = true;
        }
        continue;
      }
      if (ch == KEY_END) {
        if (detail_page_lines != 0 && detail_total_lines > detail_page_lines) {
          detail_scroll = detail_total_lines - detail_page_lines;
          dirty = true;
        }
        continue;
      }
      if (current_view == operator_console_view_t::sessions) {
        const auto& session = visible[selected];
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
          if (find_session_index_by_id(pending_requests, session.marshal_session_id)
                  .has_value()) {
            current_view = operator_console_view_t::requests;
            select_session_by_id(pending_requests, session.marshal_session_id,
                                 &selected_request);
            detail_dirty = true;
            detail_scroll = 0;
            dirty = true;
            continue;
          }
          if (find_session_index_by_id(pending_reviews, session.marshal_session_id)
                  .has_value()) {
            current_view = operator_console_view_t::summaries;
            select_session_by_id(pending_reviews, session.marshal_session_id,
                                 &selected_review);
            detail_dirty = true;
            detail_scroll = 0;
            dirty = true;
            continue;
          }
          status =
              "This session has no pending request or review lane entry.";
          status_is_error = false;
          dirty = true;
          continue;
        }
        if ((ch == 'p' || ch == 'P') &&
            (session.phase == "active" || session.phase == "running_campaign")) {
          std::string structured{};
          std::string action_error{};
          if (!pause_marshal_session(app, session, false, &structured,
                                   &action_error)) {
            status = action_error;
            status_is_error = true;
            dirty = true;
            continue;
          }
          status = "Paused selected session. Refreshing operator console...";
          status_is_error = false;
          inbox_refresh_pending = true;
          inbox_refresh_needs_paint = true;
          detail_dirty = true;
          detail_scroll = 0;
          dirty = true;
          continue;
        }
        if ((ch == 'u' || ch == 'U') && session.phase == "paused" &&
            session.pause_kind == "operator") {
          std::string structured{};
          std::string action_error{};
          if (!resume_marshal_session(app, session, &structured, &action_error)) {
            status = action_error;
            status_is_error = true;
            dirty = true;
            continue;
          }
          status = "Resumed operator-paused session. Refreshing operator console...";
          status_is_error = false;
          inbox_refresh_pending = true;
          inbox_refresh_needs_paint = true;
          detail_dirty = true;
          detail_scroll = 0;
          dirty = true;
          continue;
        }
        if ((ch == 'c' || ch == 'C') && session.phase == "idle") {
          std::string instruction{};
          bool cancelled = false;
          if (!prompt_text_dialog(
                  " Continue session ",
                  "Provide the operator instruction that should launch more work for this idle session. This is the action that continues the session.",
                  &instruction, false, false, &cancelled)) {
            status = "failed to collect continuation instruction";
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
          std::string action_error{};
          if (!continue_marshal_session(app, session, instruction, &structured,
                                      &action_error)) {
            status = action_error;
            status_is_error = true;
            dirty = true;
            continue;
          }
          status =
              "Continued idle session from the session cockpit. Refreshing...";
          status_is_error = false;
          inbox_refresh_pending = true;
          inbox_refresh_needs_paint = true;
          detail_dirty = true;
          detail_scroll = 0;
          dirty = true;
          continue;
        }
        if ((ch == 't' || ch == 'T') && session.phase != "finished") {
          bool confirmed = false;
          bool cancelled = false;
          if (!prompt_yes_no_dialog(
                  " Terminate session ",
                  "Terminate the selected session? This action is final.",
                  false, &confirmed, &cancelled)) {
            status = "failed to collect session termination confirmation";
            status_is_error = true;
            dirty = true;
            continue;
          }
          if (cancelled || !confirmed) {
            status = "Cancelled.";
            status_is_error = false;
            dirty = true;
            continue;
          }
          std::string structured{};
          std::string action_error{};
          if (!terminate_marshal_session(app, session, false, &structured,
                                       &action_error)) {
            status = action_error;
            status_is_error = true;
            dirty = true;
            continue;
          }
          status =
              "Terminated selected session from the session cockpit. Refreshing...";
          status_is_error = false;
          inbox_refresh_pending = true;
          inbox_refresh_needs_paint = true;
          detail_dirty = true;
          detail_scroll = 0;
          dirty = true;
          continue;
        }
        continue;
      }
      if (current_view == operator_console_view_t::requests &&
          (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == 'c' ||
           ch == 'C' || ch == 's' || ch == 'S')) {
        if (is_clarification_pause_kind(visible[selected].pause_kind)) {
          std::string answer{};
          bool cancelled = false;
          if (!prompt_text_dialog(
                  " Answer request ",
                  "Provide the clarification answer that Marshal Hero should continue with.",
                  &answer, false, false, &cancelled)) {
            status = "failed to collect request answer";
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
          if (!build_request_answer_and_resume(app, visible[selected], answer,
                                               &structured, &response_error)) {
            status = response_error;
            status_is_error = true;
            dirty = true;
            continue;
          }
          status =
              "Applied clarification answer. Refreshing operator inbox...";
          status_is_error = false;
          inbox_refresh_pending = true;
          inbox_refresh_needs_paint = true;
          detail_dirty = true;
          detail_scroll = 0;
          dirty = true;
          continue;
        }
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
        status =
            "Applied signed governance resolution. Refreshing operator inbox...";
        status_is_error = false;
        inbox_refresh_pending = true;
        inbox_refresh_needs_paint = true;
        detail_dirty = true;
        detail_scroll = 0;
        dirty = true;
        continue;
      }
      if (current_view == operator_console_view_t::summaries &&
          (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == 'c' ||
           ch == 'C' || ch == 'a' || ch == 'A')) {
        const bool can_continue = (visible[selected].phase == "idle");
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
          status = can_continue
                       ? "Idle review report. Use 'c' to launch more work, or 'a' to acknowledge and clear the report."
                       : "Finished review report. Use 'a' to acknowledge and clear the report after review.";
          status_is_error = false;
          dirty = true;
          continue;
        }
        if ((ch == 'c' || ch == 'C') && can_continue) {
          std::string instruction{};
          bool cancelled = false;
          if (!prompt_text_dialog(
                  " Continue session ",
                  "Provide the operator instruction that should launch more work for this idle session. This is the action that continues the session.",
                  &instruction, false, false, &cancelled)) {
            status = "failed to collect continuation instruction";
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
          std::string continue_error{};
          if (!continue_marshal_session(app, visible[selected], instruction,
                                      &structured, &continue_error)) {
            status = continue_error;
            status_is_error = true;
            dirty = true;
            continue;
          }
          status = "Continued idle session. Refreshing operator inbox...";
          status_is_error = false;
          inbox_refresh_pending = true;
          inbox_refresh_needs_paint = true;
          detail_dirty = true;
          detail_scroll = 0;
          dirty = true;
          continue;
        }
        std::string note{};
        bool cancelled = false;
        if (!prompt_text_dialog(
                " Acknowledge review ",
                "This does not continue the session. It records a signed acknowledgment and clears the review report from the queue after review. Provide a required acknowledgment message:",
                &note, false, false, &cancelled)) {
          status = "failed to collect review acknowledgment note";
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
        if (!acknowledge_session_summary(app, visible[selected], note,
                                         &structured, &ack_error)) {
          status = ack_error;
          status_is_error = true;
          dirty = true;
          continue;
        }
        status = "Acknowledged review report. Refreshing operator inbox...";
        status_is_error = false;
        inbox_refresh_pending = true;
        inbox_refresh_needs_paint = true;
        detail_dirty = true;
        detail_scroll = 0;
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
    const cuwacunu::hero::marshal::marshal_session_record_t& session) {
  std::ostringstream out;
  out << "{"
      << "\"marshal_session_id\":" << json_quote(session.marshal_session_id) << ","
      << "\"objective_name\":" << json_quote(session.objective_name) << ","
      << "\"operator_state\":"
      << json_quote(
             std::string(operator_session_state_label(operator_session_state(session))))
      << ","
      << "\"operator_state_detail\":"
      << json_quote(operator_session_state_detail(session)) << ","
      << "\"operator_action_hint\":"
      << json_quote(operator_session_action_hint(session)) << ","
      << "\"phase\":" << json_quote(session.phase) << ","
      << "\"phase_detail\":" << json_quote(session.phase_detail) << ","
      << "\"pause_kind\":" << json_quote(session.pause_kind) << ","
      << "\"request_kind\":"
      << json_quote(std::string(human_request_kind_label(session.pause_kind)))
      << ","
      << "\"checkpoint_count\":" << session.checkpoint_count << ","
      << "\"started_at_ms\":" << session.started_at_ms << ","
      << "\"updated_at_ms\":" << session.updated_at_ms << ","
      << "\"human_request_path\":"
      << json_quote(session.human_request_path) << ","
      << "\"governance_resolution_path\":"
      << json_quote(
             cuwacunu::hero::marshal::marshal_session_human_governance_resolution_latest_path(
                 std::filesystem::path(session.session_root).parent_path(),
                 session.marshal_session_id)
                 .string())
      << ",\"governance_resolution_sig_path\":"
      << json_quote(
             cuwacunu::hero::marshal::marshal_session_human_governance_resolution_latest_sig_path(
                 std::filesystem::path(session.session_root).parent_path(),
                 session.marshal_session_id)
                 .string())
      << ",\"clarification_answer_path\":"
      << json_quote(
             cuwacunu::hero::marshal::
                 marshal_session_human_clarification_answer_latest_path(
                 std::filesystem::path(session.session_root).parent_path(),
                 session.marshal_session_id)
                 .string())
      << ",\"request_excerpt\":" << json_quote(request_excerpt(session))
      << "}";
  return out.str();
}

[[nodiscard]] std::string pending_summary_row_to_json(
    const cuwacunu::hero::marshal::marshal_session_record_t& session) {
  const std::optional<std::uint64_t> elapsed_ms = marshal_session_elapsed_ms(session);
  std::ostringstream out;
  out << "{"
      << "\"marshal_session_id\":" << json_quote(session.marshal_session_id) << ","
      << "\"objective_name\":" << json_quote(session.objective_name) << ","
      << "\"operator_state\":"
      << json_quote(
             std::string(operator_session_state_label(operator_session_state(session))))
      << ","
      << "\"operator_state_detail\":"
      << json_quote(operator_session_state_detail(session)) << ","
      << "\"operator_action_hint\":"
      << json_quote(operator_session_action_hint(session)) << ","
      << "\"phase\":" << json_quote(session.phase) << ","
      << "\"phase_detail\":" << json_quote(session.phase_detail) << ","
      << "\"finish_reason\":" << json_quote(session.finish_reason) << ","
      << "\"checkpoint_count\":" << session.checkpoint_count << ","
      << "\"launch_count\":" << session.launch_count << ","
      << "\"started_at_ms\":" << session.started_at_ms << ","
      << "\"updated_at_ms\":" << session.updated_at_ms << ","
      << "\"finished_at_ms\":"
      << (session.finished_at_ms.has_value()
              ? std::to_string(*session.finished_at_ms)
                                          : "null")
      << ","
      << "\"duration_ms\":"
      << (elapsed_ms.has_value() ? std::to_string(*elapsed_ms) : "null") << ","
      << "\"max_campaign_launches\":" << session.max_campaign_launches << ","
      << "\"remaining_campaign_launches\":"
      << session.remaining_campaign_launches << ","
      << "\"effort_summary_text\":"
      << json_quote(build_summary_effort_text(session)) << ","
      << "\"summary_path\":"
      << json_quote(summary_path_for_session(session).string())
      << ","
      << "\"summary_ack_path\":"
      << json_quote(summary_ack_path_for_session(session).string()) << ","
      << "\"summary_ack_sig_path\":"
      << json_quote(summary_ack_sig_path_for_session(session).string()) << ","
      << "\"summary_excerpt\":" << json_quote(summary_excerpt(session)) << "}";
  return out.str();
}

[[nodiscard]] bool collect_all_sessions(
    const app_context_t& app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t>* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "sessions output pointer is null";
    return false;
  }
  out->clear();
  return cuwacunu::hero::marshal::scan_marshal_session_records(app.defaults.marshal_root,
                                                           out, error);
}

[[nodiscard]] bool collect_human_operator_inbox(
    const app_context_t& app, human_operator_inbox_t* out, bool sync_markers,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "operator inbox output pointer is null";
    return false;
  }
  out->actionable_requests.clear();
  out->unacknowledged_summaries.clear();
  out->all_sessions.clear();

  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> sessions{};
  if (!cuwacunu::hero::marshal::scan_marshal_session_records(app.defaults.marshal_root,
                                                         &sessions, error)) {
    return false;
  }
  out->all_sessions = sessions;
  for (const auto& session : sessions) {
    if (session.phase == "paused" &&
        is_human_request_pause_kind(session.pause_kind)) {
      out->actionable_requests.push_back(session);
      continue;
    }
    if (!cuwacunu::hero::marshal::is_marshal_session_summary_state(session.phase)) {
      continue;
    }
    const std::filesystem::path summary_path = summary_path_for_session(session);
    if (!std::filesystem::exists(summary_path) ||
        !std::filesystem::is_regular_file(summary_path)) {
      continue;
    }
    std::string ack_error{};
    if (!summary_ack_matches_current_summary(session, &ack_error)) {
      out->unacknowledged_summaries.push_back(session);
    }
  }

  if (!sync_markers) return true;
  return write_human_pending_marker_counts(
      app.defaults.marshal_root, out->actionable_requests.size(),
      out->unacknowledged_summaries.size(), error);
}

[[nodiscard]] bool collect_pending_requests(
    const app_context_t& app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t>* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "pending requests output pointer is null";
    return false;
  }
  human_operator_inbox_t inbox{};
  if (!collect_human_operator_inbox(app, &inbox, true, error)) return false;
  *out = std::move(inbox.actionable_requests);
  return true;
}

[[nodiscard]] bool collect_pending_reviews(
    const app_context_t& app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t>* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "pending reviews output pointer is null";
    return false;
  }
  human_operator_inbox_t inbox{};
  if (!collect_human_operator_inbox(app, &inbox, true, error)) return false;
  *out = std::move(inbox.unacknowledged_summaries);
  return true;
}

[[nodiscard]] bool build_request_answer_and_resume(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string answer,
    std::string* out_structured, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !out_structured || !out_error) return false;
  answer = trim_ascii(answer);
  if (answer.empty()) {
    *out_error = "human clarification answer requires a non-empty answer";
    return false;
  }

  const auto answer_path =
      cuwacunu::hero::marshal::marshal_session_human_clarification_answer_path(
      app->defaults.marshal_root, session.marshal_session_id, session.checkpoint_count);
  const auto latest_answer_path =
      cuwacunu::hero::marshal::
          marshal_session_human_clarification_answer_latest_path(
          app->defaults.marshal_root, session.marshal_session_id);
  const std::string answer_json =
      std::string("{\"schema\":") +
      json_quote(kHumanClarificationAnswerSchemaV3) +
      ",\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"checkpoint_index\":" + std::to_string(session.checkpoint_count) +
      ",\"answer\":" + json_quote(answer) + "}";
  if (!cuwacunu::hero::runtime::write_text_file_atomic(answer_path, answer_json,
                                                       out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(latest_answer_path,
                                                       answer_json,
                                                       out_error)) {
    return false;
  }

  std::string marshal_structured{};
  const std::string resume_args =
      "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"clarification_answer_path\":" + json_quote(answer_path.string()) +
      "}";
  if (!call_marshal_tool(*app, "hero.marshal.resume_session", resume_args,
                       &marshal_structured, out_error)) {
    return false;
  }
  sync_human_pending_markers_best_effort(*app);

  *out_structured = "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
                    ",\"clarification_answer_path\":" +
                    json_quote(answer_path.string()) + ",\"marshal\":" +
                    marshal_structured + "}";
  return true;
}

[[nodiscard]] bool build_resolution_and_apply(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string resolution_kind, std::string reason,
    std::string* out_structured, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !out_structured || !out_error) return false;

  resolution_kind = trim_ascii(resolution_kind);
  reason = trim_ascii(reason);
  if (resolution_kind != "grant" && resolution_kind != "deny" &&
      resolution_kind != "clarify" && resolution_kind != "terminate") {
    *out_error =
        "human resolution_kind must be grant, deny, clarify, or terminate";
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

  std::string request_sha256_hex{};
  if (!cuwacunu::hero::human::sha256_hex_file(session.human_request_path,
                                              &request_sha256_hex,
                                              out_error)) {
    return false;
  }

  const std::filesystem::path latest_intent_path =
      cuwacunu::hero::marshal::marshal_session_latest_intent_checkpoint_path(
          app->defaults.marshal_root, session.marshal_session_id);
  std::string latest_intent_json{};
  if (!cuwacunu::hero::runtime::read_text_file(latest_intent_path,
                                               &latest_intent_json,
                                               out_error)) {
    return false;
  }
  std::string intent{};
  (void)extract_json_string_field(latest_intent_json, "intent", &intent);
  intent = trim_ascii(intent);
  if (intent != "request_governance") {
    *out_error = "latest pending intent is not a governance request";
    return false;
  }
  std::string governance_json{};
  if (!extract_json_object_field(latest_intent_json, "governance",
                                 &governance_json)) {
    *out_error = "latest intent is missing governance object";
    return false;
  }
  std::string governance_kind{};
  (void)extract_json_string_field(governance_json, "kind", &governance_kind);
  governance_kind = trim_ascii(governance_kind);
  if (governance_kind.empty()) {
    *out_error = "latest intent governance is missing kind";
    return false;
  }

  std::string delta_json{};
  (void)extract_json_object_field(governance_json, "delta", &delta_json);
  human_resolution_record_t resolution{};
  resolution.marshal_session_id = session.marshal_session_id;
  resolution.checkpoint_index = session.checkpoint_count;
  resolution.request_sha256_hex = request_sha256_hex;
  resolution.operator_id = app->defaults.operator_id;
  resolution.resolved_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  resolution.resolution_kind = resolution_kind;
  resolution.governance_kind = governance_kind;
  resolution.reason = reason;
  resolution.signer_public_key_fingerprint_sha256_hex =
      std::string(64, '0');
  if (resolution_kind == "grant") {
    (void)extract_json_bool_field(delta_json, "allow_default_write",
                                  &resolution.grant_allow_default_write);
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
      cuwacunu::hero::marshal::marshal_session_human_governance_resolutions_dir(
      app->defaults.marshal_root, session.marshal_session_id);
  std::filesystem::create_directories(response_dir, ec);
  if (ec) {
    *out_error =
        "cannot create human resolution directory: " + response_dir.string();
    return false;
  }
  const auto response_path =
      cuwacunu::hero::marshal::marshal_session_human_governance_resolution_path(
          app->defaults.marshal_root, session.marshal_session_id, session.checkpoint_count);
  const auto response_sig_path =
      cuwacunu::hero::marshal::marshal_session_human_governance_resolution_sig_path(
          app->defaults.marshal_root, session.marshal_session_id, session.checkpoint_count);
  const auto latest_path =
      cuwacunu::hero::marshal::marshal_session_human_governance_resolution_latest_path(
          app->defaults.marshal_root, session.marshal_session_id);
  const auto latest_sig_path =
      cuwacunu::hero::marshal::marshal_session_human_governance_resolution_latest_sig_path(
          app->defaults.marshal_root, session.marshal_session_id);

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

  std::string marshal_structured{};
  std::string resume_args =
      "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"governance_resolution_path\":" + json_quote(response_path.string()) +
      ",\"governance_resolution_sig_path\":" +
      json_quote(response_sig_path.string()) + "}";
  if (!call_marshal_tool(*app, "hero.marshal.resume_session", resume_args,
                       &marshal_structured, out_error)) {
    return false;
  }
  sync_human_pending_markers_best_effort(*app);

  *out_structured = "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
                    ",\"governance_resolution_path\":" +
                    json_quote(response_path.string()) +
                    ",\"governance_resolution_sig_path\":" +
                    json_quote(response_sig_path.string()) +
                    ",\"fingerprint_sha256_hex\":" + json_quote(fingerprint_hex) +
                    ",\"marshal\":" + marshal_structured + "}";
  return true;
}

[[nodiscard]] bool finalize_session_summary(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string note,
    std::string disposition, std::string* out_structured,
    std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !out_structured || !out_error) return false;
  note = trim_ascii(note);
  disposition = lowercase_copy(trim_ascii(disposition));
  if (disposition != "acknowledged" && disposition != "dismissed") {
    *out_error = "invalid summary disposition";
    return false;
  }
  if (note.empty()) {
    *out_error = "review acknowledgment requires a non-empty note";
    return false;
  }
  if (disposition == "dismissed") disposition = "acknowledged";

  if (!cuwacunu::hero::marshal::is_marshal_session_summary_state(session.phase)) {
    *out_error = "session does not currently expose an informational summary";
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

  const std::filesystem::path summary_path = summary_path_for_session(session);
  std::string summary_sha256_hex{};
  if (!cuwacunu::hero::human::sha256_hex_file(summary_path, &summary_sha256_hex,
                                              out_error)) {
    return false;
  }

  const std::uint64_t acknowledged_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  std::string ack_json =
      std::string("{\"schema\":") + json_quote(kHumanSummaryAckSchemaV3) +
      ",\"marshal_session_id\":" + json_quote(session.marshal_session_id) + ",\"phase\":" +
      json_quote(session.phase) + ",\"finish_reason\":" +
      json_quote(session.finish_reason) + ",\"summary_sha256_hex\":" +
      json_quote(summary_sha256_hex) + ",\"operator_id\":" +
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
    *out_error = "summary ack signing returned invalid fingerprint length";
    return false;
  }
  ack_json =
      std::string("{\"schema\":") + json_quote(kHumanSummaryAckSchemaV3) +
      ",\"marshal_session_id\":" + json_quote(session.marshal_session_id) + ",\"phase\":" +
      json_quote(session.phase) + ",\"finish_reason\":" +
      json_quote(session.finish_reason) + ",\"summary_sha256_hex\":" +
      json_quote(summary_sha256_hex) + ",\"operator_id\":" +
      json_quote(app->defaults.operator_id) + ",\"acknowledged_at_ms\":" +
      std::to_string(acknowledged_at_ms) + ",\"disposition\":" +
      json_quote(disposition) + ",\"note\":" + json_quote(note) +
      ",\"signer_public_key_fingerprint_sha256_hex\":" +
      json_quote(fingerprint_hex) + "}";

  const std::filesystem::path ack_path = summary_ack_path_for_session(session);
  const std::filesystem::path ack_sig_path =
      summary_ack_sig_path_for_session(session);
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
      "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) + ",\"summary_path\":" +
      json_quote(summary_path.string()) + ",\"summary_ack_path\":" +
      json_quote(ack_path.string()) + ",\"summary_ack_sig_path\":" +
      json_quote(ack_sig_path.string()) + ",\"disposition\":" +
      json_quote(disposition) + ",\"note\":" + json_quote(note) +
      ",\"fingerprint_sha256_hex\":" + json_quote(fingerprint_hex) + "}";
  return true;
}

[[nodiscard]] bool acknowledge_session_summary(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string note,
    std::string* out_structured, std::string* out_error) {
  return finalize_session_summary(app, session, std::move(note),
                                  "acknowledged", out_structured, out_error);
}

[[nodiscard]] bool pause_marshal_session(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session, bool force,
    std::string* out_structured, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !out_structured || !out_error) return false;

  std::string marshal_structured{};
  const std::string args =
      std::string("{\"marshal_session_id\":") + json_quote(session.marshal_session_id) +
      ",\"force\":" + bool_json(force) + "}";
  if (!call_marshal_tool(*app, "hero.marshal.pause_session", args, &marshal_structured,
                       out_error)) {
    return false;
  }
  sync_human_pending_markers_best_effort(*app);
  *out_structured = "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
                    ",\"force\":" + bool_json(force) + ",\"marshal\":" +
                    marshal_structured + "}";
  return true;
}

[[nodiscard]] bool resume_marshal_session(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string* out_structured, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !out_structured || !out_error) return false;

  std::string marshal_structured{};
  const std::string args =
      std::string("{\"marshal_session_id\":") + json_quote(session.marshal_session_id) + "}";
  if (!call_marshal_tool(*app, "hero.marshal.resume_session", args, &marshal_structured,
                       out_error)) {
    return false;
  }
  sync_human_pending_markers_best_effort(*app);
  *out_structured = "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
                    ",\"marshal\":" + marshal_structured + "}";
  return true;
}

[[nodiscard]] bool continue_marshal_session(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session,
    std::string instruction,
    std::string* out_structured, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !out_structured || !out_error) return false;
  instruction = trim_ascii(instruction);
  if (instruction.empty()) {
    *out_error = "continue_session requires a non-empty instruction";
    return false;
  }

  std::string marshal_structured{};
  const std::string args =
      std::string("{\"marshal_session_id\":") + json_quote(session.marshal_session_id) +
      ",\"instruction\":" + json_quote(instruction) + "}";
  if (!call_marshal_tool(*app, "hero.marshal.continue_session", args,
                       &marshal_structured, out_error)) {
    return false;
  }
  sync_human_pending_markers_best_effort(*app);
  *out_structured = "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
                    ",\"instruction\":" + json_quote(instruction) +
                    ",\"marshal\":" + marshal_structured + "}";
  return true;
}

[[nodiscard]] bool terminate_marshal_session(
    app_context_t* app,
    const cuwacunu::hero::marshal::marshal_session_record_t& session, bool force,
    std::string* out_structured, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app || !out_structured || !out_error) return false;

  std::string marshal_structured{};
  const std::string args =
      std::string("{\"marshal_session_id\":") + json_quote(session.marshal_session_id) +
      ",\"force\":" + bool_json(force) + "}";
  if (!call_marshal_tool(*app, "hero.marshal.terminate_session", args,
                       &marshal_structured, out_error)) {
    return false;
  }
  sync_human_pending_markers_best_effort(*app);
  *out_structured = "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
                    ",\"force\":" + bool_json(force) + ",\"marshal\":" +
                    marshal_structured + "}";
  return true;
}

[[nodiscard]] bool handle_tool_list_requests(app_context_t* app,
                                             const std::string& arguments_json,
                                             std::string* out_structured,
                                             std::string* out_error);
[[nodiscard]] bool handle_tool_list_summaries(app_context_t* app,
                                              const std::string& arguments_json,
                                              std::string* out_structured,
                                              std::string* out_error);
[[nodiscard]] bool handle_tool_get_request(app_context_t* app,
                                           const std::string& arguments_json,
                                           std::string* out_structured,
                                           std::string* out_error);
[[nodiscard]] bool handle_tool_get_summary(app_context_t* app,
                                           const std::string& arguments_json,
                                           std::string* out_structured,
                                           std::string* out_error);
[[nodiscard]] bool handle_tool_answer_request(app_context_t* app,
                                              const std::string& arguments_json,
                                              std::string* out_structured,
                                              std::string* out_error);
[[nodiscard]] bool handle_tool_resolve_governance(app_context_t* app,
                                                  const std::string& arguments_json,
                                                  std::string* out_structured,
                                                  std::string* out_error);
[[nodiscard]] bool handle_tool_ack_summary(app_context_t* app,
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

[[nodiscard]] bool handle_tool_list_requests(app_context_t* app,
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

  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> sessions{};
  if (!collect_pending_requests(*app, &sessions, out_error)) return false;
  std::sort(sessions.begin(), sessions.end(), [newest_first](const auto& a,
                                                             const auto& b) {
    if (a.updated_at_ms != b.updated_at_ms) {
      return newest_first ? (a.updated_at_ms > b.updated_at_ms)
                          : (a.updated_at_ms < b.updated_at_ms);
    }
    return newest_first ? (a.marshal_session_id > b.marshal_session_id) : (a.marshal_session_id < b.marshal_session_id);
  });
  const std::size_t total = sessions.size();
  const std::size_t off = std::min(offset, sessions.size());
  std::size_t count = limit;
  if (count == 0) count = sessions.size() - off;
  count = std::min(count, sessions.size() - off);

  std::ostringstream rows;
  rows << "[";
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0) rows << ",";
    rows << pending_request_row_to_json(sessions[off + i]);
  }
  rows << "]";

  *out_structured =
      "{\"marshal_session_id\":\"\",\"count\":" + std::to_string(count) +
      ",\"total\":" + std::to_string(total) + ",\"requests\":" +
      rows.str() +
      "}";
  return true;
}

[[nodiscard]] bool handle_tool_get_request(app_context_t* app,
                                           const std::string& arguments_json,
                                           std::string* out_structured,
                                           std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string marshal_session_id{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id", &marshal_session_id);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "get_request requires arguments.marshal_session_id";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  if (!cuwacunu::hero::marshal::read_marshal_session_record(app->defaults.marshal_root, marshal_session_id,
                                                        &session, out_error)) {
    return false;
  }
  if (session.phase != "paused" ||
      !is_human_request_pause_kind(session.pause_kind)) {
    *out_error = "session is not currently paused for a human request";
    return false;
  }

  std::string request_text{};
  if (!cuwacunu::hero::runtime::read_text_file(session.human_request_path,
                                               &request_text, out_error)) {
    return false;
  }
  *out_structured = "{\"marshal_session_id\":" + json_quote(marshal_session_id) + ",\"request\":" +
                    pending_request_row_to_json(session) + ",\"request_text\":" +
                    json_quote(request_text) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_list_summaries(app_context_t* app,
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

  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> sessions{};
  if (!collect_pending_reviews(*app, &sessions, out_error)) return false;
  std::sort(sessions.begin(), sessions.end(), [newest_first](const auto& a,
                                                             const auto& b) {
    if (a.updated_at_ms != b.updated_at_ms) {
      return newest_first ? (a.updated_at_ms > b.updated_at_ms)
                          : (a.updated_at_ms < b.updated_at_ms);
    }
    return newest_first ? (a.marshal_session_id > b.marshal_session_id) : (a.marshal_session_id < b.marshal_session_id);
  });
  const std::size_t total = sessions.size();
  const std::size_t off = std::min(offset, sessions.size());
  std::size_t count = limit;
  if (count == 0) count = sessions.size() - off;
  count = std::min(count, sessions.size() - off);

  std::ostringstream rows;
  rows << "[";
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0) rows << ",";
    rows << pending_summary_row_to_json(sessions[off + i]);
  }
  rows << "]";

  *out_structured =
      "{\"marshal_session_id\":\"\",\"count\":" + std::to_string(count) +
      ",\"total\":" + std::to_string(total) + ",\"summaries\":" + rows.str() +
      "}";
  return true;
}

[[nodiscard]] bool handle_tool_get_summary(app_context_t* app,
                                           const std::string& arguments_json,
                                           std::string* out_structured,
                                           std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string marshal_session_id{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id", &marshal_session_id);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "get_summary requires arguments.marshal_session_id";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  if (!cuwacunu::hero::marshal::read_marshal_session_record(app->defaults.marshal_root,
                                                        marshal_session_id, &session,
                                                        out_error)) {
    return false;
  }
  if (!cuwacunu::hero::marshal::is_marshal_session_summary_state(
          session.phase)) {
    *out_error = "session does not currently expose an informational summary";
    return false;
  }

  std::string summary_text{};
  if (!read_summary_text_for_session(session, &summary_text, out_error)) {
    return false;
  }
  *out_structured = "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
                    ",\"summary\":" + pending_summary_row_to_json(session) +
                    ",\"summary_text\":" +
                    json_quote(summary_text) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_answer_request(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string marshal_session_id{};
  std::string answer{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id", &marshal_session_id);
  (void)extract_json_string_field(arguments_json, "answer", &answer);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "answer_request requires arguments.marshal_session_id";
    return false;
  }
  if (trim_ascii(answer).empty()) {
    *out_error = "answer_request requires arguments.answer";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  if (!cuwacunu::hero::marshal::read_marshal_session_record(app->defaults.marshal_root,
                                                        marshal_session_id, &session,
                                                        out_error)) {
    return false;
  }
  if (session.phase != "paused" ||
      !is_clarification_pause_kind(session.pause_kind)) {
    *out_error = "session is not currently paused for clarification";
    return false;
  }
  return build_request_answer_and_resume(app, session, answer, out_structured,
                                         out_error);
}

[[nodiscard]] bool handle_tool_resolve_governance(
    app_context_t* app, const std::string& arguments_json,
    std::string* out_structured, std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string marshal_session_id{};
  std::string resolution_kind{};
  std::string reason{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id", &marshal_session_id);
  (void)extract_json_string_field(arguments_json, "resolution_kind",
                                  &resolution_kind);
  (void)extract_json_string_field(arguments_json, "reason", &reason);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "resolve_governance requires arguments.marshal_session_id";
    return false;
  }
  if (trim_ascii(resolution_kind).empty()) {
    *out_error = "resolve_governance requires arguments.resolution_kind";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  if (!cuwacunu::hero::marshal::read_marshal_session_record(app->defaults.marshal_root, marshal_session_id,
                                                        &session, out_error)) {
    return false;
  }
  if (session.phase != "paused" || session.pause_kind != "governance") {
    *out_error = "session is not currently paused for governance";
    return false;
  }
  return build_resolution_and_apply(app, session, resolution_kind, reason,
                                    out_structured, out_error);
}

[[nodiscard]] bool handle_tool_ack_summary(app_context_t* app,
                                           const std::string& arguments_json,
                                           std::string* out_structured,
                                           std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string marshal_session_id{};
  std::string note{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id", &marshal_session_id);
  (void)extract_json_string_field(arguments_json, "note", &note);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "ack_summary requires arguments.marshal_session_id";
    return false;
  }
  if (trim_ascii(note).empty()) {
    *out_error = "ack_summary requires arguments.note";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  if (!cuwacunu::hero::marshal::read_marshal_session_record(app->defaults.marshal_root,
                                                        marshal_session_id, &session,
                                                        out_error)) {
    return false;
  }
  return acknowledge_session_summary(app, session, note, out_structured,
                                     out_error);
}

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

  out->marshal_root = cuwacunu::hero::marshal::marshal_root(
      resolve_runtime_root_from_global_config(global_config_path));
  const auto resolve_exec = [&](const char* key, std::filesystem::path* dst) {
    const auto it = values.find(key);
    if (it == values.end()) return false;
    *dst = resolve_command_path(hero_dsl_path.parent_path(), it->second);
    return !dst->empty();
  };
  if (!resolve_exec("marshal_hero_binary", &out->marshal_hero_binary)) {
    if (error) *error = "missing/invalid marshal_hero_binary in " + hero_dsl_path.string();
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
  if (out->marshal_root.empty()) {
    if (error) {
      *error = "cannot derive .marshal_hero root from GENERAL.runtime_root in " +
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
  return name == "hero.human.list_requests" ||
         name == "hero.human.list_summaries" ||
         name == "hero.human.get_request" ||
         name == "hero.human.get_summary";
}

[[nodiscard]] bool human_tool_is_destructive(std::string_view name) {
  (void)name;
  return false;
}

std::string build_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  bool first = true;
  for (const auto& tool : kHumanTools) {
    if (!first) out << ",";
    first = false;
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

[[nodiscard]] bool run_line_prompt_operator_console(app_context_t* app,
                                                    std::string* error) {
  if (error) error->clear();
  if (!app) {
    if (error) *error = "human app pointer is null";
    return false;
  }

  human_operator_inbox_t inbox{};
  if (!collect_human_operator_inbox(*app, &inbox, true, error)) return false;
  auto pending = std::move(inbox.actionable_requests);
  auto summaries = std::move(inbox.unacknowledged_summaries);
  sort_sessions_newest_first(&pending);
  sort_sessions_newest_first(&summaries);
  std::cout << "== Human Hero ==\n"
            << "operator: "
            << (app->defaults.operator_id.empty() ? "<unset>"
                                                  : app->defaults.operator_id)
            << "\n"
            << "marshal_root: " << app->defaults.marshal_root.string() << "\n"
            << "pending_requests: " << pending.size() << "\n"
            << "pending_reviews: " << summaries.size() << "\n";
  if (pending.empty() && summaries.empty()) {
    std::cout << "status: no pending human requests or review reports\n"
              << "hint: rerun this command after a Marshal Hero session pauses, "
                 "idles, finishes, or use hero.marshal.list_sessions / "
                 "hero.human.list_requests / hero.human.list_summaries.\n";
    return true;
  }
  if (!pending.empty()) {
    std::cout << "status: pending human request requires attention\n";

    std::size_t selected = 0;
    if (pending.size() > 1) {
      std::cout << "Pending requests:\n";
      for (std::size_t i = 0; i < pending.size(); ++i) {
        std::cout << "  [" << i + 1 << "] " << pending[i].marshal_session_id << "  "
                  << pending[i].objective_name << "  "
                  << operator_session_state_detail(pending[i]);
        if (!pending[i].phase_detail.empty()) {
          std::cout << "  " << pending[i].phase_detail;
        }
        std::cout << "\n";
      }
      std::cout << "Select request number (blank to cancel): " << std::flush;
      std::string line{};
      if (!std::getline(std::cin, line)) return false;
      line = trim_ascii(line);
      if (line.empty()) return true;
      std::size_t choice = 0;
      if (!parse_size_token(line, &choice) || choice == 0 ||
          choice > pending.size()) {
        if (error) *error = "invalid request selection";
        return false;
      }
      selected = choice - 1;
    }
    const auto& session = pending[selected];
    std::string request_text{};
    if (!cuwacunu::hero::runtime::read_text_file(session.human_request_path,
                                                 &request_text, error)) {
      return false;
    }
    std::cout << "\n=== Human Request ===\n" << request_text << "\n";

    if (is_clarification_pause_kind(session.pause_kind)) {
      std::cout << "Provide clarification answer (blank to cancel): "
                << std::flush;
      std::string answer{};
      if (!std::getline(std::cin, answer)) return false;
      answer = trim_ascii(answer);
      if (answer.empty()) return true;
      std::string structured{};
      if (!build_request_answer_and_resume(app, session, answer, &structured,
                                           error)) {
        return false;
      }
      std::cout << "\nClarification answer applied.\n" << structured << "\n";
      return true;
    }

    std::cout
        << "Resolve with [g]rant, [c]larify, [d]eny, [t]erminate session, or blank to cancel: "
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
    } else if (resolution == "t" || resolution == "terminate") {
      resolution_kind = "terminate";
    } else {
      if (error) *error = "invalid resolution selection";
      return false;
    }

    std::cout << "Reason shown to Marshal Hero: " << std::flush;
    std::string reason{};
    if (!std::getline(std::cin, reason)) return false;
    reason = trim_ascii(reason);
    if (reason.empty()) {
      if (error) *error = "reason is required";
      return false;
    }

    std::string structured{};
    if (!build_resolution_and_apply(app, session, resolution_kind, reason,
                                    &structured, error)) {
      return false;
    }
    std::cout << "\nSigned governance resolution applied.\n" << structured
              << "\n";
    return true;
  }

  std::cout
      << "status: review report available; continue launches more work, acknowledge records a required review message and clears the report\n";
  std::size_t selected = 0;
  if (summaries.size() > 1) {
    std::cout << "Pending reviews:\n";
    for (std::size_t i = 0; i < summaries.size(); ++i) {
      std::cout << "  [" << i + 1 << "] " << summaries[i].marshal_session_id << "  "
                << summaries[i].objective_name << "  "
                << operator_session_state_detail(summaries[i]) << "  "
                << build_summary_effort_text(summaries[i]) << "\n";
    }
    std::cout << "Select review number (blank to cancel): " << std::flush;
    std::string line{};
    if (!std::getline(std::cin, line)) return false;
    line = trim_ascii(line);
    if (line.empty()) return true;
    std::size_t choice = 0;
    if (!parse_size_token(line, &choice) || choice == 0 ||
        choice > summaries.size()) {
      if (error) *error = "invalid review selection";
      return false;
    }
    selected = choice - 1;
  }
  const auto& session = summaries[selected];
  std::string summary_text{};
  if (!read_summary_text_for_session(session, &summary_text, error))
    return false;
  std::cout << "\n=== Human Review Report ===\n"
            << "[continue launches more work; acknowledge records a required review message and clears this report from the operator queue]\n\n"
            << summary_text << "\n";

  const bool can_continue = (session.phase == "idle");
  std::cout << "Resolve this review? "
            << (can_continue
                    ? "[c]ontinue session or [a]cknowledge review, or blank to cancel: "
                    : "[a]cknowledge review, or blank to cancel: ")
            << std::flush;
  std::string control{};
  if (!std::getline(std::cin, control)) return false;
  control = lowercase_copy(trim_ascii(control));
  if (control.empty()) return true;
  if ((!can_continue ||
       (control != "c" && control != "continue")) &&
      control != "a" && control != "ack" && control != "acknowledge") {
    if (error) *error = "invalid review action selection";
    return false;
  }
  if (can_continue && (control == "c" || control == "continue")) {
    std::cout << "Instruction that should launch more work: " << std::flush;
    std::string instruction{};
    if (!std::getline(std::cin, instruction)) return false;
    instruction = trim_ascii(instruction);
    if (instruction.empty()) {
      if (error) *error = "continue instruction is required";
      return false;
    }
    std::string structured{};
    if (!continue_marshal_session(app, session, instruction, &structured, error)) {
      return false;
    }
    std::cout << "\nSession continuation applied.\n" << structured << "\n";
    return true;
  }

  std::cout
      << "Acknowledgment message (required; this does not continue the session): "
      << std::flush;
  std::string note{};
  if (!std::getline(std::cin, note)) return false;
  note = trim_ascii(note);
  if (note.empty()) {
    if (error) *error = "review acknowledgment requires a non-empty note";
    return false;
  }

  std::string structured{};
  if (!acknowledge_session_summary(app, session, note, &structured, error)) {
    return false;
  }
  std::cout << "\nHuman review acknowledgment applied.\n"
            << structured << "\n";
  return true;
}

bool run_interactive_operator_console(app_context_t* app, std::string* error) {
  if (error) error->clear();
  if (!terminal_supports_human_ncurses_ui()) {
    std::cerr << "[hero_human_mcp] terminal does not support ncurses UI"
                 " cleanly (TERM="
              << trim_ascii(std::getenv("TERM") == nullptr ? "" : std::getenv("TERM"))
              << "); using line prompt operator console instead.\n";
    return run_line_prompt_operator_console(app, error);
  }
  std::string ncurses_error{};
  if (run_ncurses_operator_console(app, &ncurses_error)) return true;
  if (!ncurses_error.empty() &&
      ncurses_error.rfind(kNcursesInitErrorPrefix, 0) == 0) {
    return run_line_prompt_operator_console(app, error);
  }
  if (error) *error = ncurses_error.empty() ? "interactive operator console failed"
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
