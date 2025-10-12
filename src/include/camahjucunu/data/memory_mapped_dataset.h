/* memory_mapped_dataset.h */
#pragma once

#include <torch/torch.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstring>   // std::memcpy
#include <cstddef>   // std::byte, offsetof
#include <execution>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"
#include "piaabo/dconfig.h"
#include "piaabo/torch_compat/torch_utils.h"

#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/BNF/implementations/training_components/training_components.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

/* ============================================================
 *  Concatenated dataset grid policy (compile-time)
 *  - MIN => densest grid (smallest step across datasets)  [default]
 *  - MAX => coarsest grid (largest step across datasets)
 *  To override:
 *    -DCUWACUNU_CONCAT_GRID_STEP_POLICY=CUWACUNU_CONCAT_GRID_STEP_POLICY_MAX
 * ============================================================ */
#ifndef CUWACUNU_CONCAT_GRID_STEP_POLICY_MIN
#define CUWACUNU_CONCAT_GRID_STEP_POLICY_MIN 0
#endif
#ifndef CUWACUNU_CONCAT_GRID_STEP_POLICY_MAX
#define CUWACUNU_CONCAT_GRID_STEP_POLICY_MAX 1
#endif
#ifndef CUWACUNU_CONCAT_GRID_STEP_POLICY
#define CUWACUNU_CONCAT_GRID_STEP_POLICY CUWACUNU_CONCAT_GRID_STEP_POLICY_MIN
#endif

/* Floating-point alignment tolerance used for snapping to the grid */
#ifndef CUWACUNU_CONCAT_ALIGN_TOL
#define CUWACUNU_CONCAT_ALIGN_TOL 1e-9
#endif

namespace cuwacunu {
namespace camahjucunu {
namespace data {

/**
 * @brief Reads a specific value from a memory-mapped structure.
 *
 * @tparam T The struct type, which defines `key_type_t`.
 */
template <typename T>
typename T::key_type_t read_memory_value(const void* data_ptr, std::size_t index, std::size_t offset) {
  typename T::key_type_t value{};
  const std::byte* base   = static_cast<const std::byte*>(data_ptr);
  const std::byte* target = base + index * sizeof(T) + offset;
  std::memcpy(&value, target, sizeof(typename T::key_type_t));
  return value;
}

/**
 * @brief Reads a full record from the memory-mapped data.
 */
template <typename T>
T read_memory_struct(const void* data_ptr, std::size_t index) {
  T record{};
  const std::byte* base   = static_cast<const std::byte*>(data_ptr);
  const std::byte* target = base + index * sizeof(T);
  std::memcpy(&record, target, sizeof(T));
  return record;
}

/**
 * @brief Reads multiple records from the memory-mapped data.
 */
template <typename T>
std::vector<T> read_memory_structs(const void* data_ptr, std::size_t index, std::size_t count) {
  std::vector<T> records(count); // Allocate memory for `count` records.
  const std::byte* base   = static_cast<const std::byte*>(data_ptr);
  const std::byte* target = base + index * sizeof(T);
  std::memcpy(records.data(), target, count * sizeof(T));
  return records;
}

/*
 * Like absolute difference but stable for signed/unsigned integrals.
 */
template <typename T>
constexpr auto absolute_difference(T a, T b) {
  if constexpr (std::is_floating_point_v<T>) {
    return std::abs(a - b);
  } else {
    using U = std::make_unsigned_t<T>;
    return (a >= b) ? U(a - b) : U(b - a);
  }
}

/* ------------------------------------------------------------
 * Helpers for grid alignment and step counting (integral/float)
 * ------------------------------------------------------------ */
namespace detail {

template <typename K, typename = std::enable_if_t<std::is_integral_v<K>>>
inline K align_up_to_grid(K x, K step, K base) {
  // returns smallest y >= x s.t. (y - base) % step == 0
  if (step <= 0) return x;
  K diff = x - base;
  K r = diff % step;
  if (r < 0) r += step;       // make remainder positive
  if (r == 0) return x;
  return static_cast<K>(x + (step - r));
}

template <typename K, typename = std::enable_if_t<std::is_integral_v<K>>>
inline K align_down_to_grid(K x, K step, K base) {
  // returns largest y <= x s.t. (y - base) % step == 0
  if (step <= 0) return x;
  K diff = x - base;
  K r = diff % step;
  if (r < 0) r += step;       // make remainder positive
  return static_cast<K>(x - r);
}

template <typename K, typename = std::enable_if_t<std::is_integral_v<K>>>
inline std::size_t steps_between_inclusive(K left_aligned, K right_aligned, K step) {
  // pre: left_aligned and right_aligned are both on the grid, step > 0
  if (right_aligned < left_aligned) return 0;
  return static_cast<std::size_t>((right_aligned - left_aligned) / step) + 1;
}

template <typename K, typename = std::enable_if_t<std::is_floating_point_v<K>>>
inline K align_up_to_grid_fp(K x, K step, K base) {
  if (step <= K(0)) return x;
  const K eps = static_cast<K>(CUWACUNU_CONCAT_ALIGN_TOL);
  K q = (x - base) / step;
  K k = std::ceil(q - eps);
  return base + k * step;
}

template <typename K, typename = std::enable_if_t<std::is_floating_point_v<K>>>
inline K align_down_to_grid_fp(K x, K step, K base) {
  if (step <= K(0)) return x;
  const K eps = static_cast<K>(CUWACUNU_CONCAT_ALIGN_TOL);
  K q = (x - base) / step;
  K k = std::floor(q + eps);
  return base + k * step;
}

template <typename K, typename = std::enable_if_t<std::is_floating_point_v<K>>>
inline std::size_t steps_between_inclusive_fp(K left_aligned, K right_aligned, K step) {
  if (step <= K(0)) return 0;
  const K eps = static_cast<K>(CUWACUNU_CONCAT_ALIGN_TOL);
  if (right_aligned + eps < left_aligned) return 0;
  K k = std::floor(((right_aligned - left_aligned) / step) + eps);
  if (k < 0) return 0;
  return static_cast<std::size_t>(k) + 1;
}

} // namespace detail

/**
 * @brief A memory-mapped dataset for PyTorch-based data loading.
 *
 * Sliding-window semantics:
 *   - N_past_:   number of past frames returned (last row is time t)
 *   - N_future_: number of future frames returned (first row is t+1)
 *   - stride is 1 (anchor moves by one raw record)
 *
 * @tparam T The struct type of the records stored in the binary file.
 */
template <typename T>
class MemoryMappedDataset : public torch::data::datasets::Dataset<MemoryMappedDataset<T>, observation_sample_t> {
private:
  struct MappedData {
    int fd_;                 /* File descriptor for the binary file */
    void* data_ptr_;         /* Pointer to the memory-mapped data */
    std::size_t file_size_;  /* Size of the binary file in bytes */

