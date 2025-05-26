/* optimizers.h */
#pragma once
#include <torch/torch.h>

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {
namespace optim {
/*  clamp_adam_step: zero the step counter once it reaches `threshold`
    so pow(beta, step) never under‑flows and raises errno 34 */
inline void clamp_adam_step(torch::optim::Optimizer& opt, int64_t threshold = 2500)
{
    if(threshold == -1) return;
    using AdamState = torch::optim::AdamWParamState;   // works for Adam too
    bool need_reset = false;
    /* 1. read the current step from the first Adam param‑state */
    for (auto& kv : opt.state()) {
        if (auto* s = dynamic_cast<AdamState*>(kv.second.get())) {
            if (s->step() >= threshold) need_reset = true;
            break;
        }
    }
    if (!need_reset) return;

    /* 2. zero every step tensor */
    for (auto& kv : opt.state()) {
        if (auto* s = dynamic_cast<AdamState*>(kv.second.get())) {
            s->step() = 0;
        }
    }
}

} /* namespace optim */
} /* namespace torch_compat */
} /* namespace piaabo */
} /* namespace cuwacunu */

