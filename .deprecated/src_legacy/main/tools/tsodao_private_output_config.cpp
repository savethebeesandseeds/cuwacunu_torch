#include <algorithm>
#include <array>
#include <cctype>
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

sync_plan_t infer_default_sync_plan(const tsodao_config_t& cfg);

enum class output_kind_t { info, error, prompt };

struct output_style_t {
  std::string actor_tag = "[user]";
  bool color_stdout = false;
  bool color_stderr = false;
};

output_style_t& output_style() {
  static output_style_t style;
  return style;
}

std::string trim_copy(std::string value);

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

std::string local_user_id() {
  if (const char* env_user = std::getenv("USER");
      env_user != nullptr && env_user[0] != '\0') {
    return env_user;
  }
  if (passwd* pw = ::getpwuid(::geteuid()); pw != nullptr &&
                                             pw->pw_name != nullptr &&
                                             pw->pw_name[0] != '\0') {
    return pw->pw_name;
  }
  return "user";
}

bool color_enabled_for_fd(int fd) {
  if (const char* no_color = std::getenv("NO_COLOR");
      no_color != nullptr && no_color[0] != '\0') {
    return false;
  }
  if (const char* force_color = std::getenv("FORCE_COLOR");
      force_color != nullptr && force_color[0] != '\0' &&
      std::string_view(force_color) != "0") {
    return true;
  }
  const char* term = std::getenv("TERM");
  if (term == nullptr || std::string_view(term) == "dumb") return false;
  return ::isatty(fd) != 0;
}

void initialize_output_style() {
  auto& style = output_style();
  style.actor_tag = "[" + local_user_id() + "]";
  style.color_stdout = color_enabled_for_fd(STDOUT_FILENO);
  style.color_stderr = color_enabled_for_fd(STDERR_FILENO);
}

std::string color_wrap(std::string_view code, std::string_view text, bool enabled) {
  if (!enabled) return std::string(text);
  return std::string(code) + std::string(text) + "\033[0m";
}

std::size_t leading_space_count(std::string_view text) {
  std::size_t count = 0;
  while (count < text.size() && text[count] == ' ') ++count;
  return count;
}

bool looks_like_section_heading(const std::string& line) {
  const std::string trimmed = trim_copy(line);
  return !trimmed.empty() && leading_space_count(line) == 0 && trimmed.back() == ':';
}

std::string section_heading_text(const std::string& line) {
  std::string trimmed = trim_copy(line);
  if (!trimmed.empty() && trimmed.back() == ':') trimmed.pop_back();
  return trimmed;
}

std::optional<std::pair<std::string, std::string>> parse_field_line(
    const std::string& line) {
  const std::string trimmed = trim_copy(line);
  const auto pos = trimmed.find(" = ");
  if (pos == std::string::npos || pos == 0) return std::nullopt;
  return std::make_pair(trimmed.substr(0, pos), trimmed.substr(pos + 3));
}

std::string title_case_copy(std::string value) {
  bool upper_next = true;
  for (char& ch : value) {
    if (ch == ' ' || ch == '-' || ch == '_' || ch == '/') {
      upper_next = true;
      continue;
    }
    if (upper_next && ch >= 'a' && ch <= 'z') {
      ch = static_cast<char>(ch - 'a' + 'A');
      upper_next = false;
    } else if (upper_next) {
      upper_next = false;
    }
  }
  return value;
}

std::string render_section_header(const std::string& title, output_kind_t kind,
                                  bool color_enabled) {
  const std::string heading = "== " + title_case_copy(title) + " ==";
  switch (kind) {
    case output_kind_t::info:
      return color_wrap("\033[1;34m", heading, color_enabled);
    case output_kind_t::error:
      return color_wrap("\033[1;31m", heading, color_enabled);
    case output_kind_t::prompt:
      return color_wrap("\033[1;33m", heading, color_enabled);
  }
  return heading;
}

std::string render_prefix(output_kind_t kind, bool color_enabled) {
  const auto& style = output_style();
  const std::string actor = color_wrap("\033[1;36m", style.actor_tag, color_enabled);
  switch (kind) {
    case output_kind_t::info:
      return actor + " ";
    case output_kind_t::error:
      return actor + " " + color_wrap("\033[1;31m", "E:", color_enabled) + " ";
    case output_kind_t::prompt:
      return actor + " " + color_wrap("\033[1;33m", "?:", color_enabled) + " ";
  }
  return actor + " ";
}