    explicit MappedData(const std::string& bin_filename)
      : fd_(-1), data_ptr_(nullptr), file_size_(0) {

      fd_ = open(bin_filename.c_str(), O_RDONLY);
      if (fd_ == -1) {
        log_fatal("[MemoryMappedDataset] Error: Could not open binary file: %s, %s \n",
                  bin_filename.c_str(), std::strerror(errno));
      }

      struct stat st{};
      if (fstat(fd_, &st) == -1) {
        close(fd_);
        log_fatal("[MemoryMappedDataset] Error: Failed to determine file size for: %s, %s \n",
                  bin_filename.c_str(), std::strerror(errno));
      }
      file_size_ = static_cast<std::size_t>(st.st_size);

      if (file_size_ == 0) {
        close(fd_);
        log_fatal("[MemoryMappedDataset] Error: File is empty: %s\n", bin_filename.c_str());
      }

      data_ptr_ = mmap(nullptr, file_size_, PROT_READ, MAP_PRIVATE, fd_, 0);
      if (data_ptr_ == MAP_FAILED) {
        data_ptr_ = nullptr;
        close(fd_);
        log_fatal("[MemoryMappedDataset] Error: Failed to memory-map the file: %s, %s\n",
                  bin_filename.c_str(), std::strerror(errno));
      }
    }

    ~MappedData() {
      if (data_ptr_ && data_ptr_ != MAP_FAILED) { munmap(data_ptr_, file_size_); }
      if (fd_ != -1) { close(fd_); }
    }
  };

private:
  std::string bin_filename_;                 /* Path to the binary file */
  std::unique_ptr<MappedData> mapped_data_;  /* Unique pointer to memory-mapped data */
  std::size_t num_records_;                  /* Raw number of records (rows) in the dataset */

public:
  /* Variables for key-value boundaries. */
  std::size_t key_value_offset_;                /* Offset (in bytes) of the key in each record */
  typename T::key_type_t leftmost_key_value_;   /* Smallest key value in the dataset */
  typename T::key_type_t rightmost_key_value_;  /* Largest key value in the dataset */
  typename T::key_type_t key_value_span_;       /* rightmost - leftmost */
  typename T::key_type_t key_value_step_;       /* regular increment in key_value between consecutive records */

