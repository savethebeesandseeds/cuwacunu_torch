/* mixture_density_network_loss.h */
#pragma once
#include <torch/torch.h>

#include "wikimyei/inference/expected_value/mdn/mixture_density_network_types.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network_utils.h"

namespace cuwacunu {
namespace wikimyei {
namespace inference {
namespace expected_value {
namespace mdn {

// ---------- Loss ----------
// Wikimyei owns the density math; Jkimyei owns training policy decoding.
// Configure this loss with MdnNllOptions produced by the MDN spec/trainer:
//   eps=<float>           (default 1e-6)
//   sigma_min=<float>     (default 1e-3, safety floor; the active head also
//                          emits sigma with its smooth floor already applied)
//   sigma_max=<float>     (default 0.0 -> disabled)
//   reduction=<mean|sum>  (default mean)
struct MdnNLLLoss {
  double eps_{1e-6};
  double sigma_min_{1e-3};
  double sigma_max_{0.0};
  bool reduce_mean_{true};

  explicit MdnNLLLoss(const MdnNllOptions &options = {},
                      bool reduce_mean = true)
      : eps_(options.eps), sigma_min_(options.sigma_min),
        sigma_max_(options.sigma_max), reduce_mean_(reduce_mean) {}

  // Active masked NLL with optional weights:
  // out: log_pi/mu/sigma [B,N,C,Df,K]
  // f  : [B,N,C,Df]
  // mask        (optional): [B,N,C,Df] - 1 valid, 0 invalid
  // weights_ch  (optional): [C]
  // weights_dim (optional): [Df]
  torch::Tensor
  compute(const MdnOut &out, const torch::Tensor &f,
          const torch::Tensor &mask = torch::Tensor(),
          const torch::Tensor &weights_ch = torch::Tensor(),
          const torch::Tensor &weights_tau = torch::Tensor(),
          const torch::Tensor &weights_dim = torch::Tensor()) const {
    TORCH_CHECK(f.defined(), "[MdnNLLLoss] future tensor f is undefined");
    TORCH_CHECK(out.log_pi.defined() && out.mu.defined() && out.sigma.defined(),
                "[MdnNLLLoss] output tensors must be defined");

    TORCH_CHECK(out.log_pi.dim() == 5,
                "[MdnNLLLoss] log_pi must be [B,N,C,Df,K]");
    TORCH_CHECK(out.mu.dim() == 5 && out.sigma.dim() == 5,
                "[MdnNLLLoss] mu/sigma must be [B,N,C,Df,K]");
    TORCH_CHECK(f.dim() == 4, "[MdnNLLLoss] f must be [B,N,C,Df]");

    const auto B = f.size(0);
    const auto N = f.size(1);
    const auto C = f.size(2);
    const auto Df = f.size(3);

    TORCH_CHECK(out.log_pi.size(0) == B && out.log_pi.size(1) == N &&
                    out.log_pi.size(2) == C && out.log_pi.size(3) == Df,
                "[MdnNLLLoss] shape mismatch (log_pi)");
    TORCH_CHECK(out.mu.sizes() == out.log_pi.sizes(),
                "[MdnNLLLoss] shape mismatch (mu)");
    TORCH_CHECK(out.sigma.sizes() == out.log_pi.sizes(),
                "[MdnNLLLoss] mu/sigma size mismatch");

    if (weights_tau.defined()) {
      TORCH_CHECK(false,
                  "[MdnNLLLoss] weights_tau is not valid for the one-step "
                  "feature-slot MDN");
    }
    MdnNllOptions nll_options{};
    nll_options.eps = eps_;
    nll_options.sigma_min = sigma_min_;
    nll_options.sigma_max = sigma_max_;
    auto nll = mdn_nll_map(out, f, nll_options);
    auto valid = torch::ones_like(nll); // [B,N,C,Df]
    if (mask.defined()) {
      TORCH_CHECK(mask.sizes() == torch::IntArrayRef({B, N, C, Df}),
                  "[MdnNLLLoss] mask must be [B,N,C,Df]");
      valid = mask.to(nll.dtype());
    }

    auto loss_sum = (nll * valid).sum();
    if (!reduce_mean_)
      return loss_sum;

    return mdn_balanced_channel_feature_nll(nll, valid, weights_ch,
                                            weights_dim);
  }

  // Compatibility operator (no mask/weights) — still available
  torch::Tensor operator()(const MdnOut &out, const torch::Tensor &f) const {
    return compute(out, f);
  }
};

} // namespace mdn
} // namespace expected_value
} // namespace inference
} // namespace wikimyei
} // namespace cuwacunu
