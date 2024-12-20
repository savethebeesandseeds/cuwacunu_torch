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
class LambdaLR : public torch::optim::LRScheduler {
  public:
    LambdaLR(torch::optim::Optimizer& optimizer, std::function<double(unsigned)> lr_lambda);
    std::vector<double> get_lrs() override;
  private:
    std::function<double(unsigned)> lr_lambda_;
};
} /* namespace schedulers */
} /* namespace optim */
} /* namespace torch_compat */
} /* namespace piaabo */
} /* namespace cuwacunu */
