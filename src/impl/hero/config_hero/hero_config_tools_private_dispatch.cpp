[[nodiscard]] const hero_config_tool_descriptor_t *
find_tool_descriptor(std::string_view name) {
  for (const auto &descriptor : kHeroConfigTools) {
    if (descriptor.name == name)
      return &descriptor;
  }
  return nullptr;
}

[[nodiscard]] bool hero_config_tool_is_read_only(std::string_view name) {
  return name == "hero.config.status" || name == "hero.config.schema" ||
         name == "hero.config.show" || name == "hero.config.get" ||
         name == "hero.config.default.read" ||
         name == "hero.config.default.list" ||
         name == "hero.config.temp.read" || name == "hero.config.temp.list" ||
         name == "hero.config.objective.read" ||
         name == "hero.config.objective.list" ||
         name == "hero.config.optim.read" || name == "hero.config.optim.list" ||
         name == "hero.config.optim.backups" ||
         name == "hero.config.validate" || name == "hero.config.diff" ||
         name == "hero.config.dry_run" || name == "hero.config.backups";
}

[[nodiscard]] bool hero_config_tool_is_destructive(std::string_view name) {
  return name == "hero.config.default.delete" ||
         name == "hero.config.temp.delete" ||
         name == "hero.config.objective.delete" ||
         name == "hero.config.optim.delete" ||
         name == "hero.config.optim.rollback" ||
         name == "hero.config.rollback" || name == "hero.config.dev_nuke_reset";
}

[[nodiscard]] std::string
hero_config_tool_annotations_json(std::string_view name) {
  std::ostringstream out;
  out << "{\"readOnlyHint\":" << bool_json(hero_config_tool_is_read_only(name))
      << ",\"destructiveHint\":"
      << bool_json(hero_config_tool_is_destructive(name))
      << ",\"openWorldHint\":false}";
  return out.str();
}

[[nodiscard]] std::string build_tools_list_result_json_impl() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < std::size(kHeroConfigTools); ++i) {
    const auto &tool = kHeroConfigTools[i];
    if (i != 0)
      out << ",";
    out << "{"
        << "\"name\":" << json_quote(tool.name) << ","
        << "\"description\":" << json_quote(tool.description) << ","
        << "\"inputSchema\":" << tool.input_schema_json << ","
        << "\"annotations\":" << hero_config_tool_annotations_json(tool.name)
        << "}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string build_tools_list_human_text_impl() {
  std::ostringstream out;
  for (const auto &tool : kHeroConfigTools) {
    out << tool.name << "\t" << tool.description << "\n";
  }
  return out.str();
}

[[nodiscard]] std::string
path_vector_json(const std::vector<std::filesystem::path> &paths) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < paths.size(); ++i) {
    if (i != 0)
      out << ",";
    out << json_quote(paths[i].string());
  }
  out << "]";
  return out.str();
}

[[nodiscard]] bool
handle_tool_status(std::string_view tool_name, const std::string &request_json,
                   cuwacunu::hero::mcp::hero_config_store_t *store,
                   std::string *out_result_json, int *out_error_code,
                   std::string *out_error_message) {
  (void)tool_name;
  (void)request_json;
  (void)out_error_code;
  (void)out_error_message;
  if (out_result_json)
    *out_result_json = build_status_result(*store);
  return true;
}

[[nodiscard]] bool
handle_tool_schema(std::string_view tool_name, const std::string &request_json,
                   cuwacunu::hero::mcp::hero_config_store_t *store,
                   std::string *out_result_json, int *out_error_code,
                   std::string *out_error_message) {
  (void)tool_name;
  (void)request_json;
  (void)store;
  (void)out_error_code;
  (void)out_error_message;
  std::ostringstream out;
  out << "{\"keys\":[";
  for (std::size_t i = 0;
       i < cuwacunu::hero::config::kRuntimeKeyDescriptors.size(); ++i) {
    const auto &d = cuwacunu::hero::config::kRuntimeKeyDescriptors[i];
    if (i != 0)
      out << ",";
    out << "{"
        << "\"key\":" << json_quote(d.key) << ","
        << "\"type\":" << json_quote(d.type) << ","
        << "\"required\":" << bool_json(d.required) << ","
        << "\"default\":" << json_quote(d.default_value) << ","
        << "\"range\":" << json_quote(d.range) << ","
        << "\"allowed\":" << json_quote(d.allowed) << "}";
  }
  out << "]}";
  if (out_result_json)
    *out_result_json = out.str();
  return true;
}

