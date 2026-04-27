#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

#include <fcntl.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <unistd.h>

#include "hero/marshal_hero/marshal_session.h"
#include "hero/runtime_hero/runtime_campaign.h"

namespace fs = std::filesystem;

static void require_impl(bool ok, const char *expr, const char *file,
                         int line) {
  if (!ok) {
    std::cerr << "[TEST FAIL] " << file << ":" << line << "  (" << expr
              << ")\n";
    std::exit(1);
  }
}
#define REQUIRE(x) require_impl((x), #x, __FILE__, __LINE__)

struct command_result_t {
  int exit_code{0};
  std::string output{};
};

static std::string random_suffix() {
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<std::uint64_t> dist;
  const std::uint64_t v = dist(gen);
  return std::to_string(static_cast<unsigned long long>(getpid())) + "_" +
         std::to_string(static_cast<unsigned long long>(v));
}

struct temp_dir_t {
  fs::path dir{};
  explicit temp_dir_t(std::string_view prefix) {
    dir = fs::temp_directory_path() /
          (std::string(prefix) + "_" + random_suffix());
    fs::create_directories(dir);
  }
  ~temp_dir_t() {
    std::error_code ec;
    fs::remove_all(dir, ec);
  }
};

static std::string shell_quote(std::string_view value) {
  std::string out;
  out.reserve(value.size() + 2);
  out.push_back('\'');
  for (const char c : value) {
    if (c == '\'') {
      out += "'\"'\"'";
    } else {
      out.push_back(c);
    }
  }
  out.push_back('\'');
  return out;
}

static command_result_t run_command(const std::string &command) {
  command_result_t out{};
  const std::string full = command + " 2>&1";
  FILE *pipe = ::popen(full.c_str(), "r");
  REQUIRE(pipe != nullptr);
  char buffer[4096];
  while (::fgets(buffer, static_cast<int>(sizeof(buffer)), pipe) != nullptr) {
    out.output.append(buffer);
  }
  const int status = ::pclose(pipe);
  if (status == -1) {
    out.exit_code = -1;
  } else if (WIFEXITED(status)) {
    out.exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    out.exit_code = 128 + WTERMSIG(status);
  } else {
    out.exit_code = -1;
  }
  return out;
}

static bool contains(std::string_view haystack, std::string_view needle) {
  return haystack.find(needle) != std::string_view::npos;
}

static constexpr const char *kHashimyeiMcpBin =
    "/cuwacunu/.build/hero/hero_hashimyei.mcp";
static constexpr const char *kLatticeMcpBin =
    "/cuwacunu/.build/hero/hero_lattice.mcp";
static constexpr const char *kMarshalMcpBin =
    "/cuwacunu/.build/hero/hero_marshal.mcp";
static constexpr const char *kHumanMcpBin =
    "/cuwacunu/.build/hero/hero_human.mcp";
static constexpr const char *kGlobalConfig = "/cuwacunu/src/config/.config";
static constexpr const char *kSmokeCodexSessionId =
    "11111111-1111-1111-1111-111111111111";

static std::string tools_list_command(const fs::path &binary_path,
                                      const fs::path &store_root,
                                      const fs::path &catalog_path) {
  const std::string req =
      R"({"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}})";
  std::ostringstream cmd;
  cmd << "req=" << shell_quote(req) << "; "
      << "{ printf 'Content-Length: %d\\r\\n\\r\\n%s' \"${#req}\" \"$req\"; } "
         "| "
      << shell_quote(binary_path.string()) << " --global-config "
      << shell_quote(kGlobalConfig) << " --store-root "
      << shell_quote(store_root.string()) << " --catalog "
      << shell_quote(catalog_path.string());
  return cmd.str();
}

static std::string list_tools_json_command(const fs::path &binary_path,
                                           const fs::path &global_config,
                                           const fs::path &hero_config = {}) {
  std::ostringstream cmd;
  cmd << shell_quote(binary_path.string()) << " --global-config "
      << shell_quote(global_config.string());
  if (!hero_config.empty()) {
    cmd << " --hero-config " << shell_quote(hero_config.string());
  }
  cmd << " --list-tools-json";
  return cmd.str();
}

