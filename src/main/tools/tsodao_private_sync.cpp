void git_pre_commit(const tsodao_config_t& cfg) {
  require_command("git");
  auto staged_plaintext = list_staged_hidden_plaintext_paths(cfg);
  if (!staged_plaintext.empty()) {
    std::ostringstream oss;
    oss << "staged hidden plaintext files detected:\n";
    for (const std::string& path : staged_plaintext) oss << path << "\n";
    oss << "Only " << cfg.hidden_archive_rel << " and " << cfg.public_keep_rel
        << " may be committed.";
    die(oss.str());
  }

  if (has_hidden_payload(cfg)) {
    if (!fs::exists(cfg.hidden_archive)) {
      if (cfg.visibility_mode == "recipient") {
        if (cfg.gpg_recipient.empty()) {
          die("tsodao.dsl gpg_recipient is empty. Run tsodao init or seal manually.");
        }
        note("pre-commit: creating " + cfg.hidden_archive_rel +
             " from TSODAO hidden root");
        seal_hidden_surface(cfg, {"--recipient", cfg.gpg_recipient});
      } else if (cfg.visibility_mode == "symmetric") {
        die("tsodao.dsl visibility_mode=symmetric cannot auto-refresh in pre-commit\n"
            "  run tsodao sync --from-plaintext --symmetric before commit");
      } else if (cfg.visibility_mode.empty() || cfg.visibility_mode == "disabled" ||
                 cfg.visibility_mode == "manual" || cfg.visibility_mode == "off" ||
                 cfg.visibility_mode == "none") {
        die("hidden plaintext payload is present but TSODAO automatic recipient sealing is not configured. Set visibility_mode=recipient and gpg_recipient in tsodao.dsl, or run tsodao sync manually before commit.");
      } else {
        die("unknown tsodao.dsl visibility_mode value: " + cfg.visibility_mode);
      }
    } else if (needs_archive_refresh(cfg)) {
      if (!known_archive_matches_local_state(cfg)) {
        die("pre-commit refused to overwrite " + cfg.hidden_archive_rel +
            "\n  the current archive does not match this machine's last synced archive\n" +
            sync_choice_guidance(false));
      }
      if (cfg.visibility_mode == "recipient") {
        if (cfg.gpg_recipient.empty()) {
          die("tsodao.dsl gpg_recipient is empty. Run tsodao init or seal manually.");
        }
        note("pre-commit: refreshing " + cfg.hidden_archive_rel +
             " from TSODAO hidden root");
        seal_hidden_surface(cfg, {"--recipient", cfg.gpg_recipient});
      } else if (cfg.visibility_mode == "symmetric") {
        die("tsodao.dsl visibility_mode=symmetric cannot auto-refresh in pre-commit\n"
            "  run tsodao sync --from-plaintext --symmetric before commit");
      } else if (cfg.visibility_mode.empty() || cfg.visibility_mode == "disabled" ||
                 cfg.visibility_mode == "manual" || cfg.visibility_mode == "off" ||
                 cfg.visibility_mode == "none") {
        die("hidden plaintext payload is present but TSODAO automatic recipient sealing is not configured. Set visibility_mode=recipient and gpg_recipient in tsodao.dsl, or run tsodao sync manually before commit.");
      } else {
        die("unknown tsodao.dsl visibility_mode value: " + cfg.visibility_mode);
      }
    } else if (!known_archive_matches_local_state(cfg)) {
      die("pre-commit found both plaintext and archive, but the archive does not match this machine's last synced archive\n" +
          sync_choice_guidance(false));
    }
    git_stage_hidden_surface(cfg);
  }

  staged_plaintext = list_staged_hidden_plaintext_paths(cfg);
  if (!staged_plaintext.empty()) {
    std::ostringstream oss;
    oss << "staged hidden plaintext files detected:\n";
    for (const std::string& path : staged_plaintext) oss << path << "\n";
    oss << "Only " << cfg.hidden_archive_rel << " and " << cfg.public_keep_rel
        << " may be committed.";
    die(oss.str());
  }
}

