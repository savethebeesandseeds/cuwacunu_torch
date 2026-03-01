#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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
    cuwacunu::piaabo::dconfig::config_space_t::update_config();
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
  const fs::path contract_cfg_path =
      "/cuwacunu/src/config/default.board.contract.config";
  const fs::path alt_contract_cfg_path =
      fs::temp_directory_path() / "default.board.contract.alt.config";

  const auto locked_contract_hash =
      cuwacunu::piaabo::dconfig::contract_space_t::register_contract_file(
          contract_cfg_path.string());
  const auto& snapshot_boot =
      cuwacunu::piaabo::dconfig::contract_space_t::snapshot(locked_contract_hash);

  const std::string global_cfg_original = read_text(global_cfg_path);
  const std::string contract_cfg_original = read_text(contract_cfg_path);
  FileRestoreGuard global_restore{global_cfg_path, global_cfg_original};
  FileRestoreGuard contract_restore{contract_cfg_path, contract_cfg_original};

  fs::copy_file(contract_cfg_path, alt_contract_cfg_path,
                fs::copy_options::overwrite_existing);
  write_text(global_cfg_path, global_cfg_original);
  write_text(contract_cfg_path, contract_cfg_original);

  const std::string locked_digest = snapshot_boot.dependency_manifest.aggregate_sha256_hex;
  const std::string locked_path = snapshot_boot.config_file_path;
  if (locked_digest.empty() || locked_path.empty()) {
    std::cerr << "[dconfig_contract_lock] snapshot metadata is incomplete\n";
    return 1;
  }
  {
    const auto sections =
        cuwacunu::piaabo::dconfig::contract_space_t::contract_instruction_sections(
            locked_contract_hash);
    const bool resolved_dsl_ok =
        !sections.wave_profile_id.empty() &&
        !sections.tsiemene_circuit_dsl.empty() &&
        !sections.tsiemene_wave_dsl.empty();
    if (!resolved_dsl_ok) {
      std::cerr << "[dconfig_contract_lock] missing canonical wave/circuit payload resolution\n";
      return 1;
    }
  }

  // Case 1: global reload without contract change must pass and preserve lock digest.
  cuwacunu::piaabo::dconfig::config_space_t::update_config();
  const auto& snapshot_after_reload =
      cuwacunu::piaabo::dconfig::contract_space_t::snapshot(locked_contract_hash);
  if (snapshot_after_reload.dependency_manifest.aggregate_sha256_hex != locked_digest) {
    std::cerr << "[dconfig_contract_lock] lock digest changed on global-only reload\n";
    return 1;
  }

  // Case 2: mutate configured contract path mid-run -> fail-fast.
  set_ini_key_value(global_cfg_path, "GENERAL", "board_contract_config_filename",
                    alt_contract_cfg_path.string());
  if (!expect_update_throws()) {
    std::cerr << "[dconfig_contract_lock] expected fail-fast for contract path mutation\n";
    return 1;
  }
  write_text(global_cfg_path, global_cfg_original);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();

  // Case 3: tamper root contract file content mid-run -> fail-fast.
  const std::string contract_original = read_text(contract_cfg_path);
  write_text(contract_cfg_path, contract_original + "\n# tamper-root\n");
  if (!expect_update_throws()) {
    std::cerr << "[dconfig_contract_lock] expected fail-fast for root contract tamper\n";
    return 1;
  }
  write_text(contract_cfg_path, contract_original);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();

  // Case 4: tamper transitive dependency (observation sources DSL) -> fail-fast.
  const fs::path obs_dsl_path =
      cuwacunu::piaabo::dconfig::contract_space_t::get<std::string>(
          locked_contract_hash, "DSL", "observation_sources_dsl_filename");
  const std::string obs_dsl_original = read_text(obs_dsl_path);
  FileRestoreGuard obs_restore{obs_dsl_path, obs_dsl_original};
  write_text(obs_dsl_path, obs_dsl_original + "\n# tamper-transitive\n");
  if (!expect_update_throws()) {
    std::cerr
        << "[dconfig_contract_lock] expected fail-fast for transitive dependency tamper\n";
    return 1;
  }
  write_text(obs_dsl_path, obs_dsl_original);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();

  fs::remove(alt_contract_cfg_path);
  global_restore.restore = false;
  contract_restore.restore = false;
  obs_restore.restore = false;
  std::cout << "[dconfig_contract_lock] pass\n";
  return 0;
} catch (const std::exception& e) {
  std::cerr << "[dconfig_contract_lock] exception: " << e.what() << "\n";
  return 1;
}
