/* memory_mapped_dataset.h */
#pragma once

#include <torch/torch.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <cstring> // For std::memcpy
#include <cstddef> /* For offsetof */
#include <execution>
#include <unordered_set>
#include <type_traits>
#include <c10/util/Optional.h> // for c10::nullopt;
#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"
#include "piaabo/torch_compat/torch_utils.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/BNF/implementations/training_pipeline/training_pipeline.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

namespace cuwacunu {
namespace camahjucunu {
namespace data {

/* A struct to represent a single sample from your dataset. */
struct observation_sample_t {
  torch::Tensor features;
  torch::Tensor mask;

  /* A custom collate function that stacks features and masks from a batch of Samples */
  static inline observation_sample_t collate_fn(const std::vector<observation_sample_t>& batch) {
    /* Collect all features and masks */
    std::vector<torch::Tensor> feature_list;
    std::vector<torch::Tensor> mask_list;

    feature_list.reserve(batch.size());
    mask_list.reserve(batch.size());

    for (const auto& item : batch) {
      feature_list.push_back(item.features);
      mask_list.push_back(item.mask);
    }

    /* Ensure all tensors in feature_list and mask_list have the same shape along non-batch dimensions */
    TORCH_CHECK(!feature_list.empty(), "(observation_sample_t)[collate_fn] Features Batch is empty!");
    TORCH_CHECK(!mask_list.empty(), "(observation_sample_t)[collate_fn] Mask Batch is empty!");

    /* Stack along a new dimension to form a batch */
    auto features = torch::stack(feature_list, 0);
    auto masks = torch::stack(mask_list, 0);

    return observation_sample_t{features, masks};
  }

  /* reset (clear) method */
  void reset() {
    features.reset();
    mask.reset();
  }
};

/**
 * @brief Reads a specific value from a memory-mapped structure.
 *
 * This template function reads a value of type `key_type_t` from a specific 
 * index and offset within a memory-mapped binary data structure.
 *
 * @tparam T The struct type, which defines `key_type_t`.
 * @param data_ptr Pointer to the memory-mapped data.
 * @param index Index of the record.
 * @param offset Offset within the record where the value is located.
 * @return The value read from the memory location.
 */
template <typename T>
typename T::key_type_t read_memory_value(const void* data_ptr, std::size_t index, std::size_t offset) {
    typename T::key_type_t value;
    const std::byte* base = static_cast<const std::byte*>(data_ptr);
    const std::byte* target = base + index * sizeof(T) + offset;
    std::memcpy(&value, target, sizeof(typename T::key_type_t));
    return value;
}

/**
 * @brief Reads a full record from the memory-mapped data.
 *
 * This function returns a const reference to a record located at the specified
 * index within the memory-mapped binary data.
 *
 * @tparam T The struct type of the record.
 * @param data_ptr Pointer to the memory-mapped data.
 * @param index Index of the record to read.
 * @return Const reference to the record.
 */
template <typename T>
T read_memory_struct(const void* data_ptr, std::size_t index) {
    T record;
    const std::byte* base = static_cast<const std::byte*>(data_ptr);
    const std::byte* target = base + index * sizeof(T);
    std::memcpy(&record, target, sizeof(T));
    return record;
}

/**
 * @brief Reads multiple records from the memory-mapped data.
 *
 * This function returns a vector of records located starting from the specified
 * index within the memory-mapped binary data.
 *
 * @tparam T The struct type of the record.
 * @param data_ptr Pointer to the memory-mapped data.
 * @param index Index of the first record to read.
 * @param count Number of records to read.
 * @return Vector containing the records.
 */
template <typename T>
std::vector<T> read_memory_structs(const void* data_ptr, std::size_t index, std::size_t count) {
  std::vector<T> records(count); // Allocate memory for `count` records.
  const std::byte* base = static_cast<const std::byte*>(data_ptr);
  const std::byte* target = base + index * sizeof(T);

  std::memcpy(records.data(), target, count * sizeof(T));

  return records;
}

/*
 * Like std::abs but stable for unsigned and signed types
 */
template <typename T>
typename std::common_type<T, T>::type absolute_difference(T a, T b) {
  return a >= b ? a - b : b - a;
}

/**
 * @brief A memory-mapped dataset for PyTorch-based data loading.
 *
 * This class implements a dataset that leverages memory mapping for efficient 
 * data access, particularly useful when working with large datasets that do not 
 * fit entirely in memory. It uses binary files containing records of a fixed size.
 *
 * @tparam T The struct type of the records stored in the binary file. 
 *           The struct must provide a `tensor_features()` method 
 *           returning a `std::vector<float>` representation.
 */
template <typename T>
class MemoryMappedDataset : public torch::data::datasets::Dataset<MemoryMappedDataset<T>, observation_sample_t> {
private:
  /**
   * @brief Holds the memory mapping details for the dataset.
   *
   * This struct manages the file descriptor, memory-mapped pointer, 
   * and file size for the memory-mapped data.
   */
  struct MappedData {
    int fd_;            /* File descriptor for the binary file */
    void* data_ptr_;    /* Pointer to the memory-mapped data */
    std::size_t file_size_;  /* Size of the binary file in bytes */

