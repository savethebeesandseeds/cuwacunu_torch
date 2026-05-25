/* mixture_density_network_utils.h */
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/inference/expected_value/mdn/mixture_density_network_types.h"

namespace cuwacunu {
namespace wikimyei {
namespace inference {
namespace expected_value {
namespace mdn {

// =============================
// Generic LR getter
// =============================
static inline std::vector<double> get_lrs(const torch::optim::Optimizer &opt) {
  std::vector<double> lrs;
  lrs.reserve(opt.param_groups().size());
  for (const auto &pg : opt.param_groups()) {
    const auto &base = pg.options();
    if (auto *o = dynamic_cast<const torch::optim::AdamOptions *>(&base))
      lrs.push_back(o->lr());
    else if (auto *o = dynamic_cast<const torch::optim::AdamWOptions *>(&base))
      lrs.push_back(o->lr());
    else if (auto *o = dynamic_cast<const torch::optim::SGDOptions *>(&base))
      lrs.push_back(o->lr());
    else if (auto *o =
                 dynamic_cast<const torch::optim::RMSpropOptions *>(&base))
      lrs.push_back(o->lr());
    else
      lrs.push_back(std::numeric_limits<double>::quiet_NaN());
  }
  return lrs;
}

static inline double get_lr_generic(const torch::optim::Optimizer &opt) {
  auto v = get_lrs(opt);
  return v.empty() ? std::numeric_limits<double>::quiet_NaN() : v.front();
}

// =============================
// Utility helpers
// =============================
inline torch::Tensor safe_softplus(const torch::Tensor &x, double eps = 1e-6) {
  auto y = torch::softplus(x);
  if (eps <= 0.0)
    return y;
  auto eps_t = y.new_full({}, eps); // scalar on same device/dtype as y
  return y + eps_t;
}

inline double softplus_inv(double y) {
  // inverse of softplus: x = log(exp(y) - 1)
  // guard against y→0 for numerical stability
  const double y_safe = std::max(y, 1e-12);
  return std::log(std::exp(y_safe) - 1.0);
}

inline torch::Tensor mdn_log_prob(const MdnOut &out, const torch::Tensor &f,
                                  double eps = 1e-6) {
  TORCH_CHECK(f.dim() == 4, "[mdn_log_prob] f must be [B,N,C,Df]");
  const auto B = f.size(0), N = f.size(1), C = f.size(2), Df = f.size(3);
  TORCH_CHECK(out.log_pi.dim() == 5 && out.mu.dim() == 5 &&
                  out.sigma.dim() == 5,
              "[mdn_log_prob] MDN outputs must be [B,N,C,Df,K]");
  TORCH_CHECK(out.log_pi.size(0) == B && out.log_pi.size(1) == N &&
                  out.log_pi.size(2) == C && out.log_pi.size(3) == Df &&
                  out.mu.sizes() == out.log_pi.sizes() &&
                  out.sigma.sizes() == out.log_pi.sizes(),
              "[mdn_log_prob] shape mismatch");
  constexpr double LOG2PI = 1.8378770664093453; // log(2π)
  auto f_b = f.unsqueeze(-1).expand_as(out.mu); // [B,N,C,Df,K]
  auto eps_t = out.sigma.new_full({}, eps);
  auto sigma = (out.sigma + eps_t);
  auto diff = (f_b - out.mu) / sigma;
  auto perd = -0.5 * diff.pow(2) - sigma.log() - 0.5 * LOG2PI; // [B,N,C,Df,K]
  return torch::logsumexp(out.log_pi + perd, /*dim=*/-1);      // [B,N,C,Df]
}

inline torch::Tensor mdn_mode(const MdnOut &out) {
  auto argmax = std::get<1>(out.log_pi.max(/*dim=*/-1, /*keepdim=*/true));
  return out.mu.gather(/*dim=*/-1, argmax).squeeze(-1); // [B,N,C,Df]
}

inline torch::Tensor mdn_topk_expectation(const MdnOut &out, int64_t topk) {
  topk = std::min<int64_t>(topk, out.log_pi.size(-1));
  auto top = std::get<0>(
      out.log_pi.topk(topk, /*dim=*/-1, /*largest=*/true, /*sorted=*/true));
  auto idx = std::get<1>(
      out.log_pi.topk(topk, /*dim=*/-1, /*largest=*/true, /*sorted=*/true));
  auto pi_top = torch::softmax(top, /*dim=*/-1);
  auto mu_top = out.mu.gather(/*dim=*/-1, idx);
  return (pi_top * mu_top).sum(/*dim=*/-1); // [B,N,C,Df]
}

// ---------- NLL map + masked reductions (general-purpose) ----------
struct MdnNllOptions {
  double eps{1e-6};
  double sigma_min{1e-3}; // 0 disables
  double sigma_max{0.0};  // 0 disables
};

// Returns per-(B,N,C,Df) negative log-likelihood map. If mask is defined, it
// is applied (0=ignore, 1=valid).
inline torch::Tensor
mdn_nll_map(const MdnOut &out,
            const torch::Tensor &f,         // [B,N,C,Df]
            const torch::Tensor &mask = {}, // [B,N,C,Df] or undef
            const MdnNllOptions &opt = {}) {
  TORCH_CHECK(f.dim() == 4, "[mdn_nll_map] f must be [B,N,C,Df]");
  const auto B = f.size(0), N = f.size(1), C = f.size(2), Df = f.size(3);
  TORCH_CHECK(out.log_pi.dim() == 5 && out.mu.sizes() == out.log_pi.sizes() &&
                  out.sigma.sizes() == out.log_pi.sizes() &&
                  out.log_pi.size(0) == B && out.log_pi.size(1) == N &&
                  out.log_pi.size(2) == C && out.log_pi.size(3) == Df,
              "[mdn_nll_map] shape mismatch");

  // Numerically stable σ handling
  auto eps_t = out.sigma.new_full({}, opt.eps);
  auto sigma = out.sigma + eps_t;
  if (opt.sigma_min > 0.0)
    sigma = sigma.clamp_min(opt.sigma_min);
  if (opt.sigma_max > 0.0)
    sigma = sigma.clamp_max(opt.sigma_max);

  // Portable constant (avoid platform M_PI)
  constexpr double LOG2PI = 1.8378770664093453; // log(2π)

  auto f_b = f.unsqueeze(-1).expand_as(out.mu); // [B,N,C,Df,K]
  auto diff = (f_b - out.mu) / sigma;
  auto perd = -0.5 * diff.pow(2) - sigma.log() - 0.5 * LOG2PI; // [B,N,C,Df,K]
  auto logp = torch::logsumexp(out.log_pi + perd, /*dim=*/-1); // [B,N,C,Df]
  auto nll = -logp;                                            // [B,N,C,Df]
  if (mask.defined()) {
    TORCH_CHECK(mask.sizes() == f.sizes(),
                "[mdn_nll_map] mask must be [B,N,C,Df]");
    nll = nll * mask.to(nll.dtype());
  }
  return nll;
}

// Average NLL per channel (mean over B, N, and target feature with mask)
inline torch::Tensor mdn_masked_mean_per_channel(const torch::Tensor &nll,
                                                 const torch::Tensor &mask) {
  TORCH_CHECK(nll.dim() == 4,
              "[mdn_masked_mean_per_channel] nll must be [B,N,C,Df]");
  auto valid = mask.defined() ? mask.to(nll.dtype()) : torch::ones_like(nll);
  auto sum = (nll * valid).sum(/*dim=*/std::vector<int64_t>{0, 1, 3});
  auto den = valid.sum(/*dim=*/std::vector<int64_t>{0, 1, 3}).clamp_min(1.0);
  return sum / den; // [C]
}

// Average NLL per selected target feature (mean over B, N, and C with mask)
inline torch::Tensor
mdn_masked_mean_per_target_feature(const torch::Tensor &nll,
                                   const torch::Tensor &mask) {
  TORCH_CHECK(nll.dim() == 4,
              "[mdn_masked_mean_per_target_feature] nll must be [B,N,C,Df]");
  auto valid = mask.defined() ? mask.to(nll.dtype()) : torch::ones_like(nll);
  auto sum = (nll * valid).sum(/*dim=*/std::vector<int64_t>{0, 1, 2});
  auto den = valid.sum(/*dim=*/std::vector<int64_t>{0, 1, 2}).clamp_min(1.0);
  return sum / den; // [Df]
}

// ---------- Conditional expectation ----------
// out: log_pi/mu [B,N,C,Df,K]
// returns E[F|X] = sum_k pi * mu -> [B,N,C,Df]
inline torch::Tensor mdn_expectation(const MdnOut &out) {
  auto pi = out.log_pi.exp();
  return (pi * out.mu).sum(/*dim=*/-1);
}

// ---------- One-step sampling ------------
// Sample a component per (B,N,C,Df), then f ~ N(mu, sigma^2)
// returns [B,N,C,Df]
inline torch::Tensor mdn_sample_one_step(const MdnOut &out) {
  auto B = out.log_pi.size(0);
  auto N = out.log_pi.size(1);
  auto C = out.log_pi.size(2);
  auto Df = out.log_pi.size(3);
  auto K = out.log_pi.size(4);

  auto pi = out.log_pi.exp().reshape({B * N * C * Df, K});
  auto k_idx = torch::multinomial(pi, /*num_samples=*/1, /*replacement=*/true)
                   .squeeze(-1);

  auto mu_ = out.mu.reshape({B * N * C * Df, K});
  auto sg_ = out.sigma.reshape({B * N * C * Df, K});

  auto idx = k_idx.view({B * N * C * Df, 1});
  auto mu_sel = mu_.gather(/*dim=*/1, idx).squeeze(1);
  auto sg_sel = sg_.gather(/*dim=*/1, idx).squeeze(1);

  auto eps = torch::randn_like(mu_sel);
  auto f = mu_sel + sg_sel * eps;
  return f.view({B, N, C, Df});
}

} // namespace mdn
} // namespace expected_value
} // namespace inference
} // namespace wikimyei
} // namespace cuwacunu
