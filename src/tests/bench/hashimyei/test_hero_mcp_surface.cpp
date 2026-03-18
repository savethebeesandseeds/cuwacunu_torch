#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>

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
static constexpr const char* kGlobalConfig = "/cuwacunu/src/config/.config";

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
  REQUIRE(!contains(result.output, "\"hero.lattice.provenance\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.run_or_get\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.matrix\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.trials\""));
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

int main() {
  test_hashimyei_tools_list_surface();
  test_lattice_tools_list_surface();
  test_hashimyei_reset_catalog_smoke();
  test_lattice_reset_catalog_smoke();
  test_lattice_removed_tool_rejected();
  std::cout << "[OK] hero mcp tool surface and reset smoke checks passed\n";
  return 0;
}
