
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
class ConcatDataset : public torch::data::datasets::Dataset<ConcatDataset<T>, torch::Tensor> {

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
  ConcatDataset(const std::string& bin_filename)
      : bin_filename_(bin_filename),
        mapped_data_(std::make_unique<MappedData>(bin_filename)),
        num_records_(mapped_data_->file_size_ / sizeof(T)),
        key_value_offset_(T::key_offset()) {
    /* Ensure file size is a multiple of the struct size. */
    if (mapped_data_->file_size_ % sizeof(T) != 0) {
      throw std::runtime_error("[ConcatDataset] Error: Binary file size is not a multiple of struct size. File: " + bin_filename_);
    }

    /* Ensure the dataset is not empty. */
    if (num_records_ == 0) {
      throw std::runtime_error("[ConcatDataset] Error: Binary Dataset is empty. File: " + bin_filename_);
    }

    /* Read the boundary key values for validation. */
    leftmost_key_value_ = read_memory_value<T>(mapped_data_->data_ptr_, 0, key_value_offset_);
    rightmost_key_value_ = read_memory_value<T>(mapped_data_->data_ptr_, num_records_ - 1, key_value_offset_);

    /* Validate that the dataset is sorted. */
    if (leftmost_key_value_ >= rightmost_key_value_) {
      throw std::runtime_error("[ConcatDataset] Error: Binary Dataset is not sorted correctly. File: " + bin_filename_);
    }

    /* Ensure the required methods are present on the template */
    static_assert(std::is_same_v<decltype(std::declval<T>().tensor_features()), std::vector<double>>, "[ConcatDataset] Error: Template argument must provide tensor_features() returning std::vector<double>");
    static_assert(std::is_standard_layout<T>::value, "[ConcatDataset] Error: Template argument must be a standard layout type");
    static_assert(std::is_trivially_copyable<T>::value, "[ConcatDataset] Error: Template argument must be trivially copyable");

    /* navigate the entire file to validate the data is sequential and increasing in the key space */
    typename T::key_type_t prev = read_memory_value<T>(mapped_data_->data_ptr_, 0, key_value_offset_);
    for(std::size_t idx = 1; idx < num_records_; idx++) {
      typename T::key_type_t curr = read_memory_value<T>(mapped_data_->data_ptr_, idx, key_value_offset_);
      if(curr < prev) {
        throw std::runtime_error("[ConcatDataset] Error: Binary Dataset is not sequential and increasing (not sorted). File: " + bin_filename_ + " on index: " + std::to_string(idx));
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
      throw std::out_of_range("[ConcatDataset] Index [" + std::to_string(index) + "] out of range [0, " + std::to_string(num_records_) + "), on file: " + bin_filename_);
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

    auto records = read_memory_structs<T>(mapped_data_->data_ptr_, index, N);
    for(T& rec: records) {
      std::cout << "\t" << rec.open_time << std::endl;
    }
    return torch::tensor(records[0].tensor_features(), torch::kDouble);
  }
};

} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
