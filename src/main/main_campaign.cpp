#include <cctype>
#include <charconv>
#include <filesystem>
#include <exception>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <openssl/sha.h>
#include <unistd.h>

#include "hero/hashimyei_hero/hashimyei_catalog.h"
#include "hero/hashimyei_hero/hashimyei_report_fragments.h"
#include "hero/lattice_hero/source_runtime_projection_runtime.h"
#include "hero/runtime_dev_loop.h"
#include "hero/runtime_hero/runtime_job.h"
#include "iitepi/runtime_binding/runtime_binding.contract.init.h"
#include "iitepi/runtime_binding/runtime_binding.builder.h"
#include "iitepi/iitepi.h"
#include "tsiemene/tsi.type.registry.h"

namespace {

const char* value_or_empty(const std::string& value) {
  return value.empty() ? "<empty>" : value.c_str();
}

std::string trim_ascii_copy(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() &&
         std::isspace(static_cast<unsigned char>(text[begin]))) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

bool sha256_hex_bytes(std::string_view payload, std::string* out_hex) {
  if (!out_hex) return false;
  unsigned char digest[SHA256_DIGEST_LENGTH];
  SHA256_CTX ctx{};
  if (SHA256_Init(&ctx) != 1) return false;
  if (!payload.empty() &&
      SHA256_Update(&ctx, payload.data(), payload.size()) != 1) {
    return false;
  }
  if (SHA256_Final(digest, &ctx) != 1) return false;

  static constexpr char kHex[] = "0123456789abcdef";
  out_hex->clear();
  out_hex->reserve(SHA256_DIGEST_LENGTH * 2);
  for (unsigned char byte : digest) {
    out_hex->push_back(kHex[(byte >> 4) & 0x0f]);
    out_hex->push_back(kHex[byte & 0x0f]);
  }
  return true;
}

bool parse_trailing_u64_suffix(std::string_view text, std::uint64_t* out) {
  if (!out) return false;
  const std::size_t dot = text.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= text.size()) return false;
  std::uint64_t value = 0;
  const auto* begin = text.data() + dot + 1;
  const auto* end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end) return false;
  *out = value;
  return true;
}

cuwacunu::hashimyei::hashimyei_t make_hashimyei_identity(
    cuwacunu::hashimyei::hashimyei_kind_e kind,
    std::string_view seed,
    std::string hash_sha256_hex = {}) {
  return cuwacunu::hashimyei::make_identity(
      kind,
      cuwacunu::hashimyei::fnv1a64(seed),
      std::move(hash_sha256_hex));
}

bool build_wave_contract_binding_identity(
    std::string_view contract_hash,
    std::string_view wave_hash,
    std::string_view binding_alias,
    cuwacunu::hero::hashimyei::wave_contract_binding_t* out) {
  if (!out) return false;
  out->contract = make_hashimyei_identity(
      cuwacunu::hashimyei::hashimyei_kind_e::CONTRACT,
      contract_hash,
      std::string(contract_hash));
  out->wave = make_hashimyei_identity(
      cuwacunu::hashimyei::hashimyei_kind_e::WAVE,
      wave_hash,
      std::string(wave_hash));
  out->binding_alias = std::string(binding_alias);

  std::ostringstream seed;
  seed << contract_hash << "|" << wave_hash << "|" << binding_alias;
  std::string binding_hash{};
  if (!sha256_hex_bytes(seed.str(), &binding_hash) || binding_hash.empty()) {
    return false;
  }
  out->identity = make_hashimyei_identity(
      cuwacunu::hashimyei::hashimyei_kind_e::WAVE_CONTRACT_BINDING,
      binding_hash,
      binding_hash);
  return true;
}