  /* Sliding-window configuration */
  std::size_t N_past_{1};
  std::size_t N_future_{1};
  std::size_t sliding_count_{0};               /* Number of sliding anchors (size()) */

public:
  explicit MemoryMappedDataset(const std::string& bin_filename,
                               std::size_t N_past = 1,
                               std::size_t N_future = 1)
      : bin_filename_(bin_filename),
        mapped_data_(std::make_unique<MappedData>(bin_filename)),
        num_records_(mapped_data_->file_size_ / sizeof(T)),
        key_value_offset_(T::key_offset()),
        N_past_(N_past),
        N_future_(N_future) {

    if (mapped_data_->file_size_ % sizeof(T) != 0) {
      log_fatal("[MemoryMappedDataset] Error: Binary file size is not a multiple of struct size. File: %s\n",
                bin_filename_.c_str());
    }
    if (num_records_ == 0) {
      log_fatal("[MemoryMappedDataset] Error: Binary Dataset is empty. File: %s\n", bin_filename_.c_str());
    }

    leftmost_key_value_  = read_memory_value<T>(mapped_data_->data_ptr_, 0,               key_value_offset_);
    rightmost_key_value_ = read_memory_value<T>(mapped_data_->data_ptr_, num_records_-1,  key_value_offset_);
    key_value_span_      = rightmost_key_value_ - leftmost_key_value_;

    if (!(leftmost_key_value_ < rightmost_key_value_)) {
      log_fatal("[MemoryMappedDataset] Error: Binary Dataset is not sorted correctly. File: %s\n",
                bin_filename_.c_str());
    }

    static_assert(std::is_same_v<decltype(std::declval<T>().tensor_features()), std::vector<double>>,
                  "[MemoryMappedDataset] Error: Template argument must provide tensor_features() returning std::vector<double>");
    static_assert(std::is_standard_layout<T>::value,
                  "[MemoryMappedDataset] Error: Template argument must be a standard layout type");
    static_assert(std::is_trivially_copyable<T>::value,
                  "[MemoryMappedDataset] Error: Template argument must be trivially copyable");

    // Infer regular step (warn on irregularities)
    typename T::key_type_t prev = read_memory_value<T>(mapped_data_->data_ptr_, 0, key_value_offset_);
    typename T::key_type_t curr = read_memory_value<T>(mapped_data_->data_ptr_, 1, key_value_offset_);
    key_value_step_ = curr - prev;

    if (key_value_step_ <= 0) {
      log_fatal("[MemoryMappedDataset] Error: negative or zero key_value_step_. File: %s.\n",
                bin_filename_.c_str());
    }

    for (std::size_t idx = 1; idx < num_records_; ++idx) {
      curr = read_memory_value<T>(mapped_data_->data_ptr_, idx, key_value_offset_);
      if (curr < prev) {
        log_fatal("[MemoryMappedDataset] Error: Binary Dataset is not sequential and increasing (not sorted). File: %s, on index: %zu\n",
                  bin_filename_.c_str(), idx);
      }
      if constexpr (std::is_floating_point_v<typename T::key_type_t>) {
        if (std::abs((curr - prev) - key_value_step_) > 1e-9) {
          log_warn("[MemoryMappedDataset] record on file [%s] irregular key delta at index [%zu]: (curr - prev): %f != step: %f\n",
                   bin_filename_.c_str(), idx,
                   static_cast<double>(curr - prev),
                   static_cast<double>(key_value_step_));
        }
      } else {
        if (((curr - prev) != key_value_step_)) {
          log_warn("[MemoryMappedDataset] record on file [%s] irregular key delta at index [%zu]: (curr - prev): %lld != step: %lld\n",
                   bin_filename_.c_str(), idx,
                   static_cast<long long>(curr - prev),
                   static_cast<long long>(key_value_step_));
        }
      }
      prev = curr;
    }

    // Compute sliding sample count (stride = 1)
    if (num_records_ >= (N_past_ + N_future_)) {
      sliding_count_ = num_records_ - (N_past_ + N_future_) + 1;
    } else {
      sliding_count_ = 0;
    }
  }

