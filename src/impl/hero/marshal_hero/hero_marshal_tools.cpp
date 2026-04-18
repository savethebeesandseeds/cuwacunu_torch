#include "hero/marshal_hero/hero_marshal_tools.h"

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
#include <system_error>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"
#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "camahjucunu/dsl/marshal_objective/marshal_objective.h"
#include "hero/human_hero/human_attestation.h"
#include "hero/marshal_hero/marshal_session.h"
#include "hero/marshal_hero/marshal_session_workspace.h"
#include "hero/runtime_hero/runtime_campaign.h"
#include "hero/runtime_hero/runtime_job.h"
#include "hero/wave_contract_binding_runtime.h"

namespace cuwacunu {
namespace hero {
namespace marshal_mcp {

namespace {

constexpr const char *kDefaultCampaignDslKey =
    "default_iitepi_campaign_dsl_filename";
constexpr const char *kDefaultMarshalObjectiveDslFilename =
    "default.marshal.objective.dsl";
constexpr const char *kServerName = "hero_marshal_mcp";
constexpr const char *kServerVersion = "0.1.0";
constexpr const char *kProtocolVersion = "2025-03-26";
constexpr const char *kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.marshal.*.";
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;
constexpr std::size_t kRuntimeToolTimeoutSec = 30;
constexpr std::size_t kHumanToolTimeoutSec = 30;
constexpr std::size_t kEmbeddedObjectiveBundleBytesCap = 64u << 10;
constexpr std::string_view kInputCheckpointSchemaV3 =
    "hero.marshal.input_checkpoint.v3";
constexpr std::string_view kIntentCheckpointSchemaV3 =
    "hero.marshal.intent_checkpoint.v3";
constexpr std::string_view kMutationCheckpointSchemaV3 =
    "hero.marshal.mutation_checkpoint.v3";
constexpr std::string_view kHumanRequestSchemaV3 =
    "hero.marshal.human_request.v3";
constexpr std::string_view kHumanClarificationAnswerSchemaV3 =
    "hero.human.clarification_answer.v3";

bool g_jsonrpc_use_content_length_framing = false;

[[nodiscard]] bool load_marshal_session_workspace_context(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    marshal_session_workspace_context_t *out, std::string *error);
[[nodiscard]] bool
call_runtime_tool(const app_context_t &app,
                  const std::filesystem::path &runtime_binary,
                  const std::string &tool_name, std::string arguments_json,
                  std::string *out_structured, std::string *error);
void append_structured_warnings(const std::string &structured_json,
                                std::string_view prefix,
                                std::vector<std::string> *out);

struct marshal_session_intent_t {
  std::string intent{};
  std::string launch_mode{};
  std::string launch_binding_id{};
  bool launch_reset_runtime_state{false};
  bool launch_requires_objective_mutation{false};
  std::string reason{};
  std::string memory_note{};
  std::string clarification_request{};
  std::string governance_kind{};
  std::string governance_request{};
  bool governance_allow_default_write{false};
  std::uint64_t governance_additional_campaign_launches{0};
};

using objective_hash_snapshot_t = std::map<std::string, std::string>;

using marshal_checkpoint_decision_t = marshal_session_intent_t;

struct run_manifest_hint_t {
  std::filesystem::path path{};
  std::string run_id{};
  std::string binding_id{};
  std::string contract_hash{};
  std::string wave_hash{};
  std::uint64_t started_at_ms{0};
};

struct objective_bundle_entry_t {
  std::string relative_path{};
  std::string text{};
};

struct objective_bundle_snapshot_t {
  std::vector<objective_bundle_entry_t> entries{};
  std::size_t embedded_bytes{0};
  std::size_t omitted_files{0};
  bool truncated{false};
};

struct marshal_objective_spec_t {
  std::string campaign_dsl_path{};
  std::string objective_md_path{};
  std::string guidance_md_path{};
  std::string objective_name{};
  std::string marshal_session_id{};
};

struct marshal_run_plan_progress_t {
  std::vector<std::string> declared_bind_ids{};
  std::vector<std::string> default_run_bind_ids{};
  std::vector<std::string> completed_run_bind_ids{};
  std::vector<std::string> attempted_run_bind_ids{};
  std::vector<std::string> pending_run_bind_ids{};
  bool ordered_prefix_valid{true};
  std::string next_pending_bind_id{};
};

struct marshal_start_session_overrides_t {
  std::string marshal_codex_model{};
  std::string marshal_codex_reasoning_effort{};
};

using marshal_tool_handler_t = bool (*)(app_context_t *app,
                                        const std::string &arguments_json,
                                        std::string *out_structured,
                                        std::string *out_error);

struct marshal_tool_descriptor_t {
  std::string_view name;
  std::string_view description;
  std::string_view input_schema_json;
  marshal_tool_handler_t handler;
};

enum class command_child_failure_stage_t : int {
  kNone = 0,
  kChdir = 1,
  kSetenv = 2,
  kStdinOpen = 3,
  kStdoutOpen = 4,
  kStderrOpen = 5,
  kExecvp = 6,
};

struct command_child_failure_t {
  int err{0};
  command_child_failure_stage_t stage{command_child_failure_stage_t::kNone};
};

void write_command_child_failure_noexcept(int fd,
                                          command_child_failure_stage_t stage,
                                          int err) {
  if (fd < 0)
    return;
  const command_child_failure_t failure{err, stage};
  const ssize_t ignored = ::write(fd, &failure, sizeof(failure));
  (void)ignored;
}

[[nodiscard]] std::string
describe_command_child_failure(const command_child_failure_t &failure,
                               const std::vector<std::string> &argv,
                               const std::filesystem::path *working_dir) {
  const std::string command =
      argv.empty() ? std::string("<empty>")
                   : cuwacunu::hero::runtime::trim_ascii(argv.front());
  const std::string reason =
      std::error_code(failure.err, std::generic_category()).message();
  switch (failure.stage) {
  case command_child_failure_stage_t::kChdir:
    return "command chdir failed for " +
           (working_dir != nullptr ? working_dir->string() : std::string(".")) +
           ": " + reason;
  case command_child_failure_stage_t::kSetenv:
    return "command environment setup failed for " + command + ": " + reason;
  case command_child_failure_stage_t::kStdinOpen:
    return "command stdin open failed for " + command + ": " + reason;
  case command_child_failure_stage_t::kStdoutOpen:
    return "command stdout open failed for " + command + ": " + reason;
  case command_child_failure_stage_t::kStderrOpen:
    return "command stderr open failed for " + command + ": " + reason;
  case command_child_failure_stage_t::kExecvp:
    return "command execvp failed for " + command + ": " + reason;
  case command_child_failure_stage_t::kNone:
  default:
    break;
  }
  return "command failed before exec: " + reason;
}

[[nodiscard]] bool run_command_with_stdio_and_timeout(
    const std::vector<std::string> &argv,
    const std::filesystem::path &stdin_path,
    const std::filesystem::path &stdout_path,
    const std::filesystem::path &stderr_path,
    const std::filesystem::path *working_dir,
    const std::vector<std::pair<std::string, std::string>> *env_overrides,
    std::size_t timeout_sec, const std::filesystem::path *pid_path,
    int *out_exit_code, std::string *error);

void append_warning_text(std::string *dst, std::string_view warning);

[[nodiscard]] bool read_marshal_session(
    const app_context_t &app, std::string_view marshal_session_id,
    cuwacunu::hero::marshal::marshal_session_record_t *out, std::string *error);

[[nodiscard]] bool read_runtime_campaign_direct(
    const app_context_t &app, std::string_view campaign_cursor,
    cuwacunu::hero::runtime::runtime_campaign_record_t *out,
    std::string *error);

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

[[nodiscard]] std::optional<std::uint64_t> marshal_session_elapsed_ms(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop) {
  const std::uint64_t end_ms = loop.finished_at_ms.has_value()
                                   ? *loop.finished_at_ms
                                   : loop.updated_at_ms;
  if (loop.started_at_ms == 0 || end_ms < loop.started_at_ms) {
    return std::nullopt;
  }
  return end_ms - loop.started_at_ms;
}

[[nodiscard]] std::string
format_compact_duration_ms(std::optional<std::uint64_t> duration_ms) {
  if (!duration_ms.has_value())
    return "<unknown>";
  if (*duration_ms < 1000)
    return std::to_string(*duration_ms) + " ms";

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
  const auto append_unit = [&](std::uint64_t value, const char *suffix) {
    if (value == 0)
      return;
    if (wrote)
      out << " ";
    out << value << suffix;
    wrote = true;
  };
  append_unit(days, "d");
  append_unit(hours, "h");
  append_unit(minutes, "m");
  if (!wrote || seconds != 0)
    append_unit(seconds, "s");
  return out.str();
}

[[nodiscard]] std::string format_effort_budget(std::uint64_t used,
                                               std::uint64_t limit,
                                               std::uint64_t remaining) {
  std::ostringstream out;
  out << used << " used";
  if (limit != 0)
    out << " / " << limit << " budgeted";
  out << " (" << remaining << " remaining)";
  return out.str();
}

[[nodiscard]] bool ends_with_ascii(std::string_view value,
                                   std::string_view suffix) {
  return value.size() >= suffix.size() &&
         value.substr(value.size() - suffix.size()) == suffix;
}

[[nodiscard]] bool
should_embed_objective_bundle_relative_path(std::string_view relative_path) {
  return ends_with_ascii(relative_path, ".dsl") ||
         ends_with_ascii(relative_path, ".md") ||
         ends_with_ascii(relative_path, ".man");
}

[[nodiscard]] bool looks_like_current_thread_id(std::string_view value) {
  if (value.size() != 36)
    return false;
  for (std::size_t i = 0; i < value.size(); ++i) {
    const char ch = value[i];
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (ch != '-')
        return false;
      continue;
    }
    if (!std::isxdigit(static_cast<unsigned char>(ch)))
      return false;
  }
  return true;
}

[[nodiscard]] std::string
extract_current_thread_id_from_log(std::string_view log_text) {
  const std::size_t marker = log_text.rfind("session id:");
  if (marker == std::string_view::npos)
    return {};
  const std::size_t value_begin =
      marker + std::string_view("session id:").size();
  const std::size_t line_end = log_text.find('\n', value_begin);
  const std::string candidate = lowercase_copy(
      trim_ascii(log_text.substr(value_begin, line_end == std::string_view::npos
                                                  ? std::string_view::npos
                                                  : line_end - value_begin)));
  if (!looks_like_current_thread_id(candidate))
    return {};
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
    if (!in_single && !in_double && (c == '#' || c == ';'))
      break;
    out.push_back(c);
  }
  return out;
}

[[nodiscard]] bool path_is_within(const std::filesystem::path &root,
                                  const std::filesystem::path &candidate) {
  if (root.empty() || candidate.empty())
    return false;
  const std::filesystem::path normalized_root = root.lexically_normal();
  const std::filesystem::path normalized_candidate =
      candidate.lexically_normal();
  auto root_it = normalized_root.begin();
  auto candidate_it = normalized_candidate.begin();
  for (; root_it != normalized_root.end(); ++root_it, ++candidate_it) {
    if (candidate_it == normalized_candidate.end() ||
        *root_it != *candidate_it) {
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

[[nodiscard]] std::string
encode_text_lines_as_jsonl(std::string_view stream_name,
                           std::string_view text) {
  std::ostringstream out;
  std::size_t line_index = 0;
  std::size_t pos = 0;
  while (pos <= text.size()) {
    const std::size_t end = text.find('\n', pos);
    const std::size_t line_end =
        end == std::string_view::npos ? text.size() : end;
    std::string_view line = text.substr(pos, line_end - pos);
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }
    if (!line.empty() || end != std::string_view::npos || pos != text.size()) {
      out << "{\"stream\":" << json_quote(stream_name)
          << ",\"line_index\":" << line_index
          << ",\"text\":" << json_quote(line) << "}\n";
      ++line_index;
    }
    if (end == std::string_view::npos)
      break;
    pos = end + 1;
  }
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
  if (path.empty())
    return {};
  const std::filesystem::path p(path);
  if (p.is_absolute())
    return p.lexically_normal().string();
  if (base_folder.empty())
    return p.lexically_normal().string();
  return (std::filesystem::path(base_folder) / p).lexically_normal().string();
}

[[nodiscard]] std::optional<std::string>
read_ini_value(const std::filesystem::path &ini_path,
               const std::string &section, const std::string &key) {
  std::ifstream in(ini_path);
  if (!in)
    return std::nullopt;
  std::string line;
  std::string current_section;
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(strip_inline_comment(line));
    if (trimmed.empty())
      continue;
    if (trimmed.size() >= 2 && trimmed.front() == '[' &&
        trimmed.back() == ']') {
      current_section = trim_ascii(trimmed.substr(1, trimmed.size() - 2));
      continue;
    }
    if (current_section != section)
      continue;
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos)
      continue;
    if (trim_ascii(trimmed.substr(0, eq)) != key)
      continue;
    return trim_ascii(trimmed.substr(eq + 1));
  }
  return std::nullopt;
}

[[nodiscard]] bool parse_size_token(std::string_view raw, std::size_t *out) {
  if (!out)
    return false;
  const std::string token = trim_ascii(raw);
  if (token.empty())
    return false;
  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(token.data(), token.data() + token.size(), parsed);
  if (ec != std::errc{} || ptr != token.data() + token.size())
    return false;
  *out = static_cast<std::size_t>(parsed);
  return true;
}

[[nodiscard]] bool parse_u64_token(std::string_view raw, std::uint64_t *out) {
  if (!out)
    return false;
  const std::string token = trim_ascii(raw);
  if (token.empty())
    return false;
  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(token.data(), token.data() + token.size(), parsed);
  if (ec != std::errc{} || ptr != token.data() + token.size())
    return false;
  *out = parsed;
  return true;
}

[[nodiscard]] bool parse_bool_token(std::string_view raw, bool *out) {
  if (!out)
    return false;
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

[[nodiscard]] bool normalize_codex_reasoning_effort(std::string_view raw,
                                                    std::string *out) {
  if (!out)
    return false;
  const std::string lowered = lowercase_copy(trim_ascii(raw));
  if (lowered == "low" || lowered == "medium" || lowered == "high" ||
      lowered == "xhigh") {
    *out = lowered;
    return true;
  }
  return false;
}

[[nodiscard]] std::string
json_array_from_strings(const std::vector<std::string> &values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0)
      out << ",";
    out << json_quote(values[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string
json_array_from_paths(const std::vector<std::filesystem::path> &values) {
  std::vector<std::string> encoded{};
  encoded.reserve(values.size());
  for (const auto &value : values)
    encoded.push_back(value.string());
  return json_array_from_strings(encoded);
}

[[nodiscard]] bool
is_valid_marshal_session_id(std::string_view marshal_session_id) {
  const std::string trimmed = trim_ascii(marshal_session_id);
  if (trimmed.empty())
    return false;
  for (const unsigned char ch : trimmed) {
    const bool ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' ||
                    ch == '-';
    if (!ok)
      return false;
  }
  return true;
}

[[nodiscard]] std::size_t skip_json_whitespace(const std::string &json,
                                               std::size_t pos) {
  while (pos < json.size() &&
         std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
    ++pos;
  }
  return pos;
}

[[nodiscard]] bool parse_json_string_at(const std::string &json,
                                        std::size_t pos, std::string *out,
                                        std::size_t *out_end) {
  if (out)
    out->clear();
  if (pos >= json.size() || json[pos] != '"')
    return false;
  ++pos;
  std::string parsed;
  while (pos < json.size()) {
    const char c = json[pos++];
    if (c == '"') {
      if (out)
        *out = parsed;
      if (out_end)
        *out_end = pos;
      return true;
    }
    if (c == '\\') {
      if (pos >= json.size())
        return false;
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
        if (pos + 4 > json.size())
          return false;
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

[[nodiscard]] bool find_json_value_end(const std::string &json, std::size_t pos,
                                       std::size_t *out_end) {
  pos = skip_json_whitespace(json, pos);
  if (pos >= json.size())
    return false;

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
      if (c == '{' || c == '[')
        ++depth;
      if (c == '}' || c == ']') {
        --depth;
        if (depth == 0) {
          if (out_end)
            *out_end = i + 1;
          return true;
        }
      }
    }
    return false;
  }
  std::size_t i = pos;
  while (i < json.size()) {
    const char c = json[i];
    if (c == ',' || c == '}' || c == ']')
      break;
    ++i;
  }
  while (i > pos &&
         std::isspace(static_cast<unsigned char>(json[i - 1])) != 0) {
    --i;
  }
  if (i == pos)
    return false;
  if (out_end)
    *out_end = i;
  return true;
}

[[nodiscard]] bool extract_json_field_raw(const std::string &json,
                                          std::string_view key,
                                          std::string *out_raw) {
  if (out_raw)
    out_raw->clear();
  const std::string needle = "\"" + std::string(key) + "\"";
  std::size_t pos = 0;
  while (true) {
    pos = json.find(needle, pos);
    if (pos == std::string::npos)
      return false;
    pos += needle.size();
    pos = skip_json_whitespace(json, pos);
    if (pos >= json.size() || json[pos] != ':')
      continue;
    pos = skip_json_whitespace(json, pos + 1);
    std::size_t end = 0;
    if (!find_json_value_end(json, pos, &end))
      return false;
    if (out_raw)
      *out_raw = json.substr(pos, end - pos);
    return true;
  }
}

[[nodiscard]] bool extract_json_string_field(const std::string &json,
                                             std::string_view key,
                                             std::string *out) {
  std::string raw;
  if (!extract_json_field_raw(json, key, &raw))
    return false;
  std::size_t end = 0;
  return parse_json_string_at(raw, 0, out, &end) &&
         skip_json_whitespace(raw, end) == raw.size();
}

[[nodiscard]] bool extract_json_bool_field(const std::string &json,
                                           std::string_view key, bool *out) {
  std::string raw;
  if (!extract_json_field_raw(json, key, &raw))
    return false;
  if (raw == "true") {
    if (out)
      *out = true;
    return true;
  }
  if (raw == "false") {
    if (out)
      *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] bool extract_json_size_field(const std::string &json,
                                           std::string_view key,
                                           std::size_t *out) {
  std::string raw;
  if (!extract_json_field_raw(json, key, &raw))
    return false;
  return parse_size_token(raw, out);
}

[[nodiscard]] bool extract_json_u64_field(const std::string &json,
                                          std::string_view key,
                                          std::uint64_t *out) {
  std::string raw;
  if (!extract_json_field_raw(json, key, &raw))
    return false;
  return parse_u64_token(raw, out);
}

[[nodiscard]] bool extract_json_object_field(const std::string &json,
                                             std::string_view key,
                                             std::string *out) {
  std::string raw;
  if (!extract_json_field_raw(json, key, &raw))
    return false;
  raw = trim_ascii(raw);
  if (raw.empty() || raw.front() != '{' || raw.back() != '}')
    return false;
  if (out)
    *out = std::move(raw);
  return true;
}

[[nodiscard]] bool extract_json_array_field(const std::string &json,
                                            std::string_view key,
                                            std::string *out) {
  std::string raw;
  if (!extract_json_field_raw(json, key, &raw))
    return false;
  raw = trim_ascii(raw);
  if (raw.empty() || raw.front() != '[' || raw.back() != ']')
    return false;
  if (out)
    *out = std::move(raw);
  return true;
}

[[nodiscard]] bool
extract_json_string_array_field(const std::string &json, std::string_view key,
                                std::vector<std::string> *out) {
  std::string raw{};
  if (!extract_json_array_field(json, key, &raw))
    return false;
  if (!out)
    return true;
  out->clear();
  std::size_t pos = skip_json_whitespace(raw, 0);
  if (pos >= raw.size() || raw[pos] != '[')
    return false;
  pos = skip_json_whitespace(raw, pos + 1);
  if (pos < raw.size() && raw[pos] == ']')
    return true;
  while (pos < raw.size()) {
    std::string value{};
    std::size_t end = 0;
    if (!parse_json_string_at(raw, pos, &value, &end))
      return false;
    out->push_back(std::move(value));
    pos = skip_json_whitespace(raw, end);
    if (pos >= raw.size())
      return false;
    if (raw[pos] == ']')
      return true;
    if (raw[pos] != ',')
      return false;
    pos = skip_json_whitespace(raw, pos + 1);
  }
  return false;
}

[[nodiscard]] bool
parse_start_session_overrides(const std::string &arguments_json,
                              marshal_start_session_overrides_t *out,
                              std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "marshal start-session overrides output pointer is null";
    return false;
  }
  *out = marshal_start_session_overrides_t{};

  if (extract_json_field_raw(arguments_json, "marshal_codex_model", nullptr)) {
    if (!extract_json_string_field(arguments_json, "marshal_codex_model",
                                   &out->marshal_codex_model)) {
      if (error)
        *error = "marshal_codex_model must be a JSON string";
      return false;
    }
    out->marshal_codex_model = trim_ascii(out->marshal_codex_model);
    if (out->marshal_codex_model.empty()) {
      if (error)
        *error = "marshal_codex_model must be non-empty";
      return false;
    }
  }

  if (extract_json_field_raw(arguments_json, "marshal_codex_reasoning_effort",
                             nullptr)) {
    std::string raw_reasoning{};
    if (!extract_json_string_field(
            arguments_json, "marshal_codex_reasoning_effort", &raw_reasoning)) {
      if (error) {
        *error = "marshal_codex_reasoning_effort must be a JSON string";
      }
      return false;
    }
    if (!normalize_codex_reasoning_effort(
            raw_reasoning, &out->marshal_codex_reasoning_effort)) {
      if (error) {
        *error = "marshal_codex_reasoning_effort must be one of: low, medium, "
                 "high, xhigh";
      }
      return false;
    }
  }

  return true;
}

[[nodiscard]] std::string
make_tool_result_json(std::string_view text, std::string_view structured_json,
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
      "{\"marshal_session_id\":\"\",\"error\":" + json_quote(message) + "}",
      true);
}

[[nodiscard]] bool tool_result_is_error(std::string_view tool_result_json) {
  return tool_result_json.find("\"isError\":true") != std::string_view::npos ||
         tool_result_json.find("\"isError\": true") != std::string_view::npos;
}

[[nodiscard]] bool
tool_result_structured_content(std::string_view tool_result_json,
                               std::string *out_structured,
                               std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!out_structured) {
    if (out_error)
      *out_error = "structuredContent output pointer is null";
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
        if (out_error)
          *out_error = error_value;
        return false;
      }
    }
    std::string text{};
    if (extract_json_string_field(json, "text", &text) && !text.empty()) {
      if (out_error)
        *out_error = text;
      return false;
    }
    if (out_error)
      *out_error = "tool returned error";
    return false;
  }
  if (!extract_json_object_field(json, "structuredContent", out_structured)) {
    if (out_error)
      *out_error = "tool result missing structuredContent";
    return false;
  }
  return true;
}

[[nodiscard]] std::filesystem::path resolve_runtime_root_from_global_config(
    const std::filesystem::path &global_config_path) {
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "GENERAL", "runtime_root");
  if (!configured.has_value())
    return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty())
    return {};
  return std::filesystem::path(resolved);
}

[[nodiscard]] std::filesystem::path resolve_repo_root_from_global_config(
    const std::filesystem::path &global_config_path) {
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "GENERAL", "repo_root");
  if (!configured.has_value())
    return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty())
    return {};
  return std::filesystem::path(resolved);
}

[[nodiscard]] std::filesystem::path resolve_campaign_grammar_from_global_config(
    const std::filesystem::path &global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "BNF", "iitepi_campaign_grammar_filename");
  if (!configured.has_value())
    return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty())
    return {};
  return std::filesystem::path(resolved);
}

[[nodiscard]] std::filesystem::path
resolve_marshal_objective_grammar_from_global_config(
    const std::filesystem::path &global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "BNF", "marshal_objective_grammar_filename");
  if (!configured.has_value())
    return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty())
    return {};
  return std::filesystem::path(resolved);
}

[[nodiscard]] std::filesystem::path resolve_default_campaign_dsl_path(
    const std::filesystem::path &global_config_path, std::string *error) {
  if (error)
    error->clear();
  const std::optional<std::string> configured =
      read_ini_value(global_config_path, "GENERAL", kDefaultCampaignDslKey);
  if (!configured.has_value()) {
    if (error) {
      *error = "missing GENERAL." + std::string(kDefaultCampaignDslKey) +
               " in " + global_config_path.string();
    }
    return {};
  }
  return std::filesystem::path(resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured));
}

[[nodiscard]] std::filesystem::path resolve_default_marshal_objective_dsl_path(
    const std::filesystem::path &global_config_path, std::string *error) {
  if (error)
    error->clear();
  const std::filesystem::path default_campaign_path =
      resolve_default_campaign_dsl_path(global_config_path, error);
  if (default_campaign_path.empty())
    return {};
  return (default_campaign_path.parent_path() /
          kDefaultMarshalObjectiveDslFilename)
      .lexically_normal();
}

[[nodiscard]] std::filesystem::path
resolve_command_path(const std::filesystem::path &base_dir,
                     std::string configured) {
  configured = trim_ascii(std::move(configured));
  if (configured.empty())
    return {};
  if (configured.find('/') != std::string::npos) {
    return std::filesystem::path(
        resolve_path_from_base_folder(base_dir.string(), configured));
  }
  const char *env_path = std::getenv("PATH");
  if (env_path != nullptr) {
    std::istringstream in(env_path);
    std::string entry{};
    while (std::getline(in, entry, ':')) {
      const std::filesystem::path candidate =
          (std::filesystem::path(entry) / configured).lexically_normal();
      if (::access(candidate.c_str(), X_OK) == 0)
        return candidate;
    }
  }
  return std::filesystem::path(configured);
}

[[nodiscard]] bool path_is_executable(const std::filesystem::path &path) {
  return !path.empty() && ::access(path.c_str(), X_OK) == 0;
}

[[nodiscard]] std::filesystem::path resolve_vscode_server_codex_binary() {
  std::vector<std::filesystem::path> extension_roots{};
  if (const char *home = std::getenv("HOME");
      home != nullptr && *home != '\0') {
    extension_roots.emplace_back(std::filesystem::path(home) /
                                 ".vscode-server/extensions");
  }
  extension_roots.emplace_back("/root/.vscode-server/extensions");

  std::vector<std::filesystem::path> candidates{};
  for (const auto &root : extension_roots) {
    std::error_code ec{};
    if (!std::filesystem::exists(root, ec) ||
        !std::filesystem::is_directory(root, ec)) {
      continue;
    }
    for (const auto &entry : std::filesystem::directory_iterator(root, ec)) {
      if (ec)
        break;
      if (!entry.is_directory())
        continue;
      const std::string dirname = entry.path().filename().string();
      if (dirname.rfind("openai.chatgpt-", 0u) != 0u)
        continue;
      const std::filesystem::path candidate =
          entry.path() / "bin/linux-x86_64/codex";
      if (path_is_executable(candidate)) {
        candidates.push_back(candidate.lexically_normal());
      }
    }
  }
  std::sort(candidates.begin(), candidates.end());
  if (!candidates.empty()) {
    return candidates.back();
  }
  return {};
}

[[nodiscard]] std::filesystem::path
resolve_marshal_codex_binary_path(const std::filesystem::path &base_dir,
                                  std::string configured) {
  const std::string trimmed = trim_ascii(configured);
  if (trimmed.empty())
    return {};

  std::filesystem::path resolved = resolve_command_path(base_dir, trimmed);
  if (path_is_executable(resolved))
    return resolved;

  if (trimmed != "codex")
    return resolved;

  const std::filesystem::path vscode_codex =
      resolve_vscode_server_codex_binary();
  if (path_is_executable(vscode_codex))
    return vscode_codex;

  for (const std::filesystem::path &candidate :
       {std::filesystem::path("/usr/local/bin/codex"),
        std::filesystem::path("/usr/bin/codex")}) {
    if (path_is_executable(candidate))
      return candidate;
  }
  return resolved;
}

[[nodiscard]] std::filesystem::path
campaigns_root_for_app(const app_context_t &app) {
  return app.defaults.marshal_root.parent_path() / ".campaigns";
}

[[nodiscard]] std::filesystem::path session_runner_lock_path(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop) {
  return cuwacunu::hero::marshal::marshal_session_runner_lock_path(
      std::filesystem::path(loop.session_root).parent_path(),
      loop.marshal_session_id);
}

[[nodiscard]] std::filesystem::path
marshal_session_latest_input_checkpoint_path(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop) {
  return cuwacunu::hero::marshal::marshal_session_latest_input_checkpoint_path(
      std::filesystem::path(loop.session_root).parent_path(),
      loop.marshal_session_id);
}

[[nodiscard]] std::filesystem::path
marshal_session_latest_intent_checkpoint_path(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop) {
  return cuwacunu::hero::marshal::marshal_session_latest_intent_checkpoint_path(
      std::filesystem::path(loop.session_root).parent_path(),
      loop.marshal_session_id);
}

[[nodiscard]] std::filesystem::path marshal_session_codex_stdout_path(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop) {
  return cuwacunu::hero::marshal::marshal_session_codex_stdout_path(
      std::filesystem::path(loop.session_root).parent_path(),
      loop.marshal_session_id);
}

[[nodiscard]] std::filesystem::path marshal_session_codex_stderr_path(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop) {
  return cuwacunu::hero::marshal::marshal_session_codex_stderr_path(
      std::filesystem::path(loop.session_root).parent_path(),
      loop.marshal_session_id);
}

[[nodiscard]] std::filesystem::path
marshal_session_runner_stdout_path(const app_context_t &app,
                                   std::string_view marshal_session_id) {
  return cuwacunu::hero::marshal::marshal_session_runner_stdout_path(
      app.defaults.marshal_root, marshal_session_id);
}

[[nodiscard]] std::filesystem::path
marshal_session_runner_stderr_path(const app_context_t &app,
                                   std::string_view marshal_session_id) {
  return cuwacunu::hero::marshal::marshal_session_runner_stderr_path(
      app.defaults.marshal_root, marshal_session_id);
}

[[nodiscard]] std::filesystem::path
resolve_marshal_session_runner_binary(const app_context_t &app) {
  const std::filesystem::path configured =
      app.self_binary_path.lexically_normal();
  if (!configured.empty()) {
    const std::string filename = configured.filename().string();
    if (filename == "hero_marshal.mcp" || filename == "hero_marshal_mcp") {
      return configured;
    }
  }

  if (!app.defaults.repo_root.empty()) {
    const std::filesystem::path fallback =
        (app.defaults.repo_root / ".build/hero/hero_marshal.mcp")
            .lexically_normal();
    std::error_code ec{};
    if (std::filesystem::exists(fallback, ec) &&
        std::filesystem::is_regular_file(fallback, ec)) {
      return fallback;
    }
  }

  return configured;
}

[[nodiscard]] std::filesystem::path
marshal_session_compat_codex_session_log_path(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop) {
  return cuwacunu::hero::marshal::marshal_session_compat_codex_session_log_path(
      std::filesystem::path(loop.session_root).parent_path(),
      loop.marshal_session_id);
}

[[nodiscard]] std::filesystem::path marshal_session_checkpoint_pid_path(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop) {
  return cuwacunu::hero::marshal::marshal_session_checkpoint_pid_path(
      std::filesystem::path(loop.session_root).parent_path(),
      loop.marshal_session_id);
}

void remove_file_noexcept(const std::filesystem::path &path) {
  if (path.empty())
    return;
  std::error_code ec{};
  (void)std::filesystem::remove(path, ec);
}

[[nodiscard]] bool read_session_checkpoint_pid(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    pid_t *out_pid) {
  if (!out_pid)
    return false;
  *out_pid = -1;
  std::string pid_text{};
  std::string ignored{};
  if (!cuwacunu::hero::runtime::read_text_file(
          marshal_session_checkpoint_pid_path(loop), &pid_text, &ignored)) {
    return false;
  }
  pid_text = trim_ascii(pid_text);
  if (pid_text.empty())
    return false;
  char *end = nullptr;
  const long parsed = std::strtol(pid_text.c_str(), &end, 10);
  if (end == nullptr || *end != '\0' || parsed <= 0)
    return false;
  *out_pid = static_cast<pid_t>(parsed);
  return true;
}

void cancel_session_checkpoint_best_effort(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop, bool force,
    std::string *out_warning) {
  pid_t checkpoint_pid = -1;
  if (!read_session_checkpoint_pid(loop, &checkpoint_pid))
    return;
  const int sig = force ? SIGKILL : SIGTERM;
  errno = 0;
  const int rc_group = ::kill(-checkpoint_pid, sig);
  const int saved_errno = errno;
  errno = 0;
  const int rc_proc = ::kill(checkpoint_pid, sig);
  const int saved_errno_proc = errno;
  if (rc_group != 0 && rc_proc != 0 && saved_errno != ESRCH &&
      saved_errno_proc != ESRCH && out_warning) {
    append_warning_text(out_warning, "checkpoint cancel degraded for pid=" +
                                         std::to_string(static_cast<long long>(
                                             checkpoint_pid)));
  }
  remove_file_noexcept(marshal_session_checkpoint_pid_path(loop));
}

[[nodiscard]] bool reload_terminal_marshal_session_if_any(
    const app_context_t &app, std::string_view marshal_session_id,
    cuwacunu::hero::marshal::marshal_session_record_t *out_loop) {
  if (!out_loop)
    return false;
  std::string ignored{};
  if (!read_marshal_session(app, marshal_session_id, out_loop, &ignored))
    return false;
  return cuwacunu::hero::marshal::is_marshal_session_terminal_state(
      out_loop->lifecycle);
}

struct scoped_temp_path_t {
  std::filesystem::path path{};

  ~scoped_temp_path_t() {
    if (path.empty())
      return;
    std::error_code ec{};
    std::filesystem::remove(path, ec);
  }
};

[[nodiscard]] bool
write_temp_marshal_decision_schema(std::string_view schema_json,
                                   scoped_temp_path_t *out_path,
                                   std::string *error) {
  if (error)
    error->clear();
  if (!out_path) {
    if (error)
      *error = "temp schema output pointer is null";
    return false;
  }
  out_path->path.clear();
  std::filesystem::path temp_path =
      std::filesystem::temp_directory_path() /
      ("hero_marshal_decision_schema." +
       std::to_string(cuwacunu::hero::runtime::now_ms_utc()) + "." +
       std::to_string(::getpid()) + ".json");
  if (!cuwacunu::hero::runtime::write_text_file_atomic(temp_path, schema_json,
                                                       error)) {
    return false;
  }
  out_path->path = std::move(temp_path);
  return true;
}

[[nodiscard]] bool write_temp_text_artifact(std::string_view prefix,
                                            std::string_view suffix,
                                            std::string_view text,
                                            scoped_temp_path_t *out_path,
                                            std::string *error) {
  if (error)
    error->clear();
  if (!out_path) {
    if (error)
      *error = "temp artifact output pointer is null";
    return false;
  }
  out_path->path.clear();
  std::filesystem::path temp_path =
      std::filesystem::temp_directory_path() /
      (std::string(prefix) + "." +
       std::to_string(cuwacunu::hero::runtime::now_ms_utc()) + "." +
       std::to_string(::getpid()) + "." + std::string(suffix));
  if (!cuwacunu::hero::runtime::write_text_file_atomic(temp_path, text,
                                                       error)) {
    return false;
  }
  out_path->path = std::move(temp_path);
  return true;
}