static std::string direct_tool_command(const fs::path &binary_path,
                                       const fs::path &global_config,
                                       const fs::path &hero_config,
                                       std::string_view tool_name,
                                       std::string_view args_json) {
  std::ostringstream cmd;
  cmd << shell_quote(binary_path.string()) << " --global-config "
      << shell_quote(global_config.string());
  if (!hero_config.empty()) {
    cmd << " --hero-config " << shell_quote(hero_config.string());
  }
  cmd << " --tool " << shell_quote(std::string(tool_name)) << " --args-json "
      << shell_quote(std::string(args_json));
  return cmd.str();
}

static std::string direct_tool_command_with_paths(const fs::path &binary_path,
                                                  const fs::path &global_config,
                                                  const fs::path &store_root,
                                                  const fs::path &catalog_path,
                                                  std::string_view tool_name,
                                                  std::string_view args_json) {
  std::ostringstream cmd;
  cmd << shell_quote(binary_path.string()) << " --global-config "
      << shell_quote(global_config.string()) << " --store-root "
      << shell_quote(store_root.string()) << " --catalog "
      << shell_quote(catalog_path.string()) << " --tool "
      << shell_quote(std::string(tool_name)) << " --args-json "
      << shell_quote(std::string(args_json));
  return cmd.str();
}

static void write_text_file(const fs::path &path, std::string_view text) {
  fs::create_directories(path.parent_path());
  std::ofstream out(path);
  REQUIRE(out.is_open());
  out << text;
  out.close();
  REQUIRE(out.good());
}

static void write_executable_file(const fs::path &path, std::string_view text) {
  write_text_file(path, text);
  fs::permissions(path,
                  fs::perms::owner_read | fs::perms::owner_write |
                      fs::perms::owner_exec | fs::perms::group_read |
                      fs::perms::group_exec | fs::perms::others_read |
                      fs::perms::others_exec,
                  fs::perm_options::replace);
}

static std::string read_text_file(const fs::path &path) {
  std::ifstream in(path);
  REQUIRE(in.is_open());
  std::ostringstream out;
  out << in.rdbuf();
  return out.str();
}

struct live_process_identity_t {
  std::string boot_id{};
  std::uint64_t pid{0};
  std::uint64_t start_ticks{0};
};

static live_process_identity_t current_process_identity() {
  live_process_identity_t out{};
  out.boot_id = cuwacunu::hero::runtime::current_boot_id();
  out.pid = static_cast<std::uint64_t>(::getpid());
  REQUIRE(cuwacunu::hero::runtime::read_process_start_ticks(out.pid,
                                                            &out.start_ticks));
  return out;
}

struct held_file_lock_t {
  int fd{-1};

  explicit held_file_lock_t(const fs::path &path) {
    fs::create_directories(path.parent_path());
    fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    REQUIRE(fd >= 0);
    REQUIRE(::flock(fd, LOCK_EX | LOCK_NB) == 0);
  }

  held_file_lock_t(const held_file_lock_t &) = delete;
  held_file_lock_t &operator=(const held_file_lock_t &) = delete;

  ~held_file_lock_t() {
    if (fd >= 0) {
      (void)::flock(fd, LOCK_UN);
      (void)::close(fd);
    }
  }
};

static std::size_t count_occurrences(std::string_view haystack,
                                     std::string_view needle) {
  if (needle.empty())
    return 0;
  std::size_t count = 0;
  std::size_t pos = 0;
  for (;;) {
    pos = haystack.find(needle, pos);
    if (pos == std::string_view::npos)
      return count;
    ++count;
    pos += needle.size();
  }
}

static std::string extract_json_string_field(std::string_view json,
                                             std::string_view key) {
  const std::string marker = "\"" + std::string(key) + "\":\"";
  const std::size_t begin = json.find(marker);
  if (begin == std::string_view::npos)
    return {};
  const std::size_t value_begin = begin + marker.size();
  const std::size_t value_end = json.find('"', value_begin);
  if (value_end == std::string_view::npos)
    return {};
  return std::string(json.substr(value_begin, value_end - value_begin));
}

static std::string extract_lls_string_value(std::string_view text,
                                            std::string_view key) {
  const std::string marker = std::string(key) + ":str = ";
  const std::size_t begin = text.find(marker);
  if (begin == std::string_view::npos)
    return {};
  const std::size_t value_begin = begin + marker.size();
  const std::size_t value_end = text.find('\n', value_begin);
  if (value_end == std::string_view::npos) {
    return std::string(text.substr(value_begin));
  }
  return std::string(text.substr(value_begin, value_end - value_begin));
}

