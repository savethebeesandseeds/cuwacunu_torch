#include <torch/torch.h>
#include <torch/optim/optimizer.h>
#include <torch/optim/schedulers/lr_scheduler.h>
#include <functional> /* required for lambdas */
#include "piaabo/dutils.h"

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {
namespace optim {
namespace schedulers {
/* PyTorch-style LambdaLR (scale factor) */
class LambdaLR : public torch::optim::LRScheduler {
public:
    LambdaLR(torch::optim::Optimizer& optimizer,
             std::function<double(unsigned)> lr_lambda);
    std::vector<double> get_lrs() override;                // absolute LRs

private:
    std::function<double(unsigned)> lr_lambda_;
    std::vector<double>             base_lrs_;
};

/* warm-up (linear) then cosine-decay
   total_epochs      : full schedule length
   warmup_epochs     : first N epochs ramp 1.0 → base_lr
   base_lr           : absolute base learning-rate
   min_lr            : absolute final learning-rate
   Returns: factor f(epoch) so that lr(epoch) = base_lr * f(epoch)      */
std::function<double(unsigned)>
warmup_cosine_lambda(unsigned warmup_epochs,
                     double   base_lr,
                     double   min_lr,
                     unsigned cycle_epochs);


} /* namespace schedulers */
} /* namespace optim */
} /* namespace torch_compat */
} /* namespace piaabo */
} /* namespace cuwacunu */