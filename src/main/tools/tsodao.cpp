#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <pwd.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/wait.h>

namespace fs = std::filesystem;

namespace {

struct tsodao_config_t {
  fs::path repo_root;
  fs::path global_config_path;
  fs::path tsodao_dsl_path;
  fs::path hidden_root;
  fs::path hidden_archive;
  fs::path public_keep_path;
  fs::path local_state_path;
  std::string visibility_mode;
  std::string gpg_recipient;
  std::string hidden_root_rel;
  std::string hidden_archive_rel;
  std::string public_keep_rel;
  std::string public_keep_basename;
};

struct run_result_t {
  int exit_code = 0;
  std::string output;
};

enum class sync_plan_t {
  nothing,
  no_action,
  plaintext_to_archive,
  archive_to_plaintext,
  ambiguous,
};

std::filesystem::path executable_path() {
  std::array<char, 4096> buf{};
  const ssize_t n = ::readlink("/proc/self/exe", buf.data(), buf.size() - 1);
  if (n > 0) {
    buf[static_cast<std::size_t>(n)] = '\0';
    return fs::path(buf.data());
  }
  return {};
}

bool is_repo_root(const fs::path& root) {
  std::error_code ec;
  return fs::exists(root / "src/config/.config", ec) &&
         fs::exists(root / "src/main/tools/Makefile", ec);
}

fs::path find_repo_root(const char* argv0) {
  std::vector<fs::path> seeds;

  const fs::path exe = executable_path();
  if (!exe.empty()) seeds.push_back(exe.parent_path());

  std::error_code ec;
  const fs::path cwd = fs::current_path(ec);
  if (!ec) seeds.push_back(cwd);

  if (argv0 != nullptr && argv0[0] != '\0') {
    const fs::path raw(argv0);
    if (raw.is_absolute()) {
      seeds.push_back(raw.parent_path());
    } else if (!cwd.empty()) {
      seeds.push_back(cwd / raw.parent_path());
    }
  }

  for (auto seed : seeds) {
    seed = fs::weakly_canonical(seed, ec);
    if (ec) continue;
    fs::path cur = seed;
    for (int depth = 0; depth < 8; ++depth) {
      if (is_repo_root(cur)) return cur;
      if (!cur.has_parent_path()) break;
      const auto parent = cur.parent_path();
      if (parent == cur) break;
      cur = parent;
    }
  }
  return {};
}

[[noreturn]] void die(const std::string& message) {
  std::cerr << "tsodao: " << message << "\n";
  std::exit(1);
}

void note(const std::string& message) { std::cout << "tsodao: " << message << "\n"; }

std::string trim_copy(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

std::string lower_ascii_copy(std::string value) {
  for (char& ch : value) {
    if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
  }
  return value;
}

bool command_exists(const std::string& name) {
  const char* path_env = std::getenv("PATH");
  if (path_env == nullptr) return false;
  std::stringstream ss(path_env);
  std::string item;
  while (std::getline(ss, item, ':')) {
    if (item.empty()) continue;
    const fs::path candidate = fs::path(item) / name;
    if (::access(candidate.c_str(), X_OK) == 0) return true;
  }
  return false;
}

void require_command(const std::string& name) {
  if (!command_exists(name)) die("missing required command: " + name);
}

fs::path resolve_near(const fs::path& base_dir, const std::string& raw_path) {
  if (raw_path.empty()) return {};
  const fs::path raw(raw_path);
  if (raw.is_absolute()) return raw.lexically_normal();
  return (base_dir / raw).lexically_normal();
}

bool path_starts_with(const fs::path& candidate, const fs::path& prefix) {
  auto it_candidate = candidate.begin();
  auto it_prefix = prefix.begin();
  for (; it_prefix != prefix.end(); ++it_prefix, ++it_candidate) {
    if (it_candidate == candidate.end() || *it_candidate != *it_prefix) return false;
  }
  return true;
}

std::string repo_relative_path(const fs::path& repo_root, const fs::path& abs_path) {
  const fs::path normalized_root = repo_root.lexically_normal();
  const fs::path normalized_path = abs_path.lexically_normal();
  if (!path_starts_with(normalized_path, normalized_root)) {
    die("path is outside repo root and cannot be managed by TSODAO git hooks: " +
        normalized_path.string());
  }
  std::error_code ec;
  const fs::path rel = fs::relative(normalized_path, normalized_root, ec);
  if (ec) die("failed to compute repo-relative path for " + normalized_path.string());
  return rel.generic_string();
}

std::optional<std::string> read_ini_value(const fs::path& ini_path,
                                          const std::string& section,
                                          const std::string& key) {
  std::ifstream in(ini_path);
  if (!in) die("failed to read " + ini_path.string());
  std::string line;
  std::string current_section;
  while (std::getline(in, line)) {
    std::string work = line;
    const auto comment_pos = work.find_first_of("#;");
    if (comment_pos != std::string::npos) work.erase(comment_pos);
    work = trim_copy(work);
    if (work.empty()) continue;
    if (work.front() == '[' && work.back() == ']') {
      current_section = trim_copy(work.substr(1, work.size() - 2));
      continue;
    }
    if (current_section != section) continue;
    const auto pos = work.find('=');
    if (pos == std::string::npos) continue;
    const std::string lhs = trim_copy(work.substr(0, pos));
    const std::string rhs = trim_copy(work.substr(pos + 1));
    if (lhs == key) return rhs;
  }
  return std::nullopt;
}

std::vector<std::string> load_dsl_lines_without_comments(const fs::path& dsl_path) {
  std::ifstream in(dsl_path);
  if (!in) die("failed to read " + dsl_path.string());
  std::vector<std::string> lines;
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
      if (start == std::string::npos) break;
      const auto end = work.find("*/", start + 2);
      if (end == std::string::npos) {
        work.erase(start);
        in_block = true;
        break;
      }
      work.erase(start, end - start + 2);
    }
    const auto comment_pos = work.find_first_of("#;");
    if (comment_pos != std::string::npos) work.erase(comment_pos);
    work = trim_copy(work);
    if (!work.empty()) lines.push_back(work);
  }
  return lines;
}

