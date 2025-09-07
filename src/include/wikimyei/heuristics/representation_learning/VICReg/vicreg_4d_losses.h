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
  TORCH_CHECK(S == m.size(1), "[vicreg_4d_losses.h](off_diagonal) off_diagonal expects a square matrix");
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
  at::Tensor cov;    // off-diagonal covariance penalty
};

struct VicRegLoss {
  double sim_coeff_;
  double std_coeff_;
  double cov_coeff_;
  bool   whiten_before_cov_ = true; 

  explicit VicRegLoss(double sim_coeff=25.0,
                      double std_coeff=25.0,
                      double cov_coeff=1.0,
                      bool   whiten_before_cov=true)
  : sim_coeff_(sim_coeff), std_coeff_(std_coeff), cov_coeff_(cov_coeff), whiten_before_cov_(whiten_before_cov) {}

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
          "cov_coeff")))
  {
    ASSERT(jk_component.loss_conf.type=="VICReg",
      "Review <training_components>.instruction file, VICREG_4D requires loss to be of type VICREG.\n");
  }

  /** returns only the total loss */
  at::Tensor operator()(const at::Tensor& x_raw, const at::Tensor& y_raw) const {
    return forward_terms(x_raw, y_raw).total;
  }

  /** computes and return all terms (total, inv, var, cov). */
  VicRegTerms forward_terms(const at::Tensor& x_raw, const at::Tensor& y_raw) const {
    auto x = x_raw.flatten(0, x_raw.dim() - 2);
    auto y = y_raw.flatten(0, y_raw.dim() - 2);

    const int64_t N = x.size(0);
    const int64_t E = x.size(1);
    TORCH_CHECK(N > 1, "[vicreg_loss] covariance needs at least 2 samples");

    const auto opts = x.options();
    auto sim_coeff = torch::tensor(sim_coeff_, opts);
    auto std_coeff = torch::tensor(std_coeff_, opts);
    auto cov_coeff = torch::tensor(cov_coeff_, opts);
    auto one   = torch::tensor(1.0,  opts);
    auto half  = torch::tensor(0.5,  opts);
    auto eps   = torch::tensor(1e-4, opts);
    auto denom = torch::tensor(static_cast<double>(N - 1), opts);
    auto e_sz  = torch::tensor(static_cast<double>(E), opts);

    // Invariance (MSE) on raw
    auto inv = torch::nn::functional::mse_loss(x, y);

    // Center
    x = x - x.mean(0);
    y = y - y.mean(0);

    // Variance hinge
    auto std_x = torch::sqrt(x.var(0, /*unbiased=*/false) + eps);
    auto std_y = torch::sqrt(y.var(0, /*unbiased=*/false) + eps);
    auto var = half * (torch::mean(torch::relu(one - std_x)) +
                       torch::mean(torch::relu(one - std_y)));

    // Covariance (optionally on whitened features)
    at::Tensor x_for_cov = x, y_for_cov = y;
    if (whiten_before_cov_) {
      x_for_cov = x / (std_x + eps);
      y_for_cov = y / (std_y + eps);
    }
    auto cov_x = x_for_cov.t().matmul(x_for_cov) / denom;
    auto cov_y = y_for_cov.t().matmul(y_for_cov) / denom;
    auto cov = (off_diagonal(cov_x).pow_(2).sum() +
                off_diagonal(cov_y).pow_(2).sum()) / e_sz;

    auto total = sim_coeff * inv + std_coeff * var + cov_coeff * cov;
    return VicRegTerms{ total, inv, var, cov };
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
