constexpr const char *kServerName = "hero_human_mcp";
constexpr const char *kServerVersion = "0.1.0";
constexpr const char *kProtocolVersion = "2025-03-26";
constexpr const char *kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.human.*.";
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;
constexpr std::size_t kMarshalToolTimeoutSec = 30;
constexpr const char *kNcursesInitErrorPrefix = "ncurses init failed: ";
constexpr std::string_view kHumanClarificationAnswerSchemaV3 =
    "hero.human.clarification_answer.v3";
constexpr std::string_view kHumanSummaryAckSchemaV3 =
    "hero.human.summary_ack.v3";

using cuwacunu::hero::human::human_resolution_record_t;

using human_tool_handler_t = bool (*)(app_context_t *app,
                                      const std::string &arguments_json,
                                      std::string *out_structured,
                                      std::string *out_error);

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
    const app_context_t &app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out,
    std::string *error);

[[nodiscard]] bool collect_all_sessions(
    const app_context_t &app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out,
    std::string *error);

[[nodiscard]] bool collect_pending_reviews(
    const app_context_t &app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out,
    std::string *error);

[[nodiscard]] bool collect_human_operator_inbox(const app_context_t &app,
                                                human_operator_inbox_t *out,
                                                bool sync_markers,
                                                std::string *error);

[[nodiscard]] bool build_resolution_and_apply(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string resolution_kind, std::string reason,
    std::string *out_structured, std::string *out_error);

[[nodiscard]] bool build_request_answer_and_resume(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string answer, std::string *out_structured, std::string *out_error);

[[nodiscard]] bool acknowledge_session_summary(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string note, std::string *out_structured, std::string *out_error);

[[nodiscard]] bool pause_marshal_session(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool force, std::string *out_structured, std::string *out_error);

[[nodiscard]] bool resume_marshal_session(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string *out_structured, std::string *out_error);

[[nodiscard]] bool message_marshal_session(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string message, std::string *out_structured,
    std::string *out_error);

[[nodiscard]] std::string summarize_message_session_structured(
    const std::string &structured, bool refreshing, bool *out_is_error);

[[nodiscard]] bool terminate_marshal_session(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool force, std::string *out_structured, std::string *out_error);

[[nodiscard]] bool list_declared_bind_ids_for_session(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::vector<std::string> *out_bind_ids, std::string *error);

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

[[nodiscard]] bool is_clarification_work_gate(std::string_view work_gate) {
  return work_gate == "clarification";
}

[[nodiscard]] bool is_human_request_work_gate(std::string_view work_gate) {
  return is_clarification_work_gate(work_gate) || work_gate == "governance";
}

[[nodiscard]] std::string_view
human_request_kind_label(std::string_view work_gate) {
  if (is_clarification_work_gate(work_gate))
    return "clarification";
  if (work_gate == "governance")
    return "governance";
  if (work_gate == "operator_pause")
    return "operator";
  if (work_gate == "open")
    return "none";
  return "unknown";
}

