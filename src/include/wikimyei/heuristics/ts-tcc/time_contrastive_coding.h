/* time_constrastive_coding.h */
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
  double learning_rate = 1e-1;
  double weight_decay = 0.05;
  int64_t hidden_dim = 10;
  int64_t embedding_dim = 1;
  int64_t num_layers = 4;
  double lr_decay = 0.995;
};

// -----------------------------------------------------------------------------
// TCCEncoder
// -----------------------------------------------------------------------------
struct TCCEncoderImpl : public torch::nn::Module {
  torch::nn::GRU gru{nullptr};
  torch::nn::Linear linear{nullptr};
  int64_t hidden_dim;
  int64_t embedding_dim;
  int64_t num_layers;
  torch::Device device_;
public:
  bool initialized = false;

  TCCEncoderImpl(int64_t hidden_dim, int64_t embedding_dim, int64_t num_layers, torch::Device device)
    : hidden_dim(hidden_dim), embedding_dim(embedding_dim), num_layers(num_layers), device_(device) {}

  void initialize_if_needed(int64_t input_dim) {
    if (!initialized) {

      gru = torch::nn::GRU(
        torch::nn::GRUOptions(input_dim, hidden_dim)
          .batch_first(true)
          .num_layers(num_layers));
      linear = torch::nn::Linear(hidden_dim, embedding_dim);
      
      gru->to(torch::kDouble);
      linear->to(torch::kDouble);

      register_module("gru", gru);
      register_module("linear", linear);

      gru->to(device_);
      linear->to(device_);

      initialized = true;
    }
  }

  // Forward: (B, C, T, D) + mask => (B, T, E)
  torch::Tensor forward(const torch::Tensor& features,
                        const torch::Tensor& mask) {
    int64_t B = features.size(0);
    int64_t C = features.size(1);
    int64_t T = features.size(2);
    int64_t D = features.size(3);

    int64_t input_dim = C * D;
    initialize_if_needed(input_dim);

    auto features_double = features.to(torch::kDouble).to(device_);
    auto mask_double     = mask.to(torch::kDouble).to(device_); // (B, C, T)

    // Sum over channels & "depth" if you want to produce a time-step mask
    auto mask_4d = mask_double.unsqueeze(-1);     // (B, C, T, 1)
    auto mask_t  = mask_4d.sum({1, 3});          // (B, T)
    mask_t       = (mask_t > 0).to(torch::kDouble); // (B, T)
    auto mask_t_expanded = mask_t.unsqueeze(-1); // (B, T, 1)

    // Reshape (B, C, T, D) -> (B, T, C*D)
    auto x_perm     = features_double.permute({0, 2, 1, 3});
    auto x_reshaped = x_perm.reshape({B, T, input_dim});

    // Apply mask
    x_reshaped = x_reshaped * mask_t_expanded.expand({B, T, input_dim});

    // GRU forward
    auto rnn_out = std::get<0>(gru->forward(x_reshaped)); // (B, T, hidden_dim)
    rnn_out = rnn_out * mask_t_expanded.expand({B, T, hidden_dim});

    // Linear projection -> embeddings
    auto emb = linear->forward(rnn_out); // (B, T, embedding_dim)
    emb = emb * mask_t_expanded.expand({B, T, embedding_dim});

    return emb;
  }
};
TORCH_MODULE(TCCEncoder);

// -----------------------------------------------------------------------------
// Factory functions
// -----------------------------------------------------------------------------
inline std::shared_ptr<TCCEncoderImpl> make_model(
    const TCCOptions& opts,
    torch::Device device = torch::kCPU)
{
  auto model = std::make_shared<TCCEncoderImpl>(
      opts.hidden_dim, opts.embedding_dim, opts.num_layers, device);
  model->to(device);
  return model;
}

inline std::unique_ptr<torch::optim::Optimizer> make_optimizer(
    std::shared_ptr<TCCEncoderImpl> model, const TCCOptions& opts)
{
  if(!model->initialized) { log_fatal("(time_contrastive_coding.h)[make_optimizer] Model needs to be initialized before calling make_optimizer.\n"); }
  torch::optim::AdamOptions adam_opts(opts.learning_rate);
  adam_opts.weight_decay(opts.weight_decay);
  return std::make_unique<torch::optim::Adam>(model->parameters(), adam_opts);
}

inline std::unique_ptr<torch::optim::LRScheduler> make_lr_scheduler(
    torch::optim::Optimizer& optimizer, const TCCOptions& opts)
{
  return std::make_unique<torch::optim::StepLR>(optimizer, 1, opts.lr_decay);
}

