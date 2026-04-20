[[nodiscard]] bool handle_tool_start_campaign(app_context_t *app,
                                              const std::string &arguments_json,
                                              std::string *out_structured,
                                              std::string *out_error);
[[nodiscard]] bool handle_tool_explain_binding_selection(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error);
[[nodiscard]] bool handle_tool_list_campaigns(app_context_t *app,
                                              const std::string &arguments_json,
                                              std::string *out_structured,
                                              std::string *out_error);
[[nodiscard]] bool handle_tool_get_campaign(app_context_t *app,
                                            const std::string &arguments_json,
                                            std::string *out_structured,
                                            std::string *out_error);
[[nodiscard]] bool handle_tool_stop_campaign(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error);
[[nodiscard]] bool handle_tool_list_jobs(app_context_t *app,
                                         const std::string &arguments_json,
                                         std::string *out_structured,
                                         std::string *out_error);
[[nodiscard]] bool handle_tool_get_job(app_context_t *app,
                                       const std::string &arguments_json,
                                       std::string *out_structured,
                                       std::string *out_error);
[[nodiscard]] bool handle_tool_stop_job(app_context_t *app,
                                        const std::string &arguments_json,
                                        std::string *out_structured,
                                        std::string *out_error);
[[nodiscard]] bool handle_tool_tail_log(app_context_t *app,
                                        const std::string &arguments_json,
                                        std::string *out_structured,
                                        std::string *out_error);
[[nodiscard]] bool handle_tool_tail_trace(app_context_t *app,
                                          const std::string &arguments_json,
                                          std::string *out_structured,
                                          std::string *out_error);
[[nodiscard]] bool handle_tool_reconcile(app_context_t *app,
                                         const std::string &arguments_json,
                                         std::string *out_structured,
                                         std::string *out_error);
[[nodiscard]] bool tail_file_lines(const std::filesystem::path &path,
                                   std::size_t lines, std::string *out,
                                   std::string *error);
[[nodiscard]] bool launch_campaign_runner_process(
    const app_context_t &app,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &seed_record,
    std::string *error);

struct bounded_text_excerpt_t {
  bool include_text{true};
  bool text_omitted{false};
  bool truncated_by_lines{false};
  bool truncated_by_bytes{false};
  std::size_t requested_lines{0};
  std::size_t line_limit{0};
  std::size_t effective_lines{0};
  std::size_t max_bytes{0};
  std::size_t returned_bytes{0};
  std::size_t returned_lines{0};
  std::uintmax_t file_bytes{0};
  std::string text{};
};

[[nodiscard]] bool read_bounded_text_excerpt(const std::filesystem::path &path,
                                             const std::string &arguments_json,
                                             std::size_t requested_lines,
                                             bounded_text_excerpt_t *out,
                                             std::string *error) {
  if (!out) {
    if (error)
      *error = "bounded text excerpt output pointer is null";
    return false;
  }

  bounded_text_excerpt_t excerpt{};
  excerpt.requested_lines = requested_lines;
  excerpt.line_limit = kDefaultInlineLogLines;
  excerpt.max_bytes = kDefaultInlineLogBytes;
  (void)extract_json_bool_field(arguments_json, "include_text",
                                &excerpt.include_text);
  (void)extract_json_size_field(arguments_json, "inline_line_limit",
                                &excerpt.line_limit);
  (void)extract_json_size_field(arguments_json, "max_bytes",
                                &excerpt.max_bytes);
  if (excerpt.line_limit == 0)
    excerpt.line_limit = kDefaultInlineLogLines;
  if (excerpt.max_bytes == 0)
    excerpt.max_bytes = kDefaultInlineLogBytes;
  excerpt.line_limit = std::min(excerpt.line_limit, kHardInlineLogLines);
  excerpt.max_bytes = std::min(excerpt.max_bytes, kHardInlineLogBytes);
  excerpt.file_bytes = file_size_or_zero(path);
  excerpt.text_omitted = !excerpt.include_text;

  if (excerpt.include_text) {
    excerpt.effective_lines =
        (requested_lines == 0) ? excerpt.line_limit
                               : std::min(requested_lines, excerpt.line_limit);
    excerpt.truncated_by_lines =
        (requested_lines == 0) || (requested_lines > excerpt.effective_lines);

    std::string text{};
    if (!tail_file_lines(path, excerpt.effective_lines, &text, error))
      return false;
    text = sanitize_inline_log_text(text);
    if (text.size() > excerpt.max_bytes) {
      text = text.substr(text.size() - excerpt.max_bytes);
      excerpt.truncated_by_bytes = true;
    }
    excerpt.returned_bytes = text.size();
    excerpt.returned_lines = count_text_lines(text);
    excerpt.text = std::move(text);
  }

  *out = std::move(excerpt);
  return true;
}

constexpr runtime_tool_descriptor_t kRuntimeTools[] = {
#define HERO_RUNTIME_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER)       \
  runtime_tool_descriptor_t{NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
#include "hero/runtime_hero/hero_runtime_tools.def"
#undef HERO_RUNTIME_TOOL
};

[[nodiscard]] const runtime_tool_descriptor_t *
find_runtime_tool_descriptor(std::string_view name) {
  for (const auto &descriptor : kRuntimeTools) {
    if (descriptor.name == name)
      return &descriptor;
  }
  return nullptr;
}

[[nodiscard]] std::string initialize_result_json() {
  std::ostringstream out;
  out << "{"
      << "\"protocolVersion\":" << json_quote(kProtocolVersion) << ","
      << "\"serverInfo\":{\"name\":" << json_quote(kServerName)
      << ",\"version\":" << json_quote(kServerVersion) << "},"
      << "\"capabilities\":{\"tools\":{\"listChanged\":false}},"
      << "\"instructions\":" << json_quote(kInitializeInstructions) << "}";
  return out.str();
}

[[nodiscard]] bool acquire_campaign_start_lock(const app_context_t &app,
                                               campaign_start_lock_guard_t *out,
                                               std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "campaign start lock output pointer is null";
    return false;
  }
  out->reset();

  std::error_code ec{};
  std::filesystem::create_directories(app.defaults.campaigns_root, ec);
  if (ec) {
    if (error) {
      *error = "cannot create campaigns_root for campaign start lock: " +
               app.defaults.campaigns_root.string();
    }
    return false;
  }

  const std::filesystem::path lock_path =
      cuwacunu::hero::runtime::runtime_campaign_start_lock_path(
          app.defaults.campaigns_root);
  const int fd = ::open(lock_path.c_str(), O_CREAT | O_RDWR, 0600);
  if (fd < 0) {
    if (error) {
      *error = "cannot open campaign start lock: " + lock_path.string();
    }
    return false;
  }

  for (;;) {
    if (::flock(fd, LOCK_EX) == 0) {
      out->fd = fd;
      return true;
    }
    if (errno == EINTR)
      continue;
    (void)::close(fd);
    if (error) {
      *error = "cannot acquire campaign start lock: " + lock_path.string();
    }
    return false;
  }
}

