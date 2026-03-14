#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "hero/runtime_hero/hero_runtime_tools.h"
#include "hero/runtime_hero/runtime_job.h"
#include "piaabo/dlogs.h"

namespace {

constexpr const char* kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char* kServerName = "hero_runtime_mcp";

__attribute__((constructor(101))) void disable_terminal_logs_pre_main() {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
}

void print_help(const char* argv0) {
  std::cout << "Usage: " << argv0 << " [options]\n"
            << "Options:\n"
            << "  --global-config <path>   Global .config used to resolve [REAL_HERO].runtime_hero_dsl_filename\n"
            << "  --hero-config <path>     Explicit Runtime HERO defaults DSL\n"
            << "  --jobs-root <path>       Override jobs_root from HERO defaults DSL\n"
            << "  --tool <name>            Execute one MCP tool and exit\n"
            << "  --args-json <json>       Tool arguments JSON object (default: {})\n"
            << "  --list-tools             Human-readable tool list\n"
            << "  --list-tools-json        Print MCP tools/list JSON and exit\n"
            << "  (without --tool, server mode reads JSON-RPC messages from stdin)\n"
            << "  --help                   Show this help\n";
}

int run_job_runner(int argc, char** argv) {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);

  std::filesystem::path jobs_root{};
  std::string job_cursor{};
  std::filesystem::path worker_binary{};
  std::string job_kind{"default_board"};
  std::string config_folder{};
  std::string board_dsl_path{};
  std::string binding_id{};
  bool reset_runtime_state = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--job-runner") continue;
    if (arg == "--jobs-root" && i + 1 < argc) {
      jobs_root = argv[++i];
      continue;
    }
    if (arg == "--job-cursor" && i + 1 < argc) {
      job_cursor = argv[++i];
      continue;
    }
    if (arg == "--worker-binary" && i + 1 < argc) {
      worker_binary = argv[++i];
      continue;
    }
    if (arg == "--job-kind" && i + 1 < argc) {
      job_kind = argv[++i];
      continue;
    }
    if (arg == "--config-folder" && i + 1 < argc) {
      config_folder = argv[++i];
      continue;
    }
    if (arg == "--board-dsl" && i + 1 < argc) {
      board_dsl_path = argv[++i];
      continue;
    }
    if (arg == "--binding" && i + 1 < argc) {
      binding_id = argv[++i];
      continue;
    }
    if (arg == "--reset-runtime-state") {
      reset_runtime_state = true;
      continue;
    }
  }

  if (jobs_root.empty() || job_cursor.empty() || worker_binary.empty()) {
    return 2;
  }

  cuwacunu::hero::runtime::runtime_job_record_t record{};
  std::string error;
  if (!cuwacunu::hero::runtime::read_runtime_job_record(jobs_root, job_cursor,
                                                        &record, &error)) {
    return 1;
  }
  record.runner_pid = static_cast<std::uint64_t>(::getpid());
  std::uint64_t runner_start_ticks = 0;
  if (cuwacunu::hero::runtime::read_process_start_ticks(*record.runner_pid,
                                                        &runner_start_ticks)) {
    record.runner_start_ticks = runner_start_ticks;
  }
  if (record.boot_id.empty()) {
    record.boot_id = cuwacunu::hero::runtime::current_boot_id();
  }
  record.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  record.state = "launching";
  record.state_detail = "runner alive";
  if (!cuwacunu::hero::runtime::write_runtime_job_record(jobs_root, record,
                                                         &error)) {
    return 1;
  }

  const int stdout_fd =
      ::open(record.stdout_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0600);
  if (stdout_fd < 0) {
    record.state = "failed_to_start";
    record.state_detail = "cannot open stdout log";
    record.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    record.finished_at_ms = record.updated_at_ms;
    record.exit_code = 126;
    std::string ignored;
    (void)cuwacunu::hero::runtime::write_runtime_job_record(jobs_root, record,
                                                            &ignored);
    return 1;
  }
  const int stderr_fd =
      ::open(record.stderr_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0600);
  if (stderr_fd < 0) {
    (void)::close(stdout_fd);
    record.state = "failed_to_start";
    record.state_detail = "cannot open stderr log";
    record.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    record.finished_at_ms = record.updated_at_ms;
    record.exit_code = 126;
    std::string ignored;
    (void)cuwacunu::hero::runtime::write_runtime_job_record(jobs_root, record,
                                                            &ignored);
    return 1;
  }

  const pid_t child = ::fork();
  if (child < 0) {
    (void)::close(stdout_fd);
    (void)::close(stderr_fd);
    record.state = "failed_to_start";
    record.state_detail = "worker fork failed";
    record.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    record.finished_at_ms = record.updated_at_ms;
    record.exit_code = 127;
    std::string ignored;
    (void)cuwacunu::hero::runtime::write_runtime_job_record(jobs_root, record,
                                                            &ignored);
    return 1;
  }
  if (child == 0) {
    (void)::setpgid(0, 0);
    const int devnull = ::open("/dev/null", O_RDONLY);
    if (devnull >= 0) {
      (void)::dup2(devnull, STDIN_FILENO);
      if (devnull > STDERR_FILENO) (void)::close(devnull);
    }
    (void)::dup2(stdout_fd, STDOUT_FILENO);
    (void)::dup2(stderr_fd, STDERR_FILENO);
    if (stdout_fd > STDERR_FILENO) (void)::close(stdout_fd);
    if (stderr_fd > STDERR_FILENO) (void)::close(stderr_fd);

    std::vector<std::string> args{};
    if (job_kind == "default_board") {
      args.push_back(worker_binary.string());
      if (!config_folder.empty()) {
        args.push_back("--config-folder");
        args.push_back(config_folder);
      }
      if (!board_dsl_path.empty()) {
        args.push_back("--board-dsl");
        args.push_back(board_dsl_path);
      }
      if (!binding_id.empty()) {
        args.push_back("--binding");
        args.push_back(binding_id);
      }
      if (reset_runtime_state) {
        args.push_back("--reset-runtime-state");
      }
    } else {
      _exit(127);
    }

    std::vector<char*> exec_argv{};
    exec_argv.reserve(args.size() + 1);
    for (auto& arg : args) exec_argv.push_back(arg.data());
    exec_argv.push_back(nullptr);
    ::execv(exec_argv[0], exec_argv.data());

    std::cerr << "[runtime_job_runner] exec failed for " << worker_binary.string()
              << " errno=" << errno << "\n";
    _exit(127);
  }

  (void)::close(stdout_fd);
  (void)::close(stderr_fd);

  record.target_pid = static_cast<std::uint64_t>(child);
  record.target_pgid = static_cast<std::uint64_t>(child);
  std::uint64_t target_start_ticks = 0;
  if (!cuwacunu::hero::runtime::read_process_start_ticks(*record.target_pid,
                                                         &target_start_ticks)) {
    (void)::kill(child, SIGKILL);
    int ignored_status = 0;
    while (::waitpid(child, &ignored_status, 0) < 0 && errno == EINTR) {
    }
    record.state = "failed_to_start";
    record.state_detail = "cannot determine worker identity";
    record.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    record.finished_at_ms = record.updated_at_ms;
    record.exit_code = 127;
    std::string ignored;
    (void)cuwacunu::hero::runtime::write_runtime_job_record(jobs_root, record,
                                                            &ignored);
    return 1;
  }
  record.target_start_ticks = target_start_ticks;
  record.state = "running";
  record.state_detail = "worker process alive";
  record.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!cuwacunu::hero::runtime::write_runtime_job_record(jobs_root, record,
                                                         &error)) {
    return 1;
  }

  int status = 0;
  while (::waitpid(child, &status, 0) < 0) {
    if (errno == EINTR) continue;
    record.state = "lost";
    record.state_detail = "waitpid failed";
    record.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    record.finished_at_ms = record.updated_at_ms;
    std::string ignored;
    (void)cuwacunu::hero::runtime::write_runtime_job_record(jobs_root, record,
                                                            &ignored);
    return 1;
  }

  record.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  record.finished_at_ms = record.updated_at_ms;
  if (WIFEXITED(status)) {
    record.exit_code = static_cast<std::int64_t>(WEXITSTATUS(status));
    record.state = (*record.exit_code == 0) ? "exited" : "failed";
    record.state_detail = "worker exited";
  } else if (WIFSIGNALED(status)) {
    record.term_signal = static_cast<std::int64_t>(WTERMSIG(status));
    record.state = "signaled";
    record.state_detail = "worker terminated by signal";
  } else {
    record.state = "lost";
    record.state_detail = "worker exit status unknown";
  }
  std::string ignored;
  (void)cuwacunu::hero::runtime::write_runtime_job_record(jobs_root, record,
                                                          &ignored);
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--job-runner") {
      return run_job_runner(argc, argv);
    }
  }

  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);

  cuwacunu::hero::runtime_mcp::app_context_t app{};
  app.global_config_path = kDefaultGlobalConfigPath;
  std::filesystem::path hero_config_path{};
  std::filesystem::path jobs_root_override{};
  bool jobs_root_overridden = false;
  std::string direct_tool_name{};
  std::string direct_tool_args_json = "{}";
  bool direct_tool_mode = false;
  bool direct_tool_args_overridden = false;
  bool list_tools = false;
  bool list_tools_json = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--global-config" && i + 1 < argc) {
      app.global_config_path = argv[++i];
      continue;
    }
    if (arg == "--hero-config" && i + 1 < argc) {
      hero_config_path = argv[++i];
      continue;
    }
    if (arg == "--jobs-root" && i + 1 < argc) {
      jobs_root_override = argv[++i];
      jobs_root_overridden = true;
      continue;
    }
    if (arg == "--tool" && i + 1 < argc) {
      direct_tool_name = argv[++i];
      direct_tool_mode = true;
      continue;
    }
    if (arg == "--args-json" && i + 1 < argc) {
      direct_tool_args_json = argv[++i];
      direct_tool_args_overridden = true;
      continue;
    }
    if (arg == "--list-tools") {
      list_tools = true;
      continue;
    }
    if (arg == "--list-tools-json") {
      list_tools_json = true;
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      print_help(argv[0]);
      return 0;
    }
    std::cerr << "Unknown argument: " << arg << "\n";
    print_help(argv[0]);
    return 2;
  }

  if (direct_tool_args_overridden && !direct_tool_mode) {
    std::cerr << "--args-json requires --tool\n";
    return 2;
  }
  if (list_tools && list_tools_json) {
    std::cerr << "--list-tools and --list-tools-json are mutually exclusive\n";
    return 2;
  }
  if ((list_tools || list_tools_json) && direct_tool_mode) {
    std::cerr << "--list-tools/--list-tools-json cannot be combined with --tool\n";
    return 2;
  }
  if (direct_tool_mode) {
    direct_tool_name = cuwacunu::hero::runtime::trim_ascii(direct_tool_name);
    if (direct_tool_name.empty()) {
      std::cerr << "--tool requires a non-empty name\n";
      return 2;
    }
    direct_tool_args_json =
        cuwacunu::hero::runtime::trim_ascii(direct_tool_args_json);
    if (direct_tool_args_json.empty()) direct_tool_args_json = "{}";
    if (direct_tool_args_json.front() != '{') {
      std::cerr << "--args-json must be a JSON object\n";
      return 2;
    }
  }

  if (list_tools_json) {
    std::cout << cuwacunu::hero::runtime_mcp::build_tools_list_result_json()
              << "\n";
    return 0;
  }
  if (list_tools) {
    std::cout << cuwacunu::hero::runtime_mcp::build_tools_list_human_text();
    return 0;
  }

  if (hero_config_path.empty()) {
    hero_config_path =
        cuwacunu::hero::runtime_mcp::resolve_runtime_hero_dsl_path(
            app.global_config_path);
  }
  app.hero_config_path = hero_config_path;
  app.self_binary_path = cuwacunu::hero::runtime_mcp::current_executable_path();
  if (app.self_binary_path.empty()) {
    app.self_binary_path = argv[0];
  }

  std::string defaults_error;
  if (!cuwacunu::hero::runtime_mcp::load_runtime_defaults(
          hero_config_path, &app.defaults, &defaults_error)) {
    std::cerr << "[" << kServerName << "] " << defaults_error << "\n";
    return 2;
  }
  if (jobs_root_overridden) app.defaults.jobs_root = jobs_root_override;

  std::error_code ec{};
  std::filesystem::create_directories(app.defaults.jobs_root, ec);
  if (ec) {
    std::cerr << "[" << kServerName << "] cannot create jobs_root: "
              << app.defaults.jobs_root.string() << "\n";
    return 2;
  }

  if (direct_tool_mode) {
    std::string tool_result_json;
    std::string tool_error;
    const bool ok = cuwacunu::hero::runtime_mcp::execute_tool_json(
        direct_tool_name, direct_tool_args_json, &app, &tool_result_json,
        &tool_error);
    if (!tool_result_json.empty()) {
      std::cout << tool_result_json << "\n";
      std::cout.flush();
    }
    if (!ok && !tool_error.empty()) {
      std::cerr << "[" << kServerName << "] " << tool_error << "\n";
    }
    return ok ? 0 : 1;
  }

  if (::isatty(STDIN_FILENO) != 0) {
    std::cerr << "[" << kServerName
              << "] no --tool provided and stdin is a terminal; default mode "
                 "expects JSON-RPC input on stdin.\n";
    print_help(argv[0]);
    return 2;
  }

  cuwacunu::hero::runtime_mcp::run_jsonrpc_stdio_loop(&app);
  return 0;
}
