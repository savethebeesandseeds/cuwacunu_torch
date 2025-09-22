/* test_mixture_density_network.cpp */
#include <iostream>
#include <torch/torch.h>

#include "piaabo/dconfig.h"

#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network.h"
#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d.h"

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

  auto y = cuwacunu::wikimyei::mdn::mdn_expectation(out);           // [8,3,4,2]
  auto yhat = cuwacunu::wikimyei::mdn::mdn_mode(out);               // [8,3,4,2]
  auto s = cuwacunu::wikimyei::mdn::mdn_sample_one_step(out);       // [8,3,4,2]

  cuwacunu::wikimyei::mdn::MdnNLLLoss loss(cuwacunu::jkimyei::jk_setup("MDN_value_estimation"));
  auto L = loss.compute(out, y);           // scalar

  return 0;
}