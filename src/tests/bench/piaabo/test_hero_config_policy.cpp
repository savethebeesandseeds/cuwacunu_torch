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

void ensure_instruction_roots(const fs::path& scope_root) {
  std::error_code ec{};
  fs::create_directories(scope_root / "instructions" / "defaults", ec);
  assert(!ec);
  fs::create_directories(scope_root / "instructions" / "objectives", ec);
  assert(!ec);
  fs::create_directories(scope_root / "instructions" / "temp", ec);
  assert(!ec);
}

[[nodiscard]] std::string bool_text(bool value) {
  return value ? "true" : "false";
}

[[nodiscard]] std::string json_quote(const std::string& in) {
  std::ostringstream out;
  out << '"';
  for (const unsigned char c : in) {
    switch (c) {
      case '"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        out << static_cast<char>(c);
        break;
    }
  }
  out << '"';
  return out.str();
}

[[nodiscard]] std::string replace_once(std::string text,
                                       std::string_view needle,
                                       std::string_view replacement) {
  const std::size_t pos = text.find(needle);
  assert(pos != std::string::npos);
  text.replace(pos, needle.size(), replacement);
  return text;
}

[[nodiscard]] std::string json_string_field(const std::string& json,
                                            std::string_view key) {
  const std::string prefix = "\"" + std::string(key) + "\":\"";
  const std::size_t begin = json.find(prefix);
  assert(begin != std::string::npos);
  const std::size_t value_begin = begin + prefix.size();
  const std::size_t value_end = json.find('"', value_begin);
  assert(value_end != std::string::npos);
  return json.substr(value_begin, value_end - value_begin);
}

[[nodiscard]] std::string build_config_text(const fs::path& scope_root,
                                            bool allow_local_write,
                                            const fs::path& write_roots,
                                            bool backup_enabled,
                                            const fs::path& backup_dir) {
  const fs::path default_roots = scope_root / "instructions" / "defaults";
  const fs::path objective_roots = scope_root / "instructions" / "objectives";
  const fs::path temp_roots = scope_root / "instructions" / "temp";
  std::ostringstream out;
  out << "protocol_layer[STDIO|HTTPS/SSE]:str = STDIO\n";
  out << "config_scope_root:str = " << scope_root.string() << "\n";
  out << "allow_local_read:bool = true\n";
  out << "allow_local_write:bool = " << bool_text(allow_local_write) << "\n";
  out << "default_roots:str = " << default_roots.string() << "\n";
  out << "objective_roots:str = " << objective_roots.string() << "\n";
  out << "temp_roots:str = " << temp_roots.string() << "\n";
  out << "allowed_extensions:str = .dsl\n";
  out << "write_roots:str = " << write_roots.string() << "\n";
  out << "backup_enabled:bool = " << bool_text(backup_enabled) << "\n";
  out << "backup_dir:str = " << backup_dir.string() << "\n";
  out << "backup_max_entries(1,+inf):int = 20\n";
  return out.str();
}