    /**
     * @brief Constructor to initialize the memory mapping.
     * 
     * @param bin_filename Path to the binary file to map.
     * @throws std::runtime_error if the file cannot be opened or mapped.
     */
    MappedData(const std::string& bin_filename) 
      : fd_(-1), data_ptr_(nullptr), file_size_(0) {
      /* Open the file for reading. */
      fd_ = open(bin_filename.c_str(), O_RDONLY);
      if (fd_ == -1) {
        log_fatal("[MemoryMappedDataset] Error: Could not open binary file: %s, %s \n", bin_filename.c_str(), std::strerror(errno));
      }

      /* Determine file size using lseek. */
      struct stat st;
      if (fstat(fd_, &st) == -1) {
        close(fd_);
        log_fatal("[MemoryMappedDataset] Error: Failed to determine file size for: %s, %s \n ", bin_filename.c_str(), std::strerror(errno));
      }
      file_size_ = static_cast<std::size_t>(st.st_size);

      /* Ensure file is not empty */
      if (file_size_ == 0) {
        close(fd_);
        log_fatal("[MemoryMappedDataset] Error: File is empty: %s\n", bin_filename.c_str());
      }

      /* Map the file into memory. */
      data_ptr_ = mmap(nullptr, file_size_, PROT_READ, MAP_PRIVATE, fd_, 0);
      if (data_ptr_ == MAP_FAILED) {
        data_ptr_ = nullptr;
        close(fd_);
        log_fatal("[MemoryMappedDataset] Error: Failed to memory-map the file: %s, %s\n", bin_filename.c_str(), std::strerror(errno));
      }
    }

