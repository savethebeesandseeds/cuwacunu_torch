/* soft_dtw_backup.h */

/*
 * This header provides implementations of Soft-Dynamic Time Warping (Soft-DTW) 
 * and its variants for alignment and distance computation between time series.
 *
 * The file integrates:
 * - Forward and backward (autograd) computations for Soft-DTW.
 * - Soft alignment extraction for detailed path information.
 * - Enhanced numeric stability using clamped exponentials.
 *
 * There are **two main approaches** to use the Soft-DTW functionality:
 *
 * 1. **High-Level Standalone Functions**:
 *    - `compute_alignment_matrix_softdtw`: Computes the alignment matrix between two embeddings.
 *      * Suitable for quick, stateless computations.
 *      * Provides a simpler API for standalone use without creating any additional objects.
 *      * Does not directly integrate with PyTorch's autograd for gradient backpropagation.
 *    
 *    Example:
 *      ```
 *      auto alignment_matrix = compute_alignment_matrix_softdtw(emb_a, emb_b, gamma);
 *      ```
 *
 * 2. **Object-Oriented Module (`SoftDTWImpl`)**:
 *    - `SoftDTWImpl::alignment`: Computes the alignment matrix with options for normalization 
 *       and direct integration into training pipelines.
 *      * Designed for repeated use and stateful operations.
 *      * Integrates seamlessly with PyTorch's autograd for gradient computation.
 *      * Offers additional flexibility, such as configurable normalization of results.
 *    
 *    Example:
 *      ```
 *      SoftDTW soft_dtw_module(gamma, normalize);
 *      auto alignment_matrix = soft_dtw_module->alignment(emb_a, emb_b);
 *      ```
 *
 * **Shapes and Notation**:
 * - Input embeddings: [B, N, D] (batch size B, sequence length N, feature dimension D).
 * - Pairwise distance matrix: [B, N, M] (N and M are sequence lengths for two inputs).
 * - Alignment matrix: [B, N, M].
 *
 * Use the high-level functions for simplicity or the module-based approach for advanced
 * use cases where gradient support or repeated operations are required.
 *
 */

#pragma once
#include <torch/torch.h>
#include <vector>
#include <limits>
#include <stdexcept>
#include <iostream>
#include <cmath>

