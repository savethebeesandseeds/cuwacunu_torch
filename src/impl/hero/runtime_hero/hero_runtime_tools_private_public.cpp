
std::filesystem::path
resolve_runtime_hero_dsl_path(const std::filesystem::path &global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "REAL_HERO", "runtime_hero_dsl_filename");
  if (!configured.has_value())
    return {};
  const std::string resolved = resolve_path_from_base_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty())
    return {};
  return std::filesystem::path(resolved);
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

[[nodiscard]] std::filesystem::path
derive_campaigns_root(const std::filesystem::path &runtime_root) {
  if (runtime_root.empty())
    return {};
  return (runtime_root / ".campaigns").lexically_normal();
}

[[nodiscard]] std::filesystem::path
derive_marshal_root(const std::filesystem::path &runtime_root) {
  return cuwacunu::hero::marshal::marshal_root(runtime_root);
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

bool load_runtime_defaults(const std::filesystem::path &hero_dsl_path,
                           const std::filesystem::path &global_config_path,
                           runtime_defaults_t *out, std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "runtime defaults output pointer is null";
    return false;
  }
  *out = runtime_defaults_t{};

  std::ifstream in(hero_dsl_path);
  if (!in) {
    if (error)
      *error =
          "cannot open runtime HERO defaults DSL: " + hero_dsl_path.string();
    return false;
  }

  std::unordered_map<std::string, std::string> values{};
  std::string line;
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

  const auto resolve_local_path = [&](const std::string &value) {
    return std::filesystem::path(resolve_path_from_base_folder(
        hero_dsl_path.parent_path().string(), value));
  };

  out->campaigns_root = derive_campaigns_root(
      resolve_runtime_root_from_global_config(global_config_path));
  out->marshal_root = derive_marshal_root(
      resolve_runtime_root_from_global_config(global_config_path));
  bool saw_cuwacunu_campaign_binary = false;
  if (const auto it = values.find("cuwacunu_campaign_binary");
      it != values.end()) {
    saw_cuwacunu_campaign_binary = true;
    out->cuwacunu_campaign_binary = resolve_local_path(it->second);
  }
  out->campaign_grammar_path =
      resolve_campaign_grammar_from_global_config(global_config_path);
  bool saw_tail_default_lines = false;
  if (const auto it = values.find("tail_default_lines"); it != values.end()) {
    saw_tail_default_lines = true;
    if (!parse_size_token(it->second, &out->tail_default_lines)) {
      if (error)
        *error = "invalid tail_default_lines in " + hero_dsl_path.string();
      return false;
    }
  }

  if (!saw_cuwacunu_campaign_binary || out->cuwacunu_campaign_binary.empty()) {
    if (error) {
      *error = "missing cuwacunu_campaign_binary in " + hero_dsl_path.string();
    }
    return false;
  }
  if (out->campaigns_root.empty()) {
    if (error) {
      *error = "missing GENERAL.runtime_root in " + global_config_path.string();
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
  if (out->campaign_grammar_path.empty()) {
    if (error) {
      *error = "missing [BNF].iitepi_campaign_grammar_filename in " +
               global_config_path.string();
    }
    return false;
  }
  if (!saw_tail_default_lines || out->tail_default_lines == 0) {
    if (error)
      *error =
          "missing/invalid tail_default_lines in " + hero_dsl_path.string();
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

std::string build_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  const std::size_t tool_count =
      sizeof(kRuntimeTools) / sizeof(kRuntimeTools[0]);
  for (std::size_t i = 0; i < tool_count; ++i) {
    const auto &tool = kRuntimeTools[i];
    if (i != 0)
      out << ",";
    const bool read_only =
        tool.name == "hero.runtime.explain_binding_selection" ||
        tool.name == "hero.runtime.list_campaigns" ||
        tool.name == "hero.runtime.get_campaign" ||
        tool.name == "hero.runtime.list_jobs" ||
        tool.name == "hero.runtime.get_job" ||
        tool.name == "hero.runtime.tail_log" ||
        tool.name == "hero.runtime.tail_trace";
    const bool destructive = tool.name == "hero.runtime.stop_campaign" ||
                             tool.name == "hero.runtime.stop_job";
    out << "{\"name\":" << json_quote(tool.name)
        << ",\"description\":" << json_quote(tool.description)
        << ",\"inputSchema\":" << tool.input_schema_json
        << ",\"annotations\":{\"readOnlyHint\":"
        << (read_only ? "true" : "false")
        << ",\"destructiveHint\":" << (destructive ? "true" : "false")
        << ",\"openWorldHint\":false}}";
  }
  out << "]}";
  return out.str();
}

std::string build_tools_list_human_text() {
  std::ostringstream out;
  for (const auto &tool : kRuntimeTools) {
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

  const auto *descriptor = find_runtime_tool_descriptor(tool_name);
  if (descriptor == nullptr) {
    *out_error_message = "unknown tool: " + tool_name;
    return false;
  }

  std::string structured;
  std::string err;
  const bool ok = descriptor->handler(app, arguments_json, &structured, &err);
  if (!ok) {
    *out_tool_result_json = tool_result_json_for_error(err);
    return true;
  }
  *out_tool_result_json = make_tool_result_json("ok", structured, false);
  return true;
}

void run_jsonrpc_stdio_loop(app_context_t *app) {
  std::string request;
  while (true) {
    bool used_content_length = false;
    if (!read_next_jsonrpc_message(&std::cin, &request, &used_content_length)) {
      return;
    }
    if (used_content_length)
      g_jsonrpc_use_content_length_framing = true;

    std::string id_json = "null";
    if (!extract_json_id_field(request, &id_json)) {
      write_jsonrpc_error("null", -32700, "invalid request id");
      continue;
    }

    std::string method;
    if (!extract_json_string_field(request, "method", &method)) {
      write_jsonrpc_error(id_json, -32600, "missing method");
      continue;
    }

    if (method.rfind("notifications/", 0) == 0)
      continue;
    if (method == "initialize") {
      write_jsonrpc_result(id_json, initialize_result_json());
      continue;
    }
    if (method == "tools/list") {
      write_jsonrpc_result(id_json, build_tools_list_result_json());
      continue;
    }
    if (method != "tools/call") {
      write_jsonrpc_error(id_json, -32601, "unsupported method");
      continue;
    }

    std::string params;
    if (!extract_json_object_field(request, "params", &params)) {
      write_jsonrpc_error(id_json, -32602, "tools/call requires params object");
      continue;
    }
    std::string name;
    if (!extract_json_string_field(params, "name", &name) || name.empty()) {
      write_jsonrpc_error(id_json, -32602, "tools/call requires params.name");
      continue;
    }
    std::string arguments = "{}";
    std::string extracted_args;
    if (extract_json_object_field(params, "arguments", &extracted_args)) {
      arguments = extracted_args;
    }

    std::string tool_result;
    std::string tool_error;
    if (!execute_tool_json(name, arguments, app, &tool_result, &tool_error)) {
      write_jsonrpc_error(id_json, -32601, tool_error);
      continue;
    }
    write_jsonrpc_result(id_json, tool_result);
  }
}

} // namespace runtime_mcp
} // namespace hero
} // namespace cuwacunu