// -----------------------------------------------------------------------------
// TemporalContrastiveCoding
// -----------------------------------------------------------------------------
class TemporalContrastiveCoding {
public:
  TemporalContrastiveCoding(
      const TCCOptions& options, 
      int64_t input_dim, 
      torch::Device device)
        : model_(make_model(options, device)), opts_(options), input_dim_(input_dim) {}
  
  // Initialization method
  void initialize() {
    
    /* initialize network */
    model_->initialize_if_needed(input_dim_);

    /* initialize optimizer and lr scheduler */
    optimizer_ = cuwacunu::wikimyei::ts_tcc::make_optimizer(model_, opts_);
    scheduler_ = cuwacunu::wikimyei::ts_tcc::make_lr_scheduler(*optimizer_, opts_);
  }

  // Forward pass through the TCC encoder
  torch::Tensor forward(const torch::Tensor& features,
                        const torch::Tensor& mask) {
    auto emb = model_->forward(features, mask);
    if (opts_.normalize_embeddings) {
      emb = torch::nn::functional::normalize(
              emb, torch::nn::functional::NormalizeFuncOptions()
                         .p(2).dim(-1));
    }
    return emb;
  }

  // Contrastive loss with a per-timestep alignment distribution
  // alignment_matrix: (B, T, T), where alignment_matrix[b, t] is a distribution
  //                   over t' in [0..T-1] that sums to 1 (or nearly so).
  torch::Tensor compute_tcc_loss(const torch::Tensor& emb_a,
    const torch::Tensor& emb_b,
    const torch::Tensor& alignment_matrix)
  {
    // Example: let opts_ be in scope, ensuring opts_.temperature > 0
    // Make sure alignment_matrix is the same device/dtype as emb_a/emb_b 
    // or you may need .to(...).

    int64_t B = emb_a.size(0);
    int64_t T = emb_a.size(1);
    int64_t E = emb_a.size(2);

    if (opts_.temperature <= 0) {
      throw std::invalid_argument("Temperature must be > 0");
    }

    // Flatten from (B, T, E) to (B*T, E)
    auto anchor_flat = emb_a.reshape({B * T, E}); 
    auto cands_flat  = emb_b.reshape({B * T, E});

    // Similarity = dot(anchor, candidate) / temperature
    // shape: (B*T, B*T)
    auto sim_matrix = torch::mm(anchor_flat, cands_flat.transpose(0,1)) / opts_.temperature;

    auto loss_all = torch::zeros({B * T}, emb_a.options());

    // For each "anchor" row i in sim_matrix:
    for (int64_t i = 0; i < B * T; i++) {
      int64_t seq_idx = i / T;   // which sequence in the batch
      int64_t t_idx   = i % T;   // which timestep within that sequence

      // "Positive" row slice for the same sequence: [seq_idx*T .. seq_idx*T+T)
      int64_t pos_start = seq_idx * T;
      int64_t pos_end   = pos_start + T;

      // Get the row of similarities for anchor i. Shape: (B*T)
      auto sim_i = sim_matrix.index({i});

      // The T "candidate" scores that correspond to the same sequence
      // (i.e. shape: (T))
      auto pos_sims = sim_i.index({torch::indexing::Slice(pos_start, pos_end)});

      // The alignment distribution for (seq_idx, t_idx). Shape: (T).
      // alignment_row[k] ~ p_k for k in [0..T-1].
      auto alignment_row = alignment_matrix.index({seq_idx, t_idx})
                  .to(pos_sims.dtype());

      // ---- CORRECTED CROSS‐ENTROPY FORMULATION ----
      //
      // If we define logits z[k] = pos_sims[k],
      // then softmax(z)[k] = exp(z[k]) / sum_j exp(z[j]).
      // The cross‐entropy w.r.t. p_k (the alignment weights) is:
      //
      //   CE(i) = -∑_k p_k * log( softmax(z)[k] )
      //          =  log(∑_j e^(z[j])) - ∑_k p_k * z[k].
      //
      // So let's compute that directly:

      // The log of the partition sum, logSumExp
      auto log_sum_exp = pos_sims.logsumexp(/*dim=*/0);

      // Weighted sum of raw logits, ∑_k p_k * z[k]
      auto weighted_sum = (pos_sims * alignment_row).sum();

      // That yields the cross-entropy for this anchor
      auto ce_i = log_sum_exp - weighted_sum;

      loss_all[i] = ce_i;
    }

    // Average across all anchors
    return loss_all.mean();
  }


public:
  std::shared_ptr<TCCEncoderImpl> model_;
  TCCOptions opts_;
  int64_t input_dim_;
  std::unique_ptr<torch::optim::Optimizer> optimizer_;
  std::unique_ptr<torch::optim::LRScheduler> scheduler_;
};

} // namespace ts_tcc
} // namespace wikimyei
} // namespace cuwacunu
