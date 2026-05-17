std::filesystem::path
resolve_human_hero_dsl_path(const std::filesystem::path &global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "REAL_HERO", "human_hero_dsl_filename");
  if (!configured.has_value())
    return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty())
    return {};
  return std::filesystem::path(resolved);
}

bool load_human_defaults(const std::filesystem::path &hero_dsl_path,
                         const std::filesystem::path &global_config_path,
                         human_defaults_t *out, std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "human defaults output pointer is null";
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
  const auto resolve_exec = [&](const char *key, std::filesystem::path *dst) {
    const auto it = values.find(key);
    if (it == values.end())
      return false;
    *dst = resolve_command_path(hero_dsl_path.parent_path(), it->second);
    return !dst->empty();
  };
  if (!resolve_exec("marshal_hero_binary", &out->marshal_hero_binary)) {
    if (error)
      *error =
          "missing/invalid marshal_hero_binary in " + hero_dsl_path.string();
    return false;
  }
  out->operator_id = trim_ascii(values["operator_id"]);
  if (operator_id_needs_bootstrap(out->operator_id)) {
    const std::string bootstrapped_operator_id = derive_bootstrap_operator_id();
    std::string bootstrap_error{};
    if (!persist_bootstrap_operator_id(hero_dsl_path, bootstrapped_operator_id,
                                       &bootstrap_error)) {
      log_warn("[hero_human_mcp] failed to persist bootstrapped "
               "operator_id to %s: %s",
               hero_dsl_path.string().c_str(), bootstrap_error.c_str());
    } else {
      log_warn("[hero_human_mcp] auto-initialized missing operator_id to %s "
               "in %s",
               bootstrapped_operator_id.c_str(),
               hero_dsl_path.string().c_str());
    }
    out->operator_id = bootstrapped_operator_id;
  }
  const auto ssh_identity_it = values.find("operator_signing_ssh_identity");
  if (ssh_identity_it != values.end() &&
      !trim_ascii(ssh_identity_it->second).empty()) {
    out->operator_signing_ssh_identity =
        std::filesystem::path(resolve_path_from_base_folder(
            hero_dsl_path.parent_path().string(), ssh_identity_it->second));
  }
  if (out->marshal_root.empty()) {
    if (error) {
      *error =
          "cannot derive .marshal_hero root from GENERAL.runtime_root in " +
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

[[nodiscard]] bool human_tool_is_read_only(std::string_view name) {
  return name == "hero.human.list_requests" ||
         name == "hero.human.list_summaries" ||
         name == "hero.human.get_request" || name == "hero.human.get_summary";
}

[[nodiscard]] bool human_tool_is_destructive(std::string_view name) {
  (void)name;
  return false;
}

std::string build_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  bool first = true;
  for (const auto &tool : kHumanTools) {
    if (!first)
      out << ",";
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
  for (const auto &tool : kHumanTools) {
    out << tool.name << "\t" << tool.description << "\n";
  }
  return out.str();
}

bool tool_result_is_error(std::string_view tool_result_json) {
  return tool_result_json.find("\"isError\":true") != std::string_view::npos ||
         tool_result_json.find("\"isError\": true") != std::string_view::npos;
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

  const auto *descriptor = find_human_tool_descriptor(tool_name);
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

[[nodiscard]] bool run_line_prompt_operator_console(app_context_t *app,
                                                    std::string *error) {
  if (error)
    error->clear();
  if (!app) {
    if (error)
      *error = "human app pointer is null";
    return false;
  }

  human_operator_inbox_t inbox{};
  if (!collect_human_operator_inbox(*app, &inbox, true, error))
    return false;
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
    std::cout
        << "status: no pending human requests or review reports\n"
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
        std::cout << "  [" << i + 1 << "] " << pending[i].marshal_session_id
                  << "  " << pending[i].objective_name << "  "
                  << operator_session_state_detail(pending[i], false);
        if (!pending[i].status_detail.empty()) {
          std::cout << "  " << pending[i].status_detail;
        }
        std::cout << "\n";
      }
      std::cout << "Select request number (blank to cancel): " << std::flush;
      std::string line{};
      if (!std::getline(std::cin, line))
        return false;
      line = trim_ascii(line);
      if (line.empty())
        return true;
      std::size_t choice = 0;
      if (!parse_size_token(line, &choice) || choice == 0 ||
          choice > pending.size()) {
        if (error)
          *error = "invalid request selection";
        return false;
      }
      selected = choice - 1;
    }
    const auto &session = pending[selected];
    std::string request_text{};
    if (!cuwacunu::hero::runtime::read_text_file(session.human_request_path,
                                                 &request_text, error)) {
      return false;
    }
    std::cout << "\n=== Human Request ===\n" << request_text << "\n";

    if (is_clarification_work_gate(session.work_gate)) {
      std::cout << "Provide clarification answer (blank to cancel): "
                << std::flush;
      std::string answer{};
      if (!std::getline(std::cin, answer))
        return false;
      answer = trim_ascii(answer);
      if (answer.empty())
        return true;
      std::string structured{};
      if (!build_request_answer_and_resume(app, session, answer, &structured,
                                           error)) {
        return false;
      }
      std::cout << "\nClarification answer applied.\n" << structured << "\n";
      return true;
    }

    std::cout << "Resolve with [g]rant, [c]larify, [d]eny, [t]erminate "
                 "session, or blank to cancel: "
              << std::flush;
    std::string resolution{};
    if (!std::getline(std::cin, resolution))
      return false;
    resolution = lowercase_copy(trim_ascii(resolution));
    if (resolution.empty())
      return true;

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
      if (error)
        *error = "invalid resolution selection";
      return false;
    }

    std::cout << "Reason shown to Marshal Hero: " << std::flush;
    std::string reason{};
    if (!std::getline(std::cin, reason))
      return false;
    reason = trim_ascii(reason);
    if (reason.empty()) {
      if (error)
        *error = "reason is required";
      return false;
    }

    std::string structured{};
    if (!build_resolution_and_apply(app, session, resolution_kind, reason,
                                    &structured, error)) {
      return false;
    }
    std::cout << "\nSigned governance resolution applied.\n"
              << structured << "\n";
    return true;
  }

  std::cout << "status: review report available; continue launches more work, "
               "acknowledge records a required review message and clears the "
               "report\n";
  std::size_t selected = 0;
  if (summaries.size() > 1) {
    std::cout << "Pending reviews:\n";
    for (std::size_t i = 0; i < summaries.size(); ++i) {
      std::cout << "  [" << i + 1 << "] " << summaries[i].marshal_session_id
                << "  " << summaries[i].objective_name << "  "
                << operator_session_state_detail(summaries[i], true) << "  "
                << build_summary_effort_text(summaries[i]) << "\n";
    }
    std::cout << "Select review number (blank to cancel): " << std::flush;
    std::string line{};
    if (!std::getline(std::cin, line))
      return false;
    line = trim_ascii(line);
    if (line.empty())
      return true;
    std::size_t choice = 0;
    if (!parse_size_token(line, &choice) || choice == 0 ||
        choice > summaries.size()) {
      if (error)
        *error = "invalid review selection";
      return false;
    }
    selected = choice - 1;
  }
  const auto &session = summaries[selected];
  std::string summary_text{};
  if (!read_summary_text_for_session(session, &summary_text, error))
    return false;
  std::cout
      << "\n=== Human Review Report ===\n"
      << "[message sends fresh operator guidance; acknowledge records a "
         "required review "
         "message and clears this report from the operator queue]\n\n"
      << summary_text << "\n";

  const bool can_message =
      (session.lifecycle == "live" && session.activity == "review");
  std::cout << "Resolve this review? "
            << (can_message ? "[m]essage session or [a]cknowledge review, or "
                              "blank to cancel: "
                            : "[a]cknowledge review, or blank to cancel: ")
            << std::flush;
  std::string control{};
  if (!std::getline(std::cin, control))
    return false;
  control = lowercase_copy(trim_ascii(control));
  if (control.empty())
    return true;
  if ((!can_message || (control != "m" && control != "message")) &&
      control != "a" && control != "ack" && control != "acknowledge") {
    if (error)
      *error = "invalid review action selection";
    return false;
  }
  if (can_message && (control == "m" || control == "message")) {
    std::cout << "Operator message for the live session: " << std::flush;
    std::string instruction{};
    if (!std::getline(std::cin, instruction))
      return false;
    instruction = trim_ascii(instruction);
    if (instruction.empty()) {
      if (error)
        *error = "operator message is required";
      return false;
    }
    std::string structured{};
    if (!message_marshal_session(app, session, instruction, &structured,
                                  error)) {
      return false;
    }
    std::cout << "\nSession message applied.\n" << structured << "\n";
    return true;
  }

  std::cout << "Acknowledgment message (required; this does not message the "
               "session): "
            << std::flush;
  std::string note{};
  if (!std::getline(std::cin, note))
    return false;
  note = trim_ascii(note);
  if (note.empty()) {
    if (error)
      *error = "review acknowledgment requires a non-empty note";
    return false;
  }

  std::string structured{};
  if (!acknowledge_session_summary(app, session, note, &structured, error)) {
    return false;
  }
  std::cout << "\nHuman review acknowledgment applied.\n" << structured << "\n";
  return true;
}

