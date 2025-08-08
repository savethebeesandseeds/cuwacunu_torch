/* soft_dtw.h */
#pragma once
// Based on: https://github.com/Sleepwalking/pytorch-softdtw/blob/master/soft_dtw.py
#include <torch/torch.h>
#include <cmath>
#include <limits>
#include <vector>

namespace cuwacunu {
namespace wikimyei {
namespace ts_tcc {

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 1) compute_softdtw
//    - D: [B, N, M]
//    - gamma: scalar
//    Returns R: [B, N+2, M+2]
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
inline torch::Tensor compute_softdtw(const torch::Tensor& D_in, double gamma)
{
    TORCH_CHECK(D_in.dim() == 3, "D must have shape [B, N, M].");
    // For safety, move to CPU and ensure contiguous:
    auto D = D_in.cpu().contiguous().to(torch::kFloat32);
    
    int64_t B = D.size(0);
    int64_t N = D.size(1);
    int64_t M = D.size(2);

    // We'll create R of shape [B, N+2, M+2], fill with +∞
    auto inf = std::numeric_limits<double>::infinity();
    auto options = D.options(); // already double + CPU
    torch::Tensor R = torch::full({B, N+2, M+2}, inf, options);

    // R[:, 0, 0] = 0
    {
      auto R_cpu = R.contiguous();
      auto R_a   = R_cpu.accessor<double, 3>();
      for (int64_t b = 0; b < B; b++) {
          R_a[b][0][0] = 0.0;
      }
      R = R_cpu; // copy back, though R was already CPU + contiguous
    }

    // Accessors for the main loop:
    auto R_a = R.accessor<double, 3>();
    auto D_a = D.accessor<double, 3>();

    // Triple-nested loop
    for (int64_t b = 0; b < B; b++) {
        for (int64_t j = 1; j <= M; j++) {
            for (int64_t i = 1; i <= N; i++) {
                double r0 = -R_a[b][i-1][j-1] / gamma;
                double r1 = -R_a[b][i-1][j  ] / gamma;
                double r2 = -R_a[b][i  ][j-1] / gamma;

                double rmax = std::max(std::max(r0, r1), r2);
                double rsum = std::exp(r0 - rmax)
                            + std::exp(r1 - rmax)
                            + std::exp(r2 - rmax);

                double softmin = -gamma * (std::log(rsum) + rmax);

                // R[b, i, j] = D[b, i-1, j-1] + softmin
                double cost_ij = D_a[b][i-1][j-1];
                R_a[b][i][j] = cost_ij + softmin;
            }
        }
    }

    return R;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2) compute_softdtw_backward
//    - D_: [B, N, M]
//    - R:  [B, N+2, M+2]
//    - gamma: scalar
//    Returns E_sub: [B, N, M]  (the gradient w.r.t. D)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
inline torch::Tensor compute_softdtw_backward(const torch::Tensor& D_in, 
                                              torch::Tensor R_in,
                                              double gamma)
{
    TORCH_CHECK(D_in.dim() == 3, "D_ must have shape [B, N, M].");
    TORCH_CHECK(R_in.dim()  == 3, "R must have shape [B, N+2, M+2].");

    // Move everything to CPU + contiguous
    auto D_ = D_in.cpu().contiguous().to(torch::kFloat32);
    auto R  = R_in.cpu().contiguous().to(torch::kFloat32);

    int64_t B = D_.size(0);
    int64_t N = D_.size(1);
    int64_t M = D_.size(2);

    // We'll create D_big [B, N+2, M+2], fill with zeros, put D_ inside
    auto options = D_.options();
    torch::Tensor D_big = torch::zeros({B, N+2, M+2}, options);

    // Place D_ in the [1..N, 1..M] block
    D_big.index_put_(
      {torch::indexing::Slice(), 
       torch::indexing::Slice(1, N+1), 
       torch::indexing::Slice(1, M+1)},
      D_
    );

    // E is also [B, N+2, M+2]
    torch::Tensor E = torch::zeros({B, N+2, M+2}, options);

    // E[:, -1, -1] = 1
    {
      auto E_a = E.accessor<double, 3>();
      for (int64_t b = 0; b < B; b++) {
          E_a[b][N+1][M+1] = 1.0;
      }
    }

    // Set R[:, :, -1] = -inf, R[:, -1, :] = -inf, then R[:, -1, -1] = R[:, -2, -2]
    double inf = -std::numeric_limits<double>::infinity();
    {
      auto R_a = R.accessor<double, 3>();
      for (int64_t b = 0; b < B; b++) {
          for (int64_t i = 0; i < (N+2); i++) {
              R_a[b][i][M+1] = inf;
          }
          for (int64_t j = 0; j < (M+2); j++) {
              R_a[b][N+1][j] = inf;
          }
          // set R[b, -1, -1] = R[b, -2, -2]
          double val = R_a[b][N][M];
          R_a[b][N+1][M+1] = val;
      }
    }

    // Accessors for the backward pass
    auto R_a = R.accessor<double, 3>();
    auto E_a = E.accessor<double, 3>();
    auto D_a = D_big.accessor<double, 3>();

    // triple nested loop (backward)
    for (int64_t b = 0; b < B; b++) {
        for (int64_t j = M; j >= 1; j--) {
            for (int64_t i = N; i >= 1; i--) {
                double a0 = ( R_a[b][i+1][j  ] 
                            - R_a[b][i  ][j  ] 
                            - D_a[b][i+1][j  ]) / gamma;
                double b0 = ( R_a[b][i  ][j+1] 
                            - R_a[b][i  ][j  ] 
                            - D_a[b][i  ][j+1]) / gamma;
                double c0 = ( R_a[b][i+1][j+1] 
                            - R_a[b][i  ][j  ] 
                            - D_a[b][i+1][j+1]) / gamma;

                double A  = std::exp(a0);
                double Bv = std::exp(b0);
                double C  = std::exp(c0);

                double valE = E_a[b][i+1][j  ] * A
                            + E_a[b][i  ][j+1] * Bv
                            + E_a[b][i+1][j+1] * C;

                E_a[b][i][j] = valE;
            }
        }
    }

    // Return E[:, 1..N, 1..M]
    torch::Tensor E_sub = E.index({
        torch::indexing::Slice(),
        torch::indexing::Slice(1, N+1),
        torch::indexing::Slice(1, M+1)
    });

    return E_sub;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 3) Custom autograd function, like _SoftDTW(Function) in Python
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct SoftDTWFunction : public torch::autograd::Function<SoftDTWFunction>
{
    static torch::Tensor forward(
        torch::autograd::AutogradContext *ctx,
        const torch::Tensor &D,
        double gamma)
    {
        // 1) Compute R
        auto R = compute_softdtw(D, gamma);

        // 2) Save for backward
        ctx->save_for_backward({D, R});
        ctx->saved_data["gamma"] = gamma;

        // 3) Return R[:, -2, -2] => final cost
        // shape of R is [B, N+2, M+2], so R[:, -2, -2] is R[:, N, M]
        auto cost = R.index({torch::indexing::Slice(), -2, -2}); 
        return cost;  // shape [B]
    }

    static torch::autograd::tensor_list backward(
        torch::autograd::AutogradContext *ctx,
        torch::autograd::tensor_list grad_outputs)
    {
        auto grad_output = grad_outputs[0]; // shape [B]

        // Retrieve saved tensors
        auto saved = ctx->get_saved_variables();
        auto D     = saved[0];
        auto R     = saved[1];
        double gamma = ctx->saved_data["gamma"].toDouble();

        // 1) We need partial derivative w.r.t. D
        auto E = compute_softdtw_backward(D, R, gamma);  // [B, N, M]

        // 2) Multiply by grad_output broadcasted
        auto expanded = grad_output.view({-1, 1, 1}); // [B,1,1]
        auto grad_D   = expanded * E;                 // [B, N, M]

        // Return gradient w.r.t. (D, gamma)
        // gamma has no gradient here => torch::Tensor()
        return {grad_D, torch::Tensor()};
    }
};

// Helper to call SoftDTWFunction in forward passes:
inline torch::Tensor soft_dtw_autograd(const torch::Tensor &D, double gamma)
{
    // Extra checks if desired:
    TORCH_CHECK(D.dim() == 3, "D must be [B, N, M]");
    // Could also ensure D is CPU here, if you want
    // TORCH_CHECK(!D.is_cuda(), "soft_dtw_autograd expects a CPU tensor for D");

    return SoftDTWFunction::apply(D, gamma);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 4) A C++ Module "SoftDTW" similar to the Python class
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct SoftDTWImpl : torch::nn::Module
{
    double gamma;
    bool   normalize;
    
    SoftDTWImpl(double gamma_ = 1.0, bool normalize_ = false)
        : gamma(gamma_), normalize(normalize_)
    {
        // empty
    }

    // x: [B, N, D], y: [B, M, D] -> dist: [B, N, M]
    torch::Tensor calc_distance_matrix(const torch::Tensor &x, const torch::Tensor &y)
    {
        // Move to CPU + contiguous if needed
        auto x_ = x.cpu().contiguous();
        auto y_ = y.cpu().contiguous();

        auto B = x_.size(0);
        auto N = x_.size(1);
        auto M = y_.size(1);
        auto D = x_.size(2);

        // Expand x => [B, N, M, D], y => [B, N, M, D]
        auto x_expand = x_.unsqueeze(2).expand({B, N, M, D});
        auto y_expand = y_.unsqueeze(1).expand({B, N, M, D});

        // Sum of squared differences along D
        auto dist = (x_expand - y_expand).pow(2).sum(/*dim=*/3); 
        return dist; // [B, N, M]
    }

    std::tuple<torch::Tensor, torch::Tensor> forward(const torch::Tensor &x, const torch::Tensor &y)
    {
        // Possibly unsqueeze if shape < 3
        auto x_ = x;
        auto y_ = y;
        bool squeezed = false;
        if (x_.dim() < 3) {
            x_ = x_.unsqueeze(0); // => [1, N, D]
            y_ = y_.unsqueeze(0); // => [1, M, D]
            squeezed = true;
        }

        auto D_xy  = calc_distance_matrix(x_, y_);
        auto out_xy = soft_dtw_autograd(D_xy, gamma);

        if (normalize) {
            // do the extra x-x, y-y
            auto D_xx   = calc_distance_matrix(x_, x_);
            auto out_xx = soft_dtw_autograd(D_xx, gamma);

            auto D_yy   = calc_distance_matrix(y_, y_);
            auto out_yy = soft_dtw_autograd(D_yy, gamma);

            // replicate: out_xy - 1/2*(out_xx + out_yy)
            auto result = out_xy - 0.5 * (out_xx + out_yy);

            return std::make_tuple(
                squeezed ? result.squeeze(0) : result,
                squeezed ? D_xy.squeeze(0)   : D_xy
            );
        } else {
            return std::make_tuple(
                squeezed ? out_xy.squeeze(0) : out_xy,
                squeezed ? D_xy.squeeze(0)   : D_xy
            );
        }
    }
};
TORCH_MODULE(SoftDTW);

} // namespace ts_tcc
} // namespace wikimyei
} // namespace cuwacunu
