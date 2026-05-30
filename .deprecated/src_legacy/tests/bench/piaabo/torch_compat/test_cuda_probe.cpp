#include <cuda_runtime.h>
#include <torch/torch.h>

#include <iostream>
#include <sstream>
#include <string>

namespace {

const char* yesno(bool value) {
  return value ? "yes" : "no";
}

std::string format_cuda_version(int version) {
  if (version <= 0) return "unknown";
  std::ostringstream oss;
  oss << (version / 1000) << "." << ((version % 1000) / 10);
  return oss.str();
}

void print_cuda_error(const char* label, cudaError_t err) {
  std::cout << label
            << ": code=" << static_cast<int>(err)
            << " message=" << cudaGetErrorString(err)
            << "\n";
}

}  // namespace

int main() {
  std::cout << "[cuda_probe] begin\n";

  int driver_version = 0;
  const cudaError_t driver_err = cudaDriverGetVersion(&driver_version);
  std::cout << "[cuda_probe] driver_version=" << format_cuda_version(driver_version)
            << " raw=" << driver_version
            << " ok=" << yesno(driver_err == cudaSuccess)
            << "\n";
  if (driver_err != cudaSuccess) {
    print_cuda_error("[cuda_probe] driver_query_error", driver_err);
  }

  int runtime_version = 0;
  const cudaError_t runtime_err = cudaRuntimeGetVersion(&runtime_version);
  std::cout << "[cuda_probe] runtime_version=" << format_cuda_version(runtime_version)
            << " raw=" << runtime_version
            << " ok=" << yesno(runtime_err == cudaSuccess)
            << "\n";
  if (runtime_err != cudaSuccess) {
    print_cuda_error("[cuda_probe] runtime_query_error", runtime_err);
  }

  int device_count = -1;
  const cudaError_t count_err = cudaGetDeviceCount(&device_count);
  std::cout << "[cuda_probe] cudaGetDeviceCount ok="
            << yesno(count_err == cudaSuccess)
            << " device_count=" << device_count
            << "\n";
  if (count_err != cudaSuccess) {
    print_cuda_error("[cuda_probe] device_count_error", count_err);
  }

  if (count_err == cudaSuccess && device_count > 0) {
    for (int i = 0; i < device_count; ++i) {
      cudaDeviceProp prop{};
      const cudaError_t prop_err = cudaGetDeviceProperties(&prop, i);
      if (prop_err != cudaSuccess) {
        std::cout << "[cuda_probe] device[" << i << "] properties_ok=no\n";
        print_cuda_error("[cuda_probe] device_properties_error", prop_err);
        continue;
      }
      std::cout << "[cuda_probe] device[" << i << "]"
                << " name=" << prop.name
                << " cc=" << prop.major << "." << prop.minor
                << " total_global_mem_mb=" << (prop.totalGlobalMem / (1024 * 1024))
                << "\n";
    }
  }

  bool torch_cuda_available = false;
  std::size_t torch_device_count = 0;
  std::string torch_probe_error{};
  try {
    torch_cuda_available = torch::cuda::is_available();
    if (torch_cuda_available) {
      torch_device_count = torch::cuda::device_count();
    }
  } catch (const c10::Error& e) {
    torch_probe_error = e.what_without_backtrace();
  } catch (const std::exception& e) {
    torch_probe_error = e.what();
  }

  std::cout << "[cuda_probe] torch::cuda::is_available="
            << yesno(torch_cuda_available)
            << " torch_device_count=" << torch_device_count
            << "\n";
  if (!torch_probe_error.empty()) {
    std::cout << "[cuda_probe] torch_probe_error=" << torch_probe_error << "\n";
  }

  bool torch_tensor_probe_ok = false;
  std::string tensor_probe_error{};
  if (torch_cuda_available) {
    try {
      auto probe = torch::zeros(
          {1},
          torch::TensorOptions().device(torch::kCUDA).dtype(torch::kFloat32));
      torch::cuda::synchronize();
      torch_tensor_probe_ok = probe.is_cuda();
    } catch (const c10::Error& e) {
      tensor_probe_error = e.what_without_backtrace();
    } catch (const std::exception& e) {
      tensor_probe_error = e.what();
    }
  } else {
    tensor_probe_error = "torch::cuda::is_available() returned false";
  }

  std::cout << "[cuda_probe] torch_tensor_probe_ok="
            << yesno(torch_tensor_probe_ok)
            << "\n";
  if (!tensor_probe_error.empty()) {
    std::cout << "[cuda_probe] torch_tensor_probe_error=" << tensor_probe_error
              << "\n";
  }

  const bool raw_cuda_ok = (count_err == cudaSuccess) && (device_count > 0);
  const bool overall_ok =
      raw_cuda_ok && torch_cuda_available && torch_tensor_probe_ok;
  std::cout << "[cuda_probe] summary raw_cuda_ok=" << yesno(raw_cuda_ok)
            << " torch_cuda_ok=" << yesno(torch_cuda_available)
            << " tensor_probe_ok=" << yesno(torch_tensor_probe_ok)
            << " overall_ok=" << yesno(overall_ok)
            << "\n";

  return overall_ok ? 0 : 1;
}
