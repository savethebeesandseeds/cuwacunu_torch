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

#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
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

inline torch::Tensor mdn_log_prob(const MdnOut& out, const torch::Tensor& f, double eps = 1e-6) {
  TORCH_CHECK(f.dim() == 4, "[mdn_log_prob] f must be [B,C,Hf,Df]");
  const auto B  = f.size(0), C = f.size(1), Hf = f.size(2), Df = f.size(3);
  TORCH_CHECK(out.mu.size(0)==B && out.mu.size(1)==C && out.mu.size(2)==Hf && out.mu.size(4)==Df,
              "[mdn_log_prob] shape mismatch");
  constexpr double LOG2PI = 1.8378770664093453; // log(2π)
  auto f_b   = f.unsqueeze(3).expand({B,C,Hf,out.log_pi.size(3),Df});  // [B,C,Hf,K,Df]
  auto eps_t = out.sigma.new_full({}, eps);
  auto sigma = (out.sigma + eps_t);
  auto diff  = (f_b - out.mu) / sigma;
  auto perd  = -0.5 * diff.pow(2) - sigma.log() - 0.5 * LOG2PI;        // [B,C,Hf,K,Df]
  auto comp  = perd.sum(-1);                                           // [B,C,Hf,K]
  return torch::logsumexp(out.log_pi + comp, 3);                       // [B,C,Hf]
}

inline torch::Tensor mdn_mode(const MdnOut& out) {
  auto argmax = std::get<1>(out.log_pi.max(/*dim=*/3, /*keepdim=*/true)); // [B,C,Hf,1]
  auto idx = argmax.unsqueeze(-1).expand_as(out.mu);                       // [B,C,Hf,1,Df]
  return out.mu.gather(/*dim=*/3, idx).squeeze(3);                         // [B,C,Hf,Df]
}

inline torch::Tensor mdn_topk_expectation(const MdnOut& out, int64_t topk) {
  topk = std::min<int64_t>(topk, out.log_pi.size(3));
  auto top = std::get<0>(out.log_pi.topk(topk, /*dim=*/3, /*largest=*/true, /*sorted=*/true));  // [B,C,Hf,topk]
  auto idx = std::get<1>(out.log_pi.topk(topk, /*dim=*/3, /*largest=*/true, /*sorted=*/true));  // [B,C,Hf,topk]
  auto pi_top = torch::softmax(top, /*dim=*/3).unsqueeze(-1);                                    // [B,C,Hf,topk,1]
  auto mu_top = out.mu.gather(/*dim=*/3, idx.unsqueeze(-1).expand({idx.size(0), idx.size(1), idx.size(2), topk, out.mu.size(4)}));
  return (pi_top * mu_top).sum(/*dim=*/3);                                                       // [B,C,Hf,Df]
}

// ---------- NLL map + masked reductions (general-purpose) ----------
struct MdnNllOptions {
  double eps{1e-6};
  double sigma_min{1e-3};  // 0 disables
  double sigma_max{0.0};   // 0 disables
};

// Returns per-(B,C,Hf) negative log-likelihood map. If mask is defined, it is applied (0=ignore, 1=valid).
inline torch::Tensor mdn_nll_map(const MdnOut& out,
                                 const torch::Tensor& f,         // [B,C,Hf,Df]
                                 const torch::Tensor& mask = {}, // [B,C,Hf] or undef
                                 const MdnNllOptions& opt = {}) {
  TORCH_CHECK(f.dim()==4, "[mdn_nll_map] f must be [B,C,Hf,Df]");
  const auto B  = f.size(0), C = f.size(1), Hf = f.size(2), Df = f.size(3);
  TORCH_CHECK(out.mu.size(0)==B && out.mu.size(1)==C && out.mu.size(2)==Hf && out.mu.size(4)==Df,
              "[mdn_nll_map] shape mismatch");
  const auto K = out.log_pi.size(3);

  // Numerically stable σ handling
  auto eps_t = out.sigma.new_full({}, opt.eps);
  auto sigma = out.sigma + eps_t;
  if (opt.sigma_min > 0.0) sigma = sigma.clamp_min(opt.sigma_min);
  if (opt.sigma_max > 0.0) sigma = sigma.clamp_max(opt.sigma_max);

  // Portable constant (avoid platform M_PI)
  constexpr double LOG2PI = 1.8378770664093453; // log(2π)

  auto f_b   = f.unsqueeze(3).expand({B, C, Hf, K, Df});               // [B,C,Hf,K,Df]
  auto diff  = (f_b - out.mu) / sigma;
  auto perd  = -0.5 * diff.pow(2) - sigma.log() - 0.5 * LOG2PI; // [B,C,Hf,K,Df]
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

// ---------- Conditional expectation for generalized shapes ----------
// out: log_pi [B,C,Hf,K], mu [B,C,Hf,K,Df]
// returns E[F|X] = sum_k pi * mu -> [B,C,Hf,Df]
inline torch::Tensor mdn_expectation(const MdnOut& out) {
  auto pi = out.log_pi.exp().unsqueeze(-1); // [B,C,Hf,K,1]
  return (pi * out.mu).sum(/*dim=*/3);      // → [B,C,Hf,Df]
}

// ---------- One-step sampling (generalized) ------------
// Sample a component per (B,C,Hf), then f ~ N(mu, sigma^2)
// returns [B,C,Hf,Df]
inline torch::Tensor mdn_sample_one_step(const MdnOut& out) {
  auto B  = out.log_pi.size(0);
  auto C  = out.log_pi.size(1);
  auto Hf = out.log_pi.size(2);
  auto K  = out.log_pi.size(3);
  auto Df = out.mu.size(4);

  auto pi = out.log_pi.exp().reshape({B * C * Hf, K}); // [B*C*Hf,K]
  auto k_idx = torch::multinomial(pi, /*num_samples=*/1, /*replacement=*/true).squeeze(-1); // [B*C*Hf]

  auto mu_ = out.mu.reshape({B*C*Hf, K, Df});
  auto sg_ = out.sigma.reshape({B*C*Hf, K, Df});

  auto idx = k_idx.view({B*C*Hf, 1, 1}).expand({B*C*Hf, 1, Df});
  auto mu_sel = mu_.gather(/*dim=*/1, idx).squeeze(1); // [B*C*Hf,Df]
  auto sg_sel = sg_.gather(/*dim=*/1, idx).squeeze(1); // [B*C*Hf,Df]

  auto eps = torch::randn_like(mu_sel);
  auto f = mu_sel + sg_sel * eps;                      // [B*C*Hf,Df]
  return f.view({B, C, Hf, Df});
}

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
