// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>

#include <torch/torch.h>

namespace cuwacunu::wikimyei::observer {

struct confidence_inputs_t {
  torch::Tensor data_quality_score{}; // [A], [0,1]
  torch::Tensor liquidity_score{};    // [A], [0,1]
  torch::Tensor calibration_score{};  // [A], [0,1], neutral 1
  torch::Tensor entropy_score{};      // [A], [0,1]
  torch::Tensor channel_score{};      // [A], [0,1]
  torch::Tensor surprise_score{};     // [A], [0,1], neutral 1
};

namespace detail {

inline torch::Tensor score_or_ones(const torch::Tensor &score, std::int64_t A,
                                   const torch::TensorOptions &options,
                                   const char *name) {
  if (!score.defined()) {
    return torch::ones({A}, options);
  }
  TORCH_CHECK(score.dim() == 1 && score.size(0) == A, "[confidence] ", name,
              " must be [A]");
  return score.to(options).clamp(0.0, 1.0);
}

} // namespace detail

[[nodiscard]] inline torch::Tensor compute_confidence(
    const confidence_inputs_t &inputs, std::int64_t A,
    torch::TensorOptions options =
        torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU)) {
  TORCH_CHECK(A >= 0, "[confidence] asset count must be nonnegative");
  auto confidence = torch::ones({A}, options);
  confidence =
      confidence * detail::score_or_ones(inputs.data_quality_score, A, options,
                                         "data_quality_score");
  confidence = confidence * detail::score_or_ones(inputs.liquidity_score, A,
                                                  options, "liquidity_score");
  confidence = confidence * detail::score_or_ones(inputs.calibration_score, A,
                                                  options, "calibration_score");
  confidence = confidence * detail::score_or_ones(inputs.entropy_score, A,
                                                  options, "entropy_score");
  confidence = confidence * detail::score_or_ones(inputs.channel_score, A,
                                                  options, "channel_score");
  confidence = confidence * detail::score_or_ones(inputs.surprise_score, A,
                                                  options, "surprise_score");
  return confidence.clamp(0.0, 1.0);
}

[[nodiscard]] inline torch::Tensor
score_from_normalized_entropy(const torch::Tensor &normalized_entropy) {
  TORCH_CHECK(normalized_entropy.defined() && normalized_entropy.dim() == 1,
              "[confidence] normalized_entropy must be [A]");
  return (1.0 - normalized_entropy.to(torch::kFloat64)).clamp(0.0, 1.0);
}

[[nodiscard]] inline torch::Tensor
score_from_channel_disagreement(const torch::Tensor &channel_disagreement,
                                double scale = 1.0) {
  TORCH_CHECK(channel_disagreement.defined() && channel_disagreement.dim() == 1,
              "[confidence] channel_disagreement must be [A]");
  TORCH_CHECK(scale >= 0.0, "[confidence] scale must be nonnegative");
  return torch::exp(-scale * channel_disagreement.to(torch::kFloat64))
      .clamp(0.0, 1.0);
}

} // namespace cuwacunu::wikimyei::observer
