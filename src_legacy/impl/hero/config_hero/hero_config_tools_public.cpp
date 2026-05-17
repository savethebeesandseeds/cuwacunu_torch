namespace cuwacunu {
namespace hero {
namespace mcp {

using namespace detail;

[[nodiscard]] std::string build_tools_list_result_json() {
  return build_tools_list_result_json_impl();
}

[[nodiscard]] std::string build_tools_list_human_text() {
  return build_tools_list_human_text_impl();
}

bool execute_tool_json(const std::string &tool_name, std::string arguments_json,
                       hero_config_store_t *store,
                       std::string *out_tool_result_json,
                       std::string *out_error_message) {
  if (out_tool_result_json)
    out_tool_result_json->clear();
  if (out_error_message)
    out_error_message->clear();

  if (!store) {
    if (out_error_message)
      *out_error_message = "store is null";
    return false;
  }

  const std::string trimmed_name = trim_ascii(tool_name);
  if (trimmed_name.empty()) {
    if (out_error_message)
      *out_error_message = "tool name is empty";
    return false;
  }

  arguments_json = trim_ascii(arguments_json);
  if (arguments_json.empty())
    arguments_json = "{}";
  if (arguments_json.front() != '{') {
    if (out_error_message) {
      *out_error_message = "tool arguments must be a JSON object";
    }
    return false;
  }

  std::string structured_result_json;
  int error_code = -32603;
  std::string error_message;
  if (!dispatch_tool_jsonrpc(trimmed_name, arguments_json, store,
                             &structured_result_json, &error_code,
                             &error_message)) {
    if (out_tool_result_json) {
      const std::string text = std::string("tool error: ") + error_message;
      *out_tool_result_json = make_text_content_result_json(
          text, true, error_code, detect_error_tag(error_message));
    }
    if (out_error_message)
      *out_error_message = error_message;
    return false;
  }

  if (out_tool_result_json) {
    *out_tool_result_json =
        make_tool_success_result_json(trimmed_name, structured_result_json);
  }
  return true;
}

void run_jsonrpc_stdio_loop(hero_config_store_t *store) {
  std::string payload;
  bool shutdown_requested = false;
  while (true) {
    if (!wait_for_stdio_activity())
      break;

    bool used_content_length = false;
    if (!read_next_jsonrpc_message(&std::cin, &payload, &used_content_length)) {
      break;
    }
    g_jsonrpc_use_content_length_framing = used_content_length;

    const std::string trimmed = trim_ascii(payload);
    if (trimmed.empty())
      continue;

    std::string method;
    if (!extract_json_string_field(trimmed, "method", &method) ||
        method.empty()) {
      emit_jsonrpc_error("null", -32600, "invalid request: missing method");
      continue;
    }

    if (method == "exit")
      break;
    if (method.rfind("notifications/", 0) == 0) {
      continue;
    }

    const bool has_id = has_json_field(trimmed, "id");
    std::string id_json = "null";
    if (!has_id) {
      emit_jsonrpc_error(id_json, -32600, "invalid request: missing id");
      continue;
    }
    if (!extract_json_id_field(trimmed, &id_json)) {
      emit_jsonrpc_error("null", -32700, "invalid request: unable to parse id");
      continue;
    }

    if (method == "shutdown") {
      emit_jsonrpc_result(id_json, "{}");
      shutdown_requested = true;
      continue;
    }
    if (shutdown_requested) {
      emit_jsonrpc_error(id_json, -32000, "server is shutting down");
      continue;
    }

    if (method == "initialize") {
      std::string protocol_version = kDefaultMcpProtocolVersion;
      std::string params_json;
      if (extract_json_object_field(trimmed, "params", &params_json)) {
        std::string protocol_candidate;
        if (extract_json_string_field(params_json, "protocolVersion",
                                      &protocol_candidate) &&
            !protocol_candidate.empty()) {
          protocol_version = protocol_candidate;
        }
      } else {
        // Fallback for initialize requests that omit the params object.
        std::string protocol_candidate;
        if (extract_json_string_field(trimmed, "protocolVersion",
                                      &protocol_candidate) &&
            !protocol_candidate.empty()) {
          protocol_version = protocol_candidate;
        }
      }
      emit_jsonrpc_result(
          id_json,
          std::string("{\"protocolVersion\":") + json_quote(protocol_version) +
              ",\"capabilities\":{\"tools\":{\"listChanged\":false}},"
              "\"serverInfo\":{\"name\":" +
              json_quote(kMcpServerName) +
              ",\"version\":" + json_quote(kMcpServerVersion) +
              "},\"instructions\":" + json_quote(kMcpInitializeInstructions) +
              "}");
      continue;
    }
    if (method == "ping") {
      emit_jsonrpc_result(id_json, "{}");
      continue;
    }
    if (method == "tools/list") {
      emit_jsonrpc_result(id_json, build_tools_list_result_json_impl());
      continue;
    }
    if (method == "resources/list") {
      emit_jsonrpc_result(id_json, "{\"resources\":[]}");
      continue;
    }
    if (method == "resources/templates/list") {
      emit_jsonrpc_result(id_json, "{\"resourceTemplates\":[]}");
      continue;
    }

    if (method == "tools/call" || method.rfind("hero.", 0) == 0) {
      std::string tool_name = method;
      std::string dispatch_json = trimmed;
      if (method == "tools/call") {
        std::string params_json;
        if (!extract_json_object_field(trimmed, "params", &params_json)) {
          if (has_id) {
            emit_jsonrpc_error(id_json, -32602,
                               "tools/call requires params object");
          }
          continue;
        }
        if (!extract_json_string_field(params_json, "name", &tool_name) ||
            tool_name.empty()) {
          if (has_id) {
            emit_jsonrpc_error(id_json, -32602,
                               "tools/call requires params.name");
          }
          continue;
        }

        std::string arguments_json;
        if (extract_json_raw_field(params_json, "arguments", &arguments_json)) {
          arguments_json = trim_ascii(arguments_json);
          if (arguments_json.empty() || arguments_json.front() != '{') {
            if (has_id) {
              emit_jsonrpc_error(id_json, -32602,
                                 "tools/call requires params.arguments object");
            }
            continue;
          }
          dispatch_json = arguments_json;
        } else {
          dispatch_json = "{}";
        }
      } else {
        std::string params_json;
        if (extract_json_object_field(trimmed, "params", &params_json)) {
          dispatch_json = params_json;
        }
      }

      std::string result_json;
      int error_code = -32603;
      std::string error_message;
      if (!dispatch_tool_jsonrpc(tool_name, dispatch_json, store, &result_json,
                                 &error_code, &error_message)) {
        if (method == "tools/call") {
          const std::string text = std::string("tool error: ") + error_message;
          emit_jsonrpc_result(id_json, make_text_content_result_json(
                                           text, true, error_code,
                                           detect_error_tag(error_message)));
        } else {
          emit_jsonrpc_error(id_json, error_code, error_message);
        }
        continue;
      }
      if (method == "tools/call") {
        emit_jsonrpc_result(
            id_json, make_tool_success_result_json(tool_name, result_json));
      } else {
        emit_jsonrpc_result(id_json, result_json);
      }
      continue;
    }

    emit_jsonrpc_error(id_json, -32601, "method not found: " + method);
  }
}

} // namespace mcp
} // namespace hero
} // namespace cuwacunu
