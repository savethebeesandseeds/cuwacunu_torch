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

inline torch::Tensor positive_sigma_from_raw(const torch::Tensor &raw,
                                             double sigma_floor = 1e-3) {
  TORCH_CHECK(sigma_floor >= 0.0,
              "[positive_sigma_from_raw] sigma_floor must be nonnegative");
  auto sigma = torch::softplus(raw);
  if (sigma_floor > 0.0) {
    sigma = sigma + sigma.new_full({}, sigma_floor);
  }
  return sigma;
}

inline double softplus_inv(double y) {
  const double y_safe = std::max(y, 1e-12);
  if (y_safe > 20.0) {
    return y_safe;
  }
  return std::log(std::expm1(y_safe));
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
  TORCH_CHECK(topk > 0, "[mdn_topk_expectation] topk must be positive");
  topk = std::min<int64_t>(topk, out.log_pi.size(-1));
  auto topk_result =
      out.log_pi.topk(topk, /*dim=*/-1, /*largest=*/true, /*sorted=*/true);
  auto top = std::get<0>(topk_result);
  auto idx = std::get<1>(topk_result);
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

// Returns raw per-(B,N,C,Df) negative log-likelihood. Masked reductions should
// apply the mask explicitly so invalid slots are never hidden as real zero NLL.
inline torch::Tensor mdn_nll_map(const MdnOut &out,
                                 const torch::Tensor &f, // [B,N,C,Df]
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
  return -logp;                                                // [B,N,C,Df]
}

inline torch::Tensor mdn_masked_nll_map(const MdnOut &out,
                                        const torch::Tensor &f,
                                        const torch::Tensor &mask,
                                        const MdnNllOptions &opt = {}) {
  auto nll = mdn_nll_map(out, f, opt);
  if (mask.defined()) {
    TORCH_CHECK(mask.sizes() == f.sizes(),
                "[mdn_masked_nll_map] mask must be [B,N,C,Df]");
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

// Average NLL per channel/target-feature cell (mean over B and N with mask)
inline torch::Tensor
mdn_masked_mean_per_channel_target_feature(const torch::Tensor &nll,
                                           const torch::Tensor &mask) {
  TORCH_CHECK(nll.dim() == 4,
              "[mdn_masked_mean_per_channel_target_feature] nll must be "
              "[B,N,C,Df]");
  auto valid = mask.defined() ? mask.to(nll.dtype()) : torch::ones_like(nll);
  auto sum = (nll * valid).sum(/*dim=*/std::vector<int64_t>{0, 1});
  auto den = valid.sum(/*dim=*/std::vector<int64_t>{0, 1}).clamp_min(1.0);
  return sum / den; // [C,Df]
}

inline torch::Tensor mdn_valid_count_per_channel(const torch::Tensor &mask) {
  TORCH_CHECK(mask.defined() && mask.dim() == 4,
              "[mdn_valid_count_per_channel] mask must be [B,N,C,Df]");
  return mask.to(torch::kInt64).sum(/*dim=*/std::vector<int64_t>{0, 1, 3});
}

inline torch::Tensor
mdn_valid_count_per_target_feature(const torch::Tensor &mask) {
  TORCH_CHECK(mask.defined() && mask.dim() == 4,
              "[mdn_valid_count_per_target_feature] mask must be [B,N,C,Df]");
  return mask.to(torch::kInt64).sum(/*dim=*/std::vector<int64_t>{0, 1, 2});
}

inline torch::Tensor
mdn_valid_count_per_channel_target_feature(const torch::Tensor &mask) {
  TORCH_CHECK(mask.defined() && mask.dim() == 4,
              "[mdn_valid_count_per_channel_target_feature] mask must be "
              "[B,N,C,Df]");
  return mask.to(torch::kInt64).sum(/*dim=*/std::vector<int64_t>{0, 1});
}

inline torch::Tensor mdn_balanced_channel_feature_nll(
    const torch::Tensor &nll, const torch::Tensor &mask,
    const torch::Tensor &weights_ch = torch::Tensor(),
    const torch::Tensor &weights_dim = torch::Tensor()) {
  TORCH_CHECK(nll.dim() == 4,
              "[mdn_balanced_channel_feature_nll] nll must be [B,N,C,Df]");
  const auto C = nll.size(2);
  const auto Df = nll.size(3);
  auto valid = mask.defined() ? mask.to(nll.dtype()) : torch::ones_like(nll);
  TORCH_CHECK(valid.sizes() == nll.sizes(),
              "[mdn_balanced_channel_feature_nll] mask must be [B,N,C,Df]");
  auto support = valid.sum(/*dim=*/std::vector<int64_t>{0, 1}); // [C,Df]
  auto sum = (nll * valid).sum(/*dim=*/std::vector<int64_t>{0, 1});
  auto mean = sum / support.clamp_min(1.0);
  auto active = support.gt(0).to(nll.dtype());
  auto weight = active;
  if (weights_ch.defined()) {
    TORCH_CHECK(weights_ch.dim() == 1 && weights_ch.size(0) == C,
                "[mdn_balanced_channel_feature_nll] weights_ch must be [C]");
    auto wch = weights_ch.to(nll.options()).view({C, 1});
    TORCH_CHECK(wch.ge(0).all().template item<bool>(),
                "[mdn_balanced_channel_feature_nll] weights_ch must be "
                "nonnegative");
    weight = weight * wch;
  }
  if (weights_dim.defined()) {
    TORCH_CHECK(weights_dim.dim() == 1 && weights_dim.size(0) == Df,
                "[mdn_balanced_channel_feature_nll] weights_dim must be [Df]");
    auto wdf = weights_dim.to(nll.options()).view({1, Df});
    TORCH_CHECK(wdf.ge(0).all().template item<bool>(),
                "[mdn_balanced_channel_feature_nll] weights_dim must be "
                "nonnegative");
    weight = weight * wdf;
  }
  const auto weight_sum = weight.sum();
  TORCH_CHECK(weight_sum.template item<double>() > 0.0,
              "[mdn_balanced_channel_feature_nll] no positive active "
              "channel-feature weights");
  return (mean * weight).sum() / weight_sum;
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
