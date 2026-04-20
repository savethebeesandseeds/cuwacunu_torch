#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "camahjucunu/dsl/runtime_binding_instruction/runtime_binding_instruction.h"
#include "iitepi/contract_space_t.h"
#include "iitepi/observation_dsl_paths.h"
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

void replace_all(std::string* text, std::string_view from, std::string_view to) {
  if (!text) throw std::runtime_error("replace_all received null text");
  if (from.empty()) throw std::runtime_error("replace_all requires non-empty from");
  std::size_t pos = 0;
  while ((pos = text->find(from, pos)) != std::string::npos) {
    text->replace(pos, from.size(), to);
    pos += to.size();
  }
}

fs::path write_relative_default_contract_bundle(const fs::path& root) {
  fs::create_directories(root);
  const fs::path instructions_root = "/cuwacunu/src/config/instructions/defaults";
  const auto copy_instruction = [&](const char* filename) {
    write_text(root / filename, read_text(instructions_root / filename));
  };

  std::string contract_text =
      read_text(instructions_root / "default.iitepi.contract.dsl");
  replace_all(
      &contract_text,
      "/cuwacunu/src/config/instructions/defaults/default.iitepi.circuit.dsl",
      "default.iitepi.circuit.dsl");
  replace_all(
      &contract_text,
      "/cuwacunu/src/config/instructions/defaults/default.tsi.source.dataloader.sources.dsl",
      "default.tsi.source.dataloader.sources.dsl");
  replace_all(
      &contract_text,
      "/cuwacunu/src/config/instructions/defaults/default.tsi.source.dataloader.channels.dsl",
      "default.tsi.source.dataloader.channels.dsl");
  write_text(root / "default.iitepi.contract.dsl", contract_text);

  std::string vicreg_text = read_text(
      instructions_root / "default.tsi.wikimyei.representation.vicreg.dsl");
  replace_all(
      &vicreg_text,
      "/cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.representation.vicreg.network_design.dsl",
      "default.tsi.wikimyei.representation.vicreg.network_design.dsl");
  replace_all(
      &vicreg_text,
      "/cuwacunu/src/config/instructions/defaults/default.tsi.wikimyei.representation.vicreg.jkimyei.dsl",
      "default.tsi.wikimyei.representation.vicreg.jkimyei.dsl");
  write_text(root / "default.tsi.wikimyei.representation.vicreg.dsl",
             vicreg_text);

  for (const char* filename : {
           "default.iitepi.circuit.dsl",
           "default.tsi.wikimyei.representation.vicreg.network_design.dsl",
           "default.tsi.wikimyei.representation.vicreg.jkimyei.dsl",
           "default.tsi.wikimyei.inference.mdn.expected_value.dsl",
           "default.tsi.wikimyei.evaluation.embedding_sequence_analytics.dsl",
           "default.tsi.wikimyei.evaluation.transfer_matrix_evaluation.dsl",
           "default.tsi.source.dataloader.sources.dsl",
           "default.tsi.source.dataloader.channels.dsl",
       }) {
    copy_instruction(filename);
  }

  return root / "default.iitepi.contract.dsl";
}

