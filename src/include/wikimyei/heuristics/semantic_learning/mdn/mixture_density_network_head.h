/* mixture_density_network_head.h */
#pragma once
#include <torch/torch.h>
#include <vector>

#include "camahjucunu/BNF/implementations/training_components/training_components.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_types.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_utils.h"

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

// =============================
// Per-channel MDN Head (diagonal Gaussian mixture)
// Produces per-channel outputs by composing C heads.
// =============================

struct MdnHeadImpl : torch::nn::Module {
  int64_t Dy{0}, K{0}, Hf{1};
  torch::nn::Linear lin_pi{nullptr}, lin_mu{nullptr}, lin_s{nullptr};

  explicit MdnHeadImpl(const MdnHeadOptions& opt)
  : Dy(opt.Dy), K(opt.K), Hf(opt.Hf) {
    // per-head (per-channel) layers output Hf horizons
    lin_pi = register_module("lin_pi", torch::nn::Linear(opt.feature_dim, Hf * K));
    lin_mu = register_module("lin_mu", torch::nn::Linear(opt.feature_dim, Hf * K * Dy));
    lin_s  = register_module("lin_s",  torch::nn::Linear(opt.feature_dim, Hf * K * Dy));
    // --- sensible init: sigma ~ 0.1, mu ~ 0, pi ~ uniform via zero logits
    {
      torch::NoGradGuard ng;
      if (lin_mu->bias.defined()) lin_mu->bias.zero_();
      if (lin_pi->bias.defined()) lin_pi->bias.zero_();
      if (lin_s->bias.defined()) {
        const double init_sigma = 0.1;         // tunable
        const double b = cuwacunu::wikimyei::mdn::softplus_inv(init_sigma);
        lin_s->bias.fill_(b);
      }
    }

  }

  // Input: h [B,H]; Output (per-channel head): log_pi [B,1,Hf,K], mu/sigma [B,1,Hf,K,Dy]
  MdnOut forward(const torch::Tensor& h) {
    const auto B = h.size(0);

    auto raw_pi = lin_pi->forward(h);                   // [B,Hf*K]
    auto raw_mu = lin_mu->forward(h);                   // [B,Hf*K*Dy]
    auto raw_s  = lin_s ->forward(h);                   // [B,Hf*K*Dy]

    auto log_pi = torch::log_softmax(raw_pi.view({B, Hf, K}), /*dim=*/-1)
                    .unsqueeze(1);                      // [B,1,Hf,K]

    auto mu     = raw_mu.view({B, Hf, K, Dy})
                    .unsqueeze(1);                      // [B,1,Hf,K,Dy]

    auto sigma  = safe_softplus(raw_s).view({B, Hf, K, Dy})
                    .unsqueeze(1);                      // [B,1,Hf,K,Dy]

    return {log_pi, mu, sigma};
  }
};
TORCH_MODULE(MdnHead);

// Container of per-channel heads: concatenates along C
struct ChannelHeadsImpl : torch::nn::Module {
  std::vector<MdnHead> heads;
  int64_t C{1}, Hf{1}, Dy{0}, K{0}, H{0};

  ChannelHeadsImpl(int64_t C_, int64_t Hf_, int64_t Dy_, int64_t K_, int64_t H_)
  : C(C_), Hf(Hf_), Dy(Dy_), K(K_), H(H_) {
    heads.reserve(C);
    for (int64_t c = 0; c < C; ++c) {
      MdnHeadOptions ho{H, Dy, K, Hf};
      auto hd = MdnHead(ho);
      heads.push_back(register_module("head_" + std::to_string(c), hd));
    }
  }

  // h: [B,H] → out.cat over channels
  MdnOut forward(const torch::Tensor& h) {
    std::vector<torch::Tensor> pis, mus, sigmas;
    pis.reserve(C); mus.reserve(C); sigmas.reserve(C);
    for (int64_t c = 0; c < C; ++c) {
      auto o = heads[c]->forward(h); // [B,1,Hf,K], [B,1,Hf,K,Dy]
      pis.push_back(o.log_pi);
      mus.push_back(o.mu);
      sigmas.push_back(o.sigma);
    }
    auto log_pi = torch::cat(pis, /*dim=*/1);    // [B,C,Hf,K]
    auto mu     = torch::cat(mus, /*dim=*/1);    // [B,C,Hf,K,Dy]
    auto sigma  = torch::cat(sigmas, /*dim=*/1); // [B,C,Hf,K,Dy]
    return {log_pi, mu, sigma};
  }
};
TORCH_MODULE(ChannelHeads);

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
