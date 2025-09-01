/* mixture_density_network_types.h */
#pragma once
#include <torch/torch.h>

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

// ---------- Output container ----------
struct MdnOut {
  torch::Tensor log_pi; // [B, K]
  torch::Tensor mu;     // [B, K, Dy]
  torch::Tensor sigma;  // [B, K, Dy] (positive)
};

struct MdnHeadOptions {
  int64_t feature_dim; // from backbone
  int64_t Dy;          // target dimension
  int64_t K;           // number of mixture components
};

struct BackboneOptions {
  int64_t input_dim;   // e.g., De (representation dim)
  int64_t feature_dim; // e.g., 256/384/512
  int64_t depth;       // number of residual blocks (e.g., 2 or 3)
};

struct residualOptions {
  int64_t in_dim;
  int64_t hidden;
};

struct InferenceConfig {};

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