std::string canonical_dsl_lhs(std::string lhs) {
  for (;;) {
    const auto open = lhs.find('[');
    if (open == std::string::npos) break;
    const auto close = lhs.find(']', open + 1);
    if (close == std::string::npos) break;
    lhs.erase(open, close - open + 1);
  }
  for (;;) {
    const auto open = lhs.find('(');
    if (open == std::string::npos) break;
    const auto close = lhs.find(')', open + 1);
    if (close == std::string::npos) break;
    lhs.erase(open, close - open + 1);
  }
  const auto colon = lhs.find(':');
  if (colon != std::string::npos) lhs.erase(colon);
  return trim_copy(lhs);
}

std::optional<std::string> read_dsl_value(const fs::path& dsl_path, const std::string& key) {
  for (const std::string& line : load_dsl_lines_without_comments(dsl_path)) {
    const auto pos = line.find('=');
    if (pos == std::string::npos) continue;
    const std::string lhs = canonical_dsl_lhs(trim_copy(line.substr(0, pos)));
    if (lhs != key) continue;
    std::string rhs = trim_copy(line.substr(pos + 1));
    if (rhs.size() >= 2 &&
        ((rhs.front() == '"' && rhs.back() == '"') ||
         (rhs.front() == '\'' && rhs.back() == '\''))) {
      rhs = rhs.substr(1, rhs.size() - 2);
    }
    return rhs;
  }
  return std::nullopt;
}