[[nodiscard]] std::string runtime_job_to_json(
    const cuwacunu::hero::runtime::runtime_job_record_t &record,
    const cuwacunu::hero::runtime::runtime_job_observation_t &observation) {
  const std::string observed_state =
      cuwacunu::hero::runtime::stable_state_for_observation(record,
                                                            observation);
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
      << "\"global_config_path\":" << json_quote(record.global_config_path)
      << ","
      << "\"source_campaign_dsl_path\":"
      << json_quote(record.source_campaign_dsl_path) << ","
      << "\"source_contract_dsl_path\":"
      << json_quote(record.source_contract_dsl_path) << ","
      << "\"source_wave_dsl_path\":" << json_quote(record.source_wave_dsl_path)
      << ","
      << "\"campaign_dsl_path\":" << json_quote(record.campaign_dsl_path) << ","
      << "\"contract_dsl_path\":" << json_quote(record.contract_dsl_path) << ","
      << "\"wave_dsl_path\":" << json_quote(record.wave_dsl_path) << ","
      << "\"binding_id\":" << json_quote(record.binding_id) << ","
      << "\"requested_device\":" << json_quote(record.requested_device) << ","
      << "\"resolved_device\":" << json_quote(record.resolved_device) << ","
      << "\"device_source_section\":"
      << json_quote(record.device_source_section) << ","
      << "\"device_contract_hash\":" << json_quote(record.device_contract_hash)
      << ","
      << "\"device_error\":" << json_quote(record.device_error) << ","
      << "\"cuda_required\":" << bool_json(record.cuda_required) << ","
      << "\"reset_runtime_state\":" << bool_json(record.reset_runtime_state)
      << ","
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
    const cuwacunu::hero::runtime::runtime_campaign_record_t &record) {
  const bool runner_alive =
      record.runner_pid.has_value() &&
      cuwacunu::hero::runtime::process_identity_alive(
          *record.runner_pid, record.runner_start_ticks, record.boot_id);
  std::ostringstream run_bind_ids_json;
  run_bind_ids_json << "[";
  for (std::size_t i = 0; i < record.run_bind_ids.size(); ++i) {
    if (i != 0)
      run_bind_ids_json << ",";
    run_bind_ids_json << json_quote(record.run_bind_ids[i]);
  }
  run_bind_ids_json << "]";

  std::ostringstream job_cursors_json;
  job_cursors_json << "[";
  for (std::size_t i = 0; i < record.job_cursors.size(); ++i) {
    if (i != 0)
      job_cursors_json << ",";
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
      << "\"marshal_session_id\":" << json_quote(record.marshal_session_id)
      << ","
      << "\"global_config_path\":" << json_quote(record.global_config_path)
      << ","
      << "\"source_campaign_dsl_path\":"
      << json_quote(record.source_campaign_dsl_path) << ","
      << "\"campaign_dsl_path\":" << json_quote(record.campaign_dsl_path) << ","
      << "\"reset_runtime_state\":" << bool_json(record.reset_runtime_state)
      << ","
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

[[nodiscard]] bool marshal_session_runner_process_alive(
    const cuwacunu::hero::marshal::marshal_session_record_t &record) {
  return record.runner_pid.has_value() &&
         cuwacunu::hero::runtime::process_identity_alive(
             *record.runner_pid, record.runner_start_ticks, record.boot_id);
}

[[nodiscard]] bool marshal_operator_message_has_text(
    const cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t
        &message) {
  return !trim_ascii(message.text).empty();
}

[[nodiscard]] bool marshal_session_has_pending_operator_message(
    const cuwacunu::hero::marshal::marshal_session_record_t &record) {
  return std::any_of(record.pending_operator_messages.begin(),
                     record.pending_operator_messages.end(),
                     [&](const auto &message) {
                       return marshal_operator_message_has_text(message);
                     });
}

[[nodiscard]] bool marshal_session_runner_should_be_attached(
    const cuwacunu::hero::marshal::marshal_session_record_t &record) {
  if (record.lifecycle == "bootstrapping")
    return true;
  if (record.lifecycle != "live")
    return false;
  if (cuwacunu::hero::marshal::marshal_session_has_active_campaign(record))
    return true;
  if (cuwacunu::hero::marshal::marshal_session_work_blocked(record))
    return false;
  if (cuwacunu::hero::marshal::marshal_session_is_review_ready(record))
    return false;
  if (marshal_session_has_pending_operator_message(record))
    return true;
  return trim_ascii(record.finish_reason) == "none";
}

[[nodiscard]] std::string marshal_session_to_json(
    const cuwacunu::hero::marshal::marshal_session_record_t &record) {
  const bool runner_alive = marshal_session_runner_process_alive(record);
  std::ostringstream pending_messages_json;
  pending_messages_json << "[";
  for (std::size_t i = 0; i < record.pending_operator_messages.size(); ++i) {
    if (i != 0)
      pending_messages_json << ",";
    const auto &message = record.pending_operator_messages[i];
    pending_messages_json
        << "{"
        << "\"message_id\":" << json_quote(message.message_id) << ","
        << "\"text\":" << json_quote(message.text) << ","
        << "\"delivery_mode\":" << json_quote(message.delivery_mode) << ","
        << "\"delivery_status\":" << json_quote(message.delivery_status)
        << ",\"thread_id_at_delivery\":"
        << json_quote(message.thread_id_at_delivery) << ","
        << "\"received_at_ms\":"
        << (message.received_at_ms.has_value()
                ? std::to_string(*message.received_at_ms)
                : "null")
        << ",\"delivered_at_ms\":"
        << (message.delivered_at_ms.has_value()
                ? std::to_string(*message.delivered_at_ms)
                : "null")
        << ",\"handled_at_ms\":"
        << (message.handled_at_ms.has_value()
                ? std::to_string(*message.handled_at_ms)
                : "null")
        << ",\"delivery_attempts\":" << message.delivery_attempts << ","
        << "\"last_error\":" << json_quote(message.last_error) << "}";
  }
  pending_messages_json << "]";
  std::ostringstream thread_lineage_json;
  thread_lineage_json << "[";
  for (std::size_t i = 0; i < record.thread_lineage.size(); ++i) {
    if (i != 0)
      thread_lineage_json << ",";
    thread_lineage_json << json_quote(record.thread_lineage[i]);
  }
  thread_lineage_json << "]";
  std::ostringstream campaign_cursors_json;
  campaign_cursors_json << "[";
  for (std::size_t i = 0; i < record.campaign_cursors.size(); ++i) {
    if (i != 0)
      campaign_cursors_json << ",";
    campaign_cursors_json << json_quote(record.campaign_cursors[i]);
  }
  campaign_cursors_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(record.schema) << ","
      << "\"marshal_session_id\":" << json_quote(record.marshal_session_id)
      << ","
      << "\"lifecycle\":" << json_quote(record.lifecycle) << ","
      << "\"status_detail\":" << json_quote(record.status_detail) << ","
      << "\"work_gate\":" << json_quote(record.work_gate) << ","
      << "\"finish_reason\":" << json_quote(record.finish_reason) << ","
      << "\"activity\":" << json_quote(record.activity) << ","
      << "\"objective_name\":" << json_quote(record.objective_name) << ","
      << "\"global_config_path\":" << json_quote(record.global_config_path)
      << ","
      << "\"boot_id\":" << json_quote(record.boot_id) << ","
      << "\"runner_pid\":"
      << (record.runner_pid.has_value() ? std::to_string(*record.runner_pid)
                                        : "null")
      << ",\"runner_start_ticks\":"
      << (record.runner_start_ticks.has_value()
              ? std::to_string(*record.runner_start_ticks)
              : "null")
      << ",\"runner_alive\":" << bool_json(runner_alive) << ","
      << "\"source_marshal_objective_dsl_path\":"
      << json_quote(record.source_marshal_objective_dsl_path) << ","
      << "\"source_campaign_dsl_path\":"
      << json_quote(record.source_campaign_dsl_path) << ","
      << "\"source_marshal_objective_md_path\":"
      << json_quote(record.source_marshal_objective_md_path) << ","
      << "\"source_marshal_guidance_md_path\":"
      << json_quote(record.source_marshal_guidance_md_path) << ","
      << "\"session_root\":" << json_quote(record.session_root) << ","
      << "\"objective_root\":" << json_quote(record.objective_root) << ","
      << "\"campaign_dsl_path\":" << json_quote(record.campaign_dsl_path) << ","
      << "\"marshal_objective_dsl_path\":"
      << json_quote(record.marshal_objective_dsl_path) << ","
      << "\"marshal_objective_md_path\":"
      << json_quote(record.marshal_objective_md_path) << ","
      << "\"marshal_guidance_md_path\":"
      << json_quote(record.marshal_guidance_md_path) << ","
      << "\"hero_marshal_dsl_path\":"
      << json_quote(record.hero_marshal_dsl_path) << ","
      << "\"config_policy_path\":" << json_quote(record.config_policy_path)
      << ","
      << "\"briefing_path\":" << json_quote(record.briefing_path) << ","
      << "\"memory_path\":" << json_quote(record.memory_path) << ","
      << "\"current_thread_id\":" << json_quote(record.current_thread_id) << ","
      << "\"codex_continuity\":" << json_quote(record.codex_continuity) << ","
      << "\"last_resume_error\":" << json_quote(record.last_resume_error) << ","
      << "\"thread_lineage\":" << thread_lineage_json.str() << ","
      << "\"resolved_marshal_codex_binary\":"
      << json_quote(record.resolved_marshal_codex_binary) << ","
      << "\"resolved_marshal_codex_model\":"
      << json_quote(record.resolved_marshal_codex_model) << ","
      << "\"resolved_marshal_codex_reasoning_effort\":"
      << json_quote(record.resolved_marshal_codex_reasoning_effort) << ","
      << "\"human_request_path\":" << json_quote(record.human_request_path)
      << ","
      << "\"codex_stdout_path\":" << json_quote(record.codex_stdout_path) << ","
      << "\"codex_stderr_path\":" << json_quote(record.codex_stderr_path) << ","
      << "\"governance_resolution_path\":"
      << json_quote(
             cuwacunu::hero::marshal::
                 marshal_session_human_governance_resolution_latest_path(
                     std::filesystem::path(record.session_root).parent_path(),
                     record.marshal_session_id)
                     .string())
      << ",\"governance_resolution_sig_path\":"
      << json_quote(
             cuwacunu::hero::marshal::
                 marshal_session_human_governance_resolution_latest_sig_path(
                     std::filesystem::path(record.session_root).parent_path(),
                     record.marshal_session_id)
                     .string())
      << ",\"clarification_answer_path\":"
      << json_quote(
             cuwacunu::hero::marshal::
                 marshal_session_human_clarification_answer_latest_path(
                     std::filesystem::path(record.session_root).parent_path(),
                     record.marshal_session_id)
                     .string())
      << ","
      << "\"events_path\":" << json_quote(record.events_path) << ","
      << "\"turns_path\":" << json_quote(record.turns_path) << ","
      << "\"started_at_ms\":" << record.started_at_ms << ","
      << "\"updated_at_ms\":" << record.updated_at_ms << ","
      << "\"finished_at_ms\":"
      << (record.finished_at_ms.has_value()
              ? std::to_string(*record.finished_at_ms)
              : "null")
      << ",\"checkpoint_count\":" << record.checkpoint_count
      << ",\"launch_count\":" << record.launch_count
      << ",\"max_campaign_launches\":" << record.max_campaign_launches
      << ",\"remaining_campaign_launches\":"
      << record.remaining_campaign_launches
      << ",\"authority_scope\":" << json_quote(record.authority_scope)
      << ",\"campaign_status\":" << json_quote(record.campaign_status) << ","
      << "\"campaign_cursor\":" << json_quote(record.campaign_cursor) << ","
      << "\"next_message_seq\":" << record.next_message_seq << ","
      << "\"pending_operator_messages\":" << pending_messages_json.str() << ","
      << "\"last_event_seq\":" << record.last_event_seq << ","
      << "\"last_codex_action\":" << json_quote(record.last_codex_action) << ","
      << "\"last_warning\":" << json_quote(record.last_warning) << ","
      << "\"last_warning_code\":" << json_quote(record.last_warning_code) << ","
      << "\"last_input_checkpoint_path\":"
      << json_quote(record.last_input_checkpoint_path) << ","
      << "\"last_intent_checkpoint_path\":"
      << json_quote(record.last_intent_checkpoint_path) << ","
      << "\"last_mutation_checkpoint_path\":"
      << json_quote(record.last_mutation_checkpoint_path) << ","
      << "\"campaign_cursors\":" << campaign_cursors_json.str() << "}";
  return out.str();
}

[[nodiscard]] bool append_marshal_session_event(
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string_view producer, std::string_view event,
    const std::string &payload_json, std::string *error, int /*payload_tag*/) {
  if (error)
    error->clear();
  if (!loop) {
    if (error)
      *error = "missing marshal session for event append";
    return false;
  }
  const std::uint64_t event_seq = ++loop->last_event_seq;
  const std::string payload =
      "{\"event_id\":" +
      json_quote(loop->marshal_session_id + ":" + std::to_string(event_seq)) +
      ",\"session_id\":" + json_quote(loop->marshal_session_id) +
      ",\"seq\":" + std::to_string(event_seq) +
      ",\"at_ms\":" + std::to_string(cuwacunu::hero::runtime::now_ms_utc()) +
      ",\"producer\":" + json_quote(std::string(producer)) +
      ",\"type\":" + json_quote(event) + ",\"payload\":" + payload_json + "}\n";
  return cuwacunu::hero::runtime::append_text_file(loop->events_path, payload,
                                                   error);
}

[[nodiscard]] bool append_marshal_session_event(
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string_view producer, std::string_view event, std::string_view detail,
    std::string *error) {
  return append_marshal_session_event(
      loop, producer, event,
      "{\"detail\":" + json_quote(std::string(detail)) + "}", error, 0);
}

[[nodiscard]] bool append_marshal_session_event(
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string_view event, std::string_view detail, std::string *error) {
  return append_marshal_session_event(loop, "marshal", event, detail, error);
}

[[nodiscard]] bool append_marshal_session_event(
    cuwacunu::hero::marshal::marshal_session_record_t **loop,
    std::string_view producer, std::string_view event, std::string_view detail,
    std::string *error) {
  if (!loop)
    return append_marshal_session_event(
        static_cast<cuwacunu::hero::marshal::marshal_session_record_t *>(
            nullptr),
        producer, event, detail, error);
  return append_marshal_session_event(*loop, producer, event, detail, error);
}

[[nodiscard]] bool append_marshal_session_event(
    cuwacunu::hero::marshal::marshal_session_record_t **loop,
    std::string_view event, std::string_view detail, std::string *error) {
  if (!loop)
    return append_marshal_session_event(
        static_cast<cuwacunu::hero::marshal::marshal_session_record_t *>(
            nullptr),
        event, detail, error);
  return append_marshal_session_event(*loop, event, detail, error);
}

[[nodiscard]] std::string make_operator_message_id(
    cuwacunu::hero::marshal::marshal_session_record_t *loop) {
  if (!loop)
    return "marshal.message.0";
  const std::uint64_t seq = ++loop->next_message_seq;
  return loop->marshal_session_id + ".msg." +
         cuwacunu::hero::runtime::base36_lower_u64(seq);
}

[[nodiscard]] std::string marshal_operator_message_to_payload_json(
    const cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t
        &message,
    std::string_view note = {}, std::string_view reply_text = {},
    std::string_view warning = {}) {
  std::ostringstream out;
  std::string detail = trim_ascii(std::string(note));
  if (!trim_ascii(reply_text).empty()) {
    if (!detail.empty())
      detail += " | ";
    detail += "reply=" + trim_ascii(reply_text);
  }
  if (!trim_ascii(warning).empty()) {
    if (!detail.empty())
      detail += " | ";
    detail += "warning=" + trim_ascii(warning);
  }
  out << "{"
      << "\"message_id\":" << json_quote(message.message_id) << ","
      << "\"text\":" << json_quote(message.text) << ","
      << "\"delivery_mode\":" << json_quote(message.delivery_mode) << ","
      << "\"delivery_status\":" << json_quote(message.delivery_status) << ","
      << "\"thread_id_at_delivery\":"
      << json_quote(message.thread_id_at_delivery) << ","
      << "\"received_at_ms\":"
      << (message.received_at_ms.has_value()
              ? std::to_string(*message.received_at_ms)
              : "null")
      << ",\"delivered_at_ms\":"
      << (message.delivered_at_ms.has_value()
              ? std::to_string(*message.delivered_at_ms)
              : "null")
      << ",\"handled_at_ms\":"
      << (message.handled_at_ms.has_value()
              ? std::to_string(*message.handled_at_ms)
              : "null")
      << ",\"delivery_attempts\":" << message.delivery_attempts << ","
      << "\"last_error\":" << json_quote(message.last_error) << ","
      << "\"detail\":" << json_quote(detail) << ","
      << "\"reply_text\":" << json_quote(std::string(reply_text)) << ","
      << "\"warning\":" << json_quote(std::string(warning)) << ","
      << "\"note\":" << json_quote(std::string(note)) << "}";
  return out.str();
}

[[nodiscard]] bool append_operator_message_event(
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string_view producer, std::string_view event,
    const cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t
        &message,
    std::string_view note, std::string_view reply_text,
    std::string_view warning, std::string *error) {
  return append_marshal_session_event(loop, producer, event,
                                      marshal_operator_message_to_payload_json(
                                          message, note, reply_text, warning),
                                      error, 0);
}

[[nodiscard]] bool append_operator_message_turn(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    const cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t
        &message,
    std::string_view reply_text, std::string *error) {
  if (error)
    error->clear();
  if (trim_ascii(loop.turns_path).empty())
    return true;
  const std::uint64_t at_ms =
      message.handled_at_ms.value_or(cuwacunu::hero::runtime::now_ms_utc());
  const std::string turn_id = loop.marshal_session_id + ".turn." +
                              std::to_string(loop.last_event_seq + 1);
  const std::string payload =
      "{\"turn_id\":" + json_quote(turn_id) +
      ",\"session_id\":" + json_quote(loop.marshal_session_id) +
      ",\"message_id\":" + json_quote(message.message_id) +
      ",\"at_ms\":" + std::to_string(at_ms) +
      ",\"thread_id\":" + json_quote(message.thread_id_at_delivery) +
      ",\"operator_text\":" + json_quote(message.text) +
      ",\"marshal_reply\":" + json_quote(std::string(reply_text)) + "}\n";
  return cuwacunu::hero::runtime::append_text_file(loop.turns_path, payload,
                                                   error);
}

[[nodiscard]] std::string read_recent_turn_log_excerpt(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::size_t max_turns) {
  if (trim_ascii(loop.turns_path).empty() || max_turns == 0)
    return {};
  std::string turns_text{};
  std::string ignored{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.turns_path, &turns_text,
                                               &ignored)) {
    return {};
  }
  std::vector<std::string> excerpts{};
  std::istringstream in{turns_text};
  std::string line{};
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(line);
    if (trimmed.empty())
      continue;
    std::string operator_text{};
    std::string marshal_reply{};
    if (!extract_json_string_field(trimmed, "operator_text", &operator_text))
      continue;
    (void)extract_json_string_field(trimmed, "marshal_reply", &marshal_reply);
    excerpts.push_back("Operator: " + trim_ascii(operator_text) +
                       "\nMarshal: " + trim_ascii(marshal_reply));
  }
  if (excerpts.empty())
    return {};
  if (excerpts.size() > max_turns) {
    excerpts.erase(excerpts.begin(),
                   excerpts.begin() +
                       static_cast<std::vector<std::string>::difference_type>(
                           excerpts.size() - max_turns));
  }
  std::ostringstream out;
  for (std::size_t i = 0; i < excerpts.size(); ++i) {
    if (i != 0)
      out << "\n\n";
    out << excerpts[i];
  }
  return out.str();
}

[[nodiscard]] std::string build_replacement_thread_rebrief_packet(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::string_view rollover_reason, std::string_view briefing_text) {
  std::ostringstream out;
  out << "Replacement Codex thread rebrief packet\n"
      << "- marshal_session_id: " << loop.marshal_session_id << "\n"
      << "- objective_name: " << loop.objective_name << "\n"
      << "- lifecycle: " << loop.lifecycle << "\n"
      << "- work_gate: " << loop.work_gate << "\n"
      << "- campaign_status: " << loop.campaign_status << "\n";
  if (!trim_ascii(loop.campaign_cursor).empty()) {
    out << "- campaign_cursor: " << loop.campaign_cursor << "\n";
  }
  if (!trim_ascii(loop.last_warning).empty()) {
    out << "- last_warning: " << loop.last_warning << "\n";
  }
  if (!trim_ascii(loop.last_resume_error).empty()) {
    out << "- last_resume_error: " << loop.last_resume_error << "\n";
  }
  out << "- rollover_reason: " << trim_ascii(rollover_reason) << "\n";
  if (!trim_ascii(loop.current_thread_id).empty()) {
    out << "- previous_thread_id: " << loop.current_thread_id << "\n";
  }
  const std::string recent_turns = read_recent_turn_log_excerpt(loop, 6);
  if (!recent_turns.empty()) {
    out << "\nRecent exact turns:\n" << recent_turns << "\n";
  }
  std::size_t pending_count = 0;
  for (const auto &pending_message : loop.pending_operator_messages) {
    if (!marshal_operator_message_has_text(pending_message))
      continue;
    if (pending_count == 0)
      out << "\nPending operator messages:\n";
    out << "- [" << pending_message.message_id << "] "
        << trim_ascii(pending_message.text) << "\n";
    if (++pending_count >= 6)
      break;
  }
  if (!trim_ascii(briefing_text).empty()) {
    out << "\nCurrent durable briefing:\n" << briefing_text;
    if (briefing_text.back() != '\n')
      out << "\n";
  }
  return out.str();
}

[[nodiscard]] bool
read_marshal_session(const app_context_t &app,
                     std::string_view marshal_session_id,
                     cuwacunu::hero::marshal::marshal_session_record_t *out,
                     std::string *error) {
  if (!cuwacunu::hero::marshal::read_marshal_session_record(
          app.defaults.marshal_root, marshal_session_id, out, error)) {
    return false;
  }
  if (out != nullptr) {
    out->schema = std::string(cuwacunu::hero::marshal::kMarshalSessionSchemaV6);
    cuwacunu::hero::marshal::canonicalize_marshal_session_record(out);
  }
  if (out != nullptr && trim_ascii(out->codex_stdout_path).empty()) {
    out->codex_stdout_path =
        cuwacunu::hero::marshal::marshal_session_codex_stdout_path(
            app.defaults.marshal_root, out->marshal_session_id)
            .string();
  }
  if (out != nullptr && trim_ascii(out->codex_stderr_path).empty()) {
    const std::filesystem::path stderr_path =
        cuwacunu::hero::marshal::marshal_session_codex_stderr_path(
            app.defaults.marshal_root, out->marshal_session_id);
    const std::filesystem::path compat_path =
        cuwacunu::hero::marshal::marshal_session_compat_codex_session_log_path(
            app.defaults.marshal_root, out->marshal_session_id);
    std::error_code ec{};
    if (std::filesystem::exists(stderr_path, ec) && !ec) {
      out->codex_stderr_path = stderr_path.string();
    } else {
      ec.clear();
      if (std::filesystem::exists(compat_path, ec) && !ec) {
        out->codex_stderr_path = compat_path.string();
      } else {
        out->codex_stderr_path = stderr_path.string();
      }
    }
  }
  return true;
}

void clear_marshal_session_runner_identity(
    cuwacunu::hero::marshal::marshal_session_record_t *loop);
void clear_marshal_session_campaign_linkage(
    cuwacunu::hero::marshal::marshal_session_record_t *loop);

[[nodiscard]] bool write_marshal_session(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &record,
    std::string *error) {
  cuwacunu::hero::marshal::marshal_session_record_t persisted = record;
  persisted.schema =
      std::string(cuwacunu::hero::marshal::kMarshalSessionSchemaV6);
  cuwacunu::hero::marshal::canonicalize_marshal_session_record(&persisted);
  if (!marshal_session_runner_should_be_attached(persisted)) {
    clear_marshal_session_runner_identity(&persisted);
  }
  auto sync_human_summary_report =
      [&](const cuwacunu::hero::marshal::marshal_session_record_t &loop,
          std::string *out_error) -> bool {
    if (out_error)
      out_error->clear();
    const std::filesystem::path summary_path =
        cuwacunu::hero::marshal::marshal_session_human_summary_path(
            app.defaults.marshal_root, loop.marshal_session_id);
    const std::filesystem::path ack_path =
        cuwacunu::hero::marshal::marshal_session_human_summary_ack_latest_path(
            app.defaults.marshal_root, loop.marshal_session_id);
    const std::filesystem::path ack_sig_path = cuwacunu::hero::marshal::
        marshal_session_human_summary_ack_latest_sig_path(
            app.defaults.marshal_root, loop.marshal_session_id);

    if (!cuwacunu::hero::marshal::is_marshal_session_summary_state(loop)) {
      std::error_code ec{};
      (void)std::filesystem::remove(summary_path, ec);
      (void)std::filesystem::remove(ack_path, ec);
      (void)std::filesystem::remove(ack_sig_path, ec);
      return true;
    }

    std::error_code ec{};
    std::filesystem::create_directories(summary_path.parent_path(), ec);
    if (ec) {
      if (out_error) {
        *out_error = "cannot create human summary dir: " +
                     summary_path.parent_path().string();
      }
      return false;
    }

    const std::string last_campaign_cursor = loop.campaign_cursors.empty()
                                                 ? std::string()
                                                 : loop.campaign_cursors.back();
    const std::optional<std::uint64_t> elapsed_ms =
        marshal_session_elapsed_ms(loop);
    std::ostringstream out;
    out << "# Human Summary Report\n\n"
        << "Marshal ID: " << loop.marshal_session_id << "\n"
        << "Lifecycle: " << loop.lifecycle << "\n"
        << "Work Gate: " << loop.work_gate << "\n"
        << "Activity: " << loop.activity << "\n"
        << "Campaign Status: " << loop.campaign_status << "\n"
        << "Finish Reason: " << loop.finish_reason << "\n"
        << "Objective: " << loop.objective_name << "\n"
        << "Started At Ms: " << loop.started_at_ms << "\n"
        << "Finished At Ms: ";
    if (loop.finished_at_ms.has_value()) {
      out << *loop.finished_at_ms;
    } else {
      out << "<unset>";
    }
    out << "\n"
        << "Effort Summary:\n"
        << "- Wall Time: " << format_compact_duration_ms(elapsed_ms);
    if (elapsed_ms.has_value())
      out << " (" << *elapsed_ms << " ms)";
    out << "\n"
        << "- Checkpoints: " << loop.checkpoint_count << "\n"
        << "- Campaign Launches: "
        << format_effort_budget(loop.launch_count, loop.max_campaign_launches,
                                loop.remaining_campaign_launches)
        << "\n"
        << "Last Codex Action: "
        << (trim_ascii(loop.last_codex_action).empty() ? "<unset>"
                                                       : loop.last_codex_action)
        << "\n";
    if (!trim_ascii(last_campaign_cursor).empty()) {
      out << "Last Campaign Cursor: " << last_campaign_cursor << "\n";
    }
    out << "\nSummary:\n"
        << (trim_ascii(loop.status_detail).empty() ? "<no state detail>"
                                                   : loop.status_detail)
        << "\n\n";
    if (!trim_ascii(loop.last_warning).empty()) {
      out << "Warnings:\n" << loop.last_warning << "\n\n";
    }
    if (cuwacunu::hero::marshal::marshal_session_is_review_ready(loop)) {
      out << "This report is actionable. Use Message in Human Hero, or "
             "hero.marshal.message_session, when you want to send more "
             "operator guidance after reviewing the outcome. Acknowledge with "
             "a review message "
             "only signs off this report and clears it from the Human "
             "inbox. Archive through Human Hero when you want to release "
             "objective ownership and stop this session from blocking a fresh "
             "launch.\n\n";
    } else {
      out << "This report is informational. Acknowledge it through Human Hero "
             "with a review message when you have reviewed the session "
             "outcome. That action only clears the report from the Human "
             "inbox; it does not reopen the session.\n\n";
    }
    out << "Key files:\n"
        << "- Session manifest: "
        << cuwacunu::hero::marshal::marshal_session_manifest_path(
               app.defaults.marshal_root, loop.marshal_session_id)
               .string()
        << "\n"
        << "- Latest input checkpoint: "
        << cuwacunu::hero::marshal::
               marshal_session_latest_input_checkpoint_path(
                   app.defaults.marshal_root, loop.marshal_session_id)
                   .string()
        << "\n"
        << "- Latest intent checkpoint: "
        << cuwacunu::hero::marshal::
               marshal_session_latest_intent_checkpoint_path(
                   app.defaults.marshal_root, loop.marshal_session_id)
                   .string()
        << "\n"
        << "- Codex stdout: " << loop.codex_stdout_path << "\n"
        << "- Codex stderr: " << loop.codex_stderr_path << "\n"
        << "- Memory: " << loop.memory_path << "\n"
        << "- Events: " << loop.events_path << "\n";
    if (!trim_ascii(last_campaign_cursor).empty()) {
      out << "- Runtime campaign: " << last_campaign_cursor << "\n";
    }
    out << "- Summary ack target: " << ack_path.string() << "\n";

    return cuwacunu::hero::runtime::write_text_file_atomic(
        summary_path, out.str(), out_error);
  };

  if (!cuwacunu::hero::marshal::write_marshal_session_record(
          app.defaults.marshal_root, persisted, error)) {
    return false;
  }
  std::string summary_error{};
  if (!sync_human_summary_report(persisted, &summary_error)) {
    std::cerr << "[hero_marshal_mcp][warning] failed to refresh Human Hero "
                 "summary artifact: "
              << summary_error << std::endl;
  }
  std::string marker_error{};
  if (!cuwacunu::hero::marshal::sync_human_pending_request_count(
          app.defaults.marshal_root, &marker_error)) {
    std::cerr << "[hero_marshal_mcp][warning] failed to refresh Human Hero "
                 "pending marker: "
              << marker_error << std::endl;
  }
  marker_error.clear();
  if (!cuwacunu::hero::marshal::sync_human_pending_summary_count(
          app.defaults.marshal_root, &marker_error)) {
    std::cerr << "[hero_marshal_mcp][warning] failed to refresh Human Hero "
                 "finished marker: "
              << marker_error << std::endl;
  }
  return true;
}

[[nodiscard]] bool list_marshal_sessions(
    const app_context_t &app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out,
    std::string *error) {
  return cuwacunu::hero::marshal::scan_marshal_session_records(
      app.defaults.marshal_root, out, error);
}

[[nodiscard]] bool read_runtime_campaign_direct(
    const app_context_t &app, std::string_view campaign_cursor,
    cuwacunu::hero::runtime::runtime_campaign_record_t *out,
    std::string *error) {
  return cuwacunu::hero::runtime::read_runtime_campaign_record(
      campaigns_root_for_app(app), campaign_cursor, out, error);
}

[[nodiscard]] bool extract_runtime_campaign_from_tool_structured(
    const std::string &structured_json,
    cuwacunu::hero::runtime::runtime_campaign_record_t *out,
    std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "runtime campaign output pointer is null";
    return false;
  }
  *out = cuwacunu::hero::runtime::runtime_campaign_record_t{};

  std::string campaign_json{};
  if (!extract_json_object_field(structured_json, "campaign", &campaign_json)) {
    if (error)
      *error = "runtime tool structured output is missing campaign object";
    return false;
  }
  std::string schema{};
  if (!extract_json_string_field(campaign_json, "schema", &schema) ||
      trim_ascii(schema) !=
          std::string(cuwacunu::hero::runtime::kRuntimeCampaignSchemaV2)) {
    if (error)
      *error = "runtime tool campaign object has unexpected schema";
    return false;
  }
  if (!extract_json_string_field(campaign_json, "campaign_cursor",
                                 &out->campaign_cursor)) {
    if (error)
      *error = "runtime tool campaign object is missing campaign_cursor";
    return false;
  }
  (void)extract_json_string_field(campaign_json, "state", &out->state);
  (void)extract_json_string_field(campaign_json, "state_detail",
                                  &out->state_detail);
  (void)extract_json_string_field(campaign_json, "marshal_session_id",
                                  &out->marshal_session_id);
  std::uint64_t current_run_index = 0;
  if (extract_json_u64_field(campaign_json, "current_run_index",
                             &current_run_index)) {
    out->current_run_index = current_run_index;
  }
  return true;
}

[[nodiscard]] bool request_runtime_campaign_stop_for_marshal_action(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string_view campaign_cursor, bool force,
    cuwacunu::hero::runtime::runtime_campaign_record_t *out_campaign,
    std::string *out_warning, std::string *error) {
  if (error)
    error->clear();
  if (out_warning)
    out_warning->clear();
  if (!out_campaign) {
    if (error)
      *error = "runtime campaign stop output pointer is null";
    return false;
  }
  *out_campaign = cuwacunu::hero::runtime::runtime_campaign_record_t{};

  marshal_session_workspace_context_t workspace_context{};
  if (!load_marshal_session_workspace_context(app, session, &workspace_context,
                                              error)) {
    return false;
  }
  std::string stop_structured{};
  const std::string stop_args =
      "{\"campaign_cursor\":" + json_quote(trim_ascii(campaign_cursor)) +
      ",\"force\":" + bool_json(force) +
      ",\"suppress_marshal_session_update\":true}";
  if (!call_runtime_tool(app, workspace_context.runtime_hero_binary,
                         "hero.runtime.stop_campaign", stop_args,
                         &stop_structured, error)) {
    return false;
  }
  std::vector<std::string> warnings{};
  append_structured_warnings(stop_structured,
                             "runtime stop warning: ", &warnings);
  for (const std::string &warning : warnings) {
    append_warning_text(out_warning, warning);
  }
  return extract_runtime_campaign_from_tool_structured(stop_structured,
                                                       out_campaign, error);
}

[[nodiscard]] bool
read_runtime_job_direct(const app_context_t &app, std::string_view job_cursor,
                        cuwacunu::hero::runtime::runtime_job_record_t *out,
                        std::string *error) {
  return cuwacunu::hero::runtime::read_runtime_job_record(
      campaigns_root_for_app(app), job_cursor, out, error);
}

[[nodiscard]] std::filesystem::path runtime_job_trace_path_for_record(
    const app_context_t &app,
    const cuwacunu::hero::runtime::runtime_job_record_t &record) {
  if (!record.trace_path.empty())
    return std::filesystem::path(record.trace_path);
  return cuwacunu::hero::runtime::runtime_job_trace_path(
      campaigns_root_for_app(app), record.job_cursor);
}

[[nodiscard]] bool is_runtime_campaign_terminal_state(std::string_view state) {
  return state != "launching" && state != "running" && state != "stopping";
}

[[nodiscard]] bool is_marshal_session_terminal_state(std::string_view state) {
  return cuwacunu::hero::marshal::is_marshal_session_terminal_state(state);
}

[[nodiscard]] std::string observed_campaign_state(
    const cuwacunu::hero::runtime::runtime_campaign_record_t &record) {
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

void clear_marshal_session_runner_identity(
    cuwacunu::hero::marshal::marshal_session_record_t *loop) {
  if (!loop)
    return;
  loop->boot_id.clear();
  loop->runner_pid.reset();
  loop->runner_start_ticks.reset();
}

void clear_marshal_session_campaign_linkage(
    cuwacunu::hero::marshal::marshal_session_record_t *loop) {
  if (!loop)
    return;
  loop->campaign_status = "none";
  loop->campaign_cursor.clear();
}

[[nodiscard]] bool session_runner_lock_is_held(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    bool *out_held, std::string *error) {
  if (error)
    error->clear();
  if (!out_held) {
    if (error)
      *error = "session runner lock held output pointer is null";
    return false;
  }
  *out_held = false;
  const std::filesystem::path lock_path = session_runner_lock_path(loop);
  const int fd = ::open(lock_path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
  if (fd < 0) {
    if (error)
      *error = "cannot open session runner lock: " + lock_path.string();
    return false;
  }
  if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
    const int err = errno;
    (void)::close(fd);
    if (err == EWOULDBLOCK || err == EAGAIN) {
      *out_held = true;
      return true;
    }
    if (error)
      *error = "cannot probe session runner lock: " + loop.marshal_session_id;
    return false;
  }
  (void)::flock(fd, LOCK_UN);
  (void)::close(fd);
  return true;
}

[[nodiscard]] bool marshal_session_runner_alive(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    bool *out_alive, std::string *error) {
  if (error)
    error->clear();
  if (!out_alive) {
    if (error)
      *error = "session runner alive output pointer is null";
    return false;
  }
  *out_alive = false;
  if (marshal_session_runner_process_alive(loop)) {
    *out_alive = true;
    return true;
  }
  return session_runner_lock_is_held(loop, out_alive, error);
}

[[nodiscard]] bool capture_process_identity_for_session(
    std::uint64_t pid_value,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string *error) {
  if (error)
    error->clear();
  if (!loop) {
    if (error)
      *error = "marshal session pointer is null";
    return false;
  }
  if (pid_value == 0) {
    if (error)
      *error = "marshal session runner pid is zero";
    return false;
  }
  loop->boot_id = cuwacunu::hero::runtime::current_boot_id();
  loop->runner_pid = pid_value;
  std::uint64_t start_ticks = 0;
  if (cuwacunu::hero::runtime::read_process_start_ticks(pid_value,
                                                        &start_ticks)) {
    loop->runner_start_ticks = start_ticks;
  } else {
    loop->runner_start_ticks.reset();
  }
  return true;
}

[[nodiscard]] bool park_session_idle_after_interruption(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string_view detail, bool *out_changed, std::string *error) {
  if (error)
    error->clear();
  if (out_changed)
    *out_changed = false;
  if (!loop) {
    if (error)
      *error = "interrupt park session pointer is null";
    return false;
  }
  const std::string normalized_detail = trim_ascii(detail);
  const std::optional<std::uint64_t> finished_at =
      loop->finished_at_ms.has_value()
          ? loop->finished_at_ms
          : std::optional<std::uint64_t>(cuwacunu::hero::runtime::now_ms_utc());
  const bool needs_change =
      !cuwacunu::hero::marshal::marshal_session_is_review_ready(*loop) ||
      loop->status_detail != normalized_detail || loop->work_gate != "open" ||
      loop->finish_reason != "interrupted" ||
      !loop->finished_at_ms.has_value() ||
      trim_ascii(loop->campaign_cursor).size() != 0 ||
      !trim_ascii(loop->boot_id).empty() || loop->runner_pid.has_value() ||
      loop->runner_start_ticks.has_value();
  if (!needs_change)
    return true;

  loop->lifecycle = "live";
  loop->activity = "review";
  loop->status_detail = normalized_detail;
  loop->work_gate = "open";
  loop->finish_reason = "interrupted";
  loop->finished_at_ms = finished_at;
  loop->updated_at_ms = *loop->finished_at_ms;
  clear_marshal_session_campaign_linkage(loop);
  clear_marshal_session_runner_identity(loop);
  if (!write_marshal_session(app, *loop, error))
    return false;
  std::string ignored{};
  (void)append_marshal_session_event(&loop, "session.reconciled",
                                     loop->status_detail, &ignored);
  if (out_changed)
    *out_changed = true;
  return true;
}

[[nodiscard]] bool reconcile_marshal_session(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop, bool *out_changed,
    std::string *error) {
  if (error)
    error->clear();
  if (out_changed)
    *out_changed = false;
  if (!loop) {
    if (error)
      *error = "marshal session pointer is null";
    return false;
  }

  bool runner_alive = false;
  if (!marshal_session_runner_alive(*loop, &runner_alive, error))
    return false;
  if (runner_alive)
    return true;

  {
    const std::string retained_campaign_cursor =
        trim_ascii(loop->campaign_cursor);
    const bool retained_work_blocked =
        loop->lifecycle == "live" && loop->work_gate != "open";
    const bool retained_review_ready =
        cuwacunu::hero::marshal::marshal_session_is_review_ready(*loop);
    if (!retained_campaign_cursor.empty() &&
        (retained_work_blocked || retained_review_ready ||
         is_marshal_session_terminal_state(loop->lifecycle))) {
      cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
      if (!read_runtime_campaign_direct(app, retained_campaign_cursor,
                                        &campaign, error)) {
        return false;
      }
      const std::string observed_state = observed_campaign_state(campaign);
      if (is_runtime_campaign_terminal_state(observed_state)) {
        clear_marshal_session_campaign_linkage(loop);
        const std::string detail_lower = lowercase_copy(loop->status_detail);
        if (retained_work_blocked &&
            detail_lower.find("pause_session requested by operator") == 0) {
          loop->status_detail = "pause_session requested by operator";
        } else if (is_marshal_session_terminal_state(loop->lifecycle) &&
                   detail_lower.find(
                       "terminate_session requested by operator") == 0) {
          loop->status_detail = "terminate_session requested by operator";
        }
        loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
        if (!write_marshal_session(app, *loop, error))
          return false;
        if (out_changed)
          *out_changed = true;
      }
    }
  }

  const bool review_ready =
      cuwacunu::hero::marshal::marshal_session_is_review_ready(*loop);
  const bool campaign_active =
      cuwacunu::hero::marshal::marshal_session_has_active_campaign(*loop);
  if (campaign_active) {
    const std::string campaign_cursor = trim_ascii(loop->campaign_cursor);
    if (campaign_cursor.empty()) {
      return park_session_idle_after_interruption(
          app, loop,
          "sudden interruption detected after the marshal runner disappeared "
          "while no active runtime campaign was recorded; session parked for "
          "review "
          "with recovery context so the operator can review runtime evidence "
          "and use message_session to resume",
          out_changed, error);
    }
    cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
    std::string campaign_error{};
    if (!read_runtime_campaign_direct(app, campaign_cursor, &campaign,
                                      &campaign_error)) {
      return park_session_idle_after_interruption(
          app, loop,
          "sudden interruption detected after the marshal runner disappeared "
          "while tracking runtime campaign " +
              campaign_cursor +
              "; runtime campaign manifest could not "
              "be read: " +
              campaign_error +
              "; session parked for review with recovery context so the "
              "operator can inspect runtime evidence and use message_session "
              "to resume",
          out_changed, error);
    }
    const std::string observed_state = observed_campaign_state(campaign);
    std::string observed_detail = trim_ascii(campaign.state_detail);
    if (observed_state != campaign.state && observed_state == "failed") {
      observed_detail = "campaign runner is no longer alive";
    }
    if (observed_detail.empty()) {
      if (observed_state == "failed") {
        observed_detail = "campaign runner is no longer alive";
      } else if (observed_state == "running") {
        observed_detail = "campaign runner alive";
      }
    }
    std::ostringstream detail;
    detail << "sudden interruption detected after runtime campaign "
           << campaign_cursor << " was left in observed state "
           << observed_state;
    if (!observed_detail.empty())
      detail << ": " << observed_detail;
    detail << "; session parked for review with recovery context so the "
              "operator can inspect runtime evidence and use message_session "
              "to resume";
    return park_session_idle_after_interruption(app, loop, detail.str(),
                                                out_changed, error);
  }

  if (loop->lifecycle == "live" && loop->work_gate == "open" && !review_ready) {
    std::ostringstream detail;
    detail << "sudden interruption detected while the marshal planning runner "
              "was active";
    const std::string checkpoint_path =
        trim_ascii(loop->last_input_checkpoint_path);
    if (!checkpoint_path.empty()) {
      detail << " near input checkpoint " << checkpoint_path;
    }
    detail << "; session parked for review with recovery context so the "
              "operator can review the interrupted checkpoint and use "
              "message_session to resume";
    return park_session_idle_after_interruption(app, loop, detail.str(),
                                                out_changed, error);
  }

  return true;
}

[[nodiscard]] bool read_marshal_session_reconciled(
    const app_context_t &app, std::string_view marshal_session_id,
    cuwacunu::hero::marshal::marshal_session_record_t *out, bool *out_changed,
    std::string *error) {
  if (error)
    error->clear();
  if (out_changed)
    *out_changed = false;
  if (!read_marshal_session(app, marshal_session_id, out, error))
    return false;
  return reconcile_marshal_session(app, out, out_changed, error);
}

[[nodiscard]] bool list_marshal_sessions_reconciled(
    const app_context_t &app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out,
    std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "missing destination for marshal session list";
    return false;
  }
  if (!list_marshal_sessions(app, out, error))
    return false;
  for (auto &session : *out) {
    bool ignored_changed = false;
    if (!reconcile_marshal_session(app, &session, &ignored_changed, error))
      return false;
  }
  return true;
}

