void init_usage() {
  std::cout
      << "usage: tsodao init [options]\n\n"
      << "Generate or validate the GPG recipient used by tsodao.dsl for hidden surfaces.\n\n"
      << "options:\n"
      << "  --global-config PATH   Global config path.\n"
      << "  --uid TEXT             GPG user id to generate, for example:\n"
      << "                         \"Cuwacunu TSODAO <alice@example.com>\"\n"
      << "  --yes                  Accept defaults without prompting.\n"
      << "  --validate             Validate the configured TSODAO recipient only.\n"
      << "  --skip-hooks           Do not install repo hooks after setup.\n"
      << "  --help                 Show this help text.\n";
}

int init_command(const tsodao_config_t& cfg, const std::vector<std::string>& args) {
  bool assume_yes = false;
  bool validate_only = false;
  bool skip_hooks = false;
  std::string user_id;

  for (std::size_t i = 0; i < args.size(); ++i) {
    const std::string& arg = args[i];
    if (arg == "--uid") {
      if (i + 1 >= args.size()) die("--uid requires a value");
      user_id = args[++i];
    } else if (arg == "--yes") {
      assume_yes = true;
    } else if (arg == "--validate") {
      validate_only = true;
    } else if (arg == "--skip-hooks") {
      skip_hooks = true;
    } else if (arg == "--help" || arg == "-h") {
      init_usage();
      return 0;
    } else {
      die("unknown init option: " + arg);
    }
  }

  emit_operator_context(cfg);
  require_command("gpg");
  setup_gpg_tty_if_needed();

  if (validate_only) {
    validate_configured_recipient(cfg);
    if (!skip_hooks) install_git_hooks(cfg.repo_root);
    return 0;
  }

  if (cfg.visibility_mode == "recipient" && !cfg.gpg_recipient.empty()) {
    const run_result_t existing =
        run_command_capture({"gpg", "--list-public-keys", cfg.gpg_recipient}, true);
    if (existing.exit_code == 0) {
      note("TSODAO recipient already configured\n"
           "  recipient = " + recipient_summary(cfg.gpg_recipient));
      if (!skip_hooks) install_git_hooks(cfg.repo_root);
      return 0;
    }
  }

  prompt_uid_if_needed(user_id, assume_yes);
  if (user_id.empty()) die("empty GPG user id");

  note("generating or selecting GPG key for TSODAO: " + user_id);
  note("gpg may open pinentry to protect the private key");
  run_command_or_die({"gpg", "--quick-generate-key", user_id, "default", "default",
                      "1y"},
                     "gpg quick-generate-key");

  const std::string fingerprint = find_fingerprint_for_identity(user_id);
  if (fingerprint.empty()) {
    die("failed to resolve fingerprint for " + user_id);
  }

  write_dsl_line(cfg.tsodao_dsl_path, "visibility_mode",
                 "visibility_mode[recipient|symmetric|manual|disabled]:str = recipient");
  write_dsl_line(cfg.tsodao_dsl_path, "gpg_recipient",
                 "gpg_recipient:str = " + fingerprint);

  note("configured tsodao.dsl for recipient mode\n"
       "  recipient = " + recipient_summary(fingerprint));

  if (!skip_hooks) install_git_hooks(cfg.repo_root);
  note("TSODAO key setup complete");
  return 0;
}

void prompt_mark_command(const tsodao_config_t& cfg, const std::vector<std::string>& args) {
  if (!args.empty()) die("prompt-mark takes no extra arguments");
  const sync_plan_t plan = infer_default_sync_plan(cfg);
  if (plan == sync_plan_t::plaintext_to_archive ||
      plan == sync_plan_t::archive_to_plaintext || plan == sync_plan_t::ambiguous) {
    std::cout << "[!]";
  }
}

void usage() {
  std::cout
      << "usage:\n"
      << "  tsodao status\n"
      << "  tsodao init [--uid TEXT] [--yes] [--validate] [--skip-hooks]\n"
      << "  tsodao sync [--from-plaintext|--from-archive] [--yes] [--symmetric|--recipient KEYID] [--scrub]\n"
      << "  tsodao prompt-mark\n"
      << "  tsodao decode\n"
      << "  tsodao seal [--symmetric] [--recipient KEYID] [--scrub]\n"
      << "  tsodao open\n"
      << "  tsodao scrub [--yes]\n"
      << "  tsodao git-pre-commit\n"
      << "  tsodao git-pre-push\n"
      << "\ncommands:\n"
      << "  status        show TSODAO hidden-surface state\n"
      << "  init          generate or validate the TSODAO GPG recipient and hook setup\n"
      << "  sync          safely reconcile plaintext and archive; without flags it never guesses on ambiguous state\n"
      << "  prompt-mark   print [!] when TSODAO sync attention is needed\n"
      << "  decode        alias for open\n"
      << "  seal          create the hidden-surface archive declared by tsodao.dsl\n"
      << "  open          decrypt the hidden-surface archive back into its hidden root\n"
      << "  scrub         remove plaintext hidden-surface payload after sealing\n"
      << "  git-pre-commit  repo hook helper: refresh/stage hidden archive and reject staged plaintext\n"
      << "  git-pre-push    repo hook helper: reject pushes that would publish hidden plaintext\n"
      << "\nnotes:\n"
      << "  - tsodao.dsl is the source of truth for the first TSODAO public/private policy\n"
      << "  - the common day-to-day flow is: tsodao init, tsodao sync, git commit, git push\n"
      << "  - sync alone auto-picks a direction only when that direction is provably safe\n"
      << "  - sync --from-plaintext means local plaintext is the source of truth and the archive should be resealed\n"
      << "  - sync --from-archive means the encrypted archive is the source of truth and the hidden root should be restored\n"
      << "  - compatibility aliases still work: --to-archive, --to-hidden, --prefer-hidden, --prefer-archive\n";
}

}  // namespace