    /**
     * @brief Destructor to unmap the file and close the descriptor.
     */
    ~MappedData() {
      if (data_ptr_ && data_ptr_ != MAP_FAILED) { munmap(data_ptr_, file_size_); }
      if (fd_ != -1) { close(fd_); }
    }
  };

private:
  std::string bin_filename_;                    /* Path to the binary file */
  std::unique_ptr<MappedData> mapped_data_;     /* Unique pointer to memory-mapped data */
  std::size_t num_records_;                     /* Number of records in the dataset */

public:
  /* Variables for key-value boundaries. */
  std::size_t key_value_offset_;                /* Offset on memory bits of the key in each record */
  typename T::key_type_t leftmost_key_value_;   /* Smallest key value in the dataset */
  typename T::key_type_t rightmost_key_value_;  /* Largest key value in the dataset */
  typename T::key_type_t key_value_span_;       /* rightmost_ - leftmost */
  typename T::key_type_t key_value_step_;       /* regular increment on the key_value at each record */

public:
  /**
   * @brief Constructs the dataset and initializes memory mapping.
   * 
   * @param bin_filename Path to the binary file containing dataset records.
   * @throws std::runtime_error for invalid file sizes, empty datasets, or unsorted data.
   */
  MemoryMappedDataset(const std::string& bin_filename)
      : bin_filename_(bin_filename),
        mapped_data_(std::make_unique<MappedData>(bin_filename)),
        num_records_(mapped_data_->file_size_ / sizeof(T)),
        key_value_offset_(T::key_offset()) {
    /* Ensure file size is a multiple of the struct size. */
    if (mapped_data_->file_size_ % sizeof(T) != 0) {
      log_fatal("[MemoryMappedDataset] Error: Binary file size is not a multiple of struct size. File: %s\n", bin_filename_.c_str());
    }

    /* Ensure the dataset is not empty. */
    if (num_records_ == 0) {
      log_fatal("[MemoryMappedDataset] Error: Binary Dataset is empty. File: %s\n", bin_filename_.c_str());
    }

    /* Read the boundary key values for validation. */
    leftmost_key_value_ = read_memory_value<T>(mapped_data_->data_ptr_, 0, key_value_offset_);
    rightmost_key_value_ = read_memory_value<T>(mapped_data_->data_ptr_, num_records_ - 1, key_value_offset_);
    key_value_span_ = rightmost_key_value_ - leftmost_key_value_;

    /* Validate that the dataset is sorted. */
    if (leftmost_key_value_ >= rightmost_key_value_) {
      log_fatal("[MemoryMappedDataset] Error: Binary Dataset is not sorted correctly. File: %s\n", bin_filename_.c_str());
    }

    /* Ensure the required methods are present on the template */
    static_assert(std::is_same_v<decltype(std::declval<T>().tensor_features()), std::vector<double>>, "[MemoryMappedDataset] Error: Template argument must provide tensor_features() returning std::vector<double>");
    static_assert(std::is_standard_layout<T>::value, "[MemoryMappedDataset] Error: Template argument must be a standard layout type");
    static_assert(std::is_trivially_copyable<T>::value, "[MemoryMappedDataset] Error: Template argument must be trivially copyable");

    /* navigate the entire file to validate the data is sequential and increasing in the key space */
    typename T::key_type_t prev = read_memory_value<T>(mapped_data_->data_ptr_, 0, key_value_offset_);
    typename T::key_type_t curr = read_memory_value<T>(mapped_data_->data_ptr_, 1, key_value_offset_);
    key_value_step_ = curr - prev;
    
    /* validate regular delta */
    if(key_value_step_ <= 0) {
      log_fatal("[MemoryMappedDataset] Error: negative or zero key_value_step_. File: %s. \n", bin_filename_.c_str());
    }

    /* validate all the records in the binary file */
    for(std::size_t idx = 1; idx < num_records_; idx++) {
      typename T::key_type_t curr = read_memory_value<T>(mapped_data_->data_ptr_, idx, key_value_offset_);
      if(curr < prev) {
        log_fatal("[MemoryMappedDataset] Error: Binary Dataset is not sequential and increasing (not sorted). File: %s, on index: %ld\n", bin_filename_.c_str(), idx);
      }
      if(std::abs((curr - prev) - key_value_step_) > 1e-9) {
        log_warn("[MemoryMappedDataset] record on file [%s] found with irregular delta of step at index [%ld]: (curr - prev): %f != key_value_step_: %f\n", bin_filename_.c_str(), idx, static_cast<double>(curr - prev), static_cast<double>(key_value_step_));
      }
      prev = curr;
    }
  }

  /**
   * @brief Retrieves a record as a tensor by its index.
   * 
   * @param index Index of the record to retrieve.
   * @return A tensor representing the record's features.
   * @throws std::out_of_range if the index is invalid.
   */
  observation_sample_t get(std::size_t index) override {
    if (index >= num_records_) {
      log_fatal("[MemoryMappedDataset] Index [%ld] out of range [0, %ld] on file %s\n", index, num_records_, bin_filename_.c_str());
    }
    
    T data_ = read_memory_struct<T>(mapped_data_->data_ptr_, index);
    return {torch::tensor(data_.tensor_features(), torch::kFloat32), torch::tensor(data_.is_valid(), torch::kBool)};
  }

  /**
   * @brief Returns the size of the dataset.
   * 
   * @return Optional size of the dataset.
   */
  torch::optional<std::size_t> size() const override {
    return num_records_;
  }
  
  /**
   * @brief A set of common Samplers, to be used by the dataloader
   */
  torch::data::samplers::SequentialSampler SequentialSampler() const {
    return torch::data::samplers::SequentialSampler(num_records_);
  }
  torch::data::DataLoaderOptions SequentialSampler_options(std::size_t batch_size = 64, std::size_t workers = 4) const {
    return torch::data::DataLoaderOptions()
            .batch_size(batch_size)
            .workers(workers);
  }

