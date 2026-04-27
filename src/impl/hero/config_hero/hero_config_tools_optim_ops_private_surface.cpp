#include "hero_config_tools_internal.h"

#include "piaabo/dlogs.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

namespace cuwacunu::hero::mcp::detail {

struct tsodao_surface_t {
  std::filesystem::path dsl_path{};
  std::filesystem::path hidden_root{};
  std::filesystem::path hidden_archive{};
  std::filesystem::path public_keep_path{};
};

struct optim_backup_entry_t {
  std::filesystem::file_time_type mtime{};
  std::filesystem::path path{};
};

[[nodiscard]] bool
parse_optim_read_include_man_flag(std::string_view tool_name,
                                  const std::string &request_json,
                                  bool *out_include_man, int *out_error_code,
                                  std::string *out_error_message) {
  if (out_include_man == nullptr) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message =
          std::string(tool_name) + " missing include_man destination";
    }
    return false;
  }
  *out_include_man = false;
  bool parsed_include_man = false;
  if (extract_json_raw_field(request_json, "include_man", nullptr)) {
    if (!extract_json_bool_field(request_json, "include_man",
                                 out_include_man)) {
      if (out_error_code)
        *out_error_code = -32602;
      if (out_error_message) {
        *out_error_message =
            std::string(tool_name) + " include_man must be boolean";
      }
      return false;
    }
    parsed_include_man = true;
  }
  if (!parsed_include_man &&
      extract_json_bool_field(request_json, "includeMan", out_include_man)) {
    parsed_include_man = true;
  }
  (void)parsed_include_man;
  return true;
}

[[nodiscard]] bool
load_simple_dsl_lines_without_comments(const std::filesystem::path &dsl_path,
                                       std::vector<std::string> *out_lines,
                                       std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!out_lines) {
    if (out_error)
      *out_error = "missing destination for DSL lines";
    return false;
  }
  out_lines->clear();

  std::ifstream in(dsl_path);
  if (!in) {
    if (out_error)
      *out_error = "failed to read " + dsl_path.string();
    return false;
  }

  std::string line;
  bool in_block = false;
  while (std::getline(in, line)) {
    std::string work = line;
    for (;;) {
      if (in_block) {
        const auto end = work.find("*/");
        if (end == std::string::npos) {
          work.clear();
          break;
        }
        work.erase(0, end + 2);
        in_block = false;
        continue;
      }
      const auto start = work.find("/*");
      if (start == std::string::npos)
        break;
      const auto end = work.find("*/", start + 2);
      if (end == std::string::npos) {
        work.erase(start);
        in_block = true;
        break;
      }
      work.erase(start, end - start + 2);
    }
    const auto comment_pos = work.find_first_of("#;");
    if (comment_pos != std::string::npos)
      work.erase(comment_pos);
    work = trim_ascii(work);
    if (!work.empty())
      out_lines->push_back(work);
  }
  return true;
}

[[nodiscard]] std::string canonical_simple_dsl_lhs(std::string lhs) {
  for (;;) {
    const auto open = lhs.find('[');
    if (open == std::string::npos)
      break;
    const auto close = lhs.find(']', open + 1);
    if (close == std::string::npos)
      break;
    lhs.erase(open, close - open + 1);
  }
  for (;;) {
    const auto open = lhs.find('(');
    if (open == std::string::npos)
      break;
    const auto close = lhs.find(')', open + 1);
    if (close == std::string::npos)
      break;
    lhs.erase(open, close - open + 1);
  }
  const auto colon = lhs.find(':');
  if (colon != std::string::npos)
    lhs.erase(colon);
  return trim_ascii(lhs);
}

