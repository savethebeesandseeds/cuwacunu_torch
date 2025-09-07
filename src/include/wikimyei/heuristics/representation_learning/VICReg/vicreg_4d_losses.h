/* vicreg_4d_losses.h */
#pragma once
#include <torch/torch.h>
#include <sstream>
#include <iomanip>
#include "camahjucunu/BNF/implementations/training_components/training_components.h"
#include "jkimyei/training_setup/jk_setup.h"

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

inline torch::Tensor off_diagonal(const torch::Tensor& m) {
  const auto S = m.size(0);
  TORCH_CHECK(S == m.size(1),
              "[vicreg_4d_losses.h](off_diagonal) off_diagonal expects a square matrix");
  // Take all but last element, then view as (S-1, S+1), drop first of each row, flatten
  return m.flatten().slice(/*dim=*/0, /*start=*/0, /*end=*/S * S - 1)
           .view({S - 1, S + 1})
           .slice(/*dim=*/1, /*start=*/1, /*end=*/S + 1)
           .flatten();
}

/** Holds VICReg loss components as tensors on the model's device/dtype. */
struct VicRegTerms {
  at::Tensor total;  // scalar [1]
  at::Tensor inv;    // MSE(x,y)
  at::Tensor var;    // variance hinge term
  at::Tensor cov;    // off-diagonal covariance penalty (robust)
};

struct VicRegLoss {
  // public coefficients (kept identical)
  double sim_coeff_;
  double std_coeff_;
  double cov_coeff_;

  // --- robust & safeguard defaults (no external config needed) ---
  // Huber transition on correlation magnitude |r| (since we whiten):
  //   0.5*r^2 for |r| <= delta,  delta*(|r| - 0.5*delta) otherwise
  double huber_delta_;     // conservative default (on correlation scale)
  // execution details
  bool   whiten_before_cov_;
  int    samples_per_dim_full_; // cov gets full weight by N >= samples_per_dim_full_*E

  explicit VicRegLoss(double sim_coeff=25.0,
                      double std_coeff=25.0,
                      double cov_coeff=1.0,
                      double huber_delta=0.03,
                      bool   whiten_before_cov=true,
                      int    samples_per_dim_full=8)
  : sim_coeff_(sim_coeff)
  , std_coeff_(std_coeff)
  , cov_coeff_(cov_coeff)
  , huber_delta_(huber_delta)
  , whiten_before_cov_(whiten_before_cov)
  , samples_per_dim_full_(samples_per_dim_full)
  {}

