#include "torch_compat/optim/schedulers/lambda_lr_scheduler.h"

namespace torch_compat {
namespace optim {
namespace schedulers {
LambdaLR::LambdaLR(torch::optim::Optimizer& optimizer, std::function<double(unsigned)> lr_lambda)
    : LRScheduler(optimizer), lr_lambda_(lr_lambda) {}

std::vector<double> LambdaLR::get_lrs() {
    auto current_lrs = get_current_lrs();  // Retrieve current learning rates from each parameter group
    std::vector<double> new_lrs;   // This will hold the new learning rates for each parameter group
    new_lrs.reserve(current_lrs.size());  // Optimization to reserve space for the new rates
    for (size_t i = 0; i < current_lrs.size(); ++i) {  // Iterate over each parameter group's current learning rate
        new_lrs.push_back(lr_lambda_(step_count_));  // Compute new LR for this group based on the epoch
    }
    return new_lrs;  // Return the list of new learning rates, one for each parameter group
}
} /* namespace schedulers */
} /* namespace optim */
} /* namespace torch_compat */