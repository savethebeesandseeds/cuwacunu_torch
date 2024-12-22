/* temporal_contrastive_coding.h */
#pragma once
#include <torch/torch.h>
#include <memory>
#include <limits>
#include <iostream>
#include "soft_dtw.h"

namespace cuwacunu {
namespace wikimyei {
namespace ts_tcc {

// -----------------------------------------------------------------------------
// TCC Options
// -----------------------------------------------------------------------------
struct TCCOptions {
  double temperature = 0.2;
  bool normalize_embeddings = true;
  double learning_rate = 1e-3;
  double weight_decay = 1e-4;
  int64_t hidden_dim = 256;
  int64_t embedding_dim = 32;
  int64_t num_layers = 16;
  double lr_decay = 0.995;
};

// -----------------------------------------------------------------------------
// TCC Encoder Model
// Accepts input of shape (B, C, T, D)
// Reorganizes internally to (B, T, C*D) for GRU processing
// Outputs (B, T, E) embeddings
// -----------------------------------------------------------------------------
struct TCCEncoderImpl : public torch::nn::Module {
  torch::nn::GRU gru{nullptr};
  torch::nn::Linear linear{nullptr};
  int64_t hidden_dim;
  int64_t embedding_dim;
  int64_t num_layers;

  bool initialized = false; // to handle lazy initialization if needed

  TCCEncoderImpl(int64_t hidden_dim, int64_t embedding_dim, int64_t num_layers=1)
    : hidden_dim(hidden_dim), embedding_dim(embedding_dim), num_layers(num_layers) {}

  void initialize_if_needed(int64_t input_dim, torch::Device device) {
    if (!initialized || gru->options.input_size() != input_dim) {
      gru = torch::nn::GRU(
        torch::nn::GRUOptions(input_dim, hidden_dim)  // Use dynamic input_dim
          .batch_first(true)
          .num_layers(num_layers)
      );
      linear = torch::nn::Linear(hidden_dim, embedding_dim);

      register_module("gru", gru);
      register_module("linear", linear);

      gru->to(device);
      linear->to(device);

      initialized = true;
    }
  }

