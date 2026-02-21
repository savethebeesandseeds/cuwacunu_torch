/* test_vicreg_4d_save_and_load.cpp
 *
 * Quick smoke test for VICReg_4D::save / load (exact round-trip).
 * ---------------------------------------------------------------
 */

#include <torch/torch.h>
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
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

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
make_model_from_confg(const DL& dl)
{
  using cuwacunu::wikimyei::vicreg_4d::VICReg_4D;
  // Minimal convenience ctor assumed: (C, T, D, component_name)
  return VICReg_4D(dl.C_, dl.T_, dl.D_, "VICReg_representation");
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
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(CONFIG_ROOT);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();

  // reproducibility
  torch::manual_seed(cuwacunu::piaabo::dconfig::config_space_t::get<int>("GENERAL", "torch_seed"));

  /* -------------------------------------------------- */
  /*  2) Build dataloader (as in training)              */
  /* -------------------------------------------------- */
  using Datatype_t = cuwacunu::camahjucunu::exchange::kline_t;
  using Dataset_t  = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>;
  using Datasample_t  = cuwacunu::camahjucunu::data::observation_sample_t;
  using Sampler_t = torch::data::samplers::SequentialSampler;

  std::string INSTRUMENT = "BTCUSDT";

  auto dl = cuwacunu::camahjucunu::data::make_obs_pipeline_mm_dataloader
    <Datatype_t, Sampler_t>(INSTRUMENT);

  /* -------------------------------------------------- */
  /*  3) Instantiate two identical models               */
  /* -------------------------------------------------- */
  auto model_A = make_model_from_confg(dl);
  auto model_B = make_model_from_confg(dl);  // independent instance, same arch

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
  auto out_A = model_A.encode(feats, mask, /*use_swa=*/true, /*detach_to_cpu=*/true).features;


  /* -------------------------------------------------- */
  /*  5) Save & Load                                    */
  /* -------------------------------------------------- */
  const std::string ckpt = "/tmp/vicreg_smoke.ckpt";
  try {
    std::filesystem::create_directories(std::filesystem::path(ckpt).parent_path());
  } catch (...) {}

  model_A.save(ckpt);
  model_B.load(ckpt);

  auto out_B = model_B.encode(feats, mask, /*use_swa=*/true, /*detach_to_cpu=*/true).features;

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
  cuwacunu::wikimyei::vicreg_4d::VICReg_4D model_C(ckpt, target_dev); // new strict ctor
  model_C.eval();
  auto out_C = model_C.encode(feats, mask, /*use_swa=*/true, /*detach_to_cpu=*/true).features;

  ASSERT_TRUE(out_A.sizes() == out_C.sizes(), "ctor-from-ckpt: output shape mismatch");
  const double diff_C = (out_A - out_C).abs().max().item<double>();
  std::cout << "Max |Δ| vs ctor-from-ckpt outputs: " << diff_C << '\n';
  ASSERT_TRUE(diff_C < 1e-6, "ctor-from-ckpt outputs differ!");
#endif

  std::cout << "VICReg save/load smoke test **PASSED** ✅\n";
  return 0;
}