void git_pre_push(const tsodao_config_t& cfg) {
  require_command("git");
  const run_result_t empty_tree_result = run_command_capture(
      {"git", "-C", cfg.repo_root.string(), "hash-object", "-t", "tree", "/dev/null"},
      true);
  if (empty_tree_result.exit_code != 0) die("git hash-object failed");
  const std::string empty_tree =
      split_lines(empty_tree_result.output).empty() ? ""
                                                    : split_lines(empty_tree_result.output).front();
  std::set<std::string> violations;
  bool had_input = false;
  std::string local_ref;
  std::string local_sha;
  std::string remote_ref;
  std::string remote_sha;
  while (std::cin >> local_ref >> local_sha >> remote_ref >> remote_sha) {
    had_input = true;
    if (local_sha == "0000000000000000000000000000000000000000") continue;
    std::string base_sha = remote_sha;
    if (base_sha.empty() || base_sha == "0000000000000000000000000000000000000000") {
      base_sha = empty_tree;
    }
    const run_result_t diff_result = run_command_capture(
        {"git", "-C", cfg.repo_root.string(), "diff", "--name-only",
         "--diff-filter=ACMR", base_sha, local_sha, "--", cfg.hidden_root_rel},
        true);
    if (diff_result.exit_code != 0) die("git diff failed while checking push");
    for (const std::string& path : split_lines(diff_result.output)) {
      if (path != cfg.public_keep_rel) violations.insert(path);
    }
  }

  if (!had_input) return;
  if (!violations.empty()) {
    std::ostringstream oss;
    oss << "push would publish TSODAO hidden plaintext files:\n";
    for (const std::string& path : violations) oss << path << "\n";
    oss << "Remove those paths from the pushed commits before publishing.";
    die(oss.str());
  }
}

void open_hidden_surface(const tsodao_config_t& cfg) {
  require_command("gpg");
  require_command("tar");
  setup_gpg_tty_if_needed();
  ensure_hidden_archive_exists(cfg);
  ensure_open_target_is_clean(cfg);

  const fs::path work_dir = create_temp_dir();
  const fs::path tar_path = work_dir / "hidden_surface.tar";
  try {
    run_command_or_die({"gpg", "--yes", "--output", tar_path.string(), "--decrypt",
                        cfg.hidden_archive.string()},
                       "gpg decrypt");
    run_command_or_die({"tar", "-xf", tar_path.string(), "-C", cfg.hidden_root.string()},
                       "tar extract");
    const auto archive_sha = archive_sha256(cfg);
    if (!archive_sha) die("failed to record archive sha256");
    remember_archive_state(cfg, *archive_sha, "open");
    note("restore complete:\n"
         "  source archive = " + display_path(cfg, cfg.hidden_archive) + "\n"
         "  archive sha256 = " + *archive_sha + "\n"
         "  destination surface = " + display_path(cfg, cfg.hidden_root));
  } catch (...) {
    cleanup_temp_dir(work_dir);
    throw;
  }
  cleanup_temp_dir(work_dir);
}

void scrub_command(const tsodao_config_t& cfg, const std::vector<std::string>& args) {
  bool assume_yes = false;
  for (const std::string& arg : args) {
    if (arg == "--yes") {
      assume_yes = true;
    } else {
      die("unknown scrub option: " + arg);
    }
  }

  ensure_hidden_root_dir(cfg);
  if (!has_hidden_payload(cfg)) {
    note("nothing to scrub; only the public keep path or an empty directory is present");
    return;
  }

  if (!assume_yes) {
    std::ostringstream oss;
    oss << "confirm scrub:\n"
        << "  surface = " << display_path(cfg, cfg.hidden_root) << "\n"
        << "  public marker kept = " << display_path(cfg, cfg.public_keep_path)
        << "\n"
        << "  action = remove local plaintext payload\n"
        << "continue? [y/N] ";
    prompt(oss.str());
    std::string answer;
    if (!std::getline(std::cin, answer)) die("scrub cancelled");
    answer = trim_copy(answer);
    if (!(answer == "y" || answer == "Y")) die("scrub cancelled");
  }

  scrub_hidden_payload(cfg);
  note("scrub complete:\n"
       "  removed plaintext payload from " + display_path(cfg, cfg.hidden_root) +
       "\n"
       "  kept public marker = " + display_path(cfg, cfg.public_keep_path));
}

