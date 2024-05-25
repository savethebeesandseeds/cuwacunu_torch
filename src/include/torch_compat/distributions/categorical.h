#include <stdexcept>
#include <torch/torch.h>
#include "dutils.h"
#include "architecture.h"
#include "torch_compat/torch_utils.h"
/* As per indicated here: https://discuss.pytorch.org/t/torch-distributions-categorical/45747/6
 * 	There is no implementation of the distributions module in Libtorch.
 *
 *	This is a pytorch/torch/distribution in cpp using libtorch:
 *		Categorical distribution
 */

namespace torch_compat {
namespace distributions {
class Categorical {
private:
  torch::Device kDevice;
  torch::Dtype kType;
  torch::Tensor logits;
  torch::Tensor log_probs;
public:
  /* Usual Constructor */
  Categorical(torch::Device device, torch::Dtype type, torch::Tensor logits);
  /* Sample */
  torch::Tensor sample(const torch::IntArrayRef& sample_shape = {}) const;
  /* Masked Sample - a mask is a boolean tensor e.g. [1, 1, 0, 1] where the probability the zero elements is near to zero */
  torch::Tensor mask_sample(const torch::Tensor& mask, const torch::IntArrayRef& sample_shape = {}) const;
  /* Log Probabitly */
  torch::Tensor log_prob(const torch::Tensor& value) const;
  /* Probs */
  torch::Tensor probs() const;
  /* Entropy */
  torch::Tensor entropy() const;
};
ENFORCE_ARCHITECTURE_DESIGN(Categorical);
} /* namespace distributions */
} /* namespace torch_compat */