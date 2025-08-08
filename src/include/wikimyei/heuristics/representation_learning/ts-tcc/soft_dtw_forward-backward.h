/* soft_dtw.h */
#pragma once
#include <torch/torch.h>
#include <limits>
#include <algorithm>

RUNTIME_WARNING("(soft_dtw.h)[] determine why the alginment matrix of two equal sequences is not a diagonal.\n");

namespace cuwacunu {
namespace wikimyei {
namespace ts_tcc {

inline double clampExp(double x, double low = -50.f, double high = 50.f) {
    // Keep exponent in a safe range to avoid NaNs or inf
    if (x > high) x = high;
    if (x < low)  x = low;
    return std::exp(x);
}

inline torch::Tensor clampExp(const torch::Tensor& input, double low = -50.0, double high = 50.0) {
    // Clamp the tensor values between low and high
    auto clamped = torch::clamp(input, low, high);
    // Apply exponential function
    return torch::exp(clamped);
}

/**
 * 1) Forward pass: build DP matrix R in a vectorized manner.
 *
 *    - `D`: [B, N, M], the pairwise distance matrix.
 *    - `gamma`: softness parameter.
 *    - Returns `R`: [B, N+1, M+1], where:
 *         R[:, 0, :] and R[:, :, 0] = +∞ except R[:,0,0] = 0
 *       and R[:, i, j] = D[:,i-1,j-1] + soft_min( R[:,i-1,j-1],
 *                                                R[:,i-1,j],
 *                                                R[:,i,  j-1] ).
 */
inline torch::Tensor compute_softdtw_matrix_vectorized(
    const torch::Tensor& D,  // [B, N, M]
    double gamma
) {
    TORCH_CHECK(D.dim() == 3, "D must have shape [B, N, M].");
    auto B = D.size(0);
    auto N = D.size(1);
    auto M = D.size(2);

    // +∞ for boundaries; double precision to reduce numerical issues
    auto inf = std::numeric_limits<double>::infinity();
    auto R = torch::full({B, N + 2, M + 2}, inf,
                         D.options().dtype(torch::kFloat32));
    // R[:,0,0] = 0
    R.index_put_({"...", 0, 0}, 0.0);


    // We'll do a diagonal sweep over k = 2..(N+M).
    // For each diagonal k, valid (i,j) satisfy i+j == k, i in [1..N], j in [1..M].
    // That way we can update R for each diagonal in one advanced-indexing pass.
    for (int64_t k = 2; k <= N + M; k++) {
        int64_t i_start = std::max<int64_t>(1, k - M);
        int64_t i_end   = std::min<int64_t>(k - 1, N);
        if (i_start > i_end) {
            continue;
        }
        // Build i_range = [i_start..i_end]
        auto i_range = torch::arange(
            i_start, i_end + 1,
            torch::TensorOptions().dtype(torch::kLong).device(D.device())
        );
        // j_range = k - i_range
        auto j_range = (k - i_range).to(torch::kLong);

        // Gather the three neighbors: R[i-1,j-1], R[i-1,j], R[i,j-1]
        // We'll do -R[..]/gamma, then a log-sum-exp trick to get the soft-min.
        auto r_im1_jm1 = R.index({torch::indexing::Slice(),
                                  i_range - 1,
                                  j_range - 1})
                        .neg().div(gamma);  // shape [B, diag_len]
        auto r_im1_j   = R.index({torch::indexing::Slice(),
                                  i_range - 1,
                                  j_range})
                        .neg().div(gamma);
        auto r_i_jm1   = R.index({torch::indexing::Slice(),
                                  i_range,
                                  j_range - 1})
                        .neg().div(gamma);

        // Stack => shape [3, B, diag_len]
        auto stacked = torch::stack({r_im1_jm1, r_im1_j, r_i_jm1}, 0);
        // r_max => [1, B, diag_len]
        auto r_max = std::get<0>(stacked.max(/*dim=*/0, /*keepdim=*/true));
        // shifted => shape [3, B, diag_len]
        auto shifted = stacked - r_max;
        auto exp_sum = clampExp(shifted).sum(/*dim=*/0); // [B, diag_len]
        auto soft_min = (-gamma) * (exp_sum.log() + r_max.squeeze(0));

        // R_update = D[i-1,j-1] + soft_min
        auto d_ij = D.index({torch::indexing::Slice(),
                             i_range - 1,
                             j_range - 1})
                    .to(torch::kFloat32);
        auto R_update = d_ij + soft_min;  // [B, diag_len]

        // Write back: R[i,j] = R_update
        R.index_put_({torch::indexing::Slice(),
                      i_range,
                      j_range},
                     R_update);
    }

    return R;
}

/**
 * 2) Extract a "soft alignment" matrix from the already-computed R.
 *
 *    This replicates the typical Soft-DTW alignment logic:
 *      - We define an auxiliary E = zeros_like(R), E[:, N, M] = 1
 *      - Then do a reverse diagonal sweep from (N,M) -> (1,1),
 *        accumulating E[i,j] += w * E[i_up,j_up] for each neighbor.
 *      - Finally alignment = exp(-R / gamma) * E (and typically we slice out [B,N,M]).
 *      - We often normalize row-wise (or globally). Here we do row-wise for each i.
 *
 *    Returns shape [B, N, M].
 *
 *    Note: Because everything is in Tensors, autograd can track the ops
 *          and backprop to the input distances if needed.
 */
inline torch::Tensor extract_soft_alignment_vectorized(
    torch::Tensor R,   // [B, N+2, M+2], from compute_softdtw_matrix_vectorized
    const double gamma
) {
    TORCH_CHECK(R.dim() == 3, "R must have shape [B, N+2, M+2].");

    auto N = R.size(1) - 2;  // R has dimension N+2 in 2nd axis
    auto M = R.size(2) - 2;  // R has dimension M+2 in 3rd axis

    // Work in double precision for convenience
    R = R.to(torch::kFloat32);

    // Initialize E and set base case
    auto E = torch::zeros_like(R);
    E.index_put_({torch::indexing::Slice(), N, M}, 1.0);

    // Boundary modifications for R
    // R.index_put_({torch::indexing::Slice(), torch::indexing::Slice(), M}, -std::numeric_limits<float>::infinity());
    // R.index_put_({torch::indexing::Slice(), N, torch::indexing::Slice()}, -std::numeric_limits<float>::infinity());
    // R.index_put_({torch::indexing::Slice(), N, M}, R.index({torch::indexing::Slice(), N - 1, M - 1}));

    R.index_put_({torch::indexing::Slice(), torch::indexing::Slice(), M+1}, -std::numeric_limits<double>::infinity());
    R.index_put_({torch::indexing::Slice(), N+1, torch::indexing::Slice()}, -std::numeric_limits<double>::infinity());
    R.index_put_({torch::indexing::Slice(), N+1, M+1}, R.index({torch::indexing::Slice(), N, M}));

    // Reverse diagonal sweep: k in [N+M, ..., 2]
    for (int64_t k = N + M; k >= 2; k--) {
        int64_t i_start = std::max<int64_t>(1, k - M);
        int64_t i_end   = std::min<int64_t>(k - 1, N);
        if (i_start > i_end) {
            continue;
        }
        auto i_range = torch::arange(i_start, i_end + 1,
                                     torch::TensorOptions().dtype(torch::kLong).device(R.device()));
        auto j_range = (k - i_range).to(torch::kLong);

        // Neighbor (i+1, j)
        {
            auto i_up = i_range + 1;
            auto j_up = j_range;
            auto mask = (i_up <= N);
            if (mask.any().item<bool>()) {
                auto i_cur_masked = i_range.index({mask});
                auto j_cur_masked = j_range.index({mask});
                auto i_up_masked = i_up.index({mask});
                auto j_up_masked = j_up.index({mask});

                auto r_cur_masked = R.index({torch::indexing::Slice(), i_cur_masked, j_cur_masked});
                auto r_up = R.index({torch::indexing::Slice(), i_up_masked, j_up_masked});
                auto e_up = E.index({torch::indexing::Slice(), i_up_masked, j_up_masked});

                auto tmp = (r_up - r_cur_masked) / gamma;
                // Replace NaNs in tmp with 0
                tmp = torch::where(torch::isnan(tmp), torch::zeros_like(tmp), tmp);
                auto w = clampExp(-tmp);

                auto current_e = E.index({torch::indexing::Slice(), i_cur_masked, j_cur_masked});
                current_e += w * e_up;
                E.index_put_({torch::indexing::Slice(), i_cur_masked, j_cur_masked}, current_e);
            }
        }

        // Neighbor (i, j+1)
        {
            auto i_up = i_range;
            auto j_up = j_range + 1;
            auto mask = (j_up <= M);
            if (mask.any().item<bool>()) {
                auto i_cur_masked = i_range.index({mask});
                auto j_cur_masked = j_range.index({mask});
                auto i_up_masked = i_up.index({mask});
                auto j_up_masked = j_up.index({mask});

                auto r_cur_masked = R.index({torch::indexing::Slice(), i_cur_masked, j_cur_masked});
                auto r_up = R.index({torch::indexing::Slice(), i_up_masked, j_up_masked});
                auto e_up = E.index({torch::indexing::Slice(), i_up_masked, j_up_masked});

                auto tmp = (r_up - r_cur_masked) / gamma;
                // Replace NaNs in tmp with 0
                tmp = torch::where(torch::isnan(tmp), torch::zeros_like(tmp), tmp);
                auto w = clampExp(-tmp);

                auto current_e = E.index({torch::indexing::Slice(), i_cur_masked, j_cur_masked});
                current_e += w * e_up;
                E.index_put_({torch::indexing::Slice(), i_cur_masked, j_cur_masked}, current_e);
            }
        }

        // Neighbor (i+1, j+1)
        {
            auto i_up = i_range + 1;
            auto j_up = j_range + 1;
            auto mask = ((i_up <= N) & (j_up <= M));
            if (mask.any().item<bool>()) {
                auto i_cur_masked = i_range.index({mask});
                auto j_cur_masked = j_range.index({mask});
                auto i_up_masked = i_up.index({mask});
                auto j_up_masked = j_up.index({mask});

                auto r_cur_masked = R.index({torch::indexing::Slice(), i_cur_masked, j_cur_masked});
                auto r_up = R.index({torch::indexing::Slice(), i_up_masked, j_up_masked});
                auto e_up = E.index({torch::indexing::Slice(), i_up_masked, j_up_masked});

                auto tmp = (r_up - r_cur_masked) / gamma;
                // Replace NaNs in tmp with 0
                tmp = torch::where(torch::isnan(tmp), torch::zeros_like(tmp), tmp);
                auto w = clampExp(-tmp);

                auto current_e = E.index({torch::indexing::Slice(), i_cur_masked, j_cur_masked});
                current_e += w * e_up;
                E.index_put_({torch::indexing::Slice(), i_cur_masked, j_cur_masked}, current_e);
            }
        }
    }

    // Compute the final alignment normalization
    auto exponented = clampExp(-R / gamma);
    auto alignment_full = exponented * E;  // [B, N+1, M+1]

    // Extract submatrix and normalize row-wise
    auto alignment_sub = alignment_full.index({
        torch::indexing::Slice(),
        torch::indexing::Slice(1, N + 1),
        torch::indexing::Slice(1, M + 1)
    });  // shape [B, N, M]

    // auto alignment_normed = alignment_sub;
    
    // auto row_sum = alignment_sub.sum(/*dim=*/2, /*keepdim=*/true) + 1e-9;
    // auto alignment_normed = alignment_sub / row_sum;


    auto global_sum = alignment_sub.sum({2, 1}, /*keepdim=*/true) + 1e-9;
    auto alignment_normed = alignment_sub / global_sum;

    return alignment_normed.to(R.dtype());
}


/**
 * 3) High-level function: compute both Soft-DTW cost and the soft alignment.
 *
 *    - If x,y have shape [N,D], we unsqueeze a batch dim.
 *    - We compute D = pairwise distances => [B, N, M].
 *    - We compute R = DP matrix => [B, N+2, M+2].
 *    - Cost = R[:, N, M] => shape [B].
 *    - alignment = extract_soft_alignment_vectorized(R, gamma).
 *
 * Returns a tuple: ( cost: [B] , alignment: [B, N, M], R_: [B, N, M] )
 */
inline std::tuple<torch::Tensor, torch::Tensor, torch::Tensor>
softdtw_alignment(
    const torch::Tensor& x, // [B, N, D] or [N, D]
    const torch::Tensor& y, // [B, M, D] or [M, D]
    double gamma
) {
    x.requires_grad_(true);
    y.requires_grad_(true);

    // 1) Possibly unsqueeze batch
    bool squeezed = false;
    auto x_ = x;
    auto y_ = y;
    if (x.dim() == 2) {
        x_ = x.unsqueeze(0);  // => [1, N, D]
        y_ = y.unsqueeze(0);  // => [1, M, D]
        squeezed = true;
    }
    auto B = x_.size(0);
    auto N = x_.size(1);
    auto M = y_.size(1);

    // 2) Compute distance matrix D => [B,N,M]
    auto x_expanded = x_.unsqueeze(2).expand({B, N, M, x_.size(2)});
    auto y_expanded = y_.unsqueeze(1).expand({B, N, M, y_.size(2)});
    auto dist = (x_expanded - y_expanded).pow(2).sum(/*dim=*/3);

    // 3) Forward DP => R: [B, N+2, M+2]
    auto R = compute_softdtw_matrix_vectorized(dist, gamma);

    // 4) Cost => R[:, N, M] => shape [B]
    auto cost = R.index({torch::indexing::Slice(), N, M});

    // 5) Soft alignment => shape [B, N, M]
    auto alignment = extract_soft_alignment_vectorized(R, gamma);

    // 6) If we had a squeezed input, cost => [1], alignment => [1,N,M].
    //    Return scalar cost and [N,M] alignment for consistency.
    torch::Tensor final_cost, final_align;
    if (squeezed) {
        final_cost = cost.squeeze(0);        // scalar
        final_align = alignment.squeeze(0);  // [N,M]
    } else {
        final_cost = cost;
        final_align = alignment;
    }

    return std::make_tuple(final_cost, final_align, R);
}

} // namespace ts_tcc
} // namespace wikimyei
} // namespace cuwacunu