bool persist_runtime_run_manifests(
    const std::string& campaign_hash,
    const tsiemene::runtime_binding_run_record_t& run_record,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>&
        runtime_binding_itself,
    std::string* error) {
  if (error) error->clear();
  if (!runtime_binding_itself) {
    if (error) *error = "missing runtime binding record while saving run manifests";
    return false;
  }
  if (run_record.run_ids.empty()) {
    if (error) *error = "runtime binding run produced no run_ids";
    return false;
  }

  const auto store_root = cuwacunu::hashimyei::store_root();
  const auto campaign_identity = make_hashimyei_identity(
      cuwacunu::hashimyei::hashimyei_kind_e::CAMPAIGN,
      campaign_hash);

  cuwacunu::hero::hashimyei::wave_contract_binding_t binding{};
  if (!build_wave_contract_binding_identity(
          run_record.contract_hash,
          run_record.wave_hash,
          run_record.binding_id,
          &binding)) {
    if (error) *error = "unable to derive wave_contract_binding identity";
    return false;
  }

  std::vector<cuwacunu::hero::hashimyei::dependency_file_t> dependency_files{};
  dependency_files.reserve(runtime_binding_itself->dependency_manifest.files.size());
  for (const auto& file : runtime_binding_itself->dependency_manifest.files) {
    dependency_files.push_back(
        {.canonical_path = file.canonical_path, .sha256_hex = file.sha256_hex});
  }

  for (const auto& run_id : run_record.run_ids) {
    cuwacunu::hero::hashimyei::run_manifest_t manifest{};
    manifest.run_id = run_id;
    manifest.campaign_identity = campaign_identity;
    manifest.wave_contract_binding = binding;
    manifest.sampler = trim_ascii_copy(run_record.resolved_sampler);
    manifest.record_type = trim_ascii_copy(run_record.resolved_record_type);
    manifest.dependency_files = dependency_files;
    if (!parse_trailing_u64_suffix(run_id, &manifest.started_at_ms)) {
      manifest.started_at_ms = 0;
    }
    std::string save_error;
    if (!cuwacunu::hero::hashimyei::save_run_manifest(
            store_root, manifest, nullptr, &save_error)) {
      if (error) {
        *error = "save_run_manifest failed for " + run_id + ": " + save_error;
      }
      return false;
    }
  }
  return true;
}

std::filesystem::path source_runtime_projection_report_path(
    std::string_view contract_hash, std::string_view binding_id,
    std::string_view run_id) {
  return cuwacunu::hashimyei::store_root() / "tsi.source" /
         "runtime_projection" / std::string(contract_hash) /
         std::string(binding_id) / std::string(run_id) /
         std::string(
             cuwacunu::hero::wave::kSourceRuntimeProjectionLatestReportFilename);
}

