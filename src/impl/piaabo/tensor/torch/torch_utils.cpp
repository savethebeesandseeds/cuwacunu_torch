/* torch_utils.cpp */
#include "piaabo/tensor/torch/torch_utils.h"

#include <iomanip>
#include <vector>

namespace cuwacunu {
namespace piaabo {
namespace tensor {
namespace torch {

::torch::Device kDevice = select_torch_device();
::torch::Dtype kType = ::torch::kFloat32;

::torch::Device select_torch_device() {
  ::torch::Device aDev =
      ::torch::cuda::is_available() ? ::torch::kCUDA : ::torch::kCPU;
  CLEAR_SYS_ERR(); /* CUDA lefts a residual error message */

  return aDev;
}

std::string seed_torch_runtime(uint64_t seed) {
  ::torch::manual_seed(seed);
  if (::torch::cuda::is_available()) {
    ::torch::cuda::manual_seed_all(seed);
    CLEAR_SYS_ERR(); /* CUDA can leave a residual error message */
    return "torch_manual_seed_cuda_manual_seed_all";
  }
  return "torch_manual_seed_cpu";
}

void validate_module_parameters(const ::torch::nn::Module &model) {
  TORCH_CHECK(model.parameters().size() > 0,
              "There are zero Parameters in the model.");
  for (const auto &named_param : model.named_parameters()) {
    const auto &name = named_param.key();
    const auto &param = named_param.value();
    TORCH_CHECK(param.defined(), "Parameter '" + name + "' is undefined.");
    TORCH_CHECK(!::torch::isnan(param).any().item<bool>(),
                "Parameter '" + name + "' contains NaN.");
    TORCH_CHECK(param.numel() > 0, "Parameter '" + name + "' is empty.");
  }
}

void validate_tensor(const ::torch::Tensor &tensor, const char *label) {
  if (!tensor.defined()) {
    log_fatal("Found undefined tensor at: %s\n", label);
  }
  if (tensor.numel() == 0) {
    log_fatal("Found empty tensor at: %s\n", label);
  }
}

void assert_tensor_shape(const ::torch::Tensor &tensor, int64_t expected_size,
                         const char *label) {
  if (tensor.size(0) != expected_size) {
    log_fatal(
        "Found tensor with incorrect size at: %s, expected: %ld, found: %ld\n",
        label, expected_size, tensor.size(0));
  }
}

/**
 *  Recursively prints the values of a “small” tensor of arbitrary rank.
 *  We print through Float64 + stride arithmetic to avoid TensorIndex while
 *  keeping the debug path safe for all floating tensor dtypes.
 */
void print_tensor_values(const double *data, const std::vector<int64_t> &sizes,
                         const std::vector<int64_t> &strides,
                         std::vector<int64_t> &idx, int dim) {
  // If we're at the last dimension, emit a flat “[v0, v1, …]”
  if (dim == (int)sizes.size() - 1) {
    fprintf(LOG_FILE, "[");
    for (int64_t i = 0; i < sizes[dim]; ++i) {
      idx[dim] = i;
      // compute linear offset = sum(idx[d] * strides[d])
      int64_t offset = 0;
      for (size_t d = 0; d < sizes.size(); ++d)
        offset += idx[d] * strides[d];
      double v = data[offset];
      fprintf(LOG_FILE, "%f", v);
      if (i + 1 < sizes[dim])
        fprintf(LOG_FILE, ", ");
    }
    fprintf(LOG_FILE, "]");
  }
  // Otherwise, recurse one level deeper, wrapping each slice in “[ … ]”
  else {
    fprintf(LOG_FILE, "[");
    for (int64_t i = 0; i < sizes[dim]; ++i) {
      idx[dim] = i;
      print_tensor_values(data, sizes, strides, idx, dim + 1);
      if (i + 1 < sizes[dim])
        fprintf(LOG_FILE, ", ");
    }
    fprintf(LOG_FILE, "]");
  }
}

void print_tensor_info(const ::torch::Tensor &tensor, const char *label) {
  ::torch::NoGradGuard guard;        // never track this in autograd
  auto dtmp = tensor.detach().cpu(); // break the graph, move to CPU

  log_dbg("Tensor info - %s:\n", label);
  validate_tensor(dtmp, "print_tensor_info");

  // Print basic metadata
  log_dbg("\tTensor sizes: (");
  for (auto s : dtmp.sizes())
    fprintf(LOG_FILE, "%ld, ", s);
  fprintf(LOG_FILE, ")\n");
  log_dbg("\tData type: %s\n", dtmp.dtype().name().data());
  log_dbg("\tDevice: %s\n", dtmp.device().str().c_str());
  log_dbg("\tRequires gradient: %s\n", dtmp.requires_grad() ? "Yes" : "No");

  // only print values if small and floating point
  if (dtmp.numel() <= 25 && dtmp.is_floating_point()) {
    log_dbg("\tValues: ");

    // 1) Copy out to CPU and ensure contiguous layout
    auto tmp = dtmp.to(::torch::kFloat64);
    if (!tmp.device().is_cpu())
      tmp = tmp.cpu();
    tmp = tmp.contiguous();

    // 2) Extract sizes/strides & data pointer
    std::vector<int64_t> sizes(tmp.sizes().begin(), tmp.sizes().end()),
        strides(tmp.strides().begin(), tmp.strides().end());
    const double *data = tmp.data_ptr<double>();

    // 3) Prepare an index vector and recurse
    std::vector<int64_t> idx(tmp.dim(), 0);
    print_tensor_values(data, sizes, strides, idx, 0);

    fprintf(LOG_FILE, "\n");
  }
}

void inspect_network_parameters(::torch::nn::Module &model, int64_t N) {
  std::cout << "Parameters snapshot:\n";
  int64_t param_count = 0;
  for (const auto &param : model.parameters()) {
    // Print e.g. first param
    if (param_count++ < N) {
      auto p_data = param.flatten();
      double first_val = p_data[0].item<double>();
      std::cout << "Param " << param_count
                << " first val: " << std::setprecision(15) << first_val << "\n";
    } else
      break;
  }
}

} /* namespace torch */
} /* namespace tensor */
} /* namespace piaabo */
} /* namespace cuwacunu */
