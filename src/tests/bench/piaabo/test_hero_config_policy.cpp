#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

#include "hero/config_hero/hero_config_store.h"
#include "hero/config_hero/hero_config_tools.h"

namespace {

namespace fs = std::filesystem;

struct PathCleanupGuard {
  fs::path path;
  ~PathCleanupGuard() {
    if (path.empty()) return;
    std::error_code ec{};
    fs::remove_all(path, ec);
  }
};

[[nodiscard]] std::string read_text(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  assert(in.good());
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

void write_text(const fs::path& path, const std::string& text) {
  std::error_code ec{};
  fs::create_directories(path.parent_path(), ec);
  assert(!ec);
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  assert(out.good());
  out << text;
  out.flush();
  assert(out.good());
}

[[nodiscard]] std::string bool_text(bool value) {
  return value ? "true" : "false";
}

[[nodiscard]] std::string build_config_text(const fs::path& scope_root,
                                            bool allow_local_write,
                                            const fs::path& write_roots,
                                            bool backup_enabled,
                                            const fs::path& backup_dir) {
  std::ostringstream out;
  out << "protocol_layer[STDIO|HTTPS/SSE]:str = STDIO\n";
  out << "config_scope_root:str = " << scope_root.string() << "\n";
  out << "allow_local_read:bool = true\n";
  out << "allow_local_write:bool = " << bool_text(allow_local_write) << "\n";
  out << "write_roots:str = " << write_roots.string() << "\n";
  out << "backup_enabled:bool = " << bool_text(backup_enabled) << "\n";
  out << "backup_dir:str = " << backup_dir.string() << "\n";
  out << "backup_max_entries(1,+inf):int = 20\n";
  return out.str();
}

[[nodiscard]] std::string build_global_config_text(const fs::path& runtime_root) {
  std::ostringstream out;
  out << "[GENERAL]\n";
  out << "runtime_root = " << runtime_root.string() << "\n\n";
  out << "[REAL_HERO]\n";
  out << "hashimyei_hero_dsl_filename = instructions/default.hero.hashimyei.dsl\n";
  out << "lattice_hero_dsl_filename = instructions/default.hero.lattice.dsl\n";
  out << "runtime_hero_dsl_filename = instructions/default.hero.runtime.dsl\n";
  return out.str();
}

[[nodiscard]] bool has_any_regular_file(const fs::path& root) {
  std::error_code ec{};
  if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) return false;
  for (const auto& entry : fs::directory_iterator(root, ec)) {
    if (ec) return false;
    if (entry.is_regular_file()) return true;
  }
  return false;
}

}  // namespace

