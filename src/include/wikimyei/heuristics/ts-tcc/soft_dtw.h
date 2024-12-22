#pragma once
#include <torch/torch.h>
#include <vector>
#include <limits>
#include <stdexcept>
#include <iostream>

namespace cuwacunu {
namespace wikimyei {
namespace ts_tcc {

/* A helper function to compute the log-sum-exp of three values in a numerically stable way. */
inline torch::Tensor logsumexp3(const torch::Tensor& a, const torch::Tensor& b, const torch::Tensor& c) {
  auto max_val = torch::max(torch::max(a, b), c);
  return max_val + ((a - max_val).exp() + (b - max_val).exp() + (c - max_val).exp()).log();
}

/* Forward pass of soft-DTW. */
/* dist: (B, T, T) distance matrix */
/* gamma: softness parameter */
/* Returns D: (B, T+1, T+1) DP table with D[:, T, T] = soft-DTW cost. */
inline torch::Tensor softdtw_forward(const torch::Tensor& dist, double gamma) {
  TORCH_CHECK(dist.dim() == 3, "(soft_dtw.h)[softdtw_forward] dist should be (B, T, T)");
  int64_t B = dist.size(0);
  int64_t T = dist.size(1);
  TORCH_CHECK(dist.size(2) == T, "(soft_dtw.h)[softdtw_forward] dist must be square along last two dims");

  /* Initialize DP table D with infinity, except D[:,0,0] = 0 */
  auto D = torch::full({B, T+1, T+1}, std::numeric_limits<float>::infinity(), dist.options());
  D.index_put_({torch::indexing::Slice(), 0, 0}, 0.0);

  auto gamma_t = torch::tensor(gamma, dist.options());

  /* Compute the DP table */
  for (int64_t i = 1; i <= T; i++) {
    for (int64_t j = 1; j <= T; j++) {
      auto d1 = D.index({torch::indexing::Slice(), i-1, j});
      auto d2 = D.index({torch::indexing::Slice(), i, j-1});
      auto d3 = D.index({torch::indexing::Slice(), i-1, j-1});

      /* We take the log-sum-exp of the negative values scaled by gamma for soft minimum */
      auto candidates = logsumexp3(-d1 / gamma_t, -d2 / gamma_t, -d3 / gamma_t);
      auto softmin_val = -gamma_t * candidates;

      auto cost_ij = dist.index({torch::indexing::Slice(), i-1, j-1});
      auto val = cost_ij + softmin_val;
      D.index_put_({torch::indexing::Slice(), i, j}, val);
    }
  }

  return D;
}

/* Compute alignment probabilities using gradients:
   We do this by calling backward on the soft-DTW cost D[:, T, T]. */
inline torch::Tensor softdtw_alignment(torch::Tensor dist, torch::Tensor D, double gamma) {
  int64_t T = dist.size(1);
  TORCH_CHECK(dist.dim() == 3, "[soft_dtw.h](softdtw_alignment) dist must be (B, T, T)");

  /* The soft-DTW cost is at D[:, T, T] */
  auto cost = D.index({torch::indexing::Slice(), T, T}); // shape: (B)
  auto cost_sum = cost.sum(); // Make it a scalar for backward

  {
    // Enable gradient mode and call backward
    torch::AutoGradMode enable_grad(true);
    cost_sum.backward(); // dist.grad() now holds partial derivatives akin to alignment
  }

  auto alignment = dist.grad();
  TORCH_CHECK(alignment.defined(), "[soft_dtw.h](softdtw_alignment) dist.grad() is undefined. Make sure dist is a leaf with requires_grad set.");

  /* Normalize alignment along the last dimension to get probability-like values.
     Add a small epsilon for numerical stability. */
  auto denom = alignment.sum({2}, true) + 1e-9;
  alignment = alignment / denom;

  /* Clear dist.grad to avoid side effects if computed multiple times */
  dist.grad().zero_();

  return alignment;
}

/* Full soft-DTW function that returns the alignment matrix:
   tensor_a: (B, C, T, D) or (B, T, E)
   tensor_b: same shape as tensor_a
   gamma: positive softness parameter
*/
inline torch::Tensor compute_alignment_matrix_softdtw(const torch::Tensor& tensor_a,
                                                      const torch::Tensor& tensor_b,
                                                      double gamma = 0.1) {
  TORCH_CHECK(gamma > 0, "[soft_dtw.h](compute_alignment_matrix_softdtw) Gamma must be positive");

  /* Flatten inputs if needed */
  torch::Tensor flat_a, flat_b;
  if (tensor_a.dim() == 4) {
    // (B, C, T, D) -> (B, T, C*D)
    int64_t B = tensor_a.size(0);
    int64_t T = tensor_a.size(2);
    int64_t C = tensor_a.size(1);
    int64_t D = tensor_a.size(3);
    flat_a = tensor_a.permute({0, 2, 1, 3}).reshape({B, T, C * D});
    flat_b = tensor_b.permute({0, 2, 1, 3}).reshape({B, T, C * D});
  } else if (tensor_a.dim() == 3) {
    // Already (B, T, E)
    flat_a = tensor_a;
    flat_b = tensor_b;
  } else {
    throw std::invalid_argument("[soft_dtw.h](compute_alignment_matrix_softdtw) Unsupported shape for tensor_a");
  }

  /* Compute the pairwise squared distances */
  auto diff = flat_a.unsqueeze(2) - flat_b.unsqueeze(1); // (B, T, T, E)
  auto dist = diff.pow(2).sum(-1);                       // (B, T, T)

  /* Enable gradient tracking on dist */
  dist.set_requires_grad(true);

  /* Forward pass of soft-DTW */
  auto D = softdtw_forward(dist, gamma);

  /* Compute alignment probabilities via backward on soft-DTW cost */
  auto alignment = softdtw_alignment(dist, D, gamma);

  return alignment.detach();
}

} // namespace ts_tcc
} // namespace wikimyei
} // namespace cuwacunu
