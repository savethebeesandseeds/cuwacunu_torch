/* mixture_density_network_head.h */
#pragma once
#include <torch/torch.h>
#include "camahjucunu/BNF/implementations/training_components/training_components.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_types.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_utils.h"

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

// =============================
// MDN Head (diagonal Gaussian mixture)
// =============================
struct MdnHeadImpl : torch::nn::Module {
  int64_t Dy{0}, K{0};
  torch::nn::Linear lin_pi{nullptr}, lin_mu{nullptr}, lin_s{nullptr};

  explicit MdnHeadImpl(const MdnHeadOptions& opt)
      : Dy(opt.Dy), K(opt.K) {
    lin_pi = register_module("lin_pi", torch::nn::Linear(opt.feature_dim, K));
    lin_mu = register_module("lin_mu", torch::nn::Linear(opt.feature_dim, K * Dy));
    lin_s  = register_module("lin_s",  torch::nn::Linear(opt.feature_dim, K * Dy));
  }

  MdnOut forward(const torch::Tensor& h) {
    auto B      = h.size(0);
    auto log_pi = torch::log_softmax(lin_pi->forward(h), /*dim=*/-1); // [B,K]
    auto mu     = lin_mu->forward(h).view({B, K, Dy});                // [B,K,Dy]
    auto sigma  = safe_softplus(lin_s->forward(h)).view({B, K, Dy});  // [B,K,Dy], >0
    return {log_pi, mu, sigma};
  }
};
TORCH_MODULE(MdnHead);

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
