#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <string>

#include <torch/torch.h>

#include "hero/hashimyei_hero/hashimyei_catalog.h"
#include "iitepi/contract_space_t.h"
#include "iitepi/runtime_binding/runtime_binding.h"
#include "iitepi/runtime_binding/runtime_binding_space_t.h"
#include "piaabo/dconfig.h"
#include "piaabo/torch_compat/torch_utils.h"
#include "tsiemene/tsi.wikimyei.representation.vicreg.h"

namespace {

namespace fs = std::filesystem;

[[nodiscard]] bool gpu_configured_without_cuda(
    const cuwacunu::iitepi::contract_hash_t& contract_hash) {
  std::string configured_device =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)
          ->get<std::string>("VICReg", "device");
  std::transform(
      configured_device.begin(),
      configured_device.end(),
      configured_device.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return !torch::cuda::is_available() &&
         (configured_device == "gpu" || configured_device == "cuda");
}

struct PathCleanupGuard {
  fs::path path;
  ~PathCleanupGuard() {
    if (path.empty()) return;
    std::error_code ec{};
    fs::remove_all(path, ec);
  }
};

[[nodiscard]] std::string unique_hashimyei_id() {
  static std::uint64_t counter = 0;
  const auto ticks = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  const std::uint64_t value = ((ticks + counter++) & 0xffffull) | 0x8000ull;
  return tsiemene::format_wikimyei_hex_hash(value);
}

[[nodiscard]] std::string ascii_upper(std::string value) {
  std::transform(
      value.begin(),
      value.end(),
      value.begin(),
      [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return value;
}

[[nodiscard]] std::int64_t read_checkpoint_i64(const std::string& path,
                                               const char* key) {
  torch::serialize::InputArchive ar;
  ar.load_from(path, torch::Device(torch::kCPU));
  torch::Tensor t;
  const bool ok = ar.try_read(key, t);
  assert(ok);
  t = t.to(torch::kCPU);
  assert(t.numel() >= 1);
  return t.item<std::int64_t>();
}

}  // namespace

int main() {
  const char* global_config_path = "/cuwacunu/src/config/.config";
  cuwacunu::iitepi::config_space_t::change_config_file(global_config_path);
  cuwacunu::iitepi::config_space_t::update_config();

  const auto contract_hash =
      cuwacunu::iitepi::runtime_binding_space_t::contract_hash_for_binding(
          cuwacunu::iitepi::config_space_t::locked_campaign_hash(),
          cuwacunu::iitepi::config_space_t::locked_binding_id());
  cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(contract_hash);

  if (gpu_configured_without_cuda(contract_hash)) {
    return 0;
  }

  constexpr tsiemene::TsiId kTsiId = 101;
  constexpr const char* kComponentName =
      "tsi.wikimyei.representation.vicreg";
  const auto contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
  assert(contract_snapshot);
  const int kChannels =
      contract_snapshot->get<int>("VARIABLES", "__obs_channels");
  const int kTimesteps =
      contract_snapshot->get<int>("VARIABLES", "__obs_seq_length");
  const int kFeatures =
      contract_snapshot->get<int>("VARIABLES", "__obs_feature_dim");
  const int kEncodingDims =
      contract_snapshot->get<int>("VARIABLES", "__embedding_dims");

  tsiemene::TsiWikimyeiRepresentationVicreg vicreg(
      kTsiId,
      "vicreg_facet_test",
      contract_hash,
      "0xtestfacet",
      kComponentName,
      kChannels,
      kTimesteps,
      kFeatures,
      false,
      true,
      true);

  assert(vicreg.supports_jkimyei_facet());

  tsiemene::TsiWikimyeiJkimyeiRuntimeState state{};
  std::string error;
  assert(vicreg.jkimyei_get_runtime_state(&state, &error));
  assert(error.empty());

  assert(state.runtime_component_name == kComponentName);
  assert(state.resolved_component_id == "tsi.wikimyei.representation.vicreg");
  assert(state.profile_id == "stable_pretrain");
  assert(!state.profile_row_id.empty());
  assert(state.run_id.empty());

  assert(!state.train_enabled);
  assert(state.use_swa);
  assert(state.detach_to_cpu);
  assert(!state.supports_runtime_profile_switch);
  assert(!state.has_pending_grad);
  assert(state.skip_on_nan);
  assert(state.zero_grad_set_to_none);
  assert(state.optimizer_steps == 0);
  assert(state.accumulate_steps == 1);
  assert(state.accum_counter == 0);
  assert(state.swa_start_iter == 1000);
  assert(state.trained_epochs == 0);
  assert(state.trained_samples == 0);
  assert(state.skipped_batches == 0);
  assert(state.clip_norm == 1.0);
  assert(state.clip_value == 0.0);
  assert(state.last_committed_loss_mean == 0.0);
  assert(state.last_committed_inv_mean == 0.0);
  assert(state.last_committed_var_mean == 0.0);
  assert(state.last_committed_cov_raw_mean == 0.0);
  assert(state.last_committed_cov_weighted_mean == 0.0);

  assert(vicreg.jkimyei_set_train_enabled(true, &error));
  assert(error.empty());
  assert(vicreg.jkimyei_get_runtime_state(&state, &error));
  assert(error.empty());
  assert(state.train_enabled);

  tsiemene::TsiWikimyeiJkimyeiFlushResult flush{};
  assert(vicreg.jkimyei_flush_pending_training_step(&flush, &error));
  assert(error.empty());
  assert(!flush.had_pending_grad);
  assert(!flush.optimizer_step_applied);
  assert(!flush.has_pending_grad_after);
  assert(flush.last_committed_loss_mean == 0.0);

  assert(vicreg.jkimyei_set_train_enabled(false, &error));
  assert(error.empty());
  assert(vicreg.jkimyei_get_runtime_state(&state, &error));
  assert(error.empty());
  assert(!state.train_enabled);

  const std::string expected_docking_signature =
      contract_snapshot->signature.docking_signature_sha256_hex;
  assert(!expected_docking_signature.empty());

  cuwacunu::hero::hashimyei::component_manifest_t compatible_manifest{};
  compatible_manifest.canonical_path =
      "tsi.wikimyei.representation.vicreg.0xcompat";
  compatible_manifest.family = "representation";
  compatible_manifest.docking_signature_sha256_hex = expected_docking_signature;
  compatible_manifest.contract_identity =
      cuwacunu::hashimyei::make_identity(
          cuwacunu::hashimyei::hashimyei_kind_e::CONTRACT,
          0x1234ull,
          "different_founding_contract_for_public_docking");
  assert(tsiemene::validate_runtime_component_manifest_public_docking(
      compatible_manifest, contract_hash, expected_docking_signature, &error));
  assert(error.empty());

  std::string mismatched_signature(64, 'f');
  if (mismatched_signature == expected_docking_signature) {
    mismatched_signature.assign(64, 'e');
  }
  compatible_manifest.docking_signature_sha256_hex = mismatched_signature;
  assert(!tsiemene::validate_runtime_component_manifest_public_docking(
      compatible_manifest, contract_hash, expected_docking_signature, &error));
  assert(!error.empty());

  const torch::Dtype configured_dtype =
      cuwacunu::iitepi::config_dtype(contract_hash, "VICReg");
  const torch::Device configured_device =
      cuwacunu::iitepi::config_device(contract_hash, "VICReg");
  const std::string source_hashimyei = unique_hashimyei_id();
  const std::string dest_hashimyei = unique_hashimyei_id();
  const std::string source_hashimyei_upper = ascii_upper(source_hashimyei);
  const std::string dest_hashimyei_upper = ascii_upper(dest_hashimyei);
  const fs::path report_root = tsiemene::wikimyei_representation_vicreg_store_root();
  const fs::path source_dir = report_root / source_hashimyei;
  const fs::path dest_dir = report_root / dest_hashimyei;
  std::error_code cleanup_ec{};
  fs::remove_all(source_dir, cleanup_ec);
  fs::remove_all(dest_dir, cleanup_ec);
  PathCleanupGuard source_cleanup{source_dir};
  PathCleanupGuard dest_cleanup{dest_dir};

  cuwacunu::wikimyei::vicreg_4d::VICReg_4D private_topology_model(
      contract_hash,
      kComponentName,
      kChannels,
      kTimesteps,
      kFeatures,
      /*encoding_dims=*/kEncodingDims,
      /*channel_expansion_dim=*/96,
      /*fused_feature_dim=*/48,
      /*encoder_hidden_dims=*/36,
      /*encoder_depth=*/6,
      "72-192-72",
      configured_dtype,
      configured_device);
  auto persisted = tsiemene::update_wikimyei_representation_vicreg_init(
      source_hashimyei_upper,
      &private_topology_model,
      /*enable_network_analytics_sidecar=*/false,
      /*enable_embedding_sequence_analytics_sidecar=*/false,
      contract_hash,
      {},
      {},
      {},
      false,
      0,
      nullptr,
      nullptr,
      {});
  assert(persisted.ok);
  assert(persisted.hashimyei == source_hashimyei);
  assert(persisted.report_fragment_directory == source_dir);
  assert(vicreg.runtime_load_from_hashimyei(source_hashimyei_upper, &error));
  assert(error.empty());
  assert(vicreg.runtime_save_to_hashimyei(dest_hashimyei_upper, &error));
  assert(error.empty());

  const fs::path saved_weights = dest_dir / "weights.init.pt";
  assert(fs::exists(saved_weights));
  assert(read_checkpoint_i64(saved_weights.string(), "meta/channel_expansion_dim") ==
         96);
  assert(read_checkpoint_i64(saved_weights.string(), "meta/fused_feature_dim") ==
         48);
  assert(read_checkpoint_i64(saved_weights.string(), "meta/encoder_hidden_dims") ==
         36);
  assert(read_checkpoint_i64(saved_weights.string(), "meta/encoder_depth") == 6);

  return 0;
}
