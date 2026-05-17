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
constexpr std::size_t kResumePromptInputCheckpointBytesCap = 24u << 10;
constexpr std::size_t kMarshalCheckpointTailExcerptBytesCap = 2u << 10;
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
[[nodiscard]] std::filesystem::path
resolve_marshal_session_runner_binary(const app_context_t &app);
[[nodiscard]] bool
call_runtime_tool(const app_context_t &app,
                  const std::filesystem::path &runtime_binary,
                  const std::string &tool_name, std::string arguments_json,
                  std::string *out_structured, std::string *error);
void append_structured_warnings(const std::string &structured_json,
                                std::string_view prefix,
                                std::vector<std::string> *out);
[[nodiscard]] bool extract_json_string_field(const std::string &json,
                                             std::string_view key,
                                             std::string *out);

struct marshal_session_intent_t {
  std::string intent{};
  std::string reply_text{};
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
  std::string marshal_codex_model{};
  std::string marshal_codex_reasoning_effort{};
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
[[nodiscard]] bool write_marshal_session(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &record,
    std::string *error);

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

[[nodiscard]] std::string
extract_current_thread_id_from_codex_jsonl(std::string_view jsonl_text) {
  std::istringstream in{std::string(jsonl_text)};
  std::string line{};
  std::string latest{};
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(line);
    if (trimmed.empty())
      continue;
    std::string type{};
    if (!extract_json_string_field(trimmed, "type", &type) ||
        type != "thread.started") {
      continue;
    }
    std::string candidate{};
    if (!extract_json_string_field(trimmed, "thread_id", &candidate))
      continue;
    candidate = lowercase_copy(trim_ascii(candidate));
    if (looks_like_current_thread_id(candidate)) {
      latest = std::move(candidate);
    }
  }
  return latest;
}

[[nodiscard]] std::string
extract_current_thread_id_from_codex_logs(std::string_view stdout_text,
                                          std::string_view stderr_text) {
  std::string current_thread_id =
      extract_current_thread_id_from_codex_jsonl(stdout_text);
  if (!current_thread_id.empty())
    return current_thread_id;
  return extract_current_thread_id_from_log(stderr_text);
}

[[nodiscard]] std::string bounded_text_excerpt(std::string_view text,
                                               std::size_t max_bytes) {
  if (max_bytes == 0 || text.size() <= max_bytes)
    return std::string(text);
  std::ostringstream out;
  out << text.substr(0, max_bytes)
      << "\n[... truncated; original_bytes=" << text.size()
      << " returned_bytes=" << max_bytes << " ...]";
  return out.str();
}

[[nodiscard]] std::string sanitize_inline_artifact_text(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  bool in_escape = false;
  for (std::size_t i = 0; i < in.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(in[i]);
    if (in_escape) {
      if ((c >= 0x40 && c <= 0x7e) || c == '\n') {
        in_escape = false;
      }
      continue;
    }
    if (c == 0x1b) {
      in_escape = true;
      continue;
    }
    if (c == '\r')
      continue;
    if (c == '\n' || c == '\t') {
      out.push_back(static_cast<char>(c));
      continue;
    }
    if (c < 0x20 || c == 0x7f) {
      if (out.empty() || out.back() != ' ')
        out.push_back(' ');
      continue;
    }
    out.push_back(static_cast<char>(c));
  }
  return out;
}

[[nodiscard]] std::string
bounded_sanitized_artifact_excerpt(std::string_view text,
                                   std::size_t max_bytes) {
  return bounded_text_excerpt(sanitize_inline_artifact_text(text), max_bytes);
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

[[nodiscard]] std::filesystem::path session_event_lock_path(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop) {
  return std::filesystem::path(loop.session_root) /
         ".marshal.session.events.lock";
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

