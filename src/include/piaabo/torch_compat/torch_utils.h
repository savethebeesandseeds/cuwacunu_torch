/* torch_utils.h */
#pragma once
#include <torch/torch.h>
#include "piaabo/dutils.h"
#include <vector>

#define DBG_T(dev_msg, t)                                 \
  do {                                                    \
    std::cout << "[DBG] " << (dev_msg)                    \
          << " shape=" << (t).sizes()                     \
          << " dev=" << (t).device()                      \
          << " rg="  << (t).requires_grad() << '\n';      \
    } while(false)

#define ENSURE_SAME_DEVICE(tensor, ref_tensor)            \
  TORCH_CHECK((tensor).device() == (ref_tensor).device(), \
          "Device mismatch: ", (tensor).device(), " vs ", (ref_tensor).device())

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {
  
/* Set default device based on availability of CUDA */
  extern torch::Device kDevice;
  extern torch::Dtype kType;
torch::Device select_torch_device();
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
void print_tensor_info(const torch::Tensor& tensor, const char *label="");

/**
 * Prints flatten nework tensor parameters.
 *
 * @param model The model to be inspected
 * @param N max parameters to be printed
 */
void inspect_network_parameters(torch::nn::Module& model, int64_t N = 5);

} /* namespace torch_compat */
} /* namespace piaabo */
} /* namespace cuwacunu */