bool persist_source_runtime_projection_reports(
    const tsiemene::runtime_binding_run_record_t& run_record,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>&
        runtime_binding_itself,
    std::string* error) {
  if (error) error->clear();
  if (!runtime_binding_itself) {
    if (error) {
      *error =
          "missing runtime binding record while saving source runtime projection";
    }
    return false;
  }
  if (run_record.run_ids.size() != run_record.runtime_binding.contracts.size()) {
    if (error) {
      *error = "runtime binding run_ids/contracts size mismatch while saving "
               "source runtime projection";
    }
    return false;
  }

  const auto& instruction = runtime_binding_itself->runtime_binding.decoded();
  const auto* bind = ::tsiemene::find_bind_by_id(instruction, run_record.binding_id);
  if (!bind) {
    if (error) {
      *error = "binding not found while saving source runtime projection: " +
               run_record.binding_id;
    }
    return false;
  }

  const auto wave_hash = cuwacunu::iitepi::runtime_binding_space_t::
      wave_hash_for_binding(run_record.campaign_hash, run_record.binding_id);
  const auto wave_itself = cuwacunu::iitepi::wave_space_t::wave_itself(wave_hash);
  if (!wave_itself) {
    if (error) {
      *error = "wave record not found while saving source runtime projection: " +
               wave_hash;
    }
    return false;
  }
  const auto& wave_set = wave_itself->wave.decoded();
  const auto* wave = ::tsiemene::find_wave_by_id(wave_set, bind->wave_ref);
  if (!wave) {
    if (error) {
      *error = "binding references unknown WAVE id while saving source runtime "
               "projection: " +
               bind->wave_ref;
    }
    return false;
  }

  cuwacunu::camahjucunu::observation_spec_t observation{};
  std::string observation_error{};
  if (!::tsiemene::runtime_binding_builder::load_wave_dataloader_observation_payloads(
          wave_itself, *wave, nullptr, nullptr, &observation,
          &observation_error)) {
    if (error) {
      *error = "cannot resolve wave observation while saving source runtime "
               "projection: " +
               observation_error;
    }
    return false;
  }

  for (std::size_t i = 0; i < run_record.runtime_binding.contracts.size(); ++i) {
    const auto& contract = run_record.runtime_binding.contracts[i];
    const std::string record_type =
        contract.spec.sample_type.empty() ? run_record.resolved_record_type
                                          : contract.spec.sample_type;
    cuwacunu::hero::wave::source_runtime_projection_fragment_t fragment{};
    std::string fragment_error{};
    if (!cuwacunu::hero::wave::build_source_runtime_projection_fragment_for_wave(
            record_type, *wave, observation, &fragment, &fragment_error)) {
      if (error) {
        *error = "cannot build source runtime projection for contract[" +
                 std::to_string(i) + "]: " + fragment_error;
      }
      return false;
    }

    std::string canonical_source_type = trim_ascii_copy(contract.spec.source_type);
    if (canonical_source_type.empty()) {
      canonical_source_type = std::string(
          tsiemene::tsi_type_token(tsiemene::TsiTypeId::SourceDataloader));
    }
    std::string symbol{};
    if (!wave->sources.empty()) symbol = trim_ascii_copy(wave->sources.front().symbol);
    std::string canonical_path = canonical_source_type;
    if (!symbol.empty()) canonical_path += "." + symbol;

    cuwacunu::hero::wave::source_runtime_projection_report_identity_t identity{};
    identity.canonical_path = canonical_path;
    identity.source_label = canonical_path;
    identity.contract_hash = contract.spec.contract_hash.empty()
                                 ? run_record.contract_hash
                                 : contract.spec.contract_hash;
    identity.binding_id = run_record.binding_id;
    identity.wave_hash = run_record.wave_hash;
    identity.wave_id = bind->wave_ref;
    identity.run_id = run_record.run_ids[i];
    identity.wave_cursor_resolution = "run";

    std::string report_payload{};
    std::string report_error{};
    if (!cuwacunu::hero::wave::emit_source_runtime_projection_runtime_report(
            fragment, identity, &report_payload, &report_error)) {
      if (error) {
        *error = "cannot serialize source runtime projection for contract[" +
                 std::to_string(i) + "]: " + report_error;
      }
      return false;
    }

    const auto output_path = source_runtime_projection_report_path(
        identity.contract_hash, identity.binding_id, identity.run_id);
    if (!cuwacunu::hero::runtime::write_text_file_atomic(
            output_path, report_payload, &report_error)) {
      if (error) {
        *error = "cannot persist source runtime projection for contract[" +
                 std::to_string(i) + "] at " + output_path.string() + ": " +
                 report_error;
      }
      return false;
    }
  }
  return true;
}

