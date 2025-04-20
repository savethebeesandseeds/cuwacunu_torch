/* vicreg_4d_losses.h */
#pragma once
#include <torch/torch.h>
#include "piaabo/dutils.h"

RUNTIME_WARNING("(vicreg_4d_losses.h)[] it would be beneficial to add soft_dtw loss to vicreg loss. \n");

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

torch::Tensor off_diagonal(const torch::Tensor& x) {
    const auto n = x.size(0);
    TORCH_CHECK(x.size(0) == x.size(1), "off_diagonal expects a square matrix");

    auto flattened = x.flatten().slice(0, 0, n * n - 1);
    auto reshaped = flattened.view({n - 1, n + 1});
    return reshaped.slice(1, 1, n + 1).flatten();
}

/**
 * @brief Computes the VICReg (Variance-Invariance-Covariance Regularization) loss.
 * 
 * This function implements the VICReg self-supervised learning objective which balances
 * three components:
 * 
 * 1. **Invariance Loss**: Mean squared error between representations x and y, encouraging
 *    the model to produce similar outputs for different augmented views of the same input.
 * 
 * 2. **Variance Loss**: Encourages diversity in the batch dimension by penalizing low 
 *    standard deviation (below 1) across each feature dimension to prevent representation collapse.
 * 
 * 3. **Covariance Loss**: Penalizes off-diagonal entries in the covariance matrix of features
 *    to decorrelate feature dimensions and encourage information disentanglement.
 * 
 * @param x_raw        First batch of feature embeddings (typically one view).
 * @param y_raw        Second batch of feature embeddings (augmented view).
 * @param sim_coeff    Weight for the invariance (MSE) loss component.
 * @param std_coeff    Weight for the variance regularization component.
 * @param cov_coeff    Weight for the covariance regularization component.
 * 
 * @return torch::Tensor   Scalar tensor representing the total VICReg loss.
 */

torch::Tensor vicreg_loss(
    const torch::Tensor& x_raw,
    const torch::Tensor& y_raw,
    double sim_coeff = 25.0,
    double std_coeff = 25.0,
    double cov_coeff = 1.0
) {
    auto x = x_raw.clone();
    auto y = y_raw.clone();

    // Invariance loss (mean squared error)
    auto repr_loss = torch::nn::functional::mse_loss(x, y);

    // Normalize (remove mean)
    x = x - x.mean(0);
    y = y - y.mean(0);

    // Variance loss
    auto std_x = torch::sqrt(x.var(0, /*unbiased=*/false) + 1e-4);
    auto std_y = torch::sqrt(y.var(0, /*unbiased=*/false) + 1e-4);
    auto std_loss = 0.5 * (torch::mean(torch::relu(1 - std_x)) +
                            torch::mean(torch::relu(1 - std_y)));

    // Covariance loss
    const auto batch_size = x.size(0);
    auto cov_x = (x.transpose(0, 1).matmul(x)) / (batch_size - 1);
    auto cov_y = (y.transpose(0, 1).matmul(y)) / (batch_size - 1);
    auto cov_loss = (off_diagonal(cov_x).pow(2).sum() +
                        off_diagonal(cov_y).pow(2).sum()) / x.size(1);

    // Total loss
    auto loss = sim_coeff * repr_loss + std_coeff * std_loss + cov_coeff * cov_loss;
    return loss;
}

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