void status_command(const tsodao_config_t& cfg) {
  std::error_code ec;
  const bool hidden_root_is_dir = fs::is_directory(cfg.hidden_root, ec);
  const bool plaintext_present = hidden_root_is_dir && has_hidden_payload(cfg);
  const bool archive_present = fs::exists(cfg.hidden_archive, ec);
  const bool state_present = fs::exists(cfg.local_state_path, ec);
  const auto current_archive_sha = archive_sha256(cfg);
  const auto last_archive_sha =
      read_state_value(cfg.local_state_path, "last_archive_sha256");
  const auto last_sync_action =
      read_state_value(cfg.local_state_path, "last_sync_action");
  const auto last_surface_listing_sha = read_state_value(
      cfg.local_state_path, "last_hidden_surface_listing_sha256");
  const std::string current_surface_listing_sha =
      hidden_root_is_dir ? hidden_surface_listing_sha256(cfg) : std::string();
  const sync_plan_t plan = infer_default_sync_plan(cfg);
  std::string archive_memory_status = "unknown";
  if (!archive_present) {
    archive_memory_status = "archive absent";
  } else if (!last_archive_sha || trim_copy(*last_archive_sha).empty()) {
    archive_memory_status = "local sync memory absent";
  } else if (current_archive_sha &&
             trim_copy(*current_archive_sha) == trim_copy(*last_archive_sha)) {
    archive_memory_status = "matches this machine's last synced archive";
  } else {
    archive_memory_status = "does not match this machine's last synced archive";
  }

  emit_operator_context(cfg);

  std::ostringstream oss;
  oss << "status:\n"
      << "  plaintext payload = ";
  if (!hidden_root_is_dir) {
    oss << "hidden-root directory missing";
  } else {
    oss << (plaintext_present ? "present" : "absent or public-only");
  }
  oss << "\n"
      << "  archive file = " << (archive_present ? "present" : "absent") << "\n"
      << "  archive sha256 = "
      << (current_archive_sha ? *current_archive_sha : std::string("absent")) << "\n"
      << "  local sync memory = " << (state_present ? "present" : "absent") << "\n"
      << "  last synced archive sha256 = "
      << (last_archive_sha ? trim_copy(*last_archive_sha) : std::string("unknown"))
      << "\n"
      << "  last sync action = "
      << (last_sync_action && !trim_copy(*last_sync_action).empty()
              ? trim_copy(*last_sync_action)
              : std::string("unknown"))
      << "\n"
      << "  current surface listing sha256 = "
      << (hidden_root_is_dir ? current_surface_listing_sha : std::string("unavailable"))
      << "\n"
      << "  last synced surface listing sha256 = "
      << (last_surface_listing_sha ? trim_copy(*last_surface_listing_sha)
                                   : std::string("unknown"))
      << "\n"
      << "  archive matches local sync memory = ";
  if (archive_memory_status == "matches this machine's last synced archive") {
    oss << "yes";
  } else if (archive_memory_status == "does not match this machine's last synced archive") {
    oss << "no";
  } else {
    oss << archive_memory_status;
  }
  oss << "\n"
      << "  sync state = ";

  switch (plan) {
    case sync_plan_t::nothing:
      oss << "nothing to sync";
      break;
    case sync_plan_t::no_action:
      oss << "already safe and current for this machine";
      break;
    case sync_plan_t::plaintext_to_archive:
      oss << "plaintext should reseal the archive";
      break;
    case sync_plan_t::archive_to_plaintext:
      oss << "archive should restore the hidden root";
      break;
    case sync_plan_t::ambiguous:
      oss << "ambiguous";
      break;
  }
  note(oss.str());

  if (plan == sync_plan_t::plaintext_to_archive) {
    note("suggested command:\n"
         "  tsodao sync\n"
         "  tsodao sync --from-plaintext");
  } else if (plan == sync_plan_t::archive_to_plaintext) {
    note("suggested command:\n"
         "  tsodao sync\n"
         "  tsodao sync --from-archive");
  } else if (plan == sync_plan_t::ambiguous) {
    note(sync_choice_guidance(true));
  }
}

sync_plan_t infer_default_sync_plan(const tsodao_config_t& cfg) {
  const bool plaintext_present = has_hidden_payload(cfg);
  const bool archive_present = fs::exists(cfg.hidden_archive);
  if (!plaintext_present && !archive_present) return sync_plan_t::nothing;
  if (plaintext_present && !archive_present) return sync_plan_t::plaintext_to_archive;
  if (!plaintext_present && archive_present) return sync_plan_t::archive_to_plaintext;
  if (known_archive_matches_local_state(cfg)) {
    return needs_archive_refresh(cfg) ? sync_plan_t::plaintext_to_archive
                                      : sync_plan_t::no_action;
  }
  return sync_plan_t::ambiguous;
}

