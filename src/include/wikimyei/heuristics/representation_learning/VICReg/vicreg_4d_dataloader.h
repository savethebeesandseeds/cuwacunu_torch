#pragma once
#include <torch/torch.h>
#include <vector>
#include <sstream>  // for logging

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

// ---- Forward declaration to ensure the alias resolves even if include order shifts
namespace camahjucunu { namespace data {
  template <typename Q, typename K, typename Td, typename Sampler>
  class MemoryMappedDataLoader;
}}

// View: wraps a raw MemoryMappedDataLoader and attaches .encoding from Model::encode
template<typename Model, typename Q, typename K, typename Td, typename Sampler>
class RepresentationDataLoaderView {
public:
  using RawLoader = cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Q, K, Td, Sampler>;

  struct Iterator {
    using RawIter = decltype(std::declval<RawLoader>().begin());

    Iterator(RawIter it, RawIter end, Model* model, bool use_swa, bool debug)
      : it_(it), end_(end), model_(model), use_swa_(use_swa), debug_(debug) {}

    Iterator& operator++() { ++it_; return *this; }
    bool operator!=(const Iterator& other) const { return it_ != other.it_; }

    // Non-const: LibTorch iterator exposes only non-const operator*()
    K operator*() {
      auto& raw_batch = *it_;          // std::vector<K>

      // 1) Collate ONCE: past + future (and encoding if present)
      K batch = K::collate_fn(raw_batch);

      // 2) Feed ONLY the past to the encoder (no second collation)
      const auto& feats = batch.features;  // [B,C,T,D]
      const auto& mask  = batch.mask;      // [B,C,T]

      if (debug_) {
        auto shape = [](const torch::Tensor& t){
          if (!t.defined()) return std::string("undef");
          std::ostringstream oss; oss << "[";
          for (int64_t i=0;i<t.dim();++i) { if(i) oss<<","; oss<<t.size(i); }
          oss << "] " << t.dtype() << " " << t.device().str();
          return oss.str();
        };
        fprintf(stderr,
          "[RepDLV] feats=%s  mask=%s  fut_feats=%s  fut_mask=%s\n",
          shape(feats).c_str(),
          shape(mask).c_str(),
          shape(batch.future_features).c_str(),
          shape(batch.future_mask).c_str());
      }

      TORCH_CHECK(feats.defined() && mask.defined(),
                  "[RepDLV] past features/mask undefined before encode()");
      TORCH_CHECK(feats.dim()==4 && mask.dim()==3,
                  "[RepDLV] unexpected dims: feats ", feats.dim(), " mask ", mask.dim());
      TORCH_CHECK(feats.size(0)==mask.size(0) && feats.size(2)==mask.size(2),
                  "[RepDLV] B/T mismatch: feats(B,T)=(", feats.size(0), ",", feats.size(2),
                  ") vs mask(B,T)=(", mask.size(0), ",", mask.size(2), ")");

      torch::NoGradGuard ng;
      auto enc = model_->encode(
          feats,              // feed PAST view
          mask,
          /*use_swa=*/use_swa_,
          /*detach_to_cpu=*/false);

      batch.encoding = std::move(enc);
      return batch;           // NRVO / move
    }

  private:
    RawIter it_, end_;
    Model*  model_;
    bool    use_swa_;
    bool    debug_;
  };

  RepresentationDataLoaderView(RawLoader& raw_loader, Model& model,
                               bool use_swa = true, bool debug = false)
    : raw_loader_(raw_loader), model_(model), use_swa_(use_swa), debug_(debug) {}

  Iterator begin() { return Iterator(raw_loader_.begin(), raw_loader_.end(), &model_, use_swa_, debug_); }
  Iterator end()   { return Iterator(raw_loader_.end(),   raw_loader_.end(), &model_, use_swa_, debug_); }

  int64_t batch_size() const { return raw_loader_.batch_size(); }

private:
  RawLoader& raw_loader_;
  Model&     model_;
  bool       use_swa_;
  bool       debug_;
};

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
