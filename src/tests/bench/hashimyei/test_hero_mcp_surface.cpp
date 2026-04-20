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
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.train/"
      "vicreg.solo.train.marshal.dsl";
  session.source_campaign_dsl_path =
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.train/"
      "iitepi.campaign.dsl";
  session.source_marshal_objective_md_path =
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.train/"
      "vicreg.solo.train.objective.md";
  session.source_marshal_guidance_md_path =
      "/cuwacunu/src/config/instructions/defaults/default.runtime.operative.guidance.md";
  session.session_root = session_root.string();
  session.objective_root =
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.train";
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
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.train/"
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

static void test_hashimyei_tools_list_surface() {
  temp_dir_t tmp("test_hashimyei_mcp_tools");
  const fs::path store_root = tmp.dir / "hash_store";
  const fs::path catalog = store_root / "hashimyei_catalog.idydb";
  fs::create_directories(store_root);

  const command_result_t result =
      run_command(tools_list_command(kHashimyeiMcpBin, store_root, catalog));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"hero.hashimyei.list\""));
  REQUIRE(contains(result.output,
                   "\"hero.hashimyei.evaluate_contract_compatibility\""));
  REQUIRE(
      contains(result.output, "\"hero.hashimyei.get_founding_dsl_bundle\""));
  REQUIRE(contains(result.output, "\"hero.hashimyei.update_rank\""));
  REQUIRE(contains(
      result.output,
      "\"required\":[\"family\",\"dock_hash\",\"ordered_hashimyeis\"]"));
  REQUIRE(contains(result.output, "\"hero.hashimyei.reset_catalog\""));
  REQUIRE(!contains(result.output, "\"anyOf\":"));
  REQUIRE(!contains(result.output, "\"oneOf\":"));
  REQUIRE(!contains(result.output, "\"allOf\":"));
  REQUIRE(!contains(result.output, "\"hero.hashimyei.get_history\""));
  REQUIRE(!contains(result.output, "\"hero.hashimyei.get_report_lls\""));
}

static void test_lattice_tools_list_surface() {
  temp_dir_t tmp("test_lattice_mcp_tools");
  const fs::path store_root = tmp.dir / "lat_store";
  const fs::path catalog = store_root / "lattice_catalog.idydb";
  fs::create_directories(store_root);

  const command_result_t result =
      run_command(tools_list_command(kLatticeMcpBin, store_root, catalog));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"hero.lattice.list_facts\""));
  REQUIRE(contains(result.output, "\"hero.lattice.get_fact\""));
  REQUIRE(contains(result.output, "\"hero.lattice.list_views\""));
  REQUIRE(contains(result.output, "\"hero.lattice.get_view\""));
  REQUIRE(contains(result.output, "\"required\":[\"view_kind\"]"));
  REQUIRE(contains(result.output, "\"hero.lattice.refresh\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.get_runs\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.list_report_fragments\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.get_report_lls\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.get_view_lls\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.reset_catalog\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.get_history\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.get_performance\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.debug_dump\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.run_or_get\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.matrix\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.trials\""));
}

static void test_marshal_tools_list_surface() {
  const command_result_t result =
      run_command(list_tools_json_command(kMarshalMcpBin, kGlobalConfig));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"hero.marshal.start_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.list_sessions\""));
  REQUIRE(contains(result.output, "\"hero.marshal.get_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.reconcile_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.pause_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.resume_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.message_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.archive_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.terminate_session\""));
  REQUIRE(!contains(result.output, "\"hero.marshal.start_loop\""));
  REQUIRE(!contains(result.output, "\"hero.marshal.list_loops\""));
  REQUIRE(!contains(result.output, "\"hero.marshal.get_loop\""));
  REQUIRE(!contains(result.output, "\"hero.marshal.stop_loop\""));
  REQUIRE(!contains(result.output, "\"hero.marshal.apply_human_resolution\""));
}

static void test_human_tools_list_surface() {
  const command_result_t result =
      run_command(list_tools_json_command(kHumanMcpBin, kGlobalConfig));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"hero.human.list_requests\""));
  REQUIRE(contains(result.output, "\"hero.human.get_request\""));
  REQUIRE(contains(result.output, "\"hero.human.answer_request\""));
  REQUIRE(contains(result.output, "\"hero.human.resolve_governance\""));
  REQUIRE(contains(result.output, "\"hero.human.list_summaries\""));
  REQUIRE(contains(result.output, "\"hero.human.get_summary\""));
  REQUIRE(contains(result.output, "\"hero.human.ack_summary\""));
  REQUIRE(contains(result.output, "\"hero.human.archive_summary\""));
  REQUIRE(!contains(result.output, "\"hero.human.list_sessions\""));
  REQUIRE(!contains(result.output, "\"hero.human.get_session\""));
  REQUIRE(!contains(result.output, "\"hero.human.pause_session\""));
  REQUIRE(!contains(result.output, "\"hero.human.resume_session\""));
  REQUIRE(!contains(result.output, "\"hero.human.message_session\""));
  REQUIRE(!contains(result.output, "\"hero.human.terminate_session\""));
  REQUIRE(!contains(result.output, "\"hero.human.answer_input\""));
  REQUIRE(!contains(result.output, "\"hero.human.dismiss_summary\""));
  REQUIRE(!contains(result.output, "\"hero.human.list_escalations\""));
  REQUIRE(!contains(result.output, "\"hero.human.get_escalation\""));
  REQUIRE(!contains(result.output, "\"hero.human.resolve_escalation\""));
  REQUIRE(!contains(result.output, "\"hero.human.list_reports\""));
  REQUIRE(!contains(result.output, "\"hero.human.get_report\""));
  REQUIRE(!contains(result.output, "\"hero.human.ack_report\""));
  REQUIRE(!contains(result.output, "\"hero.human.dismiss_report\""));
}

static void test_hashimyei_reset_catalog_smoke() {
  temp_dir_t tmp("test_hashimyei_mcp_reset");
  const fs::path store_root = tmp.dir / "hash_store";
  const fs::path catalog = store_root / "hashimyei_catalog.idydb";
  fs::create_directories(store_root);

  const command_result_t result =
      run_command(reset_hashimyei_command(store_root, catalog));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"isError\":false"));
  REQUIRE(contains(result.output, "\"component_count\":"));
  REQUIRE(fs::exists(catalog));
}

static void test_hashimyei_list_direct_tool_skips_ingest_rebuild() {
  temp_dir_t tmp("test_hashimyei_mcp_list_snapshot");
  const fs::path store_root = tmp.dir / "hash_store";
  const fs::path catalog = store_root / "hashimyei_catalog.idydb";
  const fs::path ingest_lock =
      store_root / "_meta" / "catalog" / ".ingest.lock";
  fs::create_directories(store_root);

  const command_result_t result = run_command(direct_tool_command_with_paths(
      kHashimyeiMcpBin, kGlobalConfig, store_root, catalog,
      "hero.hashimyei.list", "{}"));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"isError\":false"));
  REQUIRE(contains(result.output, "\"count\":0"));
  REQUIRE(!fs::exists(ingest_lock));
}

static void test_lattice_reset_catalog_smoke() {
  temp_dir_t tmp("test_lattice_mcp_reset");
  const fs::path store_root = tmp.dir / "lat_store";
  const fs::path catalog = store_root / "lattice_catalog.idydb";
  fs::create_directories(store_root);

  const command_result_t result =
      run_command(reset_lattice_command(store_root, catalog));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"isError\":false"));
  REQUIRE(contains(result.output, "\"runtime_report_fragment_count\":"));
  REQUIRE(fs::exists(catalog));
}

static void test_lattice_removed_tool_rejected() {
  temp_dir_t tmp("test_lattice_mcp_removed_tool");
  const fs::path store_root = tmp.dir / "lat_store";
  const fs::path catalog = store_root / "lattice_catalog.idydb";
  fs::create_directories(store_root);

  const command_result_t result =
      run_command(call_removed_lattice_tool_command(store_root, catalog));
  REQUIRE(result.exit_code != 0);
  REQUIRE(contains(result.output, "unknown tool: hero.lattice.matrix"));
}

