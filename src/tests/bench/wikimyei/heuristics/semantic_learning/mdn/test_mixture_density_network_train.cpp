/* test_mixture_density_network_train.cpp */
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
  // Create the Dataloader
  // -----------------------------------------------------
  torch::manual_seed(cuwacunu::piaabo::dconfig::config_space_t::get<int>("GENERAL", "torch_seed"));

  /* types definition */
  std::string INSTRUMENT = "BTCUSDT";                     // "UTILITIES"
  using Td = cuwacunu::camahjucunu::exchange::kline_t;    // cuwacunu::camahjucunu::exchange::basic_t;
  using Q = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Td>;
  using K = cuwacunu::camahjucunu::data::observation_sample_t;
  // using RandSamper = torch::data::samplers::RandomSampler;
  using SeqSampler = torch::data::samplers::SequentialSampler;

  TICK(create_dataloader_);
  auto raw_dataloader = cuwacunu::camahjucunu::data::create_memory_mapped_dataloader<Q, K, Td, SeqSampler>(
      INSTRUMENT,                                                                                             /* instrument */
      cuwacunu::camahjucunu::BNF::observationPipeline()
          .decode(cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction()),             /* obs_inst */ 
      cuwacunu::piaabo::dconfig::config_space_t::get<bool>    ("DATA_LOADER", "dataloader_force_binarization"),    /* force_binarization */
      cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("DATA_LOADER", "dataloader_batch_size"),            /* batch_size */
      cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("DATA_LOADER", "dataloader_workers")                /* workers */
  );
  PRINT_TOCK_ms(create_dataloader_);

  // -----------------------------------------------------
  // Instantiate VICReg_4d (from loading point)
  // -----------------------------------------------------
  TICK(load_representation_model_);
  cuwacunu::wikimyei::vicreg_4d::VICReg_4D representation_model(
    cuwacunu::piaabo::dconfig::config_space_t::get<std::string>("VICReg", "model_path"),
    cuwacunu::piaabo::dconfig::config_device("VICReg"));
  PRINT_TOCK_ms(load_representation_model_);

  // -----------------------------------------------------
  // Instantiate representation Dataloader
  // -----------------------------------------------------
  TICK(extend_dataloader_with_enbedings_);
  auto representation_dataloader =
    representation_model.make_representation_dataloader<Q, K, Td, SeqSampler>
      (raw_dataloader, /*use_swa=*/true);
  PRINT_TOCK_ms(extend_dataloader_with_enbedings_);
  
  // -----------------------------------------------------
  // Instantiate MDN (from configuration)
  // -----------------------------------------------------
  TICK(create_mdn_model_);
  cuwacunu::wikimyei::mdn::MdnModel semantic_value_model(
      "VALUE_ESTIMATION",        /* target_conf  (.config) */
      "MDN_value_estimation",    /* target_setup (<training_components>.instruction) */
      representation_model.encoding_dims, /* Dx */
      1                                   /* Dy */
  );
  PRINT_TOCK_ms(create_mdn_model_);

  return 0;
}
