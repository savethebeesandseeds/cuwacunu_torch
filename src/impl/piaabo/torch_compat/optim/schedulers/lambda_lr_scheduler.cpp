#include "piaabo/torch_compat/optim/schedulers/lambda_lr_scheduler.h"
#include <cmath>          // std::cos, M_PI
#include <algorithm>      // std::max

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {
namespace optim {
namespace schedulers {

/* ----- LambdaLR methods ------------------------------------ */

/* ---------- LambdaLR implementation ---------- */
LambdaLR::LambdaLR(torch::optim::Optimizer& optimizer,
                   std::function<double(unsigned)> lr_lambda)
    : LRScheduler(optimizer),
      lr_lambda_(std::move(lr_lambda)),
      base_lrs_(get_current_lrs()) {}

std::vector<double> LambdaLR::get_lrs() {
    double factor = lr_lambda_(step_count_);            // scale factor

    std::vector<double> new_lrs;
    new_lrs.reserve(base_lrs_.size());
    for (double base_lr : base_lrs_)
        new_lrs.push_back(base_lr * factor);            // absolute LR

    return new_lrs;                                     // base class stores it
}


/* ------------------------------------------------------------------
   warmup_cosine_lambda
   - Epoch 0 … warmup_epochs‑1     : lr = base_lr
   - After warm‑up: cosine(base_lr → min_lr) over cycle_epochs.
     When the cycle ends it restarts (cosine annealing *with* restarts).

   returns factor f(t) so that lr(t) = base_lr * f(t)                */
std::function<double(unsigned)>
warmup_cosine_lambda(unsigned warmup_epochs,
                     double   base_lr,
                     double   min_lr,
                     unsigned cycle_epochs)
{
    base_lr  = std::max(1e-12, base_lr);
    min_lr   = std::clamp(min_lr, 0.0, base_lr);
    cycle_epochs = std::max<unsigned>(1, cycle_epochs);

    const unsigned decay_part = std::max<int>(1, cycle_epochs - warmup_epochs);
    const double   inv_base   = 1.0 / base_lr;

    return [=](unsigned epoch) -> double {
        /* ---- stage 0: fixed warm‑up at base_lr ---- */
        if (epoch < warmup_epochs)
            return 1.0;                                // lr = base_lr

        /* ---- stage 1: cosine with restarts ---- */
        unsigned cyc_epoch = (epoch - warmup_epochs) % cycle_epochs;
        double progress    = static_cast<double>(cyc_epoch) / decay_part;   // 0→1

        if (cyc_epoch >= decay_part)                   // flat min segment
            return min_lr * inv_base;

        double cosine = 0.5 * (1.0 + std::cos(M_PI * progress));            // 1→0
        double lr_abs = min_lr + (base_lr - min_lr) * cosine;               // peak→valley
        return lr_abs * inv_base;                                           // scale
    };
}

}  // namespace schedulers
}  // namespace optim
}  // namespace torch_compat
}  // namespace piaabo
}  // namespace cuwacunu
