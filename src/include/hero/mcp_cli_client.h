// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <string_view>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "hero/config_path_defaults.h"

namespace cuwacunu::hero::mcp_cli {

struct process_result_t {
  int exit_code{-1};
  bool signaled{false};
  int signal_number{0};
  bool timed_out{false};
  bool stdout_truncated{false};
  bool stderr_truncated{false};
  std::string stdout_text{};
  std::string stderr_text{};
};

struct tool_call_result_t {
  bool attempted{false};
  bool process_ok{false};
  process_result_t process{};
  std::string result_json{};
  std::string error_message{};
};

[[nodiscard]] inline std::string trim_ascii(std::string_view in) {
  std::size_t begin = 0;
  while (begin < in.size() &&
         std::isspace(static_cast<unsigned char>(in[begin])) != 0) {
    ++begin;
  }
  std::size_t end = in.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(in[end - 1])) != 0) {
    --end;
  }
  return std::string(in.substr(begin, end - begin));
}

[[nodiscard]] inline bool tool_result_is_error(std::string_view result_json) {
  return result_json.find("\"isError\":true") != std::string_view::npos ||
         result_json.find("\"isError\": true") != std::string_view::npos;
}

[[nodiscard]] inline std::string
extract_json_object_from_output(std::string_view text) {
  for (std::size_t begin = 0; begin < text.size(); ++begin) {
    if (text[begin] != '{') {
      continue;
    }
    std::size_t depth = 0;
    bool in_string = false;
    bool escaped = false;
    for (std::size_t pos = begin; pos < text.size(); ++pos) {
      const char c = text[pos];
      if (in_string) {
        if (escaped) {
          escaped = false;
          continue;
        }
        if (c == '\\') {
          escaped = true;
          continue;
        }
        if (c == '"') {
          in_string = false;
        }
        continue;
      }
      if (c == '"') {
        in_string = true;
        continue;
      }
      if (c == '{') {
        ++depth;
        continue;
      }
      if (c == '}') {
        if (depth == 0) {
          break;
        }
        --depth;
        if (depth == 0) {
          return std::string(text.substr(begin, pos - begin + 1U));
        }
      }
    }
  }
  return trim_ascii(text);
}

[[nodiscard]] inline std::filesystem::path default_runtime_mcp_path() {
  return cuwacunu::hero::config_paths::default_hero_runtime_mcp_path();
}

[[nodiscard]] inline std::filesystem::path default_environment_mcp_path() {
  return cuwacunu::hero::config_paths::default_hero_environment_mcp_path();
}

namespace detail {

inline void append_capped(std::string *out, const char *data, std::size_t size,
                          std::size_t cap, bool *truncated) {
  if (out == nullptr) {
    return;
  }
  if (out->size() < cap) {
    const std::size_t room = cap - out->size();
    out->append(data, std::min(room, size));
  }
  if (size > 0 && out->size() >= cap && truncated != nullptr) {
    *truncated = true;
  }
}

inline void set_nonblocking(int fd) {
  const int flags = ::fcntl(fd, F_GETFL, 0);
  if (flags >= 0) {
    (void)::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }
}

inline void drain_fd(int fd, std::string *out, std::size_t cap, bool *truncated,
                     bool *open) {
  std::array<char, 4096> buffer{};
  while (true) {
    const ssize_t n = ::read(fd, buffer.data(), buffer.size());
    if (n > 0) {
      append_capped(out, buffer.data(), static_cast<std::size_t>(n), cap,
                    truncated);
      continue;
    }
    if (n == 0) {
      *open = false;
      (void)::close(fd);
      return;
    }
    if (errno == EINTR) {
      continue;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return;
    }
    *open = false;
    (void)::close(fd);
    return;
  }
}

} // namespace detail