  /**
   * @brief Expose raw row count for external validation.
   */
  std::size_t raw_records() const noexcept { return num_records_; }

  /**
   * @brief Retrieves both past and future windows around a key value.
   * Current time = last of past window; future starts at t+1.
   */
  observation_sample_t get_sequences_around_key_value(
      typename T::key_type_t target_key_value,
      std::size_t N_past,
      std::size_t N_future)
  {
    std::size_t i = find_closest_index(target_key_value);

    // Bounds: need [i-(N_past-1) ... i] and [i+1 ... i+N_future]
    if (i + N_future >= num_records_) {
      throw std::out_of_range("[MemoryMappedDataset] Future window exceeds dataset size at key "
        + std::to_string(static_cast<long long>(target_key_value)));
    }
    if (i + 1 < N_past) {
      throw std::out_of_range("[MemoryMappedDataset] Past window exceeds dataset start at key "
        + std::to_string(static_cast<long long>(target_key_value)));
    }

    const std::size_t past_start = i - (N_past - 1);
    auto past_records = read_memory_structs<T>(mapped_data_->data_ptr_, past_start, N_past);
    const std::size_t D = past_records[0].tensor_features().size();

    torch::Tensor past_X   = torch::empty({ static_cast<long>(N_past),   static_cast<long>(D) }, torch::kFloat32);
    torch::Tensor past_msk = torch::empty({ static_cast<long>(N_past) },                          torch::kBool);
    {
      float* x = past_X.data_ptr<float>();
      bool*  m = past_msk.data_ptr<bool>();
      for (std::size_t k = 0; k < N_past; ++k) {
        const auto& v = past_records[k].tensor_features();
        std::copy(v.begin(), v.end(), x + k * D);
        m[k] = past_records[k].is_valid();
      }
    }

    const std::size_t fut_start = i + 1;
    auto fut_records = read_memory_structs<T>(mapped_data_->data_ptr_, fut_start, N_future);

    torch::Tensor fut_X   = torch::empty({ static_cast<long>(N_future), static_cast<long>(D) }, torch::kFloat32);
    torch::Tensor fut_msk = torch::empty({ static_cast<long>(N_future) },                       torch::kBool);
    {
      float* x = fut_X.data_ptr<float>();
      bool*  m = fut_msk.data_ptr<bool>();
      for (std::size_t k = 0; k < N_future; ++k) {
        const auto& v = fut_records[k].tensor_features();
        std::copy(v.begin(), v.end(), x + k * D);
        m[k] = fut_records[k].is_valid();
      }
    }

    return observation_sample_t{ past_X, past_msk, fut_X, fut_msk };
  }

  /**
   * @brief Sliding-window get: returns {past[N_past_], future[N_future_]}, stride = 1.
   * Anchor a = (N_past_-1) + index.
   */
  observation_sample_t get(std::size_t index) override {
    if (index >= sliding_count_) {
      log_fatal("[MemoryMappedDataset] Index [%zu] out of range [0, %zu) on file %s\n",
                index, sliding_count_, bin_filename_.c_str());
    }

    const std::size_t a          = (N_past_ > 0 ? (N_past_ - 1) : 0) + index; // anchor (time t)
    const std::size_t past_start = (N_past_ > 0 ? (a - (N_past_ - 1)) : a);
    const std::size_t fut_start  = a + 1;

    auto past_records = read_memory_structs<T>(mapped_data_->data_ptr_, past_start, N_past_);
    auto fut_records  = read_memory_structs<T>(mapped_data_->data_ptr_, fut_start,  N_future_);

    const std::size_t D = past_records[0].tensor_features().size();

    torch::Tensor past_X   = torch::empty({ static_cast<long>(N_past_),   static_cast<long>(D) }, torch::kFloat32);
    torch::Tensor past_msk = torch::empty({ static_cast<long>(N_past_) },                          torch::kBool);
    {
      float* x = past_X.data_ptr<float>();
      bool*  m = past_msk.data_ptr<bool>();
      for (std::size_t k = 0; k < N_past_; ++k) {
        const auto& v = past_records[k].tensor_features();
        std::copy(v.begin(), v.end(), x + k * D);
        m[k] = past_records[k].is_valid();
      }
    }

    torch::Tensor fut_X   = torch::empty({ static_cast<long>(N_future_), static_cast<long>(D) }, torch::kFloat32);
    torch::Tensor fut_msk = torch::empty({ static_cast<long>(N_future_) },                       torch::kBool);
    {
      float* x = fut_X.data_ptr<float>();
      bool*  m = fut_msk.data_ptr<bool>();
      for (std::size_t k = 0; k < N_future_; ++k) {
        const auto& v = fut_records[k].tensor_features();
        std::copy(v.begin(), v.end(), x + k * D);
        m[k] = fut_records[k].is_valid();
      }
    }

    return observation_sample_t{ past_X, past_msk, fut_X, fut_msk };
  }