[[nodiscard]] bool
handle_tool_show(std::string_view tool_name, const std::string &request_json,
                 cuwacunu::hero::mcp::hero_config_store_t *store,
                 std::string *out_result_json, int *out_error_code,
                 std::string *out_error_message) {
  (void)tool_name;
  (void)request_json;
  (void)out_error_code;
  (void)out_error_message;
  const auto entries = store->entries_snapshot();
  std::ostringstream out;
  out << "{\"entries\":[";
  for (std::size_t i = 0; i < entries.size(); ++i) {
    if (i != 0)
      out << ",";
    out << "{"
        << "\"key\":" << json_quote(entries[i].key) << ","
        << "\"domain\":" << json_quote(entries[i].declared_domain) << ","
        << "\"type\":" << json_quote(entries[i].declared_type) << ","
        << "\"value\":" << json_quote(entries[i].value) << "}";
  }
  out << "]}";
  if (out_result_json)
    *out_result_json = out.str();
  return true;
}

[[nodiscard]] bool
handle_tool_get(std::string_view tool_name, const std::string &request_json,
                cuwacunu::hero::mcp::hero_config_store_t *store,
                std::string *out_result_json, int *out_error_code,
                std::string *out_error_message) {
  std::string key;
  if (!extract_json_string_field(request_json, "key", &key) || key.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument key";
    }
    return false;
  }
  const auto value = store->get_value(key);
  if (!value.has_value()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message)
      *out_error_message = "key not found: " + key;
    return false;
  }
  if (out_result_json) {
    *out_result_json = "{\"key\":" + json_quote(key) +
                       ",\"value\":" + json_quote(*value) + "}";
  }
  return true;
}

[[nodiscard]] bool
handle_tool_set(std::string_view tool_name, const std::string &request_json,
                cuwacunu::hero::mcp::hero_config_store_t *store,
                std::string *out_result_json, int *out_error_code,
                std::string *out_error_message) {
  std::string key;
  std::string value;
  if (!extract_json_string_field(request_json, "key", &key) || key.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument key";
    }
    return false;
  }
  if (!extract_json_string_field(request_json, "value", &value)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument value";
    }
    return false;
  }
  std::string err;
  if (!store->set_value(key, value, &err)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (out_result_json) {
    *out_result_json = "{\"updated\":true,\"key\":" + json_quote(key) +
                       ",\"value\":" + json_quote(value) + "}";
  }
  return true;
}

[[nodiscard]] bool
handle_tool_validate(std::string_view tool_name,
                     const std::string &request_json,
                     cuwacunu::hero::mcp::hero_config_store_t *store,
                     std::string *out_result_json, int *out_error_code,
                     std::string *out_error_message) {
  (void)tool_name;
  (void)request_json;
  (void)out_error_code;
  (void)out_error_message;
  const auto errors = store->validate();
  std::ostringstream out;
  out << "{\"valid\":" << bool_json(errors.empty()) << ",\"errors\":[";
  for (std::size_t i = 0; i < errors.size(); ++i) {
    if (i != 0)
      out << ",";
    out << json_quote(errors[i]);
  }
  out << "]}";
  if (out_result_json)
    *out_result_json = out.str();
  return true;
}