namespace cuwacunu {
namespace wikimyei {
namespace ts_tcc {

/*
  This header merges:
    - Soft-DTW forward/backward (autograd) from the newer code
    - Soft alignment extraction from the older code
    - Additional numeric stability (clamping exponentials)
    - A high-level "compute_alignment_matrix_softdtw" for a single call.

  Shapes & Notation:
    - D: [B, N, M]  (pairwise distances)
    - R: [B, N+2, M+2] (DP table)
    - alignment: [B, N, M]
*/

// -----------------------------------------------------------------------------
// 1) Utility: clampExp for numerical stability
// -----------------------------------------------------------------------------
inline double clampExp(double x, double low = -50.f, double high = 50.f) {
    // Keep exponent in a safe range to avoid NaNs or inf
    if (x > high) x = high;
    if (x < low)  x = low;
    return std::exp(x);
}

// -----------------------------------------------------------------------------
// 2) Forward pass: compute_softdtw
// -----------------------------------------------------------------------------
/*
  D: (B, N, M)
  gamma: softness
  returns R: (B, N+2, M+2), with boundary = +∞ except R[:,0,0]=0
  final cost is R[:, N, M].
*/
inline torch::Tensor compute_softdtw(const torch::Tensor& D, double gamma) {
    TORCH_CHECK(D.dim() == 3, "D must be [B, N, M]");

    int64_t B = D.size(0);
    int64_t N = D.size(1);
    int64_t M = D.size(2);

    // Create R with +∞
    auto R = torch::full({B, N + 2, M + 2},
                         std::numeric_limits<double>::infinity(),
                         D.options().dtype(torch::kFloat32));


    // R[:, 0, 0] = 0
    R.index_put_({"...", 0, 0}, 0.0);

    // ***** CRITICAL FIX: i-first, j-second *****
    // Fill DP with i in outer loop, j in inner loop.
    for (int64_t b = 0; b < B; b++) {
        for (int64_t i = 1; i <= N; i++) {
            for (int64_t j = 1; j <= M; j++) {
                double r0 = - R.index({b, i - 1, j - 1}).item<double>() / gamma;
                double r1 = - R.index({b, i - 1, j}).item<double>() / gamma;
                double r2 = - R.index({b, i, j - 1}).item<double>() / gamma;

                // Numerically stable: find max, exponentiate diffs
                double rmax = std::max({r0, r1, r2});
                double exp0 = clampExp(r0 - rmax);
                double exp1 = clampExp(r1 - rmax);
                double exp2 = clampExp(r2 - rmax);

                double rsum = exp0 + exp1 + exp2;
                double softmin = - gamma * (std::log(rsum) + rmax);

                double cost_ij = D.index({b, i - 1, j - 1}).item<double>();
                R.index_put_({b, i, j}, cost_ij + softmin);
            }
        }
    }

    return R;
}

// -----------------------------------------------------------------------------
// 3) Backward pass: compute_softdtw_backward
// -----------------------------------------------------------------------------
/*
  D_: [B, N, M]   original distances
  R:  [B, N+2, M+2] from forward
  gamma: softness
  returns gradient E wrt D_ => shape [B, N, M]
*/
inline torch::Tensor compute_softdtw_backward(
    const torch::Tensor& D_,
    const torch::Tensor& R,
    double gamma
) {
    TORCH_CHECK(D_.dim() == 3, "D_ must be [B, N, M]");
    TORCH_CHECK(R.dim() == 3, "R must be [B, N+2, M+2]");

    int64_t B = D_.size(0);
    int64_t N = D_.size(1);
    int64_t M = D_.size(2);

    // Embed D_ into shape [B, N+2, M+2]
    auto D = torch::zeros({B, N + 2, M + 2}, D_.options());
    D.index_put_({torch::indexing::Slice(),
                  torch::indexing::Slice(1, N + 1),
                  torch::indexing::Slice(1, M + 1)},
                 D_);

    // Initialize E for backward pass
    auto E = torch::zeros({B, N + 2, M + 2}, D_.options());
    // Boundary: E[:, N+1, M+1] = 1
    E.index_put_({torch::indexing::Slice(), N + 1, M + 1}, 1.0);

    // Copy R so we can safely modify boundary
    auto Rcopy = R.clone();

    // R[:, :, -1] = -∞, R[:, -1, :] = -∞
    // R[:, -1, -1] = R[:, -2, -2]
    for (int64_t b = 0; b < B; b++) {
        for (int64_t i = 0; i < N + 2; i++) {
            Rcopy.index_put_({b, i, M + 1}, -std::numeric_limits<double>::infinity());
        }
        for (int64_t j = 0; j < M + 2; j++) {
            Rcopy.index_put_({b, N + 1, j}, -std::numeric_limits<double>::infinity());
        }
        // Rcopy[b][N+1][M+1] = Rcopy[b][N][M]
        Rcopy.index_put_({b, N + 1, M + 1}, Rcopy.index({b, N, M}));
    }

    // Back-propagation
    for (int64_t b = 0; b < B; b++) {
        for (int64_t j = M; j >= 1; j--) {
            for (int64_t i = N; i >= 1; i--) {
                double r_cur = Rcopy.index({b, i, j}).item<double>();
                double d_cur = D.index({b, i, j}).item<double>(); // Original cost

                // From (i+1, j)
                {
                    double tmp = (Rcopy.index({b, i + 1, j}).item<double>() - r_cur - d_cur) / gamma;
                    double w = clampExp(tmp);
                    E.index_put_({b, i, j}, E.index({b, i, j}).item<double>() + w * E.index({b, i + 1, j}).item<double>());
                }
                // From (i, j+1)
                {
                    double tmp = (Rcopy.index({b, i, j + 1}).item<double>() - r_cur - d_cur) / gamma;
                    double w = clampExp(tmp);
                    E.index_put_({b, i, j}, E.index({b, i, j}).item<double>() + w * E.index({b, i, j + 1}).item<double>());
                }
                // From (i+1, j+1)
                {
                    double tmp = (Rcopy.index({b, i + 1, j + 1}).item<double>() - r_cur - d_cur) / gamma;
                    double w = clampExp(tmp);
                    E.index_put_({b, i, j}, E.index({b, i, j}).item<double>() + w * E.index({b, i + 1, j + 1}).item<double>());
                }
            }
        }
    }

    // Return [B, N, M]
    return E.index({torch::indexing::Slice(),
                    torch::indexing::Slice(1, N + 1),
                    torch::indexing::Slice(1, M + 1)});
}

// -----------------------------------------------------------------------------
// 4) "Soft Alignment" Extraction
// -----------------------------------------------------------------------------
inline torch::Tensor extract_soft_alignment(const torch::Tensor& R_in,
                                            const torch::Tensor& D_in,
                                            double gamma) {
    TORCH_CHECK(R_in.dim() == 3, "R must be [B, N+2, M+2]");
    TORCH_CHECK(D_in.dim() == 3, "D must be [B, N, M]");

    int64_t B = R_in.size(0);
    int64_t N = D_in.size(1);
    int64_t M = D_in.size(2);

    // Copy R to safely modify boundary
    auto R = R_in.clone();
    auto E = torch::zeros_like(R);

    // Shape of final alignment: [B, N, M]
    // Boundary for alignment path
    E.index_put_({torch::indexing::Slice(), N, M}, 1.0);

    for (int64_t b = 0; b < B; b++) {
        for (int64_t i = 0; i < N + 2; i++) {
            R.index_put_({b, i, M + 1}, -std::numeric_limits<double>::infinity());
        }
        for (int64_t j = 0; j < M + 2; j++) {
            R.index_put_({b, N + 1, j}, -std::numeric_limits<double>::infinity());
        }
        R.index_put_({b, N + 1, M + 1}, R.index({b, N, M}));
    }

    // Back-propagation
    for (int64_t b = 0; b < B; b++) {
        for (int64_t j = M; j >= 1; j--) {
            for (int64_t i = N; i >= 1; i--) {
                double r_cur = R.index({b, i, j}).item<double>();

                // From (i+1, j)
                {
                    double tmp = (R.index({b, i + 1, j}).item<double>() - r_cur) / gamma;
                    double w = clampExp(-tmp);
                    E.index_put_({b, i, j}, E.index({b, i, j}).item<double>() + w * E.index({b, i + 1, j}).item<double>());
                }
                // From (i, j+1)
                {
                    double tmp = (R.index({b, i, j + 1}).item<double>() - r_cur) / gamma;
                    double w = clampExp(-tmp);
                    E.index_put_({b, i, j}, E.index({b, i, j}).item<double>() + w * E.index({b, i, j + 1}).item<double>());
                }
                // From (i+1, j+1)
                {
                    double tmp = (R.index({b, i + 1, j + 1}).item<double>() - r_cur) / gamma;
                    double w = clampExp(-tmp);
                    E.index_put_({b, i, j}, E.index({b, i, j}).item<double>() + w * E.index({b, i + 1, j + 1}).item<double>());
                }
            }
        }
    }

    // Build forward factor
    auto exponented = -(R / gamma);
    exponented = exponented.clamp(-50.f, 50.f).exp();
    auto F = exponented;

    auto align_sub = F.index({torch::indexing::Slice(),
                              torch::indexing::Slice(1, N + 1),
                              torch::indexing::Slice(1, M + 1)})
                    * E.index({torch::indexing::Slice(),
                               torch::indexing::Slice(1, N + 1),
                               torch::indexing::Slice(1, M + 1)});

    auto row_sum = align_sub.sum(2, true) + 1e-9;
    auto align_normed = align_sub / row_sum;

    return align_normed;
}

// -----------------------------------------------------------------------------
// 5) Autograd wrapper: SoftDTWFunction
// -----------------------------------------------------------------------------
struct SoftDTWFunction : public torch::autograd::Function<SoftDTWFunction> {
    static torch::Tensor forward(
        torch::autograd::AutogradContext* ctx,
        const torch::Tensor& D,
        double gamma
    ) {
        auto R = compute_softdtw(D, static_cast<double>(gamma));
        ctx->save_for_backward({D, R});
        ctx->saved_data["gamma"] = gamma;

        // int64_t B = D.size(0);
        int64_t N = D.size(1);
        int64_t M = D.size(2);

        auto cost = R.index({
            torch::indexing::Slice(),
            torch::indexing::Slice(N, N+1),
            torch::indexing::Slice(M, M+1)}).squeeze(-1).squeeze(-1);
        return cost;
    }

