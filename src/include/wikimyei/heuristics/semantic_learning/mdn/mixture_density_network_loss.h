#pragma once
#include <torch/torch.h>
#include <cctype>
#include <string>

#include "piaabo/dutils.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_types.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_utils.h"

#include "jkimyei/training_setup/jk_setup.h"
#include "camahjucunu/BNF/implementations/training_components/training_components.h"

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

// ---------- Loss (configurable via jk_setup.loss_conf) ----------
// options (table row id == jk_setup.loss_conf.id):
//   eps=<float>           (default 1e-6)
//   sigma_min=<float>     (default 1e-3)
//   sigma_max=<float>     (default 0.0 → disabled)
//   reduction=<mean|sum>  (default mean)
struct MdnNLLLoss {
  double eps_{1e-6};
  double sigma_min_{1e-3};
  double sigma_max_{0.0};
  bool   reduce_mean_{true};

  explicit MdnNLLLoss(const cuwacunu::jkimyei::jk_setup_t& jk_setup) {
    ASSERT(jk_setup.loss_conf.type == "NLLLoss",
      ("Review <training_components>.instruction: MDN requires loss type 'NLLLoss', got '" + jk_setup.loss_conf.type + "'.").c_str());
    try {
      const auto& row = jk_setup.inst.retrive_row("loss_functions_table", jk_setup.loss_conf.id);
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
  // weights_ch   (optional): [C]     /* loss weights acrros the channel rank */
  // weights_tau  (optional): [Hf]    /* loss weights acrros the temporal rank */
  // weights_dim  (optional): [Dy]    /* loss weights acrros the channel rank */
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
    auto sigma = out.sigma.clamp_min(sigma_min_);
    if (sigma_max_ > 0.0) sigma = sigma.clamp_max(sigma_max_);
    auto eps_t = sigma.new_full({}, eps_);
    static const double LOG2PI = std::log(2.0 * M_PI);

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
    auto log_mix   = logsumexp(out.log_pi + comp_logp, 3, false);    // [B,C,Hf]
    auto nll       = -log_mix;                                       // [B,C,Hf]

    // (NEW) optional per-channel and per-horizon weights
    if (weights_ch.defined()) {
      TORCH_CHECK(weights_ch.dim()==1 && weights_ch.size(0)==C, "[MdnNLLLoss] weights_ch must be [C]");
      nll = nll * weights_ch.to(nll.dtype()).view({1,C,1});
    }
    if (weights_tau.defined()) {
      TORCH_CHECK(weights_tau.dim()==1 && weights_tau.size(0)==Hf, "[MdnNLLLoss] weights_tau must be [Hf]");
      nll = nll * weights_tau.to(nll.dtype()).view({1,1,Hf});
    }

    // mask + reduction
    if (mask.defined()) {
      TORCH_CHECK(mask.sizes() == torch::IntArrayRef({B,C,Hf}),
                  "[MdnNLLLoss] mask must be [B,C,Hf]");
      auto valid = mask.to(nll.dtype());
      auto loss_sum = (nll * valid).sum();
      auto denom    = valid.sum().clamp_min(1.0);
      return reduce_mean_ ? (loss_sum / denom) : loss_sum;
    }

    return reduce_mean_ ? nll.mean() : nll.sum();
  }

  // Legacy operator (no mask/weights) — still available
  torch::Tensor operator()(const MdnOut& out, const torch::Tensor& y) const {
    return compute(out, y);
  }
};

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