template <typename Predicate>
static command_result_t
wait_for_command(const std::string &command, Predicate &&predicate,
                 int timeout_ms = 10000, int poll_ms = 100) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  command_result_t last{};
  for (;;) {
    last = run_command(command);
    if (predicate(last))
      return last;
    if (std::chrono::steady_clock::now() >= deadline)
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(poll_ms));
  }
  return last;
}

static std::string wait_for_file_containing(const fs::path &path,
                                            std::string_view needle,
                                            int timeout_ms = 10000,
                                            int poll_ms = 100) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  std::string last{};
  for (;;) {
    if (fs::exists(path)) {
      last = read_text_file(path);
      if (contains(last, needle))
        return last;
    }
    if (std::chrono::steady_clock::now() >= deadline)
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(poll_ms));
  }
  return last;
}

static void write_manual_running_campaign_fixture(
    const fs::path &runtime_root, const fs::path &global_config,
    const fs::path &session_hero_marshal_dsl, std::string_view session_id,
    std::string_view campaign_cursor,
    const live_process_identity_t &campaign_identity) {
  const fs::path marshal_root = runtime_root / ".marshal_hero";
  const fs::path campaigns_root = runtime_root / ".campaigns";
  const fs::path session_root = marshal_root / std::string(session_id);
  const fs::path campaign_dir = campaigns_root / std::string(campaign_cursor);
  const std::uint64_t now_ms = cuwacunu::hero::runtime::now_ms_utc();

  fs::create_directories(session_root / "logs");
  fs::create_directories(session_root / "human");
  fs::create_directories(session_root / "checkpoints");
  fs::create_directories(campaign_dir / "jobs");
  write_text_file(campaign_dir / "campaign.dsl", "# stub campaign\n");

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  session.schema =
      std::string(cuwacunu::hero::marshal::kMarshalSessionSchemaV6);
  session.marshal_session_id = std::string(session_id);
  session.lifecycle = "live";
  session.status_detail = "campaign launched from marshal planning checkpoint";
  session.work_gate = "open";
  session.activity = "awaiting_campaign_fact";
  session.finish_reason = "none";
  session.objective_name = "runtime.operative.vicreg.solo.train";
  session.global_config_path = global_config.string();
  session.source_marshal_objective_dsl_path =
      "/cuwacunu/src/config/instructions/objectives/"
      "runtime.operative.vicreg.solo.train/"
      ".marshal.dsl";
  session.source_campaign_dsl_path =
      "/cuwacunu/src/config/instructions/objectives/"
      "runtime.operative.vicreg.solo.train/"
      "iitepi.campaign.dsl";
  session.source_marshal_objective_md_path =
      "/cuwacunu/src/config/instructions/objectives/"
      "runtime.operative.vicreg.solo.train/"
      "vicreg.solo.train.objective.md";
  session.source_marshal_guidance_md_path =
      "/cuwacunu/src/config/instructions/defaults/"
      "default.runtime.operative.guidance.md";
  session.session_root = session_root.string();
  session.objective_root = "/cuwacunu/src/config/instructions/objectives/"
                           "runtime.operative.vicreg.solo.train";
  session.campaign_dsl_path = session.source_campaign_dsl_path;
  session.marshal_objective_dsl_path =
      (session_root / "marshal.objective.dsl").string();
  session.marshal_objective_md_path =
      (session_root / "marshal.objective.md").string();
  session.marshal_guidance_md_path =
      (session_root / "marshal.guidance.md").string();
  session.hero_marshal_dsl_path = session_hero_marshal_dsl.string();
  session.config_policy_path =
      (session_root / "config.hero.policy.dsl").string();
  session.briefing_path = (session_root / "marshal.briefing.md").string();
  session.memory_path = (session_root / "marshal.session.memory.md").string();
  session.human_request_path =
      (session_root / "human" / "request.latest.md").string();
  session.events_path =
      (session_root / "marshal.session.events.jsonl").string();
  session.turns_path = (session_root / "marshal.session.turns.jsonl").string();
  session.codex_stdout_path =
      (session_root / "logs" / "codex.session.stdout.jsonl").string();
  session.codex_stderr_path =
      (session_root / "logs" / "codex.session.stderr.jsonl").string();
  session.resolved_marshal_codex_binary = "/bin/true";
  session.resolved_marshal_codex_model = "gpt-5.3-codex-spark";
  session.resolved_marshal_codex_reasoning_effort = "xhigh";
  session.started_at_ms = now_ms;
  session.updated_at_ms = now_ms;
  session.checkpoint_count = 1;
  session.launch_count = 1;
  session.max_campaign_launches = 4;
  session.remaining_campaign_launches = 3;
  session.authority_scope = "objective_only";
  session.campaign_status = "running";
  session.campaign_cursor = std::string(campaign_cursor);
  session.last_codex_action = "launch_campaign";
  session.last_input_checkpoint_path =
      (session_root / "checkpoints" / "input.0001.json").string();
  session.campaign_cursors.push_back(std::string(campaign_cursor));
  std::string session_error{};
  REQUIRE(cuwacunu::hero::marshal::write_marshal_session_record(
      marshal_root, session, &session_error));

  cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
  campaign.campaign_cursor = std::string(campaign_cursor);
  campaign.boot_id = campaign_identity.boot_id;
  campaign.state = "running";
  campaign.state_detail = "campaign runner alive";
  campaign.marshal_session_id = std::string(session_id);
  campaign.global_config_path = global_config.string();
  campaign.source_campaign_dsl_path = session.source_campaign_dsl_path;
  campaign.campaign_dsl_path = (campaign_dir / "campaign.dsl").string();
  campaign.started_at_ms = now_ms;
  campaign.updated_at_ms = now_ms;
  campaign.runner_pid = campaign_identity.pid;
  campaign.runner_start_ticks = campaign_identity.start_ticks;
  std::string campaign_error{};
  REQUIRE(cuwacunu::hero::runtime::write_runtime_campaign_record(
      campaigns_root, campaign, &campaign_error));
}