bool run_interactive_operator_console(app_context_t *app, std::string *error) {
  if (error)
    error->clear();
  if (!terminal_supports_human_ncurses_ui()) {
    log_warn("[hero_human_mcp] ncurses UI unavailable for TERM=%s; falling "
             "back to line prompt operator console.",
             trim_ascii(std::getenv("TERM") == nullptr ? "" : std::getenv("TERM"))
                 .c_str());
    return run_line_prompt_operator_console(app, error);
  }
  std::string ncurses_error{};
  if (run_ncurses_operator_console(app, &ncurses_error))
    return true;
  if (!ncurses_error.empty() &&
      ncurses_error.rfind(kNcursesInitErrorPrefix, 0) == 0) {
    return run_line_prompt_operator_console(app, error);
  }
  if (error)
    *error = ncurses_error.empty() ? "interactive operator console failed"
                                   : ncurses_error;
  return false;
}

void run_jsonrpc_stdio_loop(app_context_t *app) {
  std::string line;
  while (std::getline(std::cin, line)) {
    const std::string request_json = trim_ascii(line);
    if (request_json.empty())
      continue;
    if (request_json.size() > kMaxJsonRpcPayloadBytes)
      continue;

    std::string id_raw{"null"};
    std::string method{};
    std::string params_json{"{}"};
    if (extract_json_field_raw(request_json, "id", &id_raw)) {
      id_raw = trim_ascii(id_raw);
      if (id_raw.empty())
        id_raw = "null";
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
      std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                << ",\"result\":{\"protocolVersion\":"
                << json_quote(kProtocolVersion)
                << ",\"serverInfo\":{\"name\":" << json_quote(kServerName)
                << ",\"version\":" << json_quote(kServerVersion)
                << "},\"instructions\":" << json_quote(kInitializeInstructions)
                << "}}\n";
      std::cout.flush();
      continue;
    }
    if (method == "notifications/initialized")
      continue;
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
        std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
                  << ",\"error\":{\"code\":-32602,\"message\":\"missing tool "
                     "name\"}}\n";
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
    std::cout
        << "{\"jsonrpc\":\"2.0\",\"id\":" << id_raw
        << ",\"error\":{\"code\":-32601,\"message\":\"method not found\"}}\n";
    std::cout.flush();
  }
}
