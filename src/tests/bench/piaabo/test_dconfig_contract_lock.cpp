#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "camahjucunu/dsl/runtime_binding_instruction/runtime_binding_instruction.h"
#include "piaabo/dconfig.h"

namespace fs = std::filesystem;

namespace {

std::string read_text(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("cannot open file for read: " + path.string());
  std::ostringstream oss;
  oss << in.rdbuf();
  return oss.str();
}

void write_text(const fs::path& path, const std::string& content) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) throw std::runtime_error("cannot open file for write: " + path.string());
  out << content;
}

void set_ini_key_value(const fs::path& file_path,
                       const std::string& section,
                       const std::string& key,
                       const std::string& value) {
  std::vector<std::string> lines;
  {
    std::ifstream in(file_path);
    if (!in) throw std::runtime_error("cannot open ini file: " + file_path.string());
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
  }

  bool in_section = false;
  bool replaced = false;
  for (std::string& line : lines) {
    std::string t = line;
    const auto first_non = t.find_first_not_of(" \t");
    if (first_non == std::string::npos) continue;
    t = t.substr(first_non);
    if (!t.empty() && t.front() == '[' && t.back() == ']') {
      in_section = (t == ("[" + section + "]"));
      continue;
    }
    if (!in_section) continue;
    const std::size_t eq = t.find('=');
    if (eq == std::string::npos) continue;
    std::string lhs = t.substr(0, eq);
    const auto lhs_end = lhs.find_last_not_of(" \t");
    if (lhs_end == std::string::npos) continue;
    lhs = lhs.substr(0, lhs_end + 1);
    if (lhs != key) continue;
    line = "    " + key + " = " + value;
    replaced = true;
    break;
  }
  if (!replaced) {
    throw std::runtime_error("failed to set key " + key + " in section " + section);
  }

  std::ofstream out(file_path, std::ios::trunc);
  if (!out) throw std::runtime_error("cannot write ini file: " + file_path.string());
  for (std::size_t i = 0; i < lines.size(); ++i) {
    out << lines[i];
    if (i + 1 < lines.size()) out << '\n';
  }
}

