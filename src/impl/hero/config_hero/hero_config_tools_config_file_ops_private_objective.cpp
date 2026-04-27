[[nodiscard]] bool handle_tool_objective_read(std::string_view tool_name,
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
  std::string objective_root_raw{};
  std::string path_raw{};
  if (!extract_objective_request_paths(tool_name, request_json,
                                       &objective_root_raw, &path_raw,
                                       out_error_code, out_error_message)) {
    return false;
  }

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_objective_path_with_scope(*store, objective_root_raw, path_raw,
                                         /*allow_missing_target=*/false,
                                         &dsl_path, &err, &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
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
    out << "{\"objective_root\":" << json_quote(objective_root_raw)
        << ",\"path\":" << json_quote(dsl_path.string())
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

[[nodiscard]] bool handle_tool_objective_list(std::string_view tool_name,
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
  std::string objective_root_raw{};
  if (!extract_json_string_field(request_json, "objective_root",
                                 &objective_root_raw) ||
      objective_root_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          std::string(tool_name) + " requires argument objective_root";
    }
    return false;
  }

  std::filesystem::path objective_root{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_objective_root_with_scope(*store, objective_root_raw,
                                         &objective_root, &err, &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
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
  if (!list_instruction_files_under_root(objective_root, allowed_extensions,
                                         &files, &err)) {
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
    out << "{\"objective_root\":" << json_quote(objective_root_raw)
        << ",\"files\":[";
    for (std::size_t i = 0; i < files.size(); ++i) {
      const auto &path = files[i];
      std::error_code ec{};
      const std::string relative_path =
          std::filesystem::relative(path, objective_root, ec).string();
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
    out << "],\"count\":" << files.size() << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_objective_create(
    std::string_view tool_name, const std::string &request_json,
    hero_config_store_t *store, std::string *out_result_json,
    int *out_error_code, std::string *out_error_message) {
  std::string objective_root_raw{};
  std::string path_raw{};
  std::string content{};
  if (!extract_objective_request_paths(tool_name, request_json,
                                       &objective_root_raw, &path_raw,
                                       out_error_code, out_error_message)) {
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
  if (!resolve_objective_path_with_scope(*store, objective_root_raw, path_raw,
                                         /*allow_missing_target=*/true,
                                         &dsl_path, &err, &dsl_err)) {
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
      *out_error_message =
          "objective file already exists: " + dsl_path.string();
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
    out << "{\"created\":true"
        << ",\"objective_root\":" << json_quote(objective_root_raw)
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"bytes\":" << content.size()
        << ",\"sha256\":" << json_quote(after_sha256)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"grammar_path\":" << json_quote(grammar_path.string()) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_objective_replace(
    std::string_view tool_name, const std::string &request_json,
    hero_config_store_t *store, std::string *out_result_json,
    int *out_error_code, std::string *out_error_message) {
  std::string objective_root_raw{};
  std::string path_raw{};
  std::string content{};
  std::string expected_sha256{};
  if (!extract_objective_request_paths(tool_name, request_json,
                                       &objective_root_raw, &path_raw,
                                       out_error_code, out_error_message)) {
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
  if (!resolve_objective_path_with_scope(*store, objective_root_raw, path_raw,
                                         /*allow_missing_target=*/true,
                                         &dsl_path, &err, &dsl_err)) {
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
      *out_error_message =
          "objective file does not exist: " + dsl_path.string();
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
          "expected_sha256 does not match current objective content: " +
          dsl_path.string();
    }
    return false;
  }

  std::string validation_family{};
  std::filesystem::path grammar_path{};
  if (!validate_instruction_dsl_replacement(
          *store, dsl_path, content, &validation_family, &grammar_path, &err)) {
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
    out << "{\"replaced\":true,\"created\":" << bool_json(!existed)
        << ",\"objective_root\":" << json_quote(objective_root_raw)
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"bytes\":" << content.size()
        << ",\"before_sha256\":" << json_quote(before_sha256)
        << ",\"sha256\":" << json_quote(after_sha256)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"grammar_path\":" << json_quote(grammar_path.string()) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_objective_delete(
    std::string_view tool_name, const std::string &request_json,
    hero_config_store_t *store, std::string *out_result_json,
    int *out_error_code, std::string *out_error_message) {
  std::string objective_root_raw{};
  std::string path_raw{};
  std::string expected_sha256{};
  if (!extract_objective_request_paths(tool_name, request_json,
                                       &objective_root_raw, &path_raw,
                                       out_error_code, out_error_message)) {
    return false;
  }
  (void)extract_json_string_field(request_json, "expected_sha256",
                                  &expected_sha256);

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_objective_path_with_scope(*store, objective_root_raw, path_raw,
                                         /*allow_missing_target=*/false,
                                         &dsl_path, &err, &dsl_err)) {
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
          "expected_sha256 does not match current objective content: " +
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
          "failed to delete objective file: " + dsl_path.string();
    }
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"deleted\":true"
        << ",\"objective_root\":" << json_quote(objective_root_raw)
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"before_sha256\":" << json_quote(before_sha256) << "}";
    *out_result_json = out.str();
  }
  return true;
}

} // namespace cuwacunu::hero::mcp::detail
