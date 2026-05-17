[[nodiscard]] bool resolve_optim_path_with_scope(
    const hero_config_store_t &store, const tsodao_surface_t &surface,
    std::string_view raw_path, bool allow_missing_target,
    std::filesystem::path *out_path, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!out_path) {
    if (out_error)
      *out_error = "missing destination for optim path";
    return false;
  }
  out_path->clear();

  const std::string trimmed_path = trim_ascii(raw_path);
  if (trimmed_path.empty()) {
    if (out_error)
      *out_error = "optim path is empty";
    return false;
  }

  const std::filesystem::path rel_path(trimmed_path);
  if (rel_path.is_absolute()) {
    if (out_error) {
      *out_error = std::string(kConfigDslScopeErrorTag) +
                   ": optim path must be relative to TSODAO hidden_root: " +
                   trimmed_path;
    }
    return false;
  }

  const std::filesystem::path resolved =
      (surface.hidden_root / rel_path).lexically_normal();
  if (!path_is_within(surface.hidden_root, resolved)) {
    if (out_error) {
      *out_error =
          std::string(kConfigDslScopeErrorTag) +
          ": optim path escapes TSODAO hidden_root: " + resolved.string();
    }
    return false;
  }

  std::vector<std::string> allowed_extensions{};
  std::string extensions_error{};
  if (!collect_allowed_extensions(store, &allowed_extensions,
                                  &extensions_error)) {
    if (out_error)
      *out_error = extensions_error;
    return false;
  }
  if (!path_has_allowed_extension(resolved, allowed_extensions)) {
    if (out_error) {
      *out_error =
          std::string(kConfigDslScopeErrorTag) +
          ": optim path must use an allowed extension: " + resolved.string();
    }
    return false;
  }

  if (!allow_missing_target) {
    std::error_code ec{};
    if (!std::filesystem::exists(resolved, ec) ||
        !std::filesystem::is_regular_file(resolved, ec)) {
      if (out_error) {
        *out_error = "optim file does not exist: " + resolved.string();
      }
      return false;
    }
  }

  *out_path = resolved;
  return true;
}

[[nodiscard]] bool enforce_optim_write_target_allowed(
    const hero_config_store_t &store, const tsodao_surface_t &surface,
    const std::filesystem::path &target, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!path_is_within(surface.hidden_root, target)) {
    if (out_error) {
      *out_error = make_write_policy_error(
          "optim target escapes TSODAO hidden_root: " + target.string());
    }
    return false;
  }

  std::vector<std::filesystem::path> allowed_roots{};
  std::string err{};
  if (!collect_allowed_write_roots(store, &allowed_roots, &err)) {
    if (out_error)
      *out_error = err;
    return false;
  }
  for (const auto &root : allowed_roots) {
    if (path_is_within(root, target))
      return true;
  }
  if (out_error) {
    *out_error = make_write_policy_error("optim target escapes write_roots: " +
                                         target.string());
  }
  return false;
}

[[nodiscard]] bool
resolve_optim_backup_policy(const hero_config_store_t &store, bool *out_enabled,
                            std::filesystem::path *out_backup_dir,
                            std::int64_t *out_max_entries,
                            std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (out_enabled)
    *out_enabled = true;
  if (out_backup_dir)
    out_backup_dir->clear();
  if (out_max_entries)
    *out_max_entries = 20;

  bool enabled = true;
  std::string err{};
  if (!read_bool_config_key_or_default(store, "optim_backup_enabled", true,
                                       &enabled, &err)) {
    if (out_error)
      *out_error = err;
    return false;
  }
  std::int64_t max_entries = 20;
  if (!read_int64_config_key_or_default(store, "optim_backup_max_entries", 20,
                                        &max_entries, &err)) {
    if (out_error)
      *out_error = err;
    return false;
  }
  if (max_entries < 1) {
    if (out_error) {
      *out_error =
          make_write_policy_error("optim_backup_max_entries must be >= 1 when "
                                  "optim_backup_enabled=true");
    }
    return false;
  }

  std::string backup_dir_raw =
      trim_ascii(store.get_or_default("optim_backup_dir"));
  if (backup_dir_raw.empty()) {
    backup_dir_raw = "/cuwacunu/.backups/hero.config.optim";
  }
  const std::filesystem::path backup_dir =
      resolve_path_near_config(backup_dir_raw, store.config_path())
          .lexically_normal();
  if (backup_dir.empty()) {
    if (out_error)
      *out_error = "optim backup dir resolved to an empty path";
    return false;
  }

  if (out_enabled)
    *out_enabled = enabled;
  if (out_backup_dir)
    *out_backup_dir = backup_dir;
  if (out_max_entries)
    *out_max_entries = max_entries;
  return true;
}