  explicit VicRegLoss(const cuwacunu::jkimyei::jk_component_t& jk_component)
  : VicRegLoss(
      cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_option(
          jk_component.inst.retrive_row("loss_functions_table", jk_component.loss_conf.id),
          "sim_coeff")),
      cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_option(
          jk_component.inst.retrive_row("loss_functions_table", jk_component.loss_conf.id),
          "std_coeff")),
      cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_option(
          jk_component.inst.retrive_row("loss_functions_table", jk_component.loss_conf.id),
          "cov_coeff")),
      cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_option(
          jk_component.inst.retrive_row("loss_functions_table", jk_component.loss_conf.id),
          "huber_delta"))
        )
  {
    ASSERT(jk_component.loss_conf.type=="VICReg",
      "Review <training_components>.instruction file, VICREG_4D requires loss to be of type VICREG.\n");
  }

  /** returns only the total loss */
  at::Tensor operator()(const at::Tensor& x_raw, const at::Tensor& y_raw) const {
    return forward_terms(x_raw, y_raw).total;
  }

  /** Huber penalty on a vector of residuals r (elementwise), returns sum (not mean). */
  static inline at::Tensor huber_sum(const at::Tensor& r, double delta) {
    auto abs_r  = r.abs();
    auto d      = r.options().dtype() == torch::kFloat16
                    ? torch::tensor(delta, r.options().dtype(torch::kFloat32)).to(r.device())
                    : torch::tensor(delta, r.options());
    auto quad   = 0.5 * r.pow(2);
    auto lin    = d * (abs_r - 0.5 * d);
    return torch::where(abs_r <= d, quad, lin).sum();
  }

  /** computes and return all terms (total, inv, var, cov). */
  VicRegTerms forward_terms(const at::Tensor& x_raw, const at::Tensor& y_raw) const {
    // Accept [N,E] or [*,N,E]-like; flatten batch dimensions to N
    auto x = x_raw.flatten(0, x_raw.dim() - 2);
    auto y = y_raw.flatten(0, y_raw.dim() - 2);

    const int64_t N = x.size(0);
    const int64_t E = x.size(1);
    TORCH_CHECK(N > 1, "[vicreg_loss] covariance needs at least 2 samples");

    const auto opts  = x.options();
    auto sim_coeff   = torch::tensor(sim_coeff_, opts);
    auto std_coeff   = torch::tensor(std_coeff_, opts);
    auto cov_coeff   = torch::tensor(cov_coeff_, opts);
    auto one         = torch::tensor(1.0,  opts);
    auto half        = torch::tensor(0.5,  opts);
    auto eps         = torch::tensor(1e-4, opts);
    auto denom       = torch::tensor(static_cast<double>(N - 1), opts);
    auto e_sz        = torch::tensor(static_cast<double>(E), opts);

    // Invariance (MSE) on raw
    auto inv = torch::nn::functional::smooth_l1_loss(
      x, y, torch::nn::functional::SmoothL1LossFuncOptions().beta(0.05));

    // Center per-dimension
    x = x - x.mean(0);
    y = y - y.mean(0);

    // Variance hinge
    auto std_x = torch::sqrt(x.var(0, /*unbiased=*/false) + eps);
    auto std_y = torch::sqrt(y.var(0, /*unbiased=*/false) + eps);
    auto var = half * (torch::mean(torch::relu(one - std_x)) +
                       torch::mean(torch::relu(one - std_y)));

    // Covariance on (optionally) whitened features → correlation-like off-diagonals
    at::Tensor x_for_cov = x, y_for_cov = y;
    if (whiten_before_cov_) {
      // clamp denom softly to avoid extreme amplification when std ≈ 0
      auto denom_x = torch::clamp(std_x, /*min=*/eps.item<double>());
      auto denom_y = torch::clamp(std_y, /*min=*/eps.item<double>());
      x_for_cov = x / (denom_x + eps);
      y_for_cov = y / (denom_y + eps);
    }

    auto cov_x = x_for_cov.t().matmul(x_for_cov) / denom;  // [E,E]
    auto cov_y = y_for_cov.t().matmul(y_for_cov) / denom;  // [E,E]

    auto off_x = off_diagonal(cov_x);
    auto off_y = off_diagonal(cov_y);

    // Robustify: Huber penalty on off-diagonals (sum), then average over E to match VICReg scale
    auto cov_pen =
      (huber_sum(off_x, huber_delta_) + huber_sum(off_y, huber_delta_)) / e_sz;

    // Built-in, config-free size safeguard for tiny effective N:
    // - 0 weight if N < 2E (too few samples for stable covariance)
    // - linear ramp to full weight by N >= (samples_per_dim_full_ * E)  [default 8E]
    double w_cov = 1.0;
    {
      const double n     = static_cast<double>(N);
      const double e     = static_cast<double>(E);
      const double n0    = 2.0 * e;
      const double n1    = static_cast<double>(samples_per_dim_full_) * e;
      if (n < n0)        w_cov = 0.0;
      else if (n < n1)   w_cov = (n - n0) / (n1 - n0);
      else               w_cov = 1.0;
    }

    auto total = sim_coeff * inv + std_coeff * var + (cov_coeff * w_cov) * cov_pen;
    return VicRegTerms{ total, inv, var, cov_pen };
  }

  /** Helper: fetch doubles for logging (call-site in templates: use `.template`). */
  static inline std::array<double,4> terms_as_scalar(const VicRegTerms& t) {
    return {
      t.total.item<double>(),
      t.inv  .item<double>(),
      t.var  .item<double>(),
      t.cov  .item<double>()
    };
  }

  /** Helper: formatted one-liner (no colors; safe anywhere). */
  static inline std::string terms_debug_string(const VicRegTerms& t) {
    auto arr = terms_as_scalar(t);
    std::ostringstream os;
    os << std::fixed << std::setprecision(6)
       << "[loss] total=" << arr[0]
       << " inv="        << arr[1]
       << " var="        << arr[2]
       << " cov="        << arr[3];
    return os.str();
  }
};

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