  torch::data::samplers::RandomSampler RandomSampler() const {
    return torch::data::samplers::RandomSampler(num_records_);
  }
  torch::data::DataLoaderOptions RandomSampler_options(std::size_t batch_size = 64, std::size_t workers = 4) const {
    return torch::data::DataLoaderOptions()
            .batch_size(batch_size)
            .workers(workers);
  }

  /**
   * @brief Retrieves a record as a tensor by its key value.
   * 
   * @param target_key_value The key value to search for.
   * @return A tensor representing the record's features.
   */
  observation_sample_t get_by_key_value(typename T::key_type_t target_key_value) {
    std::size_t index = find_closest_index(target_key_value);
    return get(index);
  }

  /**
   * @brief Retrieves N tensors 
   *    the first tensor is at time t-N and the last is at time t
   *  
   * @param target_key_value The key value to search for.
   * @param N Number of records to retrieve.
   * @return A pair of tensors representing the record's features and the corresponding mask.
   */
  observation_sample_t get_sequence_ending_at_key_value(typename T::key_type_t target_key_value, std::size_t N) {
    std::size_t index = find_closest_index(target_key_value);

    if (index < N) {
      throw std::out_of_range("[MemoryMappedDataset] Target [" + std::to_string(target_key_value) + "] is too early for size N=" + std::to_string(N) + ", on file: " + bin_filename_);
    }

    /* Read the memory */
    std::vector<T> records = read_memory_structs<T>(mapped_data_->data_ptr_, index - N + 1, N);

    std::size_t feature_dim = records[0].tensor_features().size();

    /* Preallocate the tensors with the desired shapes */
    torch::Tensor features_tensor = torch::empty({static_cast<long>(N), static_cast<long>(feature_dim)}, torch::kFloat32);
    torch::Tensor mask_tensor = torch::empty({static_cast<long>(N)}, torch::kBool);

    /* Get pointers to the tensors' underlying data */
    float* tensor_data = features_tensor.data_ptr<float>();
    bool* mask_data = mask_tensor.data_ptr<bool>();

    /* Copy the feature data and mask values directly into the tensors' memory */
    for (std::size_t i = 0; i < N; ++i) {
      const auto& features = records[i].tensor_features();
      std::copy(features.begin(), features.end(), tensor_data + i * feature_dim);
      mask_data[i] = records[i].is_valid();
    }

    return {features_tensor, mask_tensor};
  }


private:
  /**
   * @brief Finds the closest index for a given key value using interpolation search,
   *        ensuring the returned index corresponds to a key value <= target_key_value.
   * 
   * @param target_key_value The target key value to search for.
   * @return The closest index to the target key value.
   * @throws std::runtime_error if the dataset is empty.
   */
  std::size_t find_closest_index(typename T::key_type_t target_key_value) {
    if (num_records_ == 0) {
      log_fatal("[MemoryMappedDataset] Error: Dataset is empty: %s\n", bin_filename_.c_str());
    }

    /* Handle edge cases for values outside the dataset's range. */
    if (target_key_value <= leftmost_key_value_) {
      return 0;
    }
    if (target_key_value >= rightmost_key_value_) {
      return num_records_ - 1;
    }

    /* Perform interpolation search. */
    std::size_t left = 0;
    std::size_t right = num_records_ - 1;
    std::size_t best_index = 0;
    typename T::key_type_t best_diff = std::numeric_limits<typename T::key_type_t>::max();
    typename T::key_type_t left_key_value = leftmost_key_value_;
    typename T::key_type_t right_key_value = rightmost_key_value_;

    while (left <= right) {
      /* Prevent division by zero. It also captures missing values */
      if (left_key_value == right_key_value) break;

      /* (Interpolation Search) Estimate the middle index using interpolation with floating-point arithmetic. */
      typename T::key_type_t numerator = static_cast<typename T::key_type_t>(target_key_value - left_key_value) * static_cast<typename T::key_type_t>(right - left);  /* might overflow */
      typename T::key_type_t denominator = static_cast<typename T::key_type_t>(right_key_value - left_key_value);
      double ratio = static_cast<double>(numerator) / static_cast<double>(denominator);
      std::size_t mid = left + static_cast<std::size_t>(ratio);
      
      /* Clamp the mid index within bounds. */
      if (mid >= num_records_) {
        mid = num_records_ - 1;
        best_index = mid;
        break;
      }

      /* Read the key value at the mid index and calculate the difference. */
      typename T::key_type_t mid_key_value = read_memory_value<T>(mapped_data_->data_ptr_, mid, key_value_offset_);

      /* This ensures we never "look into the future." */
      if (mid_key_value <= target_key_value) {
        typename T::key_type_t diff = absolute_difference(mid_key_value, target_key_value);
        if (diff < best_diff) {
          best_diff = diff;
          best_index = mid;
        }
      }

      /* Narrow the search range based on the comparison. */
      if (mid_key_value < target_key_value) {
        /* Prevent overflow */
        if (mid >= (num_records_ - 1)) break;
        /* update left */
        left = mid + 1;
        left_key_value = read_memory_value<T>(mapped_data_->data_ptr_, left, key_value_offset_);
      } else if (mid_key_value > target_key_value) {
        /* Prevent underflow */
        if (mid <= 0) break;
        /* update right */
        right = mid - 1;
        right_key_value = read_memory_value<T>(mapped_data_->data_ptr_, right, key_value_offset_);
      } else {
        /* Exact match found */
        best_index = mid;
        break;
      }
    }

    return best_index;
  }
};



/**
 * @brief A memory-mapped concatenated dataset for efficient data access.
 *
 * This class provides a mechanism to manage and access multiple memory-mapped datasets
 * as a single concatenated dataset. It is designed to handle large datasets efficiently
 * using memory mapping, minimizing memory usage and improving data access speed.
 *
 * @tparam T The underlying datatype template for the memory-mapped dataset. 
 *
 */
template <typename T>
class MemoryMappedConcatDataset : public torch::data::datasets::Dataset<MemoryMappedConcatDataset<T>, observation_sample_t> {
private:
  std::vector<std::shared_ptr<MemoryMappedDataset<T>>> datasets_;
  std::vector<std::string> file_names_;
  std::vector<std::size_t> Ns_;

public:
  std::size_t max_N_;                                 /* size of the longest sequence */
  std::size_t num_records_;                           /* amount of minimaly devisible records */
  typename T::key_type_t leftmost_key_value_;         /* in key_value, the lower bound */
  typename T::key_type_t rightmost_key_value_;        /* in key_value, the higher bound */
  typename T::key_type_t key_value_span_;             /* in key_value, rightmost - leftmost */
  typename T::key_type_t key_value_step_;             /* the minimal regular increment overall datasets */
  
public:
  MemoryMappedConcatDataset() {
    max_N_                = std::numeric_limits<std::size_t>::min();
    num_records_          = std::numeric_limits<std::size_t>::min();
    leftmost_key_value_   = std::numeric_limits<typename T::key_type_t>::min();
    rightmost_key_value_  = std::numeric_limits<typename T::key_type_t>::max();
    key_value_span_       = std::numeric_limits<typename T::key_type_t>::max();
    key_value_step_       = std::numeric_limits<typename T::key_type_t>::max();
  }