[[nodiscard]] std::optional<std::uint64_t> marshal_session_elapsed_ms(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  const std::uint64_t end_ms = session.finished_at_ms.has_value()
                                   ? *session.finished_at_ms
                                   : session.updated_at_ms;
  if (session.started_at_ms == 0 || end_ms < session.started_at_ms) {
    return std::nullopt;
  }
  return end_ms - session.started_at_ms;
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

[[nodiscard]] std::string format_effort_usage(std::uint64_t used,
                                              std::uint64_t limit,
                                              std::uint64_t remaining) {
  std::ostringstream out;
  out << used;
  if (limit != 0)
    out << "/" << limit;
  out << " (" << remaining << " rem)";
  return out.str();
}

[[nodiscard]] std::string build_summary_effort_text(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::ostringstream out;
  out << "elapsed="
      << format_compact_duration_ms(marshal_session_elapsed_ms(session))
      << " | checkpoints=" << session.checkpoint_count << " | launches="
      << format_effort_usage(session.launch_count,
                             session.max_campaign_launches,
                             session.remaining_campaign_launches);
  return out.str();
}

[[nodiscard]] bool terminal_supports_human_ncurses_ui() {
  const char *term_env = std::getenv("TERM");
  const std::string term =
      lowercase_copy(trim_ascii(term_env == nullptr ? "" : term_env));
  if (term.empty() || term == "dumb" || term == "unknown")
    return false;
  if (::isatty(STDIN_FILENO) == 0 || ::isatty(STDOUT_FILENO) == 0)
    return false;
  return true;
}

void sort_sessions_newest_first(
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *rows) {
  if (!rows)
    return;
  std::sort(rows->begin(), rows->end(), [](const auto &a, const auto &b) {
    if (a.updated_at_ms != b.updated_at_ms)
      return a.updated_at_ms > b.updated_at_ms;
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
    if (!in_single && !in_double && (c == '#' || c == ';'))
      break;
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

[[nodiscard]] bool operator_id_needs_bootstrap(std::string_view value) {
  const std::string trimmed = trim_ascii(value);
  return trimmed.empty() || trimmed == "CHANGE_ME_OPERATOR";
}

[[nodiscard]] std::string current_user_name() {
  const char *env_user = std::getenv("USER");
  if (env_user != nullptr) {
    const std::string trimmed = trim_ascii(env_user);
    if (!trimmed.empty())
      return trimmed;
  }
  const char *env_logname = std::getenv("LOGNAME");
  if (env_logname != nullptr) {
    const std::string trimmed = trim_ascii(env_logname);
    if (!trimmed.empty())
      return trimmed;
  }
  errno = 0;
  if (const passwd *pw = ::getpwuid(::geteuid());
      pw != nullptr && pw->pw_name != nullptr) {
    const std::string trimmed = trim_ascii(pw->pw_name);
    if (!trimmed.empty())
      return trimmed;
  }
  return "operator";
}

[[nodiscard]] std::string current_host_name() {
  std::array<char, 256> host{};
  if (::gethostname(host.data(), host.size() - 1) == 0) {
    host.back() = '\0';
    const std::string trimmed = trim_ascii(host.data());
    if (!trimmed.empty())
      return trimmed;
  }
  return "localhost";
}

[[nodiscard]] std::string derive_bootstrap_operator_id() {
  return current_user_name() + "@" + current_host_name();
}

[[nodiscard]] bool
persist_bootstrap_operator_id(const std::filesystem::path &hero_dsl_path,
                              std::string_view operator_id,
                              std::string *error) {
  if (error)
    error->clear();
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
        lhs = cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(
            lhs);
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
    if (i + 1 != lines.size())
      out << "\n";
  }
  out << "\n";
  return cuwacunu::hero::runtime::write_text_file_atomic(hero_dsl_path,
                                                         out.str(), error);
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
      case 'u':
        if (pos + 4 > json.size())
          return false;
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

[[nodiscard]] bool find_json_value_end(const std::string &json, std::size_t pos,
                                       std::size_t *out_end) {
  pos = skip_json_whitespace(json, pos);
  if (pos >= json.size())
    return false;
  const char first = json[pos];
  if (first == '"')
    return parse_json_string_at(json, pos, nullptr, out_end);
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
        if (c == '"')
          in_string = false;
        continue;
      }
      if (c == '"') {
        in_string = true;
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
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw))
    return false;
  std::size_t end = 0;
  return parse_json_string_at(raw, 0, out, &end) &&
         skip_json_whitespace(raw, end) == raw.size();
}

[[nodiscard]] bool extract_json_bool_field(const std::string &json,
                                           std::string_view key, bool *out) {
  std::string raw{};
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
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw))
    return false;
  return parse_size_token(raw, out);
}

[[nodiscard]] bool extract_json_u64_field(const std::string &json,
                                          std::string_view key,
                                          std::uint64_t *out) {
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw))
    return false;
  return parse_u64_token(raw, out);
}

[[nodiscard]] bool extract_json_object_field(const std::string &json,
                                             std::string_view key,
                                             std::string *out) {
  std::string raw{};
  if (!extract_json_field_raw(json, key, &raw))
    return false;
  raw = trim_ascii(raw);
  if (raw.empty() || raw.front() != '{' || raw.back() != '}')
    return false;
  if (out)
    *out = std::move(raw);
  return true;
}

[[nodiscard]] bool
tool_result_is_error_impl(std::string_view tool_result_json) {
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
  if (tool_result_is_error_impl(json)) {
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
  return std::filesystem::path(resolved).lexically_normal();
}

[[nodiscard]] std::filesystem::path
resolve_campaign_grammar_path(const std::filesystem::path &global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "BNF", "iitepi_campaign_grammar_filename");
  if (!configured.has_value())
    return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty())
    return {};
  return std::filesystem::path(resolved).lexically_normal();
}

