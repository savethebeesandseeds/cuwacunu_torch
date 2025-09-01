/* vicreg_4d_losses.h */
#pragma once
#include <torch/torch.h>
#include "camahjucunu/BNF/implementations/training_components/training_components.h"
#include "jkimyei/training_components/jk_setup.h"

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

struct VicRegLoss {
  double sim_coeff_;
  double std_coeff_;
  double cov_coeff_;
  // direct coefficients constructor
  explicit VicRegLoss(double sim_coeff=25.0,
                      double std_coeff=25.0,
                      double cov_coeff=1.0)
  : sim_coeff_(sim_coeff), std_coeff_(std_coeff), cov_coeff_(cov_coeff) {}

  // constructor from jk_setup
  explicit VicRegLoss(const cuwacunu::jkimyei::jk_setup_t& jk_setup)
  : VicRegLoss(
      cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_option(
          jk_setup.inst.retrive_row("loss_functions_table", jk_setup.loss_conf.id),
          "sim_coeff")),
      cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_option(
          jk_setup.inst.retrive_row("loss_functions_table", jk_setup.loss_conf.id),
          "std_coeff")),
      cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_option(
          jk_setup.inst.retrive_row("loss_functions_table", jk_setup.loss_conf.id),
          "cov_coeff")))
  {
    ASSERT(jk_setup.loss_conf.type=="VICReg", 
      "Review <training_components>.instruction file, VICREG_4D requires loss to be of type VICREG.\n");
  }

  at::Tensor operator()(const at::Tensor& x_raw, const at::Tensor& y_raw) const {
    /* ------------------------------------------------------------------ */
    /* 0. Flatten every leading dim so we always have [B, E]              */
    /* ------------------------------------------------------------------ */
    auto x = x_raw.flatten(0, x_raw.dim() - 2);  // [B, E]
    auto y = y_raw.flatten(0, y_raw.dim() - 2);

    const int64_t B = x.size(0);
    const int64_t E = x.size(1);
    TORCH_CHECK(B > 1, "[vicreg_4d_losses.h](vicreg_loss) VICReg covariance needs at least 2 samples");

    const auto opts = x.options();           // dtype+device

    /* ------------------------------------------------------------------ */
    /* 1. Literal constants as device‑aware tensors                       */
    /* ------------------------------------------------------------------ */
    auto sim_coeff = torch::tensor(sim_coeff_, opts);
    auto std_coeff = torch::tensor(std_coeff_, opts);
    auto cov_coeff = torch::tensor(cov_coeff_, opts);

    auto one   = torch::tensor(1.0,  opts);
    auto half  = torch::tensor(0.5,  opts);
    auto eps   = torch::tensor(1e-4, opts);
    auto denom = torch::tensor(static_cast<double>(B - 1), opts);
    auto e_sz  = torch::tensor(static_cast<double>(E),   opts);

    /* ------------------------------------------------------------------ */
    /* 2. Invariance term (MSE)                                           */
    /* ------------------------------------------------------------------ */
    auto repr_loss = torch::nn::functional::mse_loss(x, y);

    /* ------------------------------------------------------------------ */
    /* 3. Variance term                                                   */
    /* ------------------------------------------------------------------ */
    x = x - x.mean(0);
    y = y - y.mean(0);

    auto std_x = torch::sqrt(x.var(0, /*unbiased=*/false) + eps);
    auto std_y = torch::sqrt(y.var(0, /*unbiased=*/false) + eps);

    auto std_loss = half * (torch::mean(torch::relu(one - std_x)) +
                torch::mean(torch::relu(one - std_y)));

    /* ------------------------------------------------------------------ */
    /* 4. Covariance term                                                 */
    /* ------------------------------------------------------------------ */
    auto cov_x = x.t().matmul(x) / denom;     // [E, E]
    auto cov_y = y.t().matmul(y) / denom;

    auto cov_loss = (off_diagonal(cov_x).pow_(2).sum() +
            off_diagonal(cov_y).pow_(2).sum()) / e_sz;

    /* ------------------------------------------------------------------ */
    /* 5. Total loss (all CUDA tensors)                                   */
    /* ------------------------------------------------------------------ */
    auto total = 
      sim_coeff * repr_loss +
      std_coeff * std_loss  +
      cov_coeff * cov_loss;

    return total;
  }
};

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