void write_child_errno_noexcept(int fd, int child_errno) {
  const char *ptr = reinterpret_cast<const char *>(&child_errno);
  std::size_t remaining = sizeof(child_errno);
  while (remaining > 0) {
    const ssize_t written = ::write(fd, ptr, remaining);
    if (written > 0) {
      ptr += static_cast<std::size_t>(written);
      remaining -= static_cast<std::size_t>(written);
      continue;
    }
    if (written < 0 && errno == EINTR)
      continue;
    break;
  }
}

[[nodiscard]] bool rewrite_campaign_imports_absolute(
    const std::filesystem::path &source_campaign_path,
    std::string_view input_text, std::string *out_text, std::string *error) {
  if (error)
    error->clear();
  if (!out_text) {
    if (error)
      *error = "campaign snapshot output pointer is null";
    return false;
  }
  *out_text = "";

  bool in_block_comment = false;
  std::ostringstream out;
  std::size_t pos = 0;
  bool first = true;
  while (pos <= input_text.size()) {
    const std::size_t end = input_text.find('\n', pos);
    std::string line = (end == std::string::npos)
                           ? std::string(input_text.substr(pos))
                           : std::string(input_text.substr(pos, end - pos));

    const auto rewrite_import = [&](std::string_view token,
                                    const std::string &current_line) {
      std::size_t token_pos = current_line.find(token);
      if (token_pos == std::string::npos)
        return current_line;
      const std::size_t quote_begin =
          current_line.find('"', token_pos + token.size());
      if (quote_begin == std::string::npos)
        return current_line;
      const std::size_t quote_end = current_line.find('"', quote_begin + 1);
      if (quote_end == std::string::npos)
        return current_line;
      const std::filesystem::path raw_path(
          current_line.substr(quote_begin + 1, quote_end - quote_begin - 1));
      if (raw_path.is_absolute())
        return current_line;
      const std::filesystem::path rewritten =
          (source_campaign_path.parent_path() / raw_path).lexically_normal();
      return current_line.substr(0, quote_begin + 1) + rewritten.string() +
             current_line.substr(quote_end);
    };

    if (!in_block_comment) {
      line = rewrite_import("IMPORT_CONTRACT", line);
      line = rewrite_import("FROM", line);
      line = rewrite_import("IMPORT_CONTRACT_FILE", line);
      line = rewrite_import("IMPORT_WAVE_FILE", line);
      line = rewrite_import("MARSHAL", line);
    }
    if (!first)
      out << "\n";
    first = false;
    out << line;
    in_block_comment = cuwacunu::hero::wave_contract_binding_runtime::detail::
        next_block_comment_state(line, in_block_comment);
    if (end == std::string::npos)
      break;
    pos = end + 1;
  }
  *out_text = out.str();
  return true;
}

[[nodiscard]] bool prepare_campaign_dsl_snapshot(
    const app_context_t &app, const std::string &campaign_cursor,
    const std::filesystem::path &global_config_path,
    std::string *out_source_campaign_path,
    std::string *out_campaign_snapshot_path, std::string *error) {
  const std::filesystem::path source_campaign_path =
      resolve_default_campaign_dsl_path(global_config_path, error);
  if (source_campaign_path.empty())
    return false;
  return prepare_campaign_dsl_snapshot_from_source(
      app, campaign_cursor, source_campaign_path, out_source_campaign_path,
      out_campaign_snapshot_path, error);
}

[[nodiscard]] bool
validate_cuwacunu_campaign_binary_for_launch(const app_context_t &app,
                                             std::string *error) {
  if (error)
    error->clear();

  const std::filesystem::path &worker_path =
      app.defaults.cuwacunu_campaign_binary;
  if (worker_path.empty()) {
    if (error)
      *error = "runtime defaults missing cuwacunu_campaign_binary";
    return false;
  }

  std::error_code ec{};
  if (!std::filesystem::exists(worker_path, ec) ||
      !std::filesystem::is_regular_file(worker_path, ec)) {
    if (error) {
      *error = "configured cuwacunu_campaign_binary does not exist: " +
               worker_path.string();
    }
    return false;
  }

  if (::access(worker_path.c_str(), X_OK) != 0) {
    if (error) {
      *error = "configured cuwacunu_campaign_binary is not executable: " +
               worker_path.string();
    }
    return false;
  }

  return true;
}