[[nodiscard]] inline process_result_t
run_process(const std::vector<std::string> &argv, int timeout_seconds,
            std::size_t capture_cap) {
  process_result_t result{};
  if (argv.empty()) {
    result.stderr_text = "empty argv";
    return result;
  }
  if (capture_cap == 0) {
    capture_cap = 65536;
  }
  int stdout_pipe[2]{-1, -1};
  int stderr_pipe[2]{-1, -1};
  if (::pipe(stdout_pipe) != 0 || ::pipe(stderr_pipe) != 0) {
    result.stderr_text = "pipe failed";
    if (stdout_pipe[0] >= 0) {
      (void)::close(stdout_pipe[0]);
    }
    if (stdout_pipe[1] >= 0) {
      (void)::close(stdout_pipe[1]);
    }
    if (stderr_pipe[0] >= 0) {
      (void)::close(stderr_pipe[0]);
    }
    if (stderr_pipe[1] >= 0) {
      (void)::close(stderr_pipe[1]);
    }
    return result;
  }

  const pid_t pid = ::fork();
  if (pid < 0) {
    result.stderr_text = "fork failed";
    (void)::close(stdout_pipe[0]);
    (void)::close(stdout_pipe[1]);
    (void)::close(stderr_pipe[0]);
    (void)::close(stderr_pipe[1]);
    return result;
  }
  if (pid == 0) {
    (void)::dup2(stdout_pipe[1], STDOUT_FILENO);
    (void)::dup2(stderr_pipe[1], STDERR_FILENO);
    (void)::close(stdout_pipe[0]);
    (void)::close(stdout_pipe[1]);
    (void)::close(stderr_pipe[0]);
    (void)::close(stderr_pipe[1]);
    std::vector<char *> cargv;
    cargv.reserve(argv.size() + 1U);
    for (const std::string &arg : argv) {
      cargv.push_back(const_cast<char *>(arg.c_str()));
    }
    cargv.push_back(nullptr);
    ::execv(cargv[0], cargv.data());
    const std::string err =
        std::string("execv failed: ") + std::strerror(errno) + "\n";
    const ssize_t ignored = ::write(STDERR_FILENO, err.data(), err.size());
    (void)ignored;
    _exit(127);
  }

  (void)::close(stdout_pipe[1]);
  (void)::close(stderr_pipe[1]);
  detail::set_nonblocking(stdout_pipe[0]);
  detail::set_nonblocking(stderr_pipe[0]);
  bool stdout_open = true;
  bool stderr_open = true;
  bool exited = false;
  int status = 0;
  const auto start = std::chrono::steady_clock::now();

  while (stdout_open || stderr_open || !exited) {
    if (!exited) {
      const pid_t waited = ::waitpid(pid, &status, WNOHANG);
      if (waited == pid) {
        exited = true;
      }
    }
    if (!exited && timeout_seconds > 0) {
      const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::steady_clock::now() - start);
      if (elapsed.count() > timeout_seconds) {
        result.timed_out = true;
        (void)::kill(pid, SIGKILL);
        (void)::waitpid(pid, &status, 0);
        exited = true;
      }
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    int maxfd = -1;
    if (stdout_open) {
      FD_SET(stdout_pipe[0], &readfds);
      maxfd = std::max(maxfd, stdout_pipe[0]);
    }
    if (stderr_open) {
      FD_SET(stderr_pipe[0], &readfds);
      maxfd = std::max(maxfd, stderr_pipe[0]);
    }
    if (maxfd >= 0) {
      timeval tv{};
      tv.tv_sec = 0;
      tv.tv_usec = 100000;
      const int ready = ::select(maxfd + 1, &readfds, nullptr, nullptr, &tv);
      if (ready > 0) {
        if (stdout_open && FD_ISSET(stdout_pipe[0], &readfds)) {
          detail::drain_fd(stdout_pipe[0], &result.stdout_text, capture_cap,
                           &result.stdout_truncated, &stdout_open);
        }
        if (stderr_open && FD_ISSET(stderr_pipe[0], &readfds)) {
          detail::drain_fd(stderr_pipe[0], &result.stderr_text, capture_cap,
                           &result.stderr_truncated, &stderr_open);
        }
      }
    } else if (exited) {
      break;
    }
  }
  if (stdout_open) {
    detail::drain_fd(stdout_pipe[0], &result.stdout_text, capture_cap,
                     &result.stdout_truncated, &stdout_open);
  }
  if (stderr_open) {
    detail::drain_fd(stderr_pipe[0], &result.stderr_text, capture_cap,
                     &result.stderr_truncated, &stderr_open);
  }
  if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    result.signaled = true;
    result.signal_number = WTERMSIG(status);
    result.exit_code = 128 + result.signal_number;
  }
  return result;
}

