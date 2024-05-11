#include <torch/torch.h>
#include <torch/optim/schedulers/lr_scheduler.h>
#include <functional>

namespace torch {
namespace optim {

class LambdaLR : public LRScheduler {
 public:
  LambdaLR(Optimizer& optimizer, std::function<double(unsigned)> lr_lambda);
 protected:
  std::vector<double> get_lrs() override;

 private:
  std::function<double(unsigned)> lr_lambda_;
};

} /* namespace optim */
} /* namespace torch */
