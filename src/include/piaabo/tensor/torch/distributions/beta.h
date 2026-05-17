#include <torch/torch.h>
#include "piaabo/core/utils.h"
#include "piaabo/tensor/torch/torch_utils.h"
#include "piaabo/tensor/torch/distributions/gamma.h"
/* As per indicated here: https://discuss.pytorch.org/t/torch-distributions-categorical/45747/6
 * 	There is no implementation of the distributions module in Libtorch.
 *
 *	This is a pytorch/torch/distribution in cpp using libtorch:
 *		Beta distribution
 */

namespace cuwacunu {
namespace piaabo {
namespace tensor {
namespace torch {
namespace distributions {
class Beta {
private:
  ::torch::Tensor concentration0;
  ::torch::Tensor concentration1;
  ::torch::Device kDevice;
  ::torch::Dtype kType;
public:
  /* Usual Constructor */
  Beta(::torch::Device device, ::torch::Dtype type, ::torch::Tensor concentration0, ::torch::Tensor concentration1);
  /* Mean */
  ::torch::Tensor mean() const;
  /* Mode */
  ::torch::Tensor mode() const;
  /* Variance */
  ::torch::Tensor variance() const;
  /* Sample */
  ::torch::Tensor sample(const ::torch::IntArrayRef& sample_shape = {}) const;
  /* Log Probability */
  ::torch::Tensor log_prob(::torch::Tensor value) const;
  /* Entropy */
  ::torch::Tensor entropy() const;
  /* Getters for concentration parameters */
  ::torch::Tensor get_concentration1() const;
  ::torch::Tensor get_concentration0() const;
};
} /* namespace distributions */
} /* namespace torch */
} /* namespace tensor */
} /* namespace piaabo */
} /* namespace cuwacunu */