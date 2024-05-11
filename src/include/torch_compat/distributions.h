#include <stdexcept>
#include <cmath>
#include <torch/torch.h>
/* As per indicated here: https://discuss.pytorch.org/t/torch-distributions-categorical/45747/6
 * 	There is no implementation of the distributions module in Libtorch.
 *
 *	These are pytorch/torch/distributions in cpp using libtorch:
 *		Categorical distribution
 *		Beta distribution
 */
class Categorical {
private:
    torch::Tensor logits;
    torch::Device kDevice;
    torch::Dtype kType;
public:
    Categorical(torch::Device device, torch::Dtype type, torch::Tensor logits);
    /* Sample */
    torch::Tensor sample(const torch::IntArrayRef& sample_shape = {}) const;
    /* Log Probabitly */
    torch::Tensor log_prob(const torch::Tensor& value) const;
    /* Probs */
    torch::Tensor probs() const;
    /* Entropy */
    torch::Tensor entropy() const;
};

class Beta {
private:
    torch::Tensor concentration1;
    torch::Tensor concentration0;
    torch::Device kDevice;
    torch::Dtype kType;
public:
    /* Constructor */
    Beta(torch::Device device, torch::Dtype type, torch::Tensor concentration1, torch::Tensor concentration0);
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