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
    if (loop.lifecycle == "live" && has_pending_operator_message) {
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
