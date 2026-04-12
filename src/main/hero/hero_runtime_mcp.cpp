#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fcntl.h>
#include <iostream>
#include <optional>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"
#include "hero/runtime_hero/hero_runtime_tools.h"
#include "hero/runtime_hero/runtime_campaign.h"
#include "hero/runtime_hero/runtime_job.h"
#include "hero/runtime_dev_loop.h"
#include "hero/wave_contract_binding_runtime.h"
#include "iitepi/runtime_binding/runtime_binding.device_metadata.h"
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
            << "  --campaigns-root <path>  Override campaigns_root from HERO defaults DSL\n"
            << "  --tool <name>            Execute one MCP tool and exit\n"
            << "  --args-json <json>       Tool arguments JSON object (default: {})\n"
            << "  --list-tools             Human-readable tool list\n"
            << "  --list-tools-json        Print MCP tools/list JSON and exit\n"
            << "  (without --tool, server mode reads JSON-RPC messages from stdin)\n"
            << "  --help                   Show this help\n";
}

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  return cuwacunu::hero::runtime::trim_ascii(in);
}

[[nodiscard]] std::string campaign_cursor_from_job_cursor(
    std::string_view job_cursor) {
  const std::string trimmed = trim_ascii(job_cursor);
  const std::size_t marker = trimmed.find(
      cuwacunu::hero::runtime::kRuntimeJobCursorChildMarker);
  if (marker == std::string::npos || marker == 0) return {};
  return trimmed.substr(0, marker);
}

bool write_all_fd(int fd, const void* bytes, std::size_t size) {
  const char* data = reinterpret_cast<const char*>(bytes);
  std::size_t remaining = size;
  while (remaining > 0) {
    const ssize_t wrote = ::write(fd, data, remaining);
    if (wrote < 0) {
      if (errno == EINTR) continue;
      return false;
    }
    if (wrote == 0) return false;
    const auto wrote_size = static_cast<std::size_t>(wrote);
    data += wrote_size;
    remaining -= wrote_size;
  }
  return true;
}

