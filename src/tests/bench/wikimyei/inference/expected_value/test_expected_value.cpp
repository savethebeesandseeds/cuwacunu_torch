/* test_expected_value.cpp */
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
#include "camahjucunu/dsl/observation_pipeline/observation_pipeline.h"

#include "wikimyei/representation/VICReg/vicreg_4d.h"
#include "wikimyei/inference/expected_value/expected_value.h"

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
  // Instantiate VICReg_4d (from loading point)
  // -----------------------------------------------------
  TICK(load_representation_model_);
  cuwacunu::wikimyei::vicreg_4d::VICReg_4D representation_model(
    cuwacunu::piaabo::dconfig::contract_space_t::get<std::string>("VICReg", "model_path"),
    cuwacunu::piaabo::dconfig::config_device("VICReg"));
  PRINT_TOCK_ms(load_representation_model_);

  // -----------------------------------------------------
  // Create the Dataloader
  // -----------------------------------------------------
  torch::manual_seed(cuwacunu::piaabo::dconfig::config_space_t::get<int>("GENERAL", "torch_seed"));

  /* types definition */
  std::string INSTRUMENT = "BTCUSDT";                     // "UTILITIES"
  using Datatype_t = cuwacunu::camahjucunu::exchange::kline_t;    // cuwacunu::camahjucunu::exchange::basic_t;
  using Dataset_t = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>;
  using Datasample_t = cuwacunu::camahjucunu::data::observation_sample_t;
  using Sampler_t = torch::data::samplers::SequentialSampler; // using Sampler_t = torch::data::samplers::RandomSampler;

  TICK(create_dataloader_);
  auto raw_dataloader = cuwacunu::camahjucunu::data::make_obs_pipeline_mm_dataloader
    <Datatype_t, Sampler_t>(INSTRUMENT);
  PRINT_TOCK_ms(create_dataloader_);

  // -----------------------------------------------------
  // Instantiate representation Dataloader
  // -----------------------------------------------------
  TICK(extend_dataloader_with_enbedings_);
  auto representation_dataloader =
    representation_model.make_representation_dataloader<Dataset_t, Datasample_t, Datatype_t, Sampler_t>
      (raw_dataloader, /*use_swa=*/true, /* debug */ false);
  PRINT_TOCK_ms(extend_dataloader_with_enbedings_);
  
  // -----------------------------------------------------
  // Instantiate MDN (from configuration)
  // -----------------------------------------------------
  TICK(create_expected_value_model_);
  cuwacunu::wikimyei::ExpectedValue value_estimation_network("MDN_value_estimation");
  PRINT_TOCK_ms(create_expected_value_model_);

  // -----------------------------------------------------
  // Training
  // -----------------------------------------------------
  value_estimation_network.set_telemetry_every(
    /* n_epochs */  cuwacunu::piaabo::dconfig::contract_space_t::get<int>("VALUE_ESTIMATION",  "telemetry_every")
  );
  TICK(fit_value_estimation_);
  value_estimation_network.fit(representation_dataloader, 
    /* n_epochs */  cuwacunu::piaabo::dconfig::contract_space_t::get<int>("VALUE_ESTIMATION",  "n_epochs"),
    /* n_iters */   cuwacunu::piaabo::dconfig::contract_space_t::get<int>("VALUE_ESTIMATION",  "n_iters"),
    /* verbose */   cuwacunu::piaabo::dconfig::contract_space_t::get<bool>("VALUE_ESTIMATION",  "verbose_train")
  );
  PRINT_TOCK_ms(fit_value_estimation_);
  // -----------------------------------------------------
  // Save
  // -----------------------------------------------------
  TICK(save_value_estimation_network_);
  value_estimation_network.save_checkpoint(cuwacunu::piaabo::dconfig::contract_space_t::get("VALUE_ESTIMATION", "model_path"));
  PRINT_TOCK_ms(save_value_estimation_network_);

  // -----------------------------------------------------
  // Load
  // -----------------------------------------------------
  TICK(load_value_estimation_network_);
  cuwacunu::wikimyei::ExpectedValue loaded_value_estimation_network("MDN_value_estimation");
  loaded_value_estimation_network.load_checkpoint(cuwacunu::piaabo::dconfig::contract_space_t::get("VALUE_ESTIMATION", "model_path"));
  PRINT_TOCK_ms(load_value_estimation_network_);

  // -----------------------------------------------------
  // Dashboards: fetch latest vectors (CPU tensors)
  // -----------------------------------------------------
  TICK(estimation_network_dashboards_);
  auto ch = value_estimation_network.get_last_per_channel_nll();  // [C] on CPU
  auto hz = value_estimation_network.get_last_per_horizon_nll();  // [Hf] on CPU
  PRINT_TOCK_ms(estimation_network_dashboards_);

  return 0;
}