[[nodiscard]] bool tail_file_lines(const std::filesystem::path &path,
                                   std::size_t lines, std::string *out,
                                   std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "tail output pointer is null";
    return false;
  }
  *out = "";
  std::error_code ec{};
  if (!std::filesystem::exists(path, ec)) {
    if (ec) {
      if (error)
        *error = "cannot access log file: " + path.string();
      return false;
    }
    return true;
  }
  std::string text{};
  if (!cuwacunu::hero::runtime::read_text_file(path, &text, error))
    return false;
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
  if (count <= lines)
    pos = 0;
  *out = text.substr(pos);
  return true;
}

[[nodiscard]] bool
read_kv_file(const std::filesystem::path &path,
             std::unordered_map<std::string, std::string> *out,
             std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "kv output pointer is null";
    return false;
  }
  out->clear();
  std::string text{};
  if (!cuwacunu::hero::runtime::read_text_file(path, &text, error))
    return false;
  std::istringstream in(text);
  std::string line{};
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(line);
    if (trimmed.empty())
      continue;
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos)
      continue;
    (*out)[trim_ascii(trimmed.substr(0, eq))] =
        trim_ascii(trimmed.substr(eq + 1));
  }
  return true;
}

[[nodiscard]] std::string extract_logged_hash_value(std::string_view text,
                                                    std::string_view key) {
  const std::size_t pos = text.rfind(key);
  if (pos == std::string::npos)
    return {};
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

[[nodiscard]] std::vector<std::string>
unique_nonempty_strings(std::vector<std::string> values) {
  for (std::string &value : values)
    value = trim_ascii(value);
  values.erase(
      std::remove_if(values.begin(), values.end(),
                     [](const std::string &value) { return value.empty(); }),
      values.end());
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  return values;
}

[[nodiscard]] std::vector<run_manifest_hint_t>
collect_run_manifest_hints(const app_context_t &app,
                           const std::vector<std::string> &binding_ids,
                           std::size_t per_binding_limit) {
  const std::filesystem::path runtime_root =
      app.defaults.marshal_root.parent_path();
  const std::filesystem::path runs_root =
      runtime_root / ".hashimyei" / "_meta" / "runs";
  std::vector<run_manifest_hint_t> out{};
  std::error_code ec{};
  if (!std::filesystem::exists(runs_root, ec) || ec)
    return out;

  for (const std::string &binding_id : binding_ids) {
    std::vector<run_manifest_hint_t> binding_rows{};
    for (const auto &it : std::filesystem::directory_iterator(runs_root, ec)) {
      if (ec)
        break;
      if (!it.is_directory(ec) || ec)
        continue;
      const std::string dirname = it.path().filename().string();
      if (dirname.find("." + binding_id + ".") == std::string::npos)
        continue;
      const std::filesystem::path manifest = it.path() / "run.manifest.v2.kv";
      if (!std::filesystem::exists(manifest, ec) || ec)
        continue;
      std::unordered_map<std::string, std::string> kv{};
      std::string ignored{};
      if (!read_kv_file(manifest, &kv, &ignored))
        continue;
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
              [](const auto &lhs, const auto &rhs) {
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
    const std::vector<std::string> &contract_hash_candidates) {
  std::ostringstream view_queries_json;
  view_queries_json << "[";
  for (std::size_t i = 0; i < contract_hash_candidates.size(); ++i) {
    if (i != 0)
      view_queries_json << ",";
    view_queries_json
        << "{"
        << "\"tool\":\"hero.lattice.get_view\","
        << "\"arguments\":{"
        << "\"view_kind\":\"family_evaluation_report\","
        << "\"canonical_path\":" << json_quote("<family canonical_path>")
        << ",\"contract_hash\":" << json_quote(contract_hash_candidates[i])
        << "},"
        << "\"when_to_use\":"
        << json_quote("Use for semantic family-level evaluation evidence "
                      "once you know the family canonical_path selector.")
        << ",\"notes\":"
        << json_quote("family_evaluation_report requires a family "
                      "canonical_path selector plus contract_hash. If the "
                      "family selector is unknown, discover it with "
                      "hero.lattice.list_facts first.")
        << "}";
  }
  view_queries_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"preferred_evidence_order\":[" << json_quote("input_checkpoint")
      << "," << json_quote("hero.lattice.get_view_or_get_fact") << ","
      << json_quote("hero.runtime_operational_debugging") << ","
      << json_quote("direct_file_reads_as_fallback") << "],"
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
             "in the current catalog before choosing one concrete fact/view "
             "query.")
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
             "For family_evaluation_report, canonical_path should be the "
             "family "
             "selector without a hashimyei suffix. If you do not already know "
             "that selector from the objective, use hero.lattice.list_facts to "
             "discover it.")
      << "}";
  return out.str();
}

[[nodiscard]] bool rewrite_campaign_imports_absolute(
    const std::filesystem::path &source_campaign_path,
    std::string_view input_text, std::string *out_text, std::string *error) {
  if (error)
    error->clear();
  if (!out_text) {
    if (error)
      *error = "campaign snapshot output pointer is null";
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

    const auto rewrite_import = [&](std::string_view token,
                                    const std::string &current_line) {
      std::size_t token_pos = current_line.find(token);
      if (token_pos == std::string::npos)
        return current_line;
      const std::size_t quote_begin =
          current_line.find('"', token_pos + token.size());
      if (quote_begin == std::string::npos)
        return current_line;
      const std::size_t quote_end = current_line.find('"', quote_begin + 1);
      if (quote_end == std::string::npos)
        return current_line;
      const std::filesystem::path raw_path(
          current_line.substr(quote_begin + 1, quote_end - quote_begin - 1));
      if (raw_path.is_absolute())
        return current_line;
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
      line = rewrite_import("MARSHAL", line);
    }
    if (!first)
      out << "\n";
    first = false;
    out << line;
    in_block_comment = cuwacunu::hero::wave_contract_binding_runtime::detail::
        next_block_comment_state(line, in_block_comment);
    if (end == std::string::npos)
      break;
    pos = end + 1;
  }
  *out_text = out.str();
  return true;
}

[[nodiscard]] bool decode_campaign_snapshot(
    const app_context_t &app, const std::string &campaign_dsl_path,
    cuwacunu::camahjucunu::iitepi_campaign_instruction_t *out_instruction,
    std::string *error) {
  if (error)
    error->clear();
  if (!out_instruction) {
    if (error)
      *error = "campaign decode output pointer is null";
    return false;
  }
  std::string grammar_text{};
  if (!cuwacunu::hero::runtime::read_text_file(
          app.defaults.campaign_grammar_path, &grammar_text, error)) {
    return false;
  }
  std::string campaign_text{};
  if (!cuwacunu::hero::runtime::read_text_file(campaign_dsl_path,
                                               &campaign_text, error)) {
    return false;
  }
  try {
    *out_instruction =
        cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
            grammar_text, campaign_text);
  } catch (const std::exception &e) {
    if (error) {
      *error = "failed to decode campaign DSL snapshot '" + campaign_dsl_path +
               "': " + e.what();
    }
    return false;
  }
  return true;
}

[[nodiscard]] std::size_t completed_run_count_for_campaign(
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign) {
  if (trim_ascii(campaign.state) == "exited")
    return campaign.run_bind_ids.size();
  if (!campaign.current_run_index.has_value())
    return 0;
  return std::min<std::size_t>(
      static_cast<std::size_t>(*campaign.current_run_index),
      campaign.run_bind_ids.size());
}

[[nodiscard]] std::size_t attempted_run_count_for_campaign(
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign) {
  if (trim_ascii(campaign.state) == "exited")
    return campaign.run_bind_ids.size();
  if (!campaign.current_run_index.has_value())
    return 0;
  return std::min<std::size_t>(
      static_cast<std::size_t>(*campaign.current_run_index) + 1,
      campaign.run_bind_ids.size());
}

[[nodiscard]] bool collect_marshal_run_plan_progress(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    const cuwacunu::camahjucunu::iitepi_campaign_instruction_t &instruction,
    const cuwacunu::hero::runtime::runtime_campaign_record_t *campaign_hint,
    marshal_run_plan_progress_t *out, std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "run-plan progress output pointer is null";
    return false;
  }
  *out = marshal_run_plan_progress_t{};

  out->declared_bind_ids.reserve(instruction.binds.size());
  for (const auto &bind : instruction.binds) {
    const std::string bind_id = trim_ascii(bind.id);
    if (!bind_id.empty())
      out->declared_bind_ids.push_back(bind_id);
  }
  out->default_run_bind_ids.reserve(instruction.runs.size());
  for (const auto &run : instruction.runs) {
    const std::string bind_id = trim_ascii(run.bind_ref);
    if (!bind_id.empty())
      out->default_run_bind_ids.push_back(bind_id);
  }

  const std::filesystem::path normalized_campaign_source =
      std::filesystem::path(loop.campaign_dsl_path).lexically_normal();
  for (const std::string &campaign_cursor : loop.campaign_cursors) {
    cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
    if (campaign_hint != nullptr &&
        trim_ascii(campaign_hint->campaign_cursor) ==
            trim_ascii(campaign_cursor)) {
      campaign = *campaign_hint;
    } else if (!read_runtime_campaign_direct(app, campaign_cursor, &campaign,
                                             error)) {
      return false;
    }
    if (std::filesystem::path(campaign.source_campaign_dsl_path)
            .lexically_normal() != normalized_campaign_source) {
      continue;
    }

    const std::size_t attempted_count =
        attempted_run_count_for_campaign(campaign);
    const std::size_t completed_count =
        completed_run_count_for_campaign(campaign);
    for (std::size_t i = 0; i < attempted_count; ++i) {
      const std::string bind_id = trim_ascii(campaign.run_bind_ids[i]);
      if (!bind_id.empty())
        out->attempted_run_bind_ids.push_back(bind_id);
    }
    if (!out->ordered_prefix_valid)
      continue;
    for (std::size_t i = 0; i < completed_count; ++i) {
      const std::string bind_id = trim_ascii(campaign.run_bind_ids[i]);
      if (bind_id.empty())
        continue;
      if (out->completed_run_bind_ids.size() >=
          out->default_run_bind_ids.size()) {
        out->ordered_prefix_valid = false;
        break;
      }
      if (bind_id !=
          out->default_run_bind_ids[out->completed_run_bind_ids.size()]) {
        out->ordered_prefix_valid = false;
        break;
      }
      out->completed_run_bind_ids.push_back(bind_id);
    }
  }

  if (out->ordered_prefix_valid) {
    for (std::size_t i = out->completed_run_bind_ids.size();
         i < out->default_run_bind_ids.size(); ++i) {
      out->pending_run_bind_ids.push_back(out->default_run_bind_ids[i]);
    }
    if (!out->pending_run_bind_ids.empty()) {
      out->next_pending_bind_id = out->pending_run_bind_ids.front();
    }
  } else {
    out->pending_run_bind_ids = out->default_run_bind_ids;
  }
  return true;
}

[[nodiscard]] std::string
marshal_run_plan_progress_to_json(const marshal_run_plan_progress_t &progress) {
  std::ostringstream out;
  out << "{"
      << "\"declared_bind_ids\":"
      << json_array_from_strings(progress.declared_bind_ids)
      << ",\"default_run_bind_ids\":"
      << json_array_from_strings(progress.default_run_bind_ids)
      << ",\"completed_run_bind_ids\":"
      << json_array_from_strings(progress.completed_run_bind_ids)
      << ",\"attempted_run_bind_ids\":"
      << json_array_from_strings(progress.attempted_run_bind_ids)
      << ",\"pending_run_bind_ids\":"
      << json_array_from_strings(progress.pending_run_bind_ids)
      << ",\"ordered_prefix_valid\":"
      << bool_json(progress.ordered_prefix_valid)
      << ",\"next_pending_bind_id\":";
  if (trim_ascii(progress.next_pending_bind_id).empty()) {
    out << "null";
  } else {
    out << json_quote(progress.next_pending_bind_id);
  }
  out << "}";
  return out.str();
}

[[nodiscard]] bool decode_marshal_objective_snapshot(
    const app_context_t &app, const std::string &marshal_objective_dsl_path,
    marshal_objective_spec_t *out_spec, std::string *error) {
  if (error)
    error->clear();
  if (!out_spec) {
    if (error)
      *error = "marshal objective decode output pointer is null";
    return false;
  }
  *out_spec = marshal_objective_spec_t{};

  std::string grammar_text{};
  if (!cuwacunu::hero::runtime::read_text_file(
          app.defaults.marshal_objective_grammar_path, &grammar_text, error)) {
    return false;
  }
  std::string objective_text{};
  if (!cuwacunu::hero::runtime::read_text_file(marshal_objective_dsl_path,
                                               &objective_text, error)) {
    return false;
  }

  try {
    const auto decoded =
        cuwacunu::camahjucunu::dsl::decode_marshal_objective_from_dsl(
            grammar_text, objective_text);
    out_spec->campaign_dsl_path = trim_ascii(decoded.campaign_dsl_path);
    out_spec->objective_md_path = trim_ascii(decoded.objective_md_path);
    out_spec->guidance_md_path = trim_ascii(decoded.guidance_md_path);
    out_spec->objective_name = trim_ascii(decoded.objective_name);
    out_spec->marshal_session_id = trim_ascii(decoded.marshal_session_id);
  } catch (const std::exception &e) {
    if (error) {
      *error = "failed to decode marshal objective DSL '" +
               marshal_objective_dsl_path + "': " + e.what();
    }
    return false;
  }

  if (out_spec->campaign_dsl_path.empty()) {
    if (error) {
      *error = "marshal objective '" + marshal_objective_dsl_path +
               "' is missing required campaign_dsl_path";
    }
    return false;
  }
  if (out_spec->objective_md_path.empty()) {
    if (error) {
      *error = "marshal objective '" + marshal_objective_dsl_path +
               "' is missing required objective_md_path";
    }
    return false;
  }
  if (out_spec->guidance_md_path.empty()) {
    if (error) {
      *error = "marshal objective '" + marshal_objective_dsl_path +
               "' is missing required guidance_md_path";
    }
    return false;
  }
  return true;
}

[[nodiscard]] std::filesystem::path
resolve_requested_marshal_objective_source_path(
    const app_context_t &app, std::string requested_marshal_objective_dsl_path,
    std::string *error) {
  if (error)
    error->clear();
  requested_marshal_objective_dsl_path =
      trim_ascii(std::move(requested_marshal_objective_dsl_path));
  const std::filesystem::path source_marshal_objective_dsl_path =
      requested_marshal_objective_dsl_path.empty()
          ? resolve_default_marshal_objective_dsl_path(app.global_config_path,
                                                       error)
          : std::filesystem::path(resolve_path_from_base_folder(
                app.global_config_path.parent_path().string(),
                requested_marshal_objective_dsl_path));
  if (source_marshal_objective_dsl_path.empty())
    return {};

  std::error_code ec{};
  if (!std::filesystem::exists(source_marshal_objective_dsl_path, ec) ||
      !std::filesystem::is_regular_file(source_marshal_objective_dsl_path,
                                        ec)) {
    if (error) {
      *error = "marshal objective DSL does not exist: " +
               source_marshal_objective_dsl_path.string();
    }
    return {};
  }
  if (lowercase_copy(source_marshal_objective_dsl_path.extension().string()) !=
      ".dsl") {
    if (error) {
      *error = "marshal objective path must target a .dsl file: " +
               source_marshal_objective_dsl_path.string();
    }
    return {};
  }
  return source_marshal_objective_dsl_path;
}

[[nodiscard]] bool resolve_marshal_objective_member_source_path(
    const std::filesystem::path &source_marshal_objective_dsl_path,
    std::string_view raw_path, std::string_view field_name,
    std::filesystem::path *out_path, std::string *error) {
  if (error)
    error->clear();
  if (!out_path) {
    if (error)
      *error = "marshal objective member output pointer is null";
    return false;
  }
  out_path->clear();
  const std::string trimmed = trim_ascii(raw_path);
  if (trimmed.empty()) {
    if (error) {
      *error = "marshal objective '" +
               source_marshal_objective_dsl_path.string() +
               "' is missing required " + std::string(field_name);
    }
    return false;
  }

  std::filesystem::path resolved(trimmed);
  if (!resolved.is_absolute()) {
    resolved = (source_marshal_objective_dsl_path.parent_path() / resolved)
                   .lexically_normal();
  }
  const bool inside_objective_bundle =
      path_is_within(source_marshal_objective_dsl_path.parent_path(), resolved);
  const bool allow_shared_guidance =
      field_name == "guidance_md_path" &&
      path_is_within(source_marshal_objective_dsl_path.parent_path()
                         .parent_path()
                         .parent_path(),
                     resolved);
  if (!inside_objective_bundle && !allow_shared_guidance) {
    if (error) {
      *error = "marshal objective field " + std::string(field_name) +
               " escapes its objective bundle: " + resolved.string();
    }
    return false;
  }
  std::error_code ec{};
  if (!std::filesystem::exists(resolved, ec) ||
      !std::filesystem::is_regular_file(resolved, ec)) {
    if (error) {
      *error = "marshal objective field " + std::string(field_name) +
               " does not exist: " + resolved.string();
    }
    return false;
  }
  *out_path = resolved;
  return true;
}

[[nodiscard]] marshal_session_workspace_context_t
make_marshal_session_workspace_context(const app_context_t &app) {
  marshal_session_workspace_context_t context{};
  context.global_config_path = app.global_config_path;
  context.self_binary_path = app.self_binary_path;
  context.marshal_root = app.defaults.marshal_root;
  context.repo_root = app.defaults.repo_root;
  context.config_scope_root = app.defaults.config_scope_root;
  context.runtime_hero_binary = app.defaults.runtime_hero_binary;
  context.config_hero_binary = app.defaults.config_hero_binary;
  context.lattice_hero_binary = app.defaults.lattice_hero_binary;
  context.human_hero_binary = app.defaults.human_hero_binary;
  context.human_operator_identities = app.defaults.human_operator_identities;
  context.marshal_codex_binary = app.defaults.marshal_codex_binary;
  context.tail_default_lines = app.defaults.tail_default_lines;
  context.poll_interval_ms = app.defaults.poll_interval_ms;
  context.marshal_codex_timeout_sec = app.defaults.marshal_codex_timeout_sec;
  return context;
}

[[nodiscard]] bool load_marshal_session_workspace_context(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    marshal_session_workspace_context_t *out, std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error) {
      *error = "marshal session workspace context output pointer is null";
    }
    return false;
  }
  *out = make_marshal_session_workspace_context(app);

  const std::filesystem::path session_root =
      trim_ascii(session.session_root).empty()
          ? cuwacunu::hero::marshal::marshal_session_dir(
                app.defaults.marshal_root, session.marshal_session_id)
          : std::filesystem::path(session.session_root);
  if (!session_root.empty()) {
    out->marshal_root = session_root.parent_path();
  }

  const std::filesystem::path hero_marshal_dsl_path =
      trim_ascii(session.hero_marshal_dsl_path).empty()
          ? cuwacunu::hero::marshal::marshal_session_hero_marshal_dsl_path(
                out->marshal_root, session.marshal_session_id)
          : std::filesystem::path(session.hero_marshal_dsl_path);
  std::error_code ec{};
  if (!std::filesystem::exists(hero_marshal_dsl_path, ec) ||
      !std::filesystem::is_regular_file(hero_marshal_dsl_path, ec)) {
    if (error) {
      *error = "marshal session is missing hero.marshal.dsl: " +
               hero_marshal_dsl_path.string();
    }
    return false;
  }

  marshal_defaults_t resolved{};
  if (!load_marshal_defaults(hero_marshal_dsl_path, app.global_config_path,
                             &resolved, error)) {
    return false;
  }
  out->repo_root = resolved.repo_root;
  out->config_scope_root = resolved.config_scope_root;
  out->runtime_hero_binary = resolved.runtime_hero_binary;
  out->config_hero_binary = resolved.config_hero_binary;
  out->lattice_hero_binary = resolved.lattice_hero_binary;
  out->human_hero_binary = resolved.human_hero_binary;
  out->human_operator_identities = resolved.human_operator_identities;
  out->marshal_codex_binary = resolved.marshal_codex_binary;
  out->tail_default_lines = resolved.tail_default_lines;
  out->poll_interval_ms = resolved.poll_interval_ms;
  out->marshal_codex_timeout_sec = resolved.marshal_codex_timeout_sec;
  return true;
}

[[nodiscard]] std::filesystem::path
runtime_tool_io_dir(const app_context_t &app) {
  return app.defaults.marshal_root / ".runtime_io";
}

[[nodiscard]] std::filesystem::path
human_tool_io_dir(const app_context_t &app) {
  return app.defaults.marshal_root / ".human_io";
}

[[nodiscard]] std::string make_tool_io_basename_(std::string_view prefix,
                                                 std::string_view stem) {
  const std::uint64_t started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  const std::uint64_t salt = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  return std::string(prefix) + "." +
         cuwacunu::hero::runtime::base36_lower_u64(started_at_ms) + "." +
         cuwacunu::hero::runtime::base36_lower_u64(
             static_cast<std::uint64_t>(::getpid())) +
         "." + cuwacunu::hero::runtime::base36_lower_u64(salt).substr(0, 6) +
         "." + std::string(stem);
}

[[nodiscard]] std::filesystem::path
make_runtime_tool_io_path(const app_context_t &app, std::string_view stem) {
  return runtime_tool_io_dir(app) / make_tool_io_basename_("rt", stem);
}

[[nodiscard]] std::filesystem::path
make_human_tool_io_path(const app_context_t &app, std::string_view stem) {
  return human_tool_io_dir(app) / make_tool_io_basename_("h", stem);
}

void cleanup_runtime_tool_io(const std::filesystem::path &stdin_path,
                             const std::filesystem::path &stdout_path,
                             const std::filesystem::path &stderr_path) {
  std::error_code ec{};
  (void)std::filesystem::remove(stdin_path, ec);
  (void)std::filesystem::remove(stdout_path, ec);
  (void)std::filesystem::remove(stderr_path, ec);
}

void cleanup_human_tool_io(const std::filesystem::path &stdin_path,
                           const std::filesystem::path &stdout_path,
                           const std::filesystem::path &stderr_path) {
  std::error_code ec{};
  (void)std::filesystem::remove(stdin_path, ec);
  (void)std::filesystem::remove(stdout_path, ec);
  (void)std::filesystem::remove(stderr_path, ec);
}