int run_job_runner(int argc, char** argv) {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);

  std::filesystem::path campaigns_root{};
  std::string job_cursor{};
  std::filesystem::path worker_binary{};
  std::string job_kind{"campaign_run"};
  std::string global_config_path{};
  std::string campaign_dsl_path{};
  std::string binding_id{};
  std::string job_trace_path{};
  bool reset_runtime_state = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--job-runner") continue;
    if (arg == "--campaigns-root" && i + 1 < argc) {
      campaigns_root = argv[++i];
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
    if (arg == "--global-config" && i + 1 < argc) {
      global_config_path = argv[++i];
      continue;
    }
    if (arg == "--campaign-dsl" && i + 1 < argc) {
      campaign_dsl_path = argv[++i];
      continue;
    }
    if (arg == "--binding" && i + 1 < argc) {
      binding_id = argv[++i];
      continue;
    }
    if (arg == "--job-trace" && i + 1 < argc) {
      job_trace_path = argv[++i];
      continue;
    }
    if (arg == "--reset-runtime-state") {
      reset_runtime_state = true;
      continue;
    }
  }

  if (campaigns_root.empty() || job_cursor.empty() || worker_binary.empty()) {
    return 2;
  }

  cuwacunu::hero::runtime::runtime_job_record_t record{};
  std::string error;
  if (!cuwacunu::hero::runtime::read_runtime_job_record(campaigns_root, job_cursor,
                                                        &record, &error)) {
    return 1;
  }
  if (global_config_path.empty()) {
    global_config_path = record.global_config_path;
  }
  if (job_trace_path.empty()) {
    job_trace_path = record.trace_path;
  }
  if (job_trace_path.empty()) {
    job_trace_path =
        cuwacunu::hero::runtime::runtime_job_trace_path(campaigns_root, job_cursor)
            .string();
  }
  if (record.trace_path.empty()) {
    record.trace_path = job_trace_path;
  }
  if (global_config_path.empty()) {
    return 2;
  }
  cuwacunu::iitepi::config_space_t::change_config_file(global_config_path.c_str());
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
  if (!cuwacunu::hero::runtime::write_runtime_job_record(campaigns_root, record,
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
    (void)cuwacunu::hero::runtime::write_runtime_job_record(campaigns_root, record,
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
    (void)cuwacunu::hero::runtime::write_runtime_job_record(campaigns_root, record,
                                                            &ignored);
    return 1;
  }

  if (reset_runtime_state) {
    cuwacunu::hero::runtime_dev::runtime_reset_targets_t reset_targets{};
    if (!cuwacunu::hero::runtime_dev::resolve_runtime_reset_targets_from_active_config(
            &reset_targets, &error)) {
      const std::string log_line =
          "[runtime_job_runner] runtime reset target resolution failed: " +
          error + "\n";
      (void)write_all_fd(stderr_fd, log_line.data(), log_line.size());
      (void)::close(stdout_fd);
      (void)::close(stderr_fd);
      record.state = "failed_to_start";
      record.state_detail = "runtime reset target resolution failed";
      record.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      record.finished_at_ms = record.updated_at_ms;
      record.exit_code = 126;
      std::string ignored;
      (void)cuwacunu::hero::runtime::write_runtime_job_record(campaigns_root,
                                                              record, &ignored);
      return 1;
    }
    (void)cuwacunu::hero::runtime_dev::strip_control_plane_store_roots(
        &reset_targets);
    reset_targets.store_roots.erase(
        std::remove(reset_targets.store_roots.begin(),
                    reset_targets.store_roots.end(),
                    reset_targets.runtime_campaigns_root),
        reset_targets.store_roots.end());
    cuwacunu::hero::runtime_dev::runtime_reset_result_t reset_result{};
    std::string reset_error{};
    std::vector<std::string> ignored_campaign_cursors{};
    const std::string campaign_cursor =
        campaign_cursor_from_job_cursor(record.job_cursor);
    if (!campaign_cursor.empty()) {
      ignored_campaign_cursors.push_back(campaign_cursor);
    }
    if (!cuwacunu::hero::runtime_dev::reset_runtime_state(
            reset_targets, &reset_result, &reset_error,
            ignored_campaign_cursors, {record.job_cursor})) {
      const std::string log_line =
          "[runtime_job_runner] runtime reset failed: " + reset_error + "\n";
      (void)write_all_fd(stderr_fd, log_line.data(), log_line.size());
      (void)::close(stdout_fd);
      (void)::close(stderr_fd);
      record.state = "failed_to_start";
      record.state_detail = "runtime reset failed";
      record.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      record.finished_at_ms = record.updated_at_ms;
      record.exit_code = 126;
      std::string ignored;
      (void)cuwacunu::hero::runtime::write_runtime_job_record(campaigns_root,
                                                              record, &ignored);
      return 1;
    }
    reset_runtime_state = false;
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
    (void)cuwacunu::hero::runtime::write_runtime_job_record(campaigns_root, record,
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
    if (job_kind == "campaign_run") {
      args.push_back(worker_binary.string());
      args.push_back("--global-config");
      args.push_back(global_config_path);
      args.push_back("--campaigns-root");
      args.push_back(campaigns_root.string());
      args.push_back("--job-cursor");
      args.push_back(job_cursor);
      if (!campaign_dsl_path.empty()) {
        args.push_back("--campaign-dsl");
        args.push_back(campaign_dsl_path);
      }
      if (!binding_id.empty()) {
        args.push_back("--binding");
        args.push_back(binding_id);
      }
      if (!job_trace_path.empty()) {
        args.push_back("--job-trace");
        args.push_back(job_trace_path);
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
    (void)cuwacunu::hero::runtime::write_runtime_job_record(campaigns_root, record,
                                                            &ignored);
    return 1;
  }
  record.target_start_ticks = target_start_ticks;
  record.state = "running";
  record.state_detail = "worker process alive";
  record.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!cuwacunu::hero::runtime::write_runtime_job_record(campaigns_root, record,
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
    (void)cuwacunu::hero::runtime::write_runtime_job_record(campaigns_root, record,
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
  (void)cuwacunu::hero::runtime::write_runtime_job_record(campaigns_root, record,
                                                          &ignored);
  return 0;
}

[[nodiscard]] bool spawn_detached_job_runner(
    const std::filesystem::path& self_binary,
    const std::filesystem::path& campaigns_root,
    cuwacunu::hero::runtime::runtime_job_record_t* record,
    std::string* error) {
  if (error) error->clear();
  if (!record) {
    if (error) *error = "runtime job record pointer is null";
    return false;
  }

  int pipe_fds[2]{-1, -1};
#ifdef O_CLOEXEC
  if (::pipe2(pipe_fds, O_CLOEXEC) != 0) {
    if (error) *error = "pipe2 failed";
    return false;
  }
#else
  if (::pipe(pipe_fds) != 0) {
    if (error) *error = "pipe failed";
    return false;
  }
  (void)::fcntl(pipe_fds[0], F_SETFD, FD_CLOEXEC);
  (void)::fcntl(pipe_fds[1], F_SETFD, FD_CLOEXEC);
#endif

  const pid_t child = ::fork();
  if (child < 0) {
    (void)::close(pipe_fds[0]);
    (void)::close(pipe_fds[1]);
    if (error) *error = "fork failed";
    return false;
  }
  if (child == 0) {
    (void)::close(pipe_fds[0]);
    const int devnull = ::open("/dev/null", O_RDWR);
    if (devnull >= 0) {
      (void)::dup2(devnull, STDIN_FILENO);
      (void)::dup2(devnull, STDOUT_FILENO);
      (void)::dup2(devnull, STDERR_FILENO);
      if (devnull > STDERR_FILENO) (void)::close(devnull);
    }

    std::vector<std::string> args{};
    args.push_back(self_binary.string());
    args.push_back("--job-runner");
    args.push_back("--campaigns-root");
    args.push_back(campaigns_root.string());
    args.push_back("--job-cursor");
    args.push_back(record->job_cursor);
    args.push_back("--worker-binary");
    args.push_back(record->worker_binary);
    args.push_back("--job-kind");
    args.push_back(record->job_kind);
    args.push_back("--global-config");
    args.push_back(record->global_config_path);
    if (!record->campaign_dsl_path.empty()) {
      args.push_back("--campaign-dsl");
      args.push_back(record->campaign_dsl_path);
    }
    if (!record->binding_id.empty()) {
      args.push_back("--binding");
      args.push_back(record->binding_id);
    }
    if (!record->trace_path.empty()) {
      args.push_back("--job-trace");
      args.push_back(record->trace_path);
    }
    if (record->reset_runtime_state) {
      args.push_back("--reset-runtime-state");
    }

    std::vector<char*> argv{};
    argv.reserve(args.size() + 1);
    for (auto& arg : args) argv.push_back(arg.data());
    argv.push_back(nullptr);
    ::execv(argv[0], argv.data());

    int child_errno = errno;
    (void)write_all_fd(pipe_fds[1], &child_errno, sizeof(child_errno));
    _exit(127);
  }

  (void)::close(pipe_fds[1]);
  int exec_errno = 0;
  const ssize_t n = ::read(pipe_fds[0], &exec_errno, sizeof(exec_errno));
  (void)::close(pipe_fds[0]);
  if (n > 0) {
    if (error) {
      *error = "cannot exec detached runtime job runner: errno=" +
               std::to_string(exec_errno);
    }
    return false;
  }

  record->runner_pid = static_cast<std::uint64_t>(child);
  std::uint64_t runner_start_ticks = 0;
  if (!cuwacunu::hero::runtime::read_process_start_ticks(*record->runner_pid,
                                                         &runner_start_ticks)) {
    if (error) *error = "cannot determine runtime job runner identity";
    return false;
  }
  record->runner_start_ticks = runner_start_ticks;
  record->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  return cuwacunu::hero::runtime::write_runtime_job_record(campaigns_root, *record,
                                                           error);
}

int run_campaign_runner(int argc, char** argv) {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);

  std::filesystem::path campaigns_root{};
  std::string campaign_cursor{};
  std::filesystem::path worker_binary{};
  std::string global_config_path{};

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--campaign-runner") continue;
    if (arg == "--campaigns-root" && i + 1 < argc) {
      campaigns_root = argv[++i];
      continue;
    }
    if (arg == "--campaign-cursor" && i + 1 < argc) {
      campaign_cursor = argv[++i];
      continue;
    }
    if (arg == "--worker-binary" && i + 1 < argc) {
      worker_binary = argv[++i];
      continue;
    }
    if (arg == "--global-config" && i + 1 < argc) {
      global_config_path = argv[++i];
      continue;
    }
  }

  if (campaigns_root.empty() || campaign_cursor.empty() || worker_binary.empty()) {
    return 2;
  }

  std::string error{};
  cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
  if (!cuwacunu::hero::runtime::read_runtime_campaign_record(
          campaigns_root, campaign_cursor, &campaign, &error)) {
    return 1;
  }
  if (global_config_path.empty()) {
    global_config_path = campaign.global_config_path;
  }
  if (global_config_path.empty()) {
    return 2;
  }
  cuwacunu::iitepi::config_space_t::change_config_file(global_config_path.c_str());

  const int stdout_fd =
      ::open(campaign.stdout_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0600);
  if (stdout_fd >= 0) {
    (void)::dup2(stdout_fd, STDOUT_FILENO);
    if (stdout_fd > STDERR_FILENO) (void)::close(stdout_fd);
  }
  const int stderr_fd =
      ::open(campaign.stderr_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0600);
  if (stderr_fd >= 0) {
    (void)::dup2(stderr_fd, STDERR_FILENO);
    if (stderr_fd > STDERR_FILENO) (void)::close(stderr_fd);
  }

  campaign.runner_pid = static_cast<std::uint64_t>(::getpid());
  std::uint64_t runner_start_ticks = 0;
  if (cuwacunu::hero::runtime::read_process_start_ticks(*campaign.runner_pid,
                                                        &runner_start_ticks)) {
    campaign.runner_start_ticks = runner_start_ticks;
  }
  if (campaign.boot_id.empty()) {
    campaign.boot_id = cuwacunu::hero::runtime::current_boot_id();
  }
  campaign.state = "running";
  campaign.state_detail = "campaign runner alive";
  campaign.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  if (!cuwacunu::hero::runtime::write_runtime_campaign_record(
          campaigns_root, campaign, &error)) {
    return 1;
  }

  const std::size_t start_index = campaign.job_cursors.size();
  for (std::size_t i = start_index; i < campaign.run_bind_ids.size(); ++i) {
    const std::string& bind_id = campaign.run_bind_ids[i];
    campaign.current_run_index = static_cast<std::uint64_t>(i);
    campaign.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    campaign.state = "running";
    campaign.state_detail = "dispatching bind " + bind_id;
    if (!cuwacunu::hero::runtime::write_runtime_campaign_record(
            campaigns_root, campaign, &error)) {
      return 1;
    }

    const std::string job_cursor =
        cuwacunu::hero::runtime::make_job_cursor(
            campaigns_root, campaign.campaign_cursor, i);
    if (job_cursor.empty()) {
      error = "failed to allocate compact job cursor for bind " + bind_id;
      campaign.state = "failed";
      campaign.state_detail = error;
      campaign.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      campaign.updated_at_ms = *campaign.finished_at_ms;
      (void)cuwacunu::hero::runtime::write_runtime_campaign_record(campaigns_root,
                                                                   campaign, nullptr);
      return 1;
    }
    cuwacunu::hero::wave_contract_binding_runtime::
        resolved_wave_contract_binding_snapshot_t snapshot{};
    if (!cuwacunu::hero::wave_contract_binding_runtime::
            prepare_campaign_binding_snapshot(
                campaign.campaign_dsl_path, bind_id,
                cuwacunu::hero::runtime::runtime_job_dir(campaigns_root, job_cursor),
                &snapshot, &error)) {
      campaign.state = "failed";
      campaign.state_detail = error;
      campaign.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      campaign.updated_at_ms = *campaign.finished_at_ms;
      (void)cuwacunu::hero::runtime::write_runtime_campaign_record(
          campaigns_root, campaign, nullptr);
      return 1;
    }

    cuwacunu::hero::runtime::runtime_job_record_t job{};
    job.job_cursor = job_cursor;
    job.boot_id = campaign.boot_id;
    job.state = "launching";
    job.state_detail = "detached job runner requested";
    job.worker_binary = worker_binary.string();
    job.worker_command =
        worker_binary.string() + " --global-config " + global_config_path +
        " --campaigns-root " + campaigns_root.string() + " --job-cursor " +
        job_cursor +
        " --campaign-dsl " + snapshot.campaign_dsl_path + " --binding " +
        snapshot.binding_id;
    job.global_config_path = global_config_path;
    job.source_campaign_dsl_path = snapshot.source_scope_dsl_path;
    job.source_contract_dsl_path = snapshot.source_contract_dsl_path;
    job.source_wave_dsl_path = snapshot.source_wave_dsl_path;
    job.campaign_dsl_path = snapshot.campaign_dsl_path;
    job.contract_dsl_path = snapshot.contract_dsl_path;
    job.wave_dsl_path = snapshot.wave_dsl_path;
    job.binding_id = snapshot.binding_id;
    const auto device_metadata =
        tsiemene::resolve_runtime_binding_preferred_device_metadata_for_contract_path(
            std::filesystem::path(snapshot.contract_dsl_path));
    job.requested_device = device_metadata.configured_device;
    job.resolved_device = device_metadata.resolved_device;
    job.device_source_section = device_metadata.source_section;
    job.device_contract_hash = device_metadata.contract_hash;
    job.device_error = device_metadata.error;
    job.cuda_required = device_metadata.cuda_required;
    job.reset_runtime_state = campaign.reset_runtime_state && (i == 0);
    job.stdout_path =
        cuwacunu::hero::runtime::runtime_job_stdout_path(campaigns_root, job_cursor)
            .string();
    job.stderr_path =
        cuwacunu::hero::runtime::runtime_job_stderr_path(campaigns_root, job_cursor)
            .string();
    job.trace_path =
        cuwacunu::hero::runtime::runtime_job_trace_path(campaigns_root, job_cursor)
            .string();
    if (!job.trace_path.empty()) {
      job.worker_command += " --job-trace " + job.trace_path;
    }
    job.started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    job.updated_at_ms = job.started_at_ms;
    if (!cuwacunu::hero::runtime::write_runtime_job_record(campaigns_root, job,
                                                           &error)) {
      campaign.state = "failed";
      campaign.state_detail = error;
      campaign.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      campaign.updated_at_ms = *campaign.finished_at_ms;
      (void)cuwacunu::hero::runtime::write_runtime_campaign_record(
          campaigns_root, campaign, nullptr);
      return 1;
    }
    if (!spawn_detached_job_runner(std::filesystem::path(argv[0]), campaigns_root,
                                   &job,
                                   &error)) {
      job.state = "failed_to_start";
      job.state_detail = error;
      job.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      job.updated_at_ms = *job.finished_at_ms;
      job.exit_code = 127;
      (void)cuwacunu::hero::runtime::write_runtime_job_record(campaigns_root, job,
                                                              nullptr);
      campaign.state = "failed";
      campaign.state_detail = error;
      campaign.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
      campaign.updated_at_ms = *campaign.finished_at_ms;
      (void)cuwacunu::hero::runtime::write_runtime_campaign_record(
          campaigns_root, campaign, nullptr);
      return 1;
    }

    campaign.job_cursors.push_back(job_cursor);
    campaign.updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
    if (!cuwacunu::hero::runtime::write_runtime_campaign_record(
            campaigns_root, campaign, &error)) {
      return 1;
    }

    for (;;) {
      cuwacunu::hero::runtime::runtime_job_record_t observed{};
      if (!cuwacunu::hero::runtime::read_runtime_job_record(campaigns_root, job_cursor,
                                                            &observed, &error)) {
        campaign.state = "failed";
        campaign.state_detail = error;
        campaign.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
        campaign.updated_at_ms = *campaign.finished_at_ms;
        (void)cuwacunu::hero::runtime::write_runtime_campaign_record(
            campaigns_root, campaign, nullptr);
        return 1;
      }
      const auto observation =
          cuwacunu::hero::runtime::observe_runtime_job(observed);
      const std::string stable =
          cuwacunu::hero::runtime::stable_state_for_observation(observed,
                                                                observation);
      if (stable == "launching" || stable == "running" || stable == "stopping" ||
          stable == "orphaned") {
        ::usleep(100 * 1000);
        continue;
      }
      if (stable != "exited" || !observed.exit_code.has_value() ||
          *observed.exit_code != 0) {
        campaign.state = "failed";
        campaign.state_detail = "run failed for bind " + bind_id;
        campaign.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
        campaign.updated_at_ms = *campaign.finished_at_ms;
        (void)cuwacunu::hero::runtime::write_runtime_campaign_record(
            campaigns_root, campaign, nullptr);
        return 1;
      }
      break;
    }
  }

  campaign.state = "exited";
  campaign.state_detail = "campaign completed";
  campaign.finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  campaign.updated_at_ms = *campaign.finished_at_ms;
  campaign.current_run_index =
      static_cast<std::uint64_t>(campaign.run_bind_ids.size());
  (void)cuwacunu::hero::runtime::write_runtime_campaign_record(
      campaigns_root, campaign, nullptr);
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--job-runner") {
      return run_job_runner(argc, argv);
    }
    if (std::string(argv[i]) == "--campaign-runner") {
      return run_campaign_runner(argc, argv);
    }
  }

  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);

  cuwacunu::hero::runtime_mcp::app_context_t app{};
  app.global_config_path = kDefaultGlobalConfigPath;
  std::filesystem::path hero_config_path{};
  std::filesystem::path campaigns_root_override{};
  bool campaigns_root_overridden = false;
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
    if (arg == "--campaigns-root" && i + 1 < argc) {
      campaigns_root_override = argv[++i];
      campaigns_root_overridden = true;
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
  if (hero_config_path.empty()) {
    std::cerr << "[" << kServerName
              << "] missing [REAL_HERO].runtime_hero_dsl_filename in "
              << app.global_config_path.string() << "\n";
    return 2;
  }
  app.hero_config_path = hero_config_path;
  app.self_binary_path = cuwacunu::hero::runtime_mcp::current_executable_path();
  if (app.self_binary_path.empty()) {
    app.self_binary_path = argv[0];
  }

  std::string defaults_error;
  if (!cuwacunu::hero::runtime_mcp::load_runtime_defaults(
          hero_config_path, app.global_config_path, &app.defaults,
          &defaults_error)) {
    std::cerr << "[" << kServerName << "] " << defaults_error << "\n";
    return 2;
  }
  if (campaigns_root_overridden) {
    app.defaults.campaigns_root = campaigns_root_override;
  }

  std::error_code ec{};
  std::filesystem::create_directories(app.defaults.campaigns_root, ec);
  if (ec) {
    std::cerr << "[" << kServerName << "] cannot create campaigns_root: "
              << app.defaults.campaigns_root.string() << "\n";
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
    if (!ok) return 2;
    return cuwacunu::hero::runtime_mcp::tool_result_is_error(tool_result_json)
               ? 1
               : 0;
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
