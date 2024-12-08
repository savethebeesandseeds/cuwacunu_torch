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
class MemoryMappedDataset : public torch::data::datasets::Dataset<MemoryMappedDataset<T>, std::pair<torch::Tensor, torch::Tensor>> {
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
  std::unique_ptr<MappedData> mapped_data_;     /* Shared pointer to memory-mapped data */
  std::size_t num_records_;                     /* Number of records in the dataset */

public:
  /* Variables for key-value boundaries. */
  std::size_t key_value_offset_;                /* Offset of the key in each record */
  typename T::key_type_t leftmost_key_value_;   /* Smallest key value in the dataset */
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
      log_fatal("[MemoryMappedDataset] Error: Binary file size is not a multiple of struct size. File: %s\n", bin_filename_.c_str());
    }

    /* Ensure the dataset is not empty. */
    if (num_records_ == 0) {
      log_fatal("[MemoryMappedDataset] Error: Binary Dataset is empty. File: %s\n", bin_filename_.c_str());
    }

    /* Read the boundary key values for validation. */
    leftmost_key_value_ = read_memory_value<T>(mapped_data_->data_ptr_, 0, key_value_offset_);
    rightmost_key_value_ = read_memory_value<T>(mapped_data_->data_ptr_, num_records_ - 1, key_value_offset_);

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
    typename T::key_type_t regular_delta = curr - prev;
    
    /* validate regular delta */
    if(regular_delta <= 0) {
      log_fatal("[MemoryMappedDataset] Error: negative or zero regular_delta. File: %s. \n", bin_filename_.c_str());
    }

    /* validate all the records in the binary file */
    for(std::size_t idx = 1; idx < num_records_; idx++) {
      typename T::key_type_t curr = read_memory_value<T>(mapped_data_->data_ptr_, idx, key_value_offset_);
      if(curr < prev) {
        log_fatal("[MemoryMappedDataset] Error: Binary Dataset is not sequential and increasing (not sorted). File: %s, on index: %ld\n", bin_filename_.c_str(), idx);
      }
      if((curr - prev) != regular_delta) {
        log_warn("[MemoryMappedDataset] record on file [%s] found with irregular delta of step at index [%ld]: %f != %f\n", bin_filename_.c_str(), idx, static_cast<double>(curr - prev), static_cast<double>(regular_delta));
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
  std::pair<torch::Tensor, torch::Tensor> get(std::size_t index) override {
    if (index >= num_records_) {
      log_fatal("[MemoryMappedDataset] Index [%ld] out of range [0, %ld] on file %s\n", index, num_records_, bin_filename_.c_str());
    }
    
    T data_ = read_memory_struct<T>(mapped_data_->data_ptr_, index);
    return {torch::tensor(data_.tensor_features(), torch::kDouble), torch::tensor(data_.is_valid(), torch::kBool)};
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
  std::pair<torch::Tensor, torch::Tensor> get_by_key_value(typename T::key_type_t target_key_value) {
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
  std::pair<torch::Tensor, torch::Tensor> get_sequence_ending_at_key_value(typename T::key_type_t target_key_value, std::size_t N) {
    std::size_t index = find_closest_index(target_key_value);

    if (index < N) {
      throw std::out_of_range("[MemoryMappedDataset] Target [" + std::to_string(target_key_value) + "] is too early for size N=" + std::to_string(N) + ", on file: " + bin_filename_);
    }

    /* Read the memory */
    std::vector<T> records = read_memory_structs<T>(mapped_data_->data_ptr_, index - N + 1, N);

    std::size_t feature_dim = records[0].tensor_features().size();

    /* Preallocate the tensors with the desired shapes */
    torch::Tensor features_tensor = torch::empty({static_cast<long>(N), static_cast<long>(feature_dim)}, torch::kDouble);
    torch::Tensor mask_tensor = torch::empty({static_cast<long>(N)}, torch::kBool);

    /* Get pointers to the tensors' underlying data */
    double* tensor_data = features_tensor.data_ptr<double>();
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
   * @brief Finds the closest index for a given key value using interpolation search.
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
class MemoryMappedConcatDataset : public torch::data::datasets::Dataset<MemoryMappedConcatDataset<T>, std::pair<torch::Tensor, torch::Tensor>> {
private:
  std::vector<std::unique_ptr<MemoryMappedDataset<T>>> datasets_;
  std::vector<std::string> file_names_;
  std::vector<std::size_t> Ns_;
  std::mutex concat_dataset_mutex;

public:
  std::size_t max_N;
  typename T::key_type_t leftmost_key_value_;
  typename T::key_type_t rightmost_key_value_;

public:
  MemoryMappedConcatDataset() {}

  std::pair<torch::Tensor, torch::Tensor> get(std::size_t index) override {
    log_fatal("[MemoryMappedConcatDataset] .get() operation is not permited on MemoryMappedConcatDataset, use .get_by_key_value() instead, where datasets are expected to be in sync. \n");
    return {torch::empty({0}, torch::kDouble), torch::empty({0}, torch::kBool)};
  }

  torch::optional<std::size_t> size() const override {
    return c10::nullopt;
  }

  /**
   * @brief Retrieves a record as a tensor by its key value.
   * 
   * @param target_key_value The key value to search for.
   * @return A tensor representing the record's features.
   */
  std::pair<torch::Tensor, torch::Tensor> get_by_key_value(typename T::key_type_t target_key_value) {
    {LOCK_GUARD(concat_dataset_mutex);} /* just asure the mutex is not active */

    size_t num_sources = datasets_.size();

    std::vector<at::Tensor> features_list(num_sources);
    std::vector<at::Tensor> mask_list(num_sources);

    /* Use a parallel algorithm */
    std::for_each(std::execution::par, datasets_.begin(), datasets_.end(),
      [target_key_value, this, &features_list, &mask_list](auto& dataset) {
        size_t index = &dataset - &datasets_[0];
        auto [features, mask] = dataset->get_by_key_value(target_key_value);
        features_list[index] = features;
        mask_list[index] = mask;
      });

    return {torch::stack(features_list, 0), torch::stack(mask_list, 0)};
  }

  /**
   * @brief Retrieves sequences ending at a given key value from multiple datasets.
   *        The sequences are padded to the maximum sequence length (max_N), and a mask tensor is returned.
   *
   * @param target_key_value The key value to search for.
   * @return A pair of tensors: the dense tensor and the mask tensor.
   */
  std::pair<torch::Tensor, torch::Tensor> get_sequence_ending_at_key_value(typename T::key_type_t target_key_value) {
    {LOCK_GUARD(concat_dataset_mutex);} /* Ensure the mutex is not active */

    size_t num_sources = datasets_.size();

    std::vector<torch::Tensor> tensor_list(num_sources);
    std::vector<torch::Tensor> mask_list(num_sources);

    /* Use a parallel algorithm */
    std::for_each(std::execution::par, datasets_.begin(), datasets_.end(),
      [target_key_value, this, &tensor_list, &mask_list](auto& dataset) {
        size_t index = &dataset - &datasets_[0];
        int64_t N = static_cast<int64_t>(Ns_[index]);

        /* Retrieve the sequence and the mask from the dataset */
        auto [sequence, sequence_mask] = dataset->get_sequence_ending_at_key_value(target_key_value, N); /* Shapes: (N, feature_size), (N) */

        /* Pad the sequence and the mask to max_N length */
        if (N < static_cast<int64_t>(max_N)) {
          int64_t pad_length = static_cast<int64_t>(max_N - N);

          /* Pad the sequence tensor at the beginning to preserve temporal meaning, the last value is the last timestep */
          torch::Tensor pad_sequence = torch::zeros({pad_length, sequence.size(1)}, sequence.options());
          tensor_list[index] = torch::cat({pad_sequence, sequence}, 0); /* Shape: (max_N, feature_size) */

          /* Pad the mask tensor at the beginning to preserve temporal meaning, the last value is the last timestep */
          torch::Tensor pad_mask = torch::zeros({pad_length}, sequence_mask.options());
          mask_list[index] = torch::cat({pad_mask, sequence_mask}, 0); /* Shape: (max_N) */
        } else {
          /* No padding needed */
          tensor_list[index] = sequence;
          mask_list[index] = sequence_mask;
        }
      });

    /* Stack tensors to get the final dense tensor and mask tensor */
    torch::Tensor dense_tensor = torch::stack(tensor_list, 0); /* Shape: (num_sources, max_N, feature_size) */
    torch::Tensor mask_tensor = torch::stack(mask_list, 0); /* Shape: (num_sources, max_N) */

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
   * @param N The number of elements expected to be loaded from the dataset.
   * @param normalization_window rolling window of statistics to normalize the data, (set to zero to avoid normalization).
   * @param force_binarization Whether to force binary conversion of the CSV file.
   * @param buffer_size Buffer size used during the binary conversion process (default: 1024).
   * @param delimiter Delimiter character used in the CSV file (default: ',').
   *
   */
  void add_dataset(const std::string csv_filename, std::size_t N, std::size_t normalization_window = 0, bool force_binarization = false, size_t buffer_size = 1024, char delimiter = ',') {
    LOCK_GUARD(concat_dataset_mutex);

  /* --- --- --- prepare the file --- --- --- */
    /* binarize the csv file */
    std::string bin_filename = prepare_binary_file_from_csv<T>(csv_filename, force_binarization, buffer_size, delimiter);
    
  /* --- --- --- adding the dataset --- --- --- */
    /* append the file_name */
    file_names_.push_back(bin_filename);

    /* append the dataset */
    datasets_.push_back(
      std::make_unique<MemoryMappedDataset<T>>(file_names_.back())
    );

    /* append count */
    Ns_.push_back(N);
    auto& new_dataset_ = datasets_.back();

    /* Compute max_N, the maximum sequence length across all datasets */
    max_N = *std::max_element(Ns_.begin(), Ns_.end());
    std::cout << max_N << std::endl;
    
    /* update the leftmost and rightmost */
    leftmost_key_value_  = MAX(leftmost_key_value_,  new_dataset_->leftmost_key_value_ );
    rightmost_key_value_ = MIN(rightmost_key_value_, new_dataset_->rightmost_key_value_);

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
  
  /* --- --- --- Normalize --- --- --- */
    if(normalization_window != 0) {
      normalize_binary_file<T>(bin_filename, normalization_window);
    }

    /* validate file */
    log_info("waka STR ----------------------------------------- \n");
    std::vector<T> valdiation_vector = cuwacunu::piaabo::dfiles::binaryFile_to_vector<T>(bin_filename, buffer_size);
    std::size_t count = 0;
    for(T& validation_item: valdiation_vector) {
      if(count > (new_dataset_->size().value() - static_cast<long unsigned int>(10)) ) {
        validation_item.to_csv(std::cout, delimiter);
        std::cout << std::endl;
      }
      count++;
    }
    log_info("waka END ----------------------------------------- \n");
  }
};

} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
