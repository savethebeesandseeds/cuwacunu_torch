// SPDX-License-Identifier: MIT
#pragma once

#include <torch/torch.h>

namespace cuwacunu::wikimyei::observer {

struct volatility_options_t {
  double mdn_weight{0.5};
};

struct volatility_t {
  torch::Tensor mdn_variance{};      // [A]
  torch::Tensor realized_variance{}; // [A]
  torch::Tensor final_variance{};    // [A]
  torch::Tensor volatility{};        // [A]
};

[[nodiscard]] inline volatility_t
blend_mdn_and_realized_variance(const torch::Tensor &mdn_variance,
                                const torch::Tensor &realized_variance,
                                const volatility_options_t &options = {}) {
  TORCH_CHECK(mdn_variance.defined() && mdn_variance.dim() == 1,
              "[volatility] mdn_variance must be [A]");
  TORCH_CHECK(realized_variance.defined() &&
                  realized_variance.sizes() == mdn_variance.sizes(),
              "[volatility] realized_variance must be [A]");
  TORCH_CHECK(options.mdn_weight >= 0.0 && options.mdn_weight <= 1.0,
              "[volatility] mdn_weight must be in [0,1]");
  auto mdn = mdn_variance.to(torch::kFloat64).clamp_min(0.0);
  auto realized = realized_variance.to(torch::kFloat64).clamp_min(0.0);
  auto final = options.mdn_weight * mdn + (1.0 - options.mdn_weight) * realized;
  volatility_t out{};
  out.mdn_variance = std::move(mdn);
  out.realized_variance = std::move(realized);
  out.final_variance = final;
  out.volatility = final.sqrt();
  return out;
}

} // namespace cuwacunu::wikimyei::observer