[[nodiscard]] bool
list_optim_backup_entries(const hero_config_store_t &store,
                          std::vector<optim_backup_entry_t> *out_entries,
                          std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!out_entries) {
    if (out_error)
      *out_error = "missing destination for optim backups";
    return false;
  }
  out_entries->clear();

  bool enabled = true;
  std::filesystem::path backup_dir{};
  std::int64_t max_entries = 20;
  if (!resolve_optim_backup_policy(store, &enabled, &backup_dir, &max_entries,
                                   out_error)) {
    return false;
  }
  (void)max_entries;
  if (!enabled)
    return true;

  std::error_code ec{};
  if (!std::filesystem::exists(backup_dir, ec)) {
    if (ec) {
      if (out_error) {
        *out_error =
            "failed to inspect optim backup directory: " + backup_dir.string();
      }
      return false;
    }
    return true;
  }

  const std::filesystem::directory_iterator begin(backup_dir, ec);
  if (ec) {
    if (out_error) {
      *out_error =
          "failed to enumerate optim backup directory: " + backup_dir.string();
    }
    return false;
  }
  for (std::filesystem::directory_iterator it = begin, end; it != end;
       it.increment(ec)) {
    if (ec) {
      if (out_error) {
        *out_error =
            "failed to iterate optim backup directory: " + backup_dir.string();
      }
      return false;
    }
    if (!it->is_regular_file(ec) || ec)
      continue;
    const auto mtime = it->last_write_time(ec);
    if (ec)
      continue;
    out_entries->push_back({mtime, it->path()});
  }
  std::sort(
      out_entries->begin(), out_entries->end(),
      [](const optim_backup_entry_t &lhs, const optim_backup_entry_t &rhs) {
        if (lhs.mtime != rhs.mtime)
          return lhs.mtime > rhs.mtime;
        return lhs.path.string() < rhs.path.string();
      });
  return true;
}

[[nodiscard]] bool backup_current_optim_archive_with_cap(
    const hero_config_store_t &store, const tsodao_surface_t &surface,
    std::string *out_backup_path, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (out_backup_path)
    out_backup_path->clear();

  bool enabled = true;
  std::filesystem::path backup_dir{};
  std::int64_t max_entries = 20;
  if (!resolve_optim_backup_policy(store, &enabled, &backup_dir, &max_entries,
                                   out_error)) {
    return false;
  }
  if (!enabled)
    return true;

  std::error_code ec{};
  if (!std::filesystem::exists(surface.hidden_archive, ec) ||
      !std::filesystem::is_regular_file(surface.hidden_archive, ec)) {
    return true;
  }

  std::filesystem::create_directories(backup_dir, ec);
  if (ec) {
    if (out_error) {
      *out_error =
          "failed to create optim backup directory: " + backup_dir.string();
    }
    return false;
  }

  const std::string prefix =
      surface.hidden_archive.filename().string() + ".bak.";
  const auto stamp = std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
  std::filesystem::path backup_path =
      backup_dir / (prefix + std::to_string(static_cast<long long>(stamp)));
  int disambiguator = 1;
  while (std::filesystem::exists(backup_path, ec)) {
    if (ec) {
      if (out_error) {
        *out_error =
            "failed to probe optim backup path: " + backup_path.string();
      }
      return false;
    }
    backup_path =
        backup_dir / (prefix + std::to_string(static_cast<long long>(stamp)) +
                      "." + std::to_string(disambiguator++));
  }

  std::filesystem::copy_file(surface.hidden_archive, backup_path,
                             std::filesystem::copy_options::none, ec);
  if (ec) {
    if (out_error) {
      *out_error = "failed to write optim backup file: " + backup_path.string();
    }
    return false;
  }
  if (out_backup_path)
    *out_backup_path = backup_path.string();

  std::vector<optim_backup_entry_t> entries{};
  if (!list_optim_backup_entries(store, &entries, out_error))
    return false;
  const std::size_t keep = static_cast<std::size_t>(max_entries);
  while (entries.size() > keep) {
    const std::filesystem::path doomed = entries.back().path;
    entries.pop_back();
    std::filesystem::remove(doomed, ec);
    if (ec) {
      if (out_error) {
        *out_error = "failed to prune optim backup file: " + doomed.string();
      }
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool select_optim_backup_entry(
    const std::vector<optim_backup_entry_t> &entries, std::string_view selector,
    optim_backup_entry_t *out_selected, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!out_selected) {
    if (out_error)
      *out_error = "missing destination for optim backup";
    return false;
  }
  if (entries.empty()) {
    if (out_error)
      *out_error = "no optim backups available";
    return false;
  }

  const std::string trimmed = trim_ascii(selector);
  if (trimmed.empty()) {
    *out_selected = entries.front();
    return true;
  }
  for (const auto &entry : entries) {
    if (entry.path.filename() == trimmed || entry.path == trimmed) {
      *out_selected = entry;
      return true;
    }
  }
  if (out_error)
    *out_error = "optim backup not found: " + trimmed;
  return false;
}

[[nodiscard]] bool
sync_optim_surface_to_archive(const hero_config_store_t &store,
                              std::string *out_error) {
  std::string output{};
  return run_tsodao_command(store, {"sync", "--from-plaintext"}, &output,
                            out_error);
}

[[nodiscard]] bool
restore_optim_surface_from_archive(const hero_config_store_t &store,
                                   std::string *out_error) {
  std::string output{};
  return run_tsodao_command(store, {"sync", "--from-archive", "--yes"}, &output,
                            out_error);
}

[[nodiscard]] bool checkpoint_optim_surface_before_mutation(
    const hero_config_store_t &store, const tsodao_surface_t &surface,
    std::string *out_backup_path, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (out_backup_path)
    out_backup_path->clear();

  const bool plaintext_present = tsodao_hidden_payload_present(surface);
  const bool archive_present = std::filesystem::exists(surface.hidden_archive);

  if (!plaintext_present && archive_present) {
    if (!restore_optim_surface_from_archive(store, out_error))
      return false;
  }
  if (plaintext_present || archive_present) {
    if (!sync_optim_surface_to_archive(store, out_error))
      return false;
    if (!backup_current_optim_archive_with_cap(store, surface, out_backup_path,
                                               out_error)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool restore_optim_backup_into_active_surface(
    const hero_config_store_t &store, const tsodao_surface_t &surface,
    std::string_view selector, std::string *out_selected_backup,
    std::string *out_checkpoint_backup, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (out_selected_backup)
    out_selected_backup->clear();
  if (out_checkpoint_backup)
    out_checkpoint_backup->clear();

  std::vector<optim_backup_entry_t> entries{};
  if (!list_optim_backup_entries(store, &entries, out_error))
    return false;

  optim_backup_entry_t selected{};
  if (!select_optim_backup_entry(entries, selector, &selected, out_error)) {
    return false;
  }

  std::string checkpoint_backup{};
  if (!checkpoint_optim_surface_before_mutation(
          store, surface, &checkpoint_backup, out_error)) {
    return false;
  }

  std::error_code ec{};
  std::filesystem::create_directories(surface.hidden_archive.parent_path(), ec);
  if (ec) {
    if (out_error) {
      *out_error = "failed to prepare active optim archive path: " +
                   surface.hidden_archive.parent_path().string();
    }
    return false;
  }
  std::filesystem::copy_file(selected.path, surface.hidden_archive,
                             std::filesystem::copy_options::overwrite_existing,
                             ec);
  if (ec) {
    if (out_error) {
      *out_error = "failed to restore active optim archive from backup: " +
                   selected.path.string();
    }
    return false;
  }
  if (!restore_optim_surface_from_archive(store, out_error))
    return false;

  if (out_selected_backup)
    *out_selected_backup = selected.path.string();
  if (out_checkpoint_backup)
    *out_checkpoint_backup = checkpoint_backup;
  return true;
}

[[nodiscard]] bool rollback_failed_optim_mutation(
    const hero_config_store_t &store, const tsodao_surface_t &surface,
    const std::filesystem::path &mutated_path, bool had_preexisting_surface,
    std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (had_preexisting_surface) {
    return restore_optim_surface_from_archive(store, out_error);
  }

  std::error_code ec{};
  if (std::filesystem::exists(mutated_path, ec)) {
    std::filesystem::remove(mutated_path, ec);
    if (ec) {
      if (out_error) {
        *out_error = "failed to remove newly-created optim file after TSODAO "
                     "sync failure: " +
                     mutated_path.string();
      }
      return false;
    }
  }
  return true;
}
