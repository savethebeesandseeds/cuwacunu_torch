void install_hooks_by_editing_git_config(const fs::path& repo_root) {
  const fs::path git_config_path = repo_root / ".git/config";
  if (!fs::exists(git_config_path)) die("missing " + git_config_path.string());

  std::ifstream in(git_config_path);
  if (!in) die("failed to read " + git_config_path.string());
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(in, line)) lines.push_back(line);

  std::vector<std::string> out;
  bool in_core = false;
  bool saw_core = false;
  bool wrote_hook = false;
  auto push_hook = [&]() { out.push_back("\thooksPath = .githooks"); };

  for (const std::string& original : lines) {
    std::string work = trim_copy(original);
    const bool is_section = work.size() >= 2 && work.front() == '[' && work.back() == ']';
    if (is_section) {
      if (in_core && !wrote_hook) {
        push_hook();
        wrote_hook = true;
      }
      out.push_back(original);
      in_core = (work == "[core]");
      saw_core = saw_core || in_core;
      continue;
    }

    if (in_core) {
      std::string probe = original;
      const auto comment_pos = probe.find_first_of("#;");
      if (comment_pos != std::string::npos) probe.erase(comment_pos);
      probe = trim_copy(probe);
      const auto pos = probe.find('=');
      if (pos != std::string::npos) {
        const std::string lhs = trim_copy(probe.substr(0, pos));
        if (lhs == "hooksPath") {
          if (!wrote_hook) {
            push_hook();
            wrote_hook = true;
          }
          continue;
        }
      }
    }

    out.push_back(original);
  }

  if (!saw_core) {
    out.push_back("[core]");
    push_hook();
  } else if (in_core && !wrote_hook) {
    push_hook();
  }

  std::ostringstream rendered;
  for (std::size_t i = 0; i < out.size(); ++i) {
    rendered << out[i];
    if (i + 1 < out.size()) rendered << '\n';
  }
  write_text_file(git_config_path, rendered.str());
  note("set core.hooksPath=.githooks by editing " + git_config_path.string());
}

void install_git_hooks(const fs::path& repo_root) {
  if (command_exists("git")) {
    const run_result_t result = run_command_capture(
        {"git", "-C", repo_root.string(), "config", "core.hooksPath", ".githooks"},
        true);
    if (result.exit_code != 0) {
      die("git config core.hooksPath failed");
    }
    note("set core.hooksPath=.githooks via git config");
    return;
  }
  install_hooks_by_editing_git_config(repo_root);
}

tsodao_config_t load_config(const fs::path& repo_root,
                            const std::optional<fs::path>& global_config_override) {
  tsodao_config_t cfg;
  cfg.repo_root = repo_root;

  if (global_config_override) {
    cfg.global_config_path = global_config_override->lexically_normal();
  } else {
    const char* env_global = std::getenv("CUWACUNU_GLOBAL_CONFIG");
    cfg.global_config_path =
        env_global != nullptr ? fs::path(env_global).lexically_normal()
                              : (repo_root / "src/config/.config").lexically_normal();
  }

  const auto raw_tsodao_path =
      read_ini_value(cfg.global_config_path, "GENERAL", "tsodao_dsl_filename");
  if (!raw_tsodao_path || trim_copy(*raw_tsodao_path).empty()) {
    die("GENERAL.tsodao_dsl_filename is not set in " + cfg.global_config_path.string());
  }

  cfg.tsodao_dsl_path =
      resolve_near(cfg.global_config_path.parent_path(), trim_copy(*raw_tsodao_path));
  if (!fs::exists(cfg.tsodao_dsl_path)) {
    die("tsodao.dsl missing: " + cfg.tsodao_dsl_path.string());
  }

  const fs::path dsl_dir = cfg.tsodao_dsl_path.parent_path();
  cfg.hidden_root = resolve_near(dsl_dir, trim_copy(read_dsl_value(cfg.tsodao_dsl_path, "hidden_root").value_or("")));
  cfg.hidden_archive =
      resolve_near(dsl_dir, trim_copy(read_dsl_value(cfg.tsodao_dsl_path, "hidden_archive").value_or("")));
  cfg.public_keep_path =
      resolve_near(dsl_dir, trim_copy(read_dsl_value(cfg.tsodao_dsl_path, "public_keep_path").value_or("")));
  cfg.local_state_path =
      resolve_near(dsl_dir, trim_copy(read_dsl_value(cfg.tsodao_dsl_path, "local_state_path").value_or("")));
  cfg.visibility_mode =
      lower_ascii_copy(trim_copy(read_dsl_value(cfg.tsodao_dsl_path, "visibility_mode").value_or("")));
  cfg.gpg_recipient = trim_copy(read_dsl_value(cfg.tsodao_dsl_path, "gpg_recipient").value_or(""));

  if (cfg.hidden_root.empty()) die("tsodao.dsl hidden_root is empty");
  if (cfg.hidden_archive.empty()) die("tsodao.dsl hidden_archive is empty");
  if (cfg.public_keep_path.empty()) die("tsodao.dsl public_keep_path is empty");
  if (cfg.local_state_path.empty()) die("tsodao.dsl local_state_path is empty");

  cfg.public_keep_basename = cfg.public_keep_path.filename().string();
  cfg.hidden_root_rel = repo_relative_path(repo_root, cfg.hidden_root);
  cfg.hidden_archive_rel = repo_relative_path(repo_root, cfg.hidden_archive);
  cfg.public_keep_rel = repo_relative_path(repo_root, cfg.public_keep_path);
  return cfg;
}

