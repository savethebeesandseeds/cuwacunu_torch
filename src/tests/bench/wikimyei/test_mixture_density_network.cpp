/* test_mixture_density_network.cpp */
#include <iostream>
#include <torch/torch.h>

#include "piaabo/dconfig.h"

#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"

#include "wikimyei/inference/mdn/mixture_density_network.h"
#include "wikimyei/representation/VICReg/vicreg_4d.h"

int main() {
  // torch::autograd::AnomalyMode::set_enabled(true);
  torch::autograd::AnomalyMode::set_enabled(false);
  torch::globalContext().setBenchmarkCuDNN(true);
  torch::globalContext().setDeterministicCuDNN(false);
  WARM_UP_CUDA();

  /* set the test variables */
  const char* config_folder = "/cuwacunu/src/config/";

  /* read the config */
  TICK(read_config_);
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();
  const std::string contract_hash =
      cuwacunu::piaabo::dconfig::config_space_t::locked_contract_hash();
  PRINT_TOCK_ms(read_config_);
    
  // Reproducibility
  torch::manual_seed(48);

  // -----------------------------------------------------
  // Test the module
  // -----------------------------------------------------
  cuwacunu::wikimyei::mdn::MdnModel m(/*De=*/32, /*Dy=*/2, /*C=*/3, /*Hf=*/4, /*K=*/5, /*H=*/64, /*depth=*/2);
  auto enc = torch::randn({8, 32});
  auto out = m->forward_from_encoding(enc);
  TORCH_CHECK(out.log_pi.sizes() == torch::IntArrayRef({8,3,4,5}));
  TORCH_CHECK(out.mu.sizes()     == torch::IntArrayRef({8,3,4,5,2}));
  TORCH_CHECK(out.sigma.sizes()  == torch::IntArrayRef({8,3,4,5,2}));
  TORCH_CHECK(torch::isfinite(out.log_pi).all().item<bool>(), "[test_mdn] log_pi contains non-finite values");
  TORCH_CHECK(torch::isfinite(out.mu).all().item<bool>(), "[test_mdn] mu contains non-finite values");
  TORCH_CHECK(torch::isfinite(out.sigma).all().item<bool>(), "[test_mdn] sigma contains non-finite values");

  auto y = cuwacunu::wikimyei::mdn::mdn_expectation(out);           // [8,3,4,2]
  auto yhat = cuwacunu::wikimyei::mdn::mdn_mode(out);               // [8,3,4,2]
  auto s = cuwacunu::wikimyei::mdn::mdn_sample_one_step(out);       // [8,3,4,2]
  TORCH_CHECK(torch::isfinite(y).all().item<bool>(), "[test_mdn] expectation contains non-finite values");
  TORCH_CHECK(torch::isfinite(yhat).all().item<bool>(), "[test_mdn] mode contains non-finite values");
  TORCH_CHECK(torch::isfinite(s).all().item<bool>(), "[test_mdn] sample contains non-finite values");

  cuwacunu::wikimyei::mdn::MdnNLLLoss loss(
      cuwacunu::jkimyei::jk_setup("MDN_value_estimation", contract_hash));
  auto L = loss.compute(out, y);           // scalar
  TORCH_CHECK(torch::isfinite(L).all().item<bool>(), "[test_mdn] loss contains non-finite values");

  auto mask = torch::ones({8,3,4}, torch::kFloat32);
  auto nll_map = cuwacunu::wikimyei::mdn::mdn_nll_map(out, y, mask);
  TORCH_CHECK(nll_map.sizes() == torch::IntArrayRef({8,3,4}), "[test_mdn] nll_map shape mismatch");
  TORCH_CHECK(torch::isfinite(nll_map).all().item<bool>(), "[test_mdn] nll_map contains non-finite values");

  bool threw_bad_mask = false;
  try {
    (void)loss.compute(out, y, torch::ones({8,3,4,1}, torch::kFloat32));
  } catch (const c10::Error&) {
    threw_bad_mask = true;
  }
  TORCH_CHECK(threw_bad_mask, "[test_mdn] expected bad mask shape to throw");

  bool threw_bad_target = false;
  try {
    auto y_bad = y.select(-1, 0); // [8,3,4]
    (void)cuwacunu::wikimyei::mdn::mdn_nll_map(out, y_bad, mask);
  } catch (const c10::Error&) {
    threw_bad_target = true;
  }
  TORCH_CHECK(threw_bad_target, "[test_mdn] expected bad target shape to throw");

  return 0;
}