bool expect_update_throws() {
  try {
    cuwacunu::iitepi::config_space_t::update_config();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

struct FileRestoreGuard {
  fs::path path;
  std::string original;
  bool restore{true};
  ~FileRestoreGuard() {
    if (!restore) return;
    try {
      write_text(path, original);
    } catch (...) {
    }
  }
};

}  // namespace

int main() try {
  const fs::path global_cfg_path = "/cuwacunu/src/config/.config";
  const fs::path campaign_cfg_path =
      "/cuwacunu/src/config/instructions/default.iitepi.campaign.dsl";
  const fs::path alt_campaign_cfg_path =
      "/cuwacunu/src/config/instructions/default.iitepi.campaign.alt.dsl";

  cuwacunu::iitepi::config_space_t::change_config_file("/cuwacunu/src/config/");
  cuwacunu::iitepi::config_space_t::update_config();
  cuwacunu::iitepi::runtime_binding_space_t::init();
  const auto locked_runtime_binding_hash =
      cuwacunu::iitepi::runtime_binding_space_t::locked_runtime_binding_hash();
  const auto runtime_binding_itself_boot =
      cuwacunu::iitepi::runtime_binding_space_t::runtime_binding_itself(
          locked_runtime_binding_hash);

  const std::string global_cfg_original = read_text(global_cfg_path);
  const std::string campaign_cfg_original = read_text(campaign_cfg_path);
  FileRestoreGuard global_restore{global_cfg_path, global_cfg_original};
  FileRestoreGuard campaign_restore{campaign_cfg_path, campaign_cfg_original};

  fs::copy_file(campaign_cfg_path, alt_campaign_cfg_path,
                fs::copy_options::overwrite_existing);
  write_text(global_cfg_path, global_cfg_original);
  write_text(campaign_cfg_path, campaign_cfg_original);

  const std::string locked_digest =
      runtime_binding_itself_boot->dependency_manifest.aggregate_sha256_hex;
  const std::string locked_path = runtime_binding_itself_boot->config_file_path;
  if (locked_digest.empty() || locked_path.empty()) {
    std::cerr << "[dconfig_contract_lock] runtime binding metadata is incomplete\n";
    return 1;
  }
  {
    const bool resolved_dsl_ok =
        !runtime_binding_itself_boot->runtime_binding.dsl.empty();
    if (!resolved_dsl_ok) {
      std::cerr
          << "[dconfig_contract_lock] missing canonical runtime-binding payload resolution\n";
      return 1;
    }
  }

  // Case 1: global reload without contract change must pass and preserve lock digest.
  cuwacunu::iitepi::config_space_t::update_config();
  const auto runtime_binding_itself_after_reload =
      cuwacunu::iitepi::runtime_binding_space_t::runtime_binding_itself(
          locked_runtime_binding_hash);
  if (runtime_binding_itself_after_reload->dependency_manifest
          .aggregate_sha256_hex != locked_digest) {
    std::cerr << "[dconfig_contract_lock] lock digest changed on global-only reload\n";
    return 1;
  }

  // Case 2: mutate configured campaign path mid-run -> fail-fast.
  set_ini_key_value(global_cfg_path, "GENERAL",
                    "default_iitepi_campaign_dsl_filename",
                    alt_campaign_cfg_path.string());
  if (!expect_update_throws()) {
    std::cerr
        << "[dconfig_contract_lock] expected fail-fast for campaign path mutation\n";
    return 1;
  }
  write_text(global_cfg_path, global_cfg_original);
  cuwacunu::iitepi::config_space_t::update_config();

  // Case 3: tamper root campaign file content mid-run -> fail-fast.
  const std::string campaign_original = read_text(campaign_cfg_path);
  write_text(campaign_cfg_path, campaign_original + "\n# tamper-root\n");
  if (!expect_update_throws()) {
    std::cerr
        << "[dconfig_contract_lock] expected fail-fast for root campaign tamper\n";
    return 1;
  }
  write_text(campaign_cfg_path, campaign_original);
  cuwacunu::iitepi::config_space_t::update_config();

  // Case 4: tamper transitive dependency (bound wave observation sources DSL) -> fail-fast.
  const std::string binding_id =
      cuwacunu::iitepi::runtime_binding_space_t::locked_binding_id();
  const std::string locked_contract_hash =
      cuwacunu::iitepi::runtime_binding_space_t::contract_hash_for_binding(
          locked_runtime_binding_hash, binding_id);
  (void)locked_contract_hash;
  const auto runtime_binding_itself_locked =
      cuwacunu::iitepi::runtime_binding_space_t::runtime_binding_itself(
          locked_runtime_binding_hash);
  const auto& runtime_binding_instruction =
      runtime_binding_itself_locked->runtime_binding.decoded();
  const cuwacunu::camahjucunu::runtime_binding_bind_decl_t* bind = nullptr;
  for (const auto& b : runtime_binding_instruction.binds) {
    if (b.id == binding_id) {
      bind = &b;
      break;
    }
  }
  if (!bind) {
    std::cerr
        << "[dconfig_contract_lock] cannot resolve active bind in runtime-binding DSL\n";
    return 1;
  }
  const std::string locked_wave_hash =
      cuwacunu::iitepi::runtime_binding_space_t::wave_hash_for_binding(
          locked_runtime_binding_hash, binding_id);
  const auto wave_itself =
      cuwacunu::iitepi::wave_space_t::wave_itself(locked_wave_hash);
  const auto& wave_set = wave_itself->wave.decoded();
  const cuwacunu::camahjucunu::iitepi_wave_t* selected_wave = nullptr;
  for (const auto& wave : wave_set.waves) {
    if (wave.name == bind->wave_ref) {
      selected_wave = &wave;
      break;
    }
  }
  if (!selected_wave || selected_wave->sources.empty()) {
    std::cerr << "[dconfig_contract_lock] cannot resolve selected SOURCE block in wave DSL\n";
    return 1;
  }
  const fs::path obs_dsl_path = fs::path(wave_itself->config_folder) /
                                selected_wave->sources.front().sources_dsl_file;
  const std::string obs_dsl_original = read_text(obs_dsl_path);
  FileRestoreGuard obs_restore{obs_dsl_path, obs_dsl_original};
  write_text(obs_dsl_path, obs_dsl_original + "\n# tamper-transitive\n");
  if (!expect_update_throws()) {
    std::cerr
        << "[dconfig_contract_lock] expected fail-fast for transitive dependency tamper\n";
    return 1;
  }
  write_text(obs_dsl_path, obs_dsl_original);
  cuwacunu::iitepi::config_space_t::update_config();

  fs::remove(alt_campaign_cfg_path);
  global_restore.restore = false;
  campaign_restore.restore = false;
  obs_restore.restore = false;
  std::cout << "[dconfig_contract_lock] pass\n";
  return 0;
} catch (const std::exception& e) {
  std::cerr << "[dconfig_contract_lock] exception: " << e.what() << "\n";
  return 1;
}
