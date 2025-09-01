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
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_types.h"

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

// =============================
// Utility helpers
// =============================
inline torch::Tensor safe_softplus(const torch::Tensor& x, double eps = 1e-6) {
  auto y = torch::softplus(x);
  if (eps <= 0.0) return y;
  auto eps_t = y.new_full({}, eps);   // scalar on same device/dtype as y
  return y + eps_t;
}

inline torch::Tensor logsumexp(const torch::Tensor& x, int64_t dim, bool keepdim=false) {
  // numerically stable logsumexp
  auto max_x = std::get<0>(x.max(dim, /*keepdim=*/true));
  auto out = max_x + (x - max_x).exp().sum(dim, /*keepdim=*/true).log();
  return keepdim ? out : out.squeeze(dim);
}

// Gaussian log-pdf for diagonal covariance.
// y: [B, Dy], mu/sigma: [B, K, Dy] (sigma > 0)
// returns [B, K]
inline torch::Tensor diag_gaussian_logpdf(const torch::Tensor& y,
                                          const torch::Tensor& mu,
                                          const torch::Tensor& sigma,
                                          double eps = 1e-6) {
  TORCH_CHECK(y.dim() == 2, "y must be [B, Dy]");
  TORCH_CHECK(mu.dim() == 3 && sigma.dim() == 3, "mu/sigma must be [B, K, Dy]");
  auto B  = y.size(0);
  auto Dy = y.size(1);
  TORCH_CHECK(mu.size(0) == B && sigma.size(0) == B, "batch mismatch");
  TORCH_CHECK(mu.size(2) == Dy && sigma.size(2) == Dy, "Dy mismatch");

  auto y_exp = y.unsqueeze(1);                 // [B,1,Dy]
  auto diff  = (y_exp - mu);                   // [B,K,Dy]
  auto var   = (sigma + eps) * (sigma + eps);  // [B,K,Dy]
  auto log_det = var.log().sum(-1);            // [B,K]
  auto quad    = (diff * diff / var).sum(-1);  // [B,K]
  static const double LOG2PI = std::log(2.0 * M_PI);
  return -0.5 * (Dy * LOG2PI + log_det + quad);
}

// =============================
// Loss & metrics
// =============================

// MDN negative log-likelihood (diagonal Gaussian mixture)
// out: MdnOut {log_pi [B,K], mu [B,K,Dy], sigma [B,K,Dy]}
// y: [B, Dy]
inline torch::Tensor mdn_nll(const MdnOut& out, const torch::Tensor& y) {
  auto log_comp = diag_gaussian_logpdf(y, out.mu, out.sigma); // [B,K]
  auto log_mix  = out.log_pi + log_comp;                      // [B,K]
  auto lse      = logsumexp(log_mix, /*dim=*/1, /*keepdim=*/false); // [B]
  return -lse.mean(); // scalar
}

// Mixture expectation E[y|x], diagonal case → [B, Dy]
inline torch::Tensor mdn_expectation(const MdnOut& out) {
  auto pi = out.log_pi.exp().unsqueeze(-1); // [B,K,1]
  return (pi * out.mu).sum(1);              // [B,Dy]
}

// One-step sampling: y_hat ~ mixture per batch → [B, Dy]
inline torch::Tensor mdn_sample_one_step(const MdnOut& out) {
  auto device = out.log_pi.device();
  auto B  = out.log_pi.size(0);
  auto Dy = out.mu.size(2);

  auto pi    = out.log_pi.exp(); // [B,K]
  auto k_idx = torch::multinomial(pi, /*num_samples=*/1, /*replacement=*/true).squeeze(-1); // [B]

  auto idx    = k_idx.view({B, 1, 1}).expand({B, 1, Dy});
  auto mu_sel = out.mu.gather(1, idx).squeeze(1);     // [B,Dy]
  auto sg_sel = out.sigma.gather(1, idx).squeeze(1);  // [B,Dy]

  auto eps = torch::randn(mu_sel.sizes(), mu_sel.options().device(device));
  return mu_sel + sg_sel * eps; // [B,Dy]
}

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
