// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>

#include <torch/torch.h>

namespace cuwacunu::wikimyei::representation::encoding::vicreg {

struct vicreg_loss_options_t {
  double invariance_weight{25.0};
  double variance_weight{25.0};
  double covariance_weight{1.0};
  double global_aux_weight{0.0};
  double variance_floor{1.0};
  double eps{1e-4};
};

struct vicreg_loss_result_t {
  torch::Tensor loss{};
  torch::Tensor invariance_loss{};
  torch::Tensor variance_loss{};
  torch::Tensor covariance_loss{};
  torch::Tensor global_loss{};
  torch::Tensor global_invariance_loss{};
  torch::Tensor global_variance_loss{};
  torch::Tensor global_covariance_loss{};
  torch::Tensor per_channel_valid_rows{};      // [C], int64
  torch::Tensor per_channel_invariance_loss{}; // [C]
  torch::Tensor per_channel_variance_loss{};   // [C]
  torch::Tensor per_channel_covariance_loss{}; // [C]
  torch::Tensor per_channel_norm_mean{};       // [C]
  torch::Tensor cross_channel_similarity{};    // [C,C]
  int64_t valid_rows{0};
};

namespace vicreg_loss_detail {

inline torch::Tensor valid_rows(const torch::Tensor &z,
                                const torch::Tensor &mask) {
  TORCH_CHECK(z.defined(), "[vicreg_loss] z is undefined");
  TORCH_CHECK(mask.defined(), "[vicreg_loss] mask is undefined");
  if (z.dim() == 4) {
    TORCH_CHECK(mask.dim() == 3,
                "[vicreg_loss] rank-4 z requires [M,C,Hx] mask");
    TORCH_CHECK(z.size(0) == mask.size(0) && z.size(1) == mask.size(1) &&
                    z.size(2) == mask.size(2),
                "[vicreg_loss] z/mask shape mismatch");
    auto rows = mask.to(torch::kBool).reshape({-1}).nonzero().reshape({-1});
    return z.reshape({-1, z.size(3)}).index_select(/*dim=*/0, rows);
  }
  if (z.dim() == 3) {
    TORCH_CHECK(mask.dim() == 2, "[vicreg_loss] rank-3 z requires [M,C] mask");
    TORCH_CHECK(z.size(0) == mask.size(0) && z.size(1) == mask.size(1),
                "[vicreg_loss] z/mask shape mismatch");
    auto rows = mask.to(torch::kBool).reshape({-1}).nonzero().reshape({-1});
    return z.reshape({-1, z.size(2)}).index_select(/*dim=*/0, rows);
  }
  TORCH_CHECK(false, "[vicreg_loss] z must be [M,C,Hx,De] or [M,C,De]");
}

inline torch::Tensor variance_term(const torch::Tensor &z, double floor,
                                   double eps) {
  auto std = torch::sqrt(z.var(/*dim=*/0, /*unbiased=*/false) + eps);
  return torch::relu(floor - std).mean();
}

inline torch::Tensor covariance_term(const torch::Tensor &z) {
  const auto rows = z.size(0);
  const auto dim = z.size(1);
  if (rows <= 1 || dim <= 1) {
    return torch::zeros({}, z.options());
  }
  auto centered = z - z.mean(/*dim=*/0, /*keepdim=*/true);
  auto cov =
      centered.transpose(0, 1).matmul(centered) / static_cast<double>(rows - 1);
  auto off_diag = cov - torch::diag(torch::diag(cov));
  return off_diag.pow(2).sum() / static_cast<double>(dim);
}

inline torch::Tensor channel_rows(const torch::Tensor &z,
                                  const torch::Tensor &mask, int64_t channel) {
  using torch::indexing::Slice;
  if (z.dim() == 4) {
    auto zc = z.index({Slice(), channel, Slice(), Slice()});
    auto mc = mask.index({Slice(), channel, Slice()}).to(torch::kBool);
    auto rows = mc.reshape({-1}).nonzero().reshape({-1});
    return zc.reshape({-1, z.size(3)}).index_select(/*dim=*/0, rows);
  }
  auto zc = z.index({Slice(), channel, Slice()});
  auto mc = mask.index({Slice(), channel}).to(torch::kBool);
  auto rows = mc.reshape({-1}).nonzero().reshape({-1});
  return zc.index_select(/*dim=*/0, rows);
}

inline torch::Tensor mean_or_zero(const torch::Tensor &rows, int64_t dim,
                                  const torch::TensorOptions &options) {
  if (rows.size(0) == 0) {
    return torch::zeros({dim}, options);
  }
  return rows.mean(/*dim=*/0);
}

} // namespace vicreg_loss_detail

[[nodiscard]] inline vicreg_loss_result_t
compute_vicreg_loss(const torch::Tensor &z1, const torch::Tensor &mask1,
                    const torch::Tensor &z2, const torch::Tensor &mask2,
                    const vicreg_loss_options_t &options = {}) {
  TORCH_CHECK(z1.sizes() == z2.sizes(), "[vicreg_loss] z1/z2 shape mismatch");
  TORCH_CHECK(mask1.sizes() == mask2.sizes(),
              "[vicreg_loss] mask1/mask2 shape mismatch");
  auto joint_mask = mask1.to(torch::kBool).logical_and(mask2.to(torch::kBool));
  auto rows1 = vicreg_loss_detail::valid_rows(z1, joint_mask);
  auto rows2 = vicreg_loss_detail::valid_rows(z2, joint_mask);
  vicreg_loss_result_t out{};
  out.valid_rows = std::min(rows1.size(0), rows2.size(0));
  if (out.valid_rows <= 0) {
    out.global_loss = torch::zeros({}, z1.options());
    out.global_invariance_loss = torch::zeros({}, z1.options());
    out.global_variance_loss = torch::zeros({}, z1.options());
    out.global_covariance_loss = torch::zeros({}, z1.options());
  } else {
    if (rows1.size(0) != out.valid_rows) {
      rows1 = rows1.narrow(0, 0, out.valid_rows);
    }
    if (rows2.size(0) != out.valid_rows) {
      rows2 = rows2.narrow(0, 0, out.valid_rows);
    }
    out.global_invariance_loss = torch::mse_loss(rows1, rows2);
    out.global_variance_loss =
        0.5 * (vicreg_loss_detail::variance_term(rows1, options.variance_floor,
                                                 options.eps) +
               vicreg_loss_detail::variance_term(rows2, options.variance_floor,
                                                 options.eps));
    out.global_covariance_loss =
        0.5 * (vicreg_loss_detail::covariance_term(rows1) +
               vicreg_loss_detail::covariance_term(rows2));
    out.global_loss = options.invariance_weight * out.global_invariance_loss +
                      options.variance_weight * out.global_variance_loss +
                      options.covariance_weight * out.global_covariance_loss;
  }

  const int64_t C = z1.size(1);
  const int64_t De = z1.size(z1.dim() - 1);
  std::vector<torch::Tensor> valid_counts;
  std::vector<torch::Tensor> inv_terms;
  std::vector<torch::Tensor> var_terms;
  std::vector<torch::Tensor> cov_terms;
  std::vector<torch::Tensor> norm_terms;
  std::vector<torch::Tensor> mean_vectors;
  valid_counts.reserve(static_cast<std::size_t>(C));
  inv_terms.reserve(static_cast<std::size_t>(C));
  var_terms.reserve(static_cast<std::size_t>(C));
  cov_terms.reserve(static_cast<std::size_t>(C));
  norm_terms.reserve(static_cast<std::size_t>(C));
  mean_vectors.reserve(static_cast<std::size_t>(C));
  for (int64_t c = 0; c < C; ++c) {
    auto crows1 = vicreg_loss_detail::channel_rows(z1, joint_mask, c);
    auto crows2 = vicreg_loss_detail::channel_rows(z2, joint_mask, c);
    const int64_t rows = std::min(crows1.size(0), crows2.size(0));
    valid_counts.push_back(torch::full(
        {}, rows,
        torch::TensorOptions().dtype(torch::kInt64).device(z1.device())));
    if (rows == 0) {
      inv_terms.push_back(torch::zeros({}, z1.options()));
      var_terms.push_back(torch::zeros({}, z1.options()));
      cov_terms.push_back(torch::zeros({}, z1.options()));
      norm_terms.push_back(torch::zeros({}, z1.options()));
      mean_vectors.push_back(torch::zeros({De}, z1.options()));
      continue;
    }
    crows1 = crows1.narrow(0, 0, rows);
    crows2 = crows2.narrow(0, 0, rows);
    inv_terms.push_back(torch::mse_loss(crows1, crows2));
    var_terms.push_back(
        0.5 * (vicreg_loss_detail::variance_term(crows1, options.variance_floor,
                                                 options.eps) +
               vicreg_loss_detail::variance_term(crows2, options.variance_floor,
                                                 options.eps)));
    cov_terms.push_back(0.5 * (vicreg_loss_detail::covariance_term(crows1) +
                               vicreg_loss_detail::covariance_term(crows2)));
    norm_terms.push_back(crows1.norm(2, /*dim=*/1).mean());
    mean_vectors.push_back(
        vicreg_loss_detail::mean_or_zero(crows1, De, z1.options()));
  }
  out.per_channel_valid_rows = torch::stack(valid_counts);
  out.per_channel_invariance_loss = torch::stack(inv_terms);
  out.per_channel_variance_loss = torch::stack(var_terms);
  out.per_channel_covariance_loss = torch::stack(cov_terms);
  out.per_channel_norm_mean = torch::stack(norm_terms);
  auto valid_channel =
      out.per_channel_valid_rows.to(torch::kBool).to(z1.device());
  auto valid_channel_weight = valid_channel.to(z1.dtype());
  auto valid_channel_count = valid_channel_weight.sum().clamp_min(1.0);
  out.invariance_loss =
      (out.per_channel_invariance_loss * valid_channel_weight).sum() /
      valid_channel_count;
  out.variance_loss =
      (out.per_channel_variance_loss * valid_channel_weight).sum() /
      valid_channel_count;
  out.covariance_loss =
      (out.per_channel_covariance_loss * valid_channel_weight).sum() /
      valid_channel_count;
  auto primary_loss = options.invariance_weight * out.invariance_loss +
                      options.variance_weight * out.variance_loss +
                      options.covariance_weight * out.covariance_loss;
  out.loss = primary_loss + options.global_aux_weight * out.global_loss;
  auto means = torch::stack(mean_vectors); // [C,De]
  auto denom = means.norm(2, /*dim=*/1, /*keepdim=*/true).clamp_min(1e-12);
  auto normalized = means / denom;
  out.cross_channel_similarity = normalized.matmul(normalized.transpose(0, 1));
  return out;
}

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg
