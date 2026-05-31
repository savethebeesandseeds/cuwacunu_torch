// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <torch/torch.h>

namespace cuwacunu::wikimyei::observer {

struct tail_risk_t {
  torch::Tensor var_down{};  // [A]
  torch::Tensor cvar_down{}; // [A]
};

[[nodiscard]] inline tail_risk_t
compute_node_tail_risk(const torch::Tensor &scenarios, double alpha = 0.05) {
  TORCH_CHECK(scenarios.defined() && scenarios.dim() == 2,
              "[tail_risk] scenarios must be [S,A]");
  TORCH_CHECK(alpha > 0.0 && alpha < 1.0, "[tail_risk] alpha must be in (0,1)");
  const auto S = scenarios.size(0);
  const auto tail_count = std::max<std::int64_t>(
      1, static_cast<std::int64_t>(std::ceil(alpha * S)));
  auto sorted = std::get<0>(scenarios.to(torch::kFloat64).sort(/*dim=*/0));
  auto tail = sorted.narrow(/*dim=*/0, /*start=*/0, /*length=*/tail_count);
  tail_risk_t out{};
  out.var_down = sorted.select(/*dim=*/0, tail_count - 1);
  out.cvar_down = tail.mean(/*dim=*/0);
  return out;
}

} // namespace cuwacunu::wikimyei::observer
