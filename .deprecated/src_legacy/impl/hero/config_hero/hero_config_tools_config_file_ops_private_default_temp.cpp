[[nodiscard]] bool handle_tool_default_read(std::string_view tool_name,
                                            const std::string &request_json,
                                            hero_config_store_t *store,
                                            std::string *out_result_json,
                                            int *out_error_code,
                                            std::string *out_error_message) {
  bool include_man = false;
  if (!parse_read_include_man_flag(tool_name, request_json, &include_man,
                                   out_error_code, out_error_message)) {
    return false;
  }
  std::string dsl_path_raw{};
  if (!extract_json_string_field(request_json, "path", &dsl_path_raw) ||
      dsl_path_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_default_dsl_path_with_scope(*store, dsl_path_raw, &dsl_path,
                                           &err, &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::string text{};
  if (!read_text_file(dsl_path.string(), &text, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::string sha256_hex{};
  if (!sha256_hex_text(text, &sha256_hex, &err)) {
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
    out << "{\"path\":" << json_quote(dsl_path.string())
        << ",\"bytes\":" << text.size()
        << ",\"sha256\":" << json_quote(sha256_hex)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"replace_supported\":"
        << bool_json(validation_family != "unsupported")
        << ",\"content\":" << json_quote(text);
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

[[nodiscard]] bool handle_tool_default_list(std::string_view tool_name,
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
  std::string err{};
  std::vector<std::filesystem::path> default_roots{};
  if (!collect_configured_root_paths(*store, "default_roots", &default_roots,
                                     &err)) {
    if (out_error_code)
      *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::vector<std::string> allowed_extensions{};
  if (!collect_allowed_extensions(*store, &allowed_extensions, &err)) {
    if (out_error_code)
      *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::vector<std::filesystem::path> files{};
  for (const auto &root : default_roots) {
    std::vector<std::filesystem::path> root_files{};
    if (!list_instruction_files_under_root(root, allowed_extensions,
                                           &root_files, &err)) {
      if (out_error_code)
        *out_error_code = -32603;
      if (out_error_message)
        *out_error_message = err;
      return false;
    }
    files.insert(files.end(), root_files.begin(), root_files.end());
  }
  const std::filesystem::path config_root =
      resolve_config_root_from_global_config(*store);
  std::sort(files.begin(), files.end(),
            [](const std::filesystem::path &a, const std::filesystem::path &b) {
              return a.string() < b.string();
            });
  files.erase(std::unique(files.begin(), files.end(),
                          [](const std::filesystem::path &a,
                             const std::filesystem::path &b) {
                            return a.string() == b.string();
                          }),
              files.end());

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"roots\":" << path_vector_json(default_roots) << ",\"files\":[";
    for (std::size_t i = 0; i < files.size(); ++i) {
      const auto &path = files[i];
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
      std::string matched_root{};
      std::string relative_path{};
      for (const auto &root : default_roots) {
        if (!path_is_within(root, path))
          continue;
        matched_root = root.string();
        std::error_code ec{};
        relative_path = std::filesystem::relative(path, root, ec).string();
        if (ec)
          relative_path.clear();
        break;
      }
      if (i != 0)
        out << ",";
      out << "{\"root\":" << json_quote(matched_root)
          << ",\"path\":" << json_quote(path.string())
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
    out << "],\"count\":" << files.size() << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_default_create(std::string_view tool_name,
                                              const std::string &request_json,
                                              hero_config_store_t *store,
                                              std::string *out_result_json,
                                              int *out_error_code,
                                              std::string *out_error_message) {
  std::string dsl_path_raw{};
  std::string content{};
  if (!extract_json_string_field(request_json, "path", &dsl_path_raw) ||
      dsl_path_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }
  if (!extract_json_string_field(request_json, "content", &content)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          std::string(tool_name) + " requires argument content";
    }
    return false;
  }

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_default_dsl_path_with_scope(*store, dsl_path_raw, &dsl_path,
                                           &err, &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!enforce_write_target_allowed(*store, dsl_path, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::error_code ec{};
  if (std::filesystem::exists(dsl_path, ec)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = "default file already exists: " + dsl_path.string();
    }
    return false;
  }

  std::string validation_family{};
  std::filesystem::path grammar_path{};
  if (!validate_plaintext_surface_replacement(*store, dsl_path, false, content,
                                              &validation_family, &grammar_path,
                                              &err)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!validate_default_dsl_replacement_semantics(*store, dsl_path, content,
                                                  &err)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (!write_text_file_atomic(dsl_path.string(), content, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::string after_sha256{};
  if (!sha256_hex_file(dsl_path, &after_sha256, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"created\":true,\"path\":" << json_quote(dsl_path.string())
        << ",\"bytes\":" << content.size()
        << ",\"sha256\":" << json_quote(after_sha256)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"grammar_path\":" << json_quote(grammar_path.string())
        << ",\"warning\":" << json_quote(default_mutation_warning()) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_default_replace(std::string_view tool_name,
                                               const std::string &request_json,
                                               hero_config_store_t *store,
                                               std::string *out_result_json,
                                               int *out_error_code,
                                               std::string *out_error_message) {
  std::string dsl_path_raw{};
  std::string content{};
  std::string expected_sha256{};
  if (!extract_json_string_field(request_json, "path", &dsl_path_raw) ||
      dsl_path_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }
  if (!extract_json_string_field(request_json, "content", &content)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          std::string(tool_name) + " requires argument content";
    }
    return false;
  }
  (void)extract_json_string_field(request_json, "expected_sha256",
                                  &expected_sha256);

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_default_dsl_path_with_scope(*store, dsl_path_raw, &dsl_path,
                                           &err, &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!enforce_write_target_allowed(*store, dsl_path, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::error_code ec{};
  const bool existed = std::filesystem::exists(dsl_path, ec) &&
                       std::filesystem::is_regular_file(dsl_path, ec);
  if (!existed) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = "default file does not exist: " + dsl_path.string();
    }
    return false;
  }
  std::string before_sha256{};
  if (!sha256_hex_file(dsl_path, &before_sha256, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  expected_sha256 = lowercase_copy(trim_ascii(expected_sha256));
  if (!expected_sha256.empty() &&
      lowercase_copy(before_sha256) != expected_sha256) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          "expected_sha256 does not match current default content: " +
          dsl_path.string();
    }
    return false;
  }

  std::string validation_family{};
  std::filesystem::path grammar_path{};
  if (!validate_plaintext_surface_replacement(*store, dsl_path, false, content,
                                              &validation_family, &grammar_path,
                                              &err)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!validate_default_dsl_replacement_semantics(*store, dsl_path, content,
                                                  &err)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (!backup_previous_write_target_with_cap(*store, dsl_path, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (!write_text_file_atomic(dsl_path.string(), content, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::string after_sha256{};
  if (!sha256_hex_file(dsl_path, &after_sha256, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"replaced\":true,\"path\":" << json_quote(dsl_path.string())
        << ",\"bytes\":" << content.size()
        << ",\"before_sha256\":" << json_quote(before_sha256)
        << ",\"sha256\":" << json_quote(after_sha256)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"grammar_path\":" << json_quote(grammar_path.string())
        << ",\"warning\":" << json_quote(default_mutation_warning()) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_default_delete(std::string_view tool_name,
                                              const std::string &request_json,
                                              hero_config_store_t *store,
                                              std::string *out_result_json,
                                              int *out_error_code,
                                              std::string *out_error_message) {
  std::string dsl_path_raw{};
  std::string expected_sha256{};
  if (!extract_json_string_field(request_json, "path", &dsl_path_raw) ||
      dsl_path_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }
  (void)extract_json_string_field(request_json, "expected_sha256",
                                  &expected_sha256);

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_default_dsl_path_with_scope(*store, dsl_path_raw, &dsl_path,
                                           &err, &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!enforce_write_target_allowed(*store, dsl_path, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::string before_sha256{};
  if (!sha256_hex_file(dsl_path, &before_sha256, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  expected_sha256 = lowercase_copy(trim_ascii(expected_sha256));
  if (!expected_sha256.empty() &&
      lowercase_copy(before_sha256) != expected_sha256) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          "expected_sha256 does not match current default content: " +
          dsl_path.string();
    }
    return false;
  }

  if (!backup_previous_write_target_with_cap(*store, dsl_path, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::error_code ec{};
  if (!std::filesystem::remove(dsl_path, ec) || ec) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message =
          "failed to delete default file: " + dsl_path.string();
    }
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"deleted\":true,\"path\":" << json_quote(dsl_path.string())
        << ",\"before_sha256\":" << json_quote(before_sha256)
        << ",\"warning\":" << json_quote(default_mutation_warning()) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_temp_read(std::string_view tool_name,
                                         const std::string &request_json,
                                         hero_config_store_t *store,
                                         std::string *out_result_json,
                                         int *out_error_code,
                                         std::string *out_error_message) {
  bool include_man = false;
  if (!parse_read_include_man_flag(tool_name, request_json, &include_man,
                                   out_error_code, out_error_message)) {
    return false;
  }
  std::string dsl_path_raw{};
  if (!extract_json_string_field(request_json, "path", &dsl_path_raw) ||
      dsl_path_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_temp_dsl_path_with_scope(*store, dsl_path_raw, &dsl_path, &err,
                                        &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::string text{};
  if (!read_text_file(dsl_path.string(), &text, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::string sha256_hex{};
  if (!sha256_hex_text(text, &sha256_hex, &err)) {
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
    out << "{\"path\":" << json_quote(dsl_path.string())
        << ",\"bytes\":" << text.size()
        << ",\"sha256\":" << json_quote(sha256_hex)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"replace_supported\":"
        << bool_json(validation_family != "unsupported")
        << ",\"content\":" << json_quote(text);
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

[[nodiscard]] bool handle_tool_temp_list(std::string_view tool_name,
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
  std::string err{};
  std::vector<std::filesystem::path> temp_roots{};
  if (!collect_configured_root_paths(*store, "temp_roots", &temp_roots, &err)) {
    if (out_error_code)
      *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::vector<std::string> allowed_extensions{};
  if (!collect_allowed_extensions(*store, &allowed_extensions, &err)) {
    if (out_error_code)
      *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::vector<std::filesystem::path> files{};
  for (const auto &root : temp_roots) {
    std::vector<std::filesystem::path> root_files{};
    if (!list_instruction_files_under_root(root, allowed_extensions,
                                           &root_files, &err)) {
      if (out_error_code)
        *out_error_code = -32603;
      if (out_error_message)
        *out_error_message = err;
      return false;
    }
    files.insert(files.end(), root_files.begin(), root_files.end());
  }
  const std::filesystem::path config_root =
      resolve_config_root_from_global_config(*store);
  std::sort(files.begin(), files.end(),
            [](const std::filesystem::path &a, const std::filesystem::path &b) {
              return a.string() < b.string();
            });
  files.erase(std::unique(files.begin(), files.end(),
                          [](const std::filesystem::path &a,
                             const std::filesystem::path &b) {
                            return a.string() == b.string();
                          }),
              files.end());

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"roots\":" << path_vector_json(temp_roots) << ",\"files\":[";
    for (std::size_t i = 0; i < files.size(); ++i) {
      const auto &path = files[i];
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
      std::string matched_root{};
      std::string relative_path{};
      for (const auto &root : temp_roots) {
        if (!path_is_within(root, path))
          continue;
        matched_root = root.string();
        std::error_code ec{};
        relative_path = std::filesystem::relative(path, root, ec).string();
        if (ec)
          relative_path.clear();
        break;
      }
      if (i != 0)
        out << ",";
      out << "{\"root\":" << json_quote(matched_root)
          << ",\"path\":" << json_quote(path.string())
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
    out << "],\"count\":" << files.size() << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_temp_create(std::string_view tool_name,
                                           const std::string &request_json,
                                           hero_config_store_t *store,
                                           std::string *out_result_json,
                                           int *out_error_code,
                                           std::string *out_error_message) {
  std::string dsl_path_raw{};
  std::string content{};
  if (!extract_json_string_field(request_json, "path", &dsl_path_raw) ||
      dsl_path_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }
  if (!extract_json_string_field(request_json, "content", &content)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          std::string(tool_name) + " requires argument content";
    }
    return false;
  }

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_temp_dsl_path_with_scope(*store, dsl_path_raw, &dsl_path, &err,
                                        &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!enforce_write_target_allowed(*store, dsl_path, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::error_code ec{};
  if (std::filesystem::exists(dsl_path, ec)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = "temp file already exists: " + dsl_path.string();
    }
    return false;
  }

  std::string validation_family{};
  std::filesystem::path grammar_path{};
  if (!validate_plaintext_surface_replacement(*store, dsl_path, true, content,
                                              &validation_family, &grammar_path,
                                              &err)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (!write_text_file_atomic(dsl_path.string(), content, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::string after_sha256{};
  if (!sha256_hex_file(dsl_path, &after_sha256, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"created\":true,\"path\":" << json_quote(dsl_path.string())
        << ",\"bytes\":" << content.size()
        << ",\"sha256\":" << json_quote(after_sha256)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"grammar_path\":" << json_quote(grammar_path.string()) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_temp_replace(std::string_view tool_name,
                                            const std::string &request_json,
                                            hero_config_store_t *store,
                                            std::string *out_result_json,
                                            int *out_error_code,
                                            std::string *out_error_message) {
  std::string dsl_path_raw{};
  std::string content{};
  std::string expected_sha256{};
  if (!extract_json_string_field(request_json, "path", &dsl_path_raw) ||
      dsl_path_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }
  if (!extract_json_string_field(request_json, "content", &content)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          std::string(tool_name) + " requires argument content";
    }
    return false;
  }
  (void)extract_json_string_field(request_json, "expected_sha256",
                                  &expected_sha256);

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_temp_dsl_path_with_scope(*store, dsl_path_raw, &dsl_path, &err,
                                        &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!enforce_write_target_allowed(*store, dsl_path, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::error_code ec{};
  const bool existed = std::filesystem::exists(dsl_path, ec) &&
                       std::filesystem::is_regular_file(dsl_path, ec);
  if (!existed) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = "temp file does not exist: " + dsl_path.string();
    }
    return false;
  }
  std::string before_sha256{};
  if (!sha256_hex_file(dsl_path, &before_sha256, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  expected_sha256 = lowercase_copy(trim_ascii(expected_sha256));
  if (!expected_sha256.empty() &&
      lowercase_copy(before_sha256) != expected_sha256) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          "expected_sha256 does not match current temp content: " +
          dsl_path.string();
    }
    return false;
  }

  std::string validation_family{};
  std::filesystem::path grammar_path{};
  if (!validate_plaintext_surface_replacement(*store, dsl_path, true, content,
                                              &validation_family, &grammar_path,
                                              &err)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (!write_text_file_atomic(dsl_path.string(), content, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::string after_sha256{};
  if (!sha256_hex_file(dsl_path, &after_sha256, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"replaced\":true,\"path\":" << json_quote(dsl_path.string())
        << ",\"bytes\":" << content.size()
        << ",\"before_sha256\":" << json_quote(before_sha256)
        << ",\"sha256\":" << json_quote(after_sha256)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"grammar_path\":" << json_quote(grammar_path.string()) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_temp_delete(std::string_view tool_name,
                                           const std::string &request_json,
                                           hero_config_store_t *store,
                                           std::string *out_result_json,
                                           int *out_error_code,
                                           std::string *out_error_message) {
  std::string dsl_path_raw{};
  std::string expected_sha256{};
  if (!extract_json_string_field(request_json, "path", &dsl_path_raw) ||
      dsl_path_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }
  (void)extract_json_string_field(request_json, "expected_sha256",
                                  &expected_sha256);

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_temp_dsl_path_with_scope(*store, dsl_path_raw, &dsl_path, &err,
                                        &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!enforce_write_target_allowed(*store, dsl_path, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::string before_sha256{};
  if (!sha256_hex_file(dsl_path, &before_sha256, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  expected_sha256 = lowercase_copy(trim_ascii(expected_sha256));
  if (!expected_sha256.empty() &&
      lowercase_copy(before_sha256) != expected_sha256) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          "expected_sha256 does not match current temp content: " +
          dsl_path.string();
    }
    return false;
  }

  std::error_code ec{};
  if (!std::filesystem::remove(dsl_path, ec) || ec) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message = "failed to delete temp file: " + dsl_path.string();
    }
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"deleted\":true,\"path\":" << json_quote(dsl_path.string())
        << ",\"before_sha256\":" << json_quote(before_sha256) << "}";
    *out_result_json = out.str();
  }
  return true;
}