[[nodiscard]] bool
handle_tool_diff(std::string_view tool_name, const std::string &request_json,
                 cuwacunu::hero::mcp::hero_config_store_t *store,
                 std::string *out_result_json, int *out_error_code,
                 std::string *out_error_message) {
  bool include_text = false;
  bool parsed_include_text = false;
  if (extract_json_raw_field(request_json, "include_text", nullptr)) {
    if (!extract_json_bool_field(request_json, "include_text", &include_text)) {
      if (out_error_code)
        *out_error_code = -32602;
      if (out_error_message) {
        *out_error_message =
            std::string(tool_name) + " include_text must be boolean";
      }
      return false;
    }
    parsed_include_text = true;
  }
  if (!parsed_include_text &&
      extract_json_bool_field(request_json, "includeText", &include_text)) {
    parsed_include_text = true;
  }
  (void)parsed_include_text;

  cuwacunu::hero::mcp::hero_config_store_t::save_preview_t preview;
  std::string err;
  if (!store->preview_save(&preview, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (out_result_json) {
    *out_result_json = build_save_preview_result(preview, include_text);
  }
  return true;
}

[[nodiscard]] bool
handle_tool_backups(std::string_view tool_name, const std::string &request_json,
                    cuwacunu::hero::mcp::hero_config_store_t *store,
                    std::string *out_result_json, int *out_error_code,
                    std::string *out_error_message) {
  (void)tool_name;
  (void)request_json;
  std::vector<cuwacunu::hero::mcp::hero_config_store_t::backup_entry_t> backups;
  std::string err;
  if (!store->list_backups(&backups, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::ostringstream out;
  out << "{\"count\":" << backups.size() << ",\"backups\":[";
  for (std::size_t i = 0; i < backups.size(); ++i) {
    if (i != 0)
      out << ",";
    out << "{"
        << "\"index\":" << i << ","
        << "\"filename\":" << json_quote(backups[i].filename) << ","
        << "\"path\":" << json_quote(backups[i].path) << "}";
  }
  out << "]}";
  if (out_result_json)
    *out_result_json = out.str();
  return true;
}

[[nodiscard]] bool
handle_tool_rollback(std::string_view tool_name,
                     const std::string &request_json,
                     cuwacunu::hero::mcp::hero_config_store_t *store,
                     std::string *out_result_json, int *out_error_code,
                     std::string *out_error_message) {
  std::string selector;
  if (extract_json_raw_field(request_json, "backup", nullptr)) {
    if (!extract_json_string_field(request_json, "backup", &selector)) {
      if (out_error_code)
        *out_error_code = -32602;
      if (out_error_message) {
        *out_error_message = std::string(tool_name) + " backup must be string";
      }
      return false;
    }
  }

  std::string selected;
  std::string err;
  if (!enforce_runtime_config_write_policy(*store, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!store->rollback_to_backup(selector, &selected, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (out_result_json) {
    *out_result_json =
        "{\"rolled_back\":true,\"path\":" + json_quote(store->config_path()) +
        ",\"selected_backup\":" + json_quote(selected) + "}";
  }
  return true;
}

[[nodiscard]] bool
handle_tool_save(std::string_view tool_name, const std::string &request_json,
                 cuwacunu::hero::mcp::hero_config_store_t *store,
                 std::string *out_result_json, int *out_error_code,
                 std::string *out_error_message) {
  (void)tool_name;
  (void)request_json;
  cuwacunu::hero::mcp::hero_config_store_t::save_preview_t preview;
  std::string err;
  if (!store->preview_save(&preview, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!enforce_runtime_config_write_policy(*store, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!store->save(&err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (out_result_json) {
    *out_result_json =
        "{\"saved\":true,\"path\":" + json_quote(store->config_path()) +
        ",\"component_identity\":" +
        build_component_identity_receipt_json_impl(
            preview, store->config_path(), store->global_config_path()) +
        "}";
  }
  return true;
}

[[nodiscard]] bool
handle_tool_reload(std::string_view tool_name, const std::string &request_json,
                   cuwacunu::hero::mcp::hero_config_store_t *store,
                   std::string *out_result_json, int *out_error_code,
                   std::string *out_error_message) {
  (void)tool_name;
  (void)request_json;
  std::string err;
  if (!store->load(&err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (out_result_json) {
    *out_result_json =
        "{\"reloaded\":true,\"path\":" + json_quote(store->config_path()) + "}";
  }
  return true;
}

[[nodiscard]] bool
dispatch_tool_jsonrpc(const std::string &tool_name,
                      const std::string &request_json,
                      cuwacunu::hero::mcp::hero_config_store_t *store,
                      std::string *out_result_json, int *out_error_code,
                      std::string *out_error_message) {
  if (out_error_code)
    *out_error_code = -32603;
  if (out_error_message)
    *out_error_message = "internal error";

  const auto *descriptor = find_tool_descriptor(tool_name);
  if (descriptor != nullptr) {
    return descriptor->handler(tool_name, request_json, store, out_result_json,
                               out_error_code, out_error_message);
  }

  if (out_error_code)
    *out_error_code = -32601;
  if (out_error_message)
    *out_error_message = "unknown tool: " + tool_name;
  return false;
}

} // namespace cuwacunu::hero::mcp::detail
