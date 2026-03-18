#include <algorithm>
#include <cassert>
#include <cctype>
#include <string>

#include <torch/torch.h>

#include "iitepi/contract_space_t.h"
#include "iitepi/runtime_binding_space_t.h"
#include "piaabo/dconfig.h"
#include "tsiemene/tsi.wikimyei.representation.vicreg.h"

namespace {

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
  constexpr int kChannels = 2;
  constexpr int kTimesteps = 4;
  constexpr int kFeatures = 3;

  tsiemene::TsiWikimyeiRepresentationVicreg vicreg(
      kTsiId,
      "vicreg_facet_test",
      contract_hash,
      "0xtestfacet",
      "VICReg_representation",
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

  assert(state.runtime_component_name == "VICReg_representation");
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
  assert(state.clip_norm == 1.0);
  assert(state.clip_value == 0.0);
  assert(state.last_committed_loss_mean == 0.0);

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

  return 0;
}
