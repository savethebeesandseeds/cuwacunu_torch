/* test_time_constrative_coding.cpp */
#include <torch/torch.h>
#include <fstream>
#include <iostream>
#include "piaabo/torch_compat/torch_utils.h"
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

#include "wikimyei/heuristics/ts-tcc/soft_dtw.h"
#include "wikimyei/heuristics/ts-tcc/time_contrastive_coding.h"


// A helper function to print a 2D (or 1D) tensor
void print_tensor(const torch::Tensor& mat, const std::string& name) {
  std::cout << name << ":\n" << mat << "\n";
}

// Function to save tensors to a CSV file (appends to existing data)
void save_to_csv(const torch::Tensor& data_tensor, const torch::Tensor& embedding_tensor, const std::string& filename) {
  std::ofstream file(filename, std::ios::app); // Open in append mode

  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << filename << std::endl;
    return;
  }

  // Ensure tensors are on CPU and converted to float
  auto data = data_tensor.to(torch::kCPU).contiguous().view({-1});
  auto embedding = embedding_tensor.to(torch::kCPU).contiguous().view({-1});

  // Write data tensor to file
  for (int i = 0; i < data.numel(); i++) {
    file << data[i].item<float>();
    file << ((i < data.numel() - 1) ? "," : "\n");
  }

  // Write embedding tensor to file
  for (int i = 0; i < embedding.numel(); i++) {
    file << embedding[i].item<float>();
    file << ((i < embedding.numel() - 1) ? "," : "\n");
  }

  file.close();
}

