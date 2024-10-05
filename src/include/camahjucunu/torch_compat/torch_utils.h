#include "piaabo/dutils.h"
#include <torch/torch.h>

namespace cuwacunu {
/* Set default device based on availability of CUDA */
  extern torch::Device kDevice;
  extern torch::Dtype kType;
/**
 * Validates parameters of a given torch::nn::Module.
 * Ensures that the module has parameters, they are defined, not NaN, and not empty.
 *
 * @param model The module whose parameters are to be validated.
 */
void validate_module_parameters(const torch::nn::Module& model);

/**
 * Validates a given tensor to ensure it is defined and not empty.
 * Logs a fatal error if the tensor is undefined or empty.
 *
 * @param tensor The tensor to be validated.
 * @param label A label to identify the tensor in log messages.
 */
void validate_tensor(const torch::Tensor& tensor, const char* label);

/**
 * Asserts that a given tensor has the expected size along its first dimension.
 * Logs a fatal error if the size does not match the expected size.
 *
 * @param tensor The tensor to be checked.
 * @param expected_size The expected size of the tensor along its first dimension.
 * @param label A label to identify the tensor in log messages.
 */
void assert_tensor_shape(const torch::Tensor& tensor, int64_t expected_size, const char* label);

/**
 * Prints information about a given tensor.
 * Includes tensor sizes, data type, device, gradient requirement, and optionally values if small enough.
 *
 * @param tensor The tensor whose information is to be printed.
 */
void print_tensor_info(const torch::Tensor& tensor);

} /* namespace torch_compat */