std::optional<std::string> read_state_value(const fs::path& state_path, const std::string& key) {
  if (!fs::exists(state_path)) return std::nullopt;
  std::ifstream in(state_path);
  if (!in) return std::nullopt;
  std::string line;
  while (std::getline(in, line)) {
    std::string work = line;
    const auto comment_pos = work.find_first_of("#;");
    if (comment_pos != std::string::npos) work.erase(comment_pos);
    work = trim_copy(work);
    if (work.empty()) continue;
    const auto pos = work.find('=');
    if (pos == std::string::npos) continue;
    const std::string lhs = trim_copy(work.substr(0, pos));
    const std::string rhs = trim_copy(work.substr(pos + 1));
    if (lhs == key) return rhs;
  }
  return std::nullopt;
}

void write_text_file(const fs::path& path, const std::string& content) {
  const fs::path tmp = path.string() + ".tmp." + std::to_string(::getpid());
  {
    std::ofstream out(tmp);
    if (!out) die("failed to write temporary file: " + tmp.string());
    out << content;
    if (!out) die("failed to flush temporary file: " + tmp.string());
  }
  std::error_code ec;
  fs::rename(tmp, path, ec);
  if (ec) die("failed to replace " + path.string() + ": " + ec.message());
}

void write_dsl_line(const fs::path& dsl_path, const std::string& key,
                    const std::string& rendered_line) {
  std::ifstream in(dsl_path);
  if (!in) die("failed to read " + dsl_path.string());

  std::vector<std::string> lines;
  std::string line;
  bool replaced = false;
  while (std::getline(in, line)) {
    std::string work = line;
    const auto comment_pos = work.find_first_of("#;");
    if (comment_pos != std::string::npos) work.erase(comment_pos);
    work = trim_copy(work);
    const auto pos = work.find('=');
    if (!replaced && pos != std::string::npos) {
      const std::string lhs = canonical_dsl_lhs(trim_copy(work.substr(0, pos)));
      if (lhs == key) {
        lines.push_back(rendered_line);
        replaced = true;
        continue;
      }
    }
    lines.push_back(line);
  }
  if (!replaced) lines.push_back(rendered_line);

  std::ostringstream out;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    out << lines[i];
    if (i + 1 < lines.size()) out << '\n';
  }
  write_text_file(dsl_path, out.str());
}

run_result_t run_command_capture(const std::vector<std::string>& args, bool merge_stderr) {
  if (args.empty()) die("internal error: attempted to run empty command");
  int pipefd[2];
  if (::pipe(pipefd) != 0) die("pipe failed: " + std::string(std::strerror(errno)));

  const pid_t pid = ::fork();
  if (pid < 0) die("fork failed: " + std::string(std::strerror(errno)));
  if (pid == 0) {
    ::close(pipefd[0]);
    ::dup2(pipefd[1], STDOUT_FILENO);
    if (merge_stderr) ::dup2(pipefd[1], STDERR_FILENO);
    ::close(pipefd[1]);
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const std::string& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);
    ::execvp(argv[0], argv.data());
    std::perror("execvp");
    _exit(127);
  }

  ::close(pipefd[1]);
  std::string output;
  std::array<char, 4096> buf{};
  for (;;) {
    const ssize_t n = ::read(pipefd[0], buf.data(), buf.size());
    if (n < 0) {
      if (errno == EINTR) continue;
      break;
    }
    if (n == 0) break;
    output.append(buf.data(), static_cast<std::size_t>(n));
  }
  ::close(pipefd[0]);

  int status = 0;
  while (::waitpid(pid, &status, 0) < 0) {
    if (errno != EINTR) break;
  }

  run_result_t result;
  if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    result.exit_code = 128 + WTERMSIG(status);
  } else {
    result.exit_code = 1;
  }
  result.output = std::move(output);
  return result;
}

int run_command_inherit(const std::vector<std::string>& args) {
  if (args.empty()) die("internal error: attempted to run empty command");
  const pid_t pid = ::fork();
  if (pid < 0) die("fork failed: " + std::string(std::strerror(errno)));
  if (pid == 0) {
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const std::string& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);
    ::execvp(argv[0], argv.data());
    std::perror("execvp");
    _exit(127);
  }

  int status = 0;
  while (::waitpid(pid, &status, 0) < 0) {
    if (errno != EINTR) break;
  }
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
  return 1;
}