static void test_marshal_session_continue_smoke() {
  temp_dir_t tmp("test_marshal_session_continue");
  const fs::path cfg_dir = tmp.dir / "cfg";
  const fs::path runtime_root = tmp.dir / "runtime";
  const fs::path global_config = cfg_dir / "global.config";
  const fs::path marshal_dsl = cfg_dir / "default.hero.marshal.dsl";
  const fs::path human_dsl = cfg_dir / "default.hero.human.dsl";
  const fs::path codex_stub = cfg_dir / "codex_stub.sh";
  const fs::path human_identities = cfg_dir / "human_operator_identities";
  const fs::path source_objective_dsl =
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.train/"
      "vicreg.solo.train.marshal.dsl";

  write_text_file(human_identities, "# smoke-only\n");
  const std::string codex_stub_text = std::string(
                                          R"CODEX(#!/usr/bin/env bash
set -euo pipefail
state_file="$(dirname "$0")/codex_stub.state"
decision_path=""
schema_path=""
resume_mode=0
expect_model="gpt-5.4-mini"
expect_effort='model_reasoning_effort="high"'
expect_runtime='mcp_servers.hero-runtime.command="/bin/true"'
expect_config='mcp_servers.hero-config.command="/bin/echo"'
expect_lattice='mcp_servers.hero-lattice.command="/bin/cat"'
have_model=0
have_effort=0
have_json=0
have_runtime=0
have_config=0
have_lattice=0
for ((i=1;i<=$#;i++)); do
  arg="${!i}"
  if [[ "$arg" == "resume" ]]; then resume_mode=1; fi
  if [[ "$arg" == "--json" ]]; then have_json=1; fi
  if [[ "$arg" == "-o" ]]; then
    j=$((i+1))
    decision_path="${!j}"
  fi
  if [[ "$arg" == "--output-schema" ]]; then
    j=$((i+1))
    schema_path="${!j}"
  fi
  if [[ "$arg" == "-m" ]]; then
    j=$((i+1))
    if [[ "${!j}" != "$expect_model" ]]; then
      echo "unexpected codex model: ${!j}" >&2
      exit 4
    fi
    have_model=1
  fi
  if [[ "$arg" == "-c" ]]; then
    j=$((i+1))
    if [[ "${!j}" == "$expect_effort" ]]; then
      have_effort=1
    fi
    if [[ "${!j}" == "$expect_runtime" ]]; then
      have_runtime=1
    fi
    if [[ "${!j}" == "$expect_config" ]]; then
      have_config=1
    fi
    if [[ "${!j}" == "$expect_lattice" ]]; then
      have_lattice=1
    fi
  fi
done
if [[ -z "$decision_path" ]]; then
  echo "missing -o decision path" >&2
  exit 2
fi
if [[ "$have_model" != "1" || "$have_effort" != "1" || "$have_runtime" != "1" || "$have_config" != "1" || "$have_lattice" != "1" ]]; then
  echo "missing pinned codex or MCP command arguments" >&2
  exit 4
fi
if [[ "$have_json" != "1" ]]; then
  echo "missing codex --json argument" >&2
  exit 4
fi
if [[ -n "$schema_path" ]] && grep -Eq ',[[:space:]]*[}\]]' "$schema_path"; then
  echo "invalid output schema json" >&2
  exit 4
fi
printf 'session id: %s\n' ")CODEX") + kSmokeCodexSessionId +
                                      R"CODEX(" >&2
step=0
if [[ -f "$state_file" ]]; then
  step="$(cat "$state_file")"
fi
printf '{"type":"stub_event","step":"%s","resume_mode":%s}\n' "$step" "$resume_mode"
if [[ "$resume_mode" == "0" && "$step" == "0" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"pause_for_clarification","reply_text":"","launch":null,"clarification_request":{"request":"Provide the missing clarification."},"governance":null,"reason":"need clarification","memory_note":"asked for clarification"}
JSON
  echo 1 > "$state_file"
  exit 0
fi
if [[ "$resume_mode" == "1" && "$step" == "1" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"complete","reply_text":"","launch":null,"clarification_request":null,"governance":null,"reason":"objective satisfied after clarification","memory_note":"completed after clarification"}
JSON
  echo 2 > "$state_file"
  exit 0
fi
if [[ "$resume_mode" == "1" && "$step" == "2" ]]; then
  sleep 2
  cat > "$decision_path" <<'JSON'
{"intent":"complete","reply_text":"","launch":null,"clarification_request":null,"governance":null,"reason":"objective satisfied after continuation","memory_note":"completed after continuation"}
JSON
  echo 3 > "$state_file"
  exit 0
fi
echo "unexpected codex stub invocation" >&2
exit 3
)CODEX";
  write_executable_file(codex_stub, codex_stub_text);
  write_text_file(
      global_config,
      std::string("[GENERAL]\n") + "runtime_root=" + runtime_root.string() +
          "\nrepo_root=/cuwacunu\n"
          "default_iitepi_campaign_dsl_filename=/cuwacunu/src/config/"
          "instructions/defaults/default.iitepi.campaign.dsl\n"
          "\n[BNF]\n"
          "iitepi_campaign_grammar_filename=/cuwacunu/src/config/bnf/"
          "iitepi.campaign.bnf\n"
          "marshal_objective_grammar_filename=/cuwacunu/src/config/bnf/"
          "objective.marshal.bnf\n"
          "\n[REAL_HERO]\n"
          "marshal_hero_dsl_filename=" +
          marshal_dsl.string() + "\n" +
          "human_hero_dsl_filename=" + human_dsl.string() + "\n");
  write_text_file(
      marshal_dsl,
      std::string("runtime_hero_binary:path = /bin/true\n") +
          "config_hero_binary:path = /bin/echo\n"
          "lattice_hero_binary:path = /bin/cat\n"
          "human_hero_binary:path = /cuwacunu/.build/hero/hero_human.mcp\n"
          "human_operator_identities:path = " +
          human_identities.string() + "\n" +
          "config_scope_root:path = /cuwacunu/src/config\n"
          "marshal_codex_binary:path = " +
          codex_stub.string() +
          "\n"
          "marshal_codex_model:str = gpt-5.3-codex-spark\n"
          "marshal_codex_reasoning_effort:str = xhigh\n"
          "tail_default_lines(1,+inf):int = 20\n"
          "poll_interval_ms(100,+inf):int = 100\n"
          "marshal_codex_timeout_sec(1,+inf):int = 5\n"
          "marshal_max_campaign_launches(1,+inf):int = 2\n");
  write_text_file(
      human_dsl,
      "marshal_hero_binary:path = /cuwacunu/.build/hero/hero_marshal.mcp\n"
      "operator_id:str = smoke@local\n"
      "operator_signing_ssh_identity:path = /tmp/unused-human-identity\n");
  const std::string start_args =
      std::string("{\"marshal_objective_dsl_path\":\"") +
      source_objective_dsl.string() +
      "\",\"marshal_codex_model\":\"gpt-5.4-mini\","
      "\"marshal_codex_reasoning_effort\":\"high\"}";
  const command_result_t start = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.start_session", start_args));
  REQUIRE(start.exit_code == 0);
  REQUIRE(contains(start.output, "\"isError\":false"));
  const std::string session_id =
      extract_json_string_field(start.output, "marshal_session_id");
  REQUIRE(!session_id.empty());

  const std::string get_args =
      std::string("{\"marshal_session_id\":\"") + session_id + "\"}";
  const std::string get_command =
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.get_session", get_args);
  const command_result_t paused =
      wait_for_command(get_command, [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"work_gate\":\"clarification\"");
      });
  REQUIRE(paused.exit_code == 0);
  const std::string paused_codex_session =
      extract_json_string_field(paused.output, "current_thread_id");
  REQUIRE(paused_codex_session == kSmokeCodexSessionId);
  REQUIRE(extract_json_string_field(
              paused.output, "resolved_marshal_codex_model") == "gpt-5.4-mini");
  REQUIRE(extract_json_string_field(
              paused.output, "resolved_marshal_codex_reasoning_effort") ==
          "high");

  const command_result_t pending_requests =
      run_command(direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                                      "hero.human.list_requests", "{}"));
  REQUIRE(pending_requests.exit_code == 0);
  REQUIRE(contains(pending_requests.output, "\"isError\":false"));
  REQUIRE(contains(pending_requests.output, session_id));

  const command_result_t human_request =
      run_command(direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                                      "hero.human.get_request", get_args));
  REQUIRE(human_request.exit_code == 0);
  REQUIRE(contains(human_request.output, "\"isError\":false"));
  REQUIRE(
      contains(human_request.output,
               std::string("\"marshal_session_id\":\"") + session_id + "\""));

  const std::string answer_args =
      std::string("{\"marshal_session_id\":\"") + session_id +
      "\",\"answer\":\"Here is the clarification.\"}";
  const command_result_t answered = run_command(
      direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                          "hero.human.answer_request", answer_args));
  REQUIRE(answered.exit_code == 0);
  REQUIRE(contains(answered.output, "\"isError\":false"));

  const command_result_t idled =
      wait_for_command(get_command, [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"activity\":\"review\"") &&
               contains(result.output, "\"finish_reason\":\"success\"");
      });
  REQUIRE(idled.exit_code == 0);
  REQUIRE(contains(idled.output, "\"checkpoint_count\":2"));
  const std::string idled_codex_session =
      extract_json_string_field(idled.output, "current_thread_id");
  REQUIRE(idled_codex_session == paused_codex_session);

  const command_result_t pending_summaries =
      run_command(direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                                      "hero.human.list_summaries", "{}"));
  REQUIRE(pending_summaries.exit_code == 0);
  REQUIRE(contains(pending_summaries.output, "\"isError\":false"));
  REQUIRE(contains(pending_summaries.output, session_id));

  const command_result_t human_get =
      run_command(direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                                      "hero.human.get_summary", get_args));
  REQUIRE(human_get.exit_code == 0);
  REQUIRE(contains(human_get.output, "\"isError\":false"));
  REQUIRE(contains(human_get.output, "\"lifecycle\":\"live\""));
  REQUIRE(contains(human_get.output, "\"activity\":\"review\""));

  write_text_file(
      marshal_dsl,
      std::string("runtime_hero_binary:path = /bin/false\n") +
          "config_hero_binary:path = /bin/sh\n"
          "lattice_hero_binary:path = /usr/bin/env\n"
          "human_hero_binary:path = /cuwacunu/.build/hero/hero_human.mcp\n"
          "human_operator_identities:path = " +
          human_identities.string() + "\n" +
          "config_scope_root:path = /cuwacunu/src/config\n"
          "marshal_codex_binary:path = /bin/false\n"
          "marshal_codex_model:str = broken-model\n"
          "marshal_codex_reasoning_effort:str = low\n"
          "tail_default_lines(1,+inf):int = 3\n"
          "poll_interval_ms(100,+inf):int = 9000\n"
          "marshal_codex_timeout_sec(1,+inf):int = 1\n"
          "marshal_max_campaign_launches(1,+inf):int = 9\n");

  const std::string continue_args = std::string("{\"marshal_session_id\":\"") +
                                    session_id +
                                    "\",\"message\":\"Train for 10 more "
                                    "epochs from the latest checkpoint.\"}";
  const command_result_t continued = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.message_session", continue_args));
  REQUIRE(continued.exit_code == 0);
  REQUIRE(contains(continued.output, "\"isError\":false"));

  const command_result_t idled_again =
      wait_for_command(get_command, [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"activity\":\"review\"") &&
               contains(result.output, "\"finish_reason\":\"success\"") &&
               contains(result.output, "\"checkpoint_count\":3");
      });
  REQUIRE(idled_again.exit_code == 0);
  const std::string continued_codex_session =
      extract_json_string_field(idled_again.output, "current_thread_id");
  REQUIRE(continued_codex_session == idled_codex_session);

  const fs::path session_root = runtime_root / ".marshal_hero" / session_id;
  const fs::path hero_marshal_dsl = session_root / "hero.marshal.dsl";
  const fs::path codex_stdout_log =
      session_root / "logs" / "codex.session.stdout.jsonl";
  const fs::path codex_stderr_log =
      session_root / "logs" / "codex.session.stderr.jsonl";
  REQUIRE(fs::exists(hero_marshal_dsl));
  REQUIRE(contains(read_text_file(hero_marshal_dsl), "repo_root:path = "));
  REQUIRE(contains(read_text_file(hero_marshal_dsl),
                   "runtime_hero_binary:path = /bin/true"));
  REQUIRE(contains(read_text_file(hero_marshal_dsl),
                   "config_hero_binary:path = /bin/echo"));
  REQUIRE(contains(read_text_file(hero_marshal_dsl),
                   "lattice_hero_binary:path = /bin/cat"));
  REQUIRE(contains(read_text_file(hero_marshal_dsl),
                   "marshal_codex_model:str = gpt-5.4-mini"));
  REQUIRE(contains(read_text_file(hero_marshal_dsl),
                   "marshal_codex_reasoning_effort:str = high"));
  REQUIRE(contains(read_text_file(hero_marshal_dsl),
                   "marshal_codex_timeout_sec(1,+inf):int = 5"));
  REQUIRE(fs::exists(codex_stdout_log));
  REQUIRE(fs::exists(codex_stderr_log));
  REQUIRE(!fs::exists(session_root / "logs" / "codex.session.log"));
  REQUIRE(
      contains(read_text_file(codex_stdout_log), "\"type\":\"stub_event\""));
  REQUIRE(contains(read_text_file(codex_stderr_log), "\"stream\":\"stderr\""));
  REQUIRE(contains(read_text_file(codex_stderr_log), kSmokeCodexSessionId));
  const std::string events_text = wait_for_file_containing(
      session_root / "marshal.session.events.jsonl",
      "\"type\":\"operator.message_handled\"");
  REQUIRE(contains(events_text, "\"type\":\"operator.message_received\""));
  REQUIRE(contains(events_text, "\"type\":\"operator.message_delivered\""));
  REQUIRE(contains(events_text, "\"type\":\"operator.message_handled\""));
  REQUIRE(count_occurrences(events_text, "\"type\":\"checkpoint.staged\"") >=
          3);

  const std::string memory_text =
      read_text_file(session_root / "marshal.session.memory.md");
  REQUIRE(contains(memory_text, "## Checkpoint 2"));
  REQUIRE(contains(memory_text, "## Checkpoint 3"));
}

