#pragma once
#include <torch/torch.h>

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

// ---------- Output container (generalized) ----------
// log_pi : [B, C, Hf, K]
// mu     : [B, C, Hf, K, Dy]
// sigma  : [B, C, Hf, K, Dy]
struct MdnOut {
  torch::Tensor log_pi;
  torch::Tensor mu;
  torch::Tensor sigma;
};

// Head options (per-channel head, so C is handled outside)
struct MdnHeadOptions {
  int64_t feature_dim; // H, backbone output width
  int64_t Dy;          // target dims per (channel,horizon)
  int64_t K;           // mixture comps
  int64_t Hf{1};       // horizons per channel
};

struct BackboneOptions {
  int64_t input_dim;   // De
  int64_t feature_dim; // H
  int64_t depth;       // residual blocks
};

struct residualOptions {
  int64_t in_dim;
  int64_t hidden;
};

struct InferenceConfig {};

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