[[nodiscard]] bool read_simple_dsl_value(const std::filesystem::path &dsl_path,
                                         std::string_view key,
                                         std::string *out_value,
                                         std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!out_value) {
    if (out_error)
      *out_error = "missing destination for DSL value";
    return false;
  }
  out_value->clear();

  std::vector<std::string> lines{};
  if (!load_simple_dsl_lines_without_comments(dsl_path, &lines, out_error)) {
    return false;
  }
  for (const std::string &line : lines) {
    const auto pos = line.find('=');
    if (pos == std::string::npos)
      continue;
    const std::string lhs =
        canonical_simple_dsl_lhs(trim_ascii(line.substr(0, pos)));
    if (lhs != key)
      continue;
    std::string rhs = trim_ascii(line.substr(pos + 1));
    if (rhs.size() >= 2 && ((rhs.front() == '"' && rhs.back() == '"') ||
                            (rhs.front() == '\'' && rhs.back() == '\''))) {
      rhs = rhs.substr(1, rhs.size() - 2);
    }
    *out_value = rhs;
    return true;
  }
  if (out_error) {
    *out_error = "missing key " + std::string(key) + " in " + dsl_path.string();
  }
  return false;
}

[[nodiscard]] bool resolve_tsodao_surface(const hero_config_store_t &store,
                                          tsodao_surface_t *out_surface,
                                          std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!out_surface) {
    if (out_error)
      *out_error = "missing destination for TSODAO surface";
    return false;
  }
  *out_surface = tsodao_surface_t{};

  std::string tsodao_dsl_raw{};
  if (!read_ini_general_key(store.global_config_path(), "tsodao_dsl_filename",
                            &tsodao_dsl_raw) ||
      trim_ascii(tsodao_dsl_raw).empty()) {
    if (out_error) {
      *out_error = "GENERAL.tsodao_dsl_filename is not set in " +
                   store.global_config_path();
    }
    return false;
  }

  out_surface->dsl_path =
      resolve_path_near_config(tsodao_dsl_raw, store.global_config_path())
          .lexically_normal();

  std::string hidden_root_raw{};
  if (!read_simple_dsl_value(out_surface->dsl_path, "hidden_root",
                             &hidden_root_raw, out_error)) {
    return false;
  }
  std::string hidden_archive_raw{};
  if (!read_simple_dsl_value(out_surface->dsl_path, "hidden_archive",
                             &hidden_archive_raw, out_error)) {
    return false;
  }
  std::string public_keep_raw{};
  if (!read_simple_dsl_value(out_surface->dsl_path, "public_keep_path",
                             &public_keep_raw, out_error)) {
    return false;
  }

  out_surface->hidden_root =
      resolve_path_near_config(hidden_root_raw, out_surface->dsl_path.string())
          .lexically_normal();
  out_surface->hidden_archive =
      resolve_path_near_config(hidden_archive_raw,
                               out_surface->dsl_path.string())
          .lexically_normal();
  out_surface->public_keep_path =
      resolve_path_near_config(public_keep_raw, out_surface->dsl_path.string())
          .lexically_normal();

  if (out_surface->hidden_root.empty() || out_surface->hidden_archive.empty() ||
      out_surface->public_keep_path.empty()) {
    if (out_error) {
      *out_error = "tsodao.dsl resolved an empty hidden surface path";
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool
tsodao_hidden_payload_present(const tsodao_surface_t &surface) {
  std::error_code ec{};
  if (!std::filesystem::is_directory(surface.hidden_root, ec))
    return false;
  for (std::filesystem::recursive_directory_iterator
           it(surface.hidden_root,
              std::filesystem::directory_options::skip_permission_denied, ec),
       end;
       it != end; it.increment(ec)) {
    if (ec)
      break;
    const std::filesystem::path current = it->path().lexically_normal();
    if (current == surface.public_keep_path.lexically_normal())
      continue;
    return true;
  }
  return false;
}

[[nodiscard]] bool run_exec_capture(const std::vector<std::string> &argv,
                                    std::string *out_output, int *out_exit_code,
                                    std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (out_output)
    out_output->clear();
  if (out_exit_code)
    *out_exit_code = -1;
  if (argv.empty()) {
    if (out_error)
      *out_error = "empty argv";
    return false;
  }

  int pipe_fds[2]{-1, -1};
  if (::pipe(pipe_fds) != 0) {
    if (out_error)
      *out_error = "pipe() failed";
    return false;
  }

  const pid_t child = ::fork();
  if (child < 0) {
    ::close(pipe_fds[0]);
    ::close(pipe_fds[1]);
    if (out_error)
      *out_error = "fork() failed";
    return false;
  }

  if (child == 0) {
    ::dup2(pipe_fds[1], STDOUT_FILENO);
    ::dup2(pipe_fds[1], STDERR_FILENO);
    ::close(pipe_fds[0]);
    ::close(pipe_fds[1]);

    std::vector<char *> exec_argv{};
    exec_argv.reserve(argv.size() + 1);
    for (const auto &arg : argv) {
      exec_argv.push_back(const_cast<char *>(arg.c_str()));
    }
    exec_argv.push_back(nullptr);
    ::execv(exec_argv[0], exec_argv.data());

    log_err("execv failed for %s: %s", argv.front().c_str(),
            std::strerror(errno));
    _exit(127);
  }

  ::close(pipe_fds[1]);
  std::string output{};
  std::array<char, 4096> buffer{};
  for (;;) {
    const ssize_t got = ::read(pipe_fds[0], buffer.data(), buffer.size());
    if (got < 0) {
      if (errno == EINTR)
        continue;
      break;
    }
    if (got == 0)
      break;
    output.append(buffer.data(), static_cast<std::size_t>(got));
  }
  ::close(pipe_fds[0]);

  int status = 0;
  while (::waitpid(child, &status, 0) < 0) {
    if (errno == EINTR)
      continue;
    if (out_error)
      *out_error = "waitpid() failed";
    return false;
  }

  if (out_output)
    *out_output = output;
  if (WIFEXITED(status)) {
    if (out_exit_code)
      *out_exit_code = WEXITSTATUS(status);
    return true;
  }
  if (out_error)
    *out_error = "child terminated abnormally";
  return false;
}

[[nodiscard]] std::filesystem::path
resolve_tsodao_binary_path(const hero_config_store_t &store) {
  std::string repo_root_raw{};
  if (read_ini_general_key(store.global_config_path(), "repo_root",
                           &repo_root_raw) &&
      !trim_ascii(repo_root_raw).empty()) {
    return (resolve_path_near_config(repo_root_raw,
                                     store.global_config_path()) /
            ".build" / "tools" / "tsodao")
        .lexically_normal();
  }
  return std::filesystem::path("/cuwacunu/.build/tools/tsodao");
}

[[nodiscard]] bool
run_tsodao_command(const hero_config_store_t &store,
                   const std::vector<std::string> &command_and_args,
                   std::string *out_output, std::string *out_error) {
  if (out_error)
    out_error->clear();
  const std::filesystem::path binary = resolve_tsodao_binary_path(store);
  std::error_code ec{};
  if (!std::filesystem::exists(binary, ec) ||
      !std::filesystem::is_regular_file(binary, ec)) {
    if (out_error) {
      *out_error = "missing TSODAO binary: " + binary.string();
    }
    return false;
  }
  if (command_and_args.empty()) {
    if (out_error)
      *out_error = "missing TSODAO command";
    return false;
  }

  std::vector<std::string> argv{};
  argv.reserve(command_and_args.size() + 4);
  argv.push_back(binary.string());
  argv.push_back(command_and_args.front());
  argv.push_back("--global-config");
  argv.push_back(store.global_config_path());
  for (std::size_t i = 1; i < command_and_args.size(); ++i) {
    argv.push_back(command_and_args[i]);
  }

  std::string output{};
  int exit_code = -1;
  std::string exec_error{};
  if (!run_exec_capture(argv, &output, &exit_code, &exec_error)) {
    if (out_error) {
      *out_error =
          exec_error.empty() ? "failed to run TSODAO command" : exec_error;
    }
    return false;
  }
  if (out_output)
    *out_output = output;
  if (exit_code != 0) {
    if (out_error) {
      *out_error = "tsodao command failed (" + std::to_string(exit_code) +
                   "): " + trim_ascii(output);
    }
    return false;
  }
  return true;
}