bool has_hidden_payload(const tsodao_config_t& cfg) {
  std::error_code ec;
  if (!fs::is_directory(cfg.hidden_root, ec)) return false;
  for (fs::recursive_directory_iterator it(
           cfg.hidden_root, fs::directory_options::skip_permission_denied, ec),
       end;
       it != end; it.increment(ec)) {
    if (ec) break;
    const fs::path current = it->path().lexically_normal();
    if (current == cfg.public_keep_path.lexically_normal()) continue;
    return true;
  }
  return false;
}

void ensure_hidden_root_dir(const tsodao_config_t& cfg) {
  std::error_code ec;
  if (!fs::is_directory(cfg.hidden_root, ec)) {
    die("hidden root directory missing: " + cfg.hidden_root.string());
  }
}

void ensure_hidden_archive_exists(const tsodao_config_t& cfg) {
  if (!fs::exists(cfg.hidden_archive)) {
    die("hidden archive missing: " + cfg.hidden_archive.string());
  }
}

std::optional<std::string> archive_sha256(const tsodao_config_t& cfg) {
  if (!fs::exists(cfg.hidden_archive)) return std::nullopt;
  require_command("sha256sum");
  const run_result_t result =
      run_command_capture({"sha256sum", cfg.hidden_archive.string()}, true);
  if (result.exit_code != 0) {
    die("sha256sum failed for " + cfg.hidden_archive.string());
  }
  std::stringstream ss(result.output);
  std::string sha;
  ss >> sha;
  if (sha.empty()) die("sha256sum produced no digest for " + cfg.hidden_archive.string());
  return sha;
}

std::string sha256_hex_for_file(const fs::path& path) {
  require_command("sha256sum");
  const run_result_t result = run_command_capture({"sha256sum", path.string()}, true);
  if (result.exit_code != 0) die("sha256sum failed for " + path.string());
  std::stringstream ss(result.output);
  std::string sha;
  ss >> sha;
  if (sha.empty()) die("sha256sum produced no digest for " + path.string());
  return sha;
}

std::string sha256_hex_for_text_payload(std::string_view payload) {
  std::string templ = (fs::temp_directory_path() / "tsodao.sha.XXXXXX").string();
  std::vector<char> buf(templ.begin(), templ.end());
  buf.push_back('\0');
  const int fd = ::mkstemp(buf.data());
  if (fd < 0) die("mkstemp failed: " + std::string(std::strerror(errno)));
  const fs::path temp_path(buf.data());

  std::size_t offset = 0;
  while (offset < payload.size()) {
    const ssize_t written =
        ::write(fd, payload.data() + offset, payload.size() - offset);
    if (written < 0) {
      const int saved_errno = errno;
      (void)::close(fd);
      std::error_code rm_ec;
      fs::remove(temp_path, rm_ec);
      die("failed to write temporary payload file: " +
          std::string(std::strerror(saved_errno)));
    }
    offset += static_cast<std::size_t>(written);
  }
  if (::close(fd) != 0) {
    const int saved_errno = errno;
    std::error_code rm_ec;
    fs::remove(temp_path, rm_ec);
    die("failed to close temporary payload file: " +
        std::string(std::strerror(saved_errno)));
  }

  const std::string sha = sha256_hex_for_file(temp_path);
  std::error_code rm_ec;
  fs::remove(temp_path, rm_ec);
  return sha;
}