[[nodiscard]] std::string build_global_config_text(const fs::path& runtime_root) {
  std::ostringstream out;
  out << "[GENERAL]\n";
  out << "repo_root = /cuwacunu\n";
  out << "runtime_root = " << runtime_root.string() << "\n\n";
  out << "[BNF]\n";
  out << "iitepi_campaign_grammar_filename = /cuwacunu/src/config/bnf/iitepi.campaign.bnf\n";
  out << "iitepi_runtime_binding_grammar_filename = /cuwacunu/src/config/bnf/iitepi.runtime_binding.bnf\n";
  out << "iitepi_wave_grammar_filename = /cuwacunu/src/config/bnf/iitepi.wave.bnf\n";
  out << "marshal_objective_grammar_filename = /cuwacunu/src/config/bnf/objective.marshal.bnf\n";
  out << "network_design_grammar_filename = /cuwacunu/src/config/bnf/network_design.bnf\n";
  out << "vicreg_grammar_filename = /cuwacunu/src/config/bnf/latent_lineage_state.bnf\n";
  out << "expected_value_grammar_filename = /cuwacunu/src/config/bnf/latent_lineage_state.bnf\n";
  out << "wikimyei_evaluation_embedding_sequence_analytics_grammar_filename = /cuwacunu/src/config/bnf/latent_lineage_state.bnf\n";
  out << "wikimyei_evaluation_transfer_matrix_evaluation_grammar_filename = /cuwacunu/src/config/bnf/latent_lineage_state.bnf\n";
  out << "observation_sources_grammar_filename = /cuwacunu/src/config/bnf/tsi.source.dataloader.sources.bnf\n";
  out << "observation_channels_grammar_filename = /cuwacunu/src/config/bnf/tsi.source.dataloader.channels.bnf\n";
  out << "jkimyei_specs_grammar_filename = /cuwacunu/src/config/bnf/jkimyei.bnf\n";
  out << "tsiemene_circuit_grammar_filename = /cuwacunu/src/config/bnf/iitepi.circuit.bnf\n";
  out << "canonical_path_grammar_filename = /cuwacunu/src/config/bnf/canonical_path.bnf\n\n";
  out << "[REAL_HERO]\n";
  out << "hashimyei_hero_dsl_filename = instructions/defaults/default.hero.hashimyei.dsl\n";
  out << "lattice_hero_dsl_filename = instructions/defaults/default.hero.lattice.dsl\n";
  out << "runtime_hero_dsl_filename = instructions/defaults/default.hero.runtime.dsl\n";
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
        temp_root / "template_scope" / "instructions" / "defaults" /
        "default.hero.config.dsl";
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
    const fs::path cfg_path =
        scope_root / "instructions" / "defaults" / "default.hero.config.dsl";
    ensure_instruction_roots(scope_root);
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
    const std::string denied_replace_text =
        replace_once(original_text, "backup_max_entries(1,+inf):int = 20",
                     "backup_max_entries(1,+inf):int = 21");
    assert(!cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.default.replace",
        "{\"path\":" + json_quote(cfg_path.string()) + ",\"content\":" +
            json_quote(denied_replace_text) + "}",
        &store,
        &result_json, &err));
    assert(err.find("allow_local_write=false") != std::string::npos);
    assert(read_text(cfg_path) == original_text);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.default.read",
        "{\"path\":" + json_quote(cfg_path.string()) + "}", &store,
        &result_json, &err));
    assert(err.empty());
    assert(result_json.find("\"validation_family\":\"latent_lineage_state\"") !=
           std::string::npos);
    assert(result_json.find("backup_max_entries(1,+inf):int = 20") !=
           std::string::npos);

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
    const fs::path cfg_path =
        scope_root / "instructions" / "defaults" / "default.hero.config.dsl";
    ensure_instruction_roots(scope_root);
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
    const fs::path cfg_path =
        scope_root / "instructions" / "defaults" / "default.hero.config.dsl";
    const fs::path global_cfg = temp_root / "outside_global" / ".config";
    ensure_instruction_roots(scope_root);
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
    const fs::path cfg_path =
        scope_root / "instructions" / "defaults" / "default.hero.config.dsl";
    ensure_instruction_roots(scope_root);
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
    const fs::path ignored_default_file =
        scope_root / "instructions" / "defaults" / "ignored.txt";
    write_text(ignored_default_file, "ignore me\n");
    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.default.list", "{}", &store, &result_json, &err));
    assert(err.empty());
    assert(result_json.find("\"count\":") != std::string::npos);
    assert(result_json.find(json_quote(cfg_path.string())) != std::string::npos);
    assert(result_json.find(json_quote(ignored_default_file.string())) ==
           std::string::npos);
    assert(result_json.find("\"man_available\":true") != std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.default.list", "{\"include_man\":true}", &store,
        &result_json, &err));
    assert(err.empty());
    assert(result_json.find("\"man_path\":" +
                            json_quote("/cuwacunu/src/config/man/hero.config.man")) !=
           std::string::npos);
    assert(result_json.find("\"man_content\":") != std::string::npos);
    assert(result_json.find("default_roots") != std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.default.read",
        "{\"path\":" + json_quote(cfg_path.string()) + "}", &store,
        &result_json, &err));
    assert(err.empty());
    assert(result_json.find("\"man_path\":" +
                            json_quote("/cuwacunu/src/config/man/hero.config.man")) !=
           std::string::npos);
    assert(result_json.find("\"man_available\":true") != std::string::npos);
    assert(result_json.find("\"man_content\":") != std::string::npos);
    assert(result_json.find("allowed_extensions") != std::string::npos);
    const std::string expected_sha = json_string_field(result_json, "sha256");
    const std::string replace_text =
        replace_once(read_text(cfg_path), "backup_max_entries(1,+inf):int = 20",
                     "backup_max_entries(1,+inf):int = 23");

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.default.replace",
        "{\"path\":" + json_quote(cfg_path.string()) +
            ",\"expected_sha256\":" + json_quote(expected_sha) +
            ",\"content\":" + json_quote(replace_text) + "}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(read_text(cfg_path) == replace_text);
    assert(result_json.find("\"validation_family\":\"latent_lineage_state\"") !=
           std::string::npos);
    assert(result_json.find("\"warning\":") != std::string::npos);

    const fs::path created_default_dsl =
        scope_root / "instructions" / "defaults" / "generated.default.dsl";
    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.default.create",
        "{\"path\":" + json_quote(created_default_dsl.string()) +
            ",\"content\":\"alpha:uint = 5\\n\"}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(read_text(created_default_dsl) == "alpha:uint = 5\n");
    assert(result_json.find("\"warning\":") != std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.default.read",
        "{\"path\":" + json_quote(created_default_dsl.string()) + "}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(result_json.find("\"man_available\":false") != std::string::npos);
    assert(result_json.find("\"warning\":") != std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.default.delete",
        "{\"path\":" + json_quote(created_default_dsl.string()) + "}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(!fs::exists(created_default_dsl));
    assert(result_json.find("\"warning\":") != std::string::npos);

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

    const std::string invalid_config_text =
        replace_once(read_text(cfg_path), "protocol_layer[STDIO|HTTPS/SSE]:str = STDIO",
                     "protocol_layer[STDIO|HTTPS/SSE]:str = NOT_A_PROTOCOL");
    result_json.clear();
    err.clear();
    assert(!cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.default.replace",
        "{\"path\":" + json_quote(cfg_path.string()) + ",\"content\":" +
            json_quote(invalid_config_text) + "}",
        &store,
        &result_json, &err));
    assert(err.find("protocol_layer must be STDIO or HTTPS/SSE") !=
           std::string::npos);
    assert(read_text(cfg_path).find("protocol_layer[STDIO|HTTPS/SSE]:str = STDIO") !=
           std::string::npos);
  }

  {
    const fs::path scope_root = temp_root / "campaign_default_scope";
    const fs::path cfg_path =
        scope_root / "instructions" / "defaults" / "default.hero.config.dsl";
    const fs::path global_cfg = scope_root / ".config";
    const fs::path campaign_default_path =
        scope_root / "instructions" / "defaults" / "default.iitepi.campaign.dsl";
    ensure_instruction_roots(scope_root);
    write_text(cfg_path, build_config_text(scope_root,
                                           /*allow_local_write=*/true,
                                           scope_root,
                                           /*backup_enabled=*/false,
                                           scope_root / ".backups" /
                                               "hero.config"));
    write_text(global_cfg, build_global_config_text(temp_root / "campaign_default_runtime"));
    write_text(
        campaign_default_path,
        read_text("/cuwacunu/src/config/instructions/defaults/default.iitepi.campaign.dsl"));

    cuwacunu::hero::mcp::hero_config_store_t store(cfg_path.string(),
                                                   global_cfg.string());
    std::string err;
    assert(store.load(&err));
    assert(err.empty());

    const std::string campaign_default_text = read_text(campaign_default_path);
    std::string result_json;
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.default.replace",
        "{\"path\":" + json_quote(campaign_default_path.string()) +
            ",\"content\":" + json_quote(campaign_default_text) + "}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(read_text(campaign_default_path) == campaign_default_text);
    assert(result_json.find("\"validation_family\":\"iitepi_campaign\"") !=
           std::string::npos);
  }

  {
    const fs::path scope_root = temp_root / "temp_surface_scope";
    const fs::path backup_dir = scope_root / ".backups" / "hero.config";
    const fs::path cfg_path =
        scope_root / "instructions" / "defaults" / "default.hero.config.dsl";
    const fs::path temp_instruction_root =
        scope_root / "instructions" / "temp";
    const fs::path seed_temp_dsl = temp_instruction_root / "seed.dsl";
    const fs::path created_temp_dsl =
        temp_instruction_root / "generated" / "scratch.dsl";
    const fs::path ignored_temp_file = temp_instruction_root / "notes.txt";
    ensure_instruction_roots(scope_root);
    write_text(cfg_path, build_config_text(scope_root,
                                           /*allow_local_write=*/true,
                                           scope_root,
                                           /*backup_enabled=*/true,
                                           backup_dir));
    write_text(seed_temp_dsl, "alpha:uint = 1\n");
    write_text(ignored_temp_file, "ignore me\n");

    cuwacunu::hero::mcp::hero_config_store_t store(cfg_path.string());
    std::string err;
    assert(store.load(&err));
    assert(err.empty());

    std::string result_json;
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.temp.list", "{}", &store, &result_json, &err));
    assert(err.empty());
    assert(result_json.find(json_quote(seed_temp_dsl.string())) !=
           std::string::npos);
    assert(result_json.find(json_quote(ignored_temp_file.string())) ==
           std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.temp.read",
        "{\"path\":" + json_quote(seed_temp_dsl.string()) + "}", &store,
        &result_json, &err));
    assert(err.empty());
    assert(result_json.find("\"validation_family\":\"latent_lineage_state\"") !=
           std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.temp.create",
        "{\"path\":" + json_quote(created_temp_dsl.string()) +
            ",\"content\":\"alpha:uint = 9\\n\"}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(read_text(created_temp_dsl) == "alpha:uint = 9\n");
    assert(result_json.find("\"warning\":") == std::string::npos);
    assert(!has_any_regular_file(backup_dir));

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.temp.read",
        "{\"path\":" + json_quote(created_temp_dsl.string()) + "}", &store,
        &result_json, &err));
    assert(err.empty());
    const std::string temp_expected_sha = json_string_field(result_json, "sha256");

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.temp.replace",
        "{\"path\":" + json_quote(created_temp_dsl.string()) +
            ",\"expected_sha256\":" + json_quote(temp_expected_sha) +
            ",\"content\":\"alpha:uint = 10\\n\"}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(read_text(created_temp_dsl) == "alpha:uint = 10\n");
    assert(result_json.find("\"warning\":") == std::string::npos);
    assert(!has_any_regular_file(backup_dir));

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.temp.delete",
        "{\"path\":" + json_quote(created_temp_dsl.string()) + "}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(!fs::exists(created_temp_dsl));
    assert(result_json.find("\"warning\":") == std::string::npos);
    assert(!has_any_regular_file(backup_dir));
  }

  {
    const fs::path scope_root = temp_root / "marshal_objective_default_scope";
    const fs::path cfg_path =
        scope_root / "instructions" / "defaults" / "default.hero.config.dsl";
    const fs::path global_cfg = scope_root / ".config";
    const fs::path marshal_objective_default_path =
        scope_root / "instructions" / "defaults" / "default.marshal.objective.dsl";
    const fs::path marshal_objective_default_md_path =
        scope_root / "instructions" / "defaults" / "default.marshal.objective.md";
    ensure_instruction_roots(scope_root);
    write_text(cfg_path, build_config_text(scope_root,
                                           /*allow_local_write=*/true,
                                           scope_root,
                                           /*backup_enabled=*/false,
                                           scope_root / ".backups" /
                                               "hero.config"));
    write_text(global_cfg,
               build_global_config_text(temp_root / "marshal_objective_default_runtime"));
    write_text(marshal_objective_default_path,
               read_text("/cuwacunu/src/config/instructions/defaults/default.marshal.objective.dsl"));
    write_text(marshal_objective_default_md_path,
               read_text("/cuwacunu/src/config/instructions/defaults/default.marshal.objective.md"));

    cuwacunu::hero::mcp::hero_config_store_t store(cfg_path.string(),
                                                   global_cfg.string());
    std::string err;
    assert(store.load(&err));
    assert(err.empty());

    const std::string marshal_objective_default_text =
        read_text(marshal_objective_default_path);
    std::string result_json;
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.default.replace",
        "{\"path\":" + json_quote(marshal_objective_default_path.string()) +
            ",\"content\":" + json_quote(marshal_objective_default_text) + "}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(read_text(marshal_objective_default_path) ==
           marshal_objective_default_text);
    assert(result_json.find("\"validation_family\":\"marshal_objective\"") !=
           std::string::npos);
  }

  {
    const fs::path scope_root = temp_root / "objective_loop_scope";
    const fs::path runtime_root = temp_root / "objective_loop_runtime";
    const fs::path cfg_path =
        scope_root / "instructions" / "defaults" / "default.hero.config.dsl";
    const fs::path global_cfg = scope_root / ".config";
    const fs::path objective_root =
        scope_root / "instructions" / "objectives" / "vicreg.solo";
    const fs::path objective_dsl =
        objective_root / "tsi.wikimyei.representation.vicreg.dsl";
    const fs::path validated_network_dsl =
        objective_root / "tsi.wikimyei.representation.vicreg.network_design.dsl";
    const fs::path campaign_dsl = objective_root / "iitepi.campaign.dsl";
    const fs::path created_campaign_dsl =
        objective_root / "generated" / "probe.campaign.dsl";
    const fs::path marshal_objective_dsl = objective_root / "marshal.objective.dsl";
    const fs::path created_dsl = objective_root / "generated" / "probe.dsl";
    const fs::path ignored_objective_file = objective_root / "notes.txt";
    ensure_instruction_roots(scope_root);
    write_text(cfg_path, build_config_text(scope_root,
                                           /*allow_local_write=*/true,
                                           scope_root,
                                           /*backup_enabled=*/false,
                                           scope_root / ".backups" /
                                               "hero.config"));
    write_text(global_cfg, build_global_config_text(runtime_root));
    write_text(objective_dsl,
               "encoding_dims:int = 128\nencoder_depth:int = 2\n");
    write_text(validated_network_dsl,
               read_text("/cuwacunu/src/config/instructions/objectives/vicreg.solo/tsi.wikimyei.representation.vicreg.network_design.dsl"));
    write_text(
        campaign_dsl,
        read_text("/cuwacunu/src/config/instructions/defaults/default.iitepi.campaign.dsl"));
    write_text(ignored_objective_file, "not a dsl\n");
    write_text(marshal_objective_dsl,
               "campaign_dsl_path:path = iitepi.campaign.dsl\n"
               "objective_md_path:path = marshal.objective.md\n"
               "guidance_md_path:path = ../../defaults/default.marshal.guidance.md\n");

    cuwacunu::hero::mcp::hero_config_store_t store(cfg_path.string(),
                                                   global_cfg.string());
    std::string err;
    assert(store.load(&err));
    assert(err.empty());

    std::string result_json;
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.list",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"include_man\":true}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(result_json.find(json_quote(objective_dsl.string())) !=
           std::string::npos);
    assert(result_json.find(json_quote(campaign_dsl.string())) !=
           std::string::npos);
    assert(result_json.find(json_quote(ignored_objective_file.string())) ==
           std::string::npos);
    assert(result_json.find("\"man_path\":" +
                            json_quote("/cuwacunu/src/config/man/tsi.wikimyei.representation.vicreg.man")) !=
           std::string::npos);
    assert(result_json.find("\"man_content\":") != std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.read",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"tsi.wikimyei.representation.vicreg.dsl\"}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(result_json.find("\"validation_family\":\"latent_lineage_state\"") !=
           std::string::npos);
    assert(result_json.find("\"man_path\":" +
                            json_quote("/cuwacunu/src/config/man/tsi.wikimyei.representation.vicreg.man")) !=
           std::string::npos);
    assert(result_json.find("\"man_available\":true") != std::string::npos);
    assert(result_json.find("\"man_content\":") != std::string::npos);
    const std::size_t sha_label = result_json.find("\"sha256\":\"");
    assert(sha_label != std::string::npos);
    const std::size_t sha_begin = sha_label + std::string("\"sha256\":\"").size();
    const std::size_t sha_end = result_json.find('"', sha_begin);
    assert(sha_end != std::string::npos);
    const std::string expected_sha = result_json.substr(sha_begin, sha_end - sha_begin);

    const std::string replacement_text =
        "encoding_dims:int = 96\nencoder_depth:int = 3\n";
    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.replace",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"tsi.wikimyei.representation.vicreg.dsl\",\"expected_sha256\":" +
            json_quote(expected_sha) + ",\"content\":" +
            json_quote(replacement_text) + "}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(read_text(objective_dsl) == replacement_text);
    assert(result_json.find("\"validation_family\":\"latent_lineage_state\"") !=
           std::string::npos);

    result_json.clear();
    err.clear();
    const std::string original_network_text = read_text(validated_network_dsl);
    assert(!cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.replace",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"tsi.wikimyei.representation.vicreg.network_design.dsl\",\"content\":\"NETWORK broken {\\n\"}",
        &store, &result_json, &err));
    assert(err.find("validation failed") != std::string::npos);
    assert(read_text(objective_dsl) == replacement_text);
    assert(read_text(validated_network_dsl) == original_network_text);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.create",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"generated/probe.dsl\",\"content\":\"alpha:uint = 7\\n\"}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(read_text(created_dsl) == "alpha:uint = 7\n");
    assert(result_json.find("\"created\":true") != std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.list",
        "{\"objective_root\":\"" + objective_root.string() + "\"}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(result_json.find("\"relative_path\":\"generated/probe.dsl\"") !=
           std::string::npos);
    assert(result_json.find("\"warning\":") != std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.read",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"generated/probe.dsl\"}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(result_json.find("\"man_available\":false") != std::string::npos);
    assert(result_json.find("\"warning\":") != std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.read",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"iitepi.campaign.dsl\"}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(result_json.find("\"validation_family\":\"iitepi_campaign\"") !=
           std::string::npos);
    const std::string campaign_expected_sha = json_string_field(result_json, "sha256");
    const std::string campaign_text = read_text(campaign_dsl);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.create",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"generated/probe.campaign.dsl\",\"content\":" +
            json_quote(campaign_text) + "}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(read_text(created_campaign_dsl) == campaign_text);
    assert(result_json.find("\"created\":true") != std::string::npos);
    assert(result_json.find("\"validation_family\":\"iitepi_campaign\"") !=
           std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.replace",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"iitepi.campaign.dsl\",\"expected_sha256\":" +
            json_quote(campaign_expected_sha) + ",\"content\":" +
            json_quote(campaign_text) + "}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(read_text(campaign_dsl) == campaign_text);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.replace",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"marshal.objective.dsl\",\"content\":\"campaign_dsl_path:path = iitepi.campaign.dsl\\nobjective_md_path:path = marshal.objective.md\\nguidance_md_path:path = ../../defaults/default.marshal.guidance.md\\n\"}",
        &store, &result_json, &err));
    assert(err.empty());

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.delete",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"generated/probe.dsl\"}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(!fs::exists(created_dsl));

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.delete",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"generated/probe.campaign.dsl\"}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(!fs::exists(created_campaign_dsl));
  }

  {
    const fs::path scope_root = temp_root / "objective_loop_deny_scope";
    const fs::path runtime_root = temp_root / "objective_loop_deny_runtime";
    const fs::path cfg_path =
        scope_root / "instructions" / "defaults" / "default.hero.config.dsl";
    const fs::path global_cfg = scope_root / ".config";
    const fs::path objective_root =
        scope_root / "instructions" / "objectives" / "vicreg.solo";
    const fs::path objective_dsl =
        objective_root / "tsi.wikimyei.representation.vicreg.dsl";
    ensure_instruction_roots(scope_root);
    write_text(cfg_path, build_config_text(scope_root,
                                           /*allow_local_write=*/true,
                                           scope_root / "instructions" / "defaults",
                                           /*backup_enabled=*/false,
                                           scope_root / ".backups" /
                                               "hero.config"));
    write_text(global_cfg, build_global_config_text(runtime_root));
    write_text(objective_dsl, "encoding_dims:int = 64\n");

    cuwacunu::hero::mcp::hero_config_store_t store(cfg_path.string(),
                                                   global_cfg.string());
    std::string err;
    assert(store.load(&err));
    assert(err.empty());

    std::string result_json;
    assert(!cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.replace",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"tsi.wikimyei.representation.vicreg.dsl\",\"content\":\"encoding_dims:int = 96\\n\"}",
        &store, &result_json, &err));
    assert(err.find("write target escapes write_roots") != std::string::npos);
    assert(read_text(objective_dsl).find("encoding_dims:int = 64") !=
           std::string::npos);
  }

  {
    const fs::path scope_root = temp_root / "objective_contract_scope";
    const fs::path runtime_root = temp_root / "objective_contract_runtime";
    const fs::path cfg_path =
        scope_root / "instructions" / "defaults" / "default.hero.config.dsl";
    const fs::path global_cfg = scope_root / ".config";
    const fs::path objective_root =
        scope_root / "instructions" / "objectives" / "vicreg.solo";
    const fs::path defaults_root = scope_root / "instructions" / "defaults";
    const fs::path contract_dsl = objective_root / "iitepi.contract.base.dsl";
    const fs::path created_contract_dsl = objective_root / "probe.contract.dsl";
    ensure_instruction_roots(scope_root);
    write_text(cfg_path, build_config_text(scope_root,
                                           /*allow_local_write=*/true,
                                           scope_root,
                                           /*backup_enabled=*/false,
                                           scope_root / ".backups" /
                                               "hero.config"));
    write_text(global_cfg, build_global_config_text(runtime_root));
    write_text(contract_dsl,
               read_text("/cuwacunu/src/config/instructions/objectives/vicreg.solo/iitepi.contract.base.dsl"));
    write_text(objective_root / "iitepi.circuit.dsl",
               read_text("/cuwacunu/src/config/instructions/objectives/vicreg.solo/iitepi.circuit.dsl"));
    write_text(objective_root / "tsi.wikimyei.representation.vicreg.dsl",
               read_text("/cuwacunu/src/config/instructions/objectives/vicreg.solo/tsi.wikimyei.representation.vicreg.dsl"));
    write_text(
        objective_root / "tsi.wikimyei.representation.vicreg.network_design.dsl",
        read_text("/cuwacunu/src/config/instructions/objectives/vicreg.solo/tsi.wikimyei.representation.vicreg.network_design.dsl"));
    write_text(objective_root / "tsi.source.dataloader.channels.dsl",
               read_text("/cuwacunu/src/config/instructions/objectives/vicreg.solo/tsi.source.dataloader.channels.dsl"));
    write_text(defaults_root / "default.tsi.source.dataloader.sources.dsl",
               read_text("/cuwacunu/src/config/instructions/defaults/default.tsi.source.dataloader.sources.dsl"));
    write_text(defaults_root /
                   "default.tsi.wikimyei.inference.mdn.expected_value.dsl",
               read_text("/cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.inference.mdn.expected_value.dsl"));
    write_text(
        defaults_root /
            "default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl",
        read_text("/cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl"));
    write_text(
        defaults_root /
            "default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl",
        read_text("/cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl"));
    write_text(
        defaults_root / "default.tsi.wikimyei.representation.vicreg.jkimyei.dsl",
        read_text("/cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.representation.vicreg.jkimyei.dsl"));

    cuwacunu::hero::mcp::hero_config_store_t store(cfg_path.string(),
                                                   global_cfg.string());
    std::string err;
    assert(store.load(&err));
    assert(err.empty());

    std::string result_json;
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.list",
        "{\"objective_root\":\"" + objective_root.string() + "\"}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(result_json.find("\"relative_path\":\"iitepi.contract.base.dsl\"") !=
           std::string::npos);
    assert(result_json.find("\"validation_family\":\"iitepi_contract\"") !=
           std::string::npos);
    assert(result_json.find("\"replace_supported\":true") != std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.list",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"include_man\":true}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(result_json.find("\"man_path\":" +
                            json_quote("/cuwacunu/src/config/man/iitepi.contract.man")) !=
           std::string::npos);
    assert(result_json.find("\"man_content\":") != std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.read",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"iitepi.contract.base.dsl\"}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(result_json.find("\"validation_family\":\"iitepi_contract\"") !=
           std::string::npos);
    assert(result_json.find("\"man_path\":" +
                            json_quote("/cuwacunu/src/config/man/iitepi.contract.man")) !=
           std::string::npos);
    assert(result_json.find("\"man_available\":true") != std::string::npos);
    assert(result_json.find("\"man_content\":") != std::string::npos);
    const std::string contract_expected_sha =
        json_string_field(result_json, "sha256");
    const std::string contract_text = read_text(contract_dsl);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.create",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"probe.contract.dsl\",\"content\":" +
            json_quote(contract_text) + "}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(read_text(created_contract_dsl) == contract_text);
    assert(result_json.find("\"validation_family\":\"iitepi_contract\"") !=
           std::string::npos);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.replace",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"iitepi.contract.base.dsl\",\"expected_sha256\":" +
            json_quote(contract_expected_sha) + ",\"content\":" +
            json_quote(contract_text) + "}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(read_text(contract_dsl) == contract_text);
    assert(result_json.find("\"validation_family\":\"iitepi_contract\"") !=
           std::string::npos);

    result_json.clear();
    err.clear();
    assert(!cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.replace",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"iitepi.contract.base.dsl\",\"content\":\"BROKEN\\n\"}",
        &store, &result_json, &err));
    assert(err.find("iitepi_contract") != std::string::npos);
    assert(read_text(contract_dsl) == contract_text);

    result_json.clear();
    err.clear();
    assert(cuwacunu::hero::mcp::execute_tool_json(
        "hero.config.objective.delete",
        "{\"objective_root\":\"" + objective_root.string() +
            "\",\"path\":\"probe.contract.dsl\"}",
        &store, &result_json, &err));
    assert(err.empty());
    assert(!fs::exists(created_contract_dsl));
  }

  return 0;
}
