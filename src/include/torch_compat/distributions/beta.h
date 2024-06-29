#include <torch/torch.h>
#include "piaabo/dutils.h"
#include "piaabo/architecture.h"
#include "torch_compat/torch_utils.h"
#include "torch_compat/distributions/gamma.h"
/* As per indicated here: https://discuss.pytorch.org/t/torch-distributions-categorical/45747/6
 * 	There is no implementation of the distributions module in Libtorch.
 *
 *	This is a pytorch/torch/distribution in cpp using libtorch:
 *		Beta distribution
 */

namespace torch_compat {
namespace distributions {
class Beta {
private:
  torch::Tensor concentration0;
  torch::Tensor concentration1;
  torch::Device kDevice;
  torch::Dtype kType;
public:
  /* Usual Constructor */
  Beta(torch::Device device, torch::Dtype type, torch::Tensor concentration0, torch::Tensor concentration1);
  /* Mean */
  torch::Tensor mean() const;
  /* Mode */
  torch::Tensor mode() const;
  /* Variance */
  torch::Tensor variance() const;
  /* Sample */
  torch::Tensor sample(const torch::IntArrayRef& sample_shape = {}) const;
  /* Log Probability */
  torch::Tensor log_prob(torch::Tensor value) const;
  /* Entropy */
  torch::Tensor entropy() const;
  /* Getters for concentration parameters */
  torch::Tensor get_concentration1() const;
  torch::Tensor get_concentration0() const;
};
ENFORCE_ARCHITECTURE_DESIGN(Beta);
} /* namespace distributions */
} /* namespace torch_compat */