  /* size() is the number of sliding anchors */
  torch::optional<std::size_t> size() const override { return sliding_count_; }

  /* Common samplers */
  torch::data::samplers::SequentialSampler SequentialSampler() const {
    return torch::data::samplers::SequentialSampler(sliding_count_);
  }
  torch::data::DataLoaderOptions SequentialSampler_options(std::size_t batch_size = 64, std::size_t workers = 4) const {
    return torch::data::DataLoaderOptions().batch_size(batch_size).workers(workers);
  }
  torch::data::samplers::RandomSampler RandomSampler() const {
    return torch::data::samplers::RandomSampler(sliding_count_);
  }
  torch::data::DataLoaderOptions RandomSampler_options(std::size_t batch_size = 64, std::size_t workers = 4) const {
    return torch::data::DataLoaderOptions().batch_size(batch_size).workers(workers);
  }

  /**
   * @brief Finds the closest index for a given key value using a safe interpolation strategy.
   * Returns the last index whose key <= target_key_value.
   */
  std::size_t find_closest_index(typename T::key_type_t target_key_value) {
    if (num_records_ == 0) {
      log_fatal("[MemoryMappedDataset] Error: Dataset is empty: %s\n", bin_filename_.c_str());
    }
    if (target_key_value <= leftmost_key_value_) return 0;
    if (target_key_value >= rightmost_key_value_) return num_records_ - 1;

    std::size_t left = 0;
    std::size_t right = num_records_ - 1;
    std::size_t best_index = 0;
    using key_t  = typename T::key_type_t;
    using diff_t = decltype(absolute_difference(std::declval<key_t>(), std::declval<key_t>()));
    diff_t best_diff = std::numeric_limits<diff_t>::max();
    auto left_key_value  = leftmost_key_value_;
    auto right_key_value = rightmost_key_value_;

    while (left <= right) {
      if (left_key_value == right_key_value) break;

      const double num = static_cast<double>(target_key_value - left_key_value);
      const double den = static_cast<double>(right_key_value  - left_key_value);
      double r = num / den;
      if (r < 0.0) r = 0.0;
      if (r > 1.0) r = 1.0;

      std::size_t mid = left + static_cast<std::size_t>(r * (right - left));
      if (mid >= num_records_) mid = num_records_ - 1;

      auto mid_key_value = read_memory_value<T>(mapped_data_->data_ptr_, mid, key_value_offset_);

      if (mid_key_value <= target_key_value) {
        const auto diff = absolute_difference(mid_key_value, target_key_value);
        if (diff < best_diff) {
          best_diff = diff;
          best_index = mid;
        }
        if (mid == right) break;
        left = mid + 1;
        left_key_value = read_memory_value<T>(mapped_data_->data_ptr_, left, key_value_offset_);
      } else {
        if (mid == 0) break;
        right = mid - 1;
        right_key_value = read_memory_value<T>(mapped_data_->data_ptr_, right, key_value_offset_);
      }
    }
    return best_index;
  }
};

/**
 * @brief A memory-mapped concatenated dataset for efficient data access across multiple sources.
 *
 * Each source can specify (N_past, N_future). The dataset pads to (max_N_past, max_N_future).
 * Sampling domain is the intersection of valid target positions across all sources.
 *
 * The global sampling grid (step/anchors) is chosen by compile-time policy:
 *   - MIN: densest grid (smallest step across datasets)
 *   - MAX: coarsest grid (largest step across datasets)
 * The final left/right bounds are aligned to the chosen grid to avoid drift.
 */
template <typename T>
class MemoryMappedConcatDataset : public torch::data::datasets::Dataset<MemoryMappedConcatDataset<T>, observation_sample_t> {
private:
  std::vector<std::shared_ptr<MemoryMappedDataset<T>>> datasets_;
  std::vector<std::string> file_names_;
  std::vector<std::size_t> N_past_;
  std::vector<std::size_t> N_future_;