[[nodiscard]] bool decode_campaign_snapshot(
    const app_context_t &app, const std::string &campaign_dsl_path,
    cuwacunu::camahjucunu::iitepi_campaign_instruction_t *out_instruction,
    std::string *error) {
  if (error)
    error->clear();
  if (!out_instruction) {
    if (error)
      *error = "campaign decode output pointer is null";
    return false;
  }
  std::string grammar_text{};
  if (!cuwacunu::hero::runtime::read_text_file(
          app.defaults.campaign_grammar_path, &grammar_text, error)) {
    return false;
  }
  std::string campaign_text{};
  if (!cuwacunu::hero::runtime::read_text_file(campaign_dsl_path,
                                               &campaign_text, error)) {
    return false;
  }
  try {
    *out_instruction =
        cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
            grammar_text, campaign_text);
  } catch (const std::exception &e) {
    if (error) {
      *error = "failed to decode campaign DSL snapshot '" + campaign_dsl_path +
               "': " + e.what();
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool select_campaign_run_bind_ids(
    const cuwacunu::camahjucunu::iitepi_campaign_instruction_t &instruction,
    std::string requested_binding_id, std::vector<std::string> *out,
    std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "campaign run-bind output pointer is null";
    return false;
  }
  out->clear();

  requested_binding_id = trim_ascii(requested_binding_id);
  if (!requested_binding_id.empty()) {
    const auto bind_it = std::find_if(
        instruction.binds.begin(), instruction.binds.end(),
        [&](const auto &bind) { return bind.id == requested_binding_id; });
    if (bind_it == instruction.binds.end()) {
      if (error) {
        *error =
            "campaign does not declare BIND '" + requested_binding_id + "'";
      }
      return false;
    }
    out->push_back(requested_binding_id);
    return true;
  }

  if (instruction.runs.empty()) {
    if (error)
      *error = "campaign does not define any RUN entries";
    return false;
  }
  for (const auto &run : instruction.runs) {
    const std::string bind_ref = trim_ascii(run.bind_ref);
    if (!bind_ref.empty())
      out->push_back(bind_ref);
  }
  if (out->empty()) {
    if (error)
      *error = "campaign RUN list resolved to zero bind ids";
    return false;
  }
  return true;
}

[[nodiscard]] bool launch_campaign_under_lock(
    const app_context_t &app, const campaign_launch_request_t &request,
    cuwacunu::hero::runtime::runtime_campaign_record_t *out_record,
    std::string *error) {
  if (error)
    error->clear();
  if (out_record)
    *out_record = cuwacunu::hero::runtime::runtime_campaign_record_t{};

  std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> campaigns{};
  if (!list_runtime_campaigns(app, &campaigns, error))
    return false;
  std::size_t active_campaigns = 0;
  bool active_reset_campaign = false;
  for (auto &campaign : campaigns) {
    bool changed = false;
    if (!reconcile_campaign_record(app, &campaign, &changed, error))
      return false;
    if (campaign.state == "launching" || campaign.state == "running" ||
        campaign.state == "stopping") {
      ++active_campaigns;
      if (campaign.reset_runtime_state) {
        active_reset_campaign = true;
      }
    }
  }
  if (request.reset_runtime_state && active_campaigns > 0) {
    if (error) {
      *error =
          "reset_runtime_state requires no active campaigns under runtime root";
    }
    return false;
  }
  if (!request.reset_runtime_state && active_reset_campaign) {
    if (error) {
      *error = "cannot launch a campaign while a reset_runtime_state campaign "
               "is active";
    }
    return false;
  }
  if (!validate_cuwacunu_campaign_binary_for_launch(app, error))
    return false;

  const std::string campaign_cursor =
      cuwacunu::hero::runtime::make_campaign_cursor(
          app.defaults.campaigns_root);
  const std::filesystem::path source_campaign_path =
      resolve_requested_campaign_source_path(app, app.global_config_path,
                                             request.campaign_dsl_path, error);
  if (source_campaign_path.empty())
    return false;

  std::string source_campaign_dsl_path{};
  std::string campaign_dsl_path{};
  if (!prepare_campaign_dsl_snapshot_from_source(
          app, campaign_cursor, source_campaign_path, &source_campaign_dsl_path,
          &campaign_dsl_path, error)) {
    return false;
  }
  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  if (!decode_campaign_snapshot(app, campaign_dsl_path, &instruction, error)) {
    return false;
  }
  std::vector<std::string> run_bind_ids{};
  if (!select_campaign_run_bind_ids(instruction, request.binding_id,
                                    &run_bind_ids, error)) {
    return false;
  }

  cuwacunu::hero::runtime::runtime_campaign_record_t record{};
  record.campaign_cursor = campaign_cursor;
  record.boot_id = cuwacunu::hero::runtime::current_boot_id();
  record.state = "launching";
  record.state_detail = "detached campaign runner requested";
  record.marshal_session_id = request.marshal_session_id;
  record.global_config_path = app.global_config_path.string();
  record.source_campaign_dsl_path = source_campaign_dsl_path;
  record.campaign_dsl_path = campaign_dsl_path;
  record.reset_runtime_state = request.reset_runtime_state;
  record.stdout_path = cuwacunu::hero::runtime::runtime_campaign_stdout_path(
                           app.defaults.campaigns_root, campaign_cursor)
                           .string();
  record.stderr_path = cuwacunu::hero::runtime::runtime_campaign_stderr_path(
                           app.defaults.campaigns_root, campaign_cursor)
                           .string();
  record.started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  record.updated_at_ms = record.started_at_ms;
  record.run_bind_ids = std::move(run_bind_ids);

  if (!write_runtime_campaign(app, record, error))
    return false;
  if (!launch_campaign_runner_process(app, record, error)) {
    record.state = "failed_to_start";
    record.state_detail = *error;
    record.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    record.updated_at_ms = *record.finished_at_ms;
    std::string ignored{};
    (void)write_runtime_campaign(app, record, &ignored);
    return false;
  }

  for (int attempt = 0; attempt < 80; ++attempt) {
    std::string poll_error{};
    if (read_runtime_campaign(app, campaign_cursor, &record, &poll_error)) {
      if (record.runner_pid.has_value() || record.state != "launching") {
        break;
      }
    }
    ::usleep(25 * 1000);
  }
  if (!read_runtime_campaign(app, campaign_cursor, &record, error))
    return false;
  if (out_record)
    *out_record = record;
  return true;
}

[[nodiscard]] bool
run_command_with_stdio_and_timeout(const std::vector<std::string> &argv,
                                   const std::filesystem::path &stdin_path,
                                   const std::filesystem::path &stdout_path,
                                   const std::filesystem::path &stderr_path,
                                   std::size_t timeout_sec, int *out_exit_code,
                                   std::string *error) {
  if (error)
    error->clear();
  if (out_exit_code)
    *out_exit_code = -1;
  if (argv.empty()) {
    if (error)
      *error = "command argv is empty";
    return false;
  }
  std::error_code ec{};
  if (!std::filesystem::exists(stdin_path, ec) ||
      !std::filesystem::is_regular_file(stdin_path, ec)) {
    if (error)
      *error = "command stdin path does not exist: " + stdin_path.string();
    return false;
  }
  std::filesystem::create_directories(stdout_path.parent_path(), ec);
  if (ec) {
    if (error)
      *error = "cannot create command stdout parent: " + stdout_path.string();
    return false;
  }
  std::filesystem::create_directories(stderr_path.parent_path(), ec);
  if (ec) {
    if (error)
      *error = "cannot create command stderr parent: " + stderr_path.string();
    return false;
  }
  const int stdout_probe =
      ::open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (stdout_probe < 0) {
    if (error)
      *error = "cannot open command stdout path: " + stdout_path.string();
    return false;
  }
  (void)::close(stdout_probe);
  const int stderr_probe =
      ::open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (stderr_probe < 0) {
    if (error)
      *error = "cannot open command stderr path: " + stderr_path.string();
    return false;
  }
  (void)::close(stderr_probe);

  const pid_t child = ::fork();
  if (child < 0) {
    if (error)
      *error = "fork failed for command execution";
    return false;
  }
  if (child == 0) {
    (void)::setpgid(0, 0);
    const int stdin_fd = ::open(stdin_path.c_str(), O_RDONLY);
    if (stdin_fd < 0)
      _exit(126);
    (void)::dup2(stdin_fd, STDIN_FILENO);
    if (stdin_fd > STDERR_FILENO)
      (void)::close(stdin_fd);
    const int stdout_fd =
        ::open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stdout_fd < 0)
      _exit(126);
    (void)::dup2(stdout_fd, STDOUT_FILENO);
    if (stdout_fd > STDERR_FILENO)
      (void)::close(stdout_fd);
    const int stderr_fd =
        ::open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stderr_fd < 0)
      _exit(126);
    (void)::dup2(stderr_fd, STDERR_FILENO);
    if (stderr_fd > STDERR_FILENO)
      (void)::close(stderr_fd);
    std::vector<char *> exec_argv{};
    exec_argv.reserve(argv.size() + 1);
    for (const std::string &arg : argv) {
      exec_argv.push_back(const_cast<char *>(arg.c_str()));
    }
    exec_argv.push_back(nullptr);
    ::execvp(exec_argv[0], exec_argv.data());
    _exit(127);
  }

  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);
  int status = 0;
  for (;;) {
    const pid_t waited = ::waitpid(child, &status, WNOHANG);
    if (waited == child)
      break;
    if (waited < 0 && errno != EINTR) {
      if (error)
        *error = "waitpid failed for command execution";
      (void)::kill(-child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      return false;
    }
    if (std::chrono::steady_clock::now() >= deadline) {
      (void)::kill(-child, SIGKILL);
      (void)::kill(child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      if (error)
        *error = "command timed out";
      return false;
    }
    ::usleep(100 * 1000);
  }

  if (WIFEXITED(status)) {
    if (out_exit_code)
      *out_exit_code = WEXITSTATUS(status);
    return true;
  }
  if (WIFSIGNALED(status)) {
    if (out_exit_code)
      *out_exit_code = 128 + WTERMSIG(status);
    if (error)
      *error = "command terminated by signal";
    return false;
  }
  if (error)
    *error = "command ended in unknown state";
  return false;
}

[[nodiscard]] bool launch_campaign_runner_process(
    const app_context_t &app,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &seed_record,
    std::string *error) {
  if (error)
    error->clear();

  int pipe_fds[2]{-1, -1};
#ifdef O_CLOEXEC
  if (::pipe2(pipe_fds, O_CLOEXEC) != 0) {
    if (error)
      *error = "pipe2 failed";
    return false;
  }
#else
  if (::pipe(pipe_fds) != 0) {
    if (error)
      *error = "pipe failed";
    return false;
  }
  (void)::fcntl(pipe_fds[0], F_SETFD, FD_CLOEXEC);
  (void)::fcntl(pipe_fds[1], F_SETFD, FD_CLOEXEC);
#endif

  const pid_t child = ::fork();
  if (child < 0) {
    const std::string msg = "fork failed";
    (void)::close(pipe_fds[0]);
    (void)::close(pipe_fds[1]);
    if (error)
      *error = msg;
    return false;
  }
  if (child == 0) {
    (void)::close(pipe_fds[0]);
    if (::setsid() < 0) {
      const int child_errno = errno;
      write_child_errno_noexcept(pipe_fds[1], child_errno);
      _exit(127);
    }

    const pid_t grandchild = ::fork();
    if (grandchild < 0) {
      const int child_errno = errno;
      write_child_errno_noexcept(pipe_fds[1], child_errno);
      _exit(127);
    }
    if (grandchild > 0) {
      _exit(0);
    }

    const int devnull = ::open("/dev/null", O_RDWR);
    if (devnull >= 0) {
      (void)::dup2(devnull, STDIN_FILENO);
      (void)::dup2(devnull, STDOUT_FILENO);
      (void)::dup2(devnull, STDERR_FILENO);
      if (devnull > STDERR_FILENO)
        (void)::close(devnull);
    }

    std::vector<std::string> args{};
    args.push_back(app.self_binary_path.string());
    args.push_back("--campaign-runner");
    args.push_back("--campaigns-root");
    args.push_back(app.defaults.campaigns_root.string());
    args.push_back("--campaign-cursor");
    args.push_back(seed_record.campaign_cursor);
    args.push_back("--worker-binary");
    args.push_back(app.defaults.cuwacunu_campaign_binary.string());
    args.push_back("--global-config");
    args.push_back(seed_record.global_config_path);
    if (seed_record.reset_runtime_state) {
      args.push_back("--reset-runtime-state");
    }

    std::vector<char *> argv{};
    argv.reserve(args.size() + 1);
    for (auto &arg : args)
      argv.push_back(arg.data());
    argv.push_back(nullptr);
    ::execv(argv[0], argv.data());

    const int child_errno = errno;
    write_child_errno_noexcept(pipe_fds[1], child_errno);
    _exit(127);
  }

  (void)::close(pipe_fds[1]);
  int exec_errno = 0;
  const ssize_t n = ::read(pipe_fds[0], &exec_errno, sizeof(exec_errno));
  (void)::close(pipe_fds[0]);
  int ignored_status = 0;
  while (::waitpid(child, &ignored_status, 0) < 0 && errno == EINTR) {
  }
  if (n > 0) {
    if (error) {
      *error = "cannot exec detached runtime campaign runner: errno=" +
               std::to_string(exec_errno);
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool handle_tool_start_campaign(app_context_t *app,
                                              const std::string &arguments_json,
                                              std::string *out_structured,
                                              std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string binding_id{};
  bool reset_runtime_state = false;
  std::string campaign_dsl_path{};
  std::string marshal_session_id{};
  (void)extract_json_string_field(arguments_json, "binding_id", &binding_id);
  (void)extract_json_bool_field(arguments_json, "reset_runtime_state",
                                &reset_runtime_state);
  (void)extract_json_string_field(arguments_json, "campaign_dsl_path",
                                  &campaign_dsl_path);
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  binding_id = trim_ascii(binding_id);
  campaign_dsl_path = trim_ascii(campaign_dsl_path);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (app->global_config_path.empty()) {
    *out_error = "runtime MCP app missing global_config_path";
    return false;
  }

  const std::filesystem::path source_campaign_path =
      resolve_requested_campaign_source_path(*app, app->global_config_path,
                                             campaign_dsl_path, out_error);
  if (source_campaign_path.empty())
    return false;

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t source_instruction{};
  if (!decode_campaign_snapshot(*app, source_campaign_path.string(),
                                &source_instruction, out_error)) {
    return false;
  }

  std::vector<std::string> warnings{};
  if (marshal_session_id.empty() &&
      !trim_ascii(source_instruction.marshal_objective_file).empty()) {
    warnings.push_back(
        "campaign declares MARSHAL \"" +
        trim_ascii(source_instruction.marshal_objective_file) +
        "\" but Runtime Hero direct launch does not run supervision; use "
        "hero.marshal.start_session(...) when you want the MARSHAL objective "
        "honored");
  }

  campaign_launch_request_t request{};
  request.binding_id = binding_id;
  request.reset_runtime_state = reset_runtime_state;
  request.campaign_dsl_path = source_campaign_path.string();
  request.marshal_session_id = marshal_session_id;

  campaign_start_lock_guard_t start_lock{};
  if (!acquire_campaign_start_lock(*app, &start_lock, out_error))
    return false;
  cuwacunu::hero::runtime::runtime_campaign_record_t record{};
  if (!launch_campaign_under_lock(*app, request, &record, out_error)) {
    return false;
  }

  const std::string warnings_json = json_array_from_strings(warnings);
  *out_structured =
      "{\"campaign_cursor\":" + json_quote(record.campaign_cursor) +
      ",\"campaign\":" + runtime_campaign_to_json(record) +
      ",\"warnings\":" + warnings_json + "}";
  return true;
}

[[nodiscard]] bool handle_tool_explain_binding_selection(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string binding_id{};
  std::string campaign_dsl_path{};
  (void)extract_json_string_field(arguments_json, "binding_id", &binding_id);
  (void)extract_json_string_field(arguments_json, "campaign_dsl_path",
                                  &campaign_dsl_path);
  binding_id = trim_ascii(binding_id);
  campaign_dsl_path = trim_ascii(campaign_dsl_path);

  if (binding_id.empty()) {
    *out_error = "explain_binding_selection requires arguments.binding_id";
    return false;
  }
  if (app->global_config_path.empty()) {
    *out_error = "runtime MCP app missing global_config_path";
    return false;
  }

  const std::filesystem::path source_campaign_path =
      resolve_requested_campaign_source_path(*app, app->global_config_path,
                                             campaign_dsl_path, out_error);
  if (source_campaign_path.empty())
    return false;

  cuwacunu::iitepi::config_space_t::change_config_file(
      app->global_config_path.string().c_str());

  cuwacunu::hero::wave_contract_binding_runtime::
      resolved_binding_selection_explanation_t explanation{};
  std::string explain_error{};
  if (!cuwacunu::hero::wave_contract_binding_runtime::
          explain_campaign_binding_selection(source_campaign_path, binding_id,
                                             &explanation, &explain_error)) {
    if (explanation.source_campaign_dsl_path.empty()) {
      explanation.source_campaign_dsl_path = source_campaign_path.string();
    }
    if (explanation.binding_id.empty())
      explanation.binding_id = binding_id;
    if (explanation.summary.empty()) {
      explanation.summary = "binding selection explanation failed";
    }
    if (explanation.details.empty()) {
      explanation.details = explain_error.empty()
                                ? "binding selection explanation failed"
                                : explain_error;
    }
    explanation.resolved = false;
    *out_structured = binding_selection_explanation_to_json(explanation);
    return true;
  }

  *out_structured = binding_selection_explanation_to_json(explanation);
  return true;
}

[[nodiscard]] bool handle_tool_list_campaigns(app_context_t *app,
                                              const std::string &arguments_json,
                                              std::string *out_structured,
                                              std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string state_filter{};
  std::size_t limit = 0;
  std::size_t offset = 0;
  bool newest_first = true;
  (void)extract_json_string_field(arguments_json, "state", &state_filter);
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);
  (void)extract_json_bool_field(arguments_json, "newest_first", &newest_first);
  state_filter = trim_ascii(state_filter);

  std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> campaigns{};
  if (!list_runtime_campaigns(*app, &campaigns, out_error))
    return false;

  std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> rows{};
  for (auto &campaign : campaigns) {
    bool changed = false;
    if (!reconcile_campaign_record(*app, &campaign, &changed, out_error)) {
      return false;
    }
    if (!state_filter.empty() && campaign.state != state_filter)
      continue;
    rows.push_back(std::move(campaign));
  }

  std::sort(rows.begin(), rows.end(),
            [newest_first](const auto &a, const auto &b) {
              if (a.started_at_ms != b.started_at_ms) {
                return newest_first ? (a.started_at_ms > b.started_at_ms)
                                    : (a.started_at_ms < b.started_at_ms);
              }
              return newest_first ? (a.campaign_cursor > b.campaign_cursor)
                                  : (a.campaign_cursor < b.campaign_cursor);
            });

  const std::size_t total = rows.size();
  const std::size_t off = std::min(offset, rows.size());
  std::size_t count = limit;
  if (count == 0)
    count = rows.size() - off;
  count = std::min(count, rows.size() - off);

  std::ostringstream campaigns_json;
  campaigns_json << "[";
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0)
      campaigns_json << ",";
    campaigns_json << runtime_campaign_to_json(rows[off + i]);
  }
  campaigns_json << "]";

  std::ostringstream out;
  out << "{\"campaign_cursor\":\"\""
      << ",\"count\":" << count << ",\"total\":" << total
      << ",\"state\":" << json_quote(state_filter)
      << ",\"campaigns\":" << campaigns_json.str() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_tail_trace(app_context_t *app,
                                          const std::string &arguments_json,
                                          std::string *out_structured,
                                          std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string job_cursor{};
  std::size_t lines = app->defaults.tail_default_lines;
  (void)extract_json_string_field(arguments_json, "job_cursor", &job_cursor);
  (void)extract_json_size_field(arguments_json, "lines", &lines);
  job_cursor = trim_ascii(job_cursor);
  if (job_cursor.empty()) {
    *out_error = "tail_trace requires arguments.job_cursor";
    return false;
  }

  cuwacunu::hero::runtime::runtime_job_record_t record{};
  if (!read_runtime_job(*app, job_cursor, &record, out_error))
    return false;

  const auto trace_path = runtime_job_trace_path_for_record(*app, record);
  bounded_text_excerpt_t excerpt{};
  if (!read_bounded_text_excerpt(trace_path, arguments_json, lines, &excerpt,
                                 out_error))
    return false;

  std::ostringstream out;
  out << "{\"job_cursor\":" << json_quote(job_cursor)
      << ",\"requested_lines\":" << excerpt.requested_lines
      << ",\"lines\":" << excerpt.effective_lines
      << ",\"inline_line_limit\":" << excerpt.line_limit
      << ",\"max_bytes\":" << excerpt.max_bytes
      << ",\"file_bytes\":" << excerpt.file_bytes
      << ",\"returned_bytes\":" << excerpt.returned_bytes
      << ",\"returned_lines\":" << excerpt.returned_lines << ",\"truncated\":"
      << ((excerpt.truncated_by_lines || excerpt.truncated_by_bytes) ? "true"
                                                                     : "false")
      << ",\"truncated_by_lines\":"
      << (excerpt.truncated_by_lines ? "true" : "false")
      << ",\"truncated_by_bytes\":"
      << (excerpt.truncated_by_bytes ? "true" : "false")
      << ",\"text_omitted\":" << (excerpt.text_omitted ? "true" : "false")
      << ",\"path\":" << json_quote(trace_path.string())
      << ",\"text\":" << json_quote(excerpt.text) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_campaign(app_context_t *app,
                                            const std::string &arguments_json,
                                            std::string *out_structured,
                                            std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string campaign_cursor{};
  (void)extract_json_string_field(arguments_json, "campaign_cursor",
                                  &campaign_cursor);
  campaign_cursor = trim_ascii(campaign_cursor);
  if (campaign_cursor.empty()) {
    *out_error = "get_campaign requires arguments.campaign_cursor";
    return false;
  }

  cuwacunu::hero::runtime::runtime_campaign_record_t record{};
  if (!read_runtime_campaign(*app, campaign_cursor, &record, out_error)) {
    return false;
  }
  bool changed = false;
  if (!reconcile_campaign_record(*app, &record, &changed, out_error)) {
    return false;
  }
  *out_structured = "{\"campaign_cursor\":" + json_quote(campaign_cursor) +
                    ",\"campaign\":" + runtime_campaign_to_json(record) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_list_jobs(app_context_t *app,
                                         const std::string &arguments_json,
                                         std::string *out_structured,
                                         std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string state_filter{};
  std::size_t limit = 0;
  std::size_t offset = 0;
  bool newest_first = true;
  bool include_full = false;
  (void)extract_json_string_field(arguments_json, "state", &state_filter);
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);
  (void)extract_json_bool_field(arguments_json, "newest_first", &newest_first);
  (void)extract_json_bool_field(arguments_json, "include_full", &include_full);
  state_filter = trim_ascii(state_filter);

  std::vector<cuwacunu::hero::runtime::runtime_job_record_t> jobs{};
  if (!list_runtime_jobs(*app, &jobs, out_error))
    return false;

  struct row_t {
    cuwacunu::hero::runtime::runtime_job_record_t record{};
    cuwacunu::hero::runtime::runtime_job_observation_t observation{};
    std::string stable_state{};
  };
  std::vector<row_t> rows{};
  rows.reserve(jobs.size());
  for (auto &job : jobs) {
    bool changed = false;
    if (!reconcile_job_record(*app, &job, &changed, out_error))
      return false;
    row_t row{};
    row.record = job;
    row.observation = cuwacunu::hero::runtime::observe_runtime_job(job);
    row.stable_state = cuwacunu::hero::runtime::stable_state_for_observation(
        job, row.observation);
    if (!state_filter.empty() && row.stable_state != state_filter &&
        row.record.state != state_filter) {
      continue;
    }
    rows.push_back(std::move(row));
  }

  std::sort(rows.begin(), rows.end(),
            [newest_first](const row_t &a, const row_t &b) {
              if (a.record.started_at_ms != b.record.started_at_ms) {
                return newest_first
                           ? (a.record.started_at_ms > b.record.started_at_ms)
                           : (a.record.started_at_ms < b.record.started_at_ms);
              }
              return newest_first ? (a.record.job_cursor > b.record.job_cursor)
                                  : (a.record.job_cursor < b.record.job_cursor);
            });

  const std::size_t total = rows.size();
  const std::size_t off = std::min(offset, rows.size());
  std::size_t count = limit;
  if (count == 0)
    count = rows.size() - off;
  count = std::min(count, rows.size() - off);
  rows.assign(rows.begin() + static_cast<std::ptrdiff_t>(off),
              rows.begin() + static_cast<std::ptrdiff_t>(off + count));

  std::ostringstream jobs_json;
  jobs_json << "[";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i != 0)
      jobs_json << ",";
    jobs_json << (include_full
                      ? runtime_job_to_json(rows[i].record, rows[i].observation)
                      : runtime_job_summary_to_json(rows[i].record,
                                                    rows[i].observation));
  }
  jobs_json << "]";

  std::ostringstream out;
  out << "{\"job_cursor\":\"\""
      << ",\"count\":" << rows.size() << ",\"total\":" << total
      << ",\"state\":" << json_quote(state_filter)
      << ",\"compact\":" << (include_full ? "false" : "true")
      << ",\"jobs\":" << jobs_json.str() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_job(app_context_t *app,
                                       const std::string &arguments_json,
                                       std::string *out_structured,
                                       std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string job_cursor{};
  (void)extract_json_string_field(arguments_json, "job_cursor", &job_cursor);
  job_cursor = trim_ascii(job_cursor);
  if (job_cursor.empty()) {
    *out_error = "get_job requires arguments.job_cursor";
    return false;
  }

  cuwacunu::hero::runtime::runtime_job_record_t record{};
  if (!read_runtime_job(*app, job_cursor, &record, out_error))
    return false;
  bool changed = false;
  if (!reconcile_job_record(*app, &record, &changed, out_error))
    return false;
  const auto observation = cuwacunu::hero::runtime::observe_runtime_job(record);
  *out_structured = "{\"job_cursor\":" + json_quote(job_cursor) +
                    ",\"job\":" + runtime_job_to_json(record, observation) +
                    "}";
  return true;
}

[[nodiscard]] bool handle_tool_stop_job(app_context_t *app,
                                        const std::string &arguments_json,
                                        std::string *out_structured,
                                        std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string job_cursor{};
  bool force = false;
  (void)extract_json_string_field(arguments_json, "job_cursor", &job_cursor);
  (void)extract_json_bool_field(arguments_json, "force", &force);
  job_cursor = trim_ascii(job_cursor);
  if (job_cursor.empty()) {
    *out_error = "stop_job requires arguments.job_cursor";
    return false;
  }

  cuwacunu::hero::runtime::runtime_job_record_t record{};
  if (!read_runtime_job(*app, job_cursor, &record, out_error))
    return false;

  int rc = -1;
  const int sig = force ? SIGKILL : SIGTERM;
  bool use_process_group = false;
  bool used_runner_fallback = false;
  if (record.target_pgid.has_value() &&
      cuwacunu::hero::runtime::process_group_alive(
          *record.target_pgid, record.target_start_ticks, record.boot_id)) {
    use_process_group = true;
    rc = ::kill(-static_cast<pid_t>(*record.target_pgid), sig);
  } else if (record.target_pid.has_value() &&
             cuwacunu::hero::runtime::process_identity_alive(
                 *record.target_pid, record.target_start_ticks,
                 record.boot_id)) {
    rc = ::kill(static_cast<pid_t>(*record.target_pid), sig);
  } else if (record.runner_pid.has_value() &&
             cuwacunu::hero::runtime::process_identity_alive(
                 *record.runner_pid, record.runner_start_ticks,
                 record.boot_id) &&
             (!record.target_pid.has_value() ||
              !record.target_pgid.has_value() || record.state == "launching")) {
    used_runner_fallback = true;
    rc = ::kill(static_cast<pid_t>(*record.runner_pid), sig);
  } else {
    *out_error = "job has no live process identity metadata";
    return false;
  }
  if (rc != 0 && errno != ESRCH) {
    *out_error = "failed to signal job process";
    return false;
  }

  record.state = "stopping";
  if (use_process_group) {
    record.state_detail = force ? "sent SIGKILL to target process group"
                                : "sent SIGTERM to target process group";
  } else if (used_runner_fallback) {
    record.state_detail =
        force ? "sent SIGKILL to job runner before target publication"
              : "sent SIGTERM to job runner before target publication";
  } else {
    record.state_detail = force ? "sent SIGKILL to target process"
                                : "sent SIGTERM to target process";
  }
  record.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!write_runtime_job(*app, record, out_error))
    return false;

  const auto observation = cuwacunu::hero::runtime::observe_runtime_job(record);
  *out_structured = "{\"job_cursor\":" + json_quote(job_cursor) +
                    ",\"job\":" + runtime_job_to_json(record, observation) +
                    "}";
  return true;
}

[[nodiscard]] bool handle_tool_stop_campaign(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string campaign_cursor{};
  bool force = false;
  bool suppress_marshal_session_update = false;
  (void)extract_json_string_field(arguments_json, "campaign_cursor",
                                  &campaign_cursor);
  (void)extract_json_bool_field(arguments_json, "force", &force);
  (void)extract_json_bool_field(arguments_json,
                                "suppress_marshal_session_update",
                                &suppress_marshal_session_update);
  campaign_cursor = trim_ascii(campaign_cursor);
  if (campaign_cursor.empty()) {
    *out_error = "stop_campaign requires arguments.campaign_cursor";
    return false;
  }

  cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
  if (!read_runtime_campaign(*app, campaign_cursor, &campaign, out_error)) {
    return false;
  }
  bool campaign_changed = false;
  if (!reconcile_campaign_record(*app, &campaign, &campaign_changed,
                                 out_error)) {
    return false;
  }
  if (is_runtime_campaign_terminal_state(campaign.state)) {
    *out_structured = "{\"campaign_cursor\":" + json_quote(campaign_cursor) +
                      ",\"campaign\":" + runtime_campaign_to_json(campaign) +
                      "}";
    return true;
  }

  bool issued_stop = false;
  if (!campaign.job_cursors.empty()) {
    const std::string latest_job_cursor = campaign.job_cursors.back();
    cuwacunu::hero::runtime::runtime_job_record_t latest_job{};
    if (!read_runtime_job(*app, latest_job_cursor, &latest_job, out_error)) {
      return false;
    }
    bool job_changed = false;
    if (!reconcile_job_record(*app, &latest_job, &job_changed, out_error)) {
      return false;
    }
    const auto observation =
        cuwacunu::hero::runtime::observe_runtime_job(latest_job);
    const std::string stable_state =
        cuwacunu::hero::runtime::stable_state_for_observation(latest_job,
                                                              observation);
    if (!is_runtime_job_terminal_state(stable_state)) {
      std::string ignored{};
      const std::string stop_job_args =
          "{\"job_cursor\":" + json_quote(latest_job_cursor) +
          ",\"force\":" + std::string(force ? "true" : "false") + "}";
      if (!handle_tool_stop_job(app, stop_job_args, &ignored, out_error)) {
        return false;
      }
      issued_stop = true;
    }
  }

  if (campaign.runner_pid.has_value() &&
      cuwacunu::hero::runtime::process_identity_alive(
          *campaign.runner_pid, campaign.runner_start_ticks,
          campaign.boot_id)) {
    const int sig = force ? SIGKILL : SIGTERM;
    if (::kill(static_cast<pid_t>(*campaign.runner_pid), sig) != 0 &&
        errno != ESRCH) {
      if (out_error->empty())
        *out_error = "failed to signal campaign runner";
      return false;
    }
    issued_stop = true;
  }

  if (!issued_stop) {
    if (!reconcile_campaign_record(*app, &campaign, &campaign_changed,
                                   out_error)) {
      return false;
    }
    if (is_runtime_campaign_terminal_state(campaign.state)) {
      *out_structured = "{\"campaign_cursor\":" + json_quote(campaign_cursor) +
                        ",\"campaign\":" + runtime_campaign_to_json(campaign) +
                        "}";
      return true;
    }
    *out_error = "campaign has no live process identity metadata";
    return false;
  }

  campaign.state = "stopping";
  campaign.state_detail = force ? "sent SIGKILL to campaign runner"
                                : "sent SIGTERM to campaign runner";
  campaign.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!write_runtime_campaign(*app, campaign, out_error))
    return false;

  *out_structured = "{\"campaign_cursor\":" + json_quote(campaign_cursor) +
                    ",\"campaign\":" + runtime_campaign_to_json(campaign) + "}";
  return true;
}