  observation_sample_t get(std::size_t index) override {
    if(index > num_records_) {
      log_fatal("[MemoryMappedConcatDataset] get() request, index: %ld, exceed the size: %ld \n", index, num_records_);
    }
    /* index to key_value */
    typename T::key_type_t factor           = key_value_span_ / static_cast<typename T::key_type_t>(num_records_);
    typename T::key_type_t target_key_value = leftmost_key_value_ + static_cast<typename T::key_type_t>(index) * factor;

    /* get sequence */
    return get_sequence_ending_at_key_value(target_key_value);
  }

  torch::optional<std::size_t> size() const override {
    return num_records_;
  }

  /**
   * @brief A set of common Samplers, to be used by the dataloader
   */
  torch::data::samplers::SequentialSampler SequentialSampler() const {
    return torch::data::samplers::SequentialSampler(num_records_);
  }
  torch::data::DataLoaderOptions SequentialSampler_options(std::size_t batch_size = 64, std::size_t workers = 4) const {
    return torch::data::DataLoaderOptions()
            .batch_size(batch_size)
            .workers(workers);
  }

  torch::data::samplers::RandomSampler RandomSampler() const {
    return torch::data::samplers::RandomSampler(num_records_);
  }
  torch::data::DataLoaderOptions RandomSampler_options(std::size_t batch_size = 64, std::size_t workers = 4) const {
    return torch::data::DataLoaderOptions()
            .batch_size(batch_size)
            .workers(workers);
  }