fs::path write_relative_private_vicreg_override_bundle(const fs::path& root) {
  const fs::path contract_path = write_relative_default_contract_bundle(root);

  std::string vicreg_text =
      read_text(root / "default.tsi.wikimyei.representation.vicreg.dsl");
  replace_all(&vicreg_text,
              "channel_expansion_dim(1,4096):int = 64",
              "channel_expansion_dim(1,4096):int = 96");
  replace_all(&vicreg_text,
              "fused_feature_dim(1,4096):int = 32",
              "fused_feature_dim(1,4096):int = 48");
  replace_all(&vicreg_text,
              "encoder_hidden_dims(1,4096):int = 24",
              "encoder_hidden_dims(1,4096):int = 36");
  replace_all(&vicreg_text,
              "encoder_depth(1,512):int = 10",
              "encoder_depth(1,512):int = 6");
  replace_all(&vicreg_text,
              "projector_mlp_spec:str = 128-256-128",
              "projector_mlp_spec:str = 72-192-72");
  write_text(root / "default.tsi.wikimyei.representation.vicreg.dsl",
             vicreg_text);

  std::string network_design_text = read_text(
      root / "default.tsi.wikimyei.representation.vicreg.network_design.dsl");
  replace_all(&network_design_text,
              "channel_expansion_dim:int = 64;",
              "channel_expansion_dim:int = 96;");
  replace_all(&network_design_text,
              "fused_feature_dim:int = 32;",
              "fused_feature_dim:int = 48;");
  replace_all(&network_design_text,
              "hidden_dims:int = 24;",
              "hidden_dims:int = 36;");
  replace_all(&network_design_text,
              "depth:int = 10;",
              "depth:int = 6;");
  replace_all(&network_design_text,
              "dims:arr[int] = 72,128,256,128;",
              "dims:arr[int] = 72,192,72;");
  write_text(root / "default.tsi.wikimyei.representation.vicreg.network_design.dsl",
             network_design_text);

  return contract_path;
}