[[nodiscard]] bool
call_runtime_tool(const app_context_t &app,
                  const std::filesystem::path &runtime_binary,
                  const std::string &tool_name, std::string arguments_json,
                  std::string *out_structured, std::string *error) {
  if (error)
    error->clear();
  if (!out_structured) {
    if (error)
      *error = "runtime structured output pointer is null";
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

  const std::vector<std::string> argv{runtime_binary.string(),
                                      "--global-config",
                                      app.global_config_path.string(),
                                      "--tool",
                                      tool_name,
                                      "--args-json",
                                      arguments_json};
  int exit_code = -1;
  std::string invoke_error{};
  const bool invoked = run_command_with_stdio_and_timeout(
      argv, stdin_path, stdout_path, stderr_path, nullptr, nullptr,
      kRuntimeToolTimeoutSec, nullptr, &exit_code, &invoke_error);

  std::string stdout_text{};
  std::string stderr_text{};
  std::string ignored{};
  (void)cuwacunu::hero::runtime::read_text_file(stdout_path, &stdout_text,
                                                &ignored);
  (void)cuwacunu::hero::runtime::read_text_file(stderr_path, &stderr_text,
                                                &ignored);
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

[[nodiscard]] bool call_human_tool(const app_context_t &app,
                                   const std::string &tool_name,
                                   std::string arguments_json,
                                   std::string *out_structured,
                                   std::string *error) {
  if (error)
    error->clear();
  if (!out_structured) {
    if (error)
      *error = "human structured output pointer is null";
    return false;
  }
  *out_structured = "";

  std::error_code ec{};
  std::filesystem::create_directories(human_tool_io_dir(app), ec);
  if (ec) {
    if (error) {
      *error =
          "cannot create human tool io dir: " + human_tool_io_dir(app).string();
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

  const std::vector<std::string> argv{app.defaults.human_hero_binary.string(),
                                      "--global-config",
                                      app.global_config_path.string(),
                                      "--tool",
                                      tool_name,
                                      "--args-json",
                                      arguments_json};
  int exit_code = -1;
  std::string invoke_error{};
  const bool invoked = run_command_with_stdio_and_timeout(
      argv, stdin_path, stdout_path, stderr_path, nullptr, nullptr,
      kHumanToolTimeoutSec, nullptr, &exit_code, &invoke_error);

  std::string stdout_text{};
  std::string stderr_text{};
  std::string ignored{};
  (void)cuwacunu::hero::runtime::read_text_file(stdout_path, &stdout_text,
                                                &ignored);
  (void)cuwacunu::hero::runtime::read_text_file(stderr_path, &stderr_text,
                                                &ignored);
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
                   : ("human tool call produced no stdout: " + tool_name);
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
                   : ("human tool exited non-zero: " + tool_name);
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool initialize_marshal_session_record(
    const app_context_t &app,
    const std::filesystem::path &source_marshal_objective_dsl_path,
    const marshal_start_session_overrides_t &start_overrides,
    cuwacunu::hero::marshal::marshal_session_record_t *out,
    std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "marshal session output pointer is null";
    return false;
  }
  *out = cuwacunu::hero::marshal::marshal_session_record_t{};

  std::error_code ec{};
  std::filesystem::create_directories(app.defaults.marshal_root, ec);
  if (ec) {
    if (error) {
      *error = "cannot create .marshal_hero root: " +
               app.defaults.marshal_root.string();
    }
    return false;
  }

  marshal_objective_spec_t marshal_objective_spec{};
  if (!decode_marshal_objective_snapshot(
          app, source_marshal_objective_dsl_path.string(),
          &marshal_objective_spec, error)) {
    return false;
  }
  const std::string marshal_session_id =
      trim_ascii(marshal_objective_spec.marshal_session_id).empty()
          ? cuwacunu::hero::marshal::make_marshal_session_id(
                app.defaults.marshal_root)
          : trim_ascii(marshal_objective_spec.marshal_session_id);
  if (!is_valid_marshal_session_id(marshal_session_id)) {
    if (error) {
      *error = "marshal objective marshal_session_id is invalid; use only "
               "[A-Za-z0-9._-]: " +
               marshal_session_id;
    }
    return false;
  }
  std::filesystem::path source_campaign_path{};
  if (!resolve_marshal_objective_member_source_path(
          source_marshal_objective_dsl_path,
          marshal_objective_spec.campaign_dsl_path, "campaign_dsl_path",
          &source_campaign_path, error)) {
    return false;
  }
  std::filesystem::path source_marshal_objective_md_path{};
  if (!resolve_marshal_objective_member_source_path(
          source_marshal_objective_dsl_path,
          marshal_objective_spec.objective_md_path, "objective_md_path",
          &source_marshal_objective_md_path, error)) {
    return false;
  }
  std::filesystem::path source_marshal_guidance_md_path{};
  if (!resolve_marshal_objective_member_source_path(
          source_marshal_objective_dsl_path,
          marshal_objective_spec.guidance_md_path, "guidance_md_path",
          &source_marshal_guidance_md_path, error)) {
    return false;
  }
  cuwacunu::camahjucunu::iitepi_campaign_instruction_t ignored_campaign{};
  if (!decode_campaign_snapshot(app, source_campaign_path.string(),
                                &ignored_campaign, error)) {
    return false;
  }
  {
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        existing_loops{};
    std::string scan_error{};
    if (!list_marshal_sessions_reconciled(app, &existing_loops, &scan_error)) {
      if (error)
        *error = scan_error;
      return false;
    }
    const std::filesystem::path normalized_source_marshal_objective =
        source_marshal_objective_dsl_path.lexically_normal();
    for (const auto &existing : existing_loops) {
      if (cuwacunu::hero::marshal::is_marshal_session_terminal_state(
              existing.lifecycle)) {
        continue;
      }
      if (std::filesystem::path(existing.source_marshal_objective_dsl_path)
              .lexically_normal() == normalized_source_marshal_objective) {
        if (error) {
          const bool resumable_idle =
              cuwacunu::hero::marshal::marshal_session_is_review_ready(
                  existing) &&
              existing.finish_reason == "interrupted";
          *error = (resumable_idle
                        ? "another resumable marshal session already owns this "
                          "marshal objective: "
                        : "another active marshal session already owns this "
                          "marshal objective: ") +
                   existing.marshal_session_id;
        }
        return false;
      }
    }
  }
  const std::filesystem::path session_root =
      cuwacunu::hero::marshal::marshal_session_dir(app.defaults.marshal_root,
                                                   marshal_session_id);
  if (std::filesystem::exists(session_root, ec) && !ec) {
    if (error)
      *error = "marshal session already exists: " + marshal_session_id;
    return false;
  }
  std::filesystem::create_directories(session_root, ec);
  if (ec) {
    if (error)
      *error = "cannot create marshal session dir: " + session_root.string();
    return false;
  }

  const marshal_session_workspace_context_t workspace_context =
      make_marshal_session_workspace_context(app);
  const std::filesystem::path objective_root =
      source_marshal_objective_dsl_path.parent_path().lexically_normal();

  cuwacunu::hero::marshal::marshal_session_record_t loop{};
  loop.marshal_session_id = marshal_session_id;
  loop.lifecycle = "bootstrapping";
  loop.activity = "bootstrapping";
  loop.status_detail = "initializing marshal session";
  loop.work_gate = "open";
  loop.finish_reason = "none";
  loop.objective_name =
      trim_ascii(marshal_objective_spec.objective_name).empty()
          ? derive_marshal_session_objective_name(
                source_marshal_objective_dsl_path)
          : trim_ascii(marshal_objective_spec.objective_name);
  loop.global_config_path = app.global_config_path.string();
  loop.source_marshal_objective_dsl_path =
      source_marshal_objective_dsl_path.string();
  loop.source_campaign_dsl_path = source_campaign_path.string();
  loop.source_marshal_objective_md_path =
      source_marshal_objective_md_path.string();
  loop.source_marshal_guidance_md_path =
      source_marshal_guidance_md_path.string();
  loop.session_root = session_root.string();
  loop.objective_root = objective_root.string();
  loop.campaign_dsl_path = source_campaign_path.lexically_normal().string();
  loop.marshal_objective_dsl_path =
      cuwacunu::hero::marshal::marshal_session_objective_dsl_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  loop.marshal_objective_md_path =
      cuwacunu::hero::marshal::marshal_session_objective_md_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  loop.marshal_guidance_md_path =
      cuwacunu::hero::marshal::marshal_session_guidance_md_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  loop.hero_marshal_dsl_path =
      cuwacunu::hero::marshal::marshal_session_hero_marshal_dsl_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  loop.config_policy_path =
      cuwacunu::hero::marshal::marshal_session_config_policy_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  loop.briefing_path = cuwacunu::hero::marshal::marshal_session_briefing_path(
                           app.defaults.marshal_root, marshal_session_id)
                           .string();
  loop.memory_path = cuwacunu::hero::marshal::marshal_session_memory_path(
                         app.defaults.marshal_root, marshal_session_id)
                         .string();
  loop.human_request_path =
      cuwacunu::hero::marshal::marshal_session_human_request_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  loop.events_path = cuwacunu::hero::marshal::marshal_session_events_path(
                         app.defaults.marshal_root, marshal_session_id)
                         .string();
  loop.turns_path = cuwacunu::hero::marshal::marshal_session_turns_path(
                        app.defaults.marshal_root, marshal_session_id)
                        .string();
  loop.codex_stdout_path =
      cuwacunu::hero::marshal::marshal_session_codex_stdout_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  loop.codex_stderr_path =
      cuwacunu::hero::marshal::marshal_session_codex_stderr_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  {
    const std::filesystem::path resolved_codex_binary =
        resolve_marshal_codex_binary_path(
            app.defaults.repo_root, app.defaults.marshal_codex_binary.string());
    loop.resolved_marshal_codex_binary =
        resolved_codex_binary.empty()
            ? app.defaults.marshal_codex_binary.string()
            : resolved_codex_binary.string();
  }
  loop.resolved_marshal_codex_model =
      trim_ascii(start_overrides.marshal_codex_model).empty()
          ? app.defaults.marshal_codex_model
          : trim_ascii(start_overrides.marshal_codex_model);
  loop.resolved_marshal_codex_reasoning_effort =
      trim_ascii(start_overrides.marshal_codex_reasoning_effort).empty()
          ? app.defaults.marshal_codex_reasoning_effort
          : trim_ascii(start_overrides.marshal_codex_reasoning_effort);
  loop.started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop.updated_at_ms = loop.started_at_ms;
  loop.max_campaign_launches = app.defaults.marshal_max_campaign_launches;
  loop.remaining_campaign_launches = app.defaults.marshal_max_campaign_launches;
  loop.authority_scope = "objective_only";

  if (!write_marshal_session_bootstrap_files(
          workspace_context, source_marshal_objective_dsl_path,
          source_marshal_objective_md_path, source_marshal_guidance_md_path,
          loop, error)) {
    return false;
  }
  if (!write_marshal_session(app, loop, error))
    return false;
  if (!append_marshal_session_event(
          &loop, "marshal", "session.started",
          "marshal session initialized from marshal objective source", error)) {
    return false;
  }
  *out = std::move(loop);
  return true;
}

void append_memory_note(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t turn_index, std::string_view note) {
  const std::string trimmed = trim_ascii(note);
  if (trimmed.empty())
    return;
  std::ostringstream out;
  out << "\n\n## Checkpoint " << turn_index << "\n\n" << trimmed << "\n";
  std::string ignored{};
  (void)cuwacunu::hero::runtime::append_text_file(loop.memory_path, out.str(),
                                                  &ignored);
}

void append_warning_text(std::string *dst, std::string_view warning) {
  if (!dst)
    return;
  const std::string trimmed = trim_ascii(warning);
  if (trimmed.empty())
    return;
  if (!dst->empty())
    dst->append("; ");
  dst->append(trimmed);
}

void append_structured_warnings(const std::string &structured_json,
                                std::string_view prefix,
                                std::vector<std::string> *out) {
  if (!out)
    return;
  std::vector<std::string> warnings{};
  if (!extract_json_string_array_field(structured_json, "warnings",
                                       &warnings)) {
    return;
  }
  for (const std::string &warning : warnings) {
    const std::string trimmed = trim_ascii(warning);
    if (trimmed.empty())
      continue;
    out->push_back(trim_ascii(std::string(prefix) + trimmed));
  }
}

void persist_marshal_session_warning_best_effort(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string_view warning) {
  if (!loop)
    return;
  const std::string trimmed = trim_ascii(warning);
  if (trimmed.empty())
    return;
  loop->last_warning = trimmed;
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  std::string ignored{};
  (void)write_marshal_session(app, *loop, &ignored);
  std::ostringstream note;
  note << "\n\n## Runtime Warning\n\n" << trimmed << "\n";
  (void)cuwacunu::hero::runtime::append_text_file(loop->memory_path, note.str(),
                                                  &ignored);
}

[[nodiscard]] bool error_mentions_timeout(std::string_view text) {
  return lowercase_copy(text).find("timed out") != std::string::npos;
}

[[nodiscard]] std::string build_resume_degraded_warning(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index, std::string_view failure_kind,
    std::string_view detail) {
  std::ostringstream out;
  out << "codex resume degraded: checkpoint=" << checkpoint_index
      << " kind=" << trim_ascii(failure_kind) << " fallback=fresh_checkpoint";
  const std::string prior_session_id = trim_ascii(loop.current_thread_id);
  if (!prior_session_id.empty()) {
    out << " prior_session_id=" << prior_session_id;
  }
  if (error_mentions_timeout(detail)) {
    out << " resume_timeout=true";
  }
  const std::string trimmed_detail = trim_ascii(detail);
  if (!trimmed_detail.empty()) {
    out << " detail=" << trimmed_detail;
  }
  out << "; stderr_log=" << marshal_session_codex_stderr_path(loop).string()
      << "; stdout_log=" << marshal_session_codex_stdout_path(loop).string();
  return out.str();
}

[[nodiscard]] bool persist_attempted_checkpoint_state(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::uint64_t checkpoint_index,
    const marshal_checkpoint_decision_t &decision,
    std::string_view checkpoint_warning, std::string *error) {
  if (error)
    error->clear();
  if (!loop) {
    if (error)
      *error = "checkpoint bookkeeping loop pointer is null";
    return false;
  }

  loop->checkpoint_count = std::max(loop->checkpoint_count, checkpoint_index);
  loop->last_codex_action = decision.intent;
  loop->last_intent_checkpoint_path =
      cuwacunu::hero::marshal::marshal_session_intent_checkpoint_path(
          app.defaults.marshal_root, loop->marshal_session_id, checkpoint_index)
          .string();
  loop->last_warning.clear();
  append_warning_text(&loop->last_warning, checkpoint_warning);
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop->finished_at_ms.reset();
  if (!write_marshal_session(app, *loop, error))
    return false;
  append_memory_note(*loop, checkpoint_index, decision.memory_note);
  return true;
}

[[nodiscard]] bool park_session_idle_after_checkpoint_failure(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::uint64_t checkpoint_index, std::string_view failure_detail,
    std::string *error) {
  if (error)
    error->clear();
  if (!loop) {
    if (error)
      *error = "checkpoint failure loop pointer is null";
    return false;
  }

  const std::string trimmed_detail = trim_ascii(failure_detail);
  std::ostringstream detail;
  detail << "planning checkpoint " << checkpoint_index
         << " failed after intent capture";
  if (!trimmed_detail.empty()) {
    detail << ": " << trimmed_detail;
  }
  detail << "; session parked for review so the operator can message it after "
            "adjusting objective files or tooling";

  loop->lifecycle = "live";
  loop->activity = "review";
  loop->status_detail = detail.str();
  loop->work_gate = "open";
  loop->finish_reason = "failed";
  loop->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop->updated_at_ms = *loop->finished_at_ms;
  clear_marshal_session_campaign_linkage(loop);
  append_warning_text(&loop->last_warning,
                      "checkpoint parked for review after intent capture; use "
                      "message_session once the underlying issue is addressed");
  if (!write_marshal_session(app, *loop, error))
    return false;
  std::string ignored{};
  (void)append_marshal_session_event(&loop, "checkpoint.failed",
                                     loop->status_detail, &ignored);
  return true;
}

[[nodiscard]] bool
collect_objective_hash_snapshot(const std::filesystem::path &objective_root,
                                objective_hash_snapshot_t *out,
                                std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "objective hash snapshot output pointer is null";
    return false;
  }
  out->clear();
  if (objective_root.empty()) {
    if (error)
      *error = "objective root is empty";
    return false;
  }

  std::error_code ec{};
  if (!std::filesystem::exists(objective_root, ec) ||
      !std::filesystem::is_directory(objective_root, ec)) {
    if (error) {
      *error = "objective root does not exist or is not a directory: " +
               objective_root.string();
    }
    return false;
  }

  std::vector<std::filesystem::path> files{};
  for (std::filesystem::recursive_directory_iterator it(objective_root, ec),
       end;
       it != end; it.increment(ec)) {
    if (ec) {
      if (error) {
        *error = "cannot walk objective root: " + objective_root.string();
      }
      return false;
    }
    if (!it->is_regular_file(ec) || ec) {
      ec.clear();
      continue;
    }
    files.push_back(it->path().lexically_normal());
  }
  std::sort(files.begin(), files.end());

  for (const auto &file : files) {
    const std::filesystem::path relative =
        file.lexically_relative(objective_root);
    if (relative.empty())
      continue;
    std::string sha256_hex{};
    if (!cuwacunu::hero::human::sha256_hex_file(file, &sha256_hex, error)) {
      return false;
    }
    (*out)[relative.generic_string()] = std::move(sha256_hex);
  }
  return true;
}

[[nodiscard]] bool
collect_objective_bundle_snapshot(const std::filesystem::path &objective_root,
                                  objective_bundle_snapshot_t *out_snapshot,
                                  std::string *error) {
  if (error)
    error->clear();
  if (!out_snapshot) {
    if (error)
      *error = "objective bundle snapshot output pointer is null";
    return false;
  }
  *out_snapshot = objective_bundle_snapshot_t{};
  if (objective_root.empty()) {
    if (error)
      *error = "objective root is empty";
    return false;
  }

  std::error_code ec{};
  if (!std::filesystem::exists(objective_root, ec) ||
      !std::filesystem::is_directory(objective_root, ec)) {
    if (error) {
      *error = "objective root does not exist or is not a directory: " +
               objective_root.string();
    }
    return false;
  }

  std::vector<std::filesystem::path> files{};
  for (std::filesystem::recursive_directory_iterator it(objective_root, ec),
       end;
       it != end; it.increment(ec)) {
    if (ec) {
      if (error) {
        *error = "cannot walk objective root: " + objective_root.string();
      }
      return false;
    }
    if (!it->is_regular_file(ec) || ec) {
      ec.clear();
      continue;
    }
    files.push_back(it->path().lexically_normal());
  }
  std::sort(files.begin(), files.end());

  for (const auto &file : files) {
    const std::filesystem::path relative =
        file.lexically_relative(objective_root);
    if (relative.empty())
      continue;
    const std::string relative_text = relative.generic_string();
    if (!should_embed_objective_bundle_relative_path(relative_text))
      continue;

    std::string file_text{};
    if (!cuwacunu::hero::runtime::read_text_file(file, &file_text, error)) {
      return false;
    }
    const std::size_t file_bytes = file_text.size();
    if (!out_snapshot->entries.empty() &&
        out_snapshot->embedded_bytes + file_bytes >
            kEmbeddedObjectiveBundleBytesCap) {
      out_snapshot->truncated = true;
      ++out_snapshot->omitted_files;
      continue;
    }
    out_snapshot->embedded_bytes += file_bytes;
    out_snapshot->entries.push_back(
        objective_bundle_entry_t{relative_text, std::move(file_text)});
  }
  return true;
}

[[nodiscard]] std::filesystem::path mutation_checkpoint_path_for(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index) {
  return cuwacunu::hero::marshal::marshal_session_mutation_checkpoint_path(
      app.defaults.marshal_root, loop.marshal_session_id, checkpoint_index);
}

[[nodiscard]] bool has_recorded_mutation_checkpoint(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index, std::filesystem::path *out_path = nullptr) {
  const std::filesystem::path mutation_path =
      mutation_checkpoint_path_for(app, loop, checkpoint_index);
  if (out_path)
    *out_path = mutation_path;
  std::error_code ec{};
  return std::filesystem::exists(mutation_path, ec) && !ec;
}

[[nodiscard]] bool write_mutation_checkpoint_summary(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::uint64_t checkpoint_index,
    const objective_hash_snapshot_t &before_snapshot,
    const objective_hash_snapshot_t &after_snapshot, bool *out_materialized,
    std::string *error) {
  if (error)
    error->clear();
  if (out_materialized)
    *out_materialized = false;
  if (!loop) {
    if (error)
      *error = "mutation checkpoint loop pointer is null";
    return false;
  }

  struct mutation_row_t {
    std::string path{};
    std::string change_kind{};
    std::string before_sha256_hex{};
    std::string after_sha256_hex{};
  };

  std::vector<mutation_row_t> rows{};
  auto before_it = before_snapshot.begin();
  auto after_it = after_snapshot.begin();
  while (before_it != before_snapshot.end() ||
         after_it != after_snapshot.end()) {
    if (after_it == after_snapshot.end() ||
        (before_it != before_snapshot.end() &&
         before_it->first < after_it->first)) {
      rows.push_back(
          mutation_row_t{before_it->first, "deleted", before_it->second, ""});
      ++before_it;
      continue;
    }
    if (before_it == before_snapshot.end() ||
        after_it->first < before_it->first) {
      rows.push_back(
          mutation_row_t{after_it->first, "created", "", after_it->second});
      ++after_it;
      continue;
    }
    if (before_it->second != after_it->second) {
      rows.push_back(mutation_row_t{before_it->first, "updated",
                                    before_it->second, after_it->second});
    }
    ++before_it;
    ++after_it;
  }

  const std::filesystem::path mutation_path =
      mutation_checkpoint_path_for(app, *loop, checkpoint_index);
  const std::filesystem::path latest_mutation_path =
      cuwacunu::hero::marshal::marshal_session_latest_mutation_checkpoint_path(
          app.defaults.marshal_root, loop->marshal_session_id);

  if (rows.empty()) {
    if (has_recorded_mutation_checkpoint(app, *loop, checkpoint_index)) {
      loop->last_mutation_checkpoint_path = mutation_path.string();
      std::string mutation_text{};
      std::string ignored{};
      if (cuwacunu::hero::runtime::read_text_file(mutation_path, &mutation_text,
                                                  &ignored) &&
          !mutation_text.empty()) {
        (void)cuwacunu::hero::runtime::write_text_file_atomic(
            latest_mutation_path, mutation_text, &ignored);
      }
    }
    return true;
  }

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(kMutationCheckpointSchemaV3) << ","
      << "\"marshal_session_id\":" << json_quote(loop->marshal_session_id)
      << ","
      << "\"checkpoint_index\":" << checkpoint_index << ","
      << "\"objective_root\":" << json_quote(loop->objective_root) << ","
      << "\"changed_file_count\":" << rows.size() << ","
      << "\"files\":[";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i != 0)
      out << ",";
    out << "{"
        << "\"path\":" << json_quote(rows[i].path) << ","
        << "\"change_kind\":" << json_quote(rows[i].change_kind) << ","
        << "\"before_sha256_hex\":" << json_quote(rows[i].before_sha256_hex)
        << ","
        << "\"after_sha256_hex\":" << json_quote(rows[i].after_sha256_hex)
        << "}";
  }
  out << "]}";

  if (!cuwacunu::hero::runtime::write_text_file_atomic(mutation_path, out.str(),
                                                       error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(latest_mutation_path,
                                                       out.str(), error)) {
    return false;
  }
  loop->last_mutation_checkpoint_path = mutation_path.string();
  if (out_materialized)
    *out_materialized = true;
  return true;
}

[[nodiscard]] bool materialize_human_request(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t turn_index, const marshal_checkpoint_decision_t &decision,
    std::string *error) {
  if (error)
    error->clear();
  std::ostringstream out;
  out << "# Human Request\n\n"
      << "Schema: " << kHumanRequestSchemaV3 << "\n"
      << "Marshal ID: " << loop.marshal_session_id << "\n"
      << "Checkpoint: " << turn_index << "\n"
      << "Pause Kind: "
      << (decision.intent == "pause_for_clarification" ? "clarification"
                                                       : "governance")
      << "\n";
  if (decision.intent == "pause_for_clarification") {
    out << "Clarification Request:\n"
        << decision.clarification_request << "\n\n";
  } else {
    out << "Governance Kind: " << decision.governance_kind << "\n"
        << "Governance Request:\n"
        << decision.governance_request << "\n\n";
  }
  out << "Current Authority Scope: " << loop.authority_scope << "\n"
      << "Remaining Campaign Launches: " << loop.remaining_campaign_launches
      << "\n\n"
      << "Reason:\n"
      << decision.reason << "\n\n"
      << "Last Intent: " << loop.last_codex_action << "\n\n";
  if (decision.intent == "request_governance" &&
      decision.governance_kind == "authority_expansion") {
    out << "Requested authority delta:\n"
        << "- allow_default_write: "
        << (decision.governance_allow_default_write ? "true" : "false")
        << "\n\n";
  } else if (decision.intent == "request_governance" &&
             decision.governance_kind == "launch_budget_expansion") {
    out << "Requested launch budget delta:\n"
        << "- additional_campaign_launches: "
        << decision.governance_additional_campaign_launches << "\n\n";
  }
  out << "Key files:\n"
      << "- Session manifest: "
      << cuwacunu::hero::marshal::marshal_session_manifest_path(
             app.defaults.marshal_root, loop.marshal_session_id)
             .string()
      << "\n"
      << "- Latest input checkpoint: "
      << cuwacunu::hero::marshal::marshal_session_latest_input_checkpoint_path(
             app.defaults.marshal_root, loop.marshal_session_id)
             .string()
      << "\n"
      << "- Latest intent checkpoint: "
      << cuwacunu::hero::marshal::marshal_session_latest_intent_checkpoint_path(
             app.defaults.marshal_root, loop.marshal_session_id)
             .string()
      << "\n"
      << "- Codex stdout: " << loop.codex_stdout_path << "\n"
      << "- Codex stderr: " << loop.codex_stderr_path << "\n"
      << "- Memory: " << loop.memory_path << "\n"
      << "- Events: " << loop.events_path << "\n";
  return cuwacunu::hero::runtime::write_text_file_atomic(
      loop.human_request_path, out.str(), error);
}

void append_human_resolution_note(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    const cuwacunu::hero::human::human_resolution_record_t &resolution,
    const std::filesystem::path &response_path,
    const std::filesystem::path &signature_path) {
  std::ostringstream out;
  out << "\n\n## Governance Resolution to Checkpoint "
      << resolution.checkpoint_index << "\n\n"
      << "- operator_id: " << resolution.operator_id << "\n"
      << "- resolved_at_ms: " << resolution.resolved_at_ms << "\n"
      << "- resolution_kind: " << resolution.resolution_kind << "\n"
      << "- governance_kind: " << resolution.governance_kind << "\n"
      << "- resolution_path: " << response_path.string() << "\n"
      << "- resolution_sig_path: " << signature_path.string() << "\n";
  if (resolution.resolution_kind == "grant") {
    out << "- grant.allow_default_write: "
        << (resolution.grant_allow_default_write ? "true" : "false") << "\n"
        << "- grant.additional_campaign_launches: "
        << resolution.grant_additional_campaign_launches << "\n";
  }
  out << "\nReason:\n" << trim_ascii(resolution.reason) << "\n";
  std::string ignored{};
  (void)cuwacunu::hero::runtime::append_text_file(loop.memory_path, out.str(),
                                                  &ignored);
}

[[nodiscard]] bool build_marshal_governance_followup_input_checkpoint_json(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index,
    const std::filesystem::path &prior_input_checkpoint_path,
    const std::filesystem::path &response_path,
    const std::filesystem::path &response_sig_path,
    const cuwacunu::hero::human::human_resolution_record_t &response,
    std::string *out_json, std::string *error) {
  if (error)
    error->clear();
  if (!out_json) {
    if (error)
      *error = "governance followup checkpoint output pointer is null";
    return false;
  }
  out_json->clear();

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  if (!decode_campaign_snapshot(app, loop.campaign_dsl_path, &instruction,
                                error)) {
    return false;
  }
  marshal_run_plan_progress_t run_plan_progress{};
  if (!collect_marshal_run_plan_progress(app, loop, instruction, nullptr,
                                         &run_plan_progress, error)) {
    return false;
  }

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(kInputCheckpointSchemaV3) << ","
      << "\"checkpoint_kind\":\"governance_followup\","
      << "\"checkpoint_index\":" << checkpoint_index
      << ",\"session\":" << marshal_session_to_json(loop)
      << ",\"run_plan_progress\":"
      << marshal_run_plan_progress_to_json(run_plan_progress)
      << ",\"prior_input_checkpoint_path\":"
      << json_quote(prior_input_checkpoint_path.string())
      << ",\"human_request_path\":" << json_quote(loop.human_request_path)
      << ",\"governance_resolution_path\":"
      << json_quote(response_path.string())
      << ",\"governance_resolution_sig_path\":"
      << json_quote(response_sig_path.string())
      << ",\"verified_governance_resolution\":{"
      << "\"operator_id\":" << json_quote(response.operator_id) << ","
      << "\"resolved_at_ms\":" << response.resolved_at_ms << ","
      << "\"resolution_kind\":" << json_quote(response.resolution_kind) << ","
      << "\"governance_kind\":" << json_quote(response.governance_kind) << ","
      << "\"reason\":" << json_quote(response.reason) << ","
      << "\"grant_delta\":{"
      << "\"allow_default_write\":"
      << bool_json(response.grant_allow_default_write) << ","
      << "\"additional_campaign_launches\":"
      << response.grant_additional_campaign_launches << "}"
      << "},\"memory_path\":" << json_quote(loop.memory_path)
      << ",\"marshal_objective_dsl_path\":"
      << json_quote(loop.marshal_objective_dsl_path)
      << ",\"marshal_objective_md_path\":"
      << json_quote(loop.marshal_objective_md_path)
      << ",\"marshal_guidance_md_path\":"
      << json_quote(loop.marshal_guidance_md_path)
      << ",\"hero_marshal_dsl_path\":" << json_quote(loop.hero_marshal_dsl_path)
      << ",\"config_policy_path\":" << json_quote(loop.config_policy_path)
      << ",\"briefing_path\":" << json_quote(loop.briefing_path)
      << ",\"mutable_objective_root\":" << json_quote(loop.objective_root)
      << ",\"objective_campaign_dsl_path\":"
      << json_quote(loop.campaign_dsl_path)
      << ",\"objective_root\":" << json_quote(loop.objective_root)
      << ",\"campaigns_root\":"
      << json_quote(campaigns_root_for_app(app).string())
      << ",\"marshal_root\":" << json_quote(app.defaults.marshal_root.string())
      << "}";
  *out_json = out.str();
  return true;
}

[[nodiscard]] bool parse_human_clarification_answer_json(
    const std::string &json, std::string_view expected_marshal_session_id,
    std::uint64_t expected_checkpoint_index, std::string *out_answer,
    std::string *error) {
  if (error)
    error->clear();
  if (!out_answer) {
    if (error)
      *error = "clarification answer output pointer is null";
    return false;
  }
  out_answer->clear();
  std::string schema{};
  (void)extract_json_string_field(json, "schema", &schema);
  if (!trim_ascii(schema).empty() &&
      trim_ascii(schema) != std::string(kHumanClarificationAnswerSchemaV3)) {
    if (error) {
      *error = "unsupported clarification answer schema: " + trim_ascii(schema);
    }
    return false;
  }
  std::string marshal_session_id{};
  if (!extract_json_string_field(json, "marshal_session_id",
                                 &marshal_session_id) ||
      trim_ascii(marshal_session_id).empty()) {
    if (error)
      *error = "clarification answer is missing marshal_session_id";
    return false;
  }
  const std::string expected_session_id =
      trim_ascii(expected_marshal_session_id);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id != expected_session_id) {
    if (error) {
      *error = "clarification answer marshal_session_id does not match paused "
               "session";
    }
    return false;
  }
  std::uint64_t checkpoint_index = 0;
  if (!extract_json_u64_field(json, "checkpoint_index", &checkpoint_index) ||
      checkpoint_index == 0) {
    if (error)
      *error = "clarification answer is missing checkpoint_index";
    return false;
  }
  if (checkpoint_index != expected_checkpoint_index) {
    if (error) {
      *error = "clarification answer checkpoint_index does not match paused "
               "checkpoint";
    }
    return false;
  }
  if (!extract_json_string_field(json, "answer", out_answer) ||
      trim_ascii(*out_answer).empty()) {
    if (error)
      *error = "clarification answer is missing non-empty answer";
    return false;
  }
  *out_answer = trim_ascii(*out_answer);
  return true;
}

[[nodiscard]] bool build_marshal_clarification_followup_checkpoint_json(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index,
    const std::filesystem::path &prior_input_checkpoint_path,
    const std::filesystem::path &answer_path, std::string_view answer,
    std::string *out_json, std::string *error) {
  if (error)
    error->clear();
  if (!out_json) {
    if (error) {
      *error = "clarification followup checkpoint output pointer is null";
    }
    return false;
  }
  out_json->clear();

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  if (!decode_campaign_snapshot(app, loop.campaign_dsl_path, &instruction,
                                error)) {
    return false;
  }
  marshal_run_plan_progress_t run_plan_progress{};
  if (!collect_marshal_run_plan_progress(app, loop, instruction, nullptr,
                                         &run_plan_progress, error)) {
    return false;
  }

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(kInputCheckpointSchemaV3) << ","
      << "\"checkpoint_kind\":\"clarification_followup\","
      << "\"checkpoint_index\":" << checkpoint_index
      << ",\"session\":" << marshal_session_to_json(loop)
      << ",\"run_plan_progress\":"
      << marshal_run_plan_progress_to_json(run_plan_progress)
      << ",\"prior_input_checkpoint_path\":"
      << json_quote(prior_input_checkpoint_path.string())
      << ",\"human_request_path\":" << json_quote(loop.human_request_path)
      << ",\"clarification_answer_path\":" << json_quote(answer_path.string())
      << ",\"clarification_answer\":{\"answer\":" << json_quote(answer) << "}"
      << ",\"memory_path\":" << json_quote(loop.memory_path)
      << ",\"marshal_objective_dsl_path\":"
      << json_quote(loop.marshal_objective_dsl_path)
      << ",\"marshal_objective_md_path\":"
      << json_quote(loop.marshal_objective_md_path)
      << ",\"marshal_guidance_md_path\":"
      << json_quote(loop.marshal_guidance_md_path)
      << ",\"hero_marshal_dsl_path\":" << json_quote(loop.hero_marshal_dsl_path)
      << ",\"config_policy_path\":" << json_quote(loop.config_policy_path)
      << ",\"briefing_path\":" << json_quote(loop.briefing_path)
      << ",\"mutable_objective_root\":" << json_quote(loop.objective_root)
      << ",\"objective_campaign_dsl_path\":"
      << json_quote(loop.campaign_dsl_path)
      << ",\"objective_root\":" << json_quote(loop.objective_root)
      << ",\"campaigns_root\":"
      << json_quote(campaigns_root_for_app(app).string())
      << ",\"marshal_root\":" << json_quote(app.defaults.marshal_root.string())
      << "}";
  *out_json = out.str();
  return true;
}

[[nodiscard]] bool build_marshal_message_input_checkpoint_json(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index,
    const std::filesystem::path &prior_input_checkpoint_path,
    const cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t
        &message,
    std::string *out_json, std::string *error) {
  if (error)
    error->clear();
  if (!out_json) {
    if (error)
      *error = "message checkpoint output pointer is null";
    return false;
  }
  out_json->clear();

  const std::string trimmed_message = trim_ascii(message.text);
  if (trimmed_message.empty()) {
    if (error)
      *error = "message checkpoint requires non-empty message";
    return false;
  }

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t campaign_instruction{};
  if (!decode_campaign_snapshot(app, loop.campaign_dsl_path,
                                &campaign_instruction, error)) {
    return false;
  }
  marshal_run_plan_progress_t run_plan_progress{};
  if (!collect_marshal_run_plan_progress(app, loop, campaign_instruction,
                                         nullptr, &run_plan_progress, error)) {
    return false;
  }

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(kInputCheckpointSchemaV3) << ","
      << "\"checkpoint_kind\":\"operator_message\","
      << "\"checkpoint_index\":" << checkpoint_index
      << ",\"session\":" << marshal_session_to_json(loop)
      << ",\"run_plan_progress\":"
      << marshal_run_plan_progress_to_json(run_plan_progress)
      << ",\"prior_input_checkpoint_path\":"
      << json_quote(prior_input_checkpoint_path.string())
      << ",\"operator_message\":{\"message_id\":"
      << json_quote(message.message_id)
      << ",\"text\":" << json_quote(trimmed_message)
      << ",\"delivery_mode\":" << json_quote(message.delivery_mode) << "}"
      << ",\"memory_path\":" << json_quote(loop.memory_path)
      << ",\"marshal_objective_dsl_path\":"
      << json_quote(loop.marshal_objective_dsl_path)
      << ",\"marshal_objective_md_path\":"
      << json_quote(loop.marshal_objective_md_path)
      << ",\"marshal_guidance_md_path\":"
      << json_quote(loop.marshal_guidance_md_path)
      << ",\"hero_marshal_dsl_path\":" << json_quote(loop.hero_marshal_dsl_path)
      << ",\"config_policy_path\":" << json_quote(loop.config_policy_path)
      << ",\"briefing_path\":" << json_quote(loop.briefing_path)
      << ",\"mutable_objective_root\":" << json_quote(loop.objective_root)
      << ",\"objective_campaign_dsl_path\":"
      << json_quote(loop.campaign_dsl_path)
      << ",\"objective_root\":" << json_quote(loop.objective_root)
      << ",\"campaigns_root\":"
      << json_quote(campaigns_root_for_app(app).string())
      << ",\"marshal_root\":" << json_quote(app.defaults.marshal_root.string())
      << "}";
  *out_json = out.str();
  return true;
}

[[nodiscard]] bool load_verified_human_resolution(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    const std::filesystem::path &response_path,
    const std::filesystem::path &signature_path,
    cuwacunu::hero::human::human_resolution_record_t *out_response,
    std::string *out_signature_hex, std::string *error) {
  if (error)
    error->clear();
  if (!out_response || !out_signature_hex) {
    if (error)
      *error = "human resolution outputs are null";
    return false;
  }
  *out_response = cuwacunu::hero::human::human_resolution_record_t{};
  out_signature_hex->clear();

  marshal_session_workspace_context_t workspace_context{};
  if (!load_marshal_session_workspace_context(app, loop, &workspace_context,
                                              error)) {
    return false;
  }
  if (workspace_context.human_operator_identities.empty()) {
    if (error) {
      *error = "Marshal Hero defaults missing human_operator_identities; "
               "cannot verify human resolution";
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

  cuwacunu::hero::human::human_resolution_record_t response{};
  if (!cuwacunu::hero::human::parse_human_resolution_json(response_text,
                                                          &response, error)) {
    return false;
  }

  std::string verified_fingerprint{};
  if (!cuwacunu::hero::human::verify_human_attested_json_signature(
          workspace_context.human_operator_identities, response.operator_id,
          response_text, signature_hex, &verified_fingerprint, error)) {
    return false;
  }
  if (response.signer_public_key_fingerprint_sha256_hex !=
      verified_fingerprint) {
    if (error) {
      *error = "human response fingerprint does not match verified public key "
               "fingerprint";
    }
    return false;
  }
  if (response.marshal_session_id != loop.marshal_session_id) {
    if (error)
      *error = "governance resolution marshal_session_id does not match target "
               "session";
    return false;
  }
  if (response.checkpoint_index != loop.checkpoint_count) {
    if (error) {
      *error = "governance resolution checkpoint_index does not match pending "
               "checkpoint";
    }
    return false;
  }
  std::string request_sha256_hex{};
  if (!cuwacunu::hero::human::sha256_hex_file(loop.human_request_path,
                                              &request_sha256_hex, error)) {
    return false;
  }
  if (response.request_sha256_hex != request_sha256_hex) {
    if (error) {
      *error = "governance resolution request_sha256_hex does not match "
               "current request artifact";
    }
    return false;
  }
  *out_response = std::move(response);
  *out_signature_hex = std::move(signature_hex);
  return true;
}

[[nodiscard]] std::string marshal_decision_schema_json() {
  return "{\"type\":\"object\",\"properties\":{"
         "\"intent\":{\"type\":\"string\",\"enum\":[\"launch_campaign\","
         "\"pause_for_clarification\",\"request_governance\",\"complete\","
         "\"terminate\"]},"
         "\"launch\":{\"type\":[\"object\",\"null\"],\"properties\":{"
         "\"mode\":{\"type\":\"string\",\"enum\":[\"run_plan\",\"binding\"]},"
         "\"binding_id\":{\"type\":[\"string\",\"null\"]},"
         "\"reset_runtime_state\":{\"type\":\"boolean\"},"
         "\"requires_objective_mutation\":{\"type\":\"boolean\"}"
         "},\"required\":[\"mode\",\"binding_id\",\"reset_runtime_state\","
         "\"requires_objective_mutation\"],\"additionalProperties\":false},"
         "\"clarification_request\":{\"type\":[\"object\",\"null\"],"
         "\"properties\":{"
         "\"request\":{\"type\":\"string\"}"
         "},\"required\":[\"request\"],\"additionalProperties\":false},"
         "\"governance\":{\"type\":[\"object\",\"null\"],\"properties\":{"
         "\"kind\":{\"type\":\"string\",\"enum\":[\"authority_expansion\","
         "\"launch_budget_expansion\",\"policy_decision\"]},"
         "\"request\":{\"type\":\"string\"},"
         "\"delta\":{\"type\":[\"object\",\"null\"],\"properties\":{"
         "\"allow_default_write\":{\"type\":\"boolean\"},"
         "\"additional_campaign_launches\":{\"type\":\"integer\"}"
         "},\"required\":[\"allow_default_write\",\"additional_campaign_"
         "launches\"],\"additionalProperties\":false}"
         "},\"required\":[\"kind\",\"request\",\"delta\"],"
         "\"additionalProperties\":false},"
         "\"reason\":{\"type\":\"string\"},"
         "\"memory_note\":{\"type\":\"string\"}"
         "},\"required\":[\"intent\",\"launch\",\"clarification_request\","
         "\"governance\",\"reason\",\"memory_note\"],\"additionalProperties\":"
         "false}";
}

[[nodiscard]] std::string bool_text(bool value) {
  return value ? "true" : "false";
}

[[nodiscard]] std::string
describe_marshal_binary_state(const std::filesystem::path &binary_path,
                              std::uint64_t session_started_at_ms) {
  std::ostringstream out;
  out << "path=" << binary_path.string();

  std::error_code exists_ec{};
  const bool exists = std::filesystem::exists(binary_path, exists_ec);
  out << " exists=" << bool_text(exists && !exists_ec);
  if (exists_ec || !exists) {
    out << " executable=unknown rebuilt_after_session_start=unknown";
    return out.str();
  }

  out << " executable=" << bool_text(::access(binary_path.c_str(), X_OK) == 0);

  std::error_code time_ec{};
  const auto mtime = std::filesystem::last_write_time(binary_path, time_ec);
  if (time_ec) {
    out << " rebuilt_after_session_start=unknown";
    return out.str();
  }

  const std::uint64_t now_ms = cuwacunu::hero::runtime::now_ms_utc();
  bool rebuilt_after_session_start = false;
  if (session_started_at_ms != 0 && session_started_at_ms <= now_ms) {
    const auto session_age =
        std::chrono::milliseconds(now_ms - session_started_at_ms);
    const auto session_start_on_file_clock =
        decltype(mtime)::clock::now() - session_age;
    rebuilt_after_session_start = mtime > session_start_on_file_clock;
  }
  out << " rebuilt_after_session_start="
      << bool_text(rebuilt_after_session_start);
  return out.str();
}

[[nodiscard]] std::string build_missing_mutation_diagnostic(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index) {
  std::ostringstream out;
  std::filesystem::path mutation_path{};
  const bool mutation_exists = has_recorded_mutation_checkpoint(
      app, loop, checkpoint_index, &mutation_path);
  out << "launch.requires_objective_mutation=true but no same-checkpoint "
         "objective mutation was materialized and no recorded mutation "
         "checkpoint is available; "
         "mutation_checkpoint_path="
      << mutation_path.string() << " exists=" << bool_text(mutation_exists)
      << "; config_hero_binary("
      << describe_marshal_binary_state(app.defaults.config_hero_binary,
                                       loop.started_at_ms)
      << "); inspect codex stderr "
      << cuwacunu::hero::marshal::marshal_session_codex_stderr_path(
             app.defaults.marshal_root, loop.marshal_session_id)
             .string()
      << " and codex stdout "
      << cuwacunu::hero::marshal::marshal_session_codex_stdout_path(
             app.defaults.marshal_root, loop.marshal_session_id)
             .string();
  return out.str();
}

[[nodiscard]] bool validate_marshal_checkpoint_intent_action(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index,
    const marshal_checkpoint_decision_t &decision,
    bool objective_mutation_materialized, std::string *error) {
  if (error)
    error->clear();
  if (decision.intent == "launch_campaign") {
    const bool mutation_checkpoint_exists =
        has_recorded_mutation_checkpoint(app, loop, checkpoint_index);
    if (decision.launch_requires_objective_mutation &&
        !objective_mutation_materialized && !mutation_checkpoint_exists) {
      if (error) {
        *error = build_missing_mutation_diagnostic(app, loop, checkpoint_index);
      }
      return false;
    }
    if (decision.launch_mode == "run_plan") {
      return true;
    }
    if (decision.launch_mode == "binding") {
      cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
      if (!decode_campaign_snapshot(app, loop.campaign_dsl_path, &instruction,
                                    error)) {
        return false;
      }
      const auto bind_it =
          std::find_if(instruction.binds.begin(), instruction.binds.end(),
                       [&](const auto &bind) {
                         return bind.id == decision.launch_binding_id;
                       });
      if (bind_it == instruction.binds.end()) {
        if (error) {
          *error = "launch outcome binding_id is not declared in campaign: " +
                   decision.launch_binding_id;
        }
        return false;
      }
      return true;
    }
    if (error) {
      *error = "unsupported launch.mode: " + decision.launch_mode;
    }
    return false;
  }
  if (decision.intent == "pause_for_clarification") {
    if (trim_ascii(decision.clarification_request).empty()) {
      if (error) {
        *error =
            "pause_for_clarification requires clarification_request.request";
      }
      return false;
    }
    return true;
  }
  if (decision.intent == "request_governance") {
    if (decision.governance_kind != "authority_expansion" &&
        decision.governance_kind != "launch_budget_expansion" &&
        decision.governance_kind != "policy_decision") {
      if (error) {
        *error = "unsupported governance.kind: " + decision.governance_kind;
      }
      return false;
    }
    if (trim_ascii(decision.governance_request).empty()) {
      if (error)
        *error = "request_governance requires governance.request";
      return false;
    }
    if (decision.governance_kind == "authority_expansion" &&
        !decision.governance_allow_default_write) {
      if (error) {
        *error = "authority_expansion must request allow_default_write";
      }
      return false;
    }
    if (decision.governance_kind == "launch_budget_expansion" &&
        decision.governance_additional_campaign_launches == 0) {
      if (error) {
        *error =
            "launch_budget_expansion requires a positive campaign-launch delta";
      }
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool
parse_marshal_checkpoint_decision(const std::string &json,
                                  marshal_checkpoint_decision_t *out,
                                  std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "decision output pointer is null";
    return false;
  }
  *out = marshal_checkpoint_decision_t{};
  if (!extract_json_string_field(json, "reason", &out->reason)) {
    if (error)
      *error = "intent JSON missing required reason";
    return false;
  }
  if (!extract_json_string_field(json, "intent", &out->intent)) {
    if (error)
      *error = "intent JSON missing required intent";
    return false;
  }
  std::string launch_json{};
  if (extract_json_object_field(json, "launch", &launch_json)) {
    (void)extract_json_string_field(launch_json, "mode", &out->launch_mode);
    (void)extract_json_string_field(launch_json, "binding_id",
                                    &out->launch_binding_id);
    if (extract_json_field_raw(launch_json, "reset_runtime_state", nullptr) &&
        !extract_json_bool_field(launch_json, "reset_runtime_state",
                                 &out->launch_reset_runtime_state)) {
      if (error) {
        *error = "launch.reset_runtime_state must be boolean when present";
      }
      return false;
    }
    if (extract_json_field_raw(launch_json, "requires_objective_mutation",
                               nullptr) &&
        !extract_json_bool_field(launch_json, "requires_objective_mutation",
                                 &out->launch_requires_objective_mutation)) {
      if (error) {
        *error =
            "launch.requires_objective_mutation must be boolean when present";
      }
      return false;
    }
  }
  (void)extract_json_string_field(json, "memory_note", &out->memory_note);
  std::string clarification_request_json{};
  if (extract_json_object_field(json, "clarification_request",
                                &clarification_request_json)) {
    (void)extract_json_string_field(clarification_request_json, "request",
                                    &out->clarification_request);
  }
  std::string governance_json{};
  if (extract_json_object_field(json, "governance", &governance_json)) {
    (void)extract_json_string_field(governance_json, "kind",
                                    &out->governance_kind);
    (void)extract_json_string_field(governance_json, "request",
                                    &out->governance_request);
    std::string delta_json{};
    if (extract_json_object_field(governance_json, "delta", &delta_json)) {
      (void)extract_json_bool_field(delta_json, "allow_default_write",
                                    &out->governance_allow_default_write);
      (void)extract_json_u64_field(
          delta_json, "additional_campaign_launches",
          &out->governance_additional_campaign_launches);
    }
  }
  out->intent = trim_ascii(out->intent);
  out->launch_mode = trim_ascii(out->launch_mode);
  out->launch_binding_id = trim_ascii(out->launch_binding_id);
  out->clarification_request = trim_ascii(out->clarification_request);
  out->governance_kind = trim_ascii(out->governance_kind);
  out->governance_request = trim_ascii(out->governance_request);
  out->memory_note = trim_ascii(out->memory_note);
  if (out->intent != "launch_campaign" &&
      out->intent != "pause_for_clarification" &&
      out->intent != "request_governance" && out->intent != "complete" &&
      out->intent != "terminate") {
    if (error)
      *error = "unsupported intent: " + out->intent;
    return false;
  }
  if (out->intent == "launch_campaign" && out->launch_mode.empty()) {
    if (error)
      *error = "launch_campaign requires launch.mode";
    return false;
  }
  if (!out->launch_mode.empty() && out->launch_mode != "run_plan" &&
      out->launch_mode != "binding") {
    if (error)
      *error = "unsupported launch.mode: " + out->launch_mode;
    return false;
  }
  if (out->launch_mode == "binding" && out->launch_binding_id.empty()) {
    if (error)
      *error = "launch.mode=binding requires launch.binding_id";
    return false;
  }
  if (out->intent != "launch_campaign") {
    out->launch_mode.clear();
    out->launch_binding_id.clear();
    out->launch_reset_runtime_state = false;
    out->launch_requires_objective_mutation = false;
  }
  if (out->intent != "pause_for_clarification") {
    out->clarification_request.clear();
  }
  if (out->intent != "request_governance") {
    out->governance_kind.clear();
    out->governance_request.clear();
    out->governance_allow_default_write = false;
    out->governance_additional_campaign_launches = 0;
  } else if (out->governance_kind.empty()) {
    if (error)
      *error = "request_governance requires governance.kind";
    return false;
  }
  return true;
}

[[nodiscard]] bool rewrite_marshal_session_briefing(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::string *error) {
  if (error)
    error->clear();
  std::string objective_dsl_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.marshal_objective_dsl_path,
                                               &objective_dsl_text, error)) {
    return false;
  }
  std::string objective_md_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.marshal_objective_md_path,
                                               &objective_md_text, error)) {
    return false;
  }
  std::string guidance_md_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.marshal_guidance_md_path,
                                               &guidance_md_text, error)) {
    return false;
  }
  std::string hero_marshal_dsl_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.hero_marshal_dsl_path,
                                               &hero_marshal_dsl_text, error)) {
    return false;
  }
  std::string memory_text{};
  {
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(loop.memory_path,
                                                  &memory_text, &ignored);
  }
  std::string input_checkpoint_text{};
  {
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(
        cuwacunu::hero::marshal::marshal_session_latest_input_checkpoint_path(
            std::filesystem::path(loop.session_root).parent_path(),
            loop.marshal_session_id),
        &input_checkpoint_text, &ignored);
  }
  std::string checkpoint_kind{};
  std::string prior_input_checkpoint_text{};
  std::string human_request_text{};
  std::string human_resolution_text{};
  if (!trim_ascii(input_checkpoint_text).empty()) {
    (void)extract_json_string_field(input_checkpoint_text, "checkpoint_kind",
                                    &checkpoint_kind);
    if (trim_ascii(checkpoint_kind) == "governance_followup") {
      std::string prior_input_checkpoint_path{};
      if (extract_json_string_field(input_checkpoint_text,
                                    "prior_input_checkpoint_path",
                                    &prior_input_checkpoint_path) &&
          !trim_ascii(prior_input_checkpoint_path).empty()) {
        std::string ignored{};
        (void)cuwacunu::hero::runtime::read_text_file(
            prior_input_checkpoint_path, &prior_input_checkpoint_text,
            &ignored);
      }
      std::string human_resolution_path{};
      if (extract_json_string_field(input_checkpoint_text,
                                    "governance_resolution_path",
                                    &human_resolution_path) &&
          !trim_ascii(human_resolution_path).empty()) {
        std::string ignored{};
        (void)cuwacunu::hero::runtime::read_text_file(
            human_resolution_path, &human_resolution_text, &ignored);
      }
    }
  }
  {
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(
        loop.human_request_path, &human_request_text, &ignored);
  }
  std::string campaign_text{};
  {
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(loop.campaign_dsl_path,
                                                  &campaign_text, &ignored);
  }
  objective_bundle_snapshot_t objective_bundle_snapshot{};
  {
    std::string ignored{};
    (void)collect_objective_bundle_snapshot(
        std::filesystem::path(loop.objective_root), &objective_bundle_snapshot,
        &ignored);
  }
  std::string objective_campaign_relative_path =
      std::filesystem::path(loop.campaign_dsl_path).filename().string();
  {
    const std::filesystem::path relative_path =
        std::filesystem::path(loop.campaign_dsl_path)
            .lexically_relative(std::filesystem::path(loop.objective_root));
    if (!relative_path.empty() && !relative_path.is_absolute()) {
      const std::string relative_text = relative_path.string();
      if (!relative_text.empty() && relative_text.rfind("..", 0) != 0) {
        objective_campaign_relative_path = relative_text;
      }
    }
  }
  std::ostringstream out;
  out << "You are planning inside a Marshal Hero session.\n\n"
      << "Sovereignty:\n"
      << "- Marshal Hero owns the session ledger and autonomous checkpoint "
         "orchestration.\n"
      << "- Runtime Hero executes campaigns only.\n"
      << "- Config Hero is the only writer for objective/default file "
         "changes.\n"
      << "- Hashimyei and Lattice are read-only evidence surfaces in this "
         "session.\n\n"
      << "Primary files:\n"
      << "- Marshal objective DSL: " << loop.marshal_objective_dsl_path << "\n"
      << "- Marshal objective markdown: " << loop.marshal_objective_md_path
      << "\n"
      << "- Marshal guidance markdown: " << loop.marshal_guidance_md_path
      << "\n"
      << "- Marshal session settings DSL: " << loop.hero_marshal_dsl_path
      << "\n"
      << "- Marshal Hero session manifest: "
      << cuwacunu::hero::marshal::marshal_session_manifest_path(
             app.defaults.marshal_root, loop.marshal_session_id)
             .string()
      << "\n"
      << "- Config Hero policy: " << loop.config_policy_path << "\n"
      << "- Memory: " << loop.memory_path << "\n"
      << "- Latest input checkpoint: "
      << marshal_session_latest_input_checkpoint_path(loop).string() << "\n"
      << "- Human request artifact: " << loop.human_request_path << "\n"
      << "- Mutable objective root: " << loop.objective_root << "\n"
      << "- Objective campaign file for hero.config.objective.*: "
      << objective_campaign_relative_path << "\n\n"
      << "The input checkpoint includes checkpoint_kind = bootstrap, "
         "postcampaign, governance_followup, clarification_followup, or "
         "operator_message.\n"
      << "Bootstrap launch guidance:\n"
      << "- If checkpoint_kind = bootstrap and the input checkpoint already "
         "provides concrete default_run_bind_ids plus a concrete "
         "objective-local campaign, prefer intent=launch_campaign with "
         "launch.mode=run_plan unless a real blocker or required "
         "same-checkpoint mutation is evident. Marshal now advances only the "
         "next pending RUN bind for launch.mode=run_plan and then replans "
         "before later RUN steps.\n"
      << "- Do not spend bootstrap tool calls on broad Lattice discovery when "
         "no new campaign evidence exists yet.\n\n"
      << "Interpret the authored markdown in this order:\n"
      << "- objective markdown = what the session is trying to achieve\n"
      << "- guidance markdown = authored boundaries plus advisory heuristics; "
         "prefer stronger evidence when the guidance is not a hard rule\n\n"
      << "Available MCP tools in this planning checkpoint:\n"
      << "- hero.config.default.list\n"
      << "- hero.config.default.read\n"
      << "- hero.config.default.create\n"
      << "- hero.config.default.replace\n"
      << "- hero.config.default.delete\n"
      << "- hero.config.temp.list\n"
      << "- hero.config.temp.read\n"
      << "- hero.config.temp.create\n"
      << "- hero.config.temp.replace\n"
      << "- hero.config.temp.delete\n"
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
      << "2. Do not use hero.hashimyei.* tools in this planning checkpoint; "
         "prefer input-checkpoint evidence plus hero.lattice.* queries.\n"
      << "3. Use Config Hero objective.read/create/replace/delete for "
         "truth-source objective files under objective_root.\n"
      << "4. Use Config Hero temp.read/create/replace/delete for temporary "
         "instruction files under temp_roots when scratch authored artifacts "
         "help planning.\n"
      << "5. Use Config Hero default.read/create/replace/delete only when a "
         "shared default truly needs to change.\n"
      << "6. Pass objective_root=" << loop.objective_root
      << " to those Config Hero tools.\n"
      << "7. Use hero.config.objective.list when you need the exact relative "
         "path under objective_root; do not assume generic names like "
         "campaign.dsl.\n"
      << "8. In this session, the objective-local campaign file path for "
         "hero.config.objective.* is "
      << objective_campaign_relative_path << ".\n"
      << "9. Prefer whole-file replace with expected_sha256 from the prior "
         "read.\n"
      << "10. Never mutate files outside the configured objective/default/temp "
         "roots.\n"
      << "11. Prefer the input checkpoint first. On bootstrap checkpoints with "
         "concrete default_run_bind_ids and no fresh campaign evidence, prefer "
         "immediate launch over exploratory evidence gathering.\n"
      << "11b. Read run_plan_progress first when present. For "
         "launch.mode=run_plan, Marshal will launch only "
         "run_plan_progress.next_pending_bind_id and then return for another "
         "planning checkpoint after that bind's campaign terminates.\n"
      << "12. Use hero.lattice.list_views/list_facts/get_view/get_fact mainly "
         "after at least one campaign has produced evidence, or when debugging "
         "a specific preexisting report; family_evaluation_report requires a "
         "family canonical_path plus contract_hash.\n"
      << "13. Use Runtime get/tail tools mainly for operational debugging such "
         "as launch failures, missing logs, or abnormal traces.\n"
      << "14. Shell exec is unavailable in this environment. Prefer the "
         "embedded input checkpoint, memory, and objective bundle snapshot "
         "below; use MCP tools instead of shell reads.\n"
      << "15. If you change any objective, temp, or campaign DSL, describe the "
         "actual "
         "changes in memory_note.\n"
      << "16. Request governance only for authority_expansion, "
         "launch_budget_expansion, or policy_decision.\n"
      << "17. When checkpoint_kind = governance_followup, use the prior input "
         "checkpoint as the operational evidence base and the verified "
         "governance resolution as operator context.\n"
      << "18. A verified governance resolution with resolution_kind = grant or "
         "clarify authorizes more reasoning; it does not directly choose the "
         "next launch.\n"
      << "19. Use the embedded objective bundle snapshot below before spending "
         "tool calls rediscovering editable files.\n"
      << "20. Return only JSON matching the provided output schema.\n"
      << "21. intent=complete means the objective is satisfied for now and the "
         "session should park review-ready until a future operator message "
         "arrives.\n\n"
      << "Intent contract:\n"
      << "- intent = launch_campaign | pause_for_clarification | "
         "request_governance | complete | terminate\n"
      << "- intent=complete parks the session as idle rather than hard-closing "
         "it\n"
      << "- launch.mode = run_plan | binding when intent=launch_campaign; "
         "run_plan advances only the next pending RUN bind from the current "
         "objective-local campaign sequence\n"
      << "- launch.binding_id required when launch.mode=binding\n"
      << "- launch.reset_runtime_state must be boolean when "
         "intent=launch_campaign\n"
      << "- launch.requires_objective_mutation must be boolean when "
         "intent=launch_campaign; set it true when the launch depends on "
         "same-checkpoint objective-root file edits\n"
      << "- clarification_request.request must be non-empty when "
         "intent=pause_for_clarification\n"
      << "- governance.kind = authority_expansion | launch_budget_expansion | "
         "policy_decision when intent=request_governance\n"
      << "- governance.request must be non-empty when "
         "intent=request_governance\n"
      << "- governance.delta carries the requested authority/budget change "
         "when applicable\n\n"
      << "Repo root: " << app.defaults.repo_root.string() << "\n";
  if (!trim_ascii(objective_dsl_text).empty()) {
    out << "\nMarshal objective DSL contents:\n" << objective_dsl_text;
    if (objective_dsl_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(objective_md_text).empty()) {
    out << "\nMarshal objective markdown contents:\n" << objective_md_text;
    if (objective_md_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(guidance_md_text).empty()) {
    out << "\nMarshal guidance markdown contents:\n" << guidance_md_text;
    if (guidance_md_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(hero_marshal_dsl_text).empty()) {
    out << "\nMarshal session settings DSL contents:\n"
        << hero_marshal_dsl_text;
    if (hero_marshal_dsl_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(memory_text).empty()) {
    out << "\nMemory contents:\n" << memory_text;
    if (memory_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(input_checkpoint_text).empty()) {
    out << "\nInput checkpoint contents:\n" << input_checkpoint_text;
    if (input_checkpoint_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(prior_input_checkpoint_text).empty()) {
    out << "\nPrior input checkpoint contents:\n"
        << prior_input_checkpoint_text;
    if (prior_input_checkpoint_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(human_request_text).empty()) {
    out << "\nHuman request contents:\n" << human_request_text;
    if (human_request_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(human_resolution_text).empty()) {
    out << "\nVerified human resolution contents:\n" << human_resolution_text;
    if (human_resolution_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(campaign_text).empty()) {
    out << "\nObjective campaign DSL contents ("
        << objective_campaign_relative_path << "):\n"
        << campaign_text;
    if (campaign_text.back() != '\n')
      out << "\n";
  }
  if (!objective_bundle_snapshot.entries.empty()) {
    out << "\nObjective-local mutable bundle index:\n";
    for (const auto &entry : objective_bundle_snapshot.entries) {
      out << "- " << entry.relative_path << " (" << entry.text.size()
          << " bytes)\n";
    }
    if (objective_bundle_snapshot.truncated) {
      out << "- ... truncated after "
          << objective_bundle_snapshot.embedded_bytes
          << " embedded bytes; omitted files: "
          << objective_bundle_snapshot.omitted_files << "\n";
    }
    for (const auto &entry : objective_bundle_snapshot.entries) {
      out << "\nObjective-local file contents (" << entry.relative_path
          << "):\n"
          << entry.text;
      if (!entry.text.empty() && entry.text.back() != '\n')
        out << "\n";
    }
  }
  return cuwacunu::hero::runtime::write_text_file_atomic(loop.briefing_path,
                                                         out.str(), error);
}

[[nodiscard]] bool run_command_with_stdio_and_timeout(
    const std::vector<std::string> &argv,
    const std::filesystem::path &stdin_path,
    const std::filesystem::path &stdout_path,
    const std::filesystem::path &stderr_path,
    const std::filesystem::path *working_dir,
    const std::vector<std::pair<std::string, std::string>> *env_overrides,
    std::size_t timeout_sec, const std::filesystem::path *pid_path,
    int *out_exit_code, std::string *error) {
  if (error)
    error->clear();
  if (argv.empty()) {
    if (error)
      *error = "command argv is empty";
    return false;
  }
  if (out_exit_code)
    *out_exit_code = -1;
  if (pid_path != nullptr && !pid_path->empty())
    remove_file_noexcept(*pid_path);

  std::error_code ec{};
  if (!std::filesystem::exists(stdin_path, ec) ||
      !std::filesystem::is_regular_file(stdin_path, ec)) {
    if (error)
      *error = "command stdin path does not exist: " + stdin_path.string();
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
    if (error)
      *error = "command stderr path is empty";
    return false;
  }
  const int stderr_probe =
      ::open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (stderr_probe < 0) {
    if (error)
      *error = "cannot open stderr path for command: " + stderr_path.string();
    return false;
  }
  (void)::close(stderr_probe);

  int pipe_fds[2]{-1, -1};
#ifdef O_CLOEXEC
  if (::pipe2(pipe_fds, O_CLOEXEC) != 0) {
    if (error)
      *error = "pipe2 failed for command execution";
    return false;
  }
#else
  if (::pipe(pipe_fds) != 0) {
    if (error)
      *error = "pipe failed for command execution";
    return false;
  }
  (void)::fcntl(pipe_fds[0], F_SETFD, FD_CLOEXEC);
  (void)::fcntl(pipe_fds[1], F_SETFD, FD_CLOEXEC);
#endif

  const pid_t child = ::fork();
  if (child < 0) {
    (void)::close(pipe_fds[0]);
    (void)::close(pipe_fds[1]);
    if (error)
      *error = "fork failed for command execution";
    return false;
  }
  if (child == 0) {
    (void)::close(pipe_fds[0]);
    (void)::setpgid(0, 0);
    if (working_dir != nullptr && !working_dir->empty() &&
        ::chdir(working_dir->c_str()) != 0) {
      write_command_child_failure_noexcept(
          pipe_fds[1], command_child_failure_stage_t::kChdir, errno);
      _exit(125);
    }
    if (env_overrides != nullptr) {
      for (const auto &[key, value] : *env_overrides) {
        if (key.empty())
          continue;
        if (::setenv(key.c_str(), value.c_str(), 1) != 0) {
          write_command_child_failure_noexcept(
              pipe_fds[1], command_child_failure_stage_t::kSetenv, errno);
          _exit(125);
        }
      }
    }
    const int stdin_fd = ::open(stdin_path.c_str(), O_RDONLY);
    if (stdin_fd < 0) {
      write_command_child_failure_noexcept(
          pipe_fds[1], command_child_failure_stage_t::kStdinOpen, errno);
      _exit(126);
    }
    (void)::dup2(stdin_fd, STDIN_FILENO);
    if (stdin_fd > STDERR_FILENO)
      (void)::close(stdin_fd);
    const char *stdout_target =
        stdout_path.empty() ? "/dev/null" : stdout_path.c_str();
    const int stdout_fd =
        ::open(stdout_target, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stdout_fd < 0) {
      write_command_child_failure_noexcept(
          pipe_fds[1], command_child_failure_stage_t::kStdoutOpen, errno);
      _exit(126);
    }
    (void)::dup2(stdout_fd, STDOUT_FILENO);
    if (stdout_fd > STDERR_FILENO)
      (void)::close(stdout_fd);
    const int stderr_fd =
        ::open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stderr_fd < 0) {
      write_command_child_failure_noexcept(
          pipe_fds[1], command_child_failure_stage_t::kStderrOpen, errno);
      _exit(126);
    }
    (void)::dup2(stderr_fd, STDERR_FILENO);
    if (stderr_fd > STDERR_FILENO)
      (void)::close(stderr_fd);
    std::vector<char *> exec_argv{};
    exec_argv.reserve(argv.size() + 1);
    for (const std::string &arg : argv) {
      exec_argv.push_back(const_cast<char *>(arg.c_str()));
    }
    exec_argv.push_back(nullptr);
    ::execvp(exec_argv[0], exec_argv.data());
    write_command_child_failure_noexcept(
        pipe_fds[1], command_child_failure_stage_t::kExecvp, errno);
    _exit(127);
  }
  (void)::close(pipe_fds[1]);

  if (pid_path != nullptr && !pid_path->empty()) {
    std::string pid_error{};
    if (!cuwacunu::hero::runtime::write_text_file_atomic(
            *pid_path, std::to_string(static_cast<long long>(child)),
            &pid_error)) {
      (void)::kill(-child, SIGKILL);
      (void)::kill(child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      (void)::close(pipe_fds[0]);
      if (error) {
        *error = "cannot persist checkpoint pid path " + pid_path->string() +
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
    if (waited == child)
      break;
    if (waited < 0 && errno != EINTR) {
      if (error)
        *error = "waitpid failed for command execution";
      (void)::kill(-child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      (void)::close(pipe_fds[0]);
      if (pid_path != nullptr && !pid_path->empty())
        remove_file_noexcept(*pid_path);
      return false;
    }
    if (std::chrono::steady_clock::now() >= deadline) {
      (void)::kill(-child, SIGKILL);
      (void)::kill(child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      if (error)
        *error = "command timed out";
      (void)::close(pipe_fds[0]);
      if (pid_path != nullptr && !pid_path->empty())
        remove_file_noexcept(*pid_path);
      return false;
    }
    ::usleep(100 * 1000);
  }

  if (pid_path != nullptr && !pid_path->empty())
    remove_file_noexcept(*pid_path);
  if (WIFEXITED(status) && out_exit_code)
    *out_exit_code = WEXITSTATUS(status);

  command_child_failure_t child_failure{};
  const ssize_t failure_bytes =
      ::read(pipe_fds[0], &child_failure, sizeof(child_failure));
  (void)::close(pipe_fds[0]);
  if (failure_bytes == static_cast<ssize_t>(sizeof(child_failure)) &&
      child_failure.err != 0 &&
      child_failure.stage != command_child_failure_stage_t::kNone) {
    if (error) {
      *error = describe_command_child_failure(child_failure, argv, working_dir);
    }
    return false;
  }

  if (WIFEXITED(status)) {
    return true;
  }
  if (WIFSIGNALED(status)) {
    if (out_exit_code)
      *out_exit_code = 128 + WTERMSIG(status);
    if (error)
      *error = "command terminated by signal";
    return false;
  }
  if (error)
    *error = "command ended in unknown state";
  return false;
}

[[nodiscard]] bool build_marshal_postcampaign_input_checkpoint_json(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign,
    std::uint64_t checkpoint_index, std::string *out_json, std::string *error) {
  if (error)
    error->clear();
  if (!out_json) {
    if (error)
      *error = "postcampaign input checkpoint output pointer is null";
    return false;
  }
  out_json->clear();

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  if (!decode_campaign_snapshot(app, loop.campaign_dsl_path, &instruction,
                                error)) {
    return false;
  }
  marshal_session_workspace_context_t workspace_context{};
  if (!load_marshal_session_workspace_context(app, loop, &workspace_context,
                                              error)) {
    return false;
  }

  std::vector<std::string> declared_bind_ids{};
  declared_bind_ids.reserve(instruction.binds.size());
  for (const auto &bind : instruction.binds)
    declared_bind_ids.push_back(bind.id);
  std::vector<std::string> default_run_bind_ids{};
  default_run_bind_ids.reserve(instruction.runs.size());
  for (const auto &run : instruction.runs) {
    default_run_bind_ids.push_back(run.bind_ref);
  }
  marshal_run_plan_progress_t run_plan_progress{};
  if (!collect_marshal_run_plan_progress(app, loop, instruction, &campaign,
                                         &run_plan_progress, error)) {
    return false;
  }

  struct job_checkpoint_row_t {
    cuwacunu::hero::runtime::runtime_job_record_t record{};
    cuwacunu::hero::runtime::runtime_job_observation_t observation{};
    std::string stdout_tail{};
    std::string stderr_tail{};
    std::string trace_tail{};
    std::string contract_hash{};
    std::string wave_hash{};
  };
  std::vector<job_checkpoint_row_t> jobs{};
  jobs.reserve(campaign.job_cursors.size());
  for (const std::string &job_cursor : campaign.job_cursors) {
    cuwacunu::hero::runtime::runtime_job_record_t job{};
    if (!read_runtime_job_direct(app, job_cursor, &job, error))
      return false;
    job_checkpoint_row_t row{};
    row.record = job;
    row.observation = cuwacunu::hero::runtime::observe_runtime_job(job);
    (void)tail_file_lines(
        job.stdout_path,
        std::min<std::size_t>(40, workspace_context.tail_default_lines),
        &row.stdout_tail, nullptr);
    (void)tail_file_lines(
        job.stderr_path,
        std::min<std::size_t>(20, workspace_context.tail_default_lines),
        &row.stderr_tail, nullptr);
    (void)tail_file_lines(
        runtime_job_trace_path_for_record(app, job),
        std::min<std::size_t>(40, workspace_context.tail_default_lines),
        &row.trace_tail, nullptr);
    row.contract_hash =
        extract_logged_hash_value(row.stdout_tail, "contract_hash=");
    row.wave_hash = extract_logged_hash_value(row.stdout_tail, "wave_hash=");
    jobs.push_back(std::move(row));
  }

  const auto run_manifest_hints =
      collect_run_manifest_hints(app, declared_bind_ids, 2);
  std::vector<std::string> contract_hash_candidates{};
  for (const auto &row : jobs)
    contract_hash_candidates.push_back(row.contract_hash);
  for (const auto &hint : run_manifest_hints) {
    contract_hash_candidates.push_back(hint.contract_hash);
  }
  contract_hash_candidates =
      unique_nonempty_strings(std::move(contract_hash_candidates));

  std::ostringstream jobs_json;
  jobs_json << "[";
  for (std::size_t i = 0; i < jobs.size(); ++i) {
    if (i != 0)
      jobs_json << ",";
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
    if (i != 0)
      manifests_json << ",";
    manifests_json
        << "{"
        << "\"path\":" << json_quote(run_manifest_hints[i].path.string())
        << ",\"run_id\":" << json_quote(run_manifest_hints[i].run_id)
        << ",\"binding_id\":" << json_quote(run_manifest_hints[i].binding_id)
        << ",\"contract_hash\":"
        << json_quote(run_manifest_hints[i].contract_hash)
        << ",\"wave_hash\":" << json_quote(run_manifest_hints[i].wave_hash)
        << ",\"started_at_ms\":" << run_manifest_hints[i].started_at_ms << "}";
  }
  manifests_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(kInputCheckpointSchemaV3) << ","
      << "\"checkpoint_kind\":\"postcampaign\","
      << "\"checkpoint_index\":" << checkpoint_index
      << ",\"session\":" << marshal_session_to_json(loop)
      << ",\"campaign\":" << runtime_campaign_to_json(campaign)
      << ",\"declared_bind_ids\":" << json_array_from_strings(declared_bind_ids)
      << ",\"default_run_bind_ids\":"
      << json_array_from_strings(default_run_bind_ids)
      << ",\"run_plan_progress\":"
      << marshal_run_plan_progress_to_json(run_plan_progress)
      << ",\"jobs\":" << jobs_json.str()
      << ",\"run_manifest_hints\":" << manifests_json.str()
      << ",\"lattice_recommendations\":"
      << build_lattice_recommendations_json(contract_hash_candidates)
      << ",\"memory_path\":" << json_quote(loop.memory_path)
      << ",\"human_request_path\":" << json_quote(loop.human_request_path)
      << ",\"marshal_objective_dsl_path\":"
      << json_quote(loop.marshal_objective_dsl_path)
      << ",\"marshal_objective_md_path\":"
      << json_quote(loop.marshal_objective_md_path)
      << ",\"marshal_guidance_md_path\":"
      << json_quote(loop.marshal_guidance_md_path)
      << ",\"hero_marshal_dsl_path\":" << json_quote(loop.hero_marshal_dsl_path)
      << ",\"config_policy_path\":" << json_quote(loop.config_policy_path)
      << ",\"briefing_path\":" << json_quote(loop.briefing_path)
      << ",\"mutable_objective_root\":" << json_quote(loop.objective_root)
      << ",\"objective_campaign_dsl_path\":"
      << json_quote(loop.campaign_dsl_path)
      << ",\"objective_root\":" << json_quote(loop.objective_root)
      << ",\"campaigns_root\":"
      << json_quote(campaigns_root_for_app(app).string())
      << ",\"marshal_root\":" << json_quote(app.defaults.marshal_root.string())
      << "}";
  *out_json = out.str();
  return true;
}

[[nodiscard]] bool build_marshal_bootstrap_input_checkpoint_json(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index, std::string *out_json, std::string *error) {
  if (error)
    error->clear();
  if (!out_json) {
    if (error)
      *error = "bootstrap input checkpoint output pointer is null";
    return false;
  }
  out_json->clear();

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  if (!decode_campaign_snapshot(app, loop.campaign_dsl_path, &instruction,
                                error)) {
    return false;
  }

  std::vector<std::string> declared_bind_ids{};
  declared_bind_ids.reserve(instruction.binds.size());
  for (const auto &bind : instruction.binds)
    declared_bind_ids.push_back(bind.id);
  std::vector<std::string> default_run_bind_ids{};
  default_run_bind_ids.reserve(instruction.runs.size());
  for (const auto &run : instruction.runs) {
    default_run_bind_ids.push_back(run.bind_ref);
  }
  marshal_run_plan_progress_t run_plan_progress{};
  if (!collect_marshal_run_plan_progress(app, loop, instruction, nullptr,
                                         &run_plan_progress, error)) {
    return false;
  }

  const auto run_manifest_hints =
      collect_run_manifest_hints(app, declared_bind_ids, 2);
  std::vector<std::string> contract_hash_candidates{};
  for (const auto &hint : run_manifest_hints) {
    contract_hash_candidates.push_back(hint.contract_hash);
  }
  contract_hash_candidates =
      unique_nonempty_strings(std::move(contract_hash_candidates));

  std::ostringstream manifests_json;
  manifests_json << "[";
  for (std::size_t i = 0; i < run_manifest_hints.size(); ++i) {
    if (i != 0)
      manifests_json << ",";
    manifests_json
        << "{"
        << "\"path\":" << json_quote(run_manifest_hints[i].path.string())
        << ",\"run_id\":" << json_quote(run_manifest_hints[i].run_id)
        << ",\"binding_id\":" << json_quote(run_manifest_hints[i].binding_id)
        << ",\"contract_hash\":"
        << json_quote(run_manifest_hints[i].contract_hash)
        << ",\"wave_hash\":" << json_quote(run_manifest_hints[i].wave_hash)
        << ",\"started_at_ms\":" << run_manifest_hints[i].started_at_ms << "}";
  }
  manifests_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(kInputCheckpointSchemaV3) << ","
      << "\"checkpoint_kind\":\"bootstrap\","
      << "\"checkpoint_index\":" << checkpoint_index
      << ",\"session\":" << marshal_session_to_json(loop)
      << ",\"campaign\":null"
      << ",\"declared_bind_ids\":" << json_array_from_strings(declared_bind_ids)
      << ",\"default_run_bind_ids\":"
      << json_array_from_strings(default_run_bind_ids)
      << ",\"run_plan_progress\":"
      << marshal_run_plan_progress_to_json(run_plan_progress) << ",\"jobs\":[]"
      << ",\"run_manifest_hints\":" << manifests_json.str()
      << ",\"lattice_recommendations\":"
      << build_lattice_recommendations_json(contract_hash_candidates)
      << ",\"memory_path\":" << json_quote(loop.memory_path)
      << ",\"human_request_path\":" << json_quote(loop.human_request_path)
      << ",\"marshal_objective_dsl_path\":"
      << json_quote(loop.marshal_objective_dsl_path)
      << ",\"marshal_objective_md_path\":"
      << json_quote(loop.marshal_objective_md_path)
      << ",\"marshal_guidance_md_path\":"
      << json_quote(loop.marshal_guidance_md_path)
      << ",\"config_policy_path\":" << json_quote(loop.config_policy_path)
      << ",\"briefing_path\":" << json_quote(loop.briefing_path)
      << ",\"mutable_objective_root\":" << json_quote(loop.objective_root)
      << ",\"objective_campaign_dsl_path\":"
      << json_quote(loop.campaign_dsl_path)
      << ",\"objective_root\":" << json_quote(loop.objective_root)
      << ",\"campaigns_root\":"
      << json_quote(campaigns_root_for_app(app).string())
      << ",\"marshal_root\":" << json_quote(app.defaults.marshal_root.string())
      << ",\"launch_context\":{\"initial_launch\":true}"
      << "}";
  *out_json = out.str();
  return true;
}

[[nodiscard]] bool launch_runtime_campaign_for_decision(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    const marshal_checkpoint_decision_t &decision,
    std::string_view state_detail, std::string_view event_name,
    std::string *out_campaign_cursor, std::string *out_campaign_json,
    std::string *out_warning, std::string *error) {
  if (error)
    error->clear();
  if (out_campaign_cursor)
    out_campaign_cursor->clear();
  if (out_campaign_json)
    out_campaign_json->clear();
  if (out_warning)
    out_warning->clear();
  if (!loop) {
    if (error)
      *error = "marshal session pointer is null";
    return false;
  }

  std::string resolved_binding_id{};
  std::string launch_mode_warning{};
  if (decision.launch_mode == "binding") {
    resolved_binding_id = trim_ascii(decision.launch_binding_id);
  } else if (decision.launch_mode == "run_plan") {
    cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
    if (!decode_campaign_snapshot(app, loop->campaign_dsl_path, &instruction,
                                  error)) {
      return false;
    }
    marshal_run_plan_progress_t run_plan_progress{};
    if (!collect_marshal_run_plan_progress(app, *loop, instruction, nullptr,
                                           &run_plan_progress, error)) {
      return false;
    }
    if (!run_plan_progress.ordered_prefix_valid) {
      if (error) {
        *error =
            "launch.mode=run_plan cannot determine the next pending RUN step "
            "because prior successful launches no longer match the current "
            "default RUN sequence";
      }
      return false;
    }
    resolved_binding_id = trim_ascii(run_plan_progress.next_pending_bind_id);
    if (resolved_binding_id.empty()) {
      if (error) {
        *error = "launch.mode=run_plan found no pending RUN steps in the "
                 "current objective-local campaign";
      }
      return false;
    }
    launch_mode_warning = "run_plan launch decomposed to next pending bind: " +
                          resolved_binding_id;
  }

  std::string start_args =
      "{\"campaign_dsl_path\":" + json_quote(loop->campaign_dsl_path);
  start_args +=
      ",\"marshal_session_id\":" + json_quote(loop->marshal_session_id);
  if (!resolved_binding_id.empty()) {
    start_args += ",\"binding_id\":" + json_quote(resolved_binding_id);
  }
  if (decision.launch_reset_runtime_state) {
    start_args += ",\"reset_runtime_state\":true";
  }
  start_args += "}";

  marshal_session_workspace_context_t workspace_context{};
  if (!load_marshal_session_workspace_context(app, *loop, &workspace_context,
                                              error)) {
    return false;
  }
  std::string start_structured{};
  if (!call_runtime_tool(app, workspace_context.runtime_hero_binary,
                         "hero.runtime.start_campaign", start_args,
                         &start_structured, error)) {
    return false;
  }

  std::vector<std::string> launch_warnings{};
  append_structured_warnings(start_structured,
                             "runtime launch warning: ", &launch_warnings);
  std::string runtime_warning{};
  if (!launch_mode_warning.empty()) {
    append_warning_text(&runtime_warning, launch_mode_warning);
  }
  for (const std::string &item : launch_warnings) {
    append_warning_text(&runtime_warning, item);
  }

  std::string campaign_cursor{};
  if (!extract_json_string_field(start_structured, "campaign_cursor",
                                 &campaign_cursor) ||
      campaign_cursor.empty()) {
    if (error)
      *error = "Runtime Hero start_campaign did not return campaign_cursor";
    return false;
  }
  std::string campaign_json{};
  if (!extract_json_object_field(start_structured, "campaign",
                                 &campaign_json)) {
    if (error)
      *error = "Runtime Hero start_campaign did not return campaign object";
    return false;
  }

  loop->lifecycle = "live";
  loop->activity = "awaiting_campaign_fact";
  loop->campaign_status = "running";
  loop->status_detail = std::string(state_detail);
  if (!resolved_binding_id.empty()) {
    append_warning_text(&loop->status_detail,
                        "active bind=" + resolved_binding_id);
  }
  loop->work_gate = "open";
  loop->finish_reason = "none";
  loop->campaign_cursor = campaign_cursor;
  loop->campaign_cursors.push_back(campaign_cursor);
  ++loop->launch_count;
  if (loop->remaining_campaign_launches > 0) {
    --loop->remaining_campaign_launches;
  }
  for (auto &pending_message : loop->pending_operator_messages) {
    if (!marshal_operator_message_has_text(pending_message))
      continue;
    pending_message.delivered_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    pending_message.delivery_status = "handled";
    pending_message.handled_at_ms = pending_message.delivered_at_ms;
    std::string ignored{};
    (void)append_operator_message_event(
        loop, "marshal", "operator.message_delivered", pending_message,
        "consumed before campaign launch", "", "", &ignored);
    (void)append_operator_message_event(
        loop, "marshal", "operator.message_handled", pending_message,
        "consumed before campaign launch", "", "", &ignored);
  }
  loop->pending_operator_messages.clear();
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop->last_warning.clear();
  std::string bookkeeping_error{};
  if (!write_marshal_session(app, *loop, &bookkeeping_error)) {
    append_warning_text(&runtime_warning,
                        "post-launch session manifest degraded: " +
                            bookkeeping_error);
  } else if (!append_marshal_session_event(
                 &loop, "runtime", std::string(event_name), campaign_cursor,
                 &bookkeeping_error)) {
    append_warning_text(&runtime_warning,
                        "post-launch marshal event degraded: " +
                            bookkeeping_error);
  }
  if (!runtime_warning.empty()) {
    persist_marshal_session_warning_best_effort(app, loop, runtime_warning);
    if (out_warning)
      *out_warning = runtime_warning;
  }
  if (out_campaign_cursor)
    *out_campaign_cursor = std::move(campaign_cursor);
  if (out_campaign_json)
    *out_campaign_json = std::move(campaign_json);
  return true;
}

[[nodiscard]] bool run_marshal_checkpoint_with_codex(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::uint64_t checkpoint_index, marshal_checkpoint_decision_t *out_decision,
    std::string *out_warning, std::string *error) {
  if (error)
    error->clear();
  if (out_warning)
    out_warning->clear();
  if (!loop) {
    if (error)
      *error = "marshal session pointer is null";
    return false;
  }
  if (!out_decision) {
    if (error)
      *error = "marshal decision output pointer is null";
    return false;
  }
  scoped_temp_path_t decision_schema_path{};
  if (!write_temp_marshal_decision_schema(marshal_decision_schema_json(),
                                          &decision_schema_path, error)) {
    return false;
  }
  marshal_session_workspace_context_t workspace_context{};
  if (!load_marshal_session_workspace_context(app, *loop, &workspace_context,
                                              error)) {
    return false;
  }
  if (!refresh_marshal_session_hero_marshal_dsl(workspace_context, *loop,
                                                error)) {
    return false;
  }
  if (!refresh_marshal_session_config_policy_dsl(workspace_context, *loop,
                                                 error)) {
    return false;
  }
  if (!rewrite_marshal_session_briefing(app, *loop, error))
    return false;

  const std::filesystem::path decision_path =
      cuwacunu::hero::marshal::marshal_session_intent_checkpoint_path(
          app.defaults.marshal_root, loop->marshal_session_id,
          checkpoint_index);
  const std::string config_args = json_array_from_strings(
      {"--global-config", app.global_config_path.string(), "--config",
       loop->config_policy_path});
  const std::string runtime_args = json_array_from_strings(
      {"--global-config", app.global_config_path.string()});
  const std::string lattice_args = runtime_args;
  const std::string enabled_config_tools = json_array_from_strings(
      {"hero.config.default.list", "hero.config.default.read",
       "hero.config.default.create", "hero.config.default.replace",
       "hero.config.default.delete", "hero.config.temp.list",
       "hero.config.temp.read", "hero.config.temp.create",
       "hero.config.temp.replace", "hero.config.temp.delete",
       "hero.config.objective.list", "hero.config.objective.read",
       "hero.config.objective.create", "hero.config.objective.replace",
       "hero.config.objective.delete"});
  const std::string enabled_runtime_tools = json_array_from_strings(
      {"hero.runtime.get_campaign", "hero.runtime.get_job",
       "hero.runtime.list_jobs", "hero.runtime.tail_log",
       "hero.runtime.tail_trace"});
  const std::string enabled_lattice_tools = json_array_from_strings(
      {"hero.lattice.list_facts", "hero.lattice.get_fact",
       "hero.lattice.list_views", "hero.lattice.get_view"});

  auto append_common_codex_mcp_args = [&](std::vector<std::string> *argv) {
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-config.enabled=true");
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-config.command=" +
                    json_quote(workspace_context.config_hero_binary.string()));
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
                    json_quote(workspace_context.runtime_hero_binary.string()));
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
                    json_quote(workspace_context.lattice_hero_binary.string()));
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.args=" + lattice_args);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.enabled_tools=" +
                    enabled_lattice_tools);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.startup_timeout_sec=30");
  };

  auto load_decision_text = [&](std::string *out_text,
                                std::string *out_err) -> bool {
    if (!out_text) {
      if (out_err)
        *out_err = "decision output pointer is null";
      return false;
    }
    out_text->clear();
    return cuwacunu::hero::runtime::read_text_file(decision_path, out_text,
                                                   out_err);
  };

  auto run_codex_attempt = [&](bool resume_mode, std::string *out_decision_text,
                               std::string *out_session_id,
                               std::string *out_attempt_error) -> bool {
    if (out_attempt_error)
      out_attempt_error->clear();
    if (out_decision_text)
      out_decision_text->clear();
    if (out_session_id)
      out_session_id->clear();

    std::vector<std::string> argv{};
    argv.reserve(64);
    const std::filesystem::path persisted_codex_binary_path =
        resolve_marshal_codex_binary_path(workspace_context.repo_root,
                                          loop->resolved_marshal_codex_binary);
    const std::string persisted_codex_binary =
        trim_ascii(persisted_codex_binary_path.string());
    const std::string persisted_codex_model =
        trim_ascii(loop->resolved_marshal_codex_model);
    const std::string persisted_codex_reasoning_effort =
        trim_ascii(loop->resolved_marshal_codex_reasoning_effort);
    if (persisted_codex_binary.empty()) {
      if (out_attempt_error) {
        *out_attempt_error =
            "marshal session is missing resolved_marshal_codex_binary";
      }
      return false;
    }
    if (!path_is_executable(persisted_codex_binary_path)) {
      if (out_attempt_error) {
        *out_attempt_error =
            "resolved_marshal_codex_binary is not executable: " +
            persisted_codex_binary;
      }
      return false;
    }
    loop->resolved_marshal_codex_binary = persisted_codex_binary;
    if (persisted_codex_model.empty()) {
      if (out_attempt_error) {
        *out_attempt_error =
            "marshal session is missing resolved_marshal_codex_model";
      }
      return false;
    }
    if (persisted_codex_reasoning_effort.empty()) {
      if (out_attempt_error) {
        *out_attempt_error = "marshal session is missing "
                             "resolved_marshal_codex_reasoning_effort";
      }
      return false;
    }
    argv.push_back(persisted_codex_binary);
    argv.push_back("exec");
    argv.push_back("--json");
    if (resume_mode) {
      argv.push_back("resume");
    } else {
      argv.push_back("-s");
      argv.push_back("read-only");
      argv.push_back("--color");
      argv.push_back("never");
    }
    argv.push_back("-m");
    argv.push_back(persisted_codex_model);
    argv.push_back("-c");
    argv.push_back("model_reasoning_effort=" +
                   json_quote(persisted_codex_reasoning_effort));
    append_common_codex_mcp_args(&argv);
    if (!resume_mode) {
      argv.push_back("--output-schema");
      argv.push_back(decision_schema_path.path.string());
    }
    argv.push_back("-o");
    argv.push_back(decision_path.string());
    if (resume_mode) {
      argv.push_back(loop->current_thread_id);
    }
    argv.push_back("-");

    int exit_code = -1;
    std::string invoke_error{};
    loop->codex_stdout_path =
        cuwacunu::hero::marshal::marshal_session_codex_stdout_path(
            app.defaults.marshal_root, loop->marshal_session_id)
            .string();
    loop->codex_stderr_path =
        cuwacunu::hero::marshal::marshal_session_codex_stderr_path(
            app.defaults.marshal_root, loop->marshal_session_id)
            .string();
    const std::filesystem::path stdout_path(loop->codex_stdout_path);
    const std::filesystem::path stderr_jsonl_path(loop->codex_stderr_path);
    const std::filesystem::path stderr_capture_path =
        stderr_jsonl_path.parent_path() /
        (stderr_jsonl_path.filename().string() + ".tmp");
    const std::filesystem::path checkpoint_pid_path =
        marshal_session_checkpoint_pid_path(*loop);
    remove_file_noexcept(stderr_capture_path);
    remove_file_noexcept(marshal_session_compat_codex_session_log_path(*loop));

    const bool invoked = run_command_with_stdio_and_timeout(
        argv, loop->briefing_path, stdout_path, stderr_capture_path,
        &workspace_context.repo_root, nullptr,
        workspace_context.marshal_codex_timeout_sec, &checkpoint_pid_path,
        &exit_code, &invoke_error);

    std::string stderr_text{};
    std::string stderr_finalize_error{};
    if (!cuwacunu::hero::runtime::read_text_file(
            stderr_capture_path, &stderr_text, &stderr_finalize_error)) {
      std::error_code ec{};
      if (std::filesystem::exists(stderr_capture_path, ec) && !ec) {
        remove_file_noexcept(stderr_capture_path);
        if (out_attempt_error) {
          *out_attempt_error =
              "codex stderr capture read failed: " + stderr_finalize_error;
        }
        return false;
      }
      stderr_text.clear();
    }
    if (!cuwacunu::hero::runtime::write_text_file_atomic(
            stderr_jsonl_path,
            encode_text_lines_as_jsonl("stderr", stderr_text),
            &stderr_finalize_error)) {
      remove_file_noexcept(stderr_capture_path);
      if (out_attempt_error) {
        *out_attempt_error = "codex stderr capture finalization failed: " +
                             stderr_finalize_error;
      }
      return false;
    }
    remove_file_noexcept(stderr_capture_path);
    if (!stderr_text.empty() && out_session_id) {
      *out_session_id = extract_current_thread_id_from_log(stderr_text);
    }
    if (!invoked) {
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
    if (!load_decision_text(out_decision_text, out_attempt_error))
      return false;
    return true;
  };

  std::string decision_text{};
  std::string parsed_session_id{};
  bool decision_ready = false;
  const std::string prior_thread_id = trim_ascii(loop->current_thread_id);
  const bool had_prior_session = !trim_ascii(loop->current_thread_id).empty();
  if (had_prior_session) {
    loop->codex_continuity = "resuming";
    std::string resume_error{};
    std::string resume_decision_text{};
    std::string resumed_session_id{};
    if (run_codex_attempt(true, &resume_decision_text, &resumed_session_id,
                          &resume_error)) {
      if (parse_marshal_checkpoint_decision(resume_decision_text, out_decision,
                                            &resume_error)) {
        decision_text = std::move(resume_decision_text);
        loop->codex_continuity = "attached";
        loop->last_resume_error.clear();
        if (!resumed_session_id.empty()) {
          loop->current_thread_id = resumed_session_id;
          if (loop->thread_lineage.empty() ||
              loop->thread_lineage.back() != resumed_session_id) {
            loop->thread_lineage.push_back(resumed_session_id);
          }
        }
        decision_ready = true;
      } else if (out_warning) {
        loop->codex_continuity = "resume_failed";
        loop->last_resume_error = resume_error;
        (void)append_marshal_session_event(
            loop, "codex", "codex.resume_failed",
            "decision_parse_failed: " + resume_error, nullptr);
        append_warning_text(
            out_warning, build_resume_degraded_warning(*loop, checkpoint_index,
                                                       "decision_parse_failed",
                                                       resume_error));
      }
    } else if (out_warning) {
      loop->codex_continuity = "resume_failed";
      loop->last_resume_error = resume_error;
      (void)append_marshal_session_event(
          loop, "codex", "codex.resume_failed",
          "resume_command_failed: " + resume_error, nullptr);
      append_warning_text(out_warning,
                          build_resume_degraded_warning(*loop, checkpoint_index,
                                                        "resume_command_failed",
                                                        resume_error));
    }
  }

  if (!decision_ready) {
    std::string fresh_error{};
    if (!run_codex_attempt(false, &decision_text, &parsed_session_id,
                           &fresh_error)) {
      if (error)
        *error = fresh_error;
      return false;
    }
    if (!parse_marshal_checkpoint_decision(decision_text, out_decision,
                                           error)) {
      return false;
    }
    loop->codex_continuity = had_prior_session ? "restarted" : "attached";
    loop->last_resume_error.clear();
    if (!parsed_session_id.empty()) {
      loop->current_thread_id = parsed_session_id;
      if (loop->thread_lineage.empty() ||
          loop->thread_lineage.back() != parsed_session_id) {
        loop->thread_lineage.push_back(parsed_session_id);
      }
    } else {
      loop->current_thread_id.clear();
      if (out_warning) {
        append_warning_text(
            out_warning, "fresh codex checkpoint completed without a persisted "
                         "session id; future checkpoints will start fresh");
      }
    }
    if (had_prior_session && !parsed_session_id.empty() &&
        parsed_session_id != prior_thread_id) {
      loop->codex_continuity = "restarted";
    }
  }

  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          marshal_session_latest_intent_checkpoint_path(*loop), decision_text,
          error)) {
    return false;
  }
  return true;
}

[[nodiscard]] bool execute_pending_checkpoint(
    app_context_t *app, cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string *out_warning, std::string *error) {
  if (error)
    error->clear();
  if (out_warning)
    out_warning->clear();
  if (!app || !loop) {
    if (error)
      *error = "pending checkpoint inputs are null";
    return false;
  }

  const std::uint64_t checkpoint_index = loop->checkpoint_count + 1;
  objective_hash_snapshot_t objective_before_snapshot{};
  if (!collect_objective_hash_snapshot(
          std::filesystem::path(loop->objective_root),
          &objective_before_snapshot, error)) {
    return false;
  }
  marshal_checkpoint_decision_t decision{};
  std::string checkpoint_warning{};
  if (!run_marshal_checkpoint_with_codex(*app, loop, checkpoint_index,
                                         &decision, &checkpoint_warning,
                                         error)) {
    cuwacunu::hero::marshal::marshal_session_record_t terminal_loop{};
    if (reload_terminal_marshal_session_if_any(*app, loop->marshal_session_id,
                                               &terminal_loop)) {
      *loop = std::move(terminal_loop);
      if (out_warning) {
        append_warning_text(
            out_warning,
            "planning checkpoint interrupted after session became terminal");
      }
      return true;
    }
    loop->lifecycle = "terminal";
    loop->activity = "quiet";
    loop->campaign_status = "none";
    loop->status_detail = *error;
    loop->work_gate = "open";
    loop->finish_reason = "failed";
    loop->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop->updated_at_ms = *loop->finished_at_ms;
    std::string ignored{};
    (void)write_marshal_session(*app, *loop, &ignored);
    (void)append_marshal_session_event(&loop, "marshal", "checkpoint.failed",
                                       *error, &ignored);
    return false;
  }
  if (!persist_attempted_checkpoint_state(
          *app, loop, checkpoint_index, decision, checkpoint_warning, error)) {
    return false;
  }
  objective_hash_snapshot_t objective_after_snapshot{};
  if (!collect_objective_hash_snapshot(
          std::filesystem::path(loop->objective_root),
          &objective_after_snapshot, error)) {
    const std::string failure_detail = error ? *error : std::string{};
    return park_session_idle_after_checkpoint_failure(
        *app, loop, checkpoint_index, failure_detail, error);
  }
  bool objective_mutation_materialized = false;
  if (!write_mutation_checkpoint_summary(
          *app, loop, checkpoint_index, objective_before_snapshot,
          objective_after_snapshot, &objective_mutation_materialized, error)) {
    const std::string failure_detail = error ? *error : std::string{};
    return park_session_idle_after_checkpoint_failure(
        *app, loop, checkpoint_index, failure_detail, error);
  }
  if (objective_mutation_materialized &&
      trim_ascii(loop->last_mutation_checkpoint_path).empty()) {
    loop->last_mutation_checkpoint_path =
        cuwacunu::hero::marshal::marshal_session_mutation_checkpoint_path(
            app->defaults.marshal_root, loop->marshal_session_id,
            checkpoint_index)
            .string();
  }
  if (!validate_marshal_checkpoint_intent_action(
          *app, *loop, checkpoint_index, decision,
          objective_mutation_materialized, error)) {
    const std::string failure_detail = error ? *error : std::string{};
    return park_session_idle_after_checkpoint_failure(
        *app, loop, checkpoint_index, failure_detail, error);
  }

  loop->checkpoint_count = checkpoint_index;
  loop->last_codex_action = decision.intent;
  loop->last_warning.clear();
  if (!checkpoint_warning.empty()) {
    append_warning_text(&loop->last_warning, checkpoint_warning);
  }
  loop->last_intent_checkpoint_path =
      cuwacunu::hero::marshal::marshal_session_intent_checkpoint_path(
          app->defaults.marshal_root, loop->marshal_session_id,
          checkpoint_index)
          .string();
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop->finished_at_ms.reset();
  std::vector<
      cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t>
      delivered_operator_messages{};
  for (auto &pending_message : loop->pending_operator_messages) {
    if (!marshal_operator_message_has_text(pending_message))
      continue;
    pending_message.delivery_status = "delivered";
    pending_message.delivered_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    delivered_operator_messages.push_back(pending_message);
    std::string ignored{};
    (void)append_operator_message_event(
        loop, "marshal", "operator.message_delivered", pending_message,
        "staged into checkpoint input", "", "", &ignored);
  }
  loop->pending_operator_messages.clear();
  loop->work_gate = "open";
  loop->finish_reason = "none";

  if (decision.intent == "complete" || decision.intent == "terminate") {
    const std::string action_detail =
        decision.intent + ": " + trim_ascii(decision.reason);
    if (!append_marshal_session_event(loop, "codex", "codex.action_requested",
                                      action_detail, error)) {
      return false;
    }
    loop->lifecycle = (decision.intent == "complete") ? "live" : "terminal";
    loop->activity = (decision.intent == "complete") ? "review" : "quiet";
    loop->status_detail = decision.reason;
    loop->work_gate = "open";
    loop->finish_reason =
        (decision.intent == "complete") ? "success" : "terminated";
    loop->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop->updated_at_ms = *loop->finished_at_ms;
    clear_marshal_session_campaign_linkage(loop);
    for (auto pending_message : delivered_operator_messages) {
      pending_message.delivery_status = "handled";
      pending_message.handled_at_ms =
          pending_message.delivered_at_ms.has_value()
              ? pending_message.delivered_at_ms
              : std::optional<std::uint64_t>(
                    cuwacunu::hero::runtime::now_ms_utc());
      std::string ignored{};
      (void)append_operator_message_event(
          loop, "marshal", "operator.message_handled", pending_message,
          (decision.intent == "complete")
              ? "consumed before review-ready parking"
              : "consumed before terminal finish",
          "", "", &ignored);
    }
    if (!write_marshal_session(*app, *loop, error))
      return false;
    const std::string event_name = (decision.intent == "complete")
                                       ? "session.review_ready"
                                       : "session.finished";
    if (!append_marshal_session_event(&loop, "marshal", event_name,
                                      decision.reason, error)) {
      return false;
    }
    return true;
  }

  if (decision.intent == "pause_for_clarification" ||
      decision.intent == "request_governance") {
    const std::string block_detail =
        (decision.intent == "pause_for_clarification")
            ? decision.clarification_request
            : (decision.governance_kind + ": " + decision.governance_request);
    if (!append_marshal_session_event(loop, "codex", "codex.action_requested",
                                      "block_work: " + block_detail, error)) {
      return false;
    }
    loop->lifecycle = "live";
    loop->activity = "quiet";
    loop->status_detail = decision.reason;
    loop->work_gate = (decision.intent == "pause_for_clarification")
                          ? "clarification"
                          : "governance";
    loop->finish_reason = "none";
    clear_marshal_session_campaign_linkage(loop);
    for (auto pending_message : delivered_operator_messages) {
      pending_message.delivery_status = "handled";
      pending_message.handled_at_ms =
          pending_message.delivered_at_ms.has_value()
              ? pending_message.delivered_at_ms
              : std::optional<std::uint64_t>(
                    cuwacunu::hero::runtime::now_ms_utc());
      std::string ignored{};
      (void)append_operator_message_event(
          loop, "marshal", "operator.message_handled", pending_message,
          (decision.intent == "pause_for_clarification")
              ? "consumed before clarification block"
              : "consumed before governance block",
          "", "", &ignored);
    }
    if (!write_marshal_session(*app, *loop, error))
      return false;
    std::string request_error{};
    if (!materialize_human_request(*app, *loop, checkpoint_index, decision,
                                   &request_error)) {
      if (error)
        *error = request_error;
      return false;
    }
    if (!append_marshal_session_event(loop, "codex", "work.blocked",
                                      block_detail, error)) {
      return false;
    }
    return true;
  }

  if (decision.intent != "launch_campaign") {
    if (error)
      *error = "unsupported planning intent: " + decision.intent;
    return false;
  }
  if (loop->remaining_campaign_launches == 0) {
    if (!append_marshal_session_event(loop, "codex", "codex.action_requested",
                                      "launch_campaign", error)) {
      return false;
    }
    loop->lifecycle = "terminal";
    loop->activity = "quiet";
    loop->status_detail = "campaign-launch budget exhausted";
    loop->work_gate = "open";
    loop->finish_reason = "exhausted";
    loop->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop->updated_at_ms = *loop->finished_at_ms;
    clear_marshal_session_campaign_linkage(loop);
    if (!write_marshal_session(*app, *loop, error))
      return false;
    return append_marshal_session_event(&loop, "marshal", "session.finished",
                                        loop->status_detail, error);
  }

  if (!append_marshal_session_event(loop, "codex", "codex.action_requested",
                                    "launch_campaign", error)) {
    return false;
  }

  std::string next_campaign_cursor{};
  std::string ignored_campaign_json{};
  std::string runtime_warning{};
  if (!launch_runtime_campaign_for_decision(
          *app, loop, decision,
          "campaign launched from marshal planning checkpoint",
          "campaign.started", &next_campaign_cursor, &ignored_campaign_json,
          &runtime_warning, error)) {
    loop->lifecycle = "terminal";
    loop->activity = "quiet";
    loop->status_detail = *error;
    loop->work_gate = "open";
    loop->finish_reason = "failed";
    loop->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop->updated_at_ms = *loop->finished_at_ms;
    std::string ignored{};
    (void)write_marshal_session(*app, *loop, &ignored);
    (void)append_marshal_session_event(
        &loop, "runtime", "campaign.start_failed", *error, &ignored);
    return false;
  }
  if (!runtime_warning.empty() && out_warning) {
    append_warning_text(out_warning, runtime_warning);
  }
  if (!checkpoint_warning.empty()) {
    persist_marshal_session_warning_best_effort(*app, loop, checkpoint_warning);
    if (out_warning)
      append_warning_text(out_warning, checkpoint_warning);
  }
  return true;
}

[[nodiscard]] bool reopen_session_after_terminal_campaign(
    app_context_t *app, std::string_view campaign_cursor,
    std::string *out_warning, std::string *error) {
  if (error)
    error->clear();
  if (out_warning)
    out_warning->clear();
  if (!app) {
    if (error)
      *error = "marshal app pointer is null";
    return false;
  }

  cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
  if (!read_runtime_campaign_direct(*app, campaign_cursor, &campaign, error)) {
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t loop{};
  const std::string marshal_session_id =
      trim_ascii(campaign.marshal_session_id);
  if (marshal_session_id.empty()) {
    if (error)
      *error = "campaign is not linked to a marshal session";
    return false;
  }
  if (!read_marshal_session(*app, marshal_session_id, &loop, error))
    return false;
  if (is_marshal_session_terminal_state(loop.lifecycle))
    return true;

  const std::uint64_t checkpoint_index = loop.checkpoint_count + 1;
  std::string input_checkpoint_json{};
  if (!build_marshal_postcampaign_input_checkpoint_json(
          *app, loop, campaign, checkpoint_index, &input_checkpoint_json,
          error)) {
    loop.lifecycle = "terminal";
    loop.activity = "quiet";
    loop.status_detail = *error;
    loop.work_gate = "open";
    loop.finish_reason = "failed";
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_marshal_session(*app, loop, &ignored);
    (void)append_marshal_session_event(&loop, "marshal", "checkpoint.failed",
                                       *error, &ignored);
    return false;
  }
  const std::filesystem::path input_checkpoint_path =
      cuwacunu::hero::marshal::marshal_session_input_checkpoint_path(
          app->defaults.marshal_root, loop.marshal_session_id,
          checkpoint_index);
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          input_checkpoint_path, input_checkpoint_json, error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          marshal_session_latest_input_checkpoint_path(loop),
          input_checkpoint_json, error)) {
    return false;
  }
  loop.lifecycle = "live";
  loop.activity = "planning";
  loop.status_detail = "waiting for codex planning checkpoint";
  loop.work_gate = "open";
  loop.finish_reason = "none";
  loop.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  clear_marshal_session_campaign_linkage(&loop);
  loop.last_input_checkpoint_path = input_checkpoint_path.string();
  if (!write_marshal_session(*app, loop, error))
    return false;
  if (!append_marshal_session_event(&loop, "marshal", "checkpoint.staged",
                                    input_checkpoint_path.string(), error)) {
    return false;
  }

  return execute_pending_checkpoint(app, &loop, out_warning, error);
}

struct session_runner_lock_guard_t {
  int fd{-1};

  ~session_runner_lock_guard_t() {
    if (fd >= 0) {
      (void)::flock(fd, LOCK_UN);
      (void)::close(fd);
    }
  }
};

[[nodiscard]] bool acquire_session_runner_lock(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    session_runner_lock_guard_t *out_lock, bool *out_already_locked,
    std::string *error) {
  if (error)
    error->clear();
  if (out_already_locked)
    *out_already_locked = false;
  if (!out_lock) {
    if (error)
      *error = "session runner lock output pointer is null";
    return false;
  }
  out_lock->fd = -1;
  const std::filesystem::path lock_path = session_runner_lock_path(loop);
  const int fd = ::open(lock_path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
  if (fd < 0) {
    if (error)
      *error = "cannot open session runner lock: " + lock_path.string();
    return false;
  }
  if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
    const int err = errno;
    (void)::close(fd);
    if (err == EWOULDBLOCK || err == EAGAIN) {
      if (out_already_locked)
        *out_already_locked = true;
      return true;
    }
    if (error)
      *error = "cannot acquire session runner lock: " + loop.marshal_session_id;
    return false;
  }
  out_lock->fd = fd;
  return true;
}

struct session_runner_launch_message_t {
  std::uint64_t runner_pid{0};
  int exec_errno{0};
};

void write_session_runner_launch_message_noexcept(
    int fd, const session_runner_launch_message_t &message) {
  if (fd < 0)
    return;
  const ssize_t ignored = ::write(fd, &message, sizeof(message));
  (void)ignored;
}

[[nodiscard]] bool
marshal_session_events_contain_event(const app_context_t &app,
                                     std::string_view marshal_session_id,
                                     std::string_view event_name) {
  std::string events_text{};
  std::string ignored{};
  if (!cuwacunu::hero::runtime::read_text_file(
          cuwacunu::hero::marshal::marshal_session_events_path(
              app.defaults.marshal_root, marshal_session_id),
          &events_text, &ignored)) {
    return false;
  }
  const std::string needle = "\"type\":" + json_quote(std::string(event_name));
  return events_text.find(needle) != std::string::npos;
}

enum class operator_message_wait_outcome_t : std::uint8_t {
  Pending = 0,
  Delivered = 1,
  Failed = 2,
};

[[nodiscard]] operator_message_wait_outcome_t
find_operator_message_wait_outcome(std::string_view events_text,
                                   std::uint64_t after_seq,
                                   std::string_view message_id,
                                   std::string *out_reply,
                                   std::string *out_failure) {
  if (out_reply)
    out_reply->clear();
  if (out_failure)
    out_failure->clear();
  operator_message_wait_outcome_t best_outcome =
      operator_message_wait_outcome_t::Pending;
  std::uint64_t best_seq = 0;
  std::istringstream in{std::string(events_text)};
  std::string line{};
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(line);
    if (trimmed.empty())
      continue;
    std::uint64_t seq = 0;
    std::string type{};
    std::string payload{};
    std::string detail{};
    std::string payload_message_id{};
    std::string delivery_status{};
    std::string reply_text{};
    std::string warning{};
    std::string last_error{};
    if (!extract_json_u64_field(trimmed, "seq", &seq) || seq <= after_seq)
      continue;
    if (!extract_json_string_field(trimmed, "type", &type) ||
        type != "operator.message_handled") {
      continue;
    }
    if (!extract_json_object_field(trimmed, "payload", &payload)) {
      continue;
    }
    (void)extract_json_string_field(payload, "message_id", &payload_message_id);
    if (trim_ascii(payload_message_id) != trim_ascii(message_id)) {
      continue;
    }
    (void)extract_json_string_field(payload, "delivery_status",
                                    &delivery_status);
    (void)extract_json_string_field(payload, "reply_text", &reply_text);
    (void)extract_json_string_field(payload, "warning", &warning);
    (void)extract_json_string_field(payload, "last_error", &last_error);
    (void)extract_json_string_field(payload, "detail", &detail);
    if ((delivery_status == "handled" || delivery_status == "delivered") &&
        seq >= best_seq) {
      best_outcome = operator_message_wait_outcome_t::Delivered;
      best_seq = seq;
      if (out_reply) {
        *out_reply = trim_ascii(reply_text);
      }
      if (out_failure)
        out_failure->clear();
      continue;
    }
    if (delivery_status == "failed" && seq >= best_seq) {
      best_outcome = operator_message_wait_outcome_t::Failed;
      best_seq = seq;
      if (out_failure) {
        *out_failure = trim_ascii(!last_error.empty() ? last_error
                                  : !warning.empty()  ? warning
                                                      : detail);
      }
      if (out_reply)
        out_reply->clear();
    }
  }
  return best_outcome;
}

[[nodiscard]] operator_message_wait_outcome_t wait_for_operator_message_outcome(
    const app_context_t &app, std::string_view marshal_session_id,
    std::uint64_t after_seq, std::string_view message_id,
    std::string *out_reply, std::string *out_failure) {
  if (out_reply)
    out_reply->clear();
  if (out_failure)
    out_failure->clear();
  constexpr auto kWait = std::chrono::milliseconds(2500);
  constexpr useconds_t kPollUs = 50 * 1000;
  const auto deadline = std::chrono::steady_clock::now() + kWait;
  while (std::chrono::steady_clock::now() < deadline) {
    std::string events_text{};
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(
        cuwacunu::hero::marshal::marshal_session_events_path(
            app.defaults.marshal_root, marshal_session_id),
        &events_text, &ignored);
    const operator_message_wait_outcome_t outcome =
        find_operator_message_wait_outcome(events_text, after_seq, message_id,
                                           out_reply, out_failure);
    if (outcome != operator_message_wait_outcome_t::Pending)
      return outcome;
    ::usleep(kPollUs);
  }
  return operator_message_wait_outcome_t::Pending;
}

[[nodiscard]] std::string extract_last_nonempty_line(std::string_view text) {
  std::istringstream in{std::string(text)};
  std::string line{};
  std::string last{};
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(line);
    if (!trimmed.empty()) {
      last = trimmed;
    }
  }
  return last;
}

[[nodiscard]] std::string normalize_codex_reply_text(std::string_view raw) {
  const std::string trimmed = trim_ascii(raw);
  if (trimmed.empty())
    return {};
  if (trimmed.front() == '"') {
    std::string parsed{};
    std::size_t end = 0;
    if (parse_json_string_at(trimmed, 0, &parsed, &end) &&
        trim_ascii(trimmed.substr(end)).empty()) {
      return trim_ascii(parsed);
    }
  }
  return trimmed;
}

[[nodiscard]] std::string build_live_operator_prompt(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::string_view message, bool include_briefing_context,
    std::string_view briefing_text) {
  std::ostringstream out;
  out << "You are inside an active Marshal Hero session conversation.\n"
      << "An operator has sent a new live message.\n\n"
      << "Respond directly to the operator in plain text.\n"
      << "Do not emit JSON.\n"
      << "Do not launch or stop campaigns yourself in this turn.\n"
      << "Do not mutate files in this turn.\n"
      << "If the operator suggests a future change, acknowledge how it will "
         "affect later work.\n\n"
      << "Current session state:\n"
      << "- lifecycle: " << loop.lifecycle << "\n"
      << "- work_gate: " << loop.work_gate << "\n"
      << "- activity: " << loop.activity << "\n"
      << "- campaign_status: " << loop.campaign_status << "\n";
  if (!trim_ascii(loop.campaign_cursor).empty()) {
    out << "- campaign_cursor: " << loop.campaign_cursor << "\n";
  }
  if (!trim_ascii(loop.status_detail).empty()) {
    out << "- status_detail: " << loop.status_detail << "\n";
  }
  out << "\nOperator message:\n" << trim_ascii(message) << "\n";
  if (include_briefing_context && !trim_ascii(briefing_text).empty()) {
    out << "\nSession briefing context:\n" << briefing_text;
    if (briefing_text.back() != '\n')
      out << "\n";
  }
  return out.str();
}

void append_live_operator_conversation_to_memory(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::string_view message, std::string_view reply) {
  const std::string trimmed_message = trim_ascii(message);
  const std::string trimmed_reply = trim_ascii(reply);
  if (trimmed_message.empty() && trimmed_reply.empty())
    return;
  std::ostringstream note;
  note << "\n\n## Live Operator Conversation\n\n";
  if (!trimmed_message.empty()) {
    note << "Operator: " << trimmed_message << "\n";
  }
  if (!trimmed_reply.empty()) {
    note << "Marshal: " << trimmed_reply << "\n";
  }
  std::string ignored{};
  (void)cuwacunu::hero::runtime::append_text_file(loop.memory_path, note.str(),
                                                  &ignored);
}

[[nodiscard]] bool run_live_operator_message_with_codex(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string_view message, std::string *out_reply, std::string *out_warning,
    std::string *error) {
  if (error)
    error->clear();
  if (out_reply)
    out_reply->clear();
  if (out_warning)
    out_warning->clear();
  if (!loop) {
    if (error)
      *error = "live operator message session pointer is null";
    return false;
  }
  const std::string trimmed_message = trim_ascii(message);
  if (trimmed_message.empty()) {
    if (error)
      *error = "live operator message requires non-empty text";
    return false;
  }
  if (!rewrite_marshal_session_briefing(app, *loop, error))
    return false;

  marshal_session_workspace_context_t workspace_context{};
  if (!load_marshal_session_workspace_context(app, *loop, &workspace_context,
                                              error)) {
    return false;
  }

  const std::filesystem::path persisted_codex_binary_path =
      resolve_marshal_codex_binary_path(workspace_context.repo_root,
                                        loop->resolved_marshal_codex_binary);
  const std::string persisted_codex_binary =
      trim_ascii(persisted_codex_binary_path.string());
  const std::string persisted_codex_model =
      trim_ascii(loop->resolved_marshal_codex_model);
  const std::string persisted_codex_reasoning_effort =
      trim_ascii(loop->resolved_marshal_codex_reasoning_effort);
  if (persisted_codex_binary.empty()) {
    if (error)
      *error = "marshal session is missing resolved_marshal_codex_binary";
    return false;
  }
  if (!path_is_executable(persisted_codex_binary_path)) {
    if (error) {
      *error = "resolved_marshal_codex_binary is not executable: " +
               persisted_codex_binary;
    }
    return false;
  }
  if (persisted_codex_model.empty()) {
    if (error)
      *error = "marshal session is missing resolved_marshal_codex_model";
    return false;
  }
  if (persisted_codex_reasoning_effort.empty()) {
    if (error) {
      *error =
          "marshal session is missing resolved_marshal_codex_reasoning_effort";
    }
    return false;
  }
  loop->resolved_marshal_codex_binary = persisted_codex_binary;

  const std::string config_args = json_array_from_strings(
      {"--global-config", app.global_config_path.string(), "--config",
       loop->config_policy_path});
  const std::string runtime_args = json_array_from_strings(
      {"--global-config", app.global_config_path.string()});
  const std::string lattice_args = runtime_args;
  const std::string enabled_config_tools = json_array_from_strings(
      {"hero.config.default.list", "hero.config.default.read",
       "hero.config.objective.list", "hero.config.objective.read",
       "hero.config.temp.list", "hero.config.temp.read"});
  const std::string enabled_runtime_tools = json_array_from_strings(
      {"hero.runtime.get_campaign", "hero.runtime.get_job",
       "hero.runtime.list_jobs", "hero.runtime.tail_log",
       "hero.runtime.tail_trace"});
  const std::string enabled_lattice_tools = json_array_from_strings(
      {"hero.lattice.list_facts", "hero.lattice.get_fact",
       "hero.lattice.list_views", "hero.lattice.get_view"});

  auto append_common_codex_mcp_args = [&](std::vector<std::string> *argv) {
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-config.enabled=true");
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-config.command=" +
                    json_quote(workspace_context.config_hero_binary.string()));
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
                    json_quote(workspace_context.runtime_hero_binary.string()));
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
                    json_quote(workspace_context.lattice_hero_binary.string()));
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.args=" + lattice_args);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.enabled_tools=" +
                    enabled_lattice_tools);
    argv->push_back("-c");
    argv->push_back("mcp_servers.hero-lattice.startup_timeout_sec=30");
  };

  std::string briefing_text{};
  {
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(loop->briefing_path,
                                                  &briefing_text, &ignored);
  }
  const std::string resume_prompt =
      build_live_operator_prompt(*loop, trimmed_message, false, "");
  auto build_fresh_prompt = [&]() {
    const std::string replacement_rebrief =
        build_replacement_thread_rebrief_packet(
            *loop,
            trim_ascii(loop->last_resume_error).empty()
                ? "fresh live operator thread bootstrap"
                : "codex resume failed: " + loop->last_resume_error,
            briefing_text);
    return build_live_operator_prompt(*loop, trimmed_message, true,
                                      replacement_rebrief);
  };

  auto run_codex_attempt = [&](bool resume_mode, std::string_view prompt_text,
                               std::string *out_attempt_reply,
                               std::string *out_session_id,
                               std::string *out_attempt_error) -> bool {
    if (out_attempt_error)
      out_attempt_error->clear();
    if (out_attempt_reply)
      out_attempt_reply->clear();
    if (out_session_id)
      out_session_id->clear();

    scoped_temp_path_t prompt_path{};
    if (!write_temp_text_artifact("hero_marshal_operator_prompt", "txt",
                                  prompt_text, &prompt_path,
                                  out_attempt_error)) {
      return false;
    }
    scoped_temp_path_t reply_path{};
    if (!write_temp_text_artifact("hero_marshal_operator_reply", "txt", "",
                                  &reply_path, out_attempt_error)) {
      return false;
    }

    std::vector<std::string> argv{};
    argv.reserve(48);
    argv.push_back(persisted_codex_binary);
    argv.push_back("exec");
    argv.push_back("--json");
    if (resume_mode) {
      argv.push_back("resume");
    } else {
      argv.push_back("-s");
      argv.push_back("read-only");
      argv.push_back("--color");
      argv.push_back("never");
    }
    argv.push_back("-m");
    argv.push_back(persisted_codex_model);
    argv.push_back("-c");
    argv.push_back("model_reasoning_effort=" +
                   json_quote(persisted_codex_reasoning_effort));
    append_common_codex_mcp_args(&argv);
    argv.push_back("-o");
    argv.push_back(reply_path.path.string());
    if (resume_mode) {
      const std::string thread_id = trim_ascii(loop->current_thread_id);
      if (thread_id.empty()) {
        if (out_attempt_error)
          *out_attempt_error =
              "live operator message resume requires current_thread_id";
        return false;
      }
      argv.push_back(thread_id);
    }
    argv.push_back("-");

    int exit_code = -1;
    std::string invoke_error{};
    loop->codex_stdout_path =
        cuwacunu::hero::marshal::marshal_session_codex_stdout_path(
            app.defaults.marshal_root, loop->marshal_session_id)
            .string();
    loop->codex_stderr_path =
        cuwacunu::hero::marshal::marshal_session_codex_stderr_path(
            app.defaults.marshal_root, loop->marshal_session_id)
            .string();
    const std::filesystem::path stdout_path(loop->codex_stdout_path);
    const std::filesystem::path stderr_jsonl_path(loop->codex_stderr_path);
    const std::filesystem::path stderr_capture_path =
        stderr_jsonl_path.parent_path() /
        (stderr_jsonl_path.filename().string() + ".tmp");
    const std::filesystem::path checkpoint_pid_path =
        marshal_session_checkpoint_pid_path(*loop);
    remove_file_noexcept(stderr_capture_path);
    remove_file_noexcept(marshal_session_compat_codex_session_log_path(*loop));

    const bool invoked = run_command_with_stdio_and_timeout(
        argv, prompt_path.path, stdout_path, stderr_capture_path,
        &workspace_context.repo_root, nullptr,
        workspace_context.marshal_codex_timeout_sec, &checkpoint_pid_path,
        &exit_code, &invoke_error);

    std::string stderr_text{};
    std::string stderr_finalize_error{};
    if (!cuwacunu::hero::runtime::read_text_file(
            stderr_capture_path, &stderr_text, &stderr_finalize_error)) {
      std::error_code ec{};
      if (std::filesystem::exists(stderr_capture_path, ec) && !ec) {
        remove_file_noexcept(stderr_capture_path);
        if (out_attempt_error) {
          *out_attempt_error =
              "codex stderr capture read failed: " + stderr_finalize_error;
        }
        return false;
      }
      stderr_text.clear();
    }
    if (!cuwacunu::hero::runtime::write_text_file_atomic(
            stderr_jsonl_path,
            encode_text_lines_as_jsonl("stderr", stderr_text),
            &stderr_finalize_error)) {
      remove_file_noexcept(stderr_capture_path);
      if (out_attempt_error) {
        *out_attempt_error = "codex stderr capture finalization failed: " +
                             stderr_finalize_error;
      }
      return false;
    }
    remove_file_noexcept(stderr_capture_path);
    if (!stderr_text.empty() && out_session_id) {
      *out_session_id = extract_current_thread_id_from_log(stderr_text);
    }
    if (!invoked) {
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

    std::string reply_text{};
    std::string reply_error{};
    (void)cuwacunu::hero::runtime::read_text_file(reply_path.path, &reply_text,
                                                  &reply_error);
    reply_text = normalize_codex_reply_text(reply_text);
    if (reply_text.empty()) {
      std::string stdout_text{};
      std::string ignored{};
      (void)cuwacunu::hero::runtime::read_text_file(stdout_path, &stdout_text,
                                                    &ignored);
      reply_text =
          normalize_codex_reply_text(extract_last_nonempty_line(stdout_text));
      if (reply_text.empty()) {
        reply_text = normalize_codex_reply_text(stdout_text);
      }
    }
    reply_text = trim_ascii(reply_text);
    if (reply_text.empty()) {
      if (out_attempt_error) {
        *out_attempt_error =
            "codex operator reply was empty after successful execution";
      }
      return false;
    }
    if (out_attempt_reply)
      *out_attempt_reply = std::move(reply_text);
    return true;
  };

  const std::string prior_thread_id = trim_ascii(loop->current_thread_id);
  const bool had_prior_thread = !prior_thread_id.empty();
  if (had_prior_thread) {
    loop->codex_continuity = "resuming";
    std::string resume_reply{};
    std::string resumed_thread_id{};
    std::string resume_error{};
    if (run_codex_attempt(true, resume_prompt, &resume_reply,
                          &resumed_thread_id, &resume_error)) {
      loop->codex_continuity = "attached";
      loop->last_resume_error.clear();
      if (!trim_ascii(resumed_thread_id).empty()) {
        loop->current_thread_id = trim_ascii(resumed_thread_id);
        if (loop->thread_lineage.empty() ||
            loop->thread_lineage.back() != loop->current_thread_id) {
          loop->thread_lineage.push_back(loop->current_thread_id);
        }
      }
      if (out_reply)
        *out_reply = std::move(resume_reply);
      return true;
    }
    loop->codex_continuity = "resume_failed";
    loop->last_resume_error = resume_error;
    (void)append_marshal_session_event(
        loop, "codex", "codex.resume_failed",
        "{\"previous_thread_id\":" + json_quote(prior_thread_id) +
            ",\"failure_kind\":" +
            json_quote("operator_message_resume_failed") + ",\"stderr_path\":" +
            json_quote(loop->codex_stderr_path) + ",\"fallback_started\":true}",
        nullptr, 0);
    append_warning_text(out_warning,
                        build_resume_degraded_warning(
                            *loop, loop->checkpoint_count,
                            "operator_message_resume_failed", resume_error));
  }

  std::string fresh_reply{};
  std::string fresh_thread_id{};
  std::string fresh_error{};
  const std::string fresh_prompt = build_fresh_prompt();
  if (!run_codex_attempt(false, fresh_prompt, &fresh_reply, &fresh_thread_id,
                         &fresh_error)) {
    if (error)
      *error = fresh_error;
    return false;
  }
  loop->codex_continuity = had_prior_thread ? "restarted" : "attached";
  loop->last_resume_error.clear();
  if (!trim_ascii(fresh_thread_id).empty()) {
    loop->current_thread_id = trim_ascii(fresh_thread_id);
    if (loop->thread_lineage.empty() ||
        loop->thread_lineage.back() != loop->current_thread_id) {
      loop->thread_lineage.push_back(loop->current_thread_id);
    }
  } else {
    loop->current_thread_id.clear();
    append_warning_text(
        out_warning,
        "fresh live operator reply completed without a persisted session id; "
        "future live replies may need another fresh restart");
  }
  if (out_reply)
    *out_reply = std::move(fresh_reply);
  return true;
}

[[nodiscard]] bool process_pending_live_operator_message(
    app_context_t *app, cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string *out_warning, std::string *error) {
  if (error)
    error->clear();
  if (out_warning)
    out_warning->clear();
  if (!app || !loop) {
    if (error)
      *error = "live operator message inputs are null";
    return false;
  }

  auto it = std::find_if(loop->pending_operator_messages.begin(),
                         loop->pending_operator_messages.end(),
                         [&](const auto &message) {
                           return marshal_operator_message_has_text(message) &&
                                  message.delivery_status == "received";
                         });
  if (it == loop->pending_operator_messages.end())
    return true;

  auto &pending_message = *it;
  pending_message.delivery_attempts += 1;
  pending_message.delivery_mode = "live";
  const std::string message = trim_ascii(pending_message.text);
  const std::string prior_activity = trim_ascii(loop->activity);
  const std::string prior_status_detail = loop->status_detail;
  loop->activity = "handling_message";
  loop->status_detail = "delivering operator message into live codex thread";
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!write_marshal_session(*app, *loop, error))
    return false;

  std::string reply_text{};
  std::string live_warning{};
  std::string live_error{};
  if (!run_live_operator_message_with_codex(*app, loop, message, &reply_text,
                                            &live_warning, &live_error)) {
    append_warning_text(&live_warning,
                        "live operator message delivery failed: " + live_error);
    pending_message.handled_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    pending_message.delivery_status = "failed";
    pending_message.last_error = live_error;
    pending_message.thread_id_at_delivery = trim_ascii(loop->current_thread_id);
    std::string ignored{};
    (void)append_marshal_session_event(loop, "marshal", "warning.emitted",
                                       live_warning, &ignored);
    (void)append_operator_message_event(
        loop, "marshal", "operator.message_handled", pending_message,
        "live delivery failed", "", live_error, &ignored);
    it = loop->pending_operator_messages.erase(it);
    (void)it;
    if (cuwacunu::hero::marshal::marshal_session_has_active_campaign(*loop)) {
      loop->activity = "awaiting_campaign_fact";
    } else if (!prior_activity.empty() &&
               prior_activity != "handling_message") {
      loop->activity = prior_activity;
    } else {
      loop->activity = "quiet";
    }
    loop->status_detail = prior_status_detail;
    loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    if (!write_marshal_session(*app, *loop, error))
      return false;
    persist_marshal_session_warning_best_effort(*app, loop, live_warning);
    if (out_warning)
      *out_warning = live_warning;
    return true;
  }

  pending_message.thread_id_at_delivery = trim_ascii(loop->current_thread_id);
  pending_message.delivered_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  pending_message.delivery_status = "delivered";
  pending_message.last_error.clear();
  append_live_operator_conversation_to_memory(*loop, message, reply_text);
  if (!append_operator_message_event(
          loop, "marshal", "operator.message_delivered", pending_message,
          "live thread delivery completed", reply_text, "", error)) {
    return false;
  }
  pending_message.handled_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  pending_message.delivery_status = "handled";
  if (!append_operator_message_turn(*loop, pending_message, reply_text,
                                    error)) {
    return false;
  }
  if (!append_operator_message_event(
          loop, "marshal", "operator.message_handled", pending_message,
          "live reply handled", reply_text, "", error)) {
    return false;
  }
  loop->pending_operator_messages.erase(it);
  if (cuwacunu::hero::marshal::marshal_session_has_active_campaign(*loop)) {
    loop->activity = "awaiting_campaign_fact";
  } else if (!prior_activity.empty() && prior_activity != "handling_message") {
    loop->activity = prior_activity;
  } else {
    loop->activity = "quiet";
  }
  loop->status_detail = prior_status_detail;
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!live_warning.empty()) {
    append_warning_text(&loop->last_warning, live_warning);
  }
  if (!write_marshal_session(*app, *loop, error))
    return false;
  if (!live_warning.empty()) {
    persist_marshal_session_warning_best_effort(*app, loop, live_warning);
    if (out_warning)
      *out_warning = live_warning;
  }
  return true;
}