  // Cache of per-dataset valid range (in key space) after (N_past, N_future)
  std::vector<typename T::key_type_t> valid_left_;   // leftmost key where a target index is valid
  std::vector<typename T::key_type_t> valid_right_;  // rightmost key where a target index is valid

  // Index of dataset that defines the global grid (based on policy)
  std::size_t grid_ref_idx_{static_cast<std::size_t>(-1)};

public:
  std::size_t max_N_past_{0};
  std::size_t max_N_future_{0};

  std::size_t num_records_{0};                           /* number of valid target positions across intersection (on the chosen grid) */
  typename T::key_type_t leftmost_key_value_{};          /* intersection lower bound aligned to grid */
  typename T::key_type_t rightmost_key_value_{};         /* intersection upper bound aligned to grid */
  typename T::key_type_t key_value_span_{};              /* rightmost - leftmost */
  typename T::key_type_t key_value_step_{};              /* global step used by the concatenated dataset (per policy) */

public:
  MemoryMappedConcatDataset() = default;

  observation_sample_t get(std::size_t index) override {
    if (index >= num_records_) {
      log_fatal("[MemoryMappedConcatDataset] get() request, index: %zu, exceeds size: %zu \n",
                index, num_records_);
    }
    const auto target_key_value =
      leftmost_key_value_ + static_cast<typename T::key_type_t>(index) * key_value_step_;
    return get_by_key_value(target_key_value);
  }

  torch::optional<std::size_t> size() const override { return num_records_; }

  /* Common samplers */
  torch::data::samplers::SequentialSampler SequentialSampler() const {
    return torch::data::samplers::SequentialSampler(num_records_);
  }
  torch::data::DataLoaderOptions SequentialSampler_options(std::size_t batch_size = 64, std::size_t workers = 4) const {
    return torch::data::DataLoaderOptions().batch_size(batch_size).workers(workers);
  }
  torch::data::samplers::RandomSampler RandomSampler() const {
    return torch::data::samplers::RandomSampler(num_records_);
  }
  torch::data::DataLoaderOptions RandomSampler_options(std::size_t batch_size = 64, std::size_t workers = 4) const {
    return torch::data::DataLoaderOptions().batch_size(batch_size).workers(workers);
  }

  /**
   * @brief Retrieve stacked + padded windows by key across sources.
   * - Past is left-padded to max_N_past_.
   * - Future is right-padded to max_N_future_.
   */
  observation_sample_t get_by_key_value(typename T::key_type_t target_key_value) {
    const size_t K = datasets_.size();
    std::vector<torch::Tensor> feats(K), masks(K), fut_feats(K), fut_masks(K);

    for (size_t i = 0; i < K; ++i) {
      auto& d  = datasets_[i];
      const auto np = N_past_[i];
      const auto nf = N_future_[i];

      auto s = d->get_sequences_around_key_value(target_key_value, np, nf);

      // pad past at the front (so last row is time t)
      if (np < max_N_past_) {
        const int64_t pad = static_cast<int64_t>(max_N_past_ - np);
        feats[i] = torch::cat({ torch::zeros({pad, s.features.size(1)}, s.features.options()), s.features }, 0);
        masks[i] = torch::cat({ torch::zeros({pad}, s.mask.options()), s.mask }, 0);
      } else { feats[i] = s.features; masks[i] = s.mask; }

      // pad future at the end (so first row is t+1)
      if (nf < max_N_future_) {
        const int64_t pad = static_cast<int64_t>(max_N_future_ - nf);
        fut_feats[i] = torch::cat({ s.future_features, torch::zeros({pad, s.future_features.size(1)}, s.future_features.options()) }, 0);
        fut_masks[i] = torch::cat({ s.future_mask,     torch::zeros({pad}, s.future_mask.options()) }, 0);
      } else { fut_feats[i] = s.future_features; fut_masks[i] = s.future_mask; }
    }

    return observation_sample_t{
      torch::stack(feats, 0),      // [K, max_N_past,   D]
      torch::stack(masks, 0),      // [K, max_N_past]
      torch::stack(fut_feats, 0),  // [K, max_N_future, D]
      torch::stack(fut_masks, 0)   // [K, max_N_future]
    };
  }