std::string render_body_line(const std::string& line, std::size_t field_key_width,
                             bool color_enabled) {
  if (const auto field = parse_field_line(line)) {
    const std::string key =
        color_wrap("\033[0;36m", field->first, color_enabled);
    std::ostringstream oss;
    oss << "  " << key;
    if (field->first.size() < field_key_width) {
      oss << std::string(field_key_width - field->first.size(), ' ');
    }
    oss << " : " << field->second;
    return oss.str();
  }

  if (line.empty()) return "";
  if (leading_space_count(line) == 0) return "  " + line;
  return line;
}

void emit_lines(std::ostream& out, output_kind_t kind, const std::string& message,
                bool with_trailing_newline) {
  const bool color_enabled =
      (&out == &std::cout) ? output_style().color_stdout : output_style().color_stderr;
  const std::string prefix = render_prefix(kind, color_enabled);
  std::stringstream ss(message);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(ss, line)) lines.push_back(line);
  if (lines.empty()) lines.push_back("");

  const bool section_heading =
      lines.size() > 1 && looks_like_section_heading(lines.front());
  const std::size_t body_start = section_heading ? 1 : 1;
  std::size_t field_key_width = 0;
  for (std::size_t i = body_start; i < lines.size(); ++i) {
    if (const auto field = parse_field_line(lines[i])) {
      field_key_width = std::max(field_key_width, field->first.size());
    }
  }

  if (section_heading) {
    out << prefix
        << render_section_header(section_heading_text(lines.front()), kind,
                                 color_enabled);
    if (lines.size() > 1 || with_trailing_newline) out << '\n';
    for (std::size_t i = 1; i < lines.size(); ++i) {
      out << render_body_line(lines[i], field_key_width, color_enabled);
      if (i + 1 < lines.size() || with_trailing_newline) out << '\n';
    }
    out.flush();
    return;
  }

  out << prefix << lines.front();
  if (lines.size() > 1 || with_trailing_newline) out << '\n';
  for (std::size_t i = 1; i < lines.size(); ++i) {
    out << render_body_line(lines[i], field_key_width, color_enabled);
    if (i + 1 < lines.size() || with_trailing_newline) out << '\n';
  }
  out.flush();
}

[[noreturn]] void die(const std::string& message) {
  emit_lines(std::cerr, output_kind_t::error, message, true);
  std::exit(1);
}

void note(const std::string& message) {
  emit_lines(std::cout, output_kind_t::info, message, true);
}

void prompt(const std::string& message) {
  emit_lines(std::cerr, output_kind_t::prompt, message, false);
}

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

std::string display_path(const tsodao_config_t& cfg, const fs::path& path) {
  const fs::path normalized_root = cfg.repo_root.lexically_normal();
  const fs::path normalized_path = path.lexically_normal();
  if (path_starts_with(normalized_path, normalized_root)) {
    std::error_code ec;
    const fs::path rel = fs::relative(normalized_path, normalized_root, ec);
    if (!ec) return rel.generic_string();
  }
  return normalized_path.string();
}

bool is_ascii_hex(char ch) {
  return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') ||
         (ch >= 'A' && ch <= 'F');
}

std::string compact_hexish(std::string value) {
  value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char ch) {
                return std::isspace(ch) != 0;
              }),
              value.end());
  return value;
}

std::string recipient_summary(const std::string& recipient) {
  if (recipient.empty()) return "unset";
  const std::string compact = compact_hexish(recipient);
  if (compact.size() >= 16 &&
      std::all_of(compact.begin(), compact.end(), is_ascii_hex)) {
    return compact + " (key id " + compact.substr(compact.size() - 16) + ")";
  }
  return recipient;
}

void emit_operator_context(const tsodao_config_t& cfg) {
  std::ostringstream oss;
  oss << "context:\n"
      << "  policy = " << display_path(cfg, cfg.tsodao_dsl_path) << "\n"
      << "  surface = " << display_path(cfg, cfg.hidden_root) << "\n"
      << "  archive = " << display_path(cfg, cfg.hidden_archive) << "\n"
      << "  state = " << display_path(cfg, cfg.local_state_path) << "\n"
      << "  visibility = "
      << (cfg.visibility_mode.empty() ? std::string("unset") : cfg.visibility_mode)
      << "\n"
      << "  recipient = " << recipient_summary(cfg.gpg_recipient);
  note(oss.str());
}

std::string sync_choice_guidance(bool include_yes) {
  std::ostringstream oss;
  oss << "choose one source explicitly:\n"
      << "  tsodao sync --from-plaintext\n"
      << "    reseal archive from local plaintext\n"
      << "  tsodao sync --from-archive";
  if (include_yes) oss << " --yes";
  oss << "\n"
      << "    restore hidden root from current archive";
  return oss.str();
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

  prompt("GPG user id for TSODAO [" + default_uid + "]: ");
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