void sync_command(const tsodao_config_t& cfg, const std::vector<std::string>& args) {
  bool from_plaintext = false;
  bool from_archive = false;
  bool assume_yes = false;
  std::vector<std::string> forwarded;

  for (std::size_t i = 0; i < args.size(); ++i) {
    const std::string& arg = args[i];
    if (arg == "--from-plaintext" || arg == "--to-archive" ||
        arg == "--prefer-hidden") {
      from_plaintext = true;
    } else if (arg == "--from-archive" || arg == "--to-hidden" ||
               arg == "--prefer-archive") {
      from_archive = true;
    } else if (arg == "--yes") {
      assume_yes = true;
    } else {
      forwarded.push_back(arg);
    }
  }

  emit_operator_context(cfg);

  if (from_plaintext && from_archive) {
    die("choose one sync source: --from-plaintext or --from-archive");
  }

  const bool plaintext_present = has_hidden_payload(cfg);
  const bool archive_present = fs::exists(cfg.hidden_archive);
  if (!plaintext_present && !archive_present) {
    die("nothing to sync: both hidden plaintext and hidden archive are absent");
  }
  if (plaintext_present && !archive_present) {
    if (from_archive) die("cannot sync --from-archive: hidden archive is absent");
    note("sync source = plaintext\nreason = archive missing");
    seal_hidden_surface(cfg, forwarded);
    maybe_git_stage_hidden_surface(cfg);
    return;
  }
  if (!plaintext_present && archive_present) {
    if (from_plaintext) {
      die("cannot sync --from-plaintext: hidden plaintext is absent");
    }
    note("sync source = archive\nreason = plaintext absent");
    open_hidden_surface(cfg);
    return;
  }

  if (from_plaintext) {
    if (known_archive_matches_local_state(cfg) && !needs_archive_refresh(cfg)) {
      note("sync: archive already matches the hidden plaintext surface; no action taken");
    } else {
      note("sync source = plaintext\nreason = explicit operator choice");
      seal_hidden_surface(cfg, forwarded);
      maybe_git_stage_hidden_surface(cfg);
    }
    return;
  }

  if (from_archive) {
    if (known_archive_matches_local_state(cfg) && !needs_archive_refresh(cfg)) {
      note("sync: hidden plaintext already matches the encrypted archive; no action taken");
    } else {
      note("sync source = archive\nreason = explicit operator choice");
      confirm_restore_over_hidden_payload(cfg, assume_yes);
      open_hidden_surface(cfg);
    }
    return;
  }

  if (known_archive_matches_local_state(cfg)) {
    if (needs_archive_refresh(cfg)) {
      note("sync source = plaintext\nreason = archive matches local memory but plaintext changed");
      seal_hidden_surface(cfg, forwarded);
      maybe_git_stage_hidden_surface(cfg);
    } else {
      note("sync: already safe and current for this machine; no action taken");
    }
    return;
  }

  const auto current_archive_sha = archive_sha256(cfg);
  const auto last_archive_sha =
      read_state_value(cfg.local_state_path, "last_archive_sha256");
  const auto last_sync_action =
      read_state_value(cfg.local_state_path, "last_sync_action");
  std::ostringstream oss;
  oss << "sync is ambiguous for this machine\n"
      << "  both the plaintext surface and encrypted archive exist\n"
      << "  current archive sha256 = "
      << (current_archive_sha ? *current_archive_sha : std::string("unavailable"))
      << "\n"
      << "  last synced archive sha256 = "
      << (last_archive_sha ? trim_copy(*last_archive_sha) : std::string("unknown"))
      << "\n"
      << "  last sync action = "
      << (last_sync_action && !trim_copy(*last_sync_action).empty()
              ? trim_copy(*last_sync_action)
              : std::string("unknown"))
      << "\n"
      << "  refusing to overwrite either side without an explicit source\n"
      << sync_choice_guidance(true);
  die(oss.str());
}

void validate_configured_recipient(const tsodao_config_t& cfg) {
  require_command("gpg");
  if (cfg.visibility_mode.empty()) {
    die("tsodao.dsl visibility_mode is not set in " + cfg.tsodao_dsl_path.string());
  }
  if (cfg.visibility_mode != "recipient") {
    die("tsodao.dsl visibility_mode must be recipient, got: " + cfg.visibility_mode);
  }
  if (cfg.gpg_recipient.empty()) {
    die("tsodao.dsl gpg_recipient is empty in " + cfg.tsodao_dsl_path.string());
  }
  const run_result_t result =
      run_command_capture({"gpg", "--list-public-keys", cfg.gpg_recipient}, true);
  if (result.exit_code != 0) {
    die("configured GPG recipient not found in local keyring: " + cfg.gpg_recipient);
  }
  note("validated configured TSODAO recipient\n"
       "  recipient = " + recipient_summary(cfg.gpg_recipient));
}