fs::path write_relative_sources_expanded_bundle(const fs::path& root) {
  const fs::path contract_path = write_relative_default_contract_bundle(root);
  const fs::path sources_path = root / "default.tsi.source.dataloader.sources.dsl";
  std::string sources_text = read_text(sources_path);
  const std::string needle =
      "|   UTILITIES  | triangular |     basic     |  /cuwacunu/.data/raw/UTILITIES/triangular/triangular_wave.csv |";
  const std::string extra_row =
      "|   UNUSEDALT  |    1w      |     kline     |  /cuwacunu/.data/raw/ETHUSDT/1w/ETHUSDT-1w-all-years.csv     |\n";
  const std::size_t pos = sources_text.find(needle);
  if (pos == std::string::npos) {
    throw std::runtime_error("failed to locate sources table insertion point");
  }
  sources_text.insert(pos, extra_row);
  write_text(sources_path, sources_text);
  return contract_path;
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
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.settings_optimize/iitepi.campaign.dsl";
  const fs::path alt_campaign_cfg_path =
      "/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.settings_optimize/iitepi.campaign.alt.dsl";

  cuwacunu::iitepi::config_space_t::change_config_file("/cuwacunu/src/config/.config");
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
      runtime_binding_itself_boot->dependency_manifest
          .dependency_manifest_aggregate_sha256_hex;
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
          .dependency_manifest_aggregate_sha256_hex != locked_digest) {
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
  const std::string locked_contract_hash =
      cuwacunu::iitepi::runtime_binding_space_t::contract_hash_for_binding(
          locked_runtime_binding_hash, binding_id);
  const auto wave_itself =
      cuwacunu::iitepi::wave_space_t::wave_itself(locked_wave_hash);
  const auto contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(locked_contract_hash);
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
  cuwacunu::iitepi::observation_dsl_path_resolution_t observation_paths{};
  std::string observation_error;
  if (!cuwacunu::iitepi::resolve_observation_dsl_paths(
          contract_itself, wave_itself, *selected_wave, &observation_paths,
          &observation_error)) {
    std::cerr << "[dconfig_contract_lock] cannot resolve observation DSL paths: "
              << observation_error << "\n";
    return 1;
  }
  const fs::path obs_dsl_path = observation_paths.sources_path;
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

  // Case 5: equivalent relative contract bundles under different roots share one identity.
  {
    const fs::path path_stability_root =
        fs::temp_directory_path() / "dconfig_contract_lock_path_stability";
    std::error_code cleanup_ec{};
    fs::remove_all(path_stability_root, cleanup_ec);
    const fs::path contract_a =
        write_relative_default_contract_bundle(path_stability_root / "root_a");
    const fs::path contract_b =
        write_relative_default_contract_bundle(path_stability_root / "root_b");
    if (fs::weakly_canonical(contract_a) == fs::weakly_canonical(contract_b)) {
      std::cerr << "[dconfig_contract_lock] path-stability fixture roots collided\n";
      return 1;
    }

    const auto hash_a =
        cuwacunu::iitepi::contract_space_t::register_contract_file(
            contract_a.string());
    const auto hash_b =
        cuwacunu::iitepi::contract_space_t::register_contract_file(
            contract_b.string());
    if (hash_a != hash_b) {
      std::cerr << "[dconfig_contract_lock] equivalent contract bundles hashed differently\n";
      return 1;
    }

    fs::remove_all(path_stability_root, cleanup_ec);
  }

  // Case 6: private VICReg topology changes preserve public docking compatibility.
  {
    const fs::path docking_root =
        fs::temp_directory_path() / "dconfig_contract_lock_public_docking";
    std::error_code cleanup_ec{};
    fs::remove_all(docking_root, cleanup_ec);

    const fs::path contract_public =
        write_relative_default_contract_bundle(docking_root / "public");
    const fs::path contract_private =
        write_relative_private_vicreg_override_bundle(docking_root / "private");

    const auto public_hash =
        cuwacunu::iitepi::contract_space_t::register_contract_file(
            contract_public.string());
    const auto private_hash =
        cuwacunu::iitepi::contract_space_t::register_contract_file(
            contract_private.string());
    if (public_hash == private_hash) {
      std::cerr
          << "[dconfig_contract_lock] private VICReg topology unexpectedly changed exact contract identity\n";
      return 1;
    }

    const auto public_snapshot =
        cuwacunu::iitepi::contract_space_t::contract_itself(public_hash);
    const auto private_snapshot =
        cuwacunu::iitepi::contract_space_t::contract_itself(private_hash);
    if (!public_snapshot || !private_snapshot) {
      std::cerr << "[dconfig_contract_lock] missing contract snapshot for public docking comparison\n";
      return 1;
    }
    if (public_snapshot->signature.docking_signature_sha256_hex !=
        private_snapshot->signature.docking_signature_sha256_hex) {
      std::cerr
          << "[dconfig_contract_lock] private VICReg topology changed public docking signature\n";
      return 1;
    }

    fs::remove_all(docking_root, cleanup_ec);
  }

  // Case 7: source registry expansion changes exact identity but preserves public docking.
  {
    const fs::path sources_root =
        fs::temp_directory_path() / "dconfig_contract_lock_sources_registry";
    std::error_code cleanup_ec{};
    fs::remove_all(sources_root, cleanup_ec);

    const fs::path contract_base =
        write_relative_default_contract_bundle(sources_root / "base");
    const fs::path contract_expanded =
        write_relative_sources_expanded_bundle(sources_root / "expanded");

    const auto base_hash =
        cuwacunu::iitepi::contract_space_t::register_contract_file(
            contract_base.string());
    const auto expanded_hash =
        cuwacunu::iitepi::contract_space_t::register_contract_file(
            contract_expanded.string());
    if (base_hash == expanded_hash) {
      std::cerr
          << "[dconfig_contract_lock] source registry expansion unexpectedly preserved exact contract identity\n";
      return 1;
    }

    const auto base_snapshot =
        cuwacunu::iitepi::contract_space_t::contract_itself(base_hash);
    const auto expanded_snapshot =
        cuwacunu::iitepi::contract_space_t::contract_itself(expanded_hash);
    if (!base_snapshot || !expanded_snapshot) {
      std::cerr << "[dconfig_contract_lock] missing contract snapshot for source-registry docking comparison\n";
      return 1;
    }
    if (base_snapshot->signature.docking_signature_sha256_hex !=
        expanded_snapshot->signature.docking_signature_sha256_hex) {
      std::cerr
          << "[dconfig_contract_lock] source registry expansion changed public docking signature\n";
      return 1;
    }

    fs::remove_all(sources_root, cleanup_ec);
  }

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