[[nodiscard]] bool list_declared_bind_ids_for_session(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::vector<std::string> *out_bind_ids, std::string *error) {
  if (error)
    error->clear();
  if (!out_bind_ids) {
    if (error)
      *error = "bind id output pointer is null";
    return false;
  }
  out_bind_ids->clear();

  const std::filesystem::path campaign_grammar_path =
      resolve_campaign_grammar_path(app.global_config_path);
  if (campaign_grammar_path.empty()) {
    if (error)
      *error = "cannot resolve BNF.iitepi_campaign_grammar_filename";
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
  } catch (const std::exception &ex) {
    if (error) {
      *error = "failed to decode campaign DSL snapshot '" +
               session.campaign_dsl_path + "': " + ex.what();
    }
    return false;
  }

  out_bind_ids->reserve(instruction.binds.size());
  for (const auto &bind : instruction.binds) {
    const std::string bind_id = trim_ascii(bind.id);
    if (!bind_id.empty())
      out_bind_ids->push_back(bind_id);
  }
  return true;
}

[[nodiscard]] std::filesystem::path
resolve_command_path(const std::filesystem::path &base_dir,
                     std::string_view configured_value) {
  const std::string configured = trim_ascii(configured_value);
  if (configured.empty())
    return {};
  const std::filesystem::path raw(configured);
  if (configured.find('/') != std::string::npos ||
      configured.find('\\') != std::string::npos) {
    if (raw.is_absolute())
      return raw.lexically_normal();
    return (base_dir / raw).lexically_normal();
  }
  if (const char *env_path = std::getenv("PATH"); env_path != nullptr) {
    std::stringstream in(env_path);
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

[[nodiscard]] bool
run_command_with_stdio_and_timeout(const std::vector<std::string> &argv,
                                   const std::filesystem::path &stdin_path,
                                   const std::filesystem::path &stdout_path,
                                   const std::filesystem::path &stderr_path,
                                   std::size_t timeout_sec, int *out_exit_code,
                                   std::string *error) {
  if (error)
    error->clear();
  if (argv.empty()) {
    if (error)
      *error = "command argv is empty";
    return false;
  }
  if (out_exit_code)
    *out_exit_code = -1;

  std::error_code ec{};
  if (!std::filesystem::exists(stdin_path, ec) ||
      !std::filesystem::is_regular_file(stdin_path, ec)) {
    if (error)
      *error = "command stdin path does not exist: " + stdin_path.string();
    return false;
  }
  const int stdout_probe =
      ::open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (stdout_probe < 0) {
    if (error)
      *error = "cannot open stdout path for command: " + stdout_path.string();
    return false;
  }
  (void)::close(stdout_probe);
  const int stderr_probe =
      ::open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (stderr_probe < 0) {
    if (error)
      *error = "cannot open stderr path for command: " + stderr_path.string();
    return false;
  }
  (void)::close(stderr_probe);

  const pid_t child = ::fork();
  if (child < 0) {
    if (error)
      *error = "fork failed for command execution";
    return false;
  }
  if (child == 0) {
    (void)::setpgid(0, 0);
    const int stdin_fd = ::open(stdin_path.c_str(), O_RDONLY);
    if (stdin_fd < 0)
      _exit(126);
    (void)::dup2(stdin_fd, STDIN_FILENO);
    if (stdin_fd > STDERR_FILENO)
      (void)::close(stdin_fd);
    const int stdout_fd =
        ::open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stdout_fd < 0)
      _exit(126);
    (void)::dup2(stdout_fd, STDOUT_FILENO);
    if (stdout_fd > STDERR_FILENO)
      (void)::close(stdout_fd);
    const int stderr_fd =
        ::open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stderr_fd < 0)
      _exit(126);
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
    _exit(127);
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
      return false;
    }
    if (std::chrono::steady_clock::now() >= deadline) {
      (void)::kill(-child, SIGKILL);
      (void)::kill(child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      if (error)
        *error = "command timed out";
      return false;
    }
    ::usleep(100 * 1000);
  }

  if (WIFEXITED(status)) {
    if (out_exit_code)
      *out_exit_code = WEXITSTATUS(status);
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

[[nodiscard]] std::filesystem::path
human_tool_io_dir(const app_context_t &app) {
  return app.defaults.marshal_root / ".human_io";
}

[[nodiscard]] std::string make_human_tool_io_basename_(std::string_view stem) {
  const std::uint64_t started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  const std::uint64_t salt = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  return "h." + cuwacunu::hero::runtime::base36_lower_u64(started_at_ms) + "." +
         cuwacunu::hero::runtime::base36_lower_u64(
             static_cast<std::uint64_t>(::getpid())) +
         "." + cuwacunu::hero::runtime::base36_lower_u64(salt).substr(0, 6) +
         "." + std::string(stem);
}

[[nodiscard]] std::filesystem::path
make_human_tool_io_path(const app_context_t &app, std::string_view stem) {
  return human_tool_io_dir(app) / make_human_tool_io_basename_(stem);
}

void cleanup_human_tool_io(const std::filesystem::path &stdin_path,
                           const std::filesystem::path &stdout_path,
                           const std::filesystem::path &stderr_path) {
  std::error_code ec{};
  (void)std::filesystem::remove(stdin_path, ec);
  (void)std::filesystem::remove(stdout_path, ec);
  (void)std::filesystem::remove(stderr_path, ec);
}

[[nodiscard]] bool call_marshal_tool(const app_context_t &app,
                                     const std::string &tool_name,
                                     std::string arguments_json,
                                     std::string *out_structured,
                                     std::string *error) {
  if (error)
    error->clear();
  if (!out_structured) {
    if (error)
      *error = "marshal structured output pointer is null";
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

  const std::vector<std::string> argv{app.defaults.marshal_hero_binary.string(),
                                      "--global-config",
                                      app.global_config_path.string(),
                                      "--tool",
                                      tool_name,
                                      "--args-json",
                                      arguments_json};
  int exit_code = -1;
  std::string invoke_error{};
  const bool invoked = run_command_with_stdio_and_timeout(
      argv, stdin_path, stdout_path, stderr_path, kMarshalToolTimeoutSec,
      &exit_code, &invoke_error);

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
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string text{};
  std::string ignored{};
  if (!cuwacunu::hero::runtime::read_text_file(session.human_request_path,
                                               &text, &ignored)) {
    return {};
  }
  std::istringstream in(text);
  std::ostringstream out;
  std::string line{};
  std::size_t count = 0;
  while (count < 8 && std::getline(in, line)) {
    if (count != 0)
      out << "\n";
    out << line;
    ++count;
  }
  return out.str();
}

[[nodiscard]] std::filesystem::path summary_path_for_session(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return cuwacunu::hero::marshal::marshal_session_human_summary_path(
      std::filesystem::path(session.session_root).parent_path(),
      session.marshal_session_id);
}

[[nodiscard]] std::filesystem::path summary_ack_path_for_session(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return cuwacunu::hero::marshal::marshal_session_human_summary_ack_latest_path(
      std::filesystem::path(session.session_root).parent_path(),
      session.marshal_session_id);
}

[[nodiscard]] std::filesystem::path summary_ack_sig_path_for_session(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return cuwacunu::hero::marshal::
      marshal_session_human_summary_ack_latest_sig_path(
          std::filesystem::path(session.session_root).parent_path(),
          session.marshal_session_id);
}

[[nodiscard]] bool read_summary_text_for_session(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string *out, std::string *error) {
  if (!out) {
    if (error)
      *error = "summary text output pointer is null";
    return false;
  }
  return cuwacunu::hero::runtime::read_text_file(
      summary_path_for_session(session), out, error);
}

[[nodiscard]] std::string summary_excerpt(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string text{};
  std::string ignored{};
  if (!read_summary_text_for_session(session, &text, &ignored))
    return {};
  std::istringstream in(text);
  std::ostringstream out;
  std::string line{};
  std::size_t count = 0;
  while (count < 8 && std::getline(in, line)) {
    if (count != 0)
      out << "\n";
    out << line;
    ++count;
  }
  return out.str();
}

[[nodiscard]] bool summary_ack_matches_current_summary(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string *error) {
  return cuwacunu::hero::marshal::
      marshal_session_summary_ack_matches_current_summary(session, error);
}

[[nodiscard]] bool write_human_pending_marker_counts(
    const std::filesystem::path &marshal_root, std::size_t pending_requests,
    std::size_t pending_reviews, std::string *error) {
  if (error)
    error->clear();
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
          cuwacunu::hero::marshal::human_hero_pending_summary_count_path(
              marshal_root),
          std::to_string(pending_reviews) + "\n", error)) {
    return false;
  }
  return true;
}

void sync_human_pending_markers_best_effort(const app_context_t &app) {
  human_operator_inbox_t inbox{};
  std::string marker_error{};
  if (!collect_human_operator_inbox(app, &inbox, true, &marker_error)) {
    std::cerr << "[hero_human_mcp][warning] failed to refresh Human Hero "
                 "inbox markers: "
              << marker_error << std::endl;
  }
}

