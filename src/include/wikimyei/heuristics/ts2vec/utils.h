/* utils.h */
#pragma once

#include <torch/torch.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <random>
#include <filesystem>

namespace cuwacunu {
namespace wikimyei {
namespace ts2vec {

// ---------------------------
// Padding Operations
// ---------------------------

// Pad tensor with NaNs on left and right
inline torch::Tensor torch_pad_nan(torch::Tensor arr, int64_t left = 0, int64_t right = 0, int64_t dim = 0) {
    if (left > 0) {
        auto pad_shape = arr.sizes().vec();
        pad_shape[dim] = left;
        auto left_pad = torch::full(pad_shape, std::numeric_limits<double>::quiet_NaN(), arr.options());
        arr = torch::cat({left_pad, arr}, dim);
    }
    if (right > 0) {
        auto pad_shape = arr.sizes().vec();
        pad_shape[dim] = right;
        auto right_pad = torch::full(pad_shape, std::numeric_limits<double>::quiet_NaN(), arr.options());
        arr = torch::cat({arr, right_pad}, dim);
    }
    return arr;
}

// Pad numpy-like arrays to a specific length with NaNs (using tensors)
inline torch::Tensor pad_nan_to_target(torch::Tensor array, int64_t target_length, int64_t axis = 0, bool both_side = false) {
    int64_t pad_size = target_length - array.size(axis);
    if (pad_size <= 0)
        return array;

    std::vector<int64_t> pad(array.dim() * 2, 0);
    if (both_side) {
        pad[2 * (array.dim() - axis - 1) + 1] = pad_size / 2;
        pad[2 * (array.dim() - axis - 1)] = pad_size - pad_size / 2;
    } else {
        pad[2 * (array.dim() - axis - 1)] = pad_size;
    }

    return torch::constant_pad_nd(array, pad, std::numeric_limits<double>::quiet_NaN());
}

// ---------------------------
// Splitting Operations
// ---------------------------

// Split tensor into sections and pad each with NaNs to match lengths
inline torch::Tensor split_with_nan(torch::Tensor x, int sections, int64_t axis = 0) {
    auto arrs = x.tensor_split(sections, axis);
    int64_t target_length = arrs[0].size(axis);

    for (auto& arr : arrs) {
        arr = pad_nan_to_target(arr, target_length, axis);
    }

    // Concatenate along batch dimension (assuming batch dimension = 0)
    return torch::cat(arrs, /*dim=*/0);
}
// ---------------------------
// Tensor Selection
// ---------------------------

// Select segments per row based on indices
inline torch::Tensor take_per_row(const torch::Tensor& A, const torch::Tensor& indices, int64_t num_elem) {
    auto range = torch::arange(num_elem, indices.options());
    auto all_indices = indices.unsqueeze(-1) + range;
    auto rows = torch::arange(all_indices.size(0), indices.options()).unsqueeze(-1).expand_as(all_indices);
    return A.index({rows, all_indices});
}

// ---------------------------
// Centering Series
// ---------------------------

// Center series with varying lengths
inline torch::Tensor centerize_vary_length_series(torch::Tensor x) {
    auto isnan = torch::isnan(x).all(-1);
    auto prefix_zeros = (~isnan).to(torch::kInt64).argmax(1);
    auto suffix_zeros = (~isnan.flip(1)).to(torch::kInt64).argmax(1);
    auto offset = ((prefix_zeros + suffix_zeros) / 2 - prefix_zeros).unsqueeze(1);

    auto indices = torch::arange(x.size(1), x.options()).unsqueeze(0).repeat({x.size(0), 1}) - offset;
    indices = indices.clamp(0, x.size(1) - 1);

    auto rows = torch::arange(x.size(0), x.options()).unsqueeze(1).expand_as(indices);
    return x.index({rows, indices});
}

// ---------------------------
// Data Augmentation
// ---------------------------

// Dropout elements randomly and replace with NaNs
inline torch::Tensor data_dropout(torch::Tensor arr, double p) {
    auto mask = torch::rand(arr.sizes(), arr.options()) < p;
    auto res = arr.clone();
    res.masked_fill_(mask, std::numeric_limits<double>::quiet_NaN());
    return res;
}

// ---------------------------
// Utility: Naming with Datetime
// ---------------------------

inline std::string name_with_datetime(const std::string& prefix = "default") {
    auto now = std::chrono::system_clock::now();
    auto itt = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << prefix << "_" << std::put_time(std::localtime(&itt), "%Y%m%d_%H%M%S");
    return oss.str();
}

// ---------------------------
// Initialize DL Program
// ---------------------------

inline torch::Device init_dl_program(
    const std::string& device_name = "cuda",
    int seed = -1,
    bool use_cudnn = true,
    bool deterministic = false,
    bool benchmark = false,
    bool use_tf32 = false,
    int max_threads = -1
) {
    if (max_threads > 0)
        torch::set_num_threads(max_threads);

    if (seed >= 0) {
        torch::manual_seed(seed);
        std::srand(seed);
    }

    torch::Device device(device_name);
    if (device.is_cuda()) {
        torch::DeviceGuard device_guard(device);
        if (seed >= 0) {
            torch::cuda::manual_seed(seed + 1);
        }
    }

    torch::globalContext().setBenchmarkCuDNN(benchmark);
    torch::globalContext().setDeterministicCuDNN(deterministic);
    torch::globalContext().setAllowTF32CuDNN(use_tf32);
    torch::globalContext().setAllowTF32CuBLAS(use_tf32);

    return device;
}

} // namespace ts2vec
} // namespace wikimyei
} // namespace cuwacunu
