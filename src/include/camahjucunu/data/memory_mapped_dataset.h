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

#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"

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
#ifndef CUWACUNU_CONCAT_ALIGN_REL_TOL
#define CUWACUNU_CONCAT_ALIGN_REL_TOL 1e-12
#endif

namespace cuwacunu {
namespace camahjucunu {
namespace data {

/**
 * @brief Reads a specific value from a memory-mapped structure.
 *
 * @tparam Datatype_t The struct type, which defines `key_type_t`.
 */
template <typename Datatype_t>
typename Datatype_t::key_type_t read_memory_value(const void* data_ptr, std::size_t index, std::size_t offset) {
  typename Datatype_t::key_type_t value{};
  const std::byte* base   = static_cast<const std::byte*>(data_ptr);
  const std::byte* target = base + index * sizeof(Datatype_t) + offset;
  std::memcpy(&value, target, sizeof(typename Datatype_t::key_type_t));
  return value;
}

/**
 * @brief Reads a full record from the memory-mapped data.
 */
template <typename Datatype_t>
Datatype_t read_memory_struct(const void* data_ptr, std::size_t index) {
  Datatype_t record{};
  const std::byte* base   = static_cast<const std::byte*>(data_ptr);
  const std::byte* target = base + index * sizeof(Datatype_t);
  std::memcpy(&record, target, sizeof(Datatype_t));
  return record;
}

/**
 * @brief Reads multiple records from the memory-mapped data.
 */
template <typename Datatype_t>
std::vector<Datatype_t> read_memory_structs(const void* data_ptr, std::size_t index, std::size_t count) {
  if (count == 0) return {};
  std::vector<Datatype_t> records(count); // Allocate memory for `count` records.
  const std::byte* base   = static_cast<const std::byte*>(data_ptr);
  const std::byte* target = base + index * sizeof(Datatype_t);
  std::memcpy(records.data(), target, count * sizeof(Datatype_t));
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
inline K effective_fp_tol(K a, K b) {
  const K abs_tol = static_cast<K>(CUWACUNU_CONCAT_ALIGN_TOL);
  const K rel_tol = static_cast<K>(CUWACUNU_CONCAT_ALIGN_REL_TOL);
  const K scale = std::max(K(1), std::max(std::fabs(a), std::fabs(b)));
  return abs_tol + rel_tol * scale;
}

template <typename K, typename = std::enable_if_t<std::is_floating_point_v<K>>>
inline bool fp_less_with_tol(K a, K b) {
  return (a + effective_fp_tol(a, b)) < b;
}

template <typename K, typename = std::enable_if_t<std::is_floating_point_v<K>>>
inline bool fp_le_with_tol(K a, K b) {
  return !fp_less_with_tol<K>(b, a);
}

template <typename K, typename = std::enable_if_t<std::is_floating_point_v<K>>>
inline K align_up_to_grid_fp(K x, K step, K base) {
  if (step <= K(0)) return x;
  const K q = (x - base) / step;
  const K eps = effective_fp_tol<K>(q, K(0));
  K k = std::ceil(q - eps);
  return base + k * step;
}

template <typename K, typename = std::enable_if_t<std::is_floating_point_v<K>>>
inline K align_down_to_grid_fp(K x, K step, K base) {
  if (step <= K(0)) return x;
  const K q = (x - base) / step;
  const K eps = effective_fp_tol<K>(q, K(0));
  K k = std::floor(q + eps);
  return base + k * step;
}

template <typename K, typename = std::enable_if_t<std::is_floating_point_v<K>>>
inline std::size_t steps_between_inclusive_fp(K left_aligned, K right_aligned, K step) {
  if (step <= K(0)) return 0;
  if (fp_less_with_tol<K>(right_aligned, left_aligned)) return 0;
  const K span = (right_aligned - left_aligned) / step;
  const K eps = effective_fp_tol<K>(span, K(0));
  K k = std::floor(span + eps);
  if (k < 0) return 0;
  return static_cast<std::size_t>(k) + 1;
}

template<typename T>
inline constexpr const char* record_type_name_for_datatype() {
  if constexpr (std::is_same_v<T, cuwacunu::camahjucunu::exchange::kline_t>) {
    return "kline";
  } else if constexpr (std::is_same_v<T, cuwacunu::camahjucunu::exchange::trade_t>) {
    return "trade";
  } else if constexpr (std::is_same_v<T, cuwacunu::camahjucunu::exchange::basic_t>) {
    return "basic";
  } else {
    return "";
  }
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
 * @tparam Datatype_t The struct type of the records stored in the binary file.
 */
template <typename Datatype_t>
class MemoryMappedDataset : public torch::data::datasets::Dataset<MemoryMappedDataset<Datatype_t>, observation_sample_t> {
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
  typename Datatype_t::key_type_t leftmost_key_value_;   /* Smallest key value in the dataset */
  typename Datatype_t::key_type_t rightmost_key_value_;  /* Largest key value in the dataset */
  typename Datatype_t::key_type_t key_value_span_;       /* rightmost - leftmost */
  typename Datatype_t::key_type_t key_value_step_;       /* regular increment in key_value between consecutive records */

  /* Sliding-window configuration */
  std::size_t N_past_{1};
  std::size_t N_future_{1};
  std::size_t sliding_count_{0};               /* Number of sliding anchors (size()) */

private:
  // Build a 1D tensor of keys from a mutable vector (Datatype_t::key_value() is non-const).
  static inline torch::Tensor keys_from_records_1d(std::vector<Datatype_t>& recs) {
    using KeyT = typename Datatype_t::key_type_t;
    const long n = static_cast<long>(recs.size());
    if constexpr (std::is_integral_v<KeyT>) {
      if (n == 0) return torch::empty({0}, torch::TensorOptions().dtype(torch::kInt64));
    } else {
      if (n == 0) return torch::empty({0}, torch::TensorOptions().dtype(torch::kFloat64));
    }
    if constexpr (std::is_integral_v<KeyT>) {
      std::vector<long long> v(recs.size());
      for (size_t i = 0; i < recs.size(); ++i) v[i] = static_cast<long long>(recs[i].key_value());
      return torch::from_blob(v.data(), { n }, torch::TensorOptions().dtype(torch::kInt64)).clone();
    } else {
      std::vector<double> v(recs.size());
      for (size_t i = 0; i < recs.size(); ++i) v[i] = static_cast<double>(recs[i].key_value());
      return torch::from_blob(v.data(), { n }, torch::TensorOptions().dtype(torch::kFloat64)).clone();
    }
  }

public:
  explicit MemoryMappedDataset(const std::string& bin_filename,
                               std::size_t N_past = 1,
                               std::size_t N_future = 1)
      : bin_filename_(bin_filename),
        mapped_data_(std::make_unique<MappedData>(bin_filename)),
        num_records_(mapped_data_->file_size_ / sizeof(Datatype_t)),
        key_value_offset_(Datatype_t::key_offset()),
        N_past_(N_past),
        N_future_(N_future) {

    if (mapped_data_->file_size_ % sizeof(Datatype_t) != 0) {
      log_fatal("[MemoryMappedDataset] Error: Binary file size is not a multiple of struct size. File: %s\n",
                bin_filename_.c_str());
    }
    if (num_records_ == 0) {
      log_fatal("[MemoryMappedDataset] Error: Binary Dataset is empty. File: %s\n", bin_filename_.c_str());
    }
    if (N_past_ == 0) {
      log_fatal("[MemoryMappedDataset] Error: N_past must be >= 1. File: %s\n", bin_filename_.c_str());
    }

    leftmost_key_value_  = read_memory_value<Datatype_t>(mapped_data_->data_ptr_, 0,               key_value_offset_);
    rightmost_key_value_ = read_memory_value<Datatype_t>(mapped_data_->data_ptr_, num_records_-1,  key_value_offset_);
    key_value_span_      = rightmost_key_value_ - leftmost_key_value_;

    if (num_records_ > 1 && !(leftmost_key_value_ < rightmost_key_value_)) {
      log_fatal("[MemoryMappedDataset] Error: Binary Dataset is not sorted correctly. File: %s\n",
                bin_filename_.c_str());
    }

    static_assert(std::is_same_v<decltype(std::declval<Datatype_t>().tensor_features()), std::vector<double>>,
                  "[MemoryMappedDataset] Error: Template argument must provide tensor_features() returning std::vector<double>");
    static_assert(std::is_standard_layout<Datatype_t>::value,
                  "[MemoryMappedDataset] Error: Template argument must be a standard layout type");
    static_assert(std::is_trivially_copyable<Datatype_t>::value,
                  "[MemoryMappedDataset] Error: Template argument must be trivially copyable");

    if (num_records_ == 1) {
      if constexpr (std::is_integral_v<typename Datatype_t::key_type_t>) {
        key_value_step_ = static_cast<typename Datatype_t::key_type_t>(1);
      } else {
        key_value_step_ = static_cast<typename Datatype_t::key_type_t>(1.0);
      }
    } else {
      // Infer regular step (warn on irregularities)
      typename Datatype_t::key_type_t prev = read_memory_value<Datatype_t>(mapped_data_->data_ptr_, 0, key_value_offset_);
      typename Datatype_t::key_type_t curr = read_memory_value<Datatype_t>(mapped_data_->data_ptr_, 1, key_value_offset_);
      key_value_step_ = curr - prev;

      if (key_value_step_ <= 0) {
        log_fatal("[MemoryMappedDataset] Error: negative or zero key_value_step_. File: %s.\n",
                  bin_filename_.c_str());
      }

      for (std::size_t idx = 1; idx < num_records_; ++idx) {
        curr = read_memory_value<Datatype_t>(mapped_data_->data_ptr_, idx, key_value_offset_);
        if (curr < prev) {
          log_fatal("[MemoryMappedDataset] Error: Binary Dataset is not sequential and increasing (not sorted). File: %s, on index: %zu\n",
                    bin_filename_.c_str(), idx);
        }
        if constexpr (std::is_floating_point_v<typename Datatype_t::key_type_t>) {
          const auto delta = curr - prev;
          const auto tol = detail::effective_fp_tol<decltype(delta)>(delta, key_value_step_);
          if (std::abs(delta - key_value_step_) > tol) {
            log_warn("[MemoryMappedDataset] record on file [%s] irregular key delta at index [%zu]: (curr - prev): %f != step: %f\n",
                     bin_filename_.c_str(), idx,
                     static_cast<double>(delta),
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
   * (C) fills past_keys/future_keys.
   */
  observation_sample_t get_sequences_around_key_value(
      typename Datatype_t::key_type_t target_key_value,
      std::size_t N_past,
      std::size_t N_future)
  {
    if (N_past == 0) {
      throw std::invalid_argument("[MemoryMappedDataset] N_past must be >= 1 in get_sequences_around_key_value");
    }
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
    auto past_records = read_memory_structs<Datatype_t>(mapped_data_->data_ptr_, past_start, N_past);
    if (past_records.empty()) {
      throw std::runtime_error("[MemoryMappedDataset] Empty past window in get_sequences_around_key_value");
    }
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
    torch::Tensor past_keys = keys_from_records_1d(past_records);

    const std::size_t fut_start = i + 1;
    auto fut_records = read_memory_structs<Datatype_t>(mapped_data_->data_ptr_, fut_start, N_future);

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
    torch::Tensor fut_keys = keys_from_records_1d(fut_records);

    observation_sample_t s{ past_X, past_msk, fut_X, fut_msk, torch::Tensor() };
    s.past_keys   = past_keys;
    s.future_keys = fut_keys;
    s.normalized  = false; // raw by default
    return s;
  }

  /**
   * @brief Sliding-window get: returns {past[N_past_], future[N_future_]}, stride = 1.
   * (C) fills past_keys/future_keys.
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

    auto past_records = read_memory_structs<Datatype_t>(mapped_data_->data_ptr_, past_start, N_past_);
    auto fut_records  = read_memory_structs<Datatype_t>(mapped_data_->data_ptr_, fut_start,  N_future_);

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
    torch::Tensor past_keys = keys_from_records_1d(past_records);

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
    torch::Tensor fut_keys = keys_from_records_1d(fut_records);

    observation_sample_t s{ past_X, past_msk, fut_X, fut_msk, torch::Tensor() };
    s.past_keys   = past_keys;
    s.future_keys = fut_keys;
    s.normalized  = false;
    return s;
  }

  /* size() is the number of sliding anchors */
  torch::optional<std::size_t> size() const override { return sliding_count_; }

  /* Common samplers */
  torch::data::samplers::SequentialSampler SequentialSampler() const {
    return torch::data::samplers::SequentialSampler(sliding_count_);
  }
  torch::data::DataLoaderOptions SequentialSampler_options(std::size_t batch_size = 64, std::size_t workers = 0) const {
    return torch::data::DataLoaderOptions().batch_size(batch_size).workers(workers);
  }
  torch::data::samplers::RandomSampler RandomSampler() const {
    return torch::data::samplers::RandomSampler(sliding_count_);
  }
  torch::data::DataLoaderOptions RandomSampler_options(std::size_t batch_size = 64, std::size_t workers = 0) const {
    return torch::data::DataLoaderOptions().batch_size(batch_size).workers(workers);
  }

  /**
   * @brief Finds the closest index for a given key value using a safe interpolation strategy.
   * Returns the last index whose key <= target_key_value.
   */
  std::size_t find_closest_index(typename Datatype_t::key_type_t target_key_value) {
    if (num_records_ == 0) {
      log_fatal("[MemoryMappedDataset] Error: Dataset is empty: %s\n", bin_filename_.c_str());
    }
    if (target_key_value <= leftmost_key_value_) return 0;
    if (target_key_value >= rightmost_key_value_) return num_records_ - 1;

    std::size_t left = 0;
    std::size_t right = num_records_ - 1;
    std::size_t best_index = 0;
    using key_t  = typename Datatype_t::key_type_t;
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

      auto mid_key_value = read_memory_value<Datatype_t>(mapped_data_->data_ptr_, mid, key_value_offset_);

      if (mid_key_value <= target_key_value) {
        const auto diff = absolute_difference(mid_key_value, target_key_value);
        if (diff < best_diff) {
          best_diff = diff;
          best_index = mid;
        }
        if (mid == right) break;
        left = mid + 1;
        left_key_value = read_memory_value<Datatype_t>(mapped_data_->data_ptr_, left, key_value_offset_);
      } else {
        if (mid == 0) break;
        right = mid - 1;
        right_key_value = read_memory_value<Datatype_t>(mapped_data_->data_ptr_, right, key_value_offset_);
      }
    }
    return best_index;
  }

  /**
   * ====================== (A) time-range slicing ==========================
   * Return sliding samples whose anchor key is within [key_left, key_right].
   * No clamping/padding beyond the natural grid. Vector<observation_sample_t>.
   */
  std::vector<observation_sample_t>
  range_samples_by_keys(typename Datatype_t::key_type_t key_left,
                        typename Datatype_t::key_type_t key_right)
  {
    using key_t = typename Datatype_t::key_type_t;
    std::vector<observation_sample_t> out;
    if (num_records_ == 0 || sliding_count_ == 0) return out;
    if (key_right < key_left) std::swap(key_left, key_right);

    // compute raw anchor bounds [a_min, a_max] such that key[a] in [left, right]
    // first raw index >= left
    std::size_t idx_left = find_closest_index(key_left);
    key_t key_at_left = read_memory_value<Datatype_t>(mapped_data_->data_ptr_, idx_left, key_value_offset_);
    if (key_at_left < key_left && idx_left + 1 < num_records_) ++idx_left;

    // last raw index <= right
    std::size_t idx_right = find_closest_index(key_right);
    key_t key_at_right = read_memory_value<Datatype_t>(mapped_data_->data_ptr_, idx_right, key_value_offset_);
    if (key_at_right > key_right && idx_right > 0) --idx_right;

    // translate to valid anchor range respecting past/future windows
    const std::size_t a_min_natural = (N_past_ > 0 ? (N_past_ - 1) : 0);
    const std::size_t a_max_natural = num_records_ - 1 - N_future_;
    if (idx_left > a_max_natural || idx_right < a_min_natural || idx_left > idx_right) {
      return out; // empty
    }
    std::size_t a_min = std::max(idx_left, a_min_natural);
    std::size_t a_max = std::min(idx_right, a_max_natural);
    if (a_min > a_max) return out;

    // iterate natural anchors and re-use get()
    out.reserve(a_max - a_min + 1);
    for (std::size_t a = a_min; a <= a_max; ++a) {
      const std::size_t sliding_idx = a - (N_past_ > 0 ? (N_past_ - 1) : 0);
      out.emplace_back( get(sliding_idx) );
    }
    return out;
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
template <typename Datatype_t>
class MemoryMappedConcatDataset : public torch::data::datasets::Dataset<MemoryMappedConcatDataset<Datatype_t>, observation_sample_t> {
private:
  std::vector<std::shared_ptr<MemoryMappedDataset<Datatype_t>>> datasets_;
  std::vector<std::string> file_names_;
  std::vector<std::size_t> N_past_;
  std::vector<std::size_t> N_future_;

  // Cache of per-dataset valid range (in key space) after (N_past, N_future)
  std::vector<typename Datatype_t::key_type_t> valid_left_;   // leftmost key where a target index is valid
  std::vector<typename Datatype_t::key_type_t> valid_right_;  // rightmost key where a target index is valid

  // Index of dataset that defines the global grid (based on policy)
  std::size_t grid_ref_idx_{static_cast<std::size_t>(-1)};

public:
  std::size_t max_N_past_{0};
  std::size_t max_N_future_{0};

  std::size_t num_records_{0};                           /* number of valid target positions across intersection (on the chosen grid) */
  typename Datatype_t::key_type_t leftmost_key_value_{};          /* intersection lower bound aligned to grid */
  typename Datatype_t::key_type_t rightmost_key_value_{};         /* intersection upper bound aligned to grid */
  typename Datatype_t::key_type_t key_value_span_{};              /* rightmost - leftmost */
  typename Datatype_t::key_type_t key_value_step_{};              /* global step used by the concatenated dataset (per policy) */

public:
  MemoryMappedConcatDataset() = default;

  observation_sample_t get(std::size_t index) override {
    if (index >= num_records_) {
      log_fatal("[MemoryMappedConcatDataset] get() request, index: %zu, exceeds size: %zu \n",
                index, num_records_);
    }
    const auto target_key_value =
      leftmost_key_value_ + static_cast<typename Datatype_t::key_type_t>(index) * key_value_step_;
    return get_by_key_value(target_key_value);
  }

  torch::optional<std::size_t> size() const override { return num_records_; }

  /* Common samplers */
  torch::data::samplers::SequentialSampler SequentialSampler() const {
    return torch::data::samplers::SequentialSampler(num_records_);
  }
  torch::data::DataLoaderOptions SequentialSampler_options(std::size_t batch_size = 64, std::size_t workers = 0) const {
    return torch::data::DataLoaderOptions().batch_size(batch_size).workers(workers);
  }
  torch::data::samplers::RandomSampler RandomSampler() const {
    return torch::data::samplers::RandomSampler(num_records_);
  }
  torch::data::DataLoaderOptions RandomSampler_options(std::size_t batch_size = 64, std::size_t workers = 0) const {
    return torch::data::DataLoaderOptions().batch_size(batch_size).workers(workers);
  }

  /**
   * @brief Compute concatenated sliding index range for keys within [key_left, key_right].
   *        Returns false when the range is empty.
   */
  bool compute_index_range_by_keys(typename Datatype_t::key_type_t key_left,
                                   typename Datatype_t::key_type_t key_right,
                                   std::size_t* out_begin_idx,
                                   std::size_t* out_count) const {
    using key_t = typename Datatype_t::key_type_t;
    auto set_empty = [&](bool ret = false) {
      if (out_begin_idx) *out_begin_idx = 0;
      if (out_count) *out_count = 0;
      return ret;
    };

    if (num_records_ == 0) return set_empty(false);
    if (key_right < key_left) std::swap(key_left, key_right);

    // Clamp to intersection domain first.
    if (key_right < leftmost_key_value_ || key_left > rightmost_key_value_) {
      return set_empty(false);
    }

    key_t left = key_left < leftmost_key_value_ ? leftmost_key_value_ : key_left;
    key_t right = key_right > rightmost_key_value_ ? rightmost_key_value_ : key_right;
    const key_t base = valid_left_[grid_ref_idx_];

    std::size_t begin_idx = 0;
    std::size_t count = 0;
    if constexpr (std::is_integral_v<key_t>) {
      const key_t left_aligned = detail::align_up_to_grid<key_t>(left, key_value_step_, base);
      if (!(left_aligned <= right)) return set_empty(false);
      const key_t right_aligned = detail::align_down_to_grid<key_t>(right, key_value_step_, base);
      if (!(left_aligned <= right_aligned)) return set_empty(false);
      count = detail::steps_between_inclusive<key_t>(left_aligned, right_aligned, key_value_step_);
      begin_idx = static_cast<std::size_t>((left_aligned - leftmost_key_value_) / key_value_step_);
    } else {
      const key_t left_aligned = detail::align_up_to_grid_fp<key_t>(left, key_value_step_, base);
      if (detail::fp_less_with_tol<key_t>(right, left_aligned)) return set_empty(false);
      const key_t right_aligned = detail::align_down_to_grid_fp<key_t>(right, key_value_step_, base);
      if (detail::fp_less_with_tol<key_t>(right_aligned, left_aligned)) return set_empty(false);
      count = detail::steps_between_inclusive_fp<key_t>(left_aligned, right_aligned, key_value_step_);
      const double j0d = static_cast<double>((left_aligned - leftmost_key_value_) / key_value_step_);
      begin_idx = static_cast<std::size_t>(std::llround(j0d));
    }

    if (begin_idx >= num_records_ || count == 0) return set_empty(false);
    if (count > num_records_ - begin_idx) count = num_records_ - begin_idx;
    if (count == 0) return set_empty(false);

    if (out_begin_idx) *out_begin_idx = begin_idx;
    if (out_count) *out_count = count;
    return true;
  }

  /**
   * @brief Retrieve stacked + padded windows by key across sources.
   * - Past is left-padded to max_N_past_.
   * - Future is right-padded to max_N_future_.
   * (C) stacks past_keys/future_keys across channels.
   */
  observation_sample_t get_by_key_value(typename Datatype_t::key_type_t target_key_value) {
    const size_t K = datasets_.size();
    std::vector<torch::Tensor> feats(K), masks(K), fut_feats(K), fut_masks(K);
    std::vector<torch::Tensor> keys_past(K), keys_future(K);
    int64_t expected_D = -1;

    // key tensor dtype to use for padding
    auto key_opts = torch::TensorOptions().dtype(
        std::is_integral_v<typename Datatype_t::key_type_t> ? torch::kInt64 : torch::kFloat64);

    for (size_t i = 0; i < K; ++i) {
      auto& d  = datasets_[i];
      const auto np = N_past_[i];
      const auto nf = N_future_[i];

      auto s = d->get_sequences_around_key_value(target_key_value, np, nf);
      const int64_t past_D = (s.features.defined() && s.features.dim() >= 2) ? s.features.size(1) : -1;
      const int64_t fut_D  = (s.future_features.defined() && s.future_features.dim() >= 2) ? s.future_features.size(1) : -1;
      if (expected_D < 0) {
        expected_D = (past_D >= 0) ? past_D : fut_D;
      }
      if (past_D >= 0 && past_D != expected_D) {
        log_fatal("[MemoryMappedConcatDataset] Feature dimension mismatch across datasets: expected D=%lld got D=%lld on channel %zu\n",
                  static_cast<long long>(expected_D),
                  static_cast<long long>(past_D),
                  i);
      }
      if (fut_D >= 0 && fut_D != expected_D) {
        log_fatal("[MemoryMappedConcatDataset] Future feature dimension mismatch across datasets: expected D=%lld got D=%lld on channel %zu\n",
                  static_cast<long long>(expected_D),
                  static_cast<long long>(fut_D),
                  i);
      }

      // pad past at the front (so last row is time t)
      if (np < max_N_past_) {
        const int64_t pad = static_cast<int64_t>(max_N_past_ - np);
        feats[i] = torch::cat({ torch::zeros({pad, s.features.size(1)}, s.features.options()), s.features }, 0);
        masks[i] = torch::cat({ torch::zeros({pad}, s.mask.options()), s.mask }, 0);
        keys_past[i] = torch::cat({ torch::zeros({pad}, key_opts), s.past_keys }, 0);
      } else { feats[i] = s.features; masks[i] = s.mask; keys_past[i] = s.past_keys; }

      // pad future at the end (so first row is t+1)
      if (nf < max_N_future_) {
        const int64_t pad = static_cast<int64_t>(max_N_future_ - nf);
        fut_feats[i] = torch::cat({ s.future_features, torch::zeros({pad, s.future_features.size(1)}, s.future_features.options()) }, 0);
        fut_masks[i] = torch::cat({ s.future_mask,     torch::zeros({pad}, s.future_mask.options()) }, 0);
        keys_future[i] = torch::cat({ s.future_keys, torch::zeros({pad}, key_opts) }, 0);
      } else { fut_feats[i] = s.future_features; fut_masks[i] = s.future_mask; keys_future[i] = s.future_keys; }
    }

    observation_sample_t out{
      torch::stack(feats, 0),      // [K, max_N_past,   D]
      torch::stack(masks, 0),      // [K, max_N_past]
      torch::stack(fut_feats, 0),  // [K, max_N_future, D]
      torch::stack(fut_masks, 0),  // [K, max_N_future]
      torch::Tensor()              // encoding
    };
    // (C) keys
    out.past_keys   = torch::stack(keys_past, 0);   // [K,max_N_past]
    out.future_keys = torch::stack(keys_future, 0); // [K,max_N_future]
    out.normalized  = false; // raw by default
    return out;
  }

  /**
   * ====================== (A) time-range slicing ==========================
   * Return sliding samples whose anchor key is within [key_left, key_right].
   * Uses the concatenated dataset grid (already regular).
   */
  std::vector<observation_sample_t>
  range_samples_by_keys(typename Datatype_t::key_type_t key_left,
                        typename Datatype_t::key_type_t key_right)
  {
    std::vector<observation_sample_t> out;
    std::size_t begin_idx = 0;
    std::size_t count = 0;
    if (!compute_index_range_by_keys(key_left, key_right, &begin_idx, &count)) return out;
    out.reserve(count);
    for (std::size_t j = 0; j < count; ++j) out.emplace_back(get(begin_idx + j));
    return out;
  }

  /**
   * @brief Adds a dataset (CSV → bin) with per-source (N_past, N_future) and updates intersection domain.
   */
  void add_dataset(const std::string csv_filename,
                   std::size_t N_past, std::size_t N_future,
                   std::size_t normalization_window = 0,
                   bool force_rebuild_cache = false,
                   size_t buffer_size = 1024,
                   char delimiter = ',') {
    if (N_past == 0) {
      log_fatal("[MemoryMappedConcatDataset](add_dataset) N_past must be >= 1 for %s\n",
                csv_filename.c_str());
    }

    /* --- prepare the file: CSV → binary --- */
    std::string bin_filename = sanitize_csv_into_binary_file<Datatype_t>(
      csv_filename, normalization_window, force_rebuild_cache, buffer_size, delimiter);

    /* --- add dataset --- */
    file_names_.push_back(bin_filename);
    datasets_.push_back(std::make_shared<MemoryMappedDataset<Datatype_t>>(file_names_.back()));
    N_past_.push_back(N_past);
    N_future_.push_back(N_future);

    /* --- validations: sizes and duplicates --- */
    auto& d = datasets_.back();
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
    using key_t = typename Datatype_t::key_type_t;

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

      if (vright < vleft) {
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

    if (inter_right < inter_left) {
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
  * @brief Construct a new MemoryMappedConcatDataset from an observation_spec_t.
  *        Supports both past and future sequence lengths.
  */
template<typename Datatype_t>
MemoryMappedConcatDataset<Datatype_t> create_memory_mapped_concat_dataset(
  const std::string& instrument,
  cuwacunu::camahjucunu::observation_spec_t obs_inst,
  bool force_rebuild_cache = false) {

  char delimiter = ',';
  size_t buffer_size = 1024;

  MemoryMappedConcatDataset<Datatype_t> concat;
  const std::string expected_record_type = detail::record_type_name_for_datatype<Datatype_t>();
  if (expected_record_type.empty()) {
    log_fatal("[create_memory_mapped_concat_dataset] Unsupported Datatype_t for observation spec record_type matching.\n");
  }
  std::size_t matched_sources = 0;

  for (auto& in_form : obs_inst.channel_forms) {
    if (in_form.active == "true") {
      if (in_form.record_type != expected_record_type) {
        log_warn("[create_memory_mapped_concat_dataset] Skipping active input_form with record_type=%s for Datatype_t=%s\n",
                 in_form.record_type.c_str(), expected_record_type.c_str());
        continue;
      }
      for (auto& instr_form : obs_inst.filter_source_forms(instrument, in_form.record_type, in_form.interval)) {

        const std::size_t N_past   = std::stoul(in_form.seq_length);
        const std::size_t N_future = std::stoul(in_form.future_seq_length);
        std::size_t normalization_window = 0;
        try {
          normalization_window = in_form.norm_window.empty() ? 0 : std::stoul(in_form.norm_window);
        } catch (...) {
          normalization_window = 0;
        }
        if (N_past == 0) {
          log_fatal("[create_memory_mapped_concat_dataset] Invalid seq_length=0 for interval=%s, record_type=%s\n",
                    cuwacunu::camahjucunu::exchange::enum_to_string(in_form.interval).c_str(),
                    in_form.record_type.c_str());
        }

        concat.add_dataset(
          /* csv file */                 instr_form.source,
          /* N_past */                   N_past,
          /* N_future */                 N_future,
          /* normalization_window */     normalization_window,
          /* force_rebuild_cache */      force_rebuild_cache,
          /* buffer_size */              buffer_size,
          /* delimiter */                delimiter
        );
        ++matched_sources;
      }
    }
  }

  if (matched_sources == 0) {
    log_fatal("[create_memory_mapped_concat_dataset] No datasets matched instrument=%s and Datatype_t record_type=%s\n",
              instrument.c_str(), expected_record_type.c_str());
  }

  return concat;
}

} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
