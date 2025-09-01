/* vicreg_4d_dataloader.h
 *
 * A light wrapper that transforms a raw MemoryMappedDataLoader<Q,K,Td,Sampler>
 * into a stream of K-batches whose `.encoding` field is populated by a model.
 *
 * This header is model-agnostic: it does not include the model header to avoid
 * include cycles. Instantiate with the concrete model type (e.g., VICReg_4D).
 */
#pragma once

#include <torch/torch.h>
#include <vector>

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

/**
 * @tparam Model   Encoder-like type exposing:
 *                 - torch::Device device;
 *                 - torch::Tensor encode(Tensor past_features, Tensor past_mask,
 *                                        bool use_swa, bool detach_to_cpu);
 * @tparam Q       Dataset type used by the raw MemoryMappedDataLoader.
 * @tparam K       Sample type with:
 *                   static K collate_fn(const std::vector<K>&)
 *                   static past_batch_t collate_past(const std::vector<K>&),
 *                 where past_batch_t has { Tensor features; Tensor mask; }.
 * @tparam Td      Element type stored by the memory-mapped data files.
 * @tparam Sampler Sampler type used by the raw loader.
 */
template<typename Model, typename Q, typename K, typename Td, typename Sampler>
class RepresentationDataLoaderView {
public:
  using RawLoader = cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Q, K, Td, Sampler>;

  struct Iterator {
    using RawIter = decltype(std::declval<RawLoader>().begin());

    Iterator(RawIter it, RawIter end, Model* model, bool use_swa)
      : it_(it), end_(end), model_(model), use_swa_(use_swa) {}

    Iterator& operator++() { ++it_; return *this; }
    bool operator!=(const Iterator& other) const { return it_ != other.it_; }

    K operator*() const {
      const auto raw_batch = *it_;
      K batch = K::collate_fn(raw_batch);        // past + future

      torch::NoGradGuard ng;
      // Let the model move tensors to its device internally.
      auto enc = model_->encode(
          batch.features,                        // [B,C,T,D]
          batch.mask,                            // [B,C,T]
          /*use_swa=*/use_swa_,
          /*detach_to_cpu=*/false);

      batch.encoding = enc;                      // attach [B,De] or [B,T',De]
      return batch;                              // NRVO/move
    }

  private:
    RawIter it_, end_;
    Model*  model_;
    bool    use_swa_;
  };

  RepresentationDataLoaderView(RawLoader& raw_loader, Model& model, bool use_swa = true)
    : raw_loader_(raw_loader), model_(model), use_swa_(use_swa) {}

  Iterator begin() { return Iterator(raw_loader_.begin(), raw_loader_.end(), &model_, use_swa_); }
  Iterator end()   { return Iterator(raw_loader_.end(),   raw_loader_.end(), &model_, use_swa_); }

  // convenience
  int64_t batch_size() const { return raw_loader_.batch_size(); }

private:
  RawLoader& raw_loader_;
  Model&     model_;
  bool       use_swa_;
};

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