static void mark_runtime_campaign_terminal(const fs::path &runtime_root,
                                           const fs::path &global_config,
                                           std::string_view session_id,
                                           std::string_view campaign_cursor) {
  const fs::path campaigns_root = runtime_root / ".campaigns";
  cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
  campaign.campaign_cursor = std::string(campaign_cursor);
  campaign.state = "exited";
  campaign.state_detail = "campaign completed";
  campaign.marshal_session_id = std::string(session_id);
  campaign.global_config_path = global_config.string();
  campaign.source_campaign_dsl_path =
      "/cuwacunu/src/config/instructions/objectives/"
      "runtime.operative.vicreg.solo.train/"
      "iitepi.campaign.dsl";
  campaign.campaign_dsl_path =
      (campaigns_root / std::string(campaign_cursor) / "campaign.dsl").string();
  campaign.started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  campaign.updated_at_ms = campaign.started_at_ms;
  campaign.finished_at_ms = campaign.started_at_ms;
  std::string campaign_error{};
  REQUIRE(cuwacunu::hero::runtime::write_runtime_campaign_record(
      campaigns_root, campaign, &campaign_error));
}

static std::string reset_hashimyei_command(const fs::path &store_root,
                                           const fs::path &catalog_path) {
  std::ostringstream cmd;
  cmd << shell_quote(kHashimyeiMcpBin) << " --global-config "
      << shell_quote(kGlobalConfig) << " --store-root "
      << shell_quote(store_root.string()) << " --catalog "
      << shell_quote(catalog_path.string())
      << " --tool hero.hashimyei.reset_catalog"
      << " --args-json " << shell_quote(R"({"reingest":false})");
  return cmd.str();
}

static std::string reset_lattice_command(const fs::path &store_root,
                                         const fs::path &catalog_path) {
  std::ostringstream cmd;
  cmd << shell_quote(kLatticeMcpBin) << " --global-config "
      << shell_quote(kGlobalConfig) << " --store-root "
      << shell_quote(store_root.string()) << " --catalog "
      << shell_quote(catalog_path.string()) << " --tool hero.lattice.refresh"
      << " --args-json " << shell_quote(R"({"reingest":false})");
  return cmd.str();
}

static std::string
call_removed_lattice_tool_command(const fs::path &store_root,
                                  const fs::path &catalog_path) {
  std::ostringstream cmd;
  cmd << shell_quote(kLatticeMcpBin) << " --global-config "
      << shell_quote(kGlobalConfig) << " --store-root "
      << shell_quote(store_root.string()) << " --catalog "
      << shell_quote(catalog_path.string())
      << " --tool hero.lattice.matrix --args-json " << shell_quote("{}");
  return cmd.str();
}