  torch::Tensor forward(const torch::Tensor& features, const torch::Tensor& mask) {
    auto device = features.device();
    int64_t B = features.size(0); // Batch size
    int64_t C = features.size(1); // Channels
    int64_t T = features.size(2); // Time steps
    int64_t D = features.size(3); // Features per channel

    int64_t input_dim = C * D;
    initialize_if_needed(input_dim, device);

    auto features_float = features.to(torch::kFloat).to(device);
    auto mask_float = mask.to(torch::kFloat).to(device); // (B, C, T)

    // Add a dummy dimension for D since mask is (B, C, T)
    // and we want to sum over both C and D dimensions:
    auto mask_4d = mask_float.unsqueeze(-1); // (B, C, T, 1)

    // Sum over C (dim=1) and D (dim=3)
    auto mask_t = mask_4d.sum({1, 3}); // Result: (B, T)
    // Convert to binary mask (if needed, assuming >0 means valid):
    mask_t = (mask_t > 0).to(torch::kFloat); // (B, T)
    auto mask_t_expanded = mask_t.unsqueeze(-1); // (B, T, 1)

    // Prepare the input to the GRU
    auto x_perm = features_float.permute({0, 2, 1, 3});   // (B, T, C, D)
    auto x_reshaped = x_perm.reshape({B, T, input_dim});  // (B, T, input_dim)

    // Apply time-step mask to input
    x_reshaped = x_reshaped * mask_t_expanded.expand({B, T, input_dim});

    // Pass through GRU
    auto rnn_out = std::get<0>(gru->forward(x_reshaped));  // (B, T, hidden_dim)
    // Mask the GRU output at the time-step level
    rnn_out = rnn_out * mask_t_expanded.expand({B, T, hidden_dim});

    // Pass through Linear layer
    auto emb = linear->forward(rnn_out);  // (B, T, embedding_dim)
    // Mask the embeddings at the time-step level
    emb = emb * mask_t_expanded.expand({B, T, embedding_dim});

    return emb;
  }

};
TORCH_MODULE(TCCEncoder);

// -----------------------------------------------------------------------------
// get_model: returns an instance of TCCEncoder wrapped in a shared_ptr
// -----------------------------------------------------------------------------
inline std::shared_ptr<TCCEncoderImpl> get_model(const TCCOptions& opts, torch::Device device = torch::kCPU) {
  auto model = std::make_shared<TCCEncoderImpl>(opts.hidden_dim, opts.embedding_dim, opts.num_layers);
  model->to(device);
  return model;
}

// -----------------------------------------------------------------------------
// get_optimizer: returns an Adam optimizer
// -----------------------------------------------------------------------------
inline std::unique_ptr<torch::optim::Optimizer> get_optimizer(std::shared_ptr<torch::nn::Module> model, const TCCOptions& opts) {
  torch::optim::AdamOptions adam_opts(opts.learning_rate);
  adam_opts.weight_decay(opts.weight_decay);
  return std::make_unique<torch::optim::Adam>(model->parameters(), adam_opts);
}

// -----------------------------------------------------------------------------
// get_lr_scheduler: returns a StepLR scheduler
// -----------------------------------------------------------------------------
inline std::unique_ptr<torch::optim::LRScheduler> get_lr_scheduler(torch::optim::Optimizer& optimizer, const TCCOptions& opts) {
  
  return std::make_unique<torch::optim::StepLR>(optimizer, 1, opts.lr_decay);
}

// -----------------------------------------------------------------------------
// TemporalContrastiveCoding: Main class that uses the model and computes TCC loss
// -----------------------------------------------------------------------------
class TemporalContrastiveCoding {
public:
  TemporalContrastiveCoding(std::shared_ptr<TCCEncoderImpl> model, const TCCOptions& options = TCCOptions())
      : model_(model), opts_(options) {}

  torch::Tensor forward(const torch::Tensor& features, const torch::Tensor& mask) {
    auto h = model_->forward(features, mask); // Call forward directly on the specific model
    if (opts_.normalize_embeddings) {
      h = torch::nn::functional::normalize(h, torch::nn::functional::NormalizeFuncOptions().p(2).dim(-1));
    }
    return h;
  }

  torch::Tensor compute_tcc_loss(const torch::Tensor& embeddings_a,
                                 const torch::Tensor& embeddings_b,
                                 const torch::Tensor& alignment_matrix) {
    int64_t B = embeddings_a.size(0);
    int64_t T = embeddings_a.size(1);
    int64_t E = embeddings_a.size(2);

    if (opts_.temperature == 0) {
      throw std::invalid_argument("Temperature must be greater than zero");
    }

    auto anchor_flat = embeddings_a.reshape({B * T, E});
    auto candidates_flat = embeddings_b.reshape({B * T, E});
    auto sim_matrix = torch::mm(anchor_flat, candidates_flat.transpose(0, 1)) / opts_.temperature;

    auto loss_all = torch::zeros({B * T}, embeddings_a.options());

    for (int64_t i = 0; i < B * T; i++) {
      int64_t seq_idx = i / T;
      int64_t t_idx = i % T;

      int64_t pos_start = seq_idx * T;
      int64_t pos_end = pos_start + T;

      auto sim_i = sim_matrix.index({i});
      auto alignment_row = alignment_matrix.index({seq_idx, t_idx});

      auto pos_sims = sim_i.index({torch::indexing::Slice(pos_start, pos_end)});
      auto numerator = (pos_sims.exp() * alignment_row).sum();

      auto denominator = sim_i.exp().sum();

      loss_all[i] = -torch::log(numerator / denominator);
    }

    return loss_all.mean();
  }

private:
  std::shared_ptr<TCCEncoderImpl> model_;
  TCCOptions opts_;
};

} /* namespace ts_tcc */
} /* namespace wikimyei */
} /* namespace cuwacunu */