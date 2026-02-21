/* mixture_density_network_utils.h */
#pragma once
#include <torch/torch.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <memory>
#include <functional>
#include <string>
#include <unordered_map>

#include "camahjucunu/BNF/implementations/training_components/training_components.h"
#include "wikimyei/inference/mdn/mixture_density_network_types.h"

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

// =============================
// Generic LR getter
// =============================
static inline std::vector<double> get_lrs(const torch::optim::Optimizer& opt) {
  std::vector<double> lrs;
  lrs.reserve(opt.param_groups().size());
  for (const auto& pg : opt.param_groups()) {
    const auto& base = pg.options();
    if (auto* o = dynamic_cast<const torch::optim::AdamOptions*>(&base))       lrs.push_back(o->lr());
    else if (auto* o = dynamic_cast<const torch::optim::AdamWOptions*>(&base)) lrs.push_back(o->lr());
    else if (auto* o = dynamic_cast<const torch::optim::SGDOptions*>(&base))   lrs.push_back(o->lr());
    else if (auto* o = dynamic_cast<const torch::optim::RMSpropOptions*>(&base)) lrs.push_back(o->lr());
    else lrs.push_back(std::numeric_limits<double>::quiet_NaN());
  }
  return lrs;
}

static inline double get_lr_generic(const torch::optim::Optimizer& opt) {
  auto v = get_lrs(opt);
  return v.empty() ? std::numeric_limits<double>::quiet_NaN() : v.front();
}


// =============================
// Utility helpers
// =============================
inline torch::Tensor safe_softplus(const torch::Tensor& x, double eps = 1e-6) {
  auto y = torch::softplus(x);
  if (eps <= 0.0) return y;
  auto eps_t = y.new_full({}, eps);   // scalar on same device/dtype as y
  return y + eps_t;
}

inline double softplus_inv(double y) {
  // inverse of softplus: x = log(exp(y) - 1)
  // guard against y→0 for numerical stability
  const double y_safe = std::max(y, 1e-12);
  return std::log(std::exp(y_safe) - 1.0);
}

inline torch::Tensor mdn_log_prob(const MdnOut& out, const torch::Tensor& y, double eps = 1e-6) {
  TORCH_CHECK(y.dim() == 4, "[mdn_log_prob] y must be [B,C,Hf,Dy]");
  const auto B  = y.size(0), C = y.size(1), Hf = y.size(2), Dy = y.size(3);
  TORCH_CHECK(out.mu.size(0)==B && out.mu.size(1)==C && out.mu.size(2)==Hf && out.mu.size(4)==Dy,
              "[mdn_log_prob] shape mismatch");
  constexpr double LOG2PI = 1.8378770664093453; // log(2π)
  auto y_b   = y.unsqueeze(3).expand({B,C,Hf,out.log_pi.size(3),Dy});  // [B,C,Hf,K,Dy]
  auto eps_t = out.sigma.new_full({}, eps);
  auto sigma = (out.sigma + eps_t);
  auto diff  = (y_b - out.mu) / sigma;
  auto perd  = -0.5 * diff.pow(2) - sigma.log() - 0.5 * LOG2PI;        // [B,C,Hf,K,Dy]
  auto comp  = perd.sum(-1);                                           // [B,C,Hf,K]
  return torch::logsumexp(out.log_pi + comp, 3);                       // [B,C,Hf]
}

inline torch::Tensor mdn_mode(const MdnOut& out) {
  auto argmax = std::get<1>(out.log_pi.max(/*dim=*/3, /*keepdim=*/true)); // [B,C,Hf,1]
  auto idx = argmax.unsqueeze(-1).expand_as(out.mu);                       // [B,C,Hf,1,Dy]
  return out.mu.gather(/*dim=*/3, idx).squeeze(3);                         // [B,C,Hf,Dy]
}

inline torch::Tensor mdn_topk_expectation(const MdnOut& out, int64_t topk) {
  topk = std::min<int64_t>(topk, out.log_pi.size(3));
  auto top = std::get<0>(out.log_pi.topk(topk, /*dim=*/3, /*largest=*/true, /*sorted=*/true));  // [B,C,Hf,topk]
  auto idx = std::get<1>(out.log_pi.topk(topk, /*dim=*/3, /*largest=*/true, /*sorted=*/true));  // [B,C,Hf,topk]
  auto pi_top = torch::softmax(top, /*dim=*/3).unsqueeze(-1);                                    // [B,C,Hf,topk,1]
  auto mu_top = out.mu.gather(/*dim=*/3, idx.unsqueeze(-1).expand({idx.size(0), idx.size(1), idx.size(2), topk, out.mu.size(4)}));
  return (pi_top * mu_top).sum(/*dim=*/3);                                                       // [B,C,Hf,Dy]
}

// ---------- NLL map + masked reductions (general-purpose) ----------
struct MdnNllOptions {
  double eps{1e-6};
  double sigma_min{1e-3};  // 0 disables
  double sigma_max{0.0};   // 0 disables
};