[[nodiscard]] std::string
marshal_session_runner_startup_detail(const app_context_t &app,
                                      std::string_view marshal_session_id) {
  const std::filesystem::path stderr_path =
      marshal_session_runner_stderr_path(app, marshal_session_id);
  const std::filesystem::path stdout_path =
      marshal_session_runner_stdout_path(app, marshal_session_id);
  std::string stderr_text{};
  std::string stdout_text{};
  std::string ignored{};
  (void)cuwacunu::hero::runtime::read_text_file(stderr_path, &stderr_text,
                                                &ignored);
  (void)cuwacunu::hero::runtime::read_text_file(stdout_path, &stdout_text,
                                                &ignored);

  std::string detail = "session runner exited before becoming active";
  std::string excerpt = extract_last_nonempty_line(stderr_text);
  if (excerpt.empty()) {
    excerpt = extract_last_nonempty_line(stdout_text);
  }
  if (!excerpt.empty()) {
    detail.append(": ");
    detail.append(excerpt);
  }
  detail.append("; runner_stderr=");
  detail.append(stderr_path.string());
  return detail;
}

[[nodiscard]] bool wait_for_session_runner_startup(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *session,
    bool have_runner_identity, std::string *error) {
  if (error)
    error->clear();
  if (!session) {
    if (error)
      *error = "marshal session runner startup session pointer is null";
    return false;
  }

  constexpr auto kStartupWait = std::chrono::milliseconds(2000);
  constexpr useconds_t kStartupPollUs = 50 * 1000;
  const auto deadline = std::chrono::steady_clock::now() + kStartupWait;
  while (std::chrono::steady_clock::now() < deadline) {
    if (marshal_session_events_contain_event(app, session->marshal_session_id,
                                             "session.runner_active")) {
      return true;
    }
    if (have_runner_identity) {
      bool runner_alive = true;
      std::string runner_alive_error{};
      if (marshal_session_runner_alive(*session, &runner_alive,
                                       &runner_alive_error) &&
          !runner_alive) {
        std::string detail = marshal_session_runner_startup_detail(
            app, session->marshal_session_id);
        if (!runner_alive_error.empty()) {
          detail.append("; runner_probe=");
          detail.append(runner_alive_error);
        }
        session->lifecycle = "terminal";
        session->activity = "quiet";
        session->status_detail = detail;
        session->work_gate = "open";
        session->finish_reason = "failed";
        session->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
        session->updated_at_ms = *session->finished_at_ms;
        clear_marshal_session_campaign_linkage(session);
        clear_marshal_session_runner_identity(session);

        std::string persist_error{};
        if (!write_marshal_session(app, *session, &persist_error)) {
          if (error) {
            *error =
                detail +
                "; additionally failed to persist runner startup failure: " +
                persist_error;
          }
          return false;
        }
        std::string ignored{};
        (void)append_marshal_session_event(
            &session, "marshal", "session.runner_failed", detail, &ignored);
        if (error)
          *error = detail;
        return false;
      }
    }
    ::usleep(kStartupPollUs);
  }
  return true;
}