void run_command_or_die(const std::vector<std::string>& args, const std::string& what) {
  const int exit_code = run_command_inherit(args);
  if (exit_code != 0) {
    die(what + " failed with exit code " + std::to_string(exit_code));
  }
}

std::vector<std::string> split_lines(const std::string& text) {
  std::vector<std::string> lines;
  std::stringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    line = trim_copy(line);
    if (!line.empty()) lines.push_back(line);
  }
  return lines;
}

void setup_gpg_tty_if_needed() {
  if (std::getenv("GPG_TTY") != nullptr) return;
  if (!::isatty(STDIN_FILENO)) return;
  if (char* tty = ::ttyname(STDIN_FILENO); tty != nullptr) {
    ::setenv("GPG_TTY", tty, 0);
  }
}

std::string default_tsodao_uid() {
  std::string user_name = "user";
  if (const char* env_user = std::getenv("USER"); env_user != nullptr &&
                                                env_user[0] != '\0') {
    user_name = env_user;
  } else if (passwd* pw = ::getpwuid(::geteuid()); pw != nullptr &&
                                                 pw->pw_name != nullptr) {
    user_name = pw->pw_name;
  }

  std::array<char, 256> host_buf{};
  std::string host_name = "local";
  if (::gethostname(host_buf.data(), host_buf.size() - 1) == 0) {
    host_buf.back() = '\0';
    host_name = host_buf.data();
    const auto dot = host_name.find('.');
    if (dot != std::string::npos) host_name.erase(dot);
    host_name = trim_copy(host_name);
    if (host_name.empty()) host_name = "local";
  }

  return "Cuwacunu TSODAO <" + user_name + "@" + host_name + ">";
}

void prompt_uid_if_needed(std::string& user_id, bool assume_yes) {
  if (!user_id.empty()) return;
  const std::string default_uid = default_tsodao_uid();
  if (assume_yes) {
    user_id = default_uid;
    return;
  }

  std::cerr << "GPG user id for TSODAO [" << default_uid << "]: ";
  std::string reply;
  if (!std::getline(std::cin, reply)) {
    user_id = default_uid;
    return;
  }
  reply = trim_copy(reply);
  user_id = reply.empty() ? default_uid : reply;
}