  /**
   * @brief Retrieves a record as a tensor by its key value.
   * 
   * @param target_key_value The key value to search for.
   * @return A tensor representing the record's features.
   */
  observation_sample_t get_by_key_value(typename T::key_type_t target_key_value) {

    size_t num_sources = datasets_.size();

    std::vector<at::Tensor> features_list(num_sources);
    std::vector<at::Tensor> mask_list(num_sources);

    /* Use a parallel algorithm */
    std::for_each(std::execution::par, datasets_.begin(), datasets_.end(),
      [target_key_value, this, &features_list, &mask_list](auto& dataset) {
        size_t index = &dataset - &datasets_[0];
        observation_sample_t sample = dataset->get_by_key_value(target_key_value);
        features_list[index] = sample.features;
        mask_list[index] = sample.mask;
      });

    return {torch::stack(features_list, 0), torch::stack(mask_list, 0)};
  }

  /**
   * @brief Retrieves sequences ending at a given key value from multiple datasets.
   *        The sequences are padded to the maximum sequence length (max_N_), and a mask tensor is returned.
   *
   * @param target_key_value The key value to search for.
   * @return A pair of tensors: the dense tensor and the mask tensor.
   */
  observation_sample_t get_sequence_ending_at_key_value(typename T::key_type_t target_key_value) {


    size_t num_sources = datasets_.size();

    std::vector<torch::Tensor> features_list(num_sources);
    std::vector<torch::Tensor> mask_list(num_sources);

    /* Use a parallel algorithm */
    std::for_each(std::execution::par, datasets_.begin(), datasets_.end(),
      [target_key_value, this, &features_list, &mask_list](auto& dataset) {
        size_t index = &dataset - &datasets_[0];
        int64_t N = static_cast<int64_t>(Ns_[index]);

        /* Retrieve the sequence and the mask from the dataset */
        observation_sample_t sample = dataset->get_sequence_ending_at_key_value(target_key_value, N); /* Shapes: (N, feature_size), (N) */

        /* Pad the sequence and the mask to max_N_ length */
        if (N < static_cast<int64_t>(max_N_)) {
          int64_t pad_length = static_cast<int64_t>(max_N_ - N);

          /* Pad the sequence tensor at the beginning to preserve temporal meaning, the last value is the last timestep */
          torch::Tensor pad_sequence = torch::zeros({pad_length, sample.features.size(1)}, sample.features.options());
          features_list[index] = torch::cat({pad_sequence, sample.features}, 0); /* Shape: (max_N_, feature_size) */

          /* Pad the mask tensor at the beginning to preserve temporal meaning, the last value is the last timestep */
          torch::Tensor pad_mask = torch::zeros({pad_length}, sample.mask.options());
          mask_list[index] = torch::cat({pad_mask, sample.mask}, 0); /* Shape: (max_N_) */
        } else {
          /* No padding needed */
          features_list[index] = sample.features;
          mask_list[index] = sample.mask;
        }
      });

    /* Stack tensors to get the final dense tensor and mask tensor */
    torch::Tensor dense_tensor = torch::stack(features_list, 0); /* Shape: (num_sources, max_N_, feature_size) */
    torch::Tensor mask_tensor = torch::stack(mask_list, 0); /* Shape: (num_sources, max_N_) */

    return {dense_tensor, mask_tensor};
  }

public:
  /**
   * @brief Adds a dataset to the memory-mapped concatenated dataset.
   *
   * This method processes a CSV file to create a memory-mapped binary representation, 
   * then appends the resulting dataset to the internal structures for efficient sequential 
   * or random access. It ensures the added dataset meets size and uniqueness constraints.
   *
   * @param csv_filename The path to the CSV file to be added as a dataset.
   * @param N Size of the sequence, needed whenever the added dataset be loaded. 
   * @param normalization_window rolling window of statistics to normalize the data, (set to zero to avoid normalization).
   * @param force_binarization Whether to force binary conversion of the CSV file.
   * @param buffer_size Buffer size used during the binary conversion process (default: 1024).
   * @param delimiter Delimiter character used in the CSV file (default: ',').
   *
   */
  void add_dataset(const std::string csv_filename, std::size_t N, std::size_t normalization_window = 0, bool force_binarization = false, size_t buffer_size = 1024, char delimiter = ',') {

  /* --- --- --- prepare the file --- --- --- */
    /* binarize the csv file */
    std::string bin_filename = sanitize_csv_into_binary_file<T>(csv_filename, normalization_window, force_binarization, buffer_size, delimiter);
    
  /* --- --- --- adding the dataset --- --- --- */
    /* append the file_name */
    file_names_.push_back(bin_filename);

    /* append the dataset */
    datasets_.push_back(
      std::make_shared<MemoryMappedDataset<T>>(file_names_.back())
    );

    /* append count */
    Ns_.push_back(N);
    auto& new_dataset_ = datasets_.back();

  /* --- --- --- validate --- --- --- */
    /* validate the size it's enoguh to load the sequence */
    if(new_dataset_->size() < N) {
      log_fatal("[MemoryMappedConcatDataset] Trying to add dataset %s, failes due to size dataset_size:%ld < N:%ld\n", csv_filename.c_str(), new_dataset_->size().value(), N);
    }

    /* validate the sizes matches */
    if((datasets_.size() != Ns_.size()) || (file_names_.size() != Ns_.size())) {
      log_fatal("[MemoryMappedConcatDataset] Unexpected sizes match on object vectors: datasets_.size(): %ld == file_names_.size(): %ld == Ns_.size(): %ld \n", datasets_.size(), file_names_.size(), Ns_.size());
    }

    /* ensure all binary files are different */
    if(std::unordered_set<std::string>(file_names_.begin(), file_names_.end()).size() != file_names_.size()) {
      log_fatal("[MemoryMappedConcatDataset] Duplicated csv file found, on add_dataset: %s \n", csv_filename.c_str());
    }
    
  /* --- --- --- utility variables --- --- --- */
    max_N_                = *std::max_element(Ns_.begin(), Ns_.end());
    num_records_          = MAX(num_records_, static_cast<std::size_t>(new_dataset_->key_value_span_ / new_dataset_->key_value_step_));
    leftmost_key_value_   = MAX(leftmost_key_value_,  (new_dataset_->leftmost_key_value_ + static_cast<typename T::key_type_t>(N) * new_dataset_->key_value_step_));
    rightmost_key_value_  = MIN(rightmost_key_value_, new_dataset_->rightmost_key_value_);
    key_value_span_       = rightmost_key_value_ - leftmost_key_value_;
    key_value_step_       = MIN(key_value_step_, new_dataset_->key_value_step_);
  }
};



/**
  * @brief Construct a new MemoryMappedDataSet from an observation_instruction_t.
  *
  * @tparam T The underlying data type used by the dataset. 
  * @param instrument the dataset is tied to an instrument.
  * @param obs_inst The observation pipeline instruction.
  * @param force_binarization Whether or not to renormalize and binarize the memory mapped data files.
  *
  */
template<typename T>
MemoryMappedConcatDataset<T> create_memory_mapped_concat_dataset(
  std::string& instrument, 
  cuwacunu::camahjucunu::BNF::observation_instruction_t obs_inst, 
  bool force_binarization = false) {
    /* variables */
    char delimiter = ',';
    size_t buffer_size = 1024;

    /* create the dataset for T */
    auto concat_dataset = MemoryMappedConcatDataset<T>();

    /* add the datasets to the concatdataset as per in the instruction */
    for(auto& in_form: obs_inst.input_forms) {
      if(in_form.active == "true") {
        for(auto& instr_form: obs_inst.filter_instrument_forms(instrument, in_form.record_type, in_form.interval)) {
          concat_dataset.add_dataset(
            /* dataset loads sequences of size N */ instr_form.source, 
            /* dataset sequence lenght */           std::stoul(in_form.seq_length), 
            /* normalization window of size N */    std::stoul(instr_form.norm_window), 
            /* force_binarization */                force_binarization, 
            /* buffer_size */                       buffer_size, 
            /* delimiter */                         delimiter
          );
        }
      }
    }

    return concat_dataset;
}
} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