[[nodiscard]] bool launch_session_runner_process(
    const app_context_t &app, std::string_view marshal_session_id,
    std::uint64_t *out_runner_pid, std::string *error) {
  if (error)
    error->clear();
  if (out_runner_pid)
    *out_runner_pid = 0;

  const std::filesystem::path runner_stdout_path =
      marshal_session_runner_stdout_path(app, marshal_session_id);
  const std::filesystem::path runner_stderr_path =
      marshal_session_runner_stderr_path(app, marshal_session_id);
  const std::filesystem::path runner_binary =
      resolve_marshal_session_runner_binary(app);
  if (runner_binary.empty()) {
    if (error) {
      *error = "cannot resolve marshal session runner binary";
    }
    return false;
  }
  std::error_code dir_ec{};
  std::filesystem::create_directories(runner_stdout_path.parent_path(), dir_ec);
  if (dir_ec) {
    if (error) {
      *error = "cannot create marshal session runner logs dir: " +
               runner_stdout_path.parent_path().string();
    }
    return false;
  }

  int pipe_fds[2]{-1, -1};
#ifdef O_CLOEXEC
  if (::pipe2(pipe_fds, O_CLOEXEC) != 0) {
    if (error)
      *error = "pipe2 failed";
    return false;
  }
#else
  if (::pipe(pipe_fds) != 0) {
    if (error)
      *error = "pipe failed";
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
    if (error)
      *error = msg;
    return false;
  }
  if (child == 0) {
    (void)::close(pipe_fds[0]);
    if (::setsid() < 0) {
      session_runner_launch_message_t message{};
      message.exec_errno = errno;
      write_session_runner_launch_message_noexcept(pipe_fds[1], message);
      _exit(127);
    }

    const pid_t grandchild = ::fork();
    if (grandchild < 0) {
      session_runner_launch_message_t message{};
      message.exec_errno = errno;
      write_session_runner_launch_message_noexcept(pipe_fds[1], message);
      _exit(127);
    }
    if (grandchild > 0)
      _exit(0);

    const int devnull = ::open("/dev/null", O_RDONLY);
    if (devnull >= 0) {
      (void)::dup2(devnull, STDIN_FILENO);
      if (devnull > STDERR_FILENO)
        (void)::close(devnull);
    }

    const int stdout_fd =
        ::open(runner_stdout_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stdout_fd < 0) {
      session_runner_launch_message_t message{};
      message.exec_errno = errno;
      write_session_runner_launch_message_noexcept(pipe_fds[1], message);
      _exit(127);
    }
    (void)::dup2(stdout_fd, STDOUT_FILENO);
    if (stdout_fd > STDERR_FILENO)
      (void)::close(stdout_fd);

    const int stderr_fd =
        ::open(runner_stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stderr_fd < 0) {
      session_runner_launch_message_t message{};
      message.exec_errno = errno;
      write_session_runner_launch_message_noexcept(pipe_fds[1], message);
      _exit(127);
    }
    (void)::dup2(stderr_fd, STDERR_FILENO);
    if (stderr_fd > STDERR_FILENO)
      (void)::close(stderr_fd);

    std::vector<std::string> args{};
    args.push_back(runner_binary.string());
    args.push_back("--session-runner");
    args.push_back("--marshal-session-id");
    args.push_back(std::string(marshal_session_id));
    args.push_back("--global-config");
    args.push_back(app.global_config_path.string());
    args.push_back("--hero-config");
    args.push_back(app.hero_config_path.string());

    std::vector<char *> argv{};
    argv.reserve(args.size() + 1);
    for (auto &arg : args)
      argv.push_back(arg.data());
    argv.push_back(nullptr);
    session_runner_launch_message_t message{};
    message.runner_pid = static_cast<std::uint64_t>(::getpid());
    write_session_runner_launch_message_noexcept(pipe_fds[1], message);
    ::execv(argv[0], argv.data());

    message.exec_errno = errno;
    write_session_runner_launch_message_noexcept(pipe_fds[1], message);
    _exit(127);
  }

  (void)::close(pipe_fds[1]);
  int exec_errno = 0;
  std::uint64_t launched_runner_pid = 0;
  session_runner_launch_message_t message{};
  for (;;) {
    const ssize_t n = ::read(pipe_fds[0], &message, sizeof(message));
    if (n == 0)
      break;
    if (n < 0) {
      if (errno == EINTR)
        continue;
      exec_errno = errno;
      break;
    }
    if (n != static_cast<ssize_t>(sizeof(message)))
      continue;
    if (message.runner_pid != 0)
      launched_runner_pid = message.runner_pid;
    if (message.exec_errno != 0)
      exec_errno = message.exec_errno;
  }
  (void)::close(pipe_fds[0]);
  int ignored_status = 0;
  while (::waitpid(child, &ignored_status, 0) < 0 && errno == EINTR) {
  }
  if (exec_errno != 0) {
    if (error) {
      *error = "cannot exec detached marshal session runner: errno=" +
               std::to_string(exec_errno);
    }
    return false;
  }
  if (out_runner_pid)
    *out_runner_pid = launched_runner_pid;
  if (launched_runner_pid == 0) {
    if (error)
      *error = "detached marshal session runner launch did not report a pid";
    return false;
  }
  return true;
}