std::string find_fingerprint_for_identity(const std::string& identity) {
  const run_result_t result = run_command_capture(
      {"gpg", "--list-secret-keys", "--with-colons", "--fingerprint", identity},
      true);
  if (result.exit_code != 0) return "";

  for (const std::string& line : split_lines(result.output)) {
    if (line.rfind("fpr:", 0) != 0) continue;
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    while (std::getline(ss, field, ':')) fields.push_back(field);
    if (fields.size() > 9 && !fields[9].empty()) return fields[9];
  }
  return "";
}

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
    die("hidden root already has plaintext payload; use tsodao sync --to-hidden or scrub first");
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
    std::cerr << "Replace plaintext files in " << cfg.hidden_root
              << " with the encrypted TSODAO archive? [y/N] ";
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
      die("tsodao.dsl visibility_mode=recipient but gpg_recipient is empty; run tsodao init first or pass --recipient explicitly");
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
      note("sealed TSODAO hidden surface to recipient: " + recipient);
    } else {
      run_command_or_die({"gpg", "--yes", "--output", cfg.hidden_archive.string(),
                          "--symmetric", "--cipher-algo", "AES256",
                          tar_path.string()},
                         "gpg symmetric encrypt");
      note("sealed TSODAO hidden surface with symmetric encryption");
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
    note("archive written to " + cfg.hidden_archive.string());
    note("plaintext tar sha256 " + tar_sha);

    const auto archive_sha = archive_sha256(cfg);
    if (!archive_sha) die("failed to record archive sha256");
    remember_archive_state(cfg, *archive_sha, "seal");

    if (scrub_after) {
      scrub_hidden_payload(cfg);
      note("plaintext hidden payload removed; " + cfg.public_keep_basename +
           " was left in place");
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
        die("tsodao.dsl visibility_mode=symmetric cannot auto-refresh in pre-commit. Run tsodao sync --to-archive --symmetric before commit.");
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
            ": the current archive does not match this machine's last synced archive. Run tsodao sync and choose a direction explicitly first.");
      }
      if (cfg.visibility_mode == "recipient") {
        if (cfg.gpg_recipient.empty()) {
          die("tsodao.dsl gpg_recipient is empty. Run tsodao init or seal manually.");
        }
        note("pre-commit: refreshing " + cfg.hidden_archive_rel +
             " from TSODAO hidden root");
        seal_hidden_surface(cfg, {"--recipient", cfg.gpg_recipient});
      } else if (cfg.visibility_mode == "symmetric") {
        die("tsodao.dsl visibility_mode=symmetric cannot auto-refresh in pre-commit. Run tsodao sync --to-archive --symmetric before commit.");
      } else if (cfg.visibility_mode.empty() || cfg.visibility_mode == "disabled" ||
                 cfg.visibility_mode == "manual" || cfg.visibility_mode == "off" ||
                 cfg.visibility_mode == "none") {
        die("hidden plaintext payload is present but TSODAO automatic recipient sealing is not configured. Set visibility_mode=recipient and gpg_recipient in tsodao.dsl, or run tsodao sync manually before commit.");
      } else {
        die("unknown tsodao.dsl visibility_mode value: " + cfg.visibility_mode);
      }
    } else if (!known_archive_matches_local_state(cfg)) {
      die("pre-commit found both plaintext and archive, but the archive does not match this machine's last synced archive. Run tsodao sync and choose a direction explicitly first.");
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
    note("restored TSODAO hidden plaintext payload into " + cfg.hidden_root.string());
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
    std::cerr << "Remove plaintext files from " << cfg.hidden_root << "? [y/N] ";
    std::string answer;
    if (!std::getline(std::cin, answer)) die("scrub cancelled");
    answer = trim_copy(answer);
    if (!(answer == "y" || answer == "Y")) die("scrub cancelled");
  }

  scrub_hidden_payload(cfg);
  note("plaintext hidden payload removed; " + cfg.public_keep_basename +
       " was left in place");
}