  /**
   * @brief Adds a dataset (CSV → bin) with per-source (N_past, N_future) and updates intersection domain.
   */
  void add_dataset(const std::string csv_filename,
                   std::size_t N_past, std::size_t N_future,
                   std::size_t normalization_window = 0,
                   bool force_binarization = false,
                   size_t buffer_size = 1024,
                   char delimiter = ',') {

    /* --- prepare the file: CSV → binary --- */
    std::string bin_filename = sanitize_csv_into_binary_file<T>(
      csv_filename, normalization_window, force_binarization, buffer_size, delimiter);

    /* --- add dataset --- */
    file_names_.push_back(bin_filename);
    // Note: concat uses key-addressed reads; we keep per-dataset N_past/N_future only for sliding get().
    // Here we construct with defaults (1,1); this is fine because we never call d->get() from concat.
    datasets_.push_back(std::make_shared<MemoryMappedDataset<T>>(file_names_.back()));
    N_past_.push_back(N_past);
    N_future_.push_back(N_future);

    /* --- validations: sizes and duplicates --- */
    auto& d = datasets_.back();
    // Validate against RAW record count (not sliding size)
    if (d->raw_records() < (N_past + N_future)) {
      log_fatal("[MemoryMappedConcatDataset](add_dataset) Dataset %s too small: rows:%zu < N_past+N_future:%zu\n",
                csv_filename.c_str(), d->raw_records(), (N_past + N_future));
    }

    if (!(
          (datasets_.size() == file_names_.size()) &&
          (datasets_.size() == N_past_.size()) &&
          (datasets_.size() == N_future_.size())
         )) {
      log_fatal("[MemoryMappedConcatDataset](add_dataset) Internal container sizes mismatch.\n");
    }

    if (std::unordered_set<std::string>(file_names_.begin(), file_names_.end()).size() != file_names_.size()) {
      log_fatal("[MemoryMappedConcatDataset](add_dataset) Duplicated csv/bin file found on add_dataset: %s\n",
                csv_filename.c_str());
    }

    /* --- extend valid range caches to keep them aligned with datasets_ --- */
    valid_left_.resize(datasets_.size());
    valid_right_.resize(datasets_.size());

    /* --- recompute global state after adding this dataset --- */
    recompute_global_state_();
  }

private:
  void recompute_global_state_() {
    using key_t = typename T::key_type_t;

    const std::size_t K = datasets_.size();
    if (K == 0) {
      max_N_past_ = max_N_future_ = num_records_ = 0;
      return;
    }

    /* 1) maxima of (N_past, N_future) for padding */
    max_N_past_   = *std::max_element(N_past_.begin(),   N_past_.end());
    max_N_future_ = *std::max_element(N_future_.begin(), N_future_.end());

    /* 2) compute per-dataset valid ranges in key space
          valid_left  = leftmost + (N_past-1) * step
          valid_right = rightmost -  N_future   * step     */
    key_t inter_left{};
    key_t inter_right{};
    for (std::size_t i = 0; i < K; ++i) {
      auto& d = datasets_[i];
      const auto np = N_past_[i];
      const auto nf = N_future_[i];

      const key_t vleft  = d->leftmost_key_value_  + static_cast<key_t>( (np > 0 ? (np - 1) : 0) ) * d->key_value_step_;
      const key_t vright = d->rightmost_key_value_ - static_cast<key_t>( nf ) * d->key_value_step_;

      if (!(vleft < vright)) {
        log_fatal("[MemoryMappedConcatDataset] Empty per-dataset valid range after (N_past,N_future) for dataset %zu\n", i);
      }

      valid_left_[i]  = vleft;
      valid_right_[i] = vright;

      if (i == 0) {
        inter_left  = vleft;
        inter_right = vright;
      } else {
        inter_left  = std::max(inter_left,  vleft);
        inter_right = std::min(inter_right, vright);
      }
    }

    if (!(inter_left < inter_right)) {
      log_fatal("[MemoryMappedConcatDataset] Empty intersection across datasets after applying (N_past,N_future)\n");
    }

    /* 3) choose grid step and reference dataset per policy */
#if (CUWACUNU_CONCAT_GRID_STEP_POLICY == CUWACUNU_CONCAT_GRID_STEP_POLICY_MIN)
    {
      std::size_t sel_idx = 0;
      key_t sel_step = datasets_[0]->key_value_step_;
      for (std::size_t i = 1; i < K; ++i) {
        const key_t st = datasets_[i]->key_value_step_;
        if (st < sel_step) { sel_step = st; sel_idx = i; }
      }
      key_value_step_ = sel_step;
      grid_ref_idx_   = sel_idx;
    }
#elif (CUWACUNU_CONCAT_GRID_STEP_POLICY == CUWACUNU_CONCAT_GRID_STEP_POLICY_MAX)
    {
      std::size_t sel_idx = 0;
      key_t sel_step = datasets_[0]->key_value_step_;
      for (std::size_t i = 1; i < K; ++i) {
        const key_t st = datasets_[i]->key_value_step_;
        if (st > sel_step) { sel_step = st; sel_idx = i; }
      }
      key_value_step_ = sel_step;
      grid_ref_idx_   = sel_idx;
    }
#else
# error "Unknown CUWACUNU_CONCAT_GRID_STEP_POLICY"
#endif

    /* 4) align the intersection [inter_left, inter_right] to the chosen grid.
          Use the reference dataset's valid_left as the base for congruence. */
    const key_t base = valid_left_[grid_ref_idx_];

    if constexpr (std::is_integral_v<key_t>) {
      leftmost_key_value_  = detail::align_up_to_grid<key_t>(inter_left,  key_value_step_, base);
      if (!(leftmost_key_value_ <= inter_right)) {
        log_fatal("[MemoryMappedConcatDataset] Aligned left bound exceeds intersection right bound.");
      }
      rightmost_key_value_ = detail::align_down_to_grid<key_t>(inter_right, key_value_step_, base);

      if (!(leftmost_key_value_ <= rightmost_key_value_)) {
        log_fatal("[MemoryMappedConcatDataset] Empty grid after alignment (integral keys).");
      }

      num_records_ = detail::steps_between_inclusive<key_t>(leftmost_key_value_, rightmost_key_value_, key_value_step_);
      key_value_span_ = rightmost_key_value_ - leftmost_key_value_;
    } else {
      leftmost_key_value_  = detail::align_up_to_grid_fp<key_t>(inter_left,  key_value_step_, base);
      if (!(leftmost_key_value_ <= inter_right)) {
        log_fatal("[MemoryMappedConcatDataset] Aligned left bound exceeds intersection right bound (float).");
      }
      rightmost_key_value_ = detail::align_down_to_grid_fp<key_t>(inter_right, key_value_step_, base);

      if (!(leftmost_key_value_ <= rightmost_key_value_)) {
        log_fatal("[MemoryMappedConcatDataset] Empty grid after alignment (floating keys).");
      }

      num_records_ = detail::steps_between_inclusive_fp<key_t>(leftmost_key_value_, rightmost_key_value_, key_value_step_);
      key_value_span_ = rightmost_key_value_ - leftmost_key_value_;
    }

    if (num_records_ == 0) {
      log_fatal("[MemoryMappedConcatDataset] No records after alignment to global grid.");
    }
  }
};

/**
  * @brief Construct a new MemoryMappedConcatDataset from an observation_instruction_t.
  *        Supports both past and future sequence lengths.
  *
  * @tparam Td The underlying data type used by the dataset.
  * @param instrument the dataset is tied to an instrument.
  * @param obs_inst The observation pipeline instruction.
  * @param force_binarization Whether to renormalize and binarize the memory mapped data files.
  */
template<typename Td>
MemoryMappedConcatDataset<Td> create_memory_mapped_concat_dataset(
  std::string& instrument,
  cuwacunu::camahjucunu::observation_instruction_t obs_inst,
  bool force_binarization = false) {

  char delimiter = ',';
  size_t buffer_size = 1024;

  MemoryMappedConcatDataset<Td> concat;

  for (auto& in_form : obs_inst.input_forms) {
    if (in_form.active == "true") {
      for (auto& instr_form : obs_inst.filter_instrument_forms(instrument, in_form.record_type, in_form.interval)) {

        const std::size_t N_past   = std::stoul(in_form.seq_length);
        const std::size_t N_future = std::stoul(in_form.future_seq_length);

        concat.add_dataset(
          /* csv file */                 instr_form.source,
          /* N_past */                   N_past,
          /* N_future */                 N_future,
          /* normalization_window */     std::stoul(instr_form.norm_window),
          /* force_binarization */       force_binarization,
          /* buffer_size */              buffer_size,
          /* delimiter */                delimiter
        );
      }
    }
  }

  return concat;
}

} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
