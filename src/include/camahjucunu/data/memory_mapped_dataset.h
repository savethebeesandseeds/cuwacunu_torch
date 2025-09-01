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

/**
 * @brief A memory-mapped dataset for PyTorch-based data loading.
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
  std::size_t num_records_;                  /* Number of records in the dataset */

public:
  /* Variables for key-value boundaries. */
  std::size_t key_value_offset_;                /* Offset (in bytes) of the key in each record */
  typename T::key_type_t leftmost_key_value_;   /* Smallest key value in the dataset */
  typename T::key_type_t rightmost_key_value_;  /* Largest key value in the dataset */
  typename T::key_type_t key_value_span_;       /* rightmost - leftmost */
  typename T::key_type_t key_value_step_;       /* regular increment in key_value between consecutive records */

public:
  explicit MemoryMappedDataset(const std::string& bin_filename)
      : bin_filename_(bin_filename),
        mapped_data_(std::make_unique<MappedData>(bin_filename)),
        num_records_(mapped_data_->file_size_ / sizeof(T)),
        key_value_offset_(T::key_offset()) {

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
  }

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
   * @brief Retrieves a single-position sample at index (past=1, future=1 if available).
   */
  observation_sample_t get(std::size_t index) override {
    if (index >= num_records_) {
      log_fatal("[MemoryMappedDataset] Index [%zu] out of range [0, %zu] on file %s\n",
                index, num_records_, bin_filename_.c_str());
    }

    T cur = read_memory_struct<T>(mapped_data_->data_ptr_, index);
    const auto D = cur.tensor_features().size();

    torch::Tensor past_X   = torch::empty({1, static_cast<long>(D)}, torch::kFloat32);
    torch::Tensor past_msk = torch::empty({1},                       torch::kBool);
    {
      float* x = past_X.data_ptr<float>();
      const auto& v = cur.tensor_features();
      std::copy(v.begin(), v.end(), x);
      past_msk[0] = cur.is_valid();
    }

    torch::Tensor fut_X, fut_msk;
    if (index + 1 < num_records_) {
      T nxt = read_memory_struct<T>(mapped_data_->data_ptr_, index + 1);
      fut_X   = torch::empty({1, static_cast<long>(D)}, torch::kFloat32);
      fut_msk = torch::empty({1},                       torch::kBool);
      float* x = fut_X.data_ptr<float>();
      const auto& v = nxt.tensor_features();
      std::copy(v.begin(), v.end(), x);
      fut_msk[0] = nxt.is_valid();
    } else {
      fut_X   = torch::zeros({1, static_cast<long>(D)}, torch::kFloat32);
      fut_msk = torch::full({1}, false,                 torch::kBool);
    }

    return observation_sample_t{ past_X, past_msk, fut_X, fut_msk };
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
 */
template <typename T>
class MemoryMappedConcatDataset : public torch::data::datasets::Dataset<MemoryMappedConcatDataset<T>, observation_sample_t> {
private:
  std::vector<std::shared_ptr<MemoryMappedDataset<T>>> datasets_;
  std::vector<std::string> file_names_;
  std::vector<std::size_t> N_past_;
  std::vector<std::size_t> N_future_;

public:
  std::size_t max_N_past_{0};
  std::size_t max_N_future_{0};

  std::size_t num_records_{0};                           /* number of valid target positions across intersection */
  typename T::key_type_t leftmost_key_value_{};          /* intersection lower bound */
  typename T::key_type_t rightmost_key_value_{};         /* intersection upper bound */
  typename T::key_type_t key_value_span_{};              /* rightmost - leftmost */
  typename T::key_type_t key_value_step_{};              /* step (chosen across datasets) */

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
    datasets_.push_back(std::make_shared<MemoryMappedDataset<T>>(file_names_.back()));
    N_past_.push_back(N_past);
    N_future_.push_back(N_future);

    auto& d = datasets_.back();

    /* --- validations --- */
    if (d->size().value() < (N_past + N_future)) {
      log_fatal("[MemoryMappedConcatDataset](add_dataset) Dataset %s too small: size:%zu < N_past+N_future:%zu\n",
                csv_filename.c_str(), d->size().value(), (N_past + N_future));
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

    /* --- update global parameters --- */
    max_N_past_   = *std::max_element(N_past_.begin(),   N_past_.end());
    max_N_future_ = *std::max_element(N_future_.begin(), N_future_.end());

    // Step choice across datasets: conservative pick = minimum step (densest grid)
    if (datasets_.size() == 1) {
      key_value_step_ = d->key_value_step_;
    } else {
      key_value_step_ = std::min(key_value_step_, d->key_value_step_);
    }

    // Per-dataset valid key range for target index i:
    //   i >= (N_past-1)   and   i + N_future < size
    // In key space:
    const auto valid_left  = d->leftmost_key_value_  + static_cast<typename T::key_type_t>(N_past  - 1) * d->key_value_step_;
    const auto valid_right = d->rightmost_key_value_ - static_cast<typename T::key_type_t>(N_future    ) * d->key_value_step_;

    if (datasets_.size() == 1) {
      leftmost_key_value_  = valid_left;
      rightmost_key_value_ = valid_right;
    } else {
      leftmost_key_value_  = std::max(leftmost_key_value_,  valid_left);
      rightmost_key_value_ = std::min(rightmost_key_value_, valid_right);
    }

    if (!(leftmost_key_value_ < rightmost_key_value_)) {
      log_fatal("[MemoryMappedConcatDataset](add_dataset) Empty intersection after adding %s\n",
                csv_filename.c_str());
    }

    key_value_span_ = rightmost_key_value_ - leftmost_key_value_;

    // Number of valid target positions across the intersection.
    // Inclusive count if exactly divisible; using +1 to include both ends on a regular grid.
    num_records_ = static_cast<std::size_t>(key_value_span_ / key_value_step_) + 1;
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
  cuwacunu::camahjucunu::BNF::observation_instruction_t obs_inst,
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
