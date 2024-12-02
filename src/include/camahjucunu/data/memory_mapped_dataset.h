
/*
  create this class to hable just one file (kline type)

  create a composite dataset to join them base of a BNF instruction
  
  Fix the irregular skips:
    - binary datafile should be correct, interpolation search is optimal only if the data distribution is uniform
    use std::numeric_limits<double>::quiet_NaN() for missing values
      then torch::Tensor mask = data.isnan().logical_not().to(torch::kFloat32);
  
  if no exact value is found, make sure to use the correct timestamp, do not allow the model to look into the future

  if missed values are included: 
    (data augmentation via random masking): during training, repeat the sequences with mask at random to train the model in arbitrary missing values
        Normalization Layers: Be cautious with layers like Batch Normalization, as the statistics can be affected by masked inputs. Consider alternatives like Layer Normalization.

    hours knlines have much less probability of mask than minutes klines for instance

  learning:
    Curriculum Learning: first learn from full data, then learn from masks
    dont use droput or standard variation
    - L2 regularization is alright
    - srink and perturb is also alright
    Continual Backpropagation (Richard Sutton):
      - inspecting the network for the neurons that are beeing activated (utility mesarures) the less and reinitializing their parameters is nice. 
    
  Architrecute:
    - use of residual networks ResNets


  note, maybe usefull padded sequences
    auto packed_input = torch::nn::utils::rnn::pack_padded_sequence(inputs, lengths, batch_first=>true);
*/

/* memory_mapped_dataset.h */
#pragma once

#include <torch/torch.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <mutex>
#include <memory>
#include <string>
#include <cstring> // For std::memcpy
#include <cstddef> /* For offsetof */
#include <type_traits>
#include "piaabo/dutils.h"
#include "piaabo/dlarge_files.h"
#include "piaabo/torch_compat/torch_utils.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

namespace cuwacunu {
namespace camahjucunu {
namespace data {

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
 *           returning a `std::vector<double>` representation.
 */
template <typename T>
class MemoryMappedDataset : public torch::data::datasets::Dataset<MemoryMappedDataset<T>, torch::Tensor> {
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
        throw std::runtime_error("[MemoryMappedDataset] Error: Could not open binary file: " + bin_filename + " - " + std::strerror(errno));
      }

      /* Determine file size using lseek. */
      struct stat st;
      if (fstat(fd_, &st) == -1) {
        close(fd_);
        throw std::runtime_error("[MemoryMappedDataset] Error: Failed to determine file size for: " + bin_filename + " - " + std::strerror(errno));
      }
      file_size_ = static_cast<std::size_t>(st.st_size);

      /* Ensure file is not empty */
      if (file_size_ == 0) {
        close(fd_);
        throw std::runtime_error("[MemoryMappedDataset] Error: File is empty: " + bin_filename);
      }

      /* Map the file into memory. */
      data_ptr_ = mmap(nullptr, file_size_, PROT_READ, MAP_PRIVATE, fd_, 0);
      if (data_ptr_ == MAP_FAILED) {
        data_ptr_ = nullptr;
        close(fd_);
        throw std::runtime_error("[MemoryMappedDataset] Error: Failed to memory-map the file: " + bin_filename + " - " + std::strerror(errno));
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
  std::string bin_filename_;                 /* Path to the binary file */
  std::unique_ptr<MappedData> mapped_data_;  /* Shared pointer to memory-mapped data */
  std::size_t num_records_;                       /* Number of records in the dataset */

  /* Variables for key-value boundaries. */
  std::size_t key_value_offset_;             /* Offset of the key in each record */
  typename T::key_type_t leftmost_key_value_;/* Smallest key value in the dataset */
  typename T::key_type_t rightmost_key_value_;  /* Largest key value in the dataset */

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
      throw std::runtime_error("[MemoryMappedDataset] Error: Binary file size is not a multiple of struct size. File: " + bin_filename_);
    }

    /* Ensure the dataset is not empty. */
    if (num_records_ == 0) {
      throw std::runtime_error("[MemoryMappedDataset] Error: Binary Dataset is empty. File: " + bin_filename_);
    }

    /* Read the boundary key values for validation. */
    leftmost_key_value_ = read_memory_value<T>(mapped_data_->data_ptr_, 0, key_value_offset_);
    rightmost_key_value_ = read_memory_value<T>(mapped_data_->data_ptr_, num_records_ - 1, key_value_offset_);

