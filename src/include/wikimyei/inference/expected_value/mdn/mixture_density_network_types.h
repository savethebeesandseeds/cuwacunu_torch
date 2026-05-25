/* mixture_density_network_types.h */
#pragma once
#include <cstdint>

#include <torch/torch.h>

namespace cuwacunu {
namespace wikimyei {
namespace inference {
namespace expected_value {
namespace mdn {

// ---------- Output container ----------
// Active channel-context MDN contract:
// log_pi : [B, N, C, Df, K]
// mu     : [B, N, C, Df, K]
// sigma  : [B, N, C, Df, K]
//
// B  = anchor batch
// N  = graph node slot
// C  = channel slot
// Df = selected one-step target feature slot
// K  = mixture component
struct MdnOut {
  torch::Tensor log_pi;
  torch::Tensor mu;
  torch::Tensor sigma;
};

// Head options (per-channel head, so C is handled outside)
struct MdnHeadOptions {
  int64_t feature_dim; // H, backbone output width
  int64_t Df;          // selected one-step target feature width
  int64_t K;           // mixture comps
  int64_t Hf{1};       // compatibility field; active MDN requires Hf == 1
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
} // namespace expected_value
} // namespace inference
} // namespace wikimyei
} // namespace cuwacunu
