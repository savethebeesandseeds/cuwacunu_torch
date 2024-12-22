/* test_temporal_constrative_coding.cpp */
#include <torch/torch.h>
#include "piaabo/torch_compat/torch_utils.h"
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

#include "wikimyei/heuristics/ts-tcc/soft_dtw.h"
#include "wikimyei/heuristics/ts-tcc/temporal_contrastive_coding.h"

int main() {
  /* types definition */
  // using T = cuwacunu::camahjucunu::exchange::kline_t;
  using T = cuwacunu::camahjucunu::exchange::basic_t;
  using Q = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<T>;
  using K = cuwacunu::camahjucunu::data::observation_sample_t;
  using S = torch::data::samplers::SequentialSampler;

  /* set the test variables */
  const char* config_folder = "/cuwacunu/src/config/";
  std::string INSTRUMENT = "UTILITIES";
  // std::string INSTRUMENT = "BTCUSDT";
  int NUM_EPOCHS = 512;

  /* read the config */
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();

  /* read the instruction and create the observation pipeline */
  std::string instruction = cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction();
  auto obsPipe = cuwacunu::camahjucunu::BNF::observationPipeline();
  cuwacunu::camahjucunu::BNF::observation_instruction_t decoded_data = obsPipe.decode(instruction);

  /* model definition */
  auto device = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
  auto opts = cuwacunu::wikimyei::ts_tcc::TCCOptions();
  auto model = cuwacunu::wikimyei::ts_tcc::get_model(opts, device);
  cuwacunu::wikimyei::ts_tcc::TemporalContrastiveCoding tcc(model, opts);

  auto optimizer = cuwacunu::wikimyei::ts_tcc::get_optimizer(model, opts);
  auto scheduler = cuwacunu::wikimyei::ts_tcc::get_lr_scheduler(*optimizer, opts);

  /* create the dataloader */
  auto data_loader = cuwacunu::camahjucunu::data::create_memory_mapped_dataloader<Q, K, T, S>(INSTRUMENT, decoded_data, /* force_binarization */ false);

  /* train */
  for (int epoch = 0; epoch < NUM_EPOCHS; epoch++) {
    model->train();

    double total_loss = 0;

    for (auto& sample_batch : data_loader) {
      auto collacted_sample = K::collate_fn(sample_batch);
      auto sequence_a = collacted_sample.features.to(device);
      auto mask_a     = collacted_sample.mask.to(device);

      // Shuffle sequence_a along the batch dimension for sequence_b
      auto batch_size = sequence_a.size(0);
      auto shuffle_indices = torch::randperm(batch_size, device);
      auto sequence_b = sequence_a.index({shuffle_indices});
      auto mask_b     = mask_a.index({shuffle_indices});

      // Forward pass
      auto emb_a = tcc.forward(sequence_a, mask_a);
      auto emb_b = tcc.forward(sequence_b, mask_b);

      if (emb_a.size(0) != emb_b.size(0) || emb_a.size(1) != emb_b.size(1)) {
        throw std::invalid_argument("Input dimensions for alignment matrix computation do not match.");
      }

      if (torch::isnan(emb_a).any().item().to<bool>() || torch::isnan(emb_b).any().item().to<bool>()) {
        throw std::runtime_error("Embeddings contain NaN values.");
      }

      // Compute alignment and loss
      train on the sequences, come on: loss = loss_s1 + loss_s2 ...
      auto alignment_matrix = cuwacunu::wikimyei::ts_tcc::compute_alignment_matrix_softdtw(emb_a, emb_b);
      auto loss = tcc.compute_tcc_loss(emb_a, emb_b, alignment_matrix);

      total_loss += loss.item<double>();

      optimizer->zero_grad();
      loss.backward();
      optimizer->step();
    }
    
    std::cout << "total loss : [" << epoch << "] : \t" << std::fixed << std::setprecision(8) << total_loss << std::endl;

    scheduler->step();
  }

  return 0;
}

