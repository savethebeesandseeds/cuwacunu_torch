/* memory_mapped_dataset.h */
#pragma once
#include <torch/torch.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <mutex>
#include "piaabo/dutils.h"
#include "piaabo/dlarge_files.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

namespace cuwacunu {
namespace camahjucunu {
namespace data {

/**
 * @brief A memory-mapped dataset for efficient data loading in PyTorch.
 *
 * The `MemoryMappedDataset` class is a custom dataset that reads data from a memory-mapped binary file.
 * It is designed for efficient data loading when dealing with large datasets that do not fit entirely into RAM.
 *
 * @tparam T The type of the records stored in the binary file. The type `T` must have a method called
 *       `tensor_features()` that returns a `std::vector<double>`.
 *
 * @note The binary file must contain records of type `T` packed sequentially. The size of the file must be
 *     a multiple of `sizeof(T)`.
 *
 */
template <typename T>
class MemoryMappedDataset : public torch::data::datasets::Dataset<MemoryMappedDataset<T>, torch::Tensor> {
private:
  std::string bin_filename_;
  size_t num_records_;
  int fd_;
  void* data_ptr_;
  size_t file_size_;

public:
  MemoryMappedDataset(const std::string& bin_filename) : bin_filename_(bin_filename) {
    /* Open the file */
    fd_ = open(bin_filename_.c_str(), O_RDONLY);
    if (fd_ == -1) {
      throw std::runtime_error("[MemoryMappedDataset] Error: Could not open binary file: " + bin_filename_);
    }

    /* Get file size */
    file_size_ = lseek(fd_, 0, SEEK_END);
    lseek(fd_, 0, SEEK_SET);

    /* Calculate number of records */
    num_records_ = file_size_ / sizeof(T);
    if (file_size_ % sizeof(T) != 0) {
      close(fd_);
      throw std::runtime_error("[MemoryMappedDataset] Error: Binary file size is not a multiple of struct size. Of file: " + bin_filename_);
    }

    /* Memory-map the file */
    data_ptr_ = mmap(nullptr, file_size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (data_ptr_ == MAP_FAILED) {
      close(fd_);
      throw std::runtime_error("[MemoryMappedDataset] Error: Failed to memory-map the file: " + bin_filename_);
    }
  }

  ~MemoryMappedDataset() {
    munmap(data_ptr_, file_size_);
    close(fd_);
  }

  torch::Tensor get(size_t index) override {
    if (index >= num_records_) {
      throw std::out_of_range("[MemoryMappedDataset] Index [" + std::to_string(index) + "] out of range [0, " + std::to_string(num_records_) + "), on file: " + bin_filename_);
    }

    /* Access the record directly */
    const T* record_ptr = reinterpret_cast<const T*>(static_cast<const char*>(data_ptr_) + index * sizeof(T));
    const T& record = *record_ptr;

    /* Convert the record to a tensor */
    return torch::tensor(record.tensor_features(), torch::kDouble);
  }

  torch::optional<size_t> size() const override {
    return num_records_;
  }
};

} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
