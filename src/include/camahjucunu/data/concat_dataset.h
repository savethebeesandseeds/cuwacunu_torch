
/* memory_mapped_dataset.h */
#pragma once

#include <torch/torch.h>
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
 * @brief A concat dataset for PyTorch-based data loading.
 *
 * This class implements a dataset that ...
 */
struct observation_pipeline_instruction_t {
    std::string instruction;
    std::string symbol;
    std::vector<input_form_t> items;
};
template <typename T>
class ConcatDataset : public torch::data::datasets::Dataset<ConcatDataset<T>, torch::Tensor> {

private:
  observation_pipeline_instruction_t obs_instruct_;     /* instruction */
  std::vector<cuwacunu::camahjucunu::data::MemoryMappedDataset<T>> datasets;
public:
  /**
   * @brief Constructs the dataset that joins multiple other datasets acording to instruction. 
   * 
   * @param obs_instruct Instructor of how data is supposed to be read
   */
  ConcatDataset(observation_pipeline_instruction_t& obs_instruct)
    : obs_instruct_(obs_instruct) {
        auto datasets = cuwacunu::camahjucunu::data::MemoryMappedDataset<T>(bin_filename);
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

    ... paralellize

    torch::Tensor get_sequence_ending_at_key_value(typename T::key_type_t target_key_value, std::size_t N)

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
};

} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
