// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>

#include <torch/torch.h>

#include "wikimyei/observer/utility/mdn_moments.h"

namespace cuwacunu::wikimyei::observer {

struct mixture_entropy_t {
  torch::Tensor entropy{};            // [B,N,C,Df]
  torch::Tensor normalized_entropy{}; // [B,N,C,Df], [0,1] when K > 1
  torch::Tensor dominance{};          // [B,N,C,Df], max_k pi_k
};

[[nodiscard]] inline mixture_entropy_t compute_mixture_entropy(
    const cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut &out) {
  validate_mdn_out(out);
  const auto K = out.log_pi.size(-1);
  auto pi = out.log_pi.exp();
  auto entropy = -(pi * out.log_pi).sum(/*dim=*/-1);
  auto dominance = std::get<0>(pi.max(/*dim=*/-1));
  auto normalized = K > 1 ? entropy / std::log(static_cast<double>(K))
                          : torch::zeros_like(entropy);

  mixture_entropy_t result{};
  result.entropy = std::move(entropy);
  result.normalized_entropy = std::move(normalized);
  result.dominance = std::move(dominance);
  return result;
}

} // namespace cuwacunu::wikimyei::observer
