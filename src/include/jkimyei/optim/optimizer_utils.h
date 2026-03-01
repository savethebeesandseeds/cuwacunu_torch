/* optimizer_utils.h
 *
 * Optimizer runtime helpers for jkimyei.
 */
#pragma once

#include <cstdint>

#include <torch/torch.h>

namespace cuwacunu {
namespace jkimyei {
namespace optim {

/* clamp_adam_step:
 * reset Adam/AdamW step counters once threshold is reached
 * to avoid pow(beta, step) numerical underflow in long runs.
 */
inline void clamp_adam_step(torch::optim::Optimizer& opt, int64_t threshold = 2500) {
  if (threshold == -1) return;

  using AdamState = torch::optim::AdamWParamState; // compatible with Adam and AdamW
  bool need_reset = false;

  for (auto& kv : opt.state()) {
    if (auto* state = dynamic_cast<AdamState*>(kv.second.get())) {
      if (state->step() >= threshold) need_reset = true;
      break;
    }
  }

  if (!need_reset) return;

  for (auto& kv : opt.state()) {
    if (auto* state = dynamic_cast<AdamState*>(kv.second.get())) {
      state->step() = 0;
    }
  }
}

} // namespace optim
} // namespace jkimyei
} // namespace cuwacunu