[[nodiscard]] bool tail_file_lines(const std::filesystem::path &path,
                                   std::size_t lines, std::string *out,
                                   std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "tail output pointer is null";
    return false;
  }
  *out = "";
  std::error_code ec{};
  if (!std::filesystem::exists(path, ec)) {
    if (ec) {
      if (error)
        *error = "cannot access log file: " + path.string();
      return false;
    }
    return true;
  }
  std::string text{};
  if (!cuwacunu::hero::runtime::read_text_file(path, &text, error))
    return false;
  if (lines == 0) {
    *out = text;
    return true;
  }

  std::size_t count = 0;
  std::size_t pos = text.size();
  while (pos > 0) {
    --pos;
    if (text[pos] == '\n') {
      ++count;
      if (count > lines) {
        ++pos;
        break;
      }
    }
  }
  if (count <= lines)
    pos = 0;
  *out = text.substr(pos);
  return true;
}

[[nodiscard]] bool handle_tool_tail_log(app_context_t *app,
                                        const std::string &arguments_json,
                                        std::string *out_structured,
                                        std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string job_cursor{};
  std::string stream{"stdout"};
  std::size_t lines = app->defaults.tail_default_lines;
  (void)extract_json_string_field(arguments_json, "job_cursor", &job_cursor);
  (void)extract_json_string_field(arguments_json, "stream", &stream);
  (void)extract_json_size_field(arguments_json, "lines", &lines);
  job_cursor = trim_ascii(job_cursor);
  stream = lowercase_copy(trim_ascii(stream));
  if (job_cursor.empty()) {
    *out_error = "tail_log requires arguments.job_cursor";
    return false;
  }
  if (stream.empty())
    stream = "stdout";
  if (stream != "stdout" && stream != "stderr") {
    *out_error = "tail_log stream must be stdout or stderr";
    return false;
  }

  cuwacunu::hero::runtime::runtime_job_record_t record{};
  if (!read_runtime_job(*app, job_cursor, &record, out_error))
    return false;

  const std::filesystem::path log_path =
      (stream == "stderr") ? std::filesystem::path(record.stderr_path)
                           : std::filesystem::path(record.stdout_path);
  bounded_text_excerpt_t excerpt{};
  if (!read_bounded_text_excerpt(log_path, arguments_json, lines, &excerpt,
                                 out_error))
    return false;

  std::ostringstream out;
  out << "{\"job_cursor\":" << json_quote(job_cursor)
      << ",\"stream\":" << json_quote(stream)
      << ",\"requested_lines\":" << excerpt.requested_lines
      << ",\"lines\":" << excerpt.effective_lines
      << ",\"inline_line_limit\":" << excerpt.line_limit
      << ",\"max_bytes\":" << excerpt.max_bytes
      << ",\"file_bytes\":" << excerpt.file_bytes
      << ",\"returned_bytes\":" << excerpt.returned_bytes
      << ",\"returned_lines\":" << excerpt.returned_lines << ",\"truncated\":"
      << ((excerpt.truncated_by_lines || excerpt.truncated_by_bytes) ? "true"
                                                                     : "false")
      << ",\"truncated_by_lines\":"
      << (excerpt.truncated_by_lines ? "true" : "false")
      << ",\"truncated_by_bytes\":"
      << (excerpt.truncated_by_bytes ? "true" : "false")
      << ",\"text_omitted\":" << (excerpt.text_omitted ? "true" : "false")
      << ",\"path\":" << json_quote(log_path.string())
      << ",\"text\":" << json_quote(excerpt.text) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_reconcile(app_context_t *app,
                                         const std::string &arguments_json,
                                         std::string *out_structured,
                                         std::string *out_error) {
  (void)arguments_json;
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::vector<cuwacunu::hero::runtime::runtime_job_record_t> jobs{};
  if (!list_runtime_jobs(*app, &jobs, out_error))
    return false;
  std::vector<cuwacunu::hero::runtime::runtime_campaign_record_t> campaigns{};
  if (!list_runtime_campaigns(*app, &campaigns, out_error))
    return false;

  std::size_t changed_jobs = 0;
  std::size_t changed_campaigns = 0;
  std::ostringstream jobs_json;
  jobs_json << "[";
  bool first = true;
  for (auto &job : jobs) {
    bool updated = false;
    if (!reconcile_job_record(*app, &job, &updated, out_error))
      return false;
    if (updated)
      ++changed_jobs;
    const auto observation = cuwacunu::hero::runtime::observe_runtime_job(job);
    if (!first)
      jobs_json << ",";
    first = false;
    jobs_json << runtime_job_to_json(job, observation);
  }
  jobs_json << "]";

  std::ostringstream campaigns_json;
  campaigns_json << "[";
  first = true;
  for (auto &campaign : campaigns) {
    bool updated = false;
    if (!reconcile_campaign_record(*app, &campaign, &updated, out_error)) {
      return false;
    }
    if (updated)
      ++changed_campaigns;
    if (!first)
      campaigns_json << ",";
    first = false;
    campaigns_json << runtime_campaign_to_json(campaign);
  }
  campaigns_json << "]";

  std::ostringstream out;
  out << "{\"campaign_cursor\":\"\",\"job_cursor\":\"\""
      << ",\"job_count\":" << jobs.size()
      << ",\"campaign_count\":" << campaigns.size()
      << ",\"changed_jobs\":" << changed_jobs
      << ",\"changed_campaigns\":" << changed_campaigns
      << ",\"jobs\":" << jobs_json.str()
      << ",\"campaigns\":" << campaigns_json.str() << "}";
  *out_structured = out.str();
  return true;
}

} // namespace
