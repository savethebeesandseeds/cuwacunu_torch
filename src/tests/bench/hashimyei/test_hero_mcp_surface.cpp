#include <cstdio>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

static void require_impl(bool ok, const char* expr, const char* file, int line) {
  if (!ok) {
    std::cerr << "[TEST FAIL] " << file << ":" << line << "  (" << expr << ")\n";
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

static command_result_t run_command(const std::string& command) {
  command_result_t out{};
  const std::string full = command + " 2>&1";
  FILE* pipe = ::popen(full.c_str(), "r");
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

static constexpr const char* kHashimyeiMcpBin =
    "/cuwacunu/.build/hero/hero_hashimyei_mcp";
static constexpr const char* kLatticeMcpBin = "/cuwacunu/.build/hero/hero_lattice_mcp";
static constexpr const char* kMarshalMcpBin = "/cuwacunu/.build/hero/hero_marshal_mcp";
static constexpr const char* kHumanMcpBin = "/cuwacunu/.build/hero/hero_human_mcp";
static constexpr const char* kGlobalConfig = "/cuwacunu/src/config/.config";
static constexpr const char* kSmokeCodexSessionId =
    "11111111-1111-1111-1111-111111111111";

static std::string tools_list_command(const fs::path& binary_path,
                                      const fs::path& store_root,
                                      const fs::path& catalog_path) {
  const std::string req =
      R"({"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}})";
  std::ostringstream cmd;
  cmd << "req=" << shell_quote(req) << "; "
      << "{ printf 'Content-Length: %d\\r\\n\\r\\n%s' \"${#req}\" \"$req\"; } | "
      << shell_quote(binary_path.string()) << " --global-config "
      << shell_quote(kGlobalConfig) << " --store-root "
      << shell_quote(store_root.string()) << " --catalog "
      << shell_quote(catalog_path.string());
  return cmd.str();
}

static std::string list_tools_json_command(const fs::path& binary_path,
                                           const fs::path& global_config,
                                           const fs::path& hero_config = {}) {
  std::ostringstream cmd;
  cmd << shell_quote(binary_path.string()) << " --global-config "
      << shell_quote(global_config.string());
  if (!hero_config.empty()) {
    cmd << " --hero-config " << shell_quote(hero_config.string());
  }
  cmd << " --list-tools-json";
  return cmd.str();
}

static std::string direct_tool_command(const fs::path& binary_path,
                                       const fs::path& global_config,
                                       const fs::path& hero_config,
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

static std::string direct_tool_command_with_paths(const fs::path& binary_path,
                                                  const fs::path& global_config,
                                                  const fs::path& store_root,
                                                  const fs::path& catalog_path,
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

static void write_text_file(const fs::path& path, std::string_view text) {
  fs::create_directories(path.parent_path());
  std::ofstream out(path);
  REQUIRE(out.is_open());
  out << text;
  out.close();
  REQUIRE(out.good());
}

static void write_executable_file(const fs::path& path, std::string_view text) {
  write_text_file(path, text);
  fs::permissions(path,
                  fs::perms::owner_read | fs::perms::owner_write |
                      fs::perms::owner_exec | fs::perms::group_read |
                      fs::perms::group_exec | fs::perms::others_read |
                      fs::perms::others_exec,
                  fs::perm_options::replace);
}

static std::string read_text_file(const fs::path& path) {
  std::ifstream in(path);
  REQUIRE(in.is_open());
  std::ostringstream out;
  out << in.rdbuf();
  return out.str();
}

static std::size_t count_occurrences(std::string_view haystack,
                                     std::string_view needle) {
  if (needle.empty()) return 0;
  std::size_t count = 0;
  std::size_t pos = 0;
  for (;;) {
    pos = haystack.find(needle, pos);
    if (pos == std::string_view::npos) return count;
    ++count;
    pos += needle.size();
  }
}

static std::string extract_json_string_field(std::string_view json,
                                             std::string_view key) {
  const std::string marker = "\"" + std::string(key) + "\":\"";
  const std::size_t begin = json.find(marker);
  if (begin == std::string_view::npos) return {};
  const std::size_t value_begin = begin + marker.size();
  const std::size_t value_end = json.find('"', value_begin);
  if (value_end == std::string_view::npos) return {};
  return std::string(json.substr(value_begin, value_end - value_begin));
}

template <typename Predicate>
static command_result_t wait_for_command(const std::string& command,
                                         Predicate&& predicate,
                                         int timeout_ms = 10000,
                                         int poll_ms = 100) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  command_result_t last{};
  for (;;) {
    last = run_command(command);
    if (predicate(last)) return last;
    if (std::chrono::steady_clock::now() >= deadline) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(poll_ms));
  }
  return last;
}

static std::string reset_hashimyei_command(const fs::path& store_root,
                                           const fs::path& catalog_path) {
  std::ostringstream cmd;
  cmd << shell_quote(kHashimyeiMcpBin) << " --global-config "
      << shell_quote(kGlobalConfig) << " --store-root "
      << shell_quote(store_root.string()) << " --catalog "
      << shell_quote(catalog_path.string()) << " --tool hero.hashimyei.reset_catalog"
      << " --args-json " << shell_quote(R"({"reingest":false})");
  return cmd.str();
}

static std::string reset_lattice_command(const fs::path& store_root,
                                         const fs::path& catalog_path) {
  std::ostringstream cmd;
  cmd << shell_quote(kLatticeMcpBin) << " --global-config "
      << shell_quote(kGlobalConfig) << " --store-root "
      << shell_quote(store_root.string()) << " --catalog "
      << shell_quote(catalog_path.string())
      << " --tool hero.lattice.refresh"
      << " --args-json "
      << shell_quote(R"({"reingest":false})");
  return cmd.str();
}

static std::string call_removed_lattice_tool_command(
    const fs::path& store_root, const fs::path& catalog_path) {
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

  const command_result_t result = run_command(
      tools_list_command(kHashimyeiMcpBin, store_root, catalog));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"hero.hashimyei.list\""));
  REQUIRE(contains(result.output, "\"hero.hashimyei.get_founding_dsl_bundle\""));
  REQUIRE(contains(result.output, "\"hero.hashimyei.update_rank\""));
  REQUIRE(contains(result.output,
                   "\"required\":[\"family\",\"contract_hash\",\"ordered_hashimyeis\"]"));
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

  const command_result_t result = run_command(
      tools_list_command(kLatticeMcpBin, store_root, catalog));
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
  REQUIRE(contains(result.output, "\"hero.marshal.pause_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.resume_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.continue_session\""));
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
  REQUIRE(!contains(result.output, "\"hero.human.list_sessions\""));
  REQUIRE(!contains(result.output, "\"hero.human.get_session\""));
  REQUIRE(!contains(result.output, "\"hero.human.pause_session\""));
  REQUIRE(!contains(result.output, "\"hero.human.resume_session\""));
  REQUIRE(!contains(result.output, "\"hero.human.continue_session\""));
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

  const command_result_t result = run_command(
      reset_hashimyei_command(store_root, catalog));
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

  const command_result_t result = run_command(
      reset_lattice_command(store_root, catalog));
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

  const command_result_t result = run_command(
      call_removed_lattice_tool_command(store_root, catalog));
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
      "/cuwacunu/src/config/instructions/objectives/vicreg.solo.train/vicreg.solo.train.marshal.dsl";

  write_text_file(human_identities, "# smoke-only\n");
  const std::string codex_stub_text =
      std::string(
          R"CODEX(#!/usr/bin/env bash
set -euo pipefail
state_file="$(dirname "$0")/codex_stub.state"
decision_path=""
schema_path=""
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
if [[ -n "$schema_path" ]] && grep -Eq ',[[:space:]]*[}\]]' "$schema_path"; then
  echo "invalid output schema json" >&2
  exit 4
fi
printf 'session id: %s\n' ")CODEX") +
      kSmokeCodexSessionId +
      R"CODEX(" >&2
step=0
if [[ -f "$state_file" ]]; then
  step="$(cat "$state_file")"