    static torch::autograd::variable_list backward(
        torch::autograd::AutogradContext* ctx,
        torch::autograd::variable_list grad_output
    ) {
        auto saved = ctx->get_saved_variables();
        auto D = saved[0];
        auto R = saved[1];
        double gamma = ctx->saved_data["gamma"].toDouble();

        auto g_out = grad_output[0].view({-1, 1, 1});
        auto E = compute_softdtw_backward(D, R, static_cast<double>(gamma));
        auto grad_D = g_out * E;
        return {grad_D, torch::Tensor()};
    }
};

// Handy helper
inline torch::Tensor softdtw_autograd(const torch::Tensor& D, double gamma) {
    return SoftDTWFunction::apply(D, gamma);
}

// -----------------------------------------------------------------------------
// 6) SoftDTW module
// -----------------------------------------------------------------------------
struct SoftDTWImpl : public torch::nn::Module {
    double gamma;
    bool normalize;

    SoftDTWImpl(double gamma_ = 1.0, bool normalize_ = false)
        : gamma(gamma_), normalize(normalize_) {}

    torch::Tensor calc_distance_matrix(const torch::Tensor& x,
                                       const torch::Tensor& y) {
        TORCH_CHECK(x.dim() == 3 && y.dim() == 3,
                    "x, y must be [B, N, D] and [B, M, D]");
        auto B = x.size(0);
        auto N = x.size(1);
        auto M = y.size(1);

        auto x_expanded = x.unsqueeze(2).expand({B, N, M, x.size(2)});
        auto y_expanded = y.unsqueeze(1).expand({B, N, M, y.size(2)});
        auto dist = (x_expanded - y_expanded).pow(2).sum(/*dim=*/3);
        return dist;
    }

    torch::Tensor forward(const torch::Tensor& x, const torch::Tensor& y) {
        bool squeeze = false;
        auto x_ = x;
        auto y_ = y;
        if (x.dim() < 3) {
            x_ = x.unsqueeze(0);
            y_ = y.unsqueeze(0);
            squeeze = true;
        }

        auto D_xy = calc_distance_matrix(x_, y_);
        auto out_xy = softdtw_autograd(D_xy, gamma);

        if (normalize) {
            auto D_xx = calc_distance_matrix(x_, x_);
            auto out_xx = softdtw_autograd(D_xx, gamma);

            auto D_yy = calc_distance_matrix(y_, y_);
            auto out_yy = softdtw_autograd(D_yy, gamma);

            auto result = out_xy - 0.5 * (out_xx + out_yy);
            return squeeze ? result.squeeze(0) : result;
        } else {
            return squeeze ? out_xy.squeeze(0) : out_xy;
        }
    }

    torch::Tensor alignment(const torch::Tensor& x, const torch::Tensor& y) {
        bool squeeze = false;
        auto x_ = x;
        auto y_ = y;
        if (x.dim() < 3) {
            x_ = x.unsqueeze(0);
            y_ = y.unsqueeze(0);
            squeeze = true;
        }

        auto D_xy = calc_distance_matrix(x_, y_);
        auto R = compute_softdtw(D_xy, (double)gamma);
        auto ali = extract_soft_alignment(R, D_xy, (double)gamma);
        return squeeze ? ali.squeeze(0) : ali;
    }
};

TORCH_MODULE(SoftDTW);

// -----------------------------------------------------------------------------
// 7) High-level "compute_alignment_matrix_softdtw"
// -----------------------------------------------------------------------------
inline torch::Tensor compute_alignment_matrix_softdtw(
    const torch::Tensor& x,
    const torch::Tensor& y,
    double gamma = 1.0
) {
    bool squeeze = false;
    auto x_ = x;
    auto y_ = y;
    if (x.dim() < 3) {
        x_ = x.unsqueeze(0);
        y_ = y.unsqueeze(0);
        squeeze = true;
    }

    auto B = x_.size(0);
    auto N = x_.size(1);
    auto M = y_.size(1);

    auto x_expanded = x_.unsqueeze(2).expand({B, N, M, x_.size(2)});
    auto y_expanded = y_.unsqueeze(1).expand({B, N, M, y_.size(2)});
    auto dist = (x_expanded - y_expanded).pow(2).sum(-1);

    auto R = compute_softdtw(dist, (double)gamma);
    auto alignment = extract_soft_alignment(R, dist, (double)gamma);

    return squeeze ? alignment.squeeze(0) : alignment;
}

// -----------------------------------------------------------------------------
// 8) High-level "compute_softdtw_cost"
// -----------------------------------------------------------------------------
/**
 * @brief Compute the single Soft-DTW cost for each batch item, using the same DP logic
 *        as compute_softdtw().
 *
 * This is analogous to how Python code does R[:, -2, -2].
 *
 * @param x  [B, N, D] or [N, D] (unsqueezed if needed)
 * @param y  [B, M, D] or [M, D] (unsqueezed if needed)
 * @param gamma  The Soft-DTW gamma
 * @return A 1D tensor of shape [B], each entry is the final Soft-DTW cost.
 *         (If both x,y were squeezed inputs, it returns a single-scalar shape.)
 */
inline torch::Tensor compute_softdtw_cost(
    const torch::Tensor& x, 
    const torch::Tensor& y, 
    double gamma = 1.0
) {
    // If x,y have shape [N,D], we unsqueeze a batch dim
    bool squeeze = false;
    auto x_ = x;
    auto y_ = y;
    if (x.dim() < 3) {
        x_ = x.unsqueeze(0); // => [1, N, D]
        y_ = y.unsqueeze(0); // => [1, M, D]
        squeeze = true;
    }

    // Build pairwise distance matrix
    auto B = x_.size(0);
    auto N = x_.size(1);
    auto M = y_.size(1);

    // shape => [B, N, M]
    auto x_expanded = x_.unsqueeze(2).expand({B, N, M, x_.size(2)});
    auto y_expanded = y_.unsqueeze(1).expand({B, N, M, y_.size(2)});
    auto dist = (x_expanded - y_expanded).pow(2).sum(-1);

    // compute R via DP
    auto R = compute_softdtw(dist, static_cast<double>(gamma));
    // final cost => R[:, N, M] => 0-based => R[:, -2, -2]
    // We'll slice to shape [B,1,1], then squeeze
    auto cost = R.index({
        torch::indexing::Slice(),               // all batches
        torch::indexing::Slice(N, N + 1),       // N is -2 in 0-based
        torch::indexing::Slice(M, M + 1)        // M is -2 in 0-based
    }).squeeze(-1).squeeze(-1); // shape => [B]

    // If we had squeezed input, shape => [B], we can optionally do .squeeze(0) => scalar
    return squeeze ? cost.squeeze(0) : cost;
}


} // namespace ts_tcc
} // namespace wikimyei
} // namespace cuwacunu