void status_command(const tsodao_config_t& cfg) {
  note("tsodao.dsl: " + cfg.tsodao_dsl_path.string());
  note("hidden root: " + cfg.hidden_root.string());
  note("hidden archive: " + cfg.hidden_archive.string());
  note("public keep path: " + cfg.public_keep_path.string());
  note("local state path: " + cfg.local_state_path.string());

  std::error_code ec;
  if (fs::is_directory(cfg.hidden_root, ec)) {
    note(std::string("plaintext hidden payload: ") +
         (has_hidden_payload(cfg) ? "present" : "absent or public-only"));
  } else {
    note("plaintext hidden payload: directory missing");
  }

  if (fs::exists(cfg.hidden_archive, ec)) {
    note("hidden archive: present");
    const run_result_t sha = run_command_capture({"sha256sum", cfg.hidden_archive.string()}, true);
    if (sha.exit_code == 0) std::cout << trim_copy(sha.output) << "\n";
  } else {
    note("hidden archive: absent");
  }

  if (fs::exists(cfg.local_state_path, ec)) {
    note("local state: present");
    note("last known archive sha256: " +
         trim_copy(read_state_value(cfg.local_state_path, "last_archive_sha256").value_or("")));
  } else {
    note("local state: absent");
  }

  note("visibility mode: " + (cfg.visibility_mode.empty() ? std::string("unset")
                                                          : cfg.visibility_mode));
  note("gpg recipient: " +
       (cfg.gpg_recipient.empty() ? std::string("unset") : cfg.gpg_recipient));
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
  bool to_archive = false;
  bool to_hidden = false;
  bool assume_yes = false;
  std::vector<std::string> forwarded;

  for (std::size_t i = 0; i < args.size(); ++i) {
    const std::string& arg = args[i];
    if (arg == "--to-archive" || arg == "--prefer-hidden") {
      to_archive = true;
    } else if (arg == "--to-hidden" || arg == "--prefer-archive") {
      to_hidden = true;
    } else if (arg == "--yes") {
      assume_yes = true;
    } else {
      forwarded.push_back(arg);
    }
  }

  if (to_archive && to_hidden) die("choose either --to-archive or --to-hidden");

  const bool plaintext_present = has_hidden_payload(cfg);
  const bool archive_present = fs::exists(cfg.hidden_archive);
  if (!plaintext_present && !archive_present) {
    die("nothing to sync: both hidden plaintext and hidden archive are absent");
  }
  if (plaintext_present && !archive_present) {
    if (to_hidden) die("cannot sync --to-hidden: hidden archive is absent");
    note("sync direction: plaintext -> archive (archive missing)");
    seal_hidden_surface(cfg, forwarded);
    maybe_git_stage_hidden_surface(cfg);
    return;
  }
  if (!plaintext_present && archive_present) {
    if (to_archive) die("cannot sync --to-archive: hidden plaintext is absent");
    note("sync direction: archive -> plaintext (plaintext absent)");
    open_hidden_surface(cfg);
    return;
  }

  if (to_archive) {
    if (known_archive_matches_local_state(cfg) && !needs_archive_refresh(cfg)) {
      note("sync: archive already matches the hidden plaintext surface; no action taken");
    } else {
      note("sync direction: plaintext -> archive (explicit)");
      seal_hidden_surface(cfg, forwarded);
      maybe_git_stage_hidden_surface(cfg);
    }
    return;
  }

  if (to_hidden) {
    if (known_archive_matches_local_state(cfg) && !needs_archive_refresh(cfg)) {
      note("sync: hidden plaintext already matches the encrypted archive; no action taken");
    } else {
      note("sync direction: archive -> plaintext (explicit)");
      confirm_restore_over_hidden_payload(cfg, assume_yes);
      open_hidden_surface(cfg);
    }
    return;
  }

  if (known_archive_matches_local_state(cfg)) {
    if (needs_archive_refresh(cfg)) {
      note("sync direction: plaintext -> archive (known local archive is older than plaintext)");
      seal_hidden_surface(cfg, forwarded);
      maybe_git_stage_hidden_surface(cfg);
    } else {
      note("sync: already safe and current for this machine; no action taken");
    }
    return;
  }

  die("sync is ambiguous: both plaintext and archive exist, and the archive does not match this machine's last synced archive. Refusing to overwrite either side. Use tsodao status, then rerun sync with --to-archive or --to-hidden if you want to choose a direction explicitly.");
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
  note("validated TSODAO recipient " + cfg.gpg_recipient);
}

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
      note("TSODAO recipient already configured: " + cfg.gpg_recipient);
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

  note("configured tsodao.dsl visibility_mode=recipient");
  note("configured tsodao.dsl gpg_recipient=" + fingerprint);

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
      << "  tsodao sync [--to-archive|--to-hidden] [--yes] [--symmetric|--recipient KEYID] [--scrub]\n"
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
      << "  - sync --to-archive means plaintext -> encrypted archive\n"
      << "  - sync --to-hidden means encrypted archive -> plaintext hidden root\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
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
      std::cerr << "tsodao: could not locate repo root\n";
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
      open_hidden_surface(cfg);
      return 0;
    }
    if (command == "seal") {
      seal_hidden_surface(cfg, args);
      return 0;
    }
    if (command == "scrub") {
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
    std::cerr << "tsodao: " << ex.what() << "\n";
    return 1;
  }
}
