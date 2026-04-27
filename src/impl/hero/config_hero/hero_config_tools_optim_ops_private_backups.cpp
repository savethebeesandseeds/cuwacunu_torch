[[nodiscard]] bool handle_tool_optim_backups(std::string_view tool_name,
                                             const std::string &request_json,
                                             hero_config_store_t *store,
                                             std::string *out_result_json,
                                             int *out_error_code,
                                             std::string *out_error_message) {
  (void)tool_name;
  (void)request_json;

  tsodao_surface_t surface{};
  std::string err{};
  if (!resolve_tsodao_surface(*store, &surface, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  bool enabled = true;
  std::filesystem::path backup_dir{};
  std::int64_t max_entries = 20;
  if (!resolve_optim_backup_policy(*store, &enabled, &backup_dir, &max_entries,
                                   &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::vector<optim_backup_entry_t> entries{};
  if (!list_optim_backup_entries(*store, &entries, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"backup_enabled\":" << bool_json(enabled)
        << ",\"backup_dir\":" << json_quote(backup_dir.string())
        << ",\"backup_max_entries\":" << max_entries
        << ",\"archive_path\":" << json_quote(surface.hidden_archive.string())
        << ",\"count\":" << entries.size() << ",\"backups\":[";
    for (std::size_t i = 0; i < entries.size(); ++i) {
      if (i != 0)
        out << ",";
      out << "{\"index\":" << i
          << ",\"filename\":" << json_quote(entries[i].path.filename().string())
          << ",\"path\":" << json_quote(entries[i].path.string()) << "}";
    }
    out << "]}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_optim_rollback(std::string_view tool_name,
                                              const std::string &request_json,
                                              hero_config_store_t *store,
                                              std::string *out_result_json,
                                              int *out_error_code,
                                              std::string *out_error_message) {
  std::string selector{};
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

  tsodao_surface_t surface{};
  std::string err{};
  if (!resolve_tsodao_surface(*store, &surface, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  const std::filesystem::path policy_probe =
      (surface.hidden_root / ".tsodao.optim.rollback.probe.dsl")
          .lexically_normal();
  if (!enforce_optim_write_target_allowed(*store, surface, policy_probe,
                                          &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::string selected_backup{};
  std::string checkpoint_backup{};
  if (!restore_optim_backup_into_active_surface(*store, surface, selector,
                                                &selected_backup,
                                                &checkpoint_backup, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"rolled_back\":true"
        << ",\"optim_root\":" << json_quote(surface.hidden_root.string())
        << ",\"archive_path\":" << json_quote(surface.hidden_archive.string())
        << ",\"selected_backup\":" << json_quote(selected_backup)
        << ",\"checkpoint_backup\":" << json_quote(checkpoint_backup) << "}";
    *out_result_json = out.str();
  }
  return true;
}

} // namespace cuwacunu::hero::mcp::detail
