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

#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d.h"
#include "wikimyei/heuristics/semantic_learning/expected_value/expected_value.h"

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
  // Decode observation instruction
  // -----------------------------------------------------
  TICK(decode_observation_pipeline_);
  cuwacunu::camahjucunu::observation_instruction_t obs_inst = cuwacunu::camahjucunu::BNF::observationPipeline()
        .decode(cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction());
  PRINT_TOCK_ms(decode_observation_pipeline_);

  // -----------------------------------------------------
  // Create the Dataloader
  // -----------------------------------------------------
  torch::manual_seed(cuwacunu::piaabo::dconfig::config_space_t::get<int>("GENERAL", "torch_seed"));

  /* types definition */
  std::string INSTRUMENT = "BTCUSDT";                     // "UTILITIES"
  using Td = cuwacunu::camahjucunu::exchange::kline_t;    // cuwacunu::camahjucunu::exchange::basic_t;
  using Q = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Td>;
  using KBatch = cuwacunu::camahjucunu::data::observation_sample_t;
  // using RandSamper = torch::data::samplers::RandomSampler;
  using SeqSampler = torch::data::samplers::SequentialSampler;

  TICK(create_dataloader_);
  auto raw_dataloader = cuwacunu::camahjucunu::data::create_memory_mapped_dataloader<Q, KBatch, Td, SeqSampler>(
      INSTRUMENT,                                                                                             /* instrument */
      obs_inst,             /* obs_inst */ 
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
    representation_model.make_representation_dataloader<Q, KBatch, Td, SeqSampler>
      (raw_dataloader, /*use_swa=*/true, /* debug */ true);
  PRINT_TOCK_ms(extend_dataloader_with_enbedings_);
  
  // -----------------------------------------------------
  // Decode the training instruction
  // -----------------------------------------------------
  TICK(decode_training_instruction_);
  cuwacunu::jkimyei::jk_setup_t jk_setup = cuwacunu::jkimyei::build_training_setup_component(
      cuwacunu::camahjucunu::BNF::trainingPipeline().decode(
        cuwacunu::piaabo::dconfig::config_space_t::training_components_instruction()
      ), "MDN_value_estimation"    /* target_setup (<training_components>.instruction) */);
  PRINT_TOCK_ms(decode_training_instruction_);
  
  // -----------------------------------------------------
  // Instantiate MDN (from configuration)
  // -----------------------------------------------------
  TICK(create_expected_value_model_);
  cuwacunu::wikimyei::ExpectedValue value_estimation_network(
      jk_setup,
      obs_inst
  );
  PRINT_TOCK_ms(create_expected_value_model_);

  // -----------------------------------------------------
  // Training
  // -----------------------------------------------------
  std::vector<std::pair<int, double>> log_loss = value_estimation_network.fit
    <decltype(representation_dataloader), KBatch>(
      representation_dataloader, 
      cuwacunu::piaabo::dconfig::config_space_t::get<int>  ("VALUE_ESTIMATION", "n_epochs"), 
      cuwacunu::piaabo::dconfig::config_space_t::get<int>  ("VALUE_ESTIMATION", "n_iters"), 
      cuwacunu::piaabo::dconfig::config_space_t::get<bool> ("VALUE_ESTIMATION", "verbose_train")
  );

  // -----------------------------------------------------
  // Evaluate
  // -----------------------------------------------------
  value_estimation_network.semantic_model->eval();

  // Infer model device & dtype from the first parameter
  const auto& params = value_estimation_network.semantic_model->parameters();
  TORCH_CHECK(!params.empty(), "Model has no parameters.");
  const auto model_device = params.front().device();
  const auto model_dtype  = params.front().dtype();

  auto opts = torch::TensorOptions().dtype(model_dtype).device(model_device);

  auto x = torch::randn({64, value_estimation_network.semantic_model->De}, opts);
  auto y = torch::randn({64, value_estimation_network.semantic_model->Dy}, opts);

  auto out = value_estimation_network.semantic_model->forward(x);

  TORCH_CHECK(out.log_pi.sizes() == torch::IntArrayRef({64, out.mu.size(1)}), "log_pi shape mismatch");
  TORCH_CHECK(out.mu.dim() == 3 && out.sigma.dim() == 3, "mu/sigma must be [B,K,Dy]");

  // Bring scalars to host safely (no need to .to(torch::kCPU); .item<T>() syncs)
  auto nll = cuwacunu::wikimyei::mdn::mdn_nll(out, y);
  std::cout << "NLL: " << nll.item<double>() << "\n";

  auto mean_pred = cuwacunu::wikimyei::mdn::mdn_expectation(out);
  std::cout << "E[y|x] mean(abs): " << mean_pred.abs().mean().item<double>() << "\n";

  auto y_smpl = cuwacunu::wikimyei::mdn::mdn_sample_one_step(out);
  std::cout << "Sample mean(abs): " << y_smpl.abs().mean().item<double>() << "\n";

  return 0;
}
