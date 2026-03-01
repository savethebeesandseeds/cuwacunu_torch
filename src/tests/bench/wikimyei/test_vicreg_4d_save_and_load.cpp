/* test_vicreg_4d_save_and_load.cpp
 *
 * Quick smoke test for VICReg_4D::save / load (exact round-trip).
 * ---------------------------------------------------------------
 */

#include <torch/torch.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <filesystem>

#include "piaabo/torch_compat/torch_utils.h"
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"

#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"

#include "wikimyei/representation/VICReg/vicreg_4d.h"

/* handy macro - prints FAIL & exits if cond is false */
#define ASSERT_TRUE(cond, msg)                    \
  do {                                            \
    if (!(cond)) {                                \
      std::cerr << "ASSERT - " << msg             \
                << " (line " << __LINE__          \
                << ")\n";                         \
      return 1;                                   \
    }                                             \
  } while (0)

/* ---------- helper so we don’t repeat 20 ctor args ---------- */
template <class DL>
cuwacunu::wikimyei::vicreg_4d::VICReg_4D
make_model_from_confg(
    const DL& dl,
    const cuwacunu::iitepi::contract_hash_t& contract_hash)
{
  using cuwacunu::wikimyei::vicreg_4d::VICReg_4D;
  return VICReg_4D(
      contract_hash,
      "VICReg_representation",
      static_cast<int>(dl.C_),
      static_cast<int>(dl.T_),
      static_cast<int>(dl.D_));
}

int main()
{
  /* -------------------------------------------------- */
  /*  0) Torch & CUDA housekeeping                      */
  /* -------------------------------------------------- */
  torch::autograd::AnomalyMode::set_enabled(false);
  torch::globalContext().setBenchmarkCuDNN(true);
  torch::globalContext().setDeterministicCuDNN(false);
  WARM_UP_CUDA();

  /* -------------------------------------------------- */
  /*  1) Load config                                    */
  /* -------------------------------------------------- */
  const char* CONFIG_ROOT = "/cuwacunu/src/config/";
  cuwacunu::iitepi::config_space_t::change_config_file(CONFIG_ROOT);
  cuwacunu::iitepi::config_space_t::update_config();
  const std::string contract_hash =
      cuwacunu::iitepi::board_space_t::contract_hash_for_binding(
          cuwacunu::iitepi::config_space_t::locked_board_hash(),
          cuwacunu::iitepi::config_space_t::locked_board_binding_id());
  {
    std::string configured_device =
        cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)->get<std::string>("VICReg", "device");
    std::transform(configured_device.begin(),
                   configured_device.end(),
                   configured_device.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (!torch::cuda::is_available() &&
        (configured_device == "gpu" || configured_device == "cuda")) {
      std::cout << "[skip] VICReg device is configured as GPU but CUDA is unavailable\n";
      return 0;
    }
  }

  // reproducibility
  torch::manual_seed(cuwacunu::iitepi::config_space_t::get<int>("GENERAL", "torch_seed"));

  /* -------------------------------------------------- */
  /*  2) Build dataloader (as in training)              */
  /* -------------------------------------------------- */
  using Datatype_t = cuwacunu::camahjucunu::exchange::kline_t;
  using Datasample_t  = cuwacunu::camahjucunu::data::observation_sample_t;
  using Sampler_t = torch::data::samplers::SequentialSampler;

  std::string INSTRUMENT = "BTCUSDT";

  auto dl = cuwacunu::camahjucunu::data::make_obs_mm_dataloader
    <Datatype_t, Sampler_t>(INSTRUMENT, contract_hash);

  /* -------------------------------------------------- */
  /*  3) Instantiate two identical models               */
  /* -------------------------------------------------- */
  auto model_A = make_model_from_confg(dl, contract_hash);
  auto model_B = make_model_from_confg(dl, contract_hash);  // independent instance, same arch

  // Put models in eval for deterministic encodes
  model_A.eval();
  model_B.eval();

  /* -------------------------------------------------- */
  /*  4) Grab ONE batch & run forward                   */
  /* -------------------------------------------------- */
  auto it = dl.begin();
  ASSERT_TRUE(it != dl.end(), "empty dataloader");
  auto sample_batch = *it;
  auto sample       = Datasample_t::collate_fn(sample_batch);

  // --- determine target device/dtype from model params (safe) ---
  torch::Device target_dev = torch::kCPU;
  torch::Dtype  target_dt  = torch::kFloat32;
  {
    auto params = model_A.parameters();        // materialize; not a temporary
    if (!params.empty()) {
      auto p0 = params.front();                // copy Tensor (refcounted)
      if (p0.defined()) {
        target_dev = p0.device();
        target_dt  = p0.scalar_type();
      }
    }
  }
  auto opts = torch::TensorOptions().dtype(target_dt).device(target_dev);

  // keep mask boolean on model device
  auto feats = sample.features.detach().to(opts);
  auto mask  = sample.mask.detach()
                .to(torch::TensorOptions().dtype(torch::kBool).device(target_dev));

  // encode on CPU for comparison
  auto out_A = model_A.encode(feats, mask, /*use_swa=*/true, /*detach_to_cpu=*/true);


  /* -------------------------------------------------- */
  /*  5) Save & Load                                    */
  /* -------------------------------------------------- */
  const std::string ckpt = "/tmp/vicreg_smoke.ckpt";
  try {
    std::filesystem::create_directories(std::filesystem::path(ckpt).parent_path());
  } catch (...) {}

  model_A.save(ckpt);
  model_B.load(ckpt);

  auto out_B = model_B.encode(feats, mask, /*use_swa=*/true, /*detach_to_cpu=*/true);

  /* -------------------------------------------------- */
  /*  6) Compare (exactness)                            */
  /* -------------------------------------------------- */
  ASSERT_TRUE(out_A.sizes() == out_B.sizes(), "output shape mismatch after load");
  const double diff = (out_A - out_B).abs().reshape({-1}).max().item<double>();
  std::cout << "\nMax |Δ| between pre-save and post-load outputs: " << diff << '\n';
  ASSERT_TRUE(diff < 1e-6, "Outputs differ - save/load broke something!");

#ifdef TEST_FROM_CKPT_CTOR
  /* -------------------------------------------------- */
  /*  7) Also validate the new ctor-from-checkpoint     */
  /* -------------------------------------------------- */
  cuwacunu::wikimyei::vicreg_4d::VICReg_4D model_C(contract_hash, ckpt, target_dev); // new strict ctor
  model_C.eval();
  auto out_C = model_C.encode(feats, mask, /*use_swa=*/true, /*detach_to_cpu=*/true);

  ASSERT_TRUE(out_A.sizes() == out_C.sizes(), "ctor-from-ckpt: output shape mismatch");
  const double diff_C = (out_A - out_C).abs().max().item<double>();
  std::cout << "Max |Δ| vs ctor-from-ckpt outputs: " << diff_C << '\n';
  ASSERT_TRUE(diff_C < 1e-6, "ctor-from-ckpt outputs differ!");
#endif

  std::cout << "VICReg save/load smoke test **PASSED** ✅\n";
  return 0;
}
