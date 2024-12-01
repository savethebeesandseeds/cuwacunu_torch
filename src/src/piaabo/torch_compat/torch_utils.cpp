#include "piaabo/torch_compat/torch_utils.h"

RUNTIME_WARNING("(torch_utils.cpp)[] #FIXME be aware to also seed the random number generator for libtorch.\n");
RUNTIME_WARNING("(torch_utils.cpp)[] #FIXME change floats to double. \n");

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {
  
  torch::Device kDevice = select_torch_device();
  torch::Dtype kType = torch::kFloat32;

  torch::Device select_torch_device() {
    torch::Device aDev = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
    CLEAR_SYS_ERR(); /* CUDA lefts a residual error message */

    return aDev;
  }
  void validate_module_parameters(const torch::nn::Module& model) {
    TORCH_CHECK(model.parameters().size() > 0, "There are zero Parameters in the model.");
    for (const auto& named_param : model.named_parameters()) {
      const auto& name = named_param.key();
      const auto& param = named_param.value();
      TORCH_CHECK(param.defined(), "Parameter '" + name + "' is undefined.");
      TORCH_CHECK(!torch::isnan(param).any().item<bool>(), "Parameter '" + name + "' contains NaN.");
      TORCH_CHECK(param.numel() > 0, "Parameter '" + name + "' is empty.");
    }
  }

  void validate_tensor(const torch::Tensor& tensor, const char* label) {
    if(!tensor.defined()) {
      log_fatal("Found undefined tensor at: %s\n", label);
    }
    if(tensor.numel() == 0) {
      log_fatal("Found empty tensor at: %s\n", label);
    }
  }

  void assert_tensor_shape(const torch::Tensor& tensor, int64_t expected_size, const char* label) {
    if (tensor.size(0) != expected_size) {
      log_fatal("Found tensor with incorrect size at: %s, expected: %ld, found: %ld\n", label, expected_size, tensor.size(0));
    }
  }

  void print_tensor_info(const torch::Tensor& tensor) {
    log_info("Tensor info: \n");
    
    /* Validate tensor */
    validate_tensor(tensor, "print_tensor_info");

    /* Print tensor sizes */
    log_info("\tTensor sizes: (");
    for (const auto& s : tensor.sizes()) {
      fprintf(LOG_FILE, "%ld, ", s);
    }
    fprintf(LOG_FILE, ")\n");

    /* Print tensor data type */
    log_info("\tData type: %s\n", tensor.dtype().name().data());

    /* Print the device the tensor is on */
    log_info("\tDevice: %s\n", tensor.device().str().c_str());

    /* Print if the tensor requires gradient */
    log_info("\tRequires gradient: %s\n", tensor.requires_grad() ? "Yes" : "No");

    /* Optionally print tensor values if it is small enough */
    if (tensor.numel() <= 10) {  /* Adjust this number based on what you consider 'small' */
      log_info("\tValues: [");
      for (int i = 0; i < tensor.numel(); ++i) {
        /* Assuming tensor can be safely accessed and is a 1D tensor for simplicity */
        fprintf(LOG_FILE, "%f, ", tensor[i].item<float>());
      }
      fprintf(LOG_FILE, "]\n");
    }
  }

} /* namespace torch_compat */
} /* namespace piaabo */
} /* namespace cuwacunu */