int main() {
  const fs::path temp_root =
      fs::temp_directory_path() / "hero_config_policy_test";
  std::error_code cleanup_ec{};
  fs::remove_all(temp_root, cleanup_ec);
  PathCleanupGuard cleanup{temp_root};

  {
    const fs::path missing_cfg =
        temp_root / "template_scope" / "instructions" / "default.hero.config.dsl";
    cuwacunu::hero::mcp::hero_config_store_t store(missing_cfg.string());
    std::string err;
    assert(store.load(&err));
    assert(err.empty());
    assert(store.from_template());
    assert(store.get_or_default("backup_dir") ==
           "/cuwacunu/.backups/hero.config");
  }

  {
    const fs::path scope_root = temp_root / "deny_scope";
    const fs::path cfg_path = scope_root / "instructions" / "default.hero.config.dsl";
    write_text(cfg_path, build_config_text(scope_root,
                                           /*allow_local_write=*/false,
                                           scope_root,
                                           /*backup_enabled=*/false,
                                           scope_root / ".backups" /
                                               "hero.config"));
    const std::string original_text = read_text(cfg_path);
    cuwacunu::hero::mcp::hero_config_store_t store(cfg_path.string());
    std::string err;
    assert(store.load(&err));
    assert(err.empty());

    std::string result_json;
    assert(!cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.dsl.set",
        "{\"key\":\"backup_max_entries\",\"value\":\"21\"}", &store,
        &result_json, &err));
    assert(err.find("allow_local_write=false") != std::string::npos);
    assert(read_text(cfg_path) == original_text);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.dsl.get", "{\"key\":\"backup_max_entries\"}", &store,
        &result_json, &err));
    assert(err.empty());
    assert(result_json.find("\"value\":\"20\"") != std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.set",
        "{\"key\":\"backup_max_entries\",\"value\":\"21\"}", &store,
        &result_json, &err));
    assert(err.empty());

    result_json.clear();
    err.clear();
    assert(!cuwacunu::hero::mcp::execute_tool_json("hero.config.save", "{}",
                                                   &store, &result_json, &err));
    assert(err.find("allow_local_write=false") != std::string::npos);
  }

  {
    const fs::path scope_root = temp_root / "backup_escape_scope";
    const fs::path cfg_path = scope_root / "instructions" / "default.hero.config.dsl";
    write_text(cfg_path, build_config_text(scope_root,
                                           /*allow_local_write=*/true,
                                           scope_root,
                                           /*backup_enabled=*/true,
                                           temp_root / "outside_backups"));
    cuwacunu::hero::mcp::hero_config_store_t store(cfg_path.string());
    std::string err;
    assert(store.load(&err));
    assert(err.empty());

    std::string result_json;
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.set",
        "{\"key\":\"backup_max_entries\",\"value\":\"22\"}", &store,
        &result_json, &err));
    assert(err.empty());

    result_json.clear();
    err.clear();
    assert(!cuwacunu::hero::mcp::execute_tool_json("hero.config.save", "{}",
                                                   &store, &result_json, &err));
    assert(err.find("write target escapes write_roots") != std::string::npos);
  }

  {
    const fs::path scope_root = temp_root / "reset_escape_scope";
    const fs::path cfg_path = scope_root / "instructions" / "default.hero.config.dsl";
    const fs::path global_cfg = temp_root / "outside_global" / ".config";
    write_text(cfg_path, build_config_text(scope_root,
                                           /*allow_local_write=*/true,
                                           scope_root,
                                           /*backup_enabled=*/false,
                                           scope_root / ".backups" /
                                               "hero.config"));
    write_text(global_cfg,
               build_global_config_text(temp_root / "outside_runtime_root"));

    cuwacunu::hero::mcp::hero_config_store_t store(cfg_path.string(),
                                                   global_cfg.string());
    std::string err;
    assert(store.load(&err));
    assert(err.empty());

    std::string result_json;
    assert(!cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.dev_nuke_reset", "{}", &store, &result_json, &err));
    assert(err.find("escapes write_roots") != std::string::npos);
  }

  {
    const fs::path scope_root = temp_root / "allow_scope";
    const fs::path backup_dir = scope_root / ".backups" / "hero.config";
    const fs::path cfg_path = scope_root / "instructions" / "default.hero.config.dsl";
    write_text(cfg_path, build_config_text(scope_root,
                                           /*allow_local_write=*/true,
                                           scope_root,
                                           /*backup_enabled=*/true,
                                           backup_dir));
    cuwacunu::hero::mcp::hero_config_store_t store(cfg_path.string());
    std::string err;
    assert(store.load(&err));
    assert(err.empty());

    std::string result_json;
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.dsl.set",
        "{\"key\":\"backup_max_entries\",\"value\":\"23\"}", &store,
        &result_json, &err));
    assert(err.empty());
    assert(read_text(cfg_path).find("backup_max_entries(1,+inf):int = 23") !=
           std::string::npos);

    err.clear();
    assert(store.load(&err));
    assert(err.empty());

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.set",
        "{\"key\":\"backup_max_entries\",\"value\":\"24\"}", &store,
        &result_json, &err));
    assert(err.empty());

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json("hero.config.save", "{}",
                                                  &store, &result_json, &err));
    assert(err.empty());
    assert(read_text(cfg_path).find("backup_max_entries(1,+inf):int = 24") !=
           std::string::npos);
    assert(has_any_regular_file(backup_dir));
  }

  return 0;
}