int main() {
  torch::autograd::AnomalyMode::set_enabled(true);

  /* types definition */
  // using T = cuwacunu::camahjucunu::exchange::kline_t;
  using T = cuwacunu::camahjucunu::exchange::basic_t;
  using Q = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<T>;
  using K = cuwacunu::camahjucunu::data::observation_sample_t;
  using SeqSampler = torch::data::samplers::SequentialSampler;
  using RandSamper = torch::data::samplers::RandomSampler;

  /* set the test variables */
  const char* config_folder = "/cuwacunu/src/config/";
  std::string INSTRUMENT = "UTILITIES";
  std::string output_file = "/cuwacunu/src/tests/build/test_time_contrastive_coding_output.csv";

  // std::string INSTRUMENT = "BTCUSDT";
  int NUM_EPOCHS = 20;
  std::size_t BATCH_SIZE = 12;
  std::size_t dataloader_workers = 1;
  double soft_dtw_gamma = 0.1;
  // auto device = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU; // 
  auto device = torch::kCPU;
  auto opts = cuwacunu::wikimyei::ts_tcc::TCCOptions();

  /* read the config */
  TICK(read_config_);
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();
  PRINT_TOCK_ns(read_config_);

  /* read the instruction and create the observation pipeline */
  TICK(read_instruction_);
  std::string instruction = cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction();
  auto obsPipe = cuwacunu::camahjucunu::BNF::observationPipeline();
  cuwacunu::camahjucunu::observation_instruction_t obsInst = obsPipe.decode(instruction);
  PRINT_TOCK_ns(read_instruction_);

  /* create the dataloader */
  TICK(create_dataloader_);
  auto data_loader = cuwacunu::camahjucunu::data::create_memory_mapped_dataloader<Q, K, T, RandSamper>(INSTRUMENT, obsInst, /* force_binarization */ false, /* batch_size */ BATCH_SIZE, /* workers */ dataloader_workers);
  PRINT_TOCK_ns(create_dataloader_);

  /* model definition */
  TICK(define_model_);
  int64_t input_dim = data_loader.C_ * data_loader.D_;
  cuwacunu::wikimyei::ts_tcc::TemporalContrastiveCoding tcc(opts, input_dim, device);
  PRINT_TOCK_ns(define_model_);

  /* model initialization */
  TICK(initialize_model_);
  tcc.initialize();
  PRINT_TOCK_ns(initialize_model_);

  /* create soft_dtw module */
  TICK(initialize_soft_dtw_);
  auto softdtw_criterion = cuwacunu::wikimyei::ts_tcc::SoftDTW(soft_dtw_gamma /*gamma*/, false /*normalize*/);
  PRINT_TOCK_ns(initialize_soft_dtw_);

  /* train */
  tcc.model_->train();

  TICK(all_epochs_);
  for (int epoch = 0; epoch < NUM_EPOCHS; epoch++) {

    double total_loss = 0;

    TICK(one_epoch_);
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

      emb_a.retain_grad();
      emb_b.retain_grad();

      // std::cout << "--- --- --- --- " << std::endl;
      // std::cout << "mask_a sum=" << mask_a.sum().item<double>() << std::endl;
      // std::cout << "mask_b sum=" << mask_b.sum().item<double>() << std::endl;

      // cuwacunu::piaabo::torch_compat::print_tensor_info(emb_a);

      // print_tensor(emb_a, "emb_a");
      // print_tensor(emb_b, "emb_b");
      
      if (emb_a.size(0) != emb_b.size(0) || emb_a.size(1) != emb_b.size(1)) { throw std::invalid_argument("Input dimensions for alignment matrix computation do not match."); }
      if (torch::isnan(emb_a).any().item().to<bool>() || torch::isnan(emb_b).any().item().to<bool>()) { log_fatal("Embeddings contain NaN values.\n"); }
      if (emb_a.size(0) == 0 || emb_a.size(1) == 0 || emb_a.size(2) == 0) { log_fatal("Invalid emb_a shape => zero dimension!\n"); }
      if (!emb_a.requires_grad() || !emb_b.requires_grad()) { log_err("Network forward results in tensors that does not require gradients.\n");  }

      // Compute alignment 
      auto [cost_with_grad, alignment_matrix] = softdtw_criterion->forward(emb_a, emb_b);
      alignment_matrix.retain_grad();

      if (!alignment_matrix.requires_grad()) { log_err("softdtw_alignment results in tensors that does not require gradients.\n");  }

      // Compute TCC loss
      auto loss = tcc.compute_tcc_loss(emb_a, emb_b, alignment_matrix);
      loss.retain_grad();
      if (torch::isnan(loss).any().item<bool>() || torch::isinf(loss).any().item<bool>()) { log_err("Loss contains NaN or Inf.\n"); continue; }
      if (!loss.requires_grad()) { log_err("compute_tcc_loss results in tensors that does not require gradients.\n");  }

      // emb_a.register_hook           ([](torch::Tensor grad) { std::cout << "Gradient for emb_a: " << grad << "\n"; });
      // emb_b.register_hook           ([](torch::Tensor grad) { std::cout << "Gradient for emb_b: " << grad << "\n"; });
      // alignment_matrix.register_hook([](torch::Tensor grad) { std::cout << "Gradient for alignment_matrix: " << grad << "\n"; });
      // loss.register_hook([](torch::Tensor grad) { std::cout << "Gradient for loss: " << grad << "\n"; });

      // cuwacunu::piaabo::torch_compat::inspect_network_parameters(*tcc.model_, 10);

      tcc.optimizer_->zero_grad();
      loss.backward();

      // std::cout << "--- --- ........ --- --- " << std::endl;
      // for (auto& param : tcc.model_->parameters()) {
      //     if (param.grad().defined()) {
      //         auto grad_data = param.grad().flatten();
      //         double gmean = grad_data.mean().item<double>();
      //         double gmax  = grad_data.max().item<double>();
      //         double gmin  = grad_data.min().item<double>();
      //         std::cout << "grad mean=" << gmean
      //                   << ", max=" << gmax << ", min=" << gmin << "\n";
      //     }
      // }
      // std::cout << "--- --- ........ --- --- " << std::endl;

      tcc.optimizer_->step();

      // cuwacunu::piaabo::torch_compat::inspect_network_parameters(*tcc.model_, 10);

      for (const auto& param : tcc.model_->parameters()) { if (!param.grad().defined()) { log_err("Gradient not defined for a parameter in the Network.\n"); } }
      if (!emb_a.grad().defined()) { log_err("Gradient for emb_a not defined \n"); }
      if (!emb_b.grad().defined()) { log_err("Gradient for emb_b not defined \n"); }
      if (!alignment_matrix.grad().defined()) { log_err("Gradient for alignment_matrix not defined \n");}
      if (!loss.grad().defined()) { log_err("Loss gradient is not defined.\n"); }


      total_loss += loss.item<double>();

    }
    PRINT_TOCK_ns(one_epoch_);
    
    std::cout << "total loss : [" << epoch << "] : \t" << std::fixed << std::setprecision(8) << total_loss << std::endl;

    tcc.scheduler_->step();
  }
  PRINT_TOCK_ns(all_epochs_);

  // Clear the CSV file before training starts (overwrite mode)
  std::ofstream clear_file(output_file);
  clear_file.close();

  /* now we save the values of the data */
  auto data_loader_seq = cuwacunu::camahjucunu::data::create_memory_mapped_dataloader<Q, K, T, SeqSampler>(INSTRUMENT, obsInst, /* force_binarization */ false, /* batch_size */ BATCH_SIZE, /* workers */ 1);
  for (auto& sample_batch : data_loader_seq) {
    auto collacted_sample = K::collate_fn(sample_batch);
    auto sequence_a = collacted_sample.features.to(device);
    auto mask_a     = collacted_sample.mask.to(device);

    // Forward pass
    auto emb_a = tcc.forward(sequence_a, mask_a);

    // Save to file
    save_to_csv(sequence_a, emb_a, output_file);
  }
  

  return 0;
}