// Returns per-(B,C,Hf) negative log-likelihood map. If mask is defined, it is applied (0=ignore, 1=valid).
inline torch::Tensor mdn_nll_map(const MdnOut& out,
                                 const torch::Tensor& y,         // [B,C,Hf,Dy]
                                 const torch::Tensor& mask = {}, // [B,C,Hf] or undef
                                 const MdnNllOptions& opt = {}) {
  TORCH_CHECK(y.dim()==4, "[mdn_nll_map] y must be [B,C,Hf,Dy]");
  const auto B  = y.size(0), C = y.size(1), Hf = y.size(2), Dy = y.size(3);
  TORCH_CHECK(out.mu.size(0)==B && out.mu.size(1)==C && out.mu.size(2)==Hf && out.mu.size(4)==Dy,
              "[mdn_nll_map] shape mismatch");
  const auto K = out.log_pi.size(3);

  // Numerically stable σ handling
  auto eps_t = out.sigma.new_full({}, opt.eps);
  auto sigma = out.sigma + eps_t;
  if (opt.sigma_min > 0.0) sigma = sigma.clamp_min(opt.sigma_min);
  if (opt.sigma_max > 0.0) sigma = sigma.clamp_max(opt.sigma_max);

  // Portable constant (avoid platform M_PI)
  constexpr double LOG2PI = 1.8378770664093453; // log(2π)

  auto y_b   = y.unsqueeze(3).expand({B, C, Hf, K, Dy});               // [B,C,Hf,K,Dy]
  auto diff  = (y_b - out.mu) / (sigma + eps_t);
  auto perd  = -0.5 * diff.pow(2) - (sigma + eps_t).log() - 0.5 * LOG2PI; // [B,C,Hf,K,Dy]
  auto comp  = perd.sum(-1);                                            // [B,C,Hf,K]
  auto logp  = torch::logsumexp(out.log_pi + comp, /*dim=*/3);          // [B,C,Hf]
  auto nll   = -logp;                                                   // [B,C,Hf]
  if (mask.defined()) nll = nll * mask.to(nll.dtype());
  return nll;
}

// Average NLL per channel (mean over B and Hf with mask)
inline torch::Tensor mdn_masked_mean_per_channel(const torch::Tensor& nll, const torch::Tensor& mask) {
  TORCH_CHECK(nll.dim()==3, "[mdn_masked_mean_per_channel] nll must be [B,C,Hf]");
  auto valid = mask.defined() ? mask.to(nll.dtype()) : torch::ones_like(nll);
  auto sumB  = (nll * valid).sum(/*dim=*/0);             // [C,Hf]
  auto den   = valid.sum(/*dim=*/0).clamp_min(1.0);      // [C,Hf]
  return (sumB / den).mean(/*dim=*/1);                   // [C]
}

// Average NLL per horizon (mean over B and C with mask)
inline torch::Tensor mdn_masked_mean_per_horizon(const torch::Tensor& nll, const torch::Tensor& mask) {
  TORCH_CHECK(nll.dim()==3, "[mdn_masked_mean_per_horizon] nll must be [B,C,Hf]");
  auto valid = mask.defined() ? mask.to(nll.dtype()) : torch::ones_like(nll);
  auto sumBC = (nll * valid).sum(/*dim=*/std::vector<int64_t>{0,1});
  auto den   = valid.sum(/*dim=*/std::vector<int64_t>{0,1}).clamp_min(1.0);
  return sumBC / den;                                     // [Hf]
}

// ---------- Expectation for generalized shapes ----------
// out: log_pi [B,C,Hf,K], mu [B,C,Hf,K,Dy]
// returns E[y|x] = sum_k pi * mu → [B,C,Hf,Dy]
inline torch::Tensor mdn_expectation(const MdnOut& out) {
  auto pi = out.log_pi.exp().unsqueeze(-1); // [B,C,Hf,K,1]
  return (pi * out.mu).sum(/*dim=*/3);      // → [B,C,Hf,Dy]
}

// ---------- One-step sampling (generalized) ------------
// Sample a component per (B,C,Hf), then y ~ N(mu, sigma^2)
// returns [B,C,Hf,Dy]
inline torch::Tensor mdn_sample_one_step(const MdnOut& out) {
  auto B  = out.log_pi.size(0);
  auto C  = out.log_pi.size(1);
  auto Hf = out.log_pi.size(2);
  auto K  = out.log_pi.size(3);
  auto Dy = out.mu.size(4);

  auto pi = out.log_pi.exp().reshape({B * C * Hf, K}); // [B*C*Hf,K]
  auto k_idx = torch::multinomial(pi, /*num_samples=*/1, /*replacement=*/true).squeeze(-1); // [B*C*Hf]

  auto mu_ = out.mu.reshape({B*C*Hf, K, Dy});
  auto sg_ = out.sigma.reshape({B*C*Hf, K, Dy});

  auto idx = k_idx.view({B*C*Hf, 1, 1}).expand({B*C*Hf, 1, Dy});
  auto mu_sel = mu_.gather(/*dim=*/1, idx).squeeze(1); // [B*C*Hf,Dy]
  auto sg_sel = sg_.gather(/*dim=*/1, idx).squeeze(1); // [B*C*Hf,Dy]

  auto eps = torch::randn_like(mu_sel);
  auto y = mu_sel + sg_sel * eps;                      // [B*C*Hf,Dy]
  return y.view({B, C, Hf, Dy});
}

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