[[nodiscard]] inline tool_call_result_t
call_tool(const std::filesystem::path &mcp_path,
          const std::filesystem::path &global_config_path,
          const std::filesystem::path &policy_path,
          const std::string &tool_name, const std::string &arguments_json,
          int timeout_seconds = 600, std::size_t capture_cap = 1048576) {
  tool_call_result_t out{};
  out.attempted = true;
  if (::access(mcp_path.c_str(), X_OK) != 0) {
    out.error_message = "MCP binary is not executable: " + mcp_path.string();
    return out;
  }
  std::vector<std::string> argv{mcp_path.string()};
  if (!global_config_path.empty()) {
    argv.push_back("--global-config");
    argv.push_back(global_config_path.string());
  }
  if (!policy_path.empty()) {
    argv.push_back("--config");
    argv.push_back(policy_path.string());
  }
  argv.push_back("--tool");
  argv.push_back(tool_name);
  argv.push_back("--args-json");
  argv.push_back(arguments_json.empty() ? "{}" : arguments_json);
  out.process = run_process(argv, timeout_seconds, capture_cap);
  out.result_json = extract_json_object_from_output(out.process.stdout_text);
  out.process_ok = out.process.exit_code == 0 && !out.process.timed_out &&
                   !out.process.signaled && !out.result_json.empty();
  out.error_message = trim_ascii(out.process.stderr_text);
  if (out.error_message.empty()) {
    if (out.process.timed_out) {
      out.error_message = "MCP tool call timed out";
    } else if (out.process.signaled) {
      out.error_message = "MCP tool call signaled: " +
                          std::to_string(out.process.signal_number);
    } else if (out.process.exit_code != 0) {
      out.error_message = "MCP tool call exited with code " +
                          std::to_string(out.process.exit_code);
    } else if (out.result_json.empty()) {
      out.error_message = "MCP tool call produced empty stdout";
    }
  }
  return out;
}

[[nodiscard]] inline tool_call_result_t
call_runtime_tool(const std::filesystem::path &global_config_path,
                  const std::filesystem::path &policy_path,
                  const std::string &tool_name,
                  const std::string &arguments_json, int timeout_seconds = 600,
                  std::size_t capture_cap = 1048576) {
  return call_tool(default_runtime_mcp_path(), global_config_path, policy_path,
                   tool_name, arguments_json, timeout_seconds, capture_cap);
}

[[nodiscard]] inline tool_call_result_t call_runtime_replay_delegate(
    const std::filesystem::path &global_config_path,
    const std::filesystem::path &policy_path, const std::string &arguments_json,
    int timeout_seconds = 600, std::size_t capture_cap = 1048576) {
  tool_call_result_t out{};
  out.attempted = true;
  const auto mcp_path = default_runtime_mcp_path();
  if (::access(mcp_path.c_str(), X_OK) != 0) {
    out.error_message = "MCP binary is not executable: " + mcp_path.string();
    return out;
  }
  std::vector<std::string> argv{mcp_path.string()};
  if (!global_config_path.empty()) {
    argv.push_back("--global-config");
    argv.push_back(global_config_path.string());
  }
  if (!policy_path.empty()) {
    argv.push_back("--config");
    argv.push_back(policy_path.string());
  }
  argv.push_back("--runtime-replay-delegate");
  argv.push_back("--args-json");
  argv.push_back(arguments_json.empty() ? "{}" : arguments_json);
  out.process = run_process(argv, timeout_seconds, capture_cap);
  out.result_json = extract_json_object_from_output(out.process.stdout_text);
  out.process_ok = out.process.exit_code == 0 && !out.process.timed_out &&
                   !out.process.signaled && !out.result_json.empty();
  out.error_message = trim_ascii(out.process.stderr_text);
  if (out.error_message.empty()) {
    if (out.process.timed_out) {
      out.error_message = "Runtime replay delegate timed out";
    } else if (out.process.signaled) {
      out.error_message = "Runtime replay delegate signaled: " +
                          std::to_string(out.process.signal_number);
    } else if (out.process.exit_code != 0) {
      out.error_message = "Runtime replay delegate exited with code " +
                          std::to_string(out.process.exit_code);
    } else if (out.result_json.empty()) {
      out.error_message = "Runtime replay delegate produced empty stdout";
    }
  }
  return out;
}

[[nodiscard]] inline tool_call_result_t call_environment_tool(
    const std::filesystem::path &global_config_path,
    const std::filesystem::path &policy_path, const std::string &tool_name,
    const std::string &arguments_json, int timeout_seconds = 600,
    std::size_t capture_cap = 1048576) {
  return call_tool(default_environment_mcp_path(), global_config_path,
                   policy_path, tool_name, arguments_json, timeout_seconds,
                   capture_cap);
}

} // namespace cuwacunu::hero::mcp_cli