std::string hidden_surface_listing_sha256(const tsodao_config_t& cfg) {
  std::vector<std::string> entries;
  std::error_code ec;
  if (!fs::is_directory(cfg.hidden_root, ec)) {
    return sha256_hex_for_text_payload("");
  }

  const fs::path keep_path = cfg.public_keep_path.lexically_normal();
  for (fs::recursive_directory_iterator it(
           cfg.hidden_root, fs::directory_options::skip_permission_denied, ec),
       end;
       it != end; it.increment(ec)) {
    if (ec) break;
    const fs::path current = it->path().lexically_normal();
    if (current == keep_path) continue;
    if (path_starts_with(current, keep_path)) continue;

    std::error_code rel_ec;
    const fs::path rel = fs::relative(current, cfg.hidden_root, rel_ec);
    if (rel_ec) {
      die("failed to compute hidden-surface relative path for " + current.string());
    }

    std::error_code status_ec;
    const fs::file_status status = fs::symlink_status(current, status_ec);
    std::string kind = "other";
    if (!status_ec) {
      if (fs::is_directory(status)) {
        kind = "dir";
      } else if (fs::is_regular_file(status)) {
        kind = "file";
      } else if (fs::is_symlink(status)) {
        kind = "symlink";
      }
    }
    entries.push_back(kind + "|" + rel.generic_string());
  }

  std::sort(entries.begin(), entries.end());
  std::ostringstream manifest;
  for (const std::string& entry : entries) manifest << entry << "\n";
  return sha256_hex_for_text_payload(manifest.str());
}

void remember_archive_state(const tsodao_config_t& cfg, const std::string& archive_sha,
                            const std::string& action) {
  std::error_code ec;
  fs::create_directories(cfg.local_state_path.parent_path(), ec);
  if (ec) die("failed to create TSODAO state directory: " + ec.message());
  std::ofstream out(cfg.local_state_path);
  if (!out) die("failed to write local state file: " + cfg.local_state_path.string());
  out << "last_archive_sha256 = " << archive_sha << "\n";
  out << "last_sync_action = " << action << "\n";
  out << "last_archive_path = " << cfg.hidden_archive.string() << "\n";
  out << "last_hidden_root = " << cfg.hidden_root.string() << "\n";
  out << "last_hidden_surface_listing_sha256 = "
      << hidden_surface_listing_sha256(cfg) << "\n";
}

bool known_archive_matches_local_state(const tsodao_config_t& cfg) {
  const auto current_sha = archive_sha256(cfg);
  const auto known_sha = read_state_value(cfg.local_state_path, "last_archive_sha256");
  return current_sha && known_sha && trim_copy(*current_sha) == trim_copy(*known_sha);
}

bool needs_archive_refresh(const tsodao_config_t& cfg) {
  std::error_code ec;
  if (!fs::exists(cfg.hidden_archive, ec)) return true;
  const auto known_listing_sha =
      read_state_value(cfg.local_state_path, "last_hidden_surface_listing_sha256");
  if (known_listing_sha) {
    if (trim_copy(*known_listing_sha) != hidden_surface_listing_sha256(cfg)) return true;
  } else if (has_hidden_payload(cfg)) {
    // Older local-state files do not record hidden-surface membership, so the
    // safe upgrade path is to ask for one sync pass instead of assuming
    // deletions or renames did not happen.
    return true;
  }
  const auto archive_time = fs::last_write_time(cfg.hidden_archive, ec);
  if (ec) die("failed to stat hidden archive: " + ec.message());
  if (!fs::is_directory(cfg.hidden_root, ec)) return false;
  for (fs::recursive_directory_iterator it(
           cfg.hidden_root, fs::directory_options::skip_permission_denied, ec),
       end;
       it != end; it.increment(ec)) {
    if (ec) break;
    const fs::path current = it->path().lexically_normal();
    if (current == cfg.public_keep_path.lexically_normal()) continue;
    const auto current_time = fs::last_write_time(current, ec);
    if (ec) continue;
    if (current_time > archive_time) return true;
  }
  return false;
}

