/* memory_mapped_dataloader.h */
#pragma once

#include <torch/torch.h>
#include <utility>
#include <vector>
#include <string>
#include <stdexcept>
#include <optional>

#include "piaabo/torch_compat/torch_utils.h"
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"

namespace cuwacunu {
namespace camahjucunu {
namespace data {

/**
 * @brief A class that creates and manages a DataLoader for memory-mapped datasets.
 *
 * @tparam Dataset_t The dataset type. This dataset should be compatible with torch::data::Dataset and return samples of type Datasample_t from its `get(...)` method.
 * @tparam Datasample_t The sample type returned by the dataset (e.g., a struct containing features and mask tensors).
 * @tparam Datatype_t The underlying data type used by the dataset, if required.
 * @tparam Sampler_t The type of Sampler used
 * 
 * This class:
 *  - Constructs the dataset Dataset_t internally (must adjust the constructor call).
 *  - Accepts a custom collate function as a constructor argument.
 *  - Allows specifying batch size, number of workers, and optionally, a different sampler type.
 *  - Provides begin() and end() to iterate over batches and a reset() method to re-start iteration.
 */
template<typename Dataset_t, typename Datasample_t, typename Datatype_t, typename Sampler_t = torch::data::samplers::SequentialSampler>
class MemoryMappedDataLoader {
private:
  /* Define the DataLoader type with the chosen Sampler and sample type Datasample_t */
  using DataLoaderType = torch::data::StatelessDataLoader<Dataset_t, Sampler_t>;
  DataLoaderType data_loader_;
  
  /* Static assertions to validate template parameters */
  static_assert(std::is_base_of<torch::data::datasets::Dataset<Dataset_t, Datasample_t>, Dataset_t>::value, "(memory_mapped_dataloader)[] Dataset_t must derive from torch::data::Dataset");
  static_assert(std::is_base_of<torch::data::samplers::Sampler<>, Sampler_t>::value, "(memory_mapped_dataloader)[] Sampler_t must derive from torch::data::samplers::Sampler");

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
   * - Ensure that Dataset_t has a constructor that matches the arguments provided here.
   * - The dataset Dataset_t should implement size() to return the number of samples.
   */
  MemoryMappedDataLoader(
    Dataset_t& memory_mapped_dataset, 
    const Sampler_t& sampler,
    const torch::data::DataLoaderOptions& options
  ) : data_loader_(memory_mapped_dataset, sampler, options) {
    const auto dataset_size = memory_mapped_dataset.size();
    if (!dataset_size.has_value() || dataset_size.value() == 0) {
      C_ = 0;
      T_ = 0;
      D_ = 0;
      log_warn("[MemoryMappedDataLoader] Dataset is empty; shape probe skipped.\n");
      return;
    }
    Datasample_t prove_sample(memory_mapped_dataset.get(0));
    C_ = prove_sample.features.size(0);
    T_ = prove_sample.features.size(1);
    D_ = prove_sample.features.size(2);
    maybe_emit_policy_warnings_(C_);
    
    prove_sample.reset();
  }

  auto begin()  { return data_loader_.begin(); }
  auto end()    { return data_loader_.end(); }
  void reset()  { data_loader_.reset(); }

  DataLoaderType& inner() { return data_loader_; }
  const DataLoaderType& inner() const { return data_loader_; }