fi
printf '{"type":"stub_event","step":"%s","resume_mode":%s}\n' "$step" "$resume_mode"
if [[ "$resume_mode" == "0" && "$step" == "0" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"pause_for_clarification","reason":"need clarification","memory_note":"asked for clarification","clarification_request":{"request":"Provide the missing clarification."}}
JSON
  echo 1 > "$state_file"
  exit 0
fi
if [[ "$resume_mode" == "1" && "$step" == "1" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"complete","reason":"objective satisfied after clarification","memory_note":"completed after clarification"}
JSON
  echo 2 > "$state_file"
  exit 0
fi
if [[ "$resume_mode" == "1" && "$step" == "2" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"complete","reason":"objective satisfied after continuation","memory_note":"completed after continuation"}
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
          "default_iitepi_campaign_dsl_filename=/cuwacunu/src/config/instructions/defaults/default.iitepi.campaign.dsl\n"
          "\n[BNF]\n"
          "iitepi_campaign_grammar_filename=/cuwacunu/src/config/bnf/iitepi.campaign.bnf\n"
          "marshal_objective_grammar_filename=/cuwacunu/src/config/bnf/objective.marshal.bnf\n"
          "\n[REAL_HERO]\n"
          "marshal_hero_dsl_filename=" + marshal_dsl.string() + "\n" +
          "human_hero_dsl_filename=" + human_dsl.string() + "\n");
  write_text_file(
      marshal_dsl,
      std::string("runtime_hero_binary:path = /bin/true\n") +
          "config_hero_binary:path = /bin/true\n"
          "lattice_hero_binary:path = /bin/true\n"
          "human_hero_binary:path = /cuwacunu/.build/hero/hero_human_mcp\n"
          "human_operator_identities:path = " + human_identities.string() + "\n" +
          "config_scope_root:path = /cuwacunu/src/config\n"
          "marshal_codex_binary:path = " + codex_stub.string() + "\n"
          "marshal_codex_model:str = gpt-5.3-codex-spark\n"
          "marshal_codex_reasoning_effort:str = xhigh\n"
          "tail_default_lines(1,+inf):int = 20\n"
          "poll_interval_ms(100,+inf):int = 100\n"
          "marshal_codex_timeout_sec(1,+inf):int = 5\n"
          "marshal_max_campaign_launches(1,+inf):int = 2\n");
  write_text_file(
      human_dsl,
      "marshal_hero_binary:path = /cuwacunu/.build/hero/hero_marshal_mcp\n"
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
  const command_result_t paused = wait_for_command(
      get_command,
      [](const command_result_t& result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"phase\":\"paused\"") &&
               contains(result.output, "\"pause_kind\":\"clarification\"");
      });
  REQUIRE(paused.exit_code == 0);
  const std::string paused_codex_session =
      extract_json_string_field(paused.output, "codex_session_id");
  REQUIRE(paused_codex_session == kSmokeCodexSessionId);
  REQUIRE(extract_json_string_field(paused.output, "resolved_marshal_codex_model") ==
          "gpt-5.3-codex-spark");
  REQUIRE(extract_json_string_field(
              paused.output, "resolved_marshal_codex_reasoning_effort") ==
          "xhigh");

  const command_result_t pending_requests = run_command(
      direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                          "hero.human.list_requests", "{}"));
  REQUIRE(pending_requests.exit_code == 0);
  REQUIRE(contains(pending_requests.output, "\"isError\":false"));
  REQUIRE(contains(pending_requests.output, session_id));

  const command_result_t human_request = run_command(
      direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                          "hero.human.get_request", get_args));
  REQUIRE(human_request.exit_code == 0);
  REQUIRE(contains(human_request.output, "\"isError\":false"));
  REQUIRE(contains(human_request.output,
                   std::string("\"marshal_session_id\":\"") + session_id + "\""));

  const std::string answer_args =
      std::string("{\"marshal_session_id\":\"") + session_id +
      "\",\"answer\":\"Here is the clarification.\"}";
  const command_result_t answered = run_command(
      direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                          "hero.human.answer_request", answer_args));
  REQUIRE(answered.exit_code == 0);
  REQUIRE(contains(answered.output, "\"isError\":false"));

  const command_result_t idled = wait_for_command(
      get_command,
      [](const command_result_t& result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"phase\":\"idle\"") &&
               contains(result.output, "\"finish_reason\":\"success\"");
      });
  REQUIRE(idled.exit_code == 0);
  REQUIRE(contains(idled.output, "\"checkpoint_count\":2"));
  const std::string idled_codex_session =
      extract_json_string_field(idled.output, "codex_session_id");
  REQUIRE(idled_codex_session == paused_codex_session);

  const command_result_t pending_summaries = run_command(
      direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                          "hero.human.list_summaries", "{}"));
  REQUIRE(pending_summaries.exit_code == 0);
  REQUIRE(contains(pending_summaries.output, "\"isError\":false"));
  REQUIRE(contains(pending_summaries.output, session_id));

  const command_result_t human_get = run_command(
      direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                          "hero.human.get_summary", get_args));
  REQUIRE(human_get.exit_code == 0);
  REQUIRE(contains(human_get.output, "\"isError\":false"));
  REQUIRE(contains(human_get.output, "\"phase\":\"idle\""));

  const std::string continue_args =
      std::string("{\"marshal_session_id\":\"") + session_id +
      "\",\"instruction\":\"Train for 10 more epochs from the latest checkpoint.\"}";
  const command_result_t continued = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.continue_session", continue_args));
  REQUIRE(continued.exit_code == 0);
  REQUIRE(contains(continued.output, "\"isError\":false"));

  const command_result_t idled_again = wait_for_command(
      get_command,
      [](const command_result_t& result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"phase\":\"idle\"") &&
               contains(result.output, "\"finish_reason\":\"success\"") &&
               contains(result.output, "\"checkpoint_count\":3");
      });
  REQUIRE(idled_again.exit_code == 0);
  const std::string continued_codex_session =
      extract_json_string_field(idled_again.output, "codex_session_id");
  REQUIRE(continued_codex_session == idled_codex_session);

  const fs::path session_root = runtime_root / ".marshal_hero" / session_id;
  const fs::path codex_stdout_log =
      session_root / "logs" / "codex.session.stdout.jsonl";
  const fs::path codex_stderr_log =
      session_root / "logs" / "codex.session.stderr.jsonl";
  REQUIRE(fs::exists(codex_stdout_log));
  REQUIRE(fs::exists(codex_stderr_log));
  REQUIRE(!fs::exists(session_root / "logs" / "codex.session.log"));
  REQUIRE(contains(read_text_file(codex_stdout_log), "\"type\":\"stub_event\""));
  REQUIRE(contains(read_text_file(codex_stderr_log), "\"stream\":\"stderr\""));
  REQUIRE(contains(read_text_file(codex_stderr_log), kSmokeCodexSessionId));
  const std::string events_text =
      read_text_file(session_root / "marshal.session.events.jsonl");
  REQUIRE(contains(events_text, "\"event\":\"session_continued\""));
  REQUIRE(count_occurrences(events_text, "\"event\":\"input_checkpoint_staged\"") >=
          3);

  const std::string memory_text =
      read_text_file(session_root / "marshal.session.memory.md");
  REQUIRE(contains(memory_text, "## Checkpoint 2"));
  REQUIRE(contains(memory_text, "## Checkpoint 3"));
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
      "/cuwacunu/src/config/instructions/objectives/vicreg.solo.train/vicreg.solo.train.marshal.dsl";

  write_text_file(human_identities, "# smoke-only\n");
  const std::string codex_stub_text =
      std::string(
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
printf 'session id: %s\n' ")CODEX") +
      kSmokeCodexSessionId +
      R"CODEX(" >&2