void ensure_open_target_is_clean(const tsodao_config_t& cfg) {
  std::error_code ec;
  fs::create_directories(cfg.hidden_root, ec);
  if (ec) die("failed to create hidden root directory: " + ec.message());
  if (has_hidden_payload(cfg)) {
    std::ostringstream oss;
    oss << "hidden root already has plaintext payload\n"
        << "  destination surface = " << display_path(cfg, cfg.hidden_root) << "\n"
        << "  use tsodao sync --from-archive --yes to replace it safely, or run tsodao scrub first";
    die(oss.str());
  }
}

void scrub_hidden_payload(const tsodao_config_t& cfg) {
  ensure_hidden_root_dir(cfg);

  std::vector<fs::path> paths;
  std::error_code ec;
  for (fs::recursive_directory_iterator it(
           cfg.hidden_root, fs::directory_options::skip_permission_denied, ec),
       end;
       it != end; it.increment(ec)) {
    if (ec) break;
    paths.push_back(it->path().lexically_normal());
  }

  std::sort(paths.begin(), paths.end(), [](const fs::path& a, const fs::path& b) {
    const auto depth_a = static_cast<int>(std::distance(a.begin(), a.end()));
    const auto depth_b = static_cast<int>(std::distance(b.begin(), b.end()));
    if (depth_a != depth_b) return depth_a > depth_b;
    return a.generic_string() > b.generic_string();
  });

  for (const fs::path& path : paths) {
    if (path == cfg.public_keep_path.lexically_normal()) continue;
    if (path_starts_with(cfg.public_keep_path.lexically_normal(), path)) continue;
    std::error_code rm_ec;
    fs::remove_all(path, rm_ec);
    if (rm_ec) die("failed to remove " + path.string() + ": " + rm_ec.message());
  }
}

void confirm_restore_over_hidden_payload(const tsodao_config_t& cfg, bool assume_yes) {
  if (!has_hidden_payload(cfg)) return;
  if (!assume_yes) {
    const auto archive_sha = archive_sha256(cfg);
    std::ostringstream oss;
    oss << "confirm restore:\n"
        << "  source archive = " << display_path(cfg, cfg.hidden_archive) << "\n"
        << "  destination surface = " << display_path(cfg, cfg.hidden_root) << "\n"
        << "  archive sha256 = "
        << (archive_sha ? *archive_sha : std::string("unavailable")) << "\n"
        << "  action = replace local plaintext with decrypted archive\n"
        << "continue? [y/N] ";
    prompt(oss.str());
    std::string answer;
    if (!std::getline(std::cin, answer)) die("restore cancelled");
    answer = trim_copy(answer);
    if (!(answer == "y" || answer == "Y")) die("restore cancelled");
  }
  scrub_hidden_payload(cfg);
}

fs::path create_temp_dir() {
  std::string templ = (fs::temp_directory_path() / "tsodao.XXXXXX").string();
  std::vector<char> buf(templ.begin(), templ.end());
  buf.push_back('\0');
  char* made = ::mkdtemp(buf.data());
  if (made == nullptr) die("mkdtemp failed: " + std::string(std::strerror(errno)));
  return fs::path(made);
}

void cleanup_temp_dir(const fs::path& work_dir) {
  if (work_dir.empty()) return;
  std::error_code ec;
  fs::remove_all(work_dir, ec);
}

