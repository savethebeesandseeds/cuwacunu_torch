[[nodiscard]] bool handle_tool_optim_read(std::string_view tool_name,
                                          const std::string &request_json,
                                          hero_config_store_t *store,
                                          std::string *out_result_json,
                                          int *out_error_code,
                                          std::string *out_error_message) {
  bool include_man = false;
  if (!parse_optim_read_include_man_flag(tool_name, request_json, &include_man,
                                         out_error_code, out_error_message)) {
    return false;
  }
  std::string path_raw{};
  if (!extract_json_string_field(request_json, "path", &path_raw) ||
      path_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }

  tsodao_surface_t surface{};
  std::string err{};
  if (!resolve_tsodao_surface(*store, &surface, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  const bool payload_present = tsodao_hidden_payload_present(surface);
  const bool archive_present = std::filesystem::exists(surface.hidden_archive);
  if (!payload_present && archive_present) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          "optim plaintext surface is currently scrubbed under " +
          surface.hidden_root.string() +
          "; run tsodao sync to restore it before hero.config.optim.read";
    }
    return false;
  }

  std::filesystem::path dsl_path{};
  if (!resolve_optim_path_with_scope(*store, surface, path_raw,
                                     /*allow_missing_target=*/false, &dsl_path,
                                     &err)) {
    if (out_error_code)
      *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::string content{};
  if (!read_text_file(dsl_path.string(), &content, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::string sha256_hex{};
  if (!sha256_hex_text(content, &sha256_hex, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  const auto validation_family_enum =
      detect_instruction_dsl_validation_family(dsl_path);
  const std::string validation_family =
      instruction_dsl_validation_family_name(validation_family_enum);
  const std::filesystem::path config_root =
      resolve_config_root_from_global_config(*store);
  const std::filesystem::path man_path = find_associated_man_path_with_fallback(
      config_root, dsl_path, validation_family_enum);
  const std::string warning =
      man_path.empty() && should_warn_missing_associated_man(
                              dsl_path, validation_family_enum)
          ? missing_associated_man_warning(dsl_path)
          : "";
  if (!warning.empty())
    log_config_warning(warning);

  if (out_result_json) {
    std::ostringstream out;
    std::error_code ec{};
    const std::string relative_path =
        std::filesystem::relative(dsl_path, surface.hidden_root, ec).string();
    out << "{\"optim_root\":" << json_quote(surface.hidden_root.string())
        << ",\"archive_path\":" << json_quote(surface.hidden_archive.string())
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"relative_path\":" << json_quote(relative_path)
        << ",\"bytes\":" << content.size()
        << ",\"sha256\":" << json_quote(sha256_hex)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"replace_supported\":"
        << bool_json(validation_family != "unsupported")
        << ",\"content\":" << json_quote(content);
    if (!append_associated_man_fields(&out, man_path, include_man, warning,
                                      &err)) {
      if (out_error_code)
        *out_error_code = -32603;
      if (out_error_message)
        *out_error_message = err;
      return false;
    }
    out << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_optim_list(std::string_view tool_name,
                                          const std::string &request_json,
                                          hero_config_store_t *store,
                                          std::string *out_result_json,
                                          int *out_error_code,
                                          std::string *out_error_message) {
  bool include_man = false;
  bool parsed_include_man = false;
  if (extract_json_raw_field(request_json, "include_man", nullptr)) {
    if (!extract_json_bool_field(request_json, "include_man", &include_man)) {
      if (out_error_code)
        *out_error_code = -32602;
      if (out_error_message) {
        *out_error_message =
            std::string(tool_name) + " include_man must be boolean";
      }
      return false;
    }
    parsed_include_man = true;
  }
  if (!parsed_include_man &&
      extract_json_bool_field(request_json, "includeMan", &include_man)) {
    parsed_include_man = true;
  }
  (void)parsed_include_man;

  tsodao_surface_t surface{};
  std::string err{};
  if (!resolve_tsodao_surface(*store, &surface, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  const bool payload_present = tsodao_hidden_payload_present(surface);
  const bool archive_present = std::filesystem::exists(surface.hidden_archive);
  std::vector<std::string> allowed_extensions{};
  if (!collect_allowed_extensions(*store, &allowed_extensions, &err)) {
    if (out_error_code)
      *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::vector<std::filesystem::path> files{};
  if (payload_present &&
      !list_instruction_files_under_root(surface.hidden_root,
                                         allowed_extensions, &files, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  const std::filesystem::path config_root =
      resolve_config_root_from_global_config(*store);

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"optim_root\":" << json_quote(surface.hidden_root.string())
        << ",\"archive_path\":" << json_quote(surface.hidden_archive.string())
        << ",\"public_keep_path\":"
        << json_quote(surface.public_keep_path.string())
        << ",\"archive_present\":" << bool_json(archive_present)
        << ",\"payload_present\":" << bool_json(payload_present)
        << ",\"files\":[";
    for (std::size_t i = 0; i < files.size(); ++i) {
      const auto &path = files[i];
      std::error_code ec{};
      const std::string relative_path =
          std::filesystem::relative(path, surface.hidden_root, ec).string();
      const auto validation_family_enum =
          detect_instruction_dsl_validation_family(path);
      const std::string validation_family =
          instruction_dsl_validation_family_name(validation_family_enum);
      const std::filesystem::path man_path =
          find_associated_man_path_with_fallback(config_root, path,
                                                 validation_family_enum);
      const std::string warning =
          man_path.empty() && should_warn_missing_associated_man(
                                  path, validation_family_enum)
              ? missing_associated_man_warning(path)
              : "";
      if (!warning.empty())
        log_config_warning(warning);
      if (i != 0)
        out << ",";
      out << "{\"path\":" << json_quote(path.string())
          << ",\"relative_path\":" << json_quote(relative_path)
          << ",\"validation_family\":" << json_quote(validation_family)
          << ",\"replace_supported\":"
          << bool_json(validation_family != "unsupported");
      if (!append_associated_man_fields(&out, man_path, include_man, warning,
                                        &err)) {
        if (out_error_code)
          *out_error_code = -32603;
        if (out_error_message)
          *out_error_message = err;
        return false;
      }
      out << "}";
    }
    out << "],\"count\":" << files.size();
    if (!payload_present && archive_present) {
      out << ",\"note\":"
          << json_quote("optim plaintext surface is currently scrubbed; run "
                        "tsodao sync to restore it before reading files");
    }
    out << "}";
    *out_result_json = out.str();
  }
  return true;
}