step=0
if [[ -f "$state_file" ]]; then
  step="$(cat "$state_file")"
fi
printf '{"type":"stub_event","step":"%s","resume_mode":%s}\n' "$step" "$resume_mode"
if [[ "$resume_mode" == "0" && "$step" == "0" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"launch_campaign","reason":"launch after same-checkpoint objective edit","memory_note":"need softer augmentation before the next launch","launch":{"mode":"run_plan","reset_runtime_state":false,"requires_objective_mutation":true}}
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
{"intent":"complete","reason":"operator continuation succeeded after reviewing the failed checkpoint","memory_note":"recovered after failed checkpoint continuation"}
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
          "default_iitepi_campaign_dsl_filename=/cuwacunu/src/config/instructions/defaults/default.iitepi.campaign.dsl\n"
          "\n[BNF]\n"
          "iitepi_campaign_grammar_filename=/cuwacunu/src/config/bnf/iitepi.campaign.bnf\n"
          "marshal_objective_grammar_filename=/cuwacunu/src/config/bnf/objective.marshal.bnf\n"
          "\n[REAL_HERO]\n"
          "marshal_hero_dsl_filename=" + marshal_dsl.string() + "\n" +
          "human_hero_dsl_filename=" + human_dsl.string() + "\n");
  write_text_file(
      marshal_dsl,
      std::string("runtime_hero_binary:path = /bin/true\n") +
          "config_hero_binary:path = /bin/true\n"
          "lattice_hero_binary:path = /bin/true\n"
          "human_hero_binary:path = /cuwacunu/.build/hero/hero_human_mcp\n"
          "human_operator_identities:path = " + human_identities.string() + "\n" +
          "config_scope_root:path = /cuwacunu/src/config\n"
          "marshal_codex_binary:path = " + codex_stub.string() + "\n"
          "marshal_codex_model:str = gpt-5.3-codex-spark\n"
          "marshal_codex_reasoning_effort:str = xhigh\n"
          "tail_default_lines(1,+inf):int = 20\n"
          "poll_interval_ms(100,+inf):int = 100\n"
          "marshal_codex_timeout_sec(1,+inf):int = 1\n"
          "marshal_max_campaign_launches(1,+inf):int = 2\n");
  write_text_file(
      human_dsl,
      "marshal_hero_binary:path = /cuwacunu/.build/hero/hero_marshal_mcp\n"
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
  const command_result_t idled_failed = wait_for_command(
      get_command,
      [](const command_result_t& result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"phase\":\"idle\"") &&
               contains(result.output, "\"finish_reason\":\"failed\"") &&
               contains(result.output, "\"checkpoint_count\":1");
      });
  REQUIRE(idled_failed.exit_code == 0);
  REQUIRE(contains(idled_failed.output,
                   "\"last_intent_kind\":\"launch_campaign\""));
  const std::string failed_intent_path =
      extract_json_string_field(idled_failed.output,
                                "last_intent_checkpoint_path");
  REQUIRE(!failed_intent_path.empty());
  REQUIRE(contains(idled_failed.output,
                   "launch.requires_objective_mutation=true but no objective-local mutation artifact was recorded"));
  REQUIRE(contains(idled_failed.output,
                   "session parked idle so the operator can continue"));

  const fs::path session_root = runtime_root / ".marshal_hero" / session_id;
  const fs::path codex_stdout_log =
      session_root / "logs" / "codex.session.stdout.jsonl";
  const fs::path codex_stderr_log =
      session_root / "logs" / "codex.session.stderr.jsonl";
  REQUIRE(fs::exists(codex_stdout_log));
  REQUIRE(fs::exists(codex_stderr_log));
  REQUIRE(!fs::exists(session_root / "logs" / "codex.session.log"));
  REQUIRE(contains(read_text_file(codex_stdout_log), "\"type\":\"stub_event\""));
  REQUIRE(contains(read_text_file(codex_stderr_log), "\"stream\":\"stderr\""));
  REQUIRE(contains(read_text_file(codex_stderr_log), kSmokeCodexSessionId));
  const std::string summary_text =
      read_text_file(session_root / "human" / "summary.latest.md");
  REQUIRE(contains(summary_text, "This report is actionable."));
  REQUIRE(contains(summary_text, "launch.requires_objective_mutation=true"));
  REQUIRE(contains(summary_text, "Checkpoints: 1"));

  const std::string continue_args =
      std::string("{\"marshal_session_id\":\"") + session_id +
      "\",\"instruction\":\"Continue after reviewing the failed mutation checkpoint.\"}";
  const command_result_t continued = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.continue_session", continue_args));
  REQUIRE(continued.exit_code == 0);
  REQUIRE(contains(continued.output, "\"isError\":false"));

  const command_result_t idled_success = wait_for_command(
      get_command,
      [](const command_result_t& result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"phase\":\"idle\"") &&
               contains(result.output, "\"finish_reason\":\"success\"") &&
               contains(result.output, "\"checkpoint_count\":2");
      });
  REQUIRE(idled_success.exit_code == 0);
  REQUIRE(contains(idled_success.output,
                   "codex resume degraded: checkpoint=2 kind=resume_command_failed fallback=fresh_checkpoint"));
  REQUIRE(contains(idled_success.output, "resume_timeout=true"));

  const std::string events_text =
      read_text_file(session_root / "marshal.session.events.jsonl");
  REQUIRE(contains(events_text, "\"event\":\"checkpoint_failed_resumable\""));
  REQUIRE(contains(events_text, "\"event\":\"session_continued\""));

  const std::string memory_text =
      read_text_file(session_root / "marshal.session.memory.md");
  REQUIRE(contains(memory_text, "## Checkpoint 1"));
  REQUIRE(contains(memory_text, "## Checkpoint 2"));
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
  test_marshal_session_failed_checkpoint_idles_and_continues();
  std::cout << "[OK] hero mcp tool surface and smoke checks passed\n";
  return 0;
}