[[nodiscard]] bool handle_tool_start_session(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error);
[[nodiscard]] bool handle_tool_list_sessions(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error);
[[nodiscard]] bool handle_tool_get_session(app_context_t *app,
                                           const std::string &arguments_json,
                                           std::string *out_structured,
                                           std::string *out_error);
[[nodiscard]] bool handle_tool_reconcile_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error);
[[nodiscard]] bool handle_tool_pause_session(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error);
[[nodiscard]] bool handle_tool_resume_session(app_context_t *app,
                                              const std::string &arguments_json,
                                              std::string *out_structured,
                                              std::string *out_error);
[[nodiscard]] bool handle_tool_message_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error);
[[nodiscard]] bool handle_tool_archive_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error);
[[nodiscard]] bool handle_tool_terminate_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error);

#define HERO_MARSHAL_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER)       \
  {NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
constexpr marshal_tool_descriptor_t kMarshalTools[] = {
#include "hero/marshal_hero/hero_marshal_tools.def"
};
#undef HERO_MARSHAL_TOOL

[[nodiscard]] const marshal_tool_descriptor_t *
find_marshal_tool_descriptor(std::string_view tool_name) {
  for (const auto &tool : kMarshalTools) {
    if (tool.name == tool_name)
      return &tool;
  }
  return nullptr;
}

[[nodiscard]] bool handle_tool_start_session(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_objective_dsl_path{};
  (void)extract_json_string_field(arguments_json, "marshal_objective_dsl_path",
                                  &marshal_objective_dsl_path);
  marshal_objective_dsl_path = trim_ascii(marshal_objective_dsl_path);
  marshal_start_session_overrides_t start_overrides{};
  if (!parse_start_session_overrides(arguments_json, &start_overrides,
                                     out_error)) {
    return false;
  }

  const std::filesystem::path source_marshal_objective_dsl_path =
      resolve_requested_marshal_objective_source_path(
          *app, marshal_objective_dsl_path, out_error);
  if (source_marshal_objective_dsl_path.empty())
    return false;

  cuwacunu::hero::marshal::marshal_session_record_t loop{};
  if (!initialize_marshal_session_record(*app,
                                         source_marshal_objective_dsl_path,
                                         start_overrides, &loop, out_error)) {
    return false;
  }

  std::vector<std::string> warnings{};
  const std::uint64_t checkpoint_index = 1;
  std::string input_checkpoint_json{};
  if (!build_marshal_bootstrap_input_checkpoint_json(
          *app, loop, checkpoint_index, &input_checkpoint_json, out_error)) {
    loop.lifecycle = "terminal";
    loop.activity = "quiet";
    loop.status_detail = *out_error;
    loop.work_gate = "open";
    loop.finish_reason = "failed";
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_marshal_session(*app, loop, &ignored);
    (void)append_marshal_session_event(&loop, "marshal", "checkpoint.failed",
                                       *out_error, &ignored);
    return false;
  }
  const std::filesystem::path input_checkpoint_path =
      cuwacunu::hero::marshal::marshal_session_input_checkpoint_path(
          app->defaults.marshal_root, loop.marshal_session_id,
          checkpoint_index);
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          input_checkpoint_path, input_checkpoint_json, out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          marshal_session_latest_input_checkpoint_path(loop),
          input_checkpoint_json, out_error)) {
    return false;
  }
  loop.lifecycle = "live";
  loop.activity = "planning";
  loop.status_detail = "waiting for first codex planning checkpoint";
  loop.work_gate = "open";
  loop.finish_reason = "none";
  loop.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop.last_input_checkpoint_path = input_checkpoint_path.string();
  clear_marshal_session_runner_identity(&loop);
  if (!write_marshal_session(*app, loop, out_error))
    return false;
  if (!append_marshal_session_event(&loop, "marshal", "checkpoint.staged",
                                    input_checkpoint_path.string(),
                                    out_error)) {
    return false;
  }

  std::string bookkeeping_error{};
  std::uint64_t runner_pid = 0;
  if (!launch_session_runner_process(*app, loop.marshal_session_id, &runner_pid,
                                     &bookkeeping_error)) {
    warnings.push_back("marshal session runner launch failed: " +
                       bookkeeping_error);
    loop.lifecycle = "terminal";
    loop.activity = "quiet";
    loop.status_detail = "session runner launch failed: " + bookkeeping_error;
    loop.work_gate = "open";
    loop.finish_reason = "failed";
    loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    loop.updated_at_ms = *loop.finished_at_ms;
    std::string ignored{};
    (void)write_marshal_session(*app, loop, &ignored);
    (void)append_marshal_session_event(
        &loop, "marshal", "session.runner_failed", bookkeeping_error, &ignored);
    *out_error = bookkeeping_error;
    return false;
  }
  bool runner_identity_captured = false;
  std::string identity_error{};
  if (!capture_process_identity_for_session(runner_pid, &loop,
                                            &identity_error)) {
    warnings.push_back("marshal session runner identity degraded: " +
                       identity_error);
  } else {
    runner_identity_captured = true;
    loop.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    std::string ignored{};
    (void)write_marshal_session(*app, loop, &ignored);
  }
  (void)append_marshal_session_event(&loop, "marshal", "session.runner_started",
                                     "detached marshal runner launched",
                                     nullptr);
  if (!wait_for_session_runner_startup(*app, &loop, runner_identity_captured,
                                       out_error)) {
    return false;
  }
  if (!warnings.empty()) {
    std::string combined_warning{};
    for (const std::string &warning : warnings) {
      append_warning_text(&combined_warning, warning);
    }
    persist_marshal_session_warning_best_effort(*app, &loop, combined_warning);
  }

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(loop.marshal_session_id) +
      ",\"session\":" + marshal_session_to_json(loop) +
      ",\"campaign_cursor\":\"\",\"campaign\":null" +
      ",\"warnings\":" + json_array_from_strings(warnings) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_list_sessions(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string lifecycle_filter{};
  std::size_t limit = 0;
  std::size_t offset = 0;
  bool newest_first = true;
  (void)extract_json_string_field(arguments_json, "lifecycle",
                                  &lifecycle_filter);
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);
  (void)extract_json_bool_field(arguments_json, "newest_first", &newest_first);
  lifecycle_filter = trim_ascii(lifecycle_filter);

  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> sessions{};
  if (!list_marshal_sessions_reconciled(*app, &sessions, out_error))
    return false;
  if (!lifecycle_filter.empty()) {
    sessions.erase(std::remove_if(sessions.begin(), sessions.end(),
                                  [&](const auto &session) {
                                    return session.lifecycle !=
                                           lifecycle_filter;
                                  }),
                   sessions.end());
  }
  std::sort(sessions.begin(), sessions.end(),
            [newest_first](const auto &a, const auto &b) {
              if (a.started_at_ms != b.started_at_ms) {
                return newest_first ? (a.started_at_ms > b.started_at_ms)
                                    : (a.started_at_ms < b.started_at_ms);
              }
              return newest_first
                         ? (a.marshal_session_id > b.marshal_session_id)
                         : (a.marshal_session_id < b.marshal_session_id);
            });

  const std::size_t total = sessions.size();
  const std::size_t off = std::min(offset, sessions.size());
  std::size_t count = limit;
  if (count == 0)
    count = sessions.size() - off;
  count = std::min(count, sessions.size() - off);

  std::ostringstream sessions_json;
  sessions_json << "[";
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0)
      sessions_json << ",";
    sessions_json << marshal_session_to_json(sessions[off + i]);
  }
  sessions_json << "]";

  *out_structured =
      "{\"marshal_session_id\":\"\",\"count\":" + std::to_string(count) +
      ",\"total\":" + std::to_string(total) +
      ",\"lifecycle\":" + json_quote(lifecycle_filter) +
      ",\"sessions\":" + sessions_json.str() + "}";
  return true;
}

