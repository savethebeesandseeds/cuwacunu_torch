/* mixture_density_network_head.h */
#pragma once
#include <string>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/inference/expected_value/mdn/mixture_density_network_types.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network_utils.h"

namespace cuwacunu {
namespace wikimyei {
namespace inference {
namespace expected_value {
namespace mdn {

// =============================
// Per-channel, per-target-feature MDN head.
// Produces per-channel outputs by composing C heads.
// =============================

struct MdnHeadImpl : torch::nn::Module {
  int64_t Df{0}, K{0}, Hf{1};
  torch::nn::Linear lin_pi{nullptr}, lin_mu{nullptr}, lin_s{nullptr};

  explicit MdnHeadImpl(const MdnHeadOptions &opt)
      : Df(opt.Df), K(opt.K), Hf(opt.Hf) {
    TORCH_CHECK(Hf == 1,
                "[MdnHead] active channel MDN is one-step; Hf must be 1");
    lin_pi =
        register_module("lin_pi", torch::nn::Linear(opt.feature_dim, Df * K));
    lin_mu =
        register_module("lin_mu", torch::nn::Linear(opt.feature_dim, Df * K));
    lin_s =
        register_module("lin_s", torch::nn::Linear(opt.feature_dim, Df * K));
    // --- sensible init: sigma ~ 0.1, mu ~ 0, pi ~ uniform via zero logits
    {
      torch::NoGradGuard ng;
      if (lin_mu->bias.defined())
        lin_mu->bias.zero_();
      if (lin_pi->bias.defined())
        lin_pi->bias.zero_();
      if (lin_s->bias.defined()) {
        const double init_sigma = 0.1; // tunable
        const double b = softplus_inv(init_sigma);
        lin_s->bias.fill_(b);
      }
    }
  }

  // Input: h [B,H]; Output: log_pi/mu/sigma [B,1,1,Df,K].
  // The singleton axes are node and channel placeholders used by ChannelHeads.
  MdnOut forward(const torch::Tensor &h) {
    const auto B = h.size(0);

    auto raw_pi = lin_pi->forward(h); // [B,Df*K]
    auto raw_mu = lin_mu->forward(h); // [B,Df*K]
    auto raw_s = lin_s->forward(h);   // [B,Df*K]

    auto log_pi = torch::log_softmax(raw_pi.view({B, Df, K}), /*dim=*/-1)
                      .unsqueeze(1)
                      .unsqueeze(1); // [B,1,1,Df,K]

    auto mu = raw_mu.view({B, Df, K}).unsqueeze(1).unsqueeze(1); // [B,1,1,Df,K]

    auto sigma = safe_softplus(raw_s)
                     .view({B, Df, K})
                     .unsqueeze(1)
                     .unsqueeze(1); // [B,1,1,Df,K]

    return {log_pi, mu, sigma};
  }
};
TORCH_MODULE(MdnHead);

// Container of per-channel heads: concatenates along C
struct ChannelHeadsImpl : torch::nn::Module {
  std::vector<MdnHead> heads;
  int64_t C{1}, Hf{1}, Df{0}, K{0}, H{0};

  ChannelHeadsImpl(int64_t C_, int64_t Hf_, int64_t Df_, int64_t K_, int64_t H_)
      : C(C_), Hf(Hf_), Df(Df_), K(K_), H(H_) {
    TORCH_CHECK(Hf == 1,
                "[ChannelHeads] active channel MDN is one-step; Hf must be 1");
    heads.reserve(C);
    for (int64_t c = 0; c < C; ++c) {
      MdnHeadOptions ho{H, Df, K, Hf};
      auto hd = MdnHead(ho);
      heads.push_back(register_module("head_" + std::to_string(c), hd));
    }
  }

  // h: [B,H] -> log_pi/mu/sigma [B,1,C,Df,K]
  MdnOut forward(const torch::Tensor &h) {
    std::vector<torch::Tensor> pis, mus, sigmas;
    pis.reserve(C);
    mus.reserve(C);
    sigmas.reserve(C);
    for (int64_t c = 0; c < C; ++c) {
      auto o = heads[c]->forward(h); // [B,1,1,Df,K]
      pis.push_back(o.log_pi);
      mus.push_back(o.mu);
      sigmas.push_back(o.sigma);
    }
    auto log_pi = torch::cat(pis, /*dim=*/2);   // [B,1,C,Df,K]
    auto mu = torch::cat(mus, /*dim=*/2);       // [B,1,C,Df,K]
    auto sigma = torch::cat(sigmas, /*dim=*/2); // [B,1,C,Df,K]
    return {log_pi, mu, sigma};
  }
};
TORCH_MODULE(ChannelHeads);

} // namespace mdn
} // namespace expected_value
} // namespace inference
} // namespace wikimyei
} // namespace cuwacunu
