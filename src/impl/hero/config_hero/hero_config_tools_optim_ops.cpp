#include "hero_config_tools_internal.h"

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

    const std::string msg = "execv failed for " + argv.front() + ": " +
                            std::string(std::strerror(errno)) + "\n";
    (void)write_all_fd(STDERR_FILENO, msg.data(), msg.size());
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

[[nodiscard]] bool handle_tool_optim_read(std::string_view tool_name,
                                          const std::string &request_json,
                                          hero_config_store_t *store,
                                          std::string *out_result_json,
                                          int *out_error_code,
                                          std::string *out_error_message) {
  bool include_man = false;
  if (!parse_optim_read_include_man_flag(tool_name, request_json, &include_man,
                                         out_error_code, out_error_message)) {
    return false;
  }
  std::string path_raw{};
  if (!extract_json_string_field(request_json, "path", &path_raw) ||
      path_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }

  tsodao_surface_t surface{};
  std::string err{};
  if (!resolve_tsodao_surface(*store, &surface, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  const bool payload_present = tsodao_hidden_payload_present(surface);
  const bool archive_present = std::filesystem::exists(surface.hidden_archive);
  if (!payload_present && archive_present) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          "optim plaintext surface is currently scrubbed under " +
          surface.hidden_root.string() +
          "; run tsodao sync to restore it before hero.config.optim.read";
    }
    return false;
  }

  std::filesystem::path dsl_path{};
  if (!resolve_optim_path_with_scope(*store, surface, path_raw,
                                     /*allow_missing_target=*/false, &dsl_path,
                                     &err)) {
    if (out_error_code)
      *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::string content{};
  if (!read_text_file(dsl_path.string(), &content, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  std::string sha256_hex{};
  if (!sha256_hex_text(content, &sha256_hex, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  const auto validation_family_enum =
      detect_instruction_dsl_validation_family(dsl_path);
  const std::string validation_family =
      instruction_dsl_validation_family_name(validation_family_enum);
  const std::filesystem::path config_root =
      resolve_config_root_from_global_config(*store);
  const std::filesystem::path man_path = find_associated_man_path_with_fallback(
      config_root, dsl_path, validation_family_enum);
  const std::string warning =
      man_path.empty() && should_warn_missing_associated_man(
                              dsl_path, validation_family_enum)
          ? missing_associated_man_warning(dsl_path)
          : "";
  if (!warning.empty())
    log_config_warning(warning);

  if (out_result_json) {
    std::ostringstream out;
    std::error_code ec{};
    const std::string relative_path =
        std::filesystem::relative(dsl_path, surface.hidden_root, ec).string();
    out << "{\"optim_root\":" << json_quote(surface.hidden_root.string())
        << ",\"archive_path\":" << json_quote(surface.hidden_archive.string())
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"relative_path\":" << json_quote(relative_path)
        << ",\"bytes\":" << content.size()
        << ",\"sha256\":" << json_quote(sha256_hex)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"replace_supported\":"
        << bool_json(validation_family != "unsupported")
        << ",\"content\":" << json_quote(content);
    if (!append_associated_man_fields(&out, man_path, include_man, warning,
                                      &err)) {
      if (out_error_code)
        *out_error_code = -32603;
      if (out_error_message)
        *out_error_message = err;
      return false;
    }
    out << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_optim_list(std::string_view tool_name,
                                          const std::string &request_json,
                                          hero_config_store_t *store,
                                          std::string *out_result_json,
                                          int *out_error_code,
                                          std::string *out_error_message) {
  bool include_man = false;
  bool parsed_include_man = false;
  if (extract_json_raw_field(request_json, "include_man", nullptr)) {
    if (!extract_json_bool_field(request_json, "include_man", &include_man)) {
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
      extract_json_bool_field(request_json, "includeMan", &include_man)) {
    parsed_include_man = true;
  }
  (void)parsed_include_man;

  tsodao_surface_t surface{};
  std::string err{};
  if (!resolve_tsodao_surface(*store, &surface, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  const bool payload_present = tsodao_hidden_payload_present(surface);
  const bool archive_present = std::filesystem::exists(surface.hidden_archive);
  std::vector<std::string> allowed_extensions{};
  if (!collect_allowed_extensions(*store, &allowed_extensions, &err)) {
    if (out_error_code)
      *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::vector<std::filesystem::path> files{};
  if (payload_present &&
      !list_instruction_files_under_root(surface.hidden_root,
                                         allowed_extensions, &files, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  const std::filesystem::path config_root =
      resolve_config_root_from_global_config(*store);

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"optim_root\":" << json_quote(surface.hidden_root.string())
        << ",\"archive_path\":" << json_quote(surface.hidden_archive.string())
        << ",\"public_keep_path\":"
        << json_quote(surface.public_keep_path.string())
        << ",\"archive_present\":" << bool_json(archive_present)
        << ",\"payload_present\":" << bool_json(payload_present)
        << ",\"files\":[";
    for (std::size_t i = 0; i < files.size(); ++i) {
      const auto &path = files[i];
      std::error_code ec{};
      const std::string relative_path =
          std::filesystem::relative(path, surface.hidden_root, ec).string();
      const auto validation_family_enum =
          detect_instruction_dsl_validation_family(path);
      const std::string validation_family =
          instruction_dsl_validation_family_name(validation_family_enum);
      const std::filesystem::path man_path =
          find_associated_man_path_with_fallback(config_root, path,
                                                 validation_family_enum);
      const std::string warning =
          man_path.empty() && should_warn_missing_associated_man(
                                  path, validation_family_enum)
              ? missing_associated_man_warning(path)
              : "";
      if (!warning.empty())
        log_config_warning(warning);
      if (i != 0)
        out << ",";
      out << "{\"path\":" << json_quote(path.string())
          << ",\"relative_path\":" << json_quote(relative_path)
          << ",\"validation_family\":" << json_quote(validation_family)
          << ",\"replace_supported\":"
          << bool_json(validation_family != "unsupported");
      if (!append_associated_man_fields(&out, man_path, include_man, warning,
                                        &err)) {
        if (out_error_code)
          *out_error_code = -32603;
        if (out_error_message)
          *out_error_message = err;
        return false;
      }
      out << "}";
    }
    out << "],\"count\":" << files.size();
    if (!payload_present && archive_present) {
      out << ",\"note\":"
          << json_quote("optim plaintext surface is currently scrubbed; run "
                        "tsodao sync to restore it before reading files");
    }
    out << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_optim_create(std::string_view tool_name,
                                            const std::string &request_json,
                                            hero_config_store_t *store,
                                            std::string *out_result_json,
                                            int *out_error_code,
                                            std::string *out_error_message) {
  std::string path_raw{};
  std::string content{};
  if (!extract_json_string_field(request_json, "path", &path_raw) ||
      path_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }
  if (!extract_json_string_field(request_json, "content", &content)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          std::string(tool_name) + " requires argument content";
    }
    return false;
  }

  tsodao_surface_t surface{};
  std::string err{};
  if (!resolve_tsodao_surface(*store, &surface, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::filesystem::path dsl_path{};
  if (!resolve_optim_path_with_scope(*store, surface, path_raw,
                                     /*allow_missing_target=*/true, &dsl_path,
                                     &err)) {
    if (out_error_code)
      *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!enforce_optim_write_target_allowed(*store, surface, dsl_path, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  bool payload_present = tsodao_hidden_payload_present(surface);
  const bool archive_present = std::filesystem::exists(surface.hidden_archive);
  if (!payload_present && archive_present &&
      !restore_optim_surface_from_archive(*store, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  payload_present = tsodao_hidden_payload_present(surface);

  std::error_code ec{};
  if (std::filesystem::exists(dsl_path, ec)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = "optim file already exists: " + dsl_path.string();
    }
    return false;
  }

  std::string validation_family{};
  std::filesystem::path grammar_path{};
  if (!validate_instruction_dsl_replacement(
          *store, dsl_path, content, &validation_family, &grammar_path, &err)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  const bool had_preexisting_surface = payload_present || archive_present;
  std::string checkpoint_backup{};
  if (!checkpoint_optim_surface_before_mutation(*store, surface,
                                                &checkpoint_backup, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (!write_text_file_atomic(dsl_path.string(), content, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (!sync_optim_surface_to_archive(*store, &err)) {
    std::string rollback_error{};
    (void)rollback_failed_optim_mutation(
        *store, surface, dsl_path, had_preexisting_surface, &rollback_error);
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message = "failed to sync optim surface after create: " + err;
      if (!rollback_error.empty()) {
        *out_error_message += "; rollback failed: " + rollback_error;
      }
    }
    return false;
  }

  std::string after_sha256{};
  if (!sha256_hex_file(dsl_path, &after_sha256, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"created\":true"
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"archive_path\":" << json_quote(surface.hidden_archive.string())
        << ",\"bytes\":" << content.size()
        << ",\"sha256\":" << json_quote(after_sha256)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"grammar_path\":" << json_quote(grammar_path.string())
        << ",\"checkpoint_backup\":" << json_quote(checkpoint_backup) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_optim_replace(std::string_view tool_name,
                                             const std::string &request_json,
                                             hero_config_store_t *store,
                                             std::string *out_result_json,
                                             int *out_error_code,
                                             std::string *out_error_message) {
  std::string path_raw{};
  std::string content{};
  std::string expected_sha256{};
  if (!extract_json_string_field(request_json, "path", &path_raw) ||
      path_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }
  if (!extract_json_string_field(request_json, "content", &content)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          std::string(tool_name) + " requires argument content";
    }
    return false;
  }
  (void)extract_json_string_field(request_json, "expected_sha256",
                                  &expected_sha256);

  tsodao_surface_t surface{};
  std::string err{};
  if (!resolve_tsodao_surface(*store, &surface, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::filesystem::path dsl_path{};
  if (!resolve_optim_path_with_scope(*store, surface, path_raw,
                                     /*allow_missing_target=*/true, &dsl_path,
                                     &err)) {
    if (out_error_code)
      *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!enforce_optim_write_target_allowed(*store, surface, dsl_path, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  bool payload_present = tsodao_hidden_payload_present(surface);
  const bool archive_present = std::filesystem::exists(surface.hidden_archive);
  if (!payload_present && archive_present &&
      !restore_optim_surface_from_archive(*store, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  payload_present = tsodao_hidden_payload_present(surface);

  std::error_code ec{};
  const bool existed = std::filesystem::exists(dsl_path, ec) &&
                       std::filesystem::is_regular_file(dsl_path, ec);
  if (!existed) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = "optim file does not exist: " + dsl_path.string();
    }
    return false;
  }
  std::string before_sha256{};
  if (!sha256_hex_file(dsl_path, &before_sha256, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  expected_sha256 = lowercase_copy(trim_ascii(expected_sha256));
  if (!expected_sha256.empty() &&
      lowercase_copy(before_sha256) != expected_sha256) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          "expected_sha256 does not match current optim content: " +
          dsl_path.string();
    }
    return false;
  }

  std::string validation_family{};
  std::filesystem::path grammar_path{};
  if (!validate_instruction_dsl_replacement(
          *store, dsl_path, content, &validation_family, &grammar_path, &err)) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  const bool had_preexisting_surface = payload_present || archive_present;
  std::string checkpoint_backup{};
  if (!checkpoint_optim_surface_before_mutation(*store, surface,
                                                &checkpoint_backup, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (!write_text_file_atomic(dsl_path.string(), content, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (!sync_optim_surface_to_archive(*store, &err)) {
    std::string rollback_error{};
    (void)rollback_failed_optim_mutation(
        *store, surface, dsl_path, had_preexisting_surface, &rollback_error);
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message = "failed to sync optim surface after replace: " + err;
      if (!rollback_error.empty()) {
        *out_error_message += "; rollback failed: " + rollback_error;
      }
    }
    return false;
  }

  std::string after_sha256{};
  if (!sha256_hex_file(dsl_path, &after_sha256, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"replaced\":true"
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"archive_path\":" << json_quote(surface.hidden_archive.string())
        << ",\"bytes\":" << content.size()
        << ",\"before_sha256\":" << json_quote(before_sha256)
        << ",\"sha256\":" << json_quote(after_sha256)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"grammar_path\":" << json_quote(grammar_path.string())
        << ",\"checkpoint_backup\":" << json_quote(checkpoint_backup) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_optim_delete(std::string_view tool_name,
                                            const std::string &request_json,
                                            hero_config_store_t *store,
                                            std::string *out_result_json,
                                            int *out_error_code,
                                            std::string *out_error_message) {
  std::string path_raw{};
  std::string expected_sha256{};
  if (!extract_json_string_field(request_json, "path", &path_raw) ||
      path_raw.empty()) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }
  (void)extract_json_string_field(request_json, "expected_sha256",
                                  &expected_sha256);

  tsodao_surface_t surface{};
  std::string err{};
  if (!resolve_tsodao_surface(*store, &surface, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::filesystem::path dsl_path{};
  if (!resolve_optim_path_with_scope(*store, surface, path_raw,
                                     /*allow_missing_target=*/true, &dsl_path,
                                     &err)) {
    if (out_error_code)
      *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  if (!enforce_optim_write_target_allowed(*store, surface, dsl_path, &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  bool payload_present = tsodao_hidden_payload_present(surface);
  const bool archive_present = std::filesystem::exists(surface.hidden_archive);
  if (!payload_present && archive_present &&
      !restore_optim_surface_from_archive(*store, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  payload_present = tsodao_hidden_payload_present(surface);

  std::error_code ec{};
  const bool existed = std::filesystem::exists(dsl_path, ec) &&
                       std::filesystem::is_regular_file(dsl_path, ec);
  if (!existed) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = "optim file does not exist: " + dsl_path.string();
    }
    return false;
  }
  std::string before_sha256{};
  if (!sha256_hex_file(dsl_path, &before_sha256, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }
  expected_sha256 = lowercase_copy(trim_ascii(expected_sha256));
  if (!expected_sha256.empty() &&
      lowercase_copy(before_sha256) != expected_sha256) {
    if (out_error_code)
      *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          "expected_sha256 does not match current optim content: " +
          dsl_path.string();
    }
    return false;
  }

  const bool had_preexisting_surface = payload_present || archive_present;
  std::string checkpoint_backup{};
  if (!checkpoint_optim_surface_before_mutation(*store, surface,
                                                &checkpoint_backup, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (!std::filesystem::remove(dsl_path, ec) || ec) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message = "failed to delete optim file: " + dsl_path.string();
    }
    return false;
  }

  if (!sync_optim_surface_to_archive(*store, &err)) {
    std::string rollback_error{};
    (void)rollback_failed_optim_mutation(
        *store, surface, dsl_path, had_preexisting_surface, &rollback_error);
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message = "failed to sync optim surface after delete: " + err;
      if (!rollback_error.empty()) {
        *out_error_message += "; rollback failed: " + rollback_error;
      }
    }
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"deleted\":true"
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"archive_path\":" << json_quote(surface.hidden_archive.string())
        << ",\"before_sha256\":" << json_quote(before_sha256)
        << ",\"checkpoint_backup\":" << json_quote(checkpoint_backup) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_optim_backups(std::string_view tool_name,
                                             const std::string &request_json,
                                             hero_config_store_t *store,
                                             std::string *out_result_json,
                                             int *out_error_code,
                                             std::string *out_error_message) {
  (void)tool_name;
  (void)request_json;

  tsodao_surface_t surface{};
  std::string err{};
  if (!resolve_tsodao_surface(*store, &surface, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  bool enabled = true;
  std::filesystem::path backup_dir{};
  std::int64_t max_entries = 20;
  if (!resolve_optim_backup_policy(*store, &enabled, &backup_dir, &max_entries,
                                   &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::vector<optim_backup_entry_t> entries{};
  if (!list_optim_backup_entries(*store, &entries, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"backup_enabled\":" << bool_json(enabled)
        << ",\"backup_dir\":" << json_quote(backup_dir.string())
        << ",\"backup_max_entries\":" << max_entries
        << ",\"archive_path\":" << json_quote(surface.hidden_archive.string())
        << ",\"count\":" << entries.size() << ",\"backups\":[";
    for (std::size_t i = 0; i < entries.size(); ++i) {
      if (i != 0)
        out << ",";
      out << "{\"index\":" << i
          << ",\"filename\":" << json_quote(entries[i].path.filename().string())
          << ",\"path\":" << json_quote(entries[i].path.string()) << "}";
    }
    out << "]}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_optim_rollback(std::string_view tool_name,
                                              const std::string &request_json,
                                              hero_config_store_t *store,
                                              std::string *out_result_json,
                                              int *out_error_code,
                                              std::string *out_error_message) {
  std::string selector{};
  if (extract_json_raw_field(request_json, "backup", nullptr)) {
    if (!extract_json_string_field(request_json, "backup", &selector)) {
      if (out_error_code)
        *out_error_code = -32602;
      if (out_error_message) {
        *out_error_message = std::string(tool_name) + " backup must be string";
      }
      return false;
    }
  }

  tsodao_surface_t surface{};
  std::string err{};
  if (!resolve_tsodao_surface(*store, &surface, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  const std::filesystem::path policy_probe =
      (surface.hidden_root / ".tsodao.optim.rollback.probe.dsl")
          .lexically_normal();
  if (!enforce_optim_write_target_allowed(*store, surface, policy_probe,
                                          &err)) {
    if (out_error_code)
      *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  std::string selected_backup{};
  std::string checkpoint_backup{};
  if (!restore_optim_backup_into_active_surface(*store, surface, selector,
                                                &selected_backup,
                                                &checkpoint_backup, &err)) {
    if (out_error_code)
      *out_error_code = -32603;
    if (out_error_message)
      *out_error_message = err;
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"rolled_back\":true"
        << ",\"optim_root\":" << json_quote(surface.hidden_root.string())
        << ",\"archive_path\":" << json_quote(surface.hidden_archive.string())
        << ",\"selected_backup\":" << json_quote(selected_backup)
        << ",\"checkpoint_backup\":" << json_quote(checkpoint_backup) << "}";
    *out_result_json = out.str();
  }
  return true;
}

} // namespace cuwacunu::hero::mcp::detail
