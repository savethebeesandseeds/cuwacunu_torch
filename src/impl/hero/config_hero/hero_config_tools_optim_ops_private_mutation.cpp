[[nodiscard]] bool handle_tool_optim_create(std::string_view tool_name,
                                            const std::string &request_json,
                                            hero_config_store_t *store,
                                            std::string *out_result_json,
                                            int *out_error_code,
                                            std::string *out_error_message) {
  std::string path_raw{};
  std::string content{};
  if (!extract_json_string_field(request_json, "path", &path_raw) ||
      path_raw.empty()) {
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

  tsodao_surface_t surface{};
  std::string err{};
  if (!resolve_tsodao_surface(*store, &surface, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::filesystem::path dsl_path{};
  if (!resolve_optim_path_with_scope(*store, surface, path_raw,
                                     /*allow_missing_target=*/true, &dsl_path,
                                     &err)) {
    if (out_error_code)
      *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!enforce_optim_write_target_allowed(*store, surface, dsl_path, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  bool payload_present = tsodao_hidden_payload_present(surface);
  const bool archive_present = std::filesystem::exists(surface.hidden_archive);
  if (!payload_present && archive_present &&
      !restore_optim_surface_from_archive(*store, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  payload_present = tsodao_hidden_payload_present(surface);

  std::error_code ec{};
  if (std::filesystem::exists(dsl_path, ec)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = "optim file already exists: " + dsl_path.string();
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

  const bool had_preexisting_surface = payload_present || archive_present;
  std::string checkpoint_backup{};
  if (!checkpoint_optim_surface_before_mutation(*store, surface,
                                                &checkpoint_backup, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
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

  if (!sync_optim_surface_to_archive(*store, &err)) {
    std::string rollback_error{};
    (void)rollback_failed_optim_mutation(
        *store, surface, dsl_path, had_preexisting_surface, &rollback_error);
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message = "failed to sync optim surface after create: " + err;
      if (!rollback_error.empty()) {
        *out_error_message += "; rollback failed: " + rollback_error;
      }
    }
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
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"archive_path\":" << json_quote(surface.hidden_archive.string())
        << ",\"bytes\":" << content.size()
        << ",\"sha256\":" << json_quote(after_sha256)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"grammar_path\":" << json_quote(grammar_path.string())
        << ",\"checkpoint_backup\":" << json_quote(checkpoint_backup) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_optim_replace(std::string_view tool_name,
                                             const std::string &request_json,
                                             hero_config_store_t *store,
                                             std::string *out_result_json,
                                             int *out_error_code,
                                             std::string *out_error_message) {
  std::string path_raw{};
  std::string content{};
  std::string expected_sha256{};
  if (!extract_json_string_field(request_json, "path", &path_raw) ||
      path_raw.empty()) {
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

  tsodao_surface_t surface{};
  std::string err{};
  if (!resolve_tsodao_surface(*store, &surface, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::filesystem::path dsl_path{};
  if (!resolve_optim_path_with_scope(*store, surface, path_raw,
                                     /*allow_missing_target=*/true, &dsl_path,
                                     &err)) {
    if (out_error_code)
      *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!enforce_optim_write_target_allowed(*store, surface, dsl_path, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  bool payload_present = tsodao_hidden_payload_present(surface);
  const bool archive_present = std::filesystem::exists(surface.hidden_archive);
  if (!payload_present && archive_present &&
      !restore_optim_surface_from_archive(*store, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  payload_present = tsodao_hidden_payload_present(surface);

  std::error_code ec{};
  const bool existed = std::filesystem::exists(dsl_path, ec) &&
                       std::filesystem::is_regular_file(dsl_path, ec);
  if (!existed) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = "optim file does not exist: " + dsl_path.string();
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
          "expected_sha256 does not match current optim content: " +
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

  const bool had_preexisting_surface = payload_present || archive_present;
  std::string checkpoint_backup{};
  if (!checkpoint_optim_surface_before_mutation(*store, surface,
                                                &checkpoint_backup, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
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

  if (!sync_optim_surface_to_archive(*store, &err)) {
    std::string rollback_error{};
    (void)rollback_failed_optim_mutation(
        *store, surface, dsl_path, had_preexisting_surface, &rollback_error);
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message = "failed to sync optim surface after replace: " + err;
      if (!rollback_error.empty()) {
        *out_error_message += "; rollback failed: " + rollback_error;
      }
    }
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
    out << "{\"replaced\":true"
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"archive_path\":" << json_quote(surface.hidden_archive.string())
        << ",\"bytes\":" << content.size()
        << ",\"before_sha256\":" << json_quote(before_sha256)
        << ",\"sha256\":" << json_quote(after_sha256)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"grammar_path\":" << json_quote(grammar_path.string())
        << ",\"checkpoint_backup\":" << json_quote(checkpoint_backup) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_optim_delete(std::string_view tool_name,
                                            const std::string &request_json,
                                            hero_config_store_t *store,
                                            std::string *out_result_json,
                                            int *out_error_code,
                                            std::string *out_error_message) {
  std::string path_raw{};
  std::string expected_sha256{};
  if (!extract_json_string_field(request_json, "path", &path_raw) ||
      path_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }
  (void)extract_json_string_field(request_json, "expected_sha256",
                                  &expected_sha256);

  tsodao_surface_t surface{};
  std::string err{};
  if (!resolve_tsodao_surface(*store, &surface, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::filesystem::path dsl_path{};
  if (!resolve_optim_path_with_scope(*store, surface, path_raw,
                                     /*allow_missing_target=*/true, &dsl_path,
                                     &err)) {
    if (out_error_code)
      *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!enforce_optim_write_target_allowed(*store, surface, dsl_path, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  bool payload_present = tsodao_hidden_payload_present(surface);
  const bool archive_present = std::filesystem::exists(surface.hidden_archive);
  if (!payload_present && archive_present &&
      !restore_optim_surface_from_archive(*store, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  payload_present = tsodao_hidden_payload_present(surface);

  std::error_code ec{};
  const bool existed = std::filesystem::exists(dsl_path, ec) &&
                       std::filesystem::is_regular_file(dsl_path, ec);
  if (!existed) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = "optim file does not exist: " + dsl_path.string();
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
          "expected_sha256 does not match current optim content: " +
          dsl_path.string();
    }
    return false;
  }

  const bool had_preexisting_surface = payload_present || archive_present;
  std::string checkpoint_backup{};
  if (!checkpoint_optim_surface_before_mutation(*store, surface,
                                                &checkpoint_backup, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (!std::filesystem::remove(dsl_path, ec) || ec) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message = "failed to delete optim file: " + dsl_path.string();
    }
    return false;
  }

  if (!sync_optim_surface_to_archive(*store, &err)) {
    std::string rollback_error{};
    (void)rollback_failed_optim_mutation(
        *store, surface, dsl_path, had_preexisting_surface, &rollback_error);
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message = "failed to sync optim surface after delete: " + err;
      if (!rollback_error.empty()) {
        *out_error_message += "; rollback failed: " + rollback_error;
      }
    }
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"deleted\":true"
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"archive_path\":" << json_quote(surface.hidden_archive.string())
        << ",\"before_sha256\":" << json_quote(before_sha256)
        << ",\"checkpoint_backup\":" << json_quote(checkpoint_backup) << "}";
    *out_result_json = out.str();
  }
  return true;
}