void seal_hidden_surface(const tsodao_config_t& cfg,
                         const std::vector<std::string>& raw_args) {
  std::string mode = "symmetric";
  std::string recipient;
  bool scrub_after = false;
  bool explicit_mode = false;

  for (std::size_t i = 0; i < raw_args.size(); ++i) {
    const std::string& arg = raw_args[i];
    if (arg == "--symmetric") {
      if (!recipient.empty()) die("choose either --symmetric or --recipient");
      mode = "symmetric";
      explicit_mode = true;
    } else if (arg == "--recipient") {
      if (mode != "symmetric" || !recipient.empty()) {
        die("choose either --symmetric or --recipient");
      }
      if (i + 1 >= raw_args.size()) die("--recipient requires a KEYID");
      mode = "recipient";
      recipient = raw_args[++i];
      explicit_mode = true;
    } else if (arg == "--scrub") {
      scrub_after = true;
    } else {
      die("unknown seal option: " + arg);
    }
  }

  if (!explicit_mode) {
    if (cfg.visibility_mode == "recipient" && !cfg.gpg_recipient.empty()) {
      mode = "recipient";
      recipient = cfg.gpg_recipient;
    } else if (cfg.visibility_mode == "recipient") {
      die("tsodao.dsl visibility_mode=recipient but gpg_recipient is empty\n"
          "  run tsodao init first, or pass seal --recipient KEYID explicitly");
    } else if (cfg.visibility_mode == "symmetric") {
      mode = "symmetric";
    }
  }

  ensure_hidden_root_dir(cfg);
  if (!has_hidden_payload(cfg)) {
    die("no hidden plaintext payload found in " + cfg.hidden_root.string());
  }

  require_command("gpg");
  require_command("tar");
  require_command("sha256sum");
  setup_gpg_tty_if_needed();

  const fs::path work_dir = create_temp_dir();
  const fs::path tar_path = work_dir / "hidden_surface.tar";
  try {
    run_command_or_die({"tar", "--sort=name", "--mtime=UTC 1970-01-01", "--owner=0",
                        "--group=0", "--numeric-owner",
                        "--exclude=./" + cfg.public_keep_basename, "-cf",
                        tar_path.string(), "-C", cfg.hidden_root.string(), "."},
                       "tar");

    if (mode == "recipient") {
      run_command_or_die({"gpg", "--yes", "--output", cfg.hidden_archive.string(),
                          "--encrypt", "--recipient", recipient, tar_path.string()},
                         "gpg encrypt");
      note("seal mode = recipient\n"
           "recipient = " + recipient_summary(recipient));
    } else {
      run_command_or_die({"gpg", "--yes", "--output", cfg.hidden_archive.string(),
                          "--symmetric", "--cipher-algo", "AES256",
                          tar_path.string()},
                         "gpg symmetric encrypt");
      note("seal mode = symmetric AES256");
    }

    std::error_code ec;
    fs::permissions(cfg.hidden_archive,
                    fs::perms::owner_read | fs::perms::owner_write,
                    fs::perm_options::replace, ec);
    if (ec) die("failed to chmod hidden archive: " + ec.message());

    const run_result_t sha_result =
        run_command_capture({"sha256sum", tar_path.string()}, true);
    if (sha_result.exit_code != 0) die("sha256sum failed for plaintext tar");
    std::stringstream ss(sha_result.output);
    std::string tar_sha;
    ss >> tar_sha;
    note("archive written = " + display_path(cfg, cfg.hidden_archive));
    note("plaintext tar sha256 = " + tar_sha);

    const auto archive_sha = archive_sha256(cfg);
    if (!archive_sha) die("failed to record archive sha256");
    remember_archive_state(cfg, *archive_sha, "seal");
    note("archive sha256 = " + *archive_sha);

    if (scrub_after) {
      scrub_hidden_payload(cfg);
      note("scrubbed plaintext payload after sealing\n"
           "  kept public marker = " + display_path(cfg, cfg.public_keep_path));
    }
  } catch (...) {
    cleanup_temp_dir(work_dir);
    throw;
  }
  cleanup_temp_dir(work_dir);
}

std::vector<std::string> list_staged_hidden_plaintext_paths(const tsodao_config_t& cfg) {
  require_command("git");
  const run_result_t result = run_command_capture(
      {"git", "-C", cfg.repo_root.string(), "diff", "--cached", "--name-only",
       "--diff-filter=ACMR", "--", cfg.hidden_root_rel},
      true);
  if (result.exit_code != 0) die("git diff --cached failed");
  std::vector<std::string> out;
  for (const std::string& line : split_lines(result.output)) {
    if (line != cfg.public_keep_rel) out.push_back(line);
  }
  return out;
}

void git_stage_hidden_surface(const tsodao_config_t& cfg) {
  run_command_or_die({"git", "-C", cfg.repo_root.string(), "add", "--",
                      cfg.hidden_archive_rel, cfg.public_keep_rel},
                     "git add");
}

void maybe_git_stage_hidden_surface(const tsodao_config_t& cfg) {
  if (command_exists("git")) {
    git_stage_hidden_surface(cfg);
    note("staged " + cfg.hidden_archive_rel + " and " + cfg.public_keep_rel +
         " in git");
  } else {
    note("git CLI not found; hidden archive was updated but not staged");
  }
}