[[nodiscard]] bool handle_tool_get_session(app_context_t *app,
                                           const std::string &arguments_json,
                                           std::string *out_structured,
                                           std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "get_session requires arguments.marshal_session_id";
    return false;
  }
  cuwacunu::hero::marshal::marshal_session_record_t session{};
  bool reconciled = false;
  if (!read_marshal_session_reconciled(*app, marshal_session_id, &session,
                                       &reconciled, out_error))
    return false;
  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"reconciled\":" + bool_json(reconciled) +
      ",\"session\":" + marshal_session_to_json(session) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_reconcile_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "reconcile_session requires arguments.marshal_session_id";
    return false;
  }
  cuwacunu::hero::marshal::marshal_session_record_t session{};
  bool reconciled = false;
  if (!read_marshal_session_reconciled(*app, marshal_session_id, &session,
                                       &reconciled, out_error)) {
    return false;
  }
  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"reconciled\":" + bool_json(reconciled) +
      ",\"session\":" + marshal_session_to_json(session) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_pause_session(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  bool force = false;
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  (void)extract_json_bool_field(arguments_json, "force", &force);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "pause_session requires arguments.marshal_session_id";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  bool reconciled = false;
  if (!read_marshal_session_reconciled(*app, marshal_session_id, &session,
                                       &reconciled, out_error))
    return false;
  (void)reconciled;
  if (is_marshal_session_terminal_state(session.lifecycle)) {
    *out_structured =
        "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
        ",\"session\":" + marshal_session_to_json(session) +
        ",\"warning\":\"\"}";
    return true;
  }
  if (session.lifecycle == "live" && session.work_gate != "open") {
    const std::string warning =
        "session is already paused with work_gate=" + session.work_gate;
    *out_structured =
        "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
        ",\"session\":" + marshal_session_to_json(session) +
        ",\"warning\":" + json_quote(warning) + "}";
    return true;
  }
  if (cuwacunu::hero::marshal::marshal_session_is_review_ready(session)) {
    const std::string warning =
        "session is already review-ready; use message_session to resume or "
        "terminate_session to close it";
    *out_structured =
        "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
        ",\"session\":" + marshal_session_to_json(session) +
        ",\"warning\":" + json_quote(warning) + "}";
    return true;
  }

  const std::string campaign_cursor = trim_ascii(session.campaign_cursor);
  std::string runtime_warning{};
  bool active_campaign_stop_pending = false;
  cancel_session_checkpoint_best_effort(session, force, &runtime_warning);
  if (!campaign_cursor.empty()) {
    cuwacunu::hero::runtime::runtime_campaign_record_t stopped_campaign{};
    if (!request_runtime_campaign_stop_for_marshal_action(
            *app, session, campaign_cursor, force, &stopped_campaign,
            &runtime_warning, out_error)) {
      return false;
    }
    active_campaign_stop_pending =
        !is_runtime_campaign_terminal_state(trim_ascii(stopped_campaign.state));
  }

  session.lifecycle = "live";
  session.activity = "quiet";
  session.status_detail =
      active_campaign_stop_pending
          ? "pause_session requested by operator; active runtime campaign stop "
            "in progress"
          : "pause_session requested by operator";
  session.work_gate = "operator_pause";
  session.finish_reason = "none";
  session.campaign_status = active_campaign_stop_pending ? "stopping" : "none";
  session.finished_at_ms.reset();
  session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!active_campaign_stop_pending) {
    clear_marshal_session_campaign_linkage(&session);
  }
  clear_marshal_session_runner_identity(&session);
  if (!write_marshal_session(*app, session, out_error))
    return false;
  if (!append_marshal_session_event(&session, "operator", "work.blocked",
                                    session.status_detail, out_error)) {
    return false;
  }
  if (!runtime_warning.empty()) {
    persist_marshal_session_warning_best_effort(*app, &session,
                                                runtime_warning);
  }

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"session\":" + marshal_session_to_json(session) +
      ",\"warning\":" + json_quote(runtime_warning) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_resume_session(app_context_t *app,
                                              const std::string &arguments_json,
                                              std::string *out_structured,
                                              std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  std::string response_path_arg{};
  std::string response_sig_path_arg{};
  std::string clarification_answer_path_arg{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  (void)extract_json_string_field(arguments_json, "governance_resolution_path",
                                  &response_path_arg);
  (void)extract_json_string_field(
      arguments_json, "governance_resolution_sig_path", &response_sig_path_arg);
  (void)extract_json_string_field(arguments_json, "clarification_answer_path",
                                  &clarification_answer_path_arg);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "resume_session requires arguments.marshal_session_id";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  bool reconciled = false;
  if (!read_marshal_session_reconciled(*app, marshal_session_id, &session,
                                       &reconciled, out_error))
    return false;
  (void)reconciled;
  if (is_marshal_session_terminal_state(session.lifecycle)) {
    *out_structured =
        "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
        ",\"session\":" + marshal_session_to_json(session) +
        ",\"warnings\":[]}";
    return true;
  }

  const bool has_governance_resolution =
      !trim_ascii(response_path_arg).empty() ||
      !trim_ascii(response_sig_path_arg).empty();
  const bool has_clarification_answer =
      !trim_ascii(clarification_answer_path_arg).empty();
  if (has_governance_resolution && has_clarification_answer) {
    *out_error = "resume_session cannot accept both governance resolution and "
                 "clarification answer artifacts";
    return false;
  }

  auto stage_followup_and_launch =
      [&](std::string input_checkpoint_json, std::string_view status_detail,
          std::string_view event_detail,
          std::vector<std::string> *warnings) -> bool {
    const std::uint64_t checkpoint_index = session.checkpoint_count + 1;
    const std::filesystem::path input_checkpoint_path =
        cuwacunu::hero::marshal::marshal_session_input_checkpoint_path(
            app->defaults.marshal_root, session.marshal_session_id,
            checkpoint_index);
    if (!cuwacunu::hero::runtime::write_text_file_atomic(
            input_checkpoint_path, input_checkpoint_json, out_error)) {
      return false;
    }
    if (!cuwacunu::hero::runtime::write_text_file_atomic(
            marshal_session_latest_input_checkpoint_path(session),
            input_checkpoint_json, out_error)) {
      return false;
    }
    session.lifecycle = "live";
    session.activity = "planning";
    session.status_detail = std::string(status_detail);
    session.work_gate = "open";
    session.finish_reason = "none";
    session.finished_at_ms.reset();
    session.last_input_checkpoint_path = input_checkpoint_path.string();
    for (auto &pending_message : session.pending_operator_messages) {
      if (!marshal_operator_message_has_text(pending_message))
        continue;
      pending_message.delivered_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      pending_message.delivery_status = "handled";
      pending_message.handled_at_ms = pending_message.delivered_at_ms;
      std::string ignored{};
      (void)append_operator_message_event(
          &session, "marshal", "operator.message_delivered", pending_message,
          "staged into resumed checkpoint", "", "", &ignored);
      (void)append_operator_message_event(
          &session, "marshal", "operator.message_handled", pending_message,
          "staged into resumed checkpoint", "", "", &ignored);
    }
    session.pending_operator_messages.clear();
    session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    clear_marshal_session_runner_identity(&session);
    if (!write_marshal_session(*app, session, out_error))
      return false;
    if (!append_marshal_session_event(&session, "marshal", "checkpoint.staged",
                                      std::string(event_detail), out_error)) {
      return false;
    }
    std::string runner_error{};
    std::uint64_t runner_pid = 0;
    if (!launch_session_runner_process(*app, session.marshal_session_id,
                                       &runner_pid, &runner_error)) {
      if (warnings) {
        warnings->push_back("marshal session runner launch failed: " +
                            runner_error);
      }
      session.lifecycle = "terminal";
      session.activity = "quiet";
      session.status_detail = "session runner launch failed: " + runner_error;
      session.work_gate = "open";
      session.finish_reason = "failed";
      session.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      session.updated_at_ms = *session.finished_at_ms;
      std::string ignored{};
      (void)write_marshal_session(*app, session, &ignored);
      (void)append_marshal_session_event(
          &session, "marshal", "session.runner_failed", runner_error, &ignored);
      *out_error = runner_error;
      return false;
    }
    bool runner_identity_captured = false;
    std::string identity_error{};
    if (!capture_process_identity_for_session(runner_pid, &session,
                                              &identity_error)) {
      if (warnings) {
        warnings->push_back("marshal session runner identity degraded: " +
                            identity_error);
      }
    } else {
      runner_identity_captured = true;
      session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      std::string ignored{};
      (void)write_marshal_session(*app, session, &ignored);
    }
    (void)append_marshal_session_event(
        &session, "marshal", "session.runner_started",
        "detached marshal runner launched", nullptr);
    if (!wait_for_session_runner_startup(*app, &session,
                                         runner_identity_captured, out_error)) {
      return false;
    }
    return true;
  };

  std::vector<std::string> warnings{};
  if (has_governance_resolution) {
    if (session.lifecycle != "live" || session.work_gate != "governance") {
      *out_error = "resume_session governance path requires "
                   "session.lifecycle=live and work_gate=governance";
      return false;
    }
    const std::filesystem::path response_path =
        trim_ascii(response_path_arg).empty()
            ? cuwacunu::hero::marshal::
                  marshal_session_human_governance_resolution_latest_path(
                      app->defaults.marshal_root, session.marshal_session_id)
            : std::filesystem::path(resolve_path_from_base_folder(
                  app->global_config_path.parent_path().string(),
                  trim_ascii(response_path_arg)));
    const std::filesystem::path response_sig_path =
        trim_ascii(response_sig_path_arg).empty()
            ? cuwacunu::hero::marshal::
                  marshal_session_human_governance_resolution_latest_sig_path(
                      app->defaults.marshal_root, session.marshal_session_id)
            : std::filesystem::path(resolve_path_from_base_folder(
                  app->global_config_path.parent_path().string(),
                  trim_ascii(response_sig_path_arg)));

    cuwacunu::hero::human::human_resolution_record_t response{};
    std::string signature_hex{};
    if (!load_verified_human_resolution(*app, session, response_path,
                                        response_sig_path, &response,
                                        &signature_hex, out_error)) {
      return false;
    }
    append_human_resolution_note(session, response, response_path,
                                 response_sig_path);
    session.last_warning.clear();
    session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();

    std::string event_detail = response_path.string();
    if (!response.operator_id.empty()) {
      event_detail.append(" by ");
      event_detail.append(response.operator_id);
    }
    if (!append_marshal_session_event(&session, "governance", "work.unblocked",
                                      event_detail, out_error)) {
      return false;
    }

    if (response.resolution_kind == "grant") {
      if (response.governance_kind == "authority_expansion" &&
          response.grant_allow_default_write) {
        session.authority_scope = "objective_plus_defaults";
      } else if (response.governance_kind == "launch_budget_expansion") {
        session.max_campaign_launches +=
            response.grant_additional_campaign_launches;
        session.remaining_campaign_launches +=
            response.grant_additional_campaign_launches;
      }
    } else if (response.resolution_kind == "terminate") {
      session.lifecycle = "terminal";
      session.activity = "quiet";
      session.status_detail =
          "session terminated by governance resolution: " + response.reason;
      session.work_gate = "open";
      session.finish_reason = "terminated";
      session.last_codex_action = "terminate";
      session.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      session.updated_at_ms = *session.finished_at_ms;
      clear_marshal_session_campaign_linkage(&session);
      clear_marshal_session_runner_identity(&session);
      if (!write_marshal_session(*app, session, out_error))
        return false;
      if (!append_marshal_session_event(&session, "governance",
                                        "session.finished",
                                        session.status_detail, out_error)) {
        return false;
      }
      *out_structured =
          "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
          ",\"session\":" + marshal_session_to_json(session) +
          ",\"warnings\":[]}";
      return true;
    }

    const std::uint64_t checkpoint_index = session.checkpoint_count + 1;
    const std::filesystem::path prior_input_checkpoint_path =
        cuwacunu::hero::marshal::marshal_session_latest_input_checkpoint_path(
            app->defaults.marshal_root, session.marshal_session_id);
    std::string input_checkpoint_json{};
    if (!build_marshal_governance_followup_input_checkpoint_json(
            *app, session, checkpoint_index, prior_input_checkpoint_path,
            response_path, response_sig_path, response, &input_checkpoint_json,
            out_error)) {
      return false;
    }
    if (!stage_followup_and_launch(
            std::move(input_checkpoint_json),
            "waiting for codex planning checkpoint after governance resolution",
            prior_input_checkpoint_path.string(), &warnings)) {
      return false;
    }
  } else if (has_clarification_answer) {
    if (session.lifecycle != "live" || session.work_gate != "clarification") {
      *out_error = "resume_session clarification path requires "
                   "session.lifecycle=live and work_gate=clarification";
      return false;
    }
    const std::filesystem::path answer_path =
        std::filesystem::path(resolve_path_from_base_folder(
            app->global_config_path.parent_path().string(),
            trim_ascii(clarification_answer_path_arg)));
    std::string answer_json{};
    if (!cuwacunu::hero::runtime::read_text_file(answer_path, &answer_json,
                                                 out_error)) {
      return false;
    }
    std::string answer{};
    if (!parse_human_clarification_answer_json(
            answer_json, session.marshal_session_id, session.checkpoint_count,
            &answer, out_error)) {
      return false;
    }
    const std::uint64_t checkpoint_index = session.checkpoint_count + 1;
    const std::filesystem::path prior_input_checkpoint_path =
        cuwacunu::hero::marshal::marshal_session_latest_input_checkpoint_path(
            app->defaults.marshal_root, session.marshal_session_id);
    std::string input_checkpoint_json{};
    if (!build_marshal_clarification_followup_checkpoint_json(
            *app, session, checkpoint_index, prior_input_checkpoint_path,
            answer_path, answer, &input_checkpoint_json, out_error)) {
      return false;
    }
    append_memory_note(session, checkpoint_index,
                       "Human clarification answer: " + answer);
    if (!append_marshal_session_event(&session, "operator", "work.unblocked",
                                      answer, out_error)) {
      return false;
    }
    if (!stage_followup_and_launch(
            std::move(input_checkpoint_json),
            "waiting for codex planning checkpoint after human clarification",
            prior_input_checkpoint_path.string(), &warnings)) {
      return false;
    }
  } else {
    if (session.lifecycle != "live" || session.work_gate != "operator_pause") {
      *out_error = "resume_session requires an operator-paused session when no "
                   "human artifacts are provided";
      return false;
    }
    const std::string campaign_cursor = trim_ascii(session.campaign_cursor);
    if (!campaign_cursor.empty()) {
      cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
      if (!read_runtime_campaign_direct(*app, campaign_cursor, &campaign,
                                        out_error)) {
        return false;
      }
      const std::string observed_state = observed_campaign_state(campaign);
      if (!is_runtime_campaign_terminal_state(observed_state)) {
        *out_error =
            "resume_session cannot reopen an operator-paused session while "
            "the retained active runtime campaign stop is still in progress";
        return false;
      }
      clear_marshal_session_campaign_linkage(&session);
    }
    session.lifecycle = "live";
    session.activity = "planning";
    session.status_detail = "resume_session requested by operator";
    session.work_gate = "open";
    session.finish_reason = "none";
    session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    clear_marshal_session_runner_identity(&session);
    if (!write_marshal_session(*app, session, out_error))
      return false;
    if (!append_marshal_session_event(&session, "operator", "work.unblocked",
                                      session.status_detail, out_error)) {
      return false;
    }
    std::string runner_error{};
    std::uint64_t runner_pid = 0;
    if (!launch_session_runner_process(*app, session.marshal_session_id,
                                       &runner_pid, &runner_error)) {
      warnings.push_back("marshal session runner launch failed: " +
                         runner_error);
      session.lifecycle = "terminal";
      session.activity = "quiet";
      session.status_detail = "session runner launch failed: " + runner_error;
      session.work_gate = "open";
      session.finish_reason = "failed";
      session.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      session.updated_at_ms = *session.finished_at_ms;
      std::string ignored{};
      (void)write_marshal_session(*app, session, &ignored);
      (void)append_marshal_session_event(
          &session, "marshal", "session.runner_failed", runner_error, &ignored);
      *out_error = runner_error;
      return false;
    }
    bool runner_identity_captured = false;
    std::string identity_error{};
    if (!capture_process_identity_for_session(runner_pid, &session,
                                              &identity_error)) {
      warnings.push_back("marshal session runner identity degraded: " +
                         identity_error);
    } else {
      runner_identity_captured = true;
      session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      std::string ignored{};
      (void)write_marshal_session(*app, session, &ignored);
    }
    (void)append_marshal_session_event(
        &session, "marshal", "session.runner_started",
        "detached marshal runner launched", nullptr);
    if (!wait_for_session_runner_startup(*app, &session,
                                         runner_identity_captured, out_error)) {
      return false;
    }
  }

  if (!warnings.empty()) {
    std::string combined_warning{};
    for (const std::string &warning : warnings) {
      append_warning_text(&combined_warning, warning);
    }
    persist_marshal_session_warning_best_effort(*app, &session,
                                                combined_warning);
  }

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"session\":" + marshal_session_to_json(session) +
      ",\"warnings\":" + json_array_from_strings(warnings) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_message_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  std::string message{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  (void)extract_json_string_field(arguments_json, "message", &message);
  marshal_session_id = trim_ascii(marshal_session_id);
  message = trim_ascii(message);
  if (marshal_session_id.empty()) {
    *out_error = "message_session requires arguments.marshal_session_id";
    return false;
  }
  if (message.empty()) {
    *out_error = "message_session requires arguments.message";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  bool reconciled = false;
  if (!read_marshal_session_reconciled(*app, marshal_session_id, &session,
                                       &reconciled, out_error))
    return false;
  (void)reconciled;
  if (is_marshal_session_terminal_state(session.lifecycle)) {
    *out_error = "message_session requires a live session; terminal sessions "
                 "are not reopenable";
    return false;
  }
  if (session.lifecycle != "live") {
    *out_error = "message_session requires a live session";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t
      operator_message{};
  operator_message.message_id = make_operator_message_id(&session);
  operator_message.text = message;
  operator_message.received_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  operator_message.delivery_mode =
      cuwacunu::hero::marshal::marshal_session_is_review_ready(session)
          ? "checkpoint"
          : (!trim_ascii(session.current_thread_id).empty() ? "live"
                                                            : "queued");
  if (!append_operator_message_event(
          &session, "operator", "operator.message_received", operator_message,
          "operator message intake", "", "", out_error)) {
    return false;
  }

  if (!cuwacunu::hero::marshal::marshal_session_is_review_ready(session)) {
    std::vector<std::string> warnings{};
    const std::string prior_activity = session.activity;
    const std::string prior_status_detail = session.status_detail;
    session.pending_operator_messages.push_back(operator_message);
    session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    if (trim_ascii(session.status_detail).empty()) {
      session.status_detail = "operator message queued for the next safe point";
    }
    const bool can_wake_live_thread =
        !trim_ascii(session.current_thread_id).empty();
    const bool runner_alive = marshal_session_runner_process_alive(session);
    if (can_wake_live_thread && !runner_alive) {
      session.activity = "handling_message";
      session.status_detail =
          "delivering operator message into live codex thread";
      clear_marshal_session_runner_identity(&session);
    }
    if (!write_marshal_session(*app, session, out_error))
      return false;
    auto queued_notice = operator_message;
    queued_notice.handled_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    if (!append_operator_message_event(
            &session, "marshal", "operator.message_handled", queued_notice,
            can_wake_live_thread ? "queued for live thread delivery"
                                 : "queued for next safe point",
            "", "", out_error)) {
      return false;
    }
    const std::uint64_t delivery_wait_seq = session.last_event_seq;
    if (can_wake_live_thread && !runner_alive) {
      std::string runner_error{};
      std::uint64_t runner_pid = 0;
      if (!launch_session_runner_process(*app, session.marshal_session_id,
                                         &runner_pid, &runner_error)) {
        warnings.push_back("marshal session runner launch failed: " +
                           runner_error);
        session.activity = prior_activity;
        session.status_detail = prior_status_detail;
        session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
        std::string ignored{};
        (void)write_marshal_session(*app, session, &ignored);
        persist_marshal_session_warning_best_effort(
            *app, &session,
            "marshal session runner launch failed while queuing operator "
            "message: " +
                runner_error);
      } else {
        bool runner_identity_captured = false;
        std::string identity_error{};
        if (!capture_process_identity_for_session(runner_pid, &session,
                                                  &identity_error)) {
          warnings.push_back("marshal session runner identity degraded: " +
                             identity_error);
        } else {
          runner_identity_captured = true;
          session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
          std::string ignored{};
          (void)write_marshal_session(*app, session, &ignored);
        }
        (void)append_marshal_session_event(
            &session, "marshal", "session.runner_started",
            "detached marshal runner launched", nullptr);
        if (!wait_for_session_runner_startup(
                *app, &session, runner_identity_captured, out_error)) {
          return false;
        }
      }
    }
    if (can_wake_live_thread) {
      std::string reply_text{};
      std::string failure_text{};
      const operator_message_wait_outcome_t wait_outcome =
          wait_for_operator_message_outcome(
              *app, session.marshal_session_id, delivery_wait_seq,
              operator_message.message_id, &reply_text, &failure_text);
      if (wait_outcome != operator_message_wait_outcome_t::Pending) {
        cuwacunu::hero::marshal::marshal_session_record_t refreshed{};
        std::string refresh_error{};
        if (read_marshal_session_reconciled(*app, marshal_session_id,
                                            &refreshed, nullptr,
                                            &refresh_error)) {
          session = std::move(refreshed);
        } else {
          warnings.push_back("post-message session refresh degraded: " +
                             refresh_error);
        }
        if (wait_outcome == operator_message_wait_outcome_t::Delivered) {
          *out_structured =
              "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
              ",\"message_id\":" + json_quote(operator_message.message_id) +
              ",\"delivery\":\"delivered\"" +
              ",\"reply_text\":" + json_quote(reply_text) +
              ",\"session\":" + marshal_session_to_json(session) +
              ",\"warnings\":" + json_array_from_strings(warnings) + "}";
          return true;
        }
        warnings.push_back("live operator message delivery failed: " +
                           failure_text);
        *out_structured =
            "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
            ",\"message_id\":" + json_quote(operator_message.message_id) +
            ",\"delivery\":\"failed\"" +
            ",\"warning\":" + json_quote(failure_text) +
            ",\"session\":" + marshal_session_to_json(session) +
            ",\"warnings\":" + json_array_from_strings(warnings) + "}";
        return true;
      }
    }
    *out_structured =
        "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
        ",\"message_id\":" + json_quote(operator_message.message_id) +
        ",\"delivery\":\"queued\"" + ",\"reply_text\":\"\"" +
        ",\"session\":" + marshal_session_to_json(session) +
        ",\"warnings\":" + json_array_from_strings(warnings) + "}";
    return true;
  }

  const std::uint64_t checkpoint_index = session.checkpoint_count + 1;
  const std::filesystem::path prior_input_checkpoint_path =
      cuwacunu::hero::marshal::marshal_session_latest_input_checkpoint_path(
          app->defaults.marshal_root, session.marshal_session_id);
  std::string input_checkpoint_json{};
  if (!build_marshal_message_input_checkpoint_json(
          *app, session, checkpoint_index, prior_input_checkpoint_path,
          operator_message, &input_checkpoint_json, out_error)) {
    return false;
  }
  append_memory_note(session, checkpoint_index, "Operator message: " + message);

  const std::filesystem::path input_checkpoint_path =
      cuwacunu::hero::marshal::marshal_session_input_checkpoint_path(
          app->defaults.marshal_root, session.marshal_session_id,
          checkpoint_index);
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          input_checkpoint_path, input_checkpoint_json, out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          marshal_session_latest_input_checkpoint_path(session),
          input_checkpoint_json, out_error)) {
    return false;
  }

  session.lifecycle = "live";
  session.activity = "planning";
  session.status_detail =
      "waiting for codex planning checkpoint after operator message";
  session.work_gate = "open";
  session.finish_reason = "none";
  session.finished_at_ms.reset();
  session.last_input_checkpoint_path = input_checkpoint_path.string();
  session.pending_operator_messages.clear();
  session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  clear_marshal_session_runner_identity(&session);
  if (!write_marshal_session(*app, session, out_error))
    return false;
  if (!append_marshal_session_event(&session, "marshal", "checkpoint.staged",
                                    input_checkpoint_path.string(),
                                    out_error)) {
    return false;
  }
  auto delivered_message = operator_message;
  delivered_message.delivered_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  delivered_message.delivery_status = "delivered";
  if (!append_operator_message_event(
          &session, "marshal", "operator.message_delivered", delivered_message,
          "staged into immediate checkpoint", "", "", out_error)) {
    return false;
  }

  std::vector<std::string> warnings{};
  std::string runner_error{};
  std::uint64_t runner_pid = 0;
  if (!launch_session_runner_process(*app, session.marshal_session_id,
                                     &runner_pid, &runner_error)) {
    warnings.push_back("marshal session runner launch failed: " + runner_error);
    session.lifecycle = "terminal";
    session.activity = "quiet";
    session.status_detail = "session runner launch failed: " + runner_error;
    session.work_gate = "open";
    session.finish_reason = "failed";
    session.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    session.updated_at_ms = *session.finished_at_ms;
    std::string ignored{};
    (void)write_marshal_session(*app, session, &ignored);
    (void)append_marshal_session_event(
        &session, "marshal", "session.runner_failed", runner_error, &ignored);
    *out_error = runner_error;
    return false;
  }
  bool runner_identity_captured = false;
  std::string identity_error{};
  if (!capture_process_identity_for_session(runner_pid, &session,
                                            &identity_error)) {
    warnings.push_back("marshal session runner identity degraded: " +
                       identity_error);
  } else {
    runner_identity_captured = true;
    session.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    std::string ignored{};
    (void)write_marshal_session(*app, session, &ignored);
  }
  (void)append_marshal_session_event(
      &session, "marshal", "session.runner_started",
      "detached marshal runner launched", nullptr);
  if (!wait_for_session_runner_startup(*app, &session, runner_identity_captured,
                                       out_error)) {
    return false;
  }
  auto handled_message = delivered_message;
  handled_message.handled_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  handled_message.delivery_status = "handled";
  if (!append_operator_message_event(
          &session, "marshal", "operator.message_handled", handled_message,
          "review-ready session awoke immediately", "", "", out_error)) {
    return false;
  }

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"message_id\":" + json_quote(operator_message.message_id) +
      ",\"delivery\":\"delivered\"" + ",\"reply_text\":\"\"" +
      ",\"session\":" + marshal_session_to_json(session) +
      ",\"warnings\":" + json_array_from_strings(warnings) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_archive_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "archive_session requires arguments.marshal_session_id";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  bool reconciled = false;
  if (!read_marshal_session_reconciled(*app, marshal_session_id, &session,
                                       &reconciled, out_error)) {
    return false;
  }
  (void)reconciled;

  if (is_marshal_session_terminal_state(session.lifecycle)) {
    const std::string warning =
        "session is already terminal; objective ownership is already released";
    *out_structured =
        "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
        ",\"session\":" + marshal_session_to_json(session) +
        ",\"warning\":" + json_quote(warning) + "}";
    return true;
  }
  if (!cuwacunu::hero::marshal::marshal_session_is_review_ready(session)) {
    *out_error =
        "archive_session requires a review-ready live session or a terminal "
        "session";
    return false;
  }
  if (!trim_ascii(session.campaign_cursor).empty() ||
      (session.campaign_status != "none" &&
       session.campaign_status != "stopped")) {
    *out_error =
        "archive_session requires the session to have no active runtime "
        "campaign";
    return false;
  }

  session.lifecycle = "terminal";
  session.activity = "quiet";
  session.work_gate = "open";
  session.status_detail =
      "archive_session requested by operator; objective ownership released";
  if (trim_ascii(session.finish_reason).empty() ||
      session.finish_reason == "none") {
    session.finish_reason = "success";
  }
  session.last_codex_action = "archive";
  clear_marshal_session_campaign_linkage(&session);
  session.pending_operator_messages.clear();
  session.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  session.updated_at_ms = *session.finished_at_ms;
  clear_marshal_session_runner_identity(&session);
  if (!write_marshal_session(*app, session, out_error)) {
    return false;
  }
  if (!append_marshal_session_event(&session, "operator", "session.archived",
                                    session.status_detail, out_error)) {
    return false;
  }

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"session\":" + marshal_session_to_json(session) + ",\"warning\":\"\"}";
  return true;
}

[[nodiscard]] bool handle_tool_terminate_session(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  bool force = false;
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  (void)extract_json_bool_field(arguments_json, "force", &force);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "terminate_session requires arguments.marshal_session_id";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  bool reconciled = false;
  if (!read_marshal_session_reconciled(*app, marshal_session_id, &session,
                                       &reconciled, out_error))
    return false;
  (void)reconciled;
  if (is_marshal_session_terminal_state(session.lifecycle)) {
    *out_structured =
        "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
        ",\"session\":" + marshal_session_to_json(session) +
        ",\"warning\":\"\"}";
    return true;
  }

  const std::string campaign_cursor = trim_ascii(session.campaign_cursor);
  std::string runtime_warning{};
  bool active_campaign_stop_pending = false;
  if (!campaign_cursor.empty()) {
    cuwacunu::hero::runtime::runtime_campaign_record_t stopped_campaign{};
    if (!request_runtime_campaign_stop_for_marshal_action(
            *app, session, campaign_cursor, force, &stopped_campaign,
            &runtime_warning, out_error)) {
      return false;
    }
    active_campaign_stop_pending =
        !is_runtime_campaign_terminal_state(trim_ascii(stopped_campaign.state));
  }
  cancel_session_checkpoint_best_effort(session, force, &runtime_warning);

  session.lifecycle = "terminal";
  session.activity = "quiet";
  session.status_detail =
      active_campaign_stop_pending
          ? "terminate_session requested by operator; active runtime campaign "
            "stop in progress"
          : "terminate_session requested by operator";
  session.work_gate = "open";
  session.finish_reason = "terminated";
  session.last_codex_action = "terminate";
  session.campaign_status = active_campaign_stop_pending ? "stopping" : "none";
  session.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  session.updated_at_ms = *session.finished_at_ms;
  if (!active_campaign_stop_pending) {
    clear_marshal_session_campaign_linkage(&session);
  }
  clear_marshal_session_runner_identity(&session);
  if (!write_marshal_session(*app, session, out_error))
    return false;
  if (!append_marshal_session_event(&session, "operator", "session.finished",
                                    session.status_detail, out_error)) {
    return false;
  }
  if (!runtime_warning.empty()) {
    persist_marshal_session_warning_best_effort(*app, &session,
                                                runtime_warning);
  }

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"session\":" + marshal_session_to_json(session) +
      ",\"warning\":" + json_quote(runtime_warning) + "}";
  return true;
}

} // namespace

std::filesystem::path
resolve_marshal_hero_dsl_path(const std::filesystem::path &global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "REAL_HERO", "marshal_hero_dsl_filename");
  if (!configured.has_value())
    return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty())
    return {};
  return std::filesystem::path(resolved);
}

bool load_marshal_defaults(const std::filesystem::path &hero_dsl_path,
                           const std::filesystem::path &global_config_path,
                           marshal_defaults_t *out, std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "marshal defaults output pointer is null";
    return false;
  }
  *out = marshal_defaults_t{};

  std::ifstream in(hero_dsl_path);
  if (!in) {
    if (error) {
      *error =
          "cannot open Marshal HERO defaults DSL: " + hero_dsl_path.string();
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
      if (end_block == std::string::npos)
        continue;
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
    if (candidate.empty())
      continue;
    const std::size_t eq = candidate.find('=');
    if (eq == std::string::npos)
      continue;
    std::string lhs = trim_ascii(candidate.substr(0, eq));
    std::string rhs = unquote_if_wrapped(candidate.substr(eq + 1));
    lhs = cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(lhs);
    if (lhs.empty())
      continue;
    values[lhs] = trim_ascii(rhs);
  }

  out->marshal_root = cuwacunu::hero::marshal::marshal_root(
      resolve_runtime_root_from_global_config(global_config_path));
  out->repo_root = resolve_repo_root_from_global_config(global_config_path);
  out->campaign_grammar_path =
      resolve_campaign_grammar_from_global_config(global_config_path);
  out->marshal_objective_grammar_path =
      resolve_marshal_objective_grammar_from_global_config(global_config_path);
  const auto repo_root_it = values.find("repo_root");
  if (repo_root_it != values.end() &&
      !trim_ascii(repo_root_it->second).empty()) {
    out->repo_root = std::filesystem::path(resolve_path_from_base_folder(
        hero_dsl_path.parent_path().string(), repo_root_it->second));
  }
  const auto config_scope_it = values.find("config_scope_root");
  if (config_scope_it != values.end() &&
      !trim_ascii(config_scope_it->second).empty()) {
    out->config_scope_root =
        std::filesystem::path(resolve_path_from_base_folder(
            hero_dsl_path.parent_path().string(), config_scope_it->second));
  } else {
    out->config_scope_root = global_config_path.parent_path();
  }

  const auto resolve_exec = [&](const char *key, std::filesystem::path *dst) {
    const auto it = values.find(key);
    if (it == values.end())
      return false;
    if (std::string_view(key) == "marshal_codex_binary") {
      *dst = resolve_marshal_codex_binary_path(hero_dsl_path.parent_path(),
                                               it->second);
    } else {
      *dst = resolve_command_path(hero_dsl_path.parent_path(), it->second);
    }
    return !dst->empty();
  };

  bool ok = true;
  ok = resolve_exec("runtime_hero_binary", &out->runtime_hero_binary) && ok;
  ok = resolve_exec("config_hero_binary", &out->config_hero_binary) && ok;
  ok = resolve_exec("lattice_hero_binary", &out->lattice_hero_binary) && ok;
  ok = resolve_exec("human_hero_binary", &out->human_hero_binary) && ok;
  ok = resolve_exec("marshal_codex_binary", &out->marshal_codex_binary) && ok;
  if (!ok) {
    if (error)
      *error = "missing one or more binary fields in " + hero_dsl_path.string();
    return false;
  }
  const auto codex_model_it = values.find("marshal_codex_model");
  if (codex_model_it != values.end()) {
    out->marshal_codex_model = trim_ascii(codex_model_it->second);
  }
  if (out->marshal_codex_model.empty()) {
    if (error)
      *error =
          "missing/invalid marshal_codex_model in " + hero_dsl_path.string();
    return false;
  }
  const auto codex_reasoning_it = values.find("marshal_codex_reasoning_effort");
  if (codex_reasoning_it != values.end() &&
      !trim_ascii(codex_reasoning_it->second).empty()) {
    if (!normalize_codex_reasoning_effort(
            codex_reasoning_it->second, &out->marshal_codex_reasoning_effort)) {
      if (error) {
        *error = "missing/invalid marshal_codex_reasoning_effort in " +
                 hero_dsl_path.string() +
                 " (expected one of: low, medium, high, xhigh)";
      }
      return false;
    }
  }
  const auto human_identities_it = values.find("human_operator_identities");
  if (human_identities_it != values.end() &&
      !trim_ascii(human_identities_it->second).empty()) {
    out->human_operator_identities =
        std::filesystem::path(resolve_path_from_base_folder(
            hero_dsl_path.parent_path().string(), human_identities_it->second));
  }
  if (!parse_size_token(values["tail_default_lines"],
                        &out->tail_default_lines) ||
      out->tail_default_lines == 0) {
    if (error)
      *error =
          "missing/invalid tail_default_lines in " + hero_dsl_path.string();
    return false;
  }
  if (!parse_size_token(values["poll_interval_ms"], &out->poll_interval_ms) ||
      out->poll_interval_ms < 100) {
    if (error)
      *error = "missing/invalid poll_interval_ms in " + hero_dsl_path.string();
    return false;
  }
  if (!parse_size_token(values["marshal_codex_timeout_sec"],
                        &out->marshal_codex_timeout_sec) ||
      out->marshal_codex_timeout_sec == 0) {
    if (error) {
      *error = "missing/invalid marshal_codex_timeout_sec in " +
               hero_dsl_path.string();
    }
    return false;
  }
  if (!parse_size_token(values["marshal_max_campaign_launches"],
                        &out->marshal_max_campaign_launches) ||
      out->marshal_max_campaign_launches == 0) {
    if (error) {
      *error = "missing/invalid marshal_max_campaign_launches in " +
               hero_dsl_path.string();
    }
    return false;
  }
  if (out->marshal_root.empty()) {
    if (error) {
      *error =
          "cannot derive .marshal_hero root from GENERAL.runtime_root in " +
          global_config_path.string();
    }
    return false;
  }
  if (out->repo_root.empty()) {
    if (error) {
      *error =
          "missing/invalid GENERAL.repo_root in " + global_config_path.string();
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
      *error = "missing [BNF].iitepi_campaign_grammar_filename in " +
               global_config_path.string();
    }
    return false;
  }
  if (out->marshal_objective_grammar_path.empty()) {
    if (error) {
      *error = "missing [BNF].marshal_objective_grammar_filename in " +
               global_config_path.string();
    }
    return false;
  }
  return true;
}

std::filesystem::path current_executable_path() {
  std::array<char, 4096> buf{};
  const ssize_t n = ::readlink("/proc/self/exe", buf.data(), buf.size() - 1);
  if (n <= 0)
    return {};
  buf[static_cast<std::size_t>(n)] = '\0';
  return std::filesystem::path(buf.data()).lexically_normal();
}

bool tool_result_is_error(std::string_view tool_result_json) {
  return tool_result_json.find("\"isError\":true") != std::string_view::npos ||
         tool_result_json.find("\"isError\": true") != std::string_view::npos;
}

[[nodiscard]] bool marshal_tool_is_read_only(std::string_view name) {
  (void)name;
  return false;
}

[[nodiscard]] bool marshal_tool_is_destructive(std::string_view name) {
  return name == "hero.marshal.terminate_session";
}

std::string build_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < std::size(kMarshalTools); ++i) {
    const auto &tool = kMarshalTools[i];
    if (i != 0)
      out << ",";
    out << "{\"name\":" << json_quote(tool.name)
        << ",\"description\":" << json_quote(tool.description)
        << ",\"inputSchema\":" << tool.input_schema_json
        << ",\"annotations\":{\"readOnlyHint\":"
        << (marshal_tool_is_read_only(tool.name) ? "true" : "false")
        << ",\"destructiveHint\":"
        << (marshal_tool_is_destructive(tool.name) ? "true" : "false")
        << ",\"openWorldHint\":false}}";
  }
  out << "]}";
  return out.str();
}

std::string build_tools_list_human_text() {
  std::ostringstream out;
  for (const auto &tool : kMarshalTools) {
    out << tool.name << "\t" << tool.description << "\n";
  }
  return out.str();
}

bool execute_tool_json(const std::string &tool_name, std::string arguments_json,
                       app_context_t *app, std::string *out_tool_result_json,
                       std::string *out_error_message) {
  if (out_tool_result_json)
    out_tool_result_json->clear();
  if (out_error_message)
    out_error_message->clear();
  if (!app) {
    if (out_error_message)
      *out_error_message = "app context pointer is null";
    return false;
  }
  if (!out_tool_result_json || !out_error_message)
    return false;

  arguments_json = trim_ascii(arguments_json);
  if (arguments_json.empty())
    arguments_json = "{}";

  const auto *descriptor = find_marshal_tool_descriptor(tool_name);
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

bool run_session_runner(app_context_t *app, std::string_view marshal_session_id,
                        std::string *error) {
  if (error)
    error->clear();
  if (!app) {
    if (error)
      *error = "marshal app pointer is null";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t loop{};
  if (!read_marshal_session(*app, marshal_session_id, &loop, error))
    return false;
  session_runner_lock_guard_t runner_lock{};
  bool already_locked = false;
  if (!acquire_session_runner_lock(loop, &runner_lock, &already_locked,
                                   error)) {
    return false;
  }
  if (already_locked)
    return true;

  std::string ignored{};
  (void)append_marshal_session_event(&loop, "marshal", "session.runner_active",
                                     "runner lock acquired", &ignored);
  std::string identity_error{};
  if (capture_process_identity_for_session(
          static_cast<std::uint64_t>(::getpid()), &loop, &identity_error)) {
    loop.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    (void)write_marshal_session(*app, loop, &ignored);
  } else {
    persist_marshal_session_warning_best_effort(
        *app, &loop,
        "marshal session runner identity degraded after lock acquisition: " +
            identity_error);
  }
  for (;;) {
    if (!read_marshal_session(*app, marshal_session_id, &loop, error))
      return false;
    const bool has_pending_operator_message =
        marshal_session_has_pending_operator_message(loop);
    if (loop.lifecycle == "live" && has_pending_operator_message &&
        !trim_ascii(loop.current_thread_id).empty()) {
      std::string warning{};
      if (!process_pending_live_operator_message(app, &loop, &warning, error))
        return false;
      if (!warning.empty()) {
        persist_marshal_session_warning_best_effort(*app, &loop, warning);
      }
      continue;
    }
    const bool review_ready =
        cuwacunu::hero::marshal::marshal_session_is_review_ready(loop);
    const bool work_blocked =
        loop.lifecycle == "live" && loop.work_gate != "open";
    if (is_marshal_session_terminal_state(loop.lifecycle) || work_blocked ||
        review_ready) {
      return true;
    }

    const std::string campaign_cursor = trim_ascii(loop.campaign_cursor);
    if (campaign_cursor.empty()) {
      if (loop.lifecycle == "live" && loop.work_gate == "open") {
        std::string warning{};
        if (!execute_pending_checkpoint(app, &loop, &warning, error))
          return false;
        if (!warning.empty()) {
          persist_marshal_session_warning_best_effort(*app, &loop, warning);
        }
        continue;
      }
      loop.lifecycle = "terminal";
      loop.activity = "quiet";
      loop.status_detail =
          "session runner found no active campaign for non-live work state";
      loop.work_gate = "open";
      loop.finish_reason = "failed";
      loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      loop.updated_at_ms = *loop.finished_at_ms;
      if (!write_marshal_session(*app, loop, error))
        return false;
      (void)append_marshal_session_event(&loop, "marshal", "session.finished",
                                         loop.status_detail, &ignored);
      return false;
    }

    cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
    if (!read_runtime_campaign_direct(*app, campaign_cursor, &campaign,
                                      error)) {
      loop.lifecycle = "terminal";
      loop.activity = "quiet";
      loop.status_detail = *error;
      loop.work_gate = "open";
      loop.finish_reason = "failed";
      loop.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      loop.updated_at_ms = *loop.finished_at_ms;
      std::string ignored_local{};
      (void)write_marshal_session(*app, loop, &ignored_local);
      (void)append_marshal_session_event(&loop, "marshal", "session.finished",
                                         loop.status_detail, &ignored_local);
      return false;
    }
    const std::string state = observed_campaign_state(campaign);
    if (!is_runtime_campaign_terminal_state(state)) {
      marshal_session_workspace_context_t workspace_context{};
      if (!load_marshal_session_workspace_context(*app, loop,
                                                  &workspace_context, error)) {
        return false;
      }
      ::usleep(
          static_cast<useconds_t>(workspace_context.poll_interval_ms * 1000));
      continue;
    }

    std::string warning{};
    if (!reopen_session_after_terminal_campaign(app, campaign_cursor, &warning,
                                                error)) {
      return false;
    }
    if (!warning.empty()) {
      cuwacunu::hero::marshal::marshal_session_record_t latest_loop{};
      std::string warning_reload_error{};
      if (read_marshal_session(*app, marshal_session_id, &latest_loop,
                               &warning_reload_error)) {
        persist_marshal_session_warning_best_effort(*app, &latest_loop,
                                                    warning);
      } else {
        persist_marshal_session_warning_best_effort(*app, &loop, warning);
      }
    }
  }
}

[[nodiscard]] bool starts_with_case_insensitive(std::string_view value,
                                                std::string_view prefix) {
  if (value.size() < prefix.size())
    return false;
  for (std::size_t i = 0; i < prefix.size(); ++i) {
    const char a =
        static_cast<char>(std::tolower(static_cast<unsigned char>(value[i])));
    const char b =
        static_cast<char>(std::tolower(static_cast<unsigned char>(prefix[i])));
    if (a != b)
      return false;
  }
  return true;
}

[[nodiscard]] bool parse_content_length_header(std::string_view header_line,
                                               std::size_t *out_length) {
  constexpr std::string_view kPrefix = "content-length:";
  const std::string line = trim_ascii(header_line);
  if (!starts_with_case_insensitive(line, kPrefix))
    return false;
  const std::string number = trim_ascii(line.substr(kPrefix.size()));
  return parse_size_token(number, out_length);
}

[[nodiscard]] bool read_next_jsonrpc_message(std::istream *in,
                                             std::string *out_payload,
                                             bool *out_used_content_length) {
  if (out_used_content_length)
    *out_used_content_length = false;

  std::string first_line;
  while (std::getline(*in, first_line)) {
    const std::string trimmed = trim_ascii(first_line);
    if (trimmed.empty())
      continue;

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
        if (header_trimmed.empty())
          break;
        std::size_t parsed_length = 0;
        if (parse_content_length_header(header_trimmed, &parsed_length)) {
          content_length = parsed_length;
        }
      }
      if (content_length == 0)
        continue;
      if (content_length > kMaxJsonRpcPayloadBytes)
        return false;

      std::string payload(content_length, '\0');
      in->read(payload.data(), static_cast<std::streamsize>(content_length));
      if (in->gcount() != static_cast<std::streamsize>(content_length)) {
        return false;
      }
      if (out_payload)
        *out_payload = std::move(payload);
      if (out_used_content_length)
        *out_used_content_length = true;
      return true;
    }

    if (out_payload)
      *out_payload = trimmed;
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

void write_jsonrpc_result(std::string_view id_json,
                          std::string_view result_json) {
  emit_jsonrpc_payload("{\"jsonrpc\":\"2.0\",\"id\":" + std::string(id_json) +
                       ",\"result\":" + std::string(result_json) + "}");
}

void write_jsonrpc_error(std::string_view id_json, int code,
                         std::string_view message) {
  emit_jsonrpc_payload("{\"jsonrpc\":\"2.0\",\"id\":" + std::string(id_json) +
                       ",\"error\":{\"code\":" + std::to_string(code) +
                       ",\"message\":" + json_quote(message) + "}}");
}

[[nodiscard]] bool parse_jsonrpc_id(const std::string &json,
                                    std::string *out_id_json) {
  std::string raw_id{};
  if (!extract_json_field_raw(json, "id", &raw_id))
    return false;
  raw_id = trim_ascii(raw_id);
  if (raw_id.empty())
    return false;
  if (raw_id.front() == '"') {
    std::string parsed;
    std::size_t end = 0;
    if (!parse_json_string_at(raw_id, 0, &parsed, &end))
      return false;
    end = skip_json_whitespace(raw_id, end);
    if (end != raw_id.size())
      return false;
    if (out_id_json)
      *out_id_json = json_quote(parsed);
    return true;
  }
  if (out_id_json)
    *out_id_json = raw_id;
  return true;
}

void run_jsonrpc_stdio_loop(app_context_t *app) {
  if (!app)
    return;

  for (;;) {
    std::string request_json{};
    bool used_content_length = false;
    if (!read_next_jsonrpc_message(&std::cin, &request_json,
                                   &used_content_length)) {
      break;
    }
    g_jsonrpc_use_content_length_framing = used_content_length;
    request_json = trim_ascii(request_json);
    if (request_json.empty())
      continue;

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
          id_json, "{\"protocolVersion\":" + json_quote(kProtocolVersion) +
                       ",\"serverInfo\":{\"name\":" + json_quote(kServerName) +
                       ",\"version\":" + json_quote(kServerVersion) +
                       "},\"capabilities\":{\"tools\":{}},\"instructions\":" +
                       json_quote(kInitializeInstructions) + "}");
      continue;
    }
    if (method == "notifications/initialized")
      continue;
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

} // namespace marshal_mcp
} // namespace hero
} // namespace cuwacunu