 private:
  static void maybe_emit_policy_warnings_(int64_t channels) {
    static bool warned_multi_channel_step_policy = false;
    static bool warned_high_channel_count = false;

    if (channels > 1 && !warned_multi_channel_step_policy) {
      warned_multi_channel_step_policy = true;
      log_warn(
          "[MemoryMappedDataLoader] Multi-channel stepping follows the highest "
          "channel progression; this is deterministic but may not match all "
          "alignment policies.\n");
    }

    if (channels > 16 && !warned_high_channel_count) {
      warned_high_channel_count = true;
      log_warn(
          "[MemoryMappedDataLoader] High channel count detected (%lld); "
          "multi-channel loading is supported, but validate that your channel "
          "composition policy matches experiment goals.\n",
          static_cast<long long>(channels));
    }
  }
};


/**
 * @brief Method to create a dataloader from an observation_spec_t.
 *
 * @tparam Dataset_t The dataset type. This dataset should be compatible with torch::data::Dataset and return samples of type Datasample_t from its `get(...)` method.
 * @tparam Datasample_t The sample type returned by the dataset (e.g., a struct containing features and mask tensors).
 * @tparam Datatype_t The underlying data type used by the dataset, if required.
 * @tparam Sampler_t The type of Sampler used.
 */
template<typename Dataset_t, typename Datasample_t, typename Datatype_t, typename Sampler_t>
typename std::enable_if<std::is_same<Sampler_t, torch::data::samplers::SequentialSampler>::value, MemoryMappedDataLoader<Dataset_t, Datasample_t, Datatype_t, Sampler_t>>::type
create_memory_mapped_dataloader(
  std::string& instrument, cuwacunu::camahjucunu::observation_spec_t obs_inst, bool force_rebuild_cache = false, std::size_t batch_size = 64, std::size_t workers = 0) {
    /* variables */
    auto dataset          = create_memory_mapped_concat_dataset<Datatype_t>(instrument, obs_inst, force_rebuild_cache);
    auto sampler          = dataset.SequentialSampler();
    auto sampler_options  = dataset.SequentialSampler_options(batch_size, workers);
    /* dataloader */
    return cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Datasample_t, Datatype_t, Sampler_t>(
      /* dataset      */  dataset, 
      /* sampler      */  sampler,
      /* options      */  sampler_options
    );
}

template<typename Dataset_t, typename Datasample_t, typename Datatype_t, typename Sampler_t>
typename std::enable_if<std::is_same<Sampler_t, torch::data::samplers::RandomSampler>::value, MemoryMappedDataLoader<Dataset_t, Datasample_t, Datatype_t, Sampler_t>>::type
create_memory_mapped_dataloader(
  std::string& instrument, cuwacunu::camahjucunu::observation_spec_t obs_inst, bool force_rebuild_cache = false, std::size_t batch_size = 64, std::size_t workers = 0) {
    /* variables */
    auto dataset          = create_memory_mapped_concat_dataset<Datatype_t>(instrument, obs_inst, force_rebuild_cache);
    auto sampler          = dataset.RandomSampler();
    auto sampler_options  = dataset.RandomSampler_options(batch_size, workers);
    /* dataloader */
    return cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Datasample_t, Datatype_t, Sampler_t>(
      /* dataset      */  dataset, 
      /* sampler      */  sampler,
      /* options      */  sampler_options
    );
}


// ----------------------------------------------------------------------------
// Observation-spec DataLoaders (sequential / random)
//
//   auto dl = observation_sequential_mm_dataloader<Datatype_t>("BTCUSDT");
//   auto dl = observation_random_mm_dataloader   <Datatype_t>("BTCUSDT");
// ----------------------------------------------------------------------------
template<typename Datatype_t, typename Sampler>
inline auto make_obs_mm_dataloader(
    std::string_view instrument,
    const cuwacunu::piaabo::dconfig::contract_hash_t& contract_hash)
{
    if (contract_hash.empty()) {
      log_fatal("[memory_mapped_dataloader] missing contract hash for observation dataloader\n");
    }
    using Dataset = MemoryMappedConcatDataset<Datatype_t>;

    // ---- fetch config only once ------------------------------------------
    const bool force_rebuild_cache =
        cuwacunu::piaabo::dconfig::config_space_t::get<bool>(
            "DATA_LOADER", "dataloader_force_rebuild_cache");
    constexpr int batch_size = 64;
    const int  workers     = cuwacunu::piaabo::dconfig::config_space_t::get<int> ("DATA_LOADER","dataloader_workers");

    // ---- make a writable copy for   create_memory_mapped_dataloader ------
    std::string inst{instrument};

    return create_memory_mapped_dataloader<Dataset, observation_sample_t, Datatype_t, Sampler>(
        inst,
        cuwacunu::camahjucunu::decode_observation_spec_from_contract(
            contract_hash),
        force_rebuild_cache,
        batch_size,
        workers
    );
}

/**
 * ---------------------------------- sequential ----------------------------
 * @tparam Datatype_t The underlying data type used by the dataset.
 */
template<typename Datatype_t>
inline auto observation_sequential_mm_dataloader(
    std::string_view instrument,
    const cuwacunu::piaabo::dconfig::contract_hash_t& contract_hash)
{
    using Sampler = torch::data::samplers::SequentialSampler;
    return make_obs_mm_dataloader<Datatype_t, Sampler>(
        instrument, contract_hash);
}

/**
 * ---------------------------------- random --------------------------------
 * @tparam Datatype_t The underlying data type used by the dataset.
 */
template<typename Datatype_t>
inline auto observation_random_mm_dataloader(
    std::string_view instrument,
    const cuwacunu::piaabo::dconfig::contract_hash_t& contract_hash)
{
    using Sampler = torch::data::samplers::RandomSampler;
    return make_obs_mm_dataloader<Datatype_t, Sampler>(
        instrument, contract_hash);
}

} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
