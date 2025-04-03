/* memory_mapped_dataloader.h */
#pragma once

#include <torch/torch.h>
#include <utility>
#include <vector>
#include <string>
#include <stdexcept>

#include "piaabo/torch_compat/torch_utils.h"
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/BNF/implementations/training_pipeline/training_pipeline.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

namespace cuwacunu {
namespace camahjucunu {
namespace data {

/**
 * @brief A class that creates and manages a DataLoader for memory-mapped datasets.
 *
 * @tparam Q The dataset type. This dataset should be compatible with torch::data::Dataset and return samples of type K from its `get(...)` method.
 * @tparam K The sample type returned by the dataset (e.g., a struct containing features and mask tensors).
 * @tparam T The underlying data type used by the dataset, if required.
 * @tparam S The type of Sampler used
 * 
 * This class:
 *  - Constructs the dataset Q internally (you must adjust the constructor call).
 *  - Accepts a custom collate function as a constructor argument.
 *  - Allows specifying batch size, number of workers, and optionally, a different sampler type.
 *  - Provides begin() and end() to iterate over batches and a reset() method to re-start iteration.
 */
template<typename Q, typename K, typename T, typename S = torch::data::samplers::SequentialSampler>
class MemoryMappedDataLoader {
private:
  /* Define the DataLoader type with the chosen Sampler and sample type K */
  using DataLoaderType = torch::data::StatelessDataLoader<Q, S>;
  DataLoaderType data_loader_;
  
  /* Static assertions to validate template parameters */
  static_assert(std::is_base_of<torch::data::datasets::Dataset<Q, K>, Q>::value, "(memory_mapped_dataloader)[] Q must derive from torch::data::Dataset");
  static_assert(std::is_base_of<torch::data::samplers::Sampler<>, S>::value, "(memory_mapped_dataloader)[] S must derive from torch::data::samplers::Sampler");

public:

  // int64_t B; /* batch size */ // not available, sampler is not garantee to have a constant batch size
  int64_t C_;    /* num of channels */
  int64_t T_;    /* time span */
  int64_t D_;    /* dimensionality of sample */

  /**
   * @brief Construct a new MemoryMappedDataLoader object.
   *
   * @param memory_mapped_dataset Dataset.
   * @param sampler The sampler to be used.
   * @param options DataLoaderOptions to configure the DataLoader.
   *
   * Note:
   * - Ensure that Q has a constructor that matches the arguments you provide here.
   * - The dataset Q should implement size() to return the number of samples.
   */
  MemoryMappedDataLoader(
    Q& memory_mapped_dataset, 
    const S& sampler,
    const torch::data::DataLoaderOptions& options
  ) : data_loader_(memory_mapped_dataset, sampler, options) {
    K prove_sample(memory_mapped_dataset.get(0));
    C_ = prove_sample.features.size(0);
    T_ = prove_sample.features.size(1);
    D_ = prove_sample.features.size(2);
    
    prove_sample.reset();
  }

  auto begin()  { return data_loader_.begin(); }
  auto end()    { return data_loader_.end(); }
  void reset()  { data_loader_.reset(); }

  DataLoaderType& operator*()   { return data_loader_; }
  DataLoaderType* operator->()  { return &data_loader_; }
};


/**
 * @brief Method to create a dataloader from an observation_instruction_t.
 *
 * @tparam Q The dataset type. This dataset should be compatible with torch::data::Dataset and return samples of type K from its `get(...)` method.
 * @tparam K The sample type returned by the dataset (e.g., a struct containing features and mask tensors).
 * @tparam T The underlying data type used by the dataset, if required.
 * @tparam S The type of Sampler used.
 */
template<typename Q, typename K, typename T, typename S>
typename std::enable_if<std::is_same<S, torch::data::samplers::SequentialSampler>::value, MemoryMappedDataLoader<Q, K, T, S>>::type
create_memory_mapped_dataloader(
  std::string& instrument, cuwacunu::camahjucunu::BNF::observation_instruction_t obs_inst, bool force_binarization = false, std::size_t batch_size = 64, std::size_t workers = 4) {
    /* variables */
    auto dataset          = create_memory_mapped_concat_dataset<T>(instrument, obs_inst, force_binarization);
    auto sampler          = dataset.SequentialSampler();
    auto sampler_options  = dataset.SequentialSampler_options(batch_size, workers);
    /* dataloader */
    return cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Q, K, T, S>(
      /* dataset      */  dataset, 
      /* sampler      */  sampler,
      /* options      */  sampler_options
    );
}

template<typename Q, typename K, typename T, typename S>
typename std::enable_if<std::is_same<S, torch::data::samplers::RandomSampler>::value, MemoryMappedDataLoader<Q, K, T, S>>::type
create_memory_mapped_dataloader(
  std::string& instrument, cuwacunu::camahjucunu::BNF::observation_instruction_t obs_inst, bool force_binarization = false, std::size_t batch_size = 64, std::size_t workers = 4) {
    /* variables */
    auto dataset          = create_memory_mapped_concat_dataset<T>(instrument, obs_inst, force_binarization);
    auto sampler          = dataset.RandomSampler();
    auto sampler_options  = dataset.RandomSampler_options(batch_size, workers);
    /* dataloader */
    return cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Q, K, T, S>(
      /* dataset      */  dataset, 
      /* sampler      */  sampler,
      /* options      */  sampler_options
    );
}

} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