static void test_marshal_run_plan_replans_between_bind_steps() {
  temp_dir_t tmp("test_marshal_run_plan_replans_between_bind_steps");
  const fs::path cfg_dir = tmp.dir / "cfg";
  const fs::path runtime_root = tmp.dir / "runtime";
  const fs::path global_config = cfg_dir / "global.config";
  const fs::path marshal_dsl = cfg_dir / "default.hero.marshal.dsl";
  const fs::path human_dsl = cfg_dir / "default.hero.human.dsl";
  const fs::path codex_stub = cfg_dir / "codex_stub.sh";
  const fs::path runtime_stub = cfg_dir / "runtime_stub.sh";
  const fs::path human_identities = cfg_dir / "human_operator_identities";
  const fs::path source_objective_dsl =
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.train/"
      "vicreg.solo.train.marshal.dsl";

  write_text_file(human_identities, "# smoke-only\n");
  const std::string runtime_stub_text = std::string(
                                            R"RUNTIME(#!/usr/bin/env bash
set -euo pipefail
campaigns_root=")RUNTIME") + (runtime_root / ".campaigns").string() +
                                        R"RUNTIME("
tool_name=""
args_json="{}"
global_config=""
for ((i=1;i<=$#;i++)); do
  arg="${!i}"
  if [[ "$arg" == "--tool" ]]; then
    j=$((i+1))
    tool_name="${!j}"
  fi
  if [[ "$arg" == "--args-json" ]]; then
    j=$((i+1))
    args_json="${!j}"
  fi
  if [[ "$arg" == "--global-config" ]]; then
    j=$((i+1))
    global_config="${!j}"
  fi
done
if [[ "$tool_name" != "hero.runtime.start_campaign" ]]; then
  printf '{"content":[{"type":"text","text":"unsupported tool"}],"structuredContent":{},"isError":true}\n'
  exit 1
fi
binding_id="$(sed -n 's/.*"binding_id":"\([^"]*\)".*/\1/p' <<<"$args_json" | head -n 1)"
marshal_session_id="$(sed -n 's/.*"marshal_session_id":"\([^"]*\)".*/\1/p' <<<"$args_json" | head -n 1)"
campaign_dsl_path="$(sed -n 's/.*"campaign_dsl_path":"\([^"]*\)".*/\1/p' <<<"$args_json" | head -n 1)"
reset_runtime_state=false
grep -F '"reset_runtime_state":true' <<<"$args_json" >/dev/null && reset_runtime_state=true
if [[ -z "$binding_id" || -z "$marshal_session_id" || -z "$campaign_dsl_path" || -z "$global_config" ]]; then
  printf '{"content":[{"type":"text","text":"missing required start_campaign field"}],"structuredContent":{},"isError":true}\n'
  exit 1
fi
state_file="$(dirname "$0")/runtime_stub.state"
count=0
if [[ -f "$state_file" ]]; then
  count="$(cat "$state_file")"
fi
count=$((count+1))
printf '%s' "$count" > "$state_file"
campaign_cursor="$(printf 'campaign.stub.%04d' "$count")"
campaign_dir="$campaigns_root/$campaign_cursor"
mkdir -p "$campaign_dir"
cp "$campaign_dsl_path" "$campaign_dir/campaign.dsl"
started_at="$((1000 + count))"
cat > "$campaign_dir/runtime.campaign.manifest.lls" <<EOF
schema:str = hero.runtime.campaign.v2
campaign_cursor:str = $campaign_cursor
boot_id:str = stub-boot
state:str = exited
state_detail:str = campaign completed
marshal_session_id:str = $marshal_session_id
global_config_path:str = $global_config
source_campaign_dsl_path:str = $campaign_dsl_path
campaign_dsl_path:str = $campaign_dir/campaign.dsl
reset_runtime_state:bool = $reset_runtime_state
stdout_path:str = $campaign_dir/stdout.log
stderr_path:str = $campaign_dir/stderr.log
started_at_ms(0,+inf):uint = $started_at
updated_at_ms(0,+inf):uint = $started_at
finished_at_ms(0,+inf):uint = $started_at
current_run_index(0,+inf):uint = 1
run_bind_id.0000:str = $binding_id
EOF
: > "$campaign_dir/stdout.log"
: > "$campaign_dir/stderr.log"
printf '{"content":[{"type":"text","text":"ok"}],"structuredContent":{"campaign_cursor":"%s","campaign":{"schema":"hero.runtime.campaign.v2","campaign_cursor":"%s","state":"exited","state_detail":"campaign completed","marshal_session_id":"%s","run_bind_ids":["%s"],"current_run_index":1},"warnings":[]},"isError":false}\n' \
  "$campaign_cursor" "$campaign_cursor" "$marshal_session_id" "$binding_id"
)RUNTIME";
  write_executable_file(runtime_stub, runtime_stub_text);

  const std::string codex_stub_text = std::string(
                                          R"CODEX(#!/usr/bin/env bash
set -euo pipefail
state_file="$(dirname "$0")/codex_stub.state"
decision_path=""
resume_mode=0
for ((i=1;i<=$#;i++)); do
  arg="${!i}"
  if [[ "$arg" == "resume" ]]; then resume_mode=1; fi
  if [[ "$arg" == "-o" ]]; then
    j=$((i+1))
    decision_path="${!j}"
  fi
done
if [[ -z "$decision_path" ]]; then
  echo "missing -o decision path" >&2
  exit 2
fi
briefing="$(cat)"
printf 'session id: %s\n' ")CODEX") + kSmokeCodexSessionId +
                                      R"CODEX(" >&2
step=0
if [[ -f "$state_file" ]]; then
  step="$(cat "$state_file")"
fi
case "$step" in
  0)
    [[ "$resume_mode" == "0" ]] || { echo "expected fresh checkpoint" >&2; exit 3; }
    grep -F '"checkpoint_kind":"bootstrap"' <<<"$briefing" >/dev/null || { echo "missing bootstrap checkpoint" >&2; exit 4; }
    grep -F '"run_plan_progress":{' <<<"$briefing" >/dev/null || { echo "missing run plan progress" >&2; exit 4; }
    grep -F '"next_pending_bind_id":"bind_train_vicreg_primary_btcusdt"' <<<"$briefing" >/dev/null || { echo "missing first pending bind" >&2; exit 4; }
    cat > "$decision_path" <<'JSON'
{"intent":"launch_campaign","reply_text":"","launch":{"mode":"run_plan","binding_id":null,"reset_runtime_state":false,"requires_objective_mutation":false},"clarification_request":null,"governance":null,"reason":"launch the first pending RUN step","memory_note":"launch first pending bind"}
JSON
    echo 1 > "$state_file"
    exit 0
    ;;
  1)
    grep -F '"checkpoint_kind":"postcampaign"' <<<"$briefing" >/dev/null || { echo "missing postcampaign checkpoint 1" >&2; exit 4; }
    grep -F '"completed_run_bind_ids":["bind_train_vicreg_primary_btcusdt"]' <<<"$briefing" >/dev/null || { echo "missing completed bind 1" >&2; exit 4; }
    grep -F '"next_pending_bind_id":"bind_eval_vicreg_payload_btcusdt"' <<<"$briefing" >/dev/null || { echo "missing second pending bind" >&2; exit 4; }
    cat > "$decision_path" <<'JSON'
{"intent":"launch_campaign","reply_text":"","launch":{"mode":"run_plan","binding_id":null,"reset_runtime_state":false,"requires_objective_mutation":false},"clarification_request":null,"governance":null,"reason":"launch the second pending RUN step","memory_note":"launch second pending bind"}
JSON
    echo 2 > "$state_file"
    exit 0
    ;;
  2)
    grep -F '"completed_run_bind_ids":["bind_train_vicreg_primary_btcusdt","bind_eval_vicreg_payload_btcusdt"]' <<<"$briefing" >/dev/null || { echo "missing completed binds 1-2" >&2; exit 4; }
    grep -F '"next_pending_bind_id":"bind_eval_vicreg_payload_btcusdt_untouched_test"' <<<"$briefing" >/dev/null || { echo "missing third pending bind" >&2; exit 4; }
    cat > "$decision_path" <<'JSON'
{"intent":"launch_campaign","reply_text":"","launch":{"mode":"run_plan","binding_id":null,"reset_runtime_state":false,"requires_objective_mutation":false},"clarification_request":null,"governance":null,"reason":"launch the final pending RUN step","memory_note":"launch third pending bind"}
JSON
    echo 3 > "$state_file"
    exit 0
    ;;
  3)
    grep -F '"completed_run_bind_ids":["bind_train_vicreg_primary_btcusdt","bind_eval_vicreg_payload_btcusdt","bind_eval_vicreg_payload_btcusdt_untouched_test"]' <<<"$briefing" >/dev/null || { echo "missing completed binds 1-3" >&2; exit 4; }
    grep -F '"next_pending_bind_id":null' <<<"$briefing" >/dev/null || { echo "missing null next pending bind" >&2; exit 4; }
    cat > "$decision_path" <<'JSON'
{"intent":"complete","reply_text":"","launch":null,"clarification_request":null,"governance":null,"reason":"all pending RUN steps completed with replanning between binds","memory_note":"completed after per-run replanning"}
JSON
    echo 4 > "$state_file"
    exit 0
    ;;
esac
echo "unexpected codex stub invocation" >&2
exit 5
)CODEX";
  write_executable_file(codex_stub, codex_stub_text);
  write_text_file(
      global_config,
      std::string("[GENERAL]\n") + "runtime_root=" + runtime_root.string() +
          "\nrepo_root=/cuwacunu\n"
          "default_iitepi_campaign_dsl_filename=/cuwacunu/src/config/"
          "instructions/defaults/default.iitepi.campaign.dsl\n"
          "\n[BNF]\n"
          "iitepi_campaign_grammar_filename=/cuwacunu/src/config/bnf/"
          "iitepi.campaign.bnf\n"
          "marshal_objective_grammar_filename=/cuwacunu/src/config/bnf/"
          "objective.marshal.bnf\n"
          "\n[REAL_HERO]\n"
          "marshal_hero_dsl_filename=" +
          marshal_dsl.string() + "\n" +
          "human_hero_dsl_filename=" + human_dsl.string() + "\n");
  write_text_file(
      marshal_dsl,
      std::string("runtime_hero_binary:path = ") + runtime_stub.string() +
          "\n" +
          "config_hero_binary:path = /bin/true\n"
          "lattice_hero_binary:path = /bin/true\n"
          "human_hero_binary:path = /cuwacunu/.build/hero/hero_human.mcp\n"
          "human_operator_identities:path = " +
          human_identities.string() + "\n" +
          "config_scope_root:path = /cuwacunu/src/config\n" +
          "marshal_codex_binary:path = " + codex_stub.string() + "\n" +
          "marshal_codex_model:str = gpt-5.3-codex-spark\n"
          "marshal_codex_reasoning_effort:str = xhigh\n"
          "tail_default_lines(1,+inf):int = 20\n"
          "poll_interval_ms(100,+inf):int = 100\n"
          "marshal_codex_timeout_sec(1,+inf):int = 5\n"
          "marshal_max_campaign_launches(1,+inf):int = 4\n");
  write_text_file(
      human_dsl,
      "marshal_hero_binary:path = /cuwacunu/.build/hero/hero_marshal.mcp\n"
      "operator_id:str = smoke@local\n"
      "operator_signing_ssh_identity:path = /tmp/unused-human-identity\n");

  const std::string start_args =
      std::string("{\"marshal_objective_dsl_path\":\"") +
      source_objective_dsl.string() + "\"}";
  const command_result_t start = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.start_session", start_args));
  REQUIRE(start.exit_code == 0);
  const std::string session_id =
      extract_json_string_field(start.output, "marshal_session_id");
  REQUIRE(!session_id.empty());

  const std::string get_args =
      std::string("{\"marshal_session_id\":\"") + session_id + "\"}";
  const std::string get_command =
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.get_session", get_args);
  const command_result_t idled = wait_for_command(
      get_command,
      [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"activity\":\"review\"") &&
               contains(result.output, "\"finish_reason\":\"success\"") &&
               contains(result.output, "\"checkpoint_count\":4") &&
               contains(result.output, "\"launch_count\":3");
      },
      20000, 100);
  REQUIRE(idled.exit_code == 0);
  const fs::path session_root = runtime_root / ".marshal_hero" / session_id;
  const std::string memory_text =
      read_text_file(session_root / "marshal.session.memory.md");
  REQUIRE(
      contains(memory_text, "run_plan launch decomposed to next pending bind"));

  const fs::path campaigns_root = runtime_root / ".campaigns";
  REQUIRE(fs::exists(campaigns_root));
  std::size_t campaign_count = 0;
  bool saw_train = false;
  bool saw_eval = false;
  bool saw_untouched = false;
  for (const auto &it : fs::directory_iterator(campaigns_root)) {
    if (!it.is_directory())
      continue;
    ++campaign_count;
    const std::string manifest_text =
        read_text_file(it.path() / "runtime.campaign.manifest.lls");
    REQUIRE(contains(manifest_text,
                     std::string("marshal_session_id:str = ") + session_id));
    REQUIRE(contains(manifest_text, "run_bind_id.0000:str = "));
    REQUIRE(!contains(manifest_text, "run_bind_id.0001:str = "));
    const std::string bind_id =
        extract_lls_string_value(manifest_text, "run_bind_id.0000");
    if (bind_id == "bind_train_vicreg_primary_btcusdt")
      saw_train = true;
    if (bind_id == "bind_eval_vicreg_payload_btcusdt")
      saw_eval = true;
    if (bind_id == "bind_eval_vicreg_payload_btcusdt_untouched_test")
      saw_untouched = true;
  }
  REQUIRE(campaign_count == 3);
  REQUIRE(saw_train);
  REQUIRE(saw_eval);
  REQUIRE(saw_untouched);
}

static void test_marshal_resume_rejects_mismatched_clarification_artifact() {
  temp_dir_t tmp("test_marshal_resume_rejects_bad_clarification");
  const fs::path cfg_dir = tmp.dir / "cfg";
  const fs::path runtime_root = tmp.dir / "runtime";
  const fs::path global_config = cfg_dir / "global.config";
  const fs::path marshal_dsl = cfg_dir / "default.hero.marshal.dsl";
  const fs::path human_dsl = cfg_dir / "default.hero.human.dsl";
  const fs::path codex_stub = cfg_dir / "codex_stub.sh";
  const fs::path human_identities = cfg_dir / "human_operator_identities";
  const fs::path bad_answer = cfg_dir / "bad_answer.json";
  const fs::path source_objective_dsl =
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.train/"
      "vicreg.solo.train.marshal.dsl";

  write_text_file(human_identities, "# smoke-only\n");
  const std::string codex_stub_text = std::string(
                                          R"CODEX(#!/usr/bin/env bash
set -euo pipefail
state_file="$(dirname "$0")/codex_stub.state"
decision_path=""
resume_mode=0
for ((i=1;i<=$#;i++)); do
  arg="${!i}"
  if [[ "$arg" == "resume" ]]; then resume_mode=1; fi
  if [[ "$arg" == "-o" ]]; then
    j=$((i+1))
    decision_path="${!j}"
  fi
done
if [[ -z "$decision_path" ]]; then
  echo "missing -o decision path" >&2
  exit 2
fi
printf 'session id: %s\n' ")CODEX") + kSmokeCodexSessionId +
                                      R"CODEX(" >&2
step=0
if [[ -f "$state_file" ]]; then
  step="$(cat "$state_file")"
fi
if [[ "$resume_mode" == "0" && "$step" == "0" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"pause_for_clarification","reply_text":"","launch":null,"clarification_request":{"request":"Provide the missing clarification."},"governance":null,"reason":"need clarification","memory_note":"asked for clarification"}
JSON
  echo 1 > "$state_file"
  exit 0
fi
if [[ "$resume_mode" == "1" && "$step" == "1" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"complete","reply_text":"","launch":null,"clarification_request":null,"governance":null,"reason":"clarification accepted","memory_note":"completed after valid clarification"}
JSON
  echo 2 > "$state_file"
  exit 0
fi
echo "unexpected codex stub invocation" >&2
exit 3
)CODEX";
  write_executable_file(codex_stub, codex_stub_text);
  write_text_file(
      global_config,
      std::string("[GENERAL]\n") + "runtime_root=" + runtime_root.string() +
          "\nrepo_root=/cuwacunu\n"
          "default_iitepi_campaign_dsl_filename=/cuwacunu/src/config/"
          "instructions/defaults/default.iitepi.campaign.dsl\n"
          "\n[BNF]\n"
          "iitepi_campaign_grammar_filename=/cuwacunu/src/config/bnf/"
          "iitepi.campaign.bnf\n"
          "marshal_objective_grammar_filename=/cuwacunu/src/config/bnf/"
          "objective.marshal.bnf\n"
          "\n[REAL_HERO]\n"
          "marshal_hero_dsl_filename=" +
          marshal_dsl.string() + "\n" +
          "human_hero_dsl_filename=" + human_dsl.string() + "\n");
  write_text_file(
      marshal_dsl,
      std::string("runtime_hero_binary:path = /bin/true\n") +
          "config_hero_binary:path = /bin/echo\n"
          "lattice_hero_binary:path = /bin/cat\n"
          "human_hero_binary:path = /cuwacunu/.build/hero/hero_human.mcp\n"
          "human_operator_identities:path = " +
          human_identities.string() + "\n" +
          "config_scope_root:path = /cuwacunu/src/config\n" +
          "marshal_codex_binary:path = " + codex_stub.string() + "\n" +
          "marshal_codex_model:str = gpt-5.3-codex-spark\n"
          "marshal_codex_reasoning_effort:str = xhigh\n"
          "tail_default_lines(1,+inf):int = 20\n"
          "poll_interval_ms(100,+inf):int = 100\n"
          "marshal_codex_timeout_sec(1,+inf):int = 5\n"
          "marshal_max_campaign_launches(1,+inf):int = 2\n");
  write_text_file(
      human_dsl,
      "marshal_hero_binary:path = /cuwacunu/.build/hero/hero_marshal.mcp\n"
      "operator_id:str = smoke@local\n"
      "operator_signing_ssh_identity:path = /tmp/unused-human-identity\n");

  const std::string start_args =
      std::string("{\"marshal_objective_dsl_path\":\"") +
      source_objective_dsl.string() + "\"}";
  const command_result_t start = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.start_session", start_args));
  REQUIRE(start.exit_code == 0);
  const std::string session_id =
      extract_json_string_field(start.output, "marshal_session_id");
  REQUIRE(!session_id.empty());

  const std::string get_args =
      std::string("{\"marshal_session_id\":\"") + session_id + "\"}";
  const std::string get_command =
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.get_session", get_args);
  const command_result_t paused =
      wait_for_command(get_command, [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"work_gate\":\"clarification\"");
      });
  REQUIRE(paused.exit_code == 0);

  write_text_file(bad_answer,
                  "{\"schema\":\"hero.human.clarification_answer.v3\","
                  "\"marshal_session_id\":\"marshal.wrong\","
                  "\"checkpoint_index\":999,"
                  "\"answer\":\"bad\"}");
  const std::string resume_args =
      std::string("{\"marshal_session_id\":\"") + session_id +
      "\",\"clarification_answer_path\":\"" + bad_answer.string() + "\"}";
  const command_result_t rejected = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.resume_session", resume_args));
  REQUIRE(rejected.exit_code != 0);
  REQUIRE(contains(rejected.output,
                   "clarification answer marshal_session_id does not match "
                   "paused session"));

  const command_result_t still_paused = run_command(get_command);
  REQUIRE(still_paused.exit_code == 0);
  REQUIRE(contains(still_paused.output, "\"lifecycle\":\"live\""));
  REQUIRE(contains(still_paused.output, "\"work_gate\":\"clarification\""));

  const std::string answer_args =
      std::string("{\"marshal_session_id\":\"") + session_id +
      "\",\"answer\":\"Here is the valid clarification.\"}";
  const command_result_t answered = run_command(
      direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                          "hero.human.answer_request", answer_args));
  REQUIRE(answered.exit_code == 0);

  const command_result_t idled =
      wait_for_command(get_command, [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"activity\":\"review\"") &&
               contains(result.output, "\"finish_reason\":\"success\"");
      });
  REQUIRE(idled.exit_code == 0);
}

static void test_marshal_session_failed_checkpoint_idles_and_continues() {
  temp_dir_t tmp("test_marshal_session_failed_checkpoint");
  const fs::path cfg_dir = tmp.dir / "cfg";
  const fs::path runtime_root = tmp.dir / "runtime";
  const fs::path global_config = cfg_dir / "global.config";
  const fs::path marshal_dsl = cfg_dir / "default.hero.marshal.dsl";
  const fs::path human_dsl = cfg_dir / "default.hero.human.dsl";
  const fs::path codex_stub = cfg_dir / "codex_stub.sh";
  const fs::path human_identities = cfg_dir / "human_operator_identities";
  const fs::path source_objective_dsl =
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.train/"
      "vicreg.solo.train.marshal.dsl";

  write_text_file(human_identities, "# smoke-only\n");
  const std::string codex_stub_text = std::string(
                                          R"CODEX(#!/usr/bin/env bash
set -euo pipefail
state_file="$(dirname "$0")/codex_stub.state"
decision_path=""
resume_mode=0
expect_model="gpt-5.3-codex-spark"
expect_effort='model_reasoning_effort="xhigh"'
have_model=0
have_effort=0
have_json=0
for ((i=1;i<=$#;i++)); do
  arg="${!i}"
  if [[ "$arg" == "resume" ]]; then resume_mode=1; fi
  if [[ "$arg" == "--json" ]]; then have_json=1; fi
  if [[ "$arg" == "-o" ]]; then
    j=$((i+1))
    decision_path="${!j}"
  fi
  if [[ "$arg" == "-m" ]]; then
    j=$((i+1))
    if [[ "${!j}" != "$expect_model" ]]; then
      echo "unexpected codex model: ${!j}" >&2
      exit 4
    fi
    have_model=1
  fi
  if [[ "$arg" == "-c" ]]; then
    j=$((i+1))
    if [[ "${!j}" == "$expect_effort" ]]; then
      have_effort=1
    fi
  fi
done
if [[ -z "$decision_path" ]]; then
  echo "missing -o decision path" >&2
  exit 2
fi
if [[ "$have_model" != "1" || "$have_effort" != "1" ]]; then
  echo "missing codex model/effort arguments" >&2
  exit 4
fi
if [[ "$have_json" != "1" ]]; then
  echo "missing codex --json argument" >&2
  exit 4
fi
printf 'session id: %s\n' ")CODEX") + kSmokeCodexSessionId +
                                      R"CODEX(" >&2
step=0
if [[ -f "$state_file" ]]; then
  step="$(cat "$state_file")"
fi
printf '{"type":"stub_event","step":"%s","resume_mode":%s}\n' "$step" "$resume_mode"
if [[ "$resume_mode" == "0" && "$step" == "0" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"launch_campaign","reply_text":"","launch":{"mode":"run_plan","binding_id":null,"reset_runtime_state":false,"requires_objective_mutation":true},"clarification_request":null,"governance":null,"reason":"launch after same-checkpoint objective edit","memory_note":"need softer augmentation before the next launch"}
JSON
  echo 1 > "$state_file"
  exit 0
fi
if [[ "$resume_mode" == "1" && "$step" == "1" ]]; then
  sleep 2
  exit 0
fi
if [[ "$resume_mode" == "0" && "$step" == "1" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"complete","reply_text":"","launch":null,"clarification_request":null,"governance":null,"reason":"operator continuation succeeded after reviewing the failed checkpoint","memory_note":"recovered after failed checkpoint continuation"}
JSON
  echo 2 > "$state_file"
  exit 0
fi
echo "unexpected codex stub invocation" >&2
exit 3
)CODEX";
  write_executable_file(codex_stub, codex_stub_text);
  write_text_file(
      global_config,
      std::string("[GENERAL]\n") + "runtime_root=" + runtime_root.string() +
          "\nrepo_root=/cuwacunu\n"
          "default_iitepi_campaign_dsl_filename=/cuwacunu/src/config/"
          "instructions/defaults/default.iitepi.campaign.dsl\n"
          "\n[BNF]\n"
          "iitepi_campaign_grammar_filename=/cuwacunu/src/config/bnf/"
          "iitepi.campaign.bnf\n"
          "marshal_objective_grammar_filename=/cuwacunu/src/config/bnf/"
          "objective.marshal.bnf\n"
          "\n[REAL_HERO]\n"
          "marshal_hero_dsl_filename=" +
          marshal_dsl.string() + "\n" +
          "human_hero_dsl_filename=" + human_dsl.string() + "\n");
  write_text_file(
      marshal_dsl,
      std::string("runtime_hero_binary:path = /bin/true\n") +
          "config_hero_binary:path = /bin/true\n"
          "lattice_hero_binary:path = /bin/true\n"
          "human_hero_binary:path = /cuwacunu/.build/hero/hero_human.mcp\n"
          "human_operator_identities:path = " +
          human_identities.string() + "\n" +
          "config_scope_root:path = /cuwacunu/src/config\n"
          "marshal_codex_binary:path = " +
          codex_stub.string() +
          "\n"
          "marshal_codex_model:str = gpt-5.3-codex-spark\n"
          "marshal_codex_reasoning_effort:str = xhigh\n"
          "tail_default_lines(1,+inf):int = 20\n"
          "poll_interval_ms(100,+inf):int = 100\n"
          "marshal_codex_timeout_sec(1,+inf):int = 1\n"
          "marshal_max_campaign_launches(1,+inf):int = 2\n");
  write_text_file(
      human_dsl,
      "marshal_hero_binary:path = /cuwacunu/.build/hero/hero_marshal.mcp\n"
      "operator_id:str = smoke@local\n"
      "operator_signing_ssh_identity:path = /tmp/unused-human-identity\n");

  const std::string start_args =
      std::string("{\"marshal_objective_dsl_path\":\"") +
      source_objective_dsl.string() + "\"}";
  const command_result_t start = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.start_session", start_args));
  REQUIRE(start.exit_code == 0);
  REQUIRE(contains(start.output, "\"isError\":false"));
  const std::string session_id =
      extract_json_string_field(start.output, "marshal_session_id");
  REQUIRE(!session_id.empty());

  const std::string get_args =
      std::string("{\"marshal_session_id\":\"") + session_id + "\"}";
  const std::string get_command =
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.get_session", get_args);
  const command_result_t idled_failed =
      wait_for_command(get_command, [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"activity\":\"review\"") &&
               contains(result.output, "\"finish_reason\":\"failed\"") &&
               contains(result.output, "\"checkpoint_count\":1");
      });
  REQUIRE(idled_failed.exit_code == 0);
  REQUIRE(contains(idled_failed.output,
                   "\"last_codex_action\":\"launch_campaign\""));
  const std::string failed_intent_path = extract_json_string_field(
      idled_failed.output, "last_intent_checkpoint_path");
  REQUIRE(!failed_intent_path.empty());
  REQUIRE(contains(idled_failed.output,
                   "launch.requires_objective_mutation=true but no "
                   "same-checkpoint objective mutation was materialized"));
  REQUIRE(contains(idled_failed.output,
                   "session parked for review so the operator can message it "
                   "after adjusting objective files or tooling"));

  const fs::path session_root = runtime_root / ".marshal_hero" / session_id;
  const fs::path codex_stdout_log =
      session_root / "logs" / "codex.session.stdout.jsonl";
  const fs::path codex_stderr_log =
      session_root / "logs" / "codex.session.stderr.jsonl";
  REQUIRE(fs::exists(codex_stdout_log));
  REQUIRE(fs::exists(codex_stderr_log));
  REQUIRE(!fs::exists(session_root / "logs" / "codex.session.log"));
  REQUIRE(
      contains(read_text_file(codex_stdout_log), "\"type\":\"stub_event\""));
  REQUIRE(contains(read_text_file(codex_stderr_log), "\"stream\":\"stderr\""));
  REQUIRE(contains(read_text_file(codex_stderr_log), kSmokeCodexSessionId));
  const std::string summary_text =
      read_text_file(session_root / "human" / "summary.latest.md");
  REQUIRE(contains(summary_text, "This report is actionable."));
  REQUIRE(contains(summary_text, "launch.requires_objective_mutation=true"));
  REQUIRE(contains(summary_text, "Checkpoints: 1"));

  const std::string continue_args =
      std::string("{\"marshal_session_id\":\"") + session_id +
      "\",\"message\":\"Continue after reviewing the failed mutation "
      "checkpoint.\"}";
  const command_result_t continued = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.message_session", continue_args));
  REQUIRE(continued.exit_code == 0);
  REQUIRE(contains(continued.output, "\"isError\":false"));

  const command_result_t idled_success =
      wait_for_command(get_command, [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"activity\":\"review\"") &&
               contains(result.output, "\"finish_reason\":\"success\"") &&
               contains(result.output, "\"checkpoint_count\":2");
      });
  REQUIRE(idled_success.exit_code == 0);
  REQUIRE(contains(idled_success.output,
                   "codex resume degraded: checkpoint=2 "
                   "kind=resume_command_failed fallback=fresh_checkpoint"));
  REQUIRE(contains(idled_success.output, "resume_timeout=true"));

  const std::string events_text = wait_for_file_containing(
      session_root / "marshal.session.events.jsonl",
      "\"type\":\"operator.message_handled\"");
  REQUIRE(contains(events_text, "\"type\":\"checkpoint.failed\""));
  REQUIRE(contains(events_text, "\"type\":\"codex.resume_failed\""));
  REQUIRE(contains(events_text, "\"type\":\"operator.message_received\""));
  REQUIRE(contains(events_text, "\"type\":\"operator.message_delivered\""));
  REQUIRE(contains(events_text, "\"type\":\"operator.message_handled\""));

  const std::string memory_text =
      read_text_file(session_root / "marshal.session.memory.md");
  REQUIRE(contains(memory_text, "## Checkpoint 1"));
  REQUIRE(contains(memory_text, "## Checkpoint 2"));
}

static void
test_marshal_pause_session_retains_active_campaign_until_runtime_stop_finishes() {
  temp_dir_t tmp("test_marshal_pause_session_retains_active_campaign");
  const fs::path cfg_dir = tmp.dir / "cfg";
  const fs::path runtime_root = tmp.dir / "runtime";
  const fs::path global_config = cfg_dir / "global.config";
  const fs::path marshal_dsl = cfg_dir / "default.hero.marshal.dsl";
  const fs::path human_identities = cfg_dir / "human_operator_identities";
  const fs::path runtime_stub = cfg_dir / "runtime_stub.sh";
  const fs::path runtime_trace = cfg_dir / "runtime_stop_args.json";
  const std::string session_id = "marshal.pause.pending";
  const std::string campaign_cursor = "campaign.pause.pending";
  const fs::path session_hero_marshal_dsl =
      runtime_root / ".marshal_hero" / session_id / "hero.marshal.dsl";

  write_text_file(human_identities, "# smoke-only\n");
  const std::string runtime_stub_text = std::string(
                                            R"RUNTIME(#!/usr/bin/env bash
set -euo pipefail
trace_file=")RUNTIME") + runtime_trace.string() +
                                        R"RUNTIME("
tool_name=""
args_json="{}"
for ((i=1;i<=$#;i++)); do
  arg="${!i}"
  if [[ "$arg" == "--tool" ]]; then
    j=$((i+1))
    tool_name="${!j}"
  fi
  if [[ "$arg" == "--args-json" ]]; then
    j=$((i+1))
    args_json="${!j}"
  fi
done
if [[ "$tool_name" != "hero.runtime.stop_campaign" ]]; then
  printf '{"content":[{"type":"text","text":"unsupported tool"}],"structuredContent":{},"isError":true}\n'
  exit 1
fi
printf '%s' "$args_json" > "$trace_file"
grep -F '"suppress_marshal_session_update":true' <<<"$args_json" >/dev/null || {
  printf '{"content":[{"type":"text","text":"missing suppress flag"}],"structuredContent":{},"isError":true}\n'
  exit 2
}
campaign_cursor="$(sed -n 's/.*"campaign_cursor":"\([^"]*\)".*/\1/p' <<<"$args_json" | head -n 1)"
if [[ -z "$campaign_cursor" ]]; then
  printf '{"content":[{"type":"text","text":"missing campaign cursor"}],"structuredContent":{},"isError":true}\n'
  exit 3
fi
printf '{"content":[{"type":"text","text":"ok"}],"structuredContent":{"campaign_cursor":"%s","campaign":{"schema":"hero.runtime.campaign.v2","campaign_cursor":"%s","state":"stopping","state_detail":"stub stop in progress","marshal_session_id":"marshal.pause.pending"},"warnings":[]},"isError":false}\n' \
  "$campaign_cursor" "$campaign_cursor"
)RUNTIME";
  write_executable_file(runtime_stub, runtime_stub_text);

  write_text_file(
      global_config,
      std::string("[GENERAL]\n") + "runtime_root=" + runtime_root.string() +
          "\nrepo_root=/cuwacunu\n"
          "default_iitepi_campaign_dsl_filename=/cuwacunu/src/config/"
          "instructions/defaults/default.iitepi.campaign.dsl\n"
          "\n[BNF]\n"
          "iitepi_campaign_grammar_filename=/cuwacunu/src/config/bnf/"
          "iitepi.campaign.bnf\n"
          "marshal_objective_grammar_filename=/cuwacunu/src/config/bnf/"
          "objective.marshal.bnf\n"
          "\n[REAL_HERO]\n"
          "marshal_hero_dsl_filename=" +
          marshal_dsl.string() + "\n");
  const std::string marshal_dsl_text =
      std::string("runtime_hero_binary:path = ") + runtime_stub.string() +
      "\n"
      "config_hero_binary:path = /bin/true\n"
      "lattice_hero_binary:path = /bin/true\n"
      "human_hero_binary:path = /bin/true\n"
      "human_operator_identities:path = " +
      human_identities.string() + "\n" +
      "config_scope_root:path = /cuwacunu/src/config\n"
      "marshal_codex_binary:path = /bin/true\n"
      "marshal_codex_model:str = gpt-5.3-codex-spark\n"
      "marshal_codex_reasoning_effort:str = xhigh\n"
      "tail_default_lines(1,+inf):int = 20\n"
      "poll_interval_ms(100,+inf):int = 100\n"
      "marshal_codex_timeout_sec(1,+inf):int = 5\n"
      "marshal_max_campaign_launches(1,+inf):int = 2\n";
  write_text_file(marshal_dsl, marshal_dsl_text);
  write_text_file(session_hero_marshal_dsl, marshal_dsl_text);

  const live_process_identity_t current_identity = current_process_identity();
  write_manual_running_campaign_fixture(runtime_root, global_config,
                                        session_hero_marshal_dsl, session_id,
                                        campaign_cursor, current_identity);
  {
    const held_file_lock_t session_runner_lock(
        cuwacunu::hero::marshal::marshal_session_runner_lock_path(
            runtime_root / ".marshal_hero", session_id));

    const std::string pause_args = std::string("{\"marshal_session_id\":\"") +
                                   session_id + "\",\"force\":false}";
    const command_result_t paused = run_command(
        direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                            "hero.marshal.pause_session", pause_args));
    REQUIRE(paused.exit_code == 0);
    REQUIRE(contains(paused.output, "\"isError\":false"));
    REQUIRE(contains(paused.output, "\"lifecycle\":\"live\""));
    REQUIRE(contains(paused.output, "\"work_gate\":\"operator_pause\""));
    REQUIRE(contains(paused.output, "\"status_detail\":\"pause_session "
                                    "requested by operator; active runtime "
                                    "campaign stop in progress\""));
    REQUIRE(contains(paused.output, std::string("\"campaign_cursor\":\"") +
                                        campaign_cursor + "\""));
    REQUIRE(contains(read_text_file(runtime_trace),
                     "\"suppress_marshal_session_update\":true"));

    const std::string resume_args =
        std::string("{\"marshal_session_id\":\"") + session_id + "\"}";
    const command_result_t rejected = run_command(
        direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                            "hero.marshal.resume_session", resume_args));
    REQUIRE(rejected.exit_code != 0);
    REQUIRE(contains(
        rejected.output,
        "resume_session cannot reopen an operator-paused session while the "
        "retained active runtime campaign stop is still in progress"));
  }

  mark_runtime_campaign_terminal(runtime_root, global_config, session_id,
                                 campaign_cursor);
  const std::string get_args =
      std::string("{\"marshal_session_id\":\"") + session_id + "\"}";
  const command_result_t reconciled = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.get_session", get_args));
  REQUIRE(reconciled.exit_code == 0);
  REQUIRE(contains(reconciled.output, "\"reconciled\":true"));
  REQUIRE(contains(reconciled.output, "\"lifecycle\":\"live\""));
  REQUIRE(contains(reconciled.output, "\"work_gate\":\"operator_pause\""));
  REQUIRE(
      contains(reconciled.output,
               "\"status_detail\":\"pause_session requested by operator\""));
  REQUIRE(contains(reconciled.output, "\"campaign_cursor\":\"\""));
}

static void
test_marshal_terminate_session_retains_active_campaign_until_runtime_stop_finishes() {
  temp_dir_t tmp("test_marshal_terminate_session_retains_active_campaign");
  const fs::path cfg_dir = tmp.dir / "cfg";
  const fs::path runtime_root = tmp.dir / "runtime";
  const fs::path global_config = cfg_dir / "global.config";
  const fs::path marshal_dsl = cfg_dir / "default.hero.marshal.dsl";
  const fs::path human_identities = cfg_dir / "human_operator_identities";
  const fs::path runtime_stub = cfg_dir / "runtime_stub.sh";
  const fs::path runtime_trace = cfg_dir / "runtime_stop_args.json";
  const std::string session_id = "marshal.terminate.pending";
  const std::string campaign_cursor = "campaign.terminate.pending";
  const fs::path session_hero_marshal_dsl =
      runtime_root / ".marshal_hero" / session_id / "hero.marshal.dsl";

  write_text_file(human_identities, "# smoke-only\n");
  const std::string runtime_stub_text = std::string(
                                            R"RUNTIME(#!/usr/bin/env bash
set -euo pipefail
trace_file=")RUNTIME") + runtime_trace.string() +
                                        R"RUNTIME("
tool_name=""
args_json="{}"
for ((i=1;i<=$#;i++)); do
  arg="${!i}"
  if [[ "$arg" == "--tool" ]]; then
    j=$((i+1))
    tool_name="${!j}"
  fi
  if [[ "$arg" == "--args-json" ]]; then
    j=$((i+1))
    args_json="${!j}"
  fi
done
if [[ "$tool_name" != "hero.runtime.stop_campaign" ]]; then
  printf '{"content":[{"type":"text","text":"unsupported tool"}],"structuredContent":{},"isError":true}\n'
  exit 1
fi
printf '%s' "$args_json" > "$trace_file"
grep -F '"suppress_marshal_session_update":true' <<<"$args_json" >/dev/null || {
  printf '{"content":[{"type":"text","text":"missing suppress flag"}],"structuredContent":{},"isError":true}\n'
  exit 2
}
campaign_cursor="$(sed -n 's/.*"campaign_cursor":"\([^"]*\)".*/\1/p' <<<"$args_json" | head -n 1)"
if [[ -z "$campaign_cursor" ]]; then
  printf '{"content":[{"type":"text","text":"missing campaign cursor"}],"structuredContent":{},"isError":true}\n'
  exit 3
fi
printf '{"content":[{"type":"text","text":"ok"}],"structuredContent":{"campaign_cursor":"%s","campaign":{"schema":"hero.runtime.campaign.v2","campaign_cursor":"%s","state":"stopping","state_detail":"stub stop in progress","marshal_session_id":"marshal.terminate.pending"},"warnings":[]},"isError":false}\n' \
  "$campaign_cursor" "$campaign_cursor"
)RUNTIME";
  write_executable_file(runtime_stub, runtime_stub_text);

  write_text_file(
      global_config,
      std::string("[GENERAL]\n") + "runtime_root=" + runtime_root.string() +
          "\nrepo_root=/cuwacunu\n"
          "default_iitepi_campaign_dsl_filename=/cuwacunu/src/config/"
          "instructions/defaults/default.iitepi.campaign.dsl\n"
          "\n[BNF]\n"
          "iitepi_campaign_grammar_filename=/cuwacunu/src/config/bnf/"
          "iitepi.campaign.bnf\n"
          "marshal_objective_grammar_filename=/cuwacunu/src/config/bnf/"
          "objective.marshal.bnf\n"
          "\n[REAL_HERO]\n"
          "marshal_hero_dsl_filename=" +
          marshal_dsl.string() + "\n");
  const std::string marshal_dsl_text =
      std::string("runtime_hero_binary:path = ") + runtime_stub.string() +
      "\n"
      "config_hero_binary:path = /bin/true\n"
      "lattice_hero_binary:path = /bin/true\n"
      "human_hero_binary:path = /bin/true\n"
      "human_operator_identities:path = " +
      human_identities.string() + "\n" +
      "config_scope_root:path = /cuwacunu/src/config\n"
      "marshal_codex_binary:path = /bin/true\n"
      "marshal_codex_model:str = gpt-5.3-codex-spark\n"
      "marshal_codex_reasoning_effort:str = xhigh\n"
      "tail_default_lines(1,+inf):int = 20\n"
      "poll_interval_ms(100,+inf):int = 100\n"
      "marshal_codex_timeout_sec(1,+inf):int = 5\n"
      "marshal_max_campaign_launches(1,+inf):int = 2\n";
  write_text_file(marshal_dsl, marshal_dsl_text);
  write_text_file(session_hero_marshal_dsl, marshal_dsl_text);

  const live_process_identity_t current_identity = current_process_identity();
  write_manual_running_campaign_fixture(runtime_root, global_config,
                                        session_hero_marshal_dsl, session_id,
                                        campaign_cursor, current_identity);
  {
    const held_file_lock_t session_runner_lock(
        cuwacunu::hero::marshal::marshal_session_runner_lock_path(
            runtime_root / ".marshal_hero", session_id));

    const std::string terminate_args =
        std::string("{\"marshal_session_id\":\"") + session_id +
        "\",\"force\":false}";
    const command_result_t terminated = run_command(
        direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                            "hero.marshal.terminate_session", terminate_args));
    REQUIRE(terminated.exit_code == 0);
    REQUIRE(contains(terminated.output, "\"isError\":false"));
    REQUIRE(contains(terminated.output, "\"lifecycle\":\"terminal\""));
    REQUIRE(contains(terminated.output, "\"finish_reason\":\"terminated\""));
    REQUIRE(contains(
        terminated.output,
        "\"status_detail\":\"terminate_session requested by operator; active "
        "runtime campaign stop in progress\""));
    REQUIRE(contains(terminated.output, std::string("\"campaign_cursor\":\"") +
                                            campaign_cursor + "\""));
    REQUIRE(contains(read_text_file(runtime_trace),
                     "\"suppress_marshal_session_update\":true"));
  }

  mark_runtime_campaign_terminal(runtime_root, global_config, session_id,
                                 campaign_cursor);
  const std::string get_args =
      std::string("{\"marshal_session_id\":\"") + session_id + "\"}";
  const command_result_t reconciled = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.get_session", get_args));
  REQUIRE(reconciled.exit_code == 0);
  REQUIRE(contains(reconciled.output, "\"reconciled\":true"));
  REQUIRE(contains(reconciled.output, "\"lifecycle\":\"terminal\""));
  REQUIRE(contains(reconciled.output, "\"finish_reason\":\"terminated\""));
  REQUIRE(contains(
      reconciled.output,
      "\"status_detail\":\"terminate_session requested by operator\""));
  REQUIRE(contains(reconciled.output, "\"campaign_cursor\":\"\""));
}

static void test_marshal_session_reconcile_rebooted_running_campaign() {
  temp_dir_t tmp("test_marshal_session_reconcile_rebooted_running_campaign");
  const fs::path cfg_dir = tmp.dir / "cfg";
  const fs::path runtime_root = tmp.dir / "runtime";
  const fs::path global_config = cfg_dir / "global.config";
  const fs::path marshal_dsl = cfg_dir / "default.hero.marshal.dsl";
  const fs::path human_identities = cfg_dir / "human_operator_identities";
  const fs::path marshal_root = runtime_root / ".marshal_hero";
  const fs::path campaigns_root = runtime_root / ".campaigns";
  const std::string session_id = "marshal.rebooted";
  const std::string campaign_cursor = "campaign.rebooted";
  const std::uint64_t now_ms = cuwacunu::hero::runtime::now_ms_utc();

  write_text_file(human_identities, "# smoke-only\n");
  write_text_file(
      global_config,
      std::string("[GENERAL]\n") + "runtime_root=" + runtime_root.string() +
          "\nrepo_root=/cuwacunu\n"
          "default_iitepi_campaign_dsl_filename=/cuwacunu/src/config/"
          "instructions/defaults/default.iitepi.campaign.dsl\n"
          "\n[BNF]\n"
          "iitepi_campaign_grammar_filename=/cuwacunu/src/config/bnf/"
          "iitepi.campaign.bnf\n"
          "marshal_objective_grammar_filename=/cuwacunu/src/config/bnf/"
          "objective.marshal.bnf\n"
          "\n[REAL_HERO]\n"
          "marshal_hero_dsl_filename=" +
          marshal_dsl.string() + "\n");
  write_text_file(marshal_dsl,
                  std::string("runtime_hero_binary:path = /bin/true\n") +
                      "config_hero_binary:path = /bin/true\n"
                      "lattice_hero_binary:path = /bin/true\n"
                      "human_hero_binary:path = /bin/true\n"
                      "human_operator_identities:path = " +
                      human_identities.string() + "\n" +
                      "config_scope_root:path = /cuwacunu/src/config\n"
                      "marshal_codex_binary:path = /bin/true\n"
                      "marshal_codex_model:str = gpt-5.3-codex-spark\n"
                      "marshal_codex_reasoning_effort:str = xhigh\n"
                      "tail_default_lines(1,+inf):int = 20\n"
                      "poll_interval_ms(100,+inf):int = 100\n"
                      "marshal_codex_timeout_sec(1,+inf):int = 5\n"
                      "marshal_max_campaign_launches(1,+inf):int = 2\n");

  fs::create_directories(marshal_root / session_id / "logs");
  fs::create_directories(marshal_root / session_id / "human");
  fs::create_directories(marshal_root / session_id / "checkpoints");
  fs::create_directories(campaigns_root / campaign_cursor / "jobs");

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  session.schema =
      std::string(cuwacunu::hero::marshal::kMarshalSessionSchemaV6);
  session.marshal_session_id = session_id;
  session.lifecycle = "live";
  session.status_detail = "campaign launched from marshal planning checkpoint";
  session.work_gate = "open";
  session.activity = "awaiting_campaign_fact";
  session.finish_reason = "none";
  session.objective_name = "runtime.operative.vicreg.solo.train";
  session.global_config_path = global_config.string();
  session.boot_id = "stale-boot-id";
  session.runner_pid = 999999;
  session.runner_start_ticks = 1;
  session.source_marshal_objective_dsl_path =
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.train/"
      "vicreg.solo.train.marshal.dsl";
  session.source_campaign_dsl_path =
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.train/"
      "iitepi.campaign.dsl";
  session.source_marshal_objective_md_path =
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.train/"
      "vicreg.solo.train.objective.md";
  session.source_marshal_guidance_md_path =
      "/cuwacunu/src/config/instructions/defaults/default.runtime.operative.guidance.md";
  session.session_root = (marshal_root / session_id).string();
  session.objective_root =
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.train";
  session.campaign_dsl_path = session.source_campaign_dsl_path;
  session.marshal_objective_dsl_path =
      (marshal_root / session_id / "marshal.objective.dsl").string();
  session.marshal_objective_md_path =
      (marshal_root / session_id / "marshal.objective.md").string();
  session.marshal_guidance_md_path =
      (marshal_root / session_id / "marshal.guidance.md").string();
  session.hero_marshal_dsl_path =
      (marshal_root / session_id / "hero.marshal.dsl").string();
  session.config_policy_path =
      (marshal_root / session_id / "config.hero.policy.dsl").string();
  session.briefing_path =
      (marshal_root / session_id / "marshal.briefing.md").string();
  session.memory_path =
      (marshal_root / session_id / "marshal.session.memory.md").string();
  session.human_request_path =
      (marshal_root / session_id / "human" / "request.latest.md").string();
  session.events_path =
      (marshal_root / session_id / "marshal.session.events.jsonl").string();
  session.turns_path =
      (marshal_root / session_id / "marshal.session.turns.jsonl").string();
  session.codex_stdout_path =
      (marshal_root / session_id / "logs" / "codex.session.stdout.jsonl")
          .string();
  session.codex_stderr_path =
      (marshal_root / session_id / "logs" / "codex.session.stderr.jsonl")
          .string();
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
  session.campaign_cursor = campaign_cursor;
  session.last_codex_action = "launch_campaign";
  session.last_input_checkpoint_path =
      (marshal_root / session_id / "checkpoints" / "input.0001.json").string();
  session.campaign_cursors.push_back(campaign_cursor);
  std::string session_error{};
  REQUIRE(cuwacunu::hero::marshal::write_marshal_session_record(
      marshal_root, session, &session_error));

  cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
  campaign.campaign_cursor = campaign_cursor;
  campaign.boot_id = "stale-boot-id";
  campaign.state = "running";
  campaign.state_detail = "campaign runner alive";
  campaign.marshal_session_id = session_id;
  campaign.global_config_path = global_config.string();
  campaign.source_campaign_dsl_path = session.source_campaign_dsl_path;
  campaign.campaign_dsl_path =
      (campaigns_root / campaign_cursor / "campaign.dsl").string();
  campaign.started_at_ms = now_ms;
  campaign.updated_at_ms = now_ms;
  campaign.runner_pid = 999998;
  campaign.runner_start_ticks = 1;
  std::string campaign_error{};
  REQUIRE(cuwacunu::hero::runtime::write_runtime_campaign_record(
      campaigns_root, campaign, &campaign_error));

  const std::string get_args =
      std::string("{\"marshal_session_id\":\"") + session_id + "\"}";
  const command_result_t get = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.get_session", get_args));
  REQUIRE(get.exit_code == 0);
  REQUIRE(contains(get.output, "\"isError\":false"));
  REQUIRE(contains(get.output, "\"reconciled\":true"));
  REQUIRE(contains(get.output, "\"schema\":\"hero.marshal.session.v6\""));
  REQUIRE(contains(get.output, "\"lifecycle\":\"live\""));
  REQUIRE(contains(get.output, "\"activity\":\"review\""));
  REQUIRE(contains(get.output, "\"finish_reason\":\"interrupted\""));
  REQUIRE(contains(get.output, campaign_cursor));
  REQUIRE(contains(get.output, "campaign runner is no longer alive"));

  const command_result_t list = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.list_sessions", "{}"));
  REQUIRE(list.exit_code == 0);
  REQUIRE(contains(list.output, "\"isError\":false"));
  REQUIRE(contains(list.output, session_id));
  REQUIRE(contains(list.output, "\"lifecycle\":\"live\""));
  REQUIRE(contains(list.output, "\"activity\":\"review\""));
  REQUIRE(contains(list.output, "\"finish_reason\":\"interrupted\""));

  const std::string manifest_text = read_text_file(
      marshal_root / session_id / "marshal.session.manifest.lls");
  REQUIRE(contains(manifest_text, "schema:str = hero.marshal.session.v6"));
  REQUIRE(contains(manifest_text, "lifecycle:str = live"));
  REQUIRE(contains(manifest_text, "activity:str = review"));
  REQUIRE(contains(manifest_text, "finish_reason:str = interrupted"));
  REQUIRE(!contains(manifest_text,
                    std::string("campaign_cursor:str = ") + campaign_cursor));
}

static void
test_marshal_session_canonicalize_clears_stale_campaign_status_without_cursor() {
  cuwacunu::hero::marshal::marshal_session_record_t review_ready{};
  review_ready.lifecycle = "live";
  review_ready.activity = "review";
  review_ready.work_gate = "open";
  review_ready.campaign_status = "running";
  cuwacunu::hero::marshal::canonicalize_marshal_session_record(&review_ready);
  REQUIRE(review_ready.campaign_status == "none");

  cuwacunu::hero::marshal::marshal_session_record_t work_blocked{};
  work_blocked.lifecycle = "live";
  work_blocked.activity = "quiet";
  work_blocked.work_gate = "clarification";
  work_blocked.campaign_status = "stopping";
  cuwacunu::hero::marshal::canonicalize_marshal_session_record(&work_blocked);
  REQUIRE(work_blocked.campaign_status == "none");

  cuwacunu::hero::marshal::marshal_session_record_t terminal{};
  terminal.lifecycle = "terminal";
  terminal.activity = "quiet";
  terminal.work_gate = "open";
  terminal.campaign_status = "running";
  cuwacunu::hero::marshal::canonicalize_marshal_session_record(&terminal);
  REQUIRE(terminal.campaign_status == "none");

  cuwacunu::hero::marshal::marshal_session_record_t planning{};
  planning.lifecycle = "live";
  planning.activity = "planning";
  planning.work_gate = "open";
  planning.campaign_status = "running";
  cuwacunu::hero::marshal::canonicalize_marshal_session_record(&planning);
  REQUIRE(planning.campaign_status == "running");
}

int main() {
  test_hashimyei_tools_list_surface();
  test_lattice_tools_list_surface();
  test_marshal_tools_list_surface();
  test_human_tools_list_surface();
  test_hashimyei_reset_catalog_smoke();
  test_hashimyei_list_direct_tool_skips_ingest_rebuild();
  test_lattice_reset_catalog_smoke();
  test_lattice_removed_tool_rejected();
  test_marshal_session_continue_smoke();
  test_marshal_run_plan_replans_between_bind_steps();
  test_marshal_resume_rejects_mismatched_clarification_artifact();
  test_marshal_session_failed_checkpoint_idles_and_continues();
  test_marshal_pause_session_retains_active_campaign_until_runtime_stop_finishes();
  test_marshal_terminate_session_retains_active_campaign_until_runtime_stop_finishes();
  test_marshal_session_reconcile_rebooted_running_campaign();
  test_marshal_session_canonicalize_clears_stale_campaign_status_without_cursor();
  std::cout << "[OK] hero mcp tool surface and smoke checks passed\n";
  return 0;
}
