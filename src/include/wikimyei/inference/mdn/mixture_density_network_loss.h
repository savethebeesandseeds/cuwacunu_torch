/* mixture_density_network_loss.h */
#pragma once
#include <torch/torch.h>
#include <cctype>
#include <string>

#include "piaabo/dutils.h"
#include "wikimyei/inference/mdn/mixture_density_network_types.h"
#include "wikimyei/inference/mdn/mixture_density_network_utils.h"

#include "jkimyei/training_setup/jk_setup.h"
#include "camahjucunu/BNF/implementations/training_components/training_components.h"

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

// ---------- Loss (configurable via jk_component.loss_conf) ----------
// options (table row id == jk_component.loss_conf.id):
//   eps=<float>           (default 1e-6)
//   sigma_min=<float>     (default 1e-3)
//   sigma_max=<float>     (default 0.0 → disabled)
//   reduction=<mean|sum>  (default mean)
struct MdnNLLLoss {
  double eps_{1e-6};
  double sigma_min_{1e-3};
  double sigma_max_{0.0};
  bool   reduce_mean_{true};

  explicit MdnNLLLoss(const cuwacunu::jkimyei::jk_component_t& jk_component) {
    ASSERT(jk_component.loss_conf.type == "NLLLoss",
      ("Review <training_components>.instruction: MDN requires loss type 'NLLLoss', got '" + jk_component.loss_conf.type + "'.").c_str());
    try {
      const auto& row = jk_component.inst.retrive_row("loss_functions_table", jk_component.loss_conf.id);
      if (cuwacunu::camahjucunu::has_option(row, "eps"))
        eps_ = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "eps"));
      if (cuwacunu::camahjucunu::has_option(row, "sigma_min"))
        sigma_min_ = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "sigma_min"));
      if (cuwacunu::camahjucunu::has_option(row, "sigma_max"))
        sigma_max_ = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "sigma_max"));
      if (cuwacunu::camahjucunu::has_option(row, "reduction")) {
        std::string red = cuwacunu::camahjucunu::require_option(row, "reduction");
        for (auto& c : red) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        reduce_mean_ = (red != "sum");
      }
    } catch (...) { /* keep defaults */ }
  }

  // Generalized masked NLL with optional weights:
  // out: log_pi [B,C,Hf,K], mu/sigma [B,C,Hf,K,Dy]
  // y  : [B,C,Hf,Dy]
  // mask         (optional): [B,C,Hf] — 1 valid, 0 invalid
  // weights_ch   (optional): [C]     /* loss weights across the channel rank */
  // weights_tau  (optional): [Hf]    /* loss weights across the temporal rank */
  // weights_dim  (optional): [Dy]    /* loss weights across the feature rank */
  torch::Tensor compute(const MdnOut& out,
                        const torch::Tensor& y,
                        const torch::Tensor& mask = torch::Tensor(),
                        const torch::Tensor& weights_ch  = torch::Tensor(),
                        const torch::Tensor& weights_tau = torch::Tensor(),
                        const torch::Tensor& weights_dim = torch::Tensor()) const
  {
    TORCH_CHECK(y.defined(), "[MdnNLLLoss] target y is undefined");
    TORCH_CHECK(out.log_pi.defined() && out.mu.defined() && out.sigma.defined(),
                "[MdnNLLLoss] output tensors must be defined");

    TORCH_CHECK(out.log_pi.dim() == 4, "[MdnNLLLoss] log_pi must be [B,C,Hf,K]");
    TORCH_CHECK(out.mu.dim() == 5 && out.sigma.dim() == 5, "[MdnNLLLoss] mu/sigma must be [B,C,Hf,K,Dy]");
    TORCH_CHECK(y.dim() == 4, "[MdnNLLLoss] y must be [B,C,Hf,Dy]");

    const auto B  = y.size(0);
    const auto C  = y.size(1);
    const auto Hf = y.size(2);
    const auto Dy = y.size(3);
    const auto K  = out.log_pi.size(3);

    TORCH_CHECK(out.mu.size(0) == B && out.mu.size(1) == C && out.mu.size(2) == Hf && out.mu.size(4) == Dy, "[MdnNLLLoss] shape mismatch (mu)");
    TORCH_CHECK(out.sigma.sizes() == out.mu.sizes(), "[MdnNLLLoss] mu/sigma size mismatch");
    TORCH_CHECK(out.log_pi.size(0) == B && out.log_pi.size(1) == C && out.log_pi.size(2) == Hf && out.log_pi.size(3) == K, "[MdnNLLLoss] shape mismatch (log_pi)");

    // broadcast y -> [B,C,Hf,K,Dy]
    auto y_b  = y.unsqueeze(3).expand({B,C,Hf,K,Dy});

    // clamp sigma
    auto eps_t = out.sigma.new_full({}, eps_);
    auto sigma = out.sigma + eps_t;                       // additive ε floor
    if (sigma_min_ > 0.0) sigma = sigma.clamp_min(sigma_min_);  // optional hard floor
    if (sigma_max_ > 0.0) sigma = sigma.clamp_max(sigma_max_);

    static constexpr double LOG2PI = 1.8378770664093453; // log(2π)

    // per-dimension log-prob (do not sum over Dy yet)
    auto diff   = (y_b - out.mu) / (sigma + eps_t);
    auto perdim = -0.5 * diff.pow(2) - (sigma + eps_t).log() - 0.5 * LOG2PI; // [B,C,Hf,K,Dy]

    // (NEW) optional feature weights w_d
    if (weights_dim.defined()) {
      TORCH_CHECK(weights_dim.dim() == 1 && weights_dim.size(0) == Dy,
                  "[MdnNLLLoss] weights_dim must be [Dy]");
      auto wd = weights_dim.to(perdim.dtype()).view({1,1,1,1,Dy});
      perdim = perdim * wd;
    }

    // sum over Dy → component log-prob
    auto comp_logp = perdim.sum(-1);                                 // [B,C,Hf,K]
    auto log_mix = torch::logsumexp(out.log_pi + comp_logp, 3);      // [B,C,Hf]
    auto nll       = -log_mix;                                       // [B,C,Hf]

    // --- unified weighting/masking/mean ---
    auto w = torch::ones_like(nll);                                  // [B,C,Hf]
    if (weights_ch.defined()) {
      TORCH_CHECK(weights_ch.dim()==1 && weights_ch.size(0)==C, "[MdnNLLLoss] weights_ch must be [C]");
      w = w * weights_ch.to(nll.dtype()).view({1, C, 1});
    }
    if (weights_tau.defined()) {
      TORCH_CHECK(weights_tau.dim()==1 && weights_tau.size(0)==Hf, "[MdnNLLLoss] weights_tau must be [Hf]");
      w = w * weights_tau.to(nll.dtype()).view({1, 1, Hf});
    }
    if (mask.defined()) {
      TORCH_CHECK(mask.sizes() == torch::IntArrayRef({B, C, Hf}), "[MdnNLLLoss] mask must be [B,C,Hf]");
      w = w * mask.to(nll.dtype());
    }

    auto loss_sum = (nll * w).sum();
    if (!reduce_mean_) return loss_sum;

    auto denom = w.sum().clamp_min(1.0);
    return loss_sum / denom;
  }

  // Compatibility operator (no mask/weights) — still available
  torch::Tensor operator()(const MdnOut& out, const torch::Tensor& y) const {
    return compute(out, y);
  }
};

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
