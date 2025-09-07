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
// Generic LR getter
// =============================
static inline double get_lr_generic(const torch::optim::Optimizer& opt) {
  const auto& pg = opt.param_groups().front();
  if (typeid(pg.options()) == typeid(torch::optim::AdamOptions))
    return static_cast<const torch::optim::AdamOptions&>(pg.options()).lr();
  if (typeid(pg.options()) == typeid(torch::optim::AdamWOptions))
    return static_cast<const torch::optim::AdamWOptions&>(pg.options()).lr();
  if (typeid(pg.options()) == typeid(torch::optim::SGDOptions))
    return static_cast<const torch::optim::SGDOptions&>(pg.options()).lr();
  if (typeid(pg.options()) == typeid(torch::optim::RMSpropOptions))
    return static_cast<const torch::optim::RMSpropOptions&>(pg.options()).lr();
  return std::numeric_limits<double>::quiet_NaN();
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

inline torch::Tensor logsumexp(const torch::Tensor& x, int64_t dim, bool keepdim=false) {
  auto max_x = std::get<0>(x.max(dim, /*keepdim=*/true));
  auto out = max_x + (x - max_x).exp().sum(dim, /*keepdim=*/true).log();
  return keepdim ? out : out.squeeze(dim);
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
  auto device = out.log_pi.device();
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

  auto eps = torch::randn(mu_sel.sizes(), mu_sel.options().device(device));
  auto y = mu_sel + sg_sel * eps;                      // [B*C*Hf,Dy]
  return y.view({B, C, Hf, Dy});
}

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
