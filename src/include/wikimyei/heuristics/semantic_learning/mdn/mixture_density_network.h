/* mixture_density_network.h
 *
 * Mixture Density Network with a shared backbone and per-channel, per-horizon heads.
 * This header declares the MdnModelImpl module (no training loop here).
 */
#pragma once
#include <torch/torch.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "piaabo/dutils.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_loss.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_types.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_utils.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_head.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_backbone.h"

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

/**
 * # MdnModelImpl
 * Mixture Density Network with a **shared trunk** (Backbone) and **per-channel, per-horizon heads** (ChannelHeads).
 *
 * ## What this file is (and isn't)
 * - **Pure architecture only**: no optimizer, scheduler, loss, or training loop here.
 * - The *ExpectedValue* wrapper owns all training wiring and target/mask shaping.
 *
 * ## Shapes (by convention)
 * - Input encoding:
 *     - `[B, De]`                — single embedding per sample, or
 *     - `[B, T', De]`            — a sequence of embeddings; we reduce with mean over `T'`.
 * - Outputs (per batch, channel, horizon, mixture component, target-dim):
 *     - `log_pi : [B, C, Hf, K]`
 *     - `mu     : [B, C, Hf, K, Dy]`
 *     - `sigma  : [B, C, Hf, K, Dy]` (positive; diagonal covariance)
 *
 * ## Key symbols
 * - `B`  : batch size
 * - `De` : input embedding dimension
 * - `Dy` : target feature dimension (e.g., if predicting `{close}` then Dy=1; `{high,close}` then Dy=2)
 * - `C`  : number of channels
 * - `Hf` : number of forecast horizons (time steps ahead)
 * - `K`  : number of mixture components
 * - `H`  : hidden feature width of the trunk
 * - `depth` : number of residual blocks in the trunk
 *
 * ## Typical call flow
 * ```
 * // encoding is [B,De] or [B,T',De]
 * auto out = model->forward_from_encoding(encoding);
 * // out.{log_pi, mu, sigma} match shapes above
 * ```
 *
 * ## Gotchas / tips
 * - **Dy must match your target selection**: If predict `{1,3}` from `future_features[..., D]`,
 *   then construct the model with `Dy=2` and make sure the loss selects those same two dims.
 * - **C and Hf are architectural**: the head is built to output for *all* channels and horizons.
 *   Pass the values intended to train/evaluate with.
 * - **Temporal reduction**: when passing `[B,T',De]` encodings, this class uses a **mean** over `T'`.
 *   If needed a different reduction (e.g., last, attention), change `temporal_pool`.
 */
struct MdnModelImpl : torch::nn::Module {
  // --- Architecture hyperparameters (immutable after construction)
  int64_t De{0};     ///< Input embedding dimension
  int64_t Dy{0};     ///< Target dimension per (channel,horizon)
  int64_t C_axes{1}; ///< Number of channels (heads are replicated C times)
  int64_t Hf_axes{1};///< Forecast horizons per channel
  int64_t K{0};      ///< Mixture components
  int64_t H{0};      ///< Trunk hidden width
  int64_t depth{0};  ///< Trunk residual depth

  // --- Execution precision and device placement
  torch::Dtype  dtype{torch::kFloat32};
  torch::Device device{torch::kCPU};

  // --- Submodules
  Backbone     backbone{nullptr};   ///< Residual MLP trunk: [B,De] -> [B,H]
  ChannelHeads ch_heads{nullptr};   ///< Per-(C,Hf) MDN heads: [B,H] -> MdnOut

  /**
   * ## Constructor (explicit; recommended)
   * Build the architecture with explicit sizes (no config reads here).
   *
   * @param De_    Input embedding dim
   * @param Dy_    Target dim per (channel,horizon)
   * @param C_     Number of channels (>=1)
   * @param Hf_    Number of forecast horizons per channel (>=1)
   * @param K_     Mixture components
   * @param H_     Trunk hidden width
   * @param depth_ Trunk residual depth (number of ResMLP blocks)
   * @param dtype_ Torch dtype for params and compute
   * @param device_ Torch device for the module
   */
  MdnModelImpl(
      int64_t De_,
      int64_t Dy_,
      int64_t C_,
      int64_t Hf_,
      int64_t K_,
      int64_t H_,
      int64_t depth_,
      torch::Dtype dtype_ = torch::kFloat32,
      torch::Device device_ = torch::kCPU,
      bool display_model_ = true);

  /**
   * ## Temporal pooling for encodings
   * Accepts either `[B,De]` or `[B,T',De]`. For the latter, we apply a mean over `T'`.
   * Change here if you need another reduction strategy.
   */
  static torch::Tensor temporal_pool(const torch::Tensor& enc);

  /**
   * Forward from a single-step embedding `[B,De]` (kept for back-compat).
   * Prefer `forward_from_encoding()` if you may pass `[B,T',De]`.
   */
  MdnOut forward(const torch::Tensor& x);

  /**
   * Forward from encoding `[B,De]` or `[B,T',De]` (mean-pooled over T' if present).
   */
  MdnOut forward_from_encoding(const torch::Tensor& encoding);

  /**
   * Convenience: E[y|x] directly from encoding (handles temporal pooling)
   */
  torch::Tensor expectation_from_encoding(const torch::Tensor& encoding);

  /**
   * Lightweight warmup to initialize CUDA kernels / allocator paths.
   * No-op on CPU. Safe to remove if you don’t care about first-iteration latency.
   */
  void warm_up();

  /// Placeholder for any future inference utilities (sampling, etc.).
  std::vector<torch::Tensor> inference(const torch::Tensor& /*enc*/, const InferenceConfig& /*cfg*/);

  /// Pretty-print the current architecture and placement.
  void display_model() const;
};

TORCH_MODULE(MdnModel);

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
