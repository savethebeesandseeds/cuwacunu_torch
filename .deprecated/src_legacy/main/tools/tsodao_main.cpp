
int main(int argc, char** argv) {
  try {
    initialize_output_style();
    if (argc < 2) {
      usage();
      return 1;
    }

    const std::string command = argv[1];
    if (command == "-h" || command == "--help" || command == "help") {
      usage();
      return 0;
    }

    const fs::path repo_root = find_repo_root(argc > 0 ? argv[0] : nullptr);
    if (repo_root.empty()) {
      emit_lines(std::cerr, output_kind_t::error, "could not locate repo root", true);
      return 2;
    }
    std::optional<fs::path> global_config_override;
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--global-config") {
        if (i + 1 >= argc) die("--global-config requires a path");
        global_config_override = fs::path(argv[++i]);
        continue;
      }
      args.emplace_back(arg);
    }
    const tsodao_config_t cfg = load_config(repo_root, global_config_override);

    if (command == "status") {
      if (!args.empty()) die("status takes no extra arguments");
      status_command(cfg);
      return 0;
    }
    if (command == "init") {
      return init_command(cfg, args);
    }
    if (command == "sync") {
      sync_command(cfg, args);
      return 0;
    }
    if (command == "prompt-mark") {
      prompt_mark_command(cfg, args);
      return 0;
    }
    if (command == "decode" || command == "open") {
      if (!args.empty()) die(command + " takes no extra arguments");
      emit_operator_context(cfg);
      open_hidden_surface(cfg);
      return 0;
    }
    if (command == "seal") {
      emit_operator_context(cfg);
      seal_hidden_surface(cfg, args);
      return 0;
    }
    if (command == "scrub") {
      emit_operator_context(cfg);
      scrub_command(cfg, args);
      return 0;
    }
    if (command == "git-pre-commit") {
      if (!args.empty()) die("git-pre-commit takes no extra arguments");
      git_pre_commit(cfg);
      return 0;
    }
    if (command == "git-pre-push") {
      if (!args.empty()) die("git-pre-push takes no extra arguments");
      git_pre_push(cfg);
      return 0;
    }

    die("unknown command: " + command);
  } catch (const std::exception& ex) {
    emit_lines(std::cerr, output_kind_t::error, ex.what(), true);
    return 1;
  }
}