void print_help(const char* argv0) {
  std::cerr << "Usage: " << argv0
            << " [--config-folder <path>] [--binding <binding_id>]"
            << " [--campaign-dsl <path>] [--reset-runtime-state]\n"
            << "Defaults:\n"
            << "  --config-folder " << DEFAULT_CONFIG_FOLDER << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  std::string config_folder = DEFAULT_CONFIG_FOLDER;
  std::string binding_override;
  std::string campaign_dsl_override;
  bool reset_runtime_state_flag = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config-folder" && i + 1 < argc) {
      config_folder = argv[++i];
      continue;
    }
    if (arg == "--binding" && i + 1 < argc) {
      binding_override = argv[++i];
      continue;
    }
    if (arg == "--campaign-dsl" && i + 1 < argc) {
      campaign_dsl_override = argv[++i];
      continue;
    }
    if (arg == "--reset-runtime-state") {
      reset_runtime_state_flag = true;
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      print_help(argv[0]);
      return 0;
    }
    std::cerr << "Unknown argument: " << arg << "\n";
    print_help(argv[0]);
    return 2;
  }

  try {
    cuwacunu::iitepi::config_space_t::change_config_file(config_folder.c_str());
    if (!campaign_dsl_override.empty()) {
      cuwacunu::iitepi::config_space_t::set_runtime_campaign_dsl_override(
          campaign_dsl_override);
    } else {
      cuwacunu::iitepi::config_space_t::clear_runtime_campaign_dsl_override();
    }
    cuwacunu::iitepi::config_space_t::update_config();

    if (reset_runtime_state_flag) {
      cuwacunu::hero::runtime_dev::runtime_reset_result_t reset{};
      std::string reset_error;
      if (!cuwacunu::hero::runtime_dev::reset_runtime_state_from_active_config(
              &reset, &reset_error)) {
        std::cerr << "[main_campaign] runtime reset failed: " << reset_error
                  << "\n";
        return 1;
      }
      log_info(
          "[main_campaign] runtime reset ok removed_store_roots=%zu removed_catalogs=%zu removed_entries=%llu\n",
          reset.removed_store_roots.size(),
          reset.removed_catalog_paths.size(),
          static_cast<unsigned long long>(reset.removed_store_entries));
      for (const auto& path : reset.removed_store_roots) {
        log_info("[main_campaign] runtime reset store_root=%s\n",
                 path.string().c_str());
      }
      for (const auto& path : reset.removed_catalog_paths) {
        log_info("[main_campaign] runtime reset catalog_path=%s\n",
                 path.string().c_str());
      }
    }

    const std::string configured_campaign_path =
        cuwacunu::iitepi::config_space_t::effective_campaign_dsl_path();
    if (configured_campaign_path.empty()) {
      std::cerr << "[main_campaign] empty configured campaign path\n";
      return 1;
    }

    if (!binding_override.empty()) {
      cuwacunu::iitepi::runtime_binding_space_t::init(configured_campaign_path,
                                            binding_override);
    } else {
      cuwacunu::iitepi::runtime_binding_space_t::init(configured_campaign_path);
    }
    cuwacunu::iitepi::runtime_binding_space_t::assert_locked_runtime_intact_or_fail_fast();

    const std::string binding_id =
        cuwacunu::iitepi::runtime_binding_space_t::locked_binding_id();

    const std::string campaign_hash =
        cuwacunu::iitepi::runtime_binding_space_t::locked_runtime_binding_hash();
    log_info("[main_campaign] campaign_hash=%s binding=%s campaign=%s\n",
             value_or_empty(campaign_hash), value_or_empty(binding_id),
             value_or_empty(configured_campaign_path));

    const auto run = cuwacunu::iitepi::run_runtime_binding(binding_id);
    if (!run.ok) {
      log_err(
          "[main_campaign] run failed campaign_hash=%s binding=%s error=%s\n",
          value_or_empty(run.campaign_hash),
          value_or_empty(run.binding_id),
          value_or_empty(run.error));
      log_err(
          "[main_campaign] contract_hash=%s wave_hash=%s record_type=%s sampler=%s total_steps=%llu\n",
          value_or_empty(run.contract_hash),
          value_or_empty(run.wave_hash),
          value_or_empty(run.resolved_record_type),
          value_or_empty(run.resolved_sampler),
          static_cast<unsigned long long>(run.total_steps));
      return 1;
    }

    const auto runtime_binding_itself =
        cuwacunu::iitepi::runtime_binding_space_t::runtime_binding_itself(
            campaign_hash);
    std::string manifest_error;
    if (!persist_runtime_run_manifests(
            campaign_hash, run, runtime_binding_itself, &manifest_error)) {
      log_err(
          "[main_campaign] run manifest persistence failed campaign_hash=%s binding=%s error=%s\n",
          value_or_empty(run.campaign_hash),
          value_or_empty(run.binding_id),
          value_or_empty(manifest_error));
      return 1;
    }
    if (!persist_source_runtime_projection_reports(
            run, runtime_binding_itself, &manifest_error)) {
      log_err(
          "[main_campaign] source runtime projection persistence failed campaign_hash=%s binding=%s error=%s\n",
          value_or_empty(run.campaign_hash),
          value_or_empty(run.binding_id),
          value_or_empty(manifest_error));
      return 1;
    }

    log_info(
        "[main_campaign] run ok campaign_hash=%s binding=%s contract_hash=%s wave_hash=%s record_type=%s sampler=%s contracts=%zu total_steps=%llu\n",
        value_or_empty(run.campaign_hash),
        value_or_empty(run.binding_id),
        value_or_empty(run.contract_hash),
        value_or_empty(run.wave_hash),
        value_or_empty(run.resolved_record_type),
        value_or_empty(run.resolved_sampler),
        run.contract_steps.size(),
        static_cast<unsigned long long>(run.total_steps));
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[main_campaign] exception: " << e.what() << "\n";
    return 1;
  }
}