    /* Validate that the dataset is sorted. */
    if (leftmost_key_value_ >= rightmost_key_value_) {
      throw std::runtime_error("[MemoryMappedDataset] Error: Binary Dataset is not sorted correctly. File: " + bin_filename_);
    }

    /* Ensure the required methods are present on the template */
    static_assert(std::is_same_v<decltype(std::declval<T>().tensor_features()), std::vector<double>>, "[MemoryMappedDataset] Error: Template argument must provide tensor_features() returning std::vector<double>");
    static_assert(std::is_standard_layout<T>::value, "[MemoryMappedDataset] Error: Template argument must be a standard layout type");
    static_assert(std::is_trivially_copyable<T>::value, "[MemoryMappedDataset] Error: Template argument must be trivially copyable");

    /* navigate the entire file to validate the data is sequential and increasing in the key space */
    typename T::key_type_t prev = read_memory_value<T>(mapped_data_->data_ptr_, 0, key_value_offset_);
    typename T::key_type_t curr = read_memory_value<T>(mapped_data_->data_ptr_, 1, key_value_offset_);
    typename T::key_type_t regular_delta = curr - prev;
    for(std::size_t idx = 2; idx < num_records_; idx++) {
      typename T::key_type_t curr = read_memory_value<T>(mapped_data_->data_ptr_, idx, key_value_offset_);
      if(curr < prev) {
        throw std::runtime_error("[MemoryMappedDataset] Error: Binary Dataset is not sequential and increasing (not sorted). File: " + bin_filename_ + " on index: " + std::to_string(idx));
      }
      if((curr - prev) != regular_delta) {
        log_warn("[MemoryMappedDataset] record on file [%s] found with irregular delta of step: %f\n", bin_filename_.c_str(), static_cast<double>(curr - prev) / static_cast<double>(regular_delta));
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
  torch::Tensor get(std::size_t index) override {
    if (index >= num_records_) {
      throw std::out_of_range("[MemoryMappedDataset] Index [" + std::to_string(index) + "] out of range [0, " + std::to_string(num_records_) + "), on file: " + bin_filename_);
    }

    // return torch::tensor(read_memory_struct<T>(mapped_data_->data_ptr_, index).tensor_features(), 
    //   torch::TensorOptions().dtype(cuwacunu::piaabo::torch_compat::kType).device(cuwacunu::piaabo::torch_compat::kDevice));
    return torch::tensor(read_memory_struct<T>(mapped_data_->data_ptr_, index).tensor_features(), torch::kDouble);
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
   * @brief Retrieves a record as a tensor by its key value.
   * 
   * @param target_key_value The key value to search for.
   * @return A tensor representing the record's features.
   */
  torch::Tensor get_by_key_value(typename T::key_type_t target_key_value) {
    std::size_t index = find_closest_index(target_key_value);
    return get(index);
  }

  /**
   * @brief Retrives N tensors 
   *    the first tensor is at time t-N and the last is at time t
   *  
   * @param target_key_value The key value to search for.
   * @param N number of record to retrive
   * @return A tensor representing the record's features.
   */
  torch::Tensor get_sequence_ending_at_key_value(typename T::key_type_t target_key_value, std::size_t N) {
    std::size_t index = find_closest_index(target_key_value);

    if(index <= N){
      throw std::out_of_range("[MemoryMappedDataset] Target [" + std::to_string(target_key_value) + "] is to early for size N=" + std::to_string(N) + ", on file: " + bin_filename_);
    }

    auto records = read_memory_structs<T>(mapped_data_->data_ptr_, index, N);

    for(T& rec: records) {
      std::cout << "waka \t" << rec.open_time << std::endl;
    }
    return torch::tensor(records[0].tensor_features(), torch::kDouble);
  }

private:
  /**
   * @brief Finds the closest index for a given key value using interpolation search.
   * 
   * @param target_key_value The target key value to search for.
   * @return The closest index to the target key value.
   * @throws std::runtime_error if the dataset is empty.
   */
  std::size_t find_closest_index(typename T::key_type_t target_key_value) {
    if (num_records_ == 0) {
      throw std::runtime_error("[MemoryMappedDataset] Error: Dataset is empty: " + bin_filename_);
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
      typename T::key_type_t diff = absolute_difference(mid_key_value, target_key_value);
      if (diff < best_diff) {
        best_diff = diff;
        best_index = mid;
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

} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
