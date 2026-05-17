#include "piaabo/tensor/torch/config_adapter.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

#include "piaabo/log/dlogs.h"

namespace cuwacunu {
namespace piaabo {
namespace tensor {
namespace torch {
namespace {

std::string trim_lower_ascii(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
    ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
    --e;
  s = s.substr(b, e - b);
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

std::string strip_torch_type_prefixes(std::string v) {
  if (v.rfind("torch::", 0) == 0)
    v = v.substr(7);
  if (v.rfind("at::", 0) == 0)
    v = v.substr(4);
  if (v.rfind("k", 0) == 0 && v.size() > 1 &&
      std::isalpha(static_cast<unsigned char>(v[1]))) {
    v = v.substr(1);
  }
  return v;
}

bool requests_cuda_device(const std::string &s) {
  std::string v = trim_lower_ascii(s);
  if (v == "gpu")
    return true;
  if (v.rfind("gpu:", 0) == 0)
    return true;
  v = strip_torch_type_prefixes(v);
  return v == "cuda" || v.rfind("cuda:", 0) == 0;
}

::torch::Dtype parse_dtype(const std::string &s) {
  std::string v = strip_torch_type_prefixes(trim_lower_ascii(s));

  if (v == "bool")
    return ::torch::kBool;
  if (v == "int8")
    return ::torch::kInt8;
  if (v == "int16")
    return ::torch::kInt16;
  if (v == "int32")
    return ::torch::kInt32;
  if (v == "int64")
    return ::torch::kInt64;
  if (v == "float16" || v == "half" || v == "f16")
    return ::torch::kFloat16;
  if (v == "float32" || v == "float" || v == "f32")
    return ::torch::kFloat32;
  if (v == "float64" || v == "double" || v == "f64")
    return ::torch::kFloat64;

  throw std::runtime_error("Unknown configured dtype '" + s + "'");
}

::torch::Device parse_device(const std::string &s) {
  std::string v = trim_lower_ascii(s);
  if (v == "gpu")
    v = "cuda";

  if (v.rfind("gpu:", 0) == 0) {
    v = "cuda:" + v.substr(4);
  }

  v = strip_torch_type_prefixes(v);
  const auto log_cpu_fallback_loud = [&](const std::string &requested,
                                         const std::string &attempted,
                                         const char *reason,
                                         const std::string &detail =
                                             std::string()) {
    if (detail.empty()) {
      log_warn(
          "[torch_utils][CPU_FALLBACK_ACTIVE] requested='%s' attempted='%s' "
          "reason='%s'. Heavy compute will run on CPU for this component.\n",
          requested.c_str(), attempted.c_str(), reason);
      return;
    }
    log_warn("[torch_utils][CPU_FALLBACK_ACTIVE] requested='%s' attempted='%s' "
             "reason='%s' detail='%s'. Heavy compute will run on CPU for this "
             "component.\n",
             requested.c_str(), attempted.c_str(), reason, detail.c_str());
  };
  const auto fallback_cpu_with_warn =
      [&](const ::torch::Device &attempted, const char *reason,
          const std::string &detail = std::string()) {
        log_cpu_fallback_loud(s, attempted.str(), reason, detail);
        return ::torch::Device(::torch::kCPU);
      };
  const auto ensure_device_runtime_usable =
      [&](const ::torch::Device &dev) -> ::torch::Device {
    if (!dev.is_cuda())
      return dev;
    try {
      auto probe = ::torch::zeros(
          {1}, ::torch::TensorOptions().dtype(::torch::kFloat32).device(dev));
      (void)probe;
      ::torch::cuda::synchronize();
      return dev;
    } catch (const c10::Error &e) {
      return fallback_cpu_with_warn(dev, "cuda runtime probe failed",
                                    e.what_without_backtrace());
    } catch (const std::exception &e) {
      return fallback_cpu_with_warn(dev, "cuda runtime probe failed", e.what());
    }
  };

  if (v == "cpu")
    return ::torch::Device(::torch::kCPU);
  if (v == "cuda") {
    if (!::torch::cuda::is_available()) {
      log_cpu_fallback_loud(s, "cuda", "cuda unavailable");
      return ::torch::Device(::torch::kCPU);
    }
    return ensure_device_runtime_usable(::torch::Device(::torch::kCUDA));
  }
  if (v.rfind("cuda:", 0) == 0 && !::torch::cuda::is_available()) {
    log_cpu_fallback_loud(s, v, "cuda unavailable");
    return ::torch::Device(::torch::kCPU);
  }

  try {
    const ::torch::Device parsed(v);
    return ensure_device_runtime_usable(parsed);
  } catch (const c10::Error &) {
    throw std::runtime_error("Invalid configured device '" + s + "'");
  }
}

} // namespace

::torch::Dtype
config_dtype(const cuwacunu::iitepi::contract_hash_t &contract_hash,
             const std::string &section) {
  if (section == "GENERAL") {
    return parse_dtype(
        cuwacunu::iitepi::config_space_t::get<std::string>("GENERAL", "dtype"));
  }
  return parse_dtype(
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)
          ->get<std::string>(section, "dtype"));
}

::torch::Device
config_device(const cuwacunu::iitepi::contract_hash_t &contract_hash,
              const std::string &section) {
  const std::string configured_device =
      (section == "GENERAL")
          ? cuwacunu::iitepi::config_space_t::get<std::string>("GENERAL",
                                                               "device")
          : cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)
                ->get<std::string>(section, "device");
  const ::torch::Device resolved_device = parse_device(configured_device);
  if (resolved_device.is_cpu() && requests_cuda_device(configured_device)) {
    log_warn(
        "[torch_utils][CUDA_REQUESTED_BUT_CPU_SELECTED] contract_hash='%s' "
        "section='%s' configured_device='%s' resolved_device='%s'.\n",
        contract_hash.c_str(), section.c_str(), configured_device.c_str(),
        resolved_device.str().c_str());
    log_warn(
        "[torch_utils][CUDA_REQUESTED_BUT_CPU_SELECTED] CUDA was explicitly "
        "requested for this component, but execution will proceed on CPU. "
        "Inspect "
        "earlier [torch_utils][CPU_FALLBACK_ACTIVE] warnings for the "
        "runtime-probe "
        "failure details.\n");
  }
  return resolved_device;
}

} // namespace torch
} // namespace tensor
} // namespace piaabo
} // namespace cuwacunu
