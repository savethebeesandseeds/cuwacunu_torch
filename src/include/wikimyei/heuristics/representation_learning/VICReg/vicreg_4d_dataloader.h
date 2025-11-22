/* vicreg_4d_dataloader.h */
#pragma once
#include <torch/torch.h>
#include <vector>
#include <sstream>  // for logging
#include <algorithm>

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"
#include "camahjucunu/data/observation_sample.h"

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

// ---- Forward declaration to ensure the alias resolves even if include order shifts
namespace camahjucunu { namespace data {
  template <typename Dataset_t, typename Datasample_t, typename Datatype_t, typename Sampler>
  class MemoryMappedDataLoader;
}}

// View: wraps a raw MemoryMappedDataLoader and attaches .encoding from EmbeddingModel_t::encode,
// but **skips compute** if samples already provide encoding.
template<typename EmbeddingModel_t, typename Dataset_t, typename Datasample_t, typename Datatype_t, typename Sampler>
class RepresentationDataloaderView {
public:
  using RawLoader = cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Datasample_t, Datatype_t, Sampler>;

  struct Iterator {
    using RawIter = decltype(std::declval<RawLoader>().begin());

    Iterator(RawIter it, RawIter end, EmbeddingModel_t* model, bool use_swa, bool debug)
      : it_(it), end_(end), model_(model), use_swa_(use_swa), debug_(debug) {}

    Iterator& operator++() { ++it_; return *this; }
    bool operator!=(const Iterator& other) const { return it_ != other.it_; }

    Datasample_t operator*() {
      auto& raw_batch = *it_;          // std::vector<Datasample_t>

      // Fast path: if EVERY sample already has encoding, just collate & return.
      const bool all_have_enc = std::all_of(raw_batch.begin(), raw_batch.end(),
        [](const Datasample_t& s){ return s.encoding.defined(); });

      if (all_have_enc) {
        // Full collate preserves encodings (see observation_sample_t::collate_fn).
        return Datasample_t::collate_fn(raw_batch);
      }

      // Otherwise: compute encoding once for the batch from the PAST only.
      // 1) Collate full batch (for features/future to pass through)
      Datasample_t batch = Datasample_t::collate_fn(raw_batch);

      // 2) Also collate PAST-only view for encode()
      Datasample_t batch_past = Datasample_t::collate_fn_past(raw_batch);
      const auto& feats = batch_past.features;  // [B,C,T,D]
      const auto& mask  = batch_past.mask;      // [B,C,T]

      if (debug_) {
        auto shape = [](const torch::Tensor& t){
          if (!t.defined()) return std::string("undef");
          std::ostringstream oss; oss << "[";
          for (int64_t i=0;i<t.dim();++i) { if(i) oss<<","; oss<<t.size(i); }
          oss << "] " << t.dtype() << " " << t.device().str();
          return oss.str();
        };
        fprintf(stderr,
          "[RepDLV] feats=%s  mask=%s  fut_feats=%s  fut_mask=%s  (computing encoding)\n",
          shape(feats).c_str(),
          shape(mask).c_str(),
          shape(batch.future_features).c_str(),
          shape(batch.future_mask).c_str());
      }

      TORCH_CHECK(feats.defined() && mask.defined(),
                  "[RepDLV] past features/mask undefined before encode()");
      TORCH_CHECK(feats.dim()==4 && mask.dim()==3,
                  "[RepDLV] unexpected dims: feats ", feats.dim(), " mask ", mask.dim());

      torch::NoGradGuard ng;
      auto enc = model_->encode(
          feats,              // [B,C,T,D]
          mask,               // [B,C,T]
          /*use_swa=*/use_swa_,
          /*detach_to_cpu=*/false);

      batch.encoding = std::move(enc);
      return batch;           // NRVO / move
    }

  private:
    RawIter it_, end_;
    EmbeddingModel_t*  model_;
    bool    use_swa_;
    bool    debug_;
  };

  RepresentationDataloaderView(RawLoader& raw_loader, EmbeddingModel_t& model,
                               bool use_swa = true, bool debug = false)
    : raw_loader_(raw_loader), model_(model), use_swa_(use_swa), debug_(debug) {}

  Iterator begin() { return Iterator(raw_loader_.begin(), raw_loader_.end(), &model_, use_swa_, debug_); }
  Iterator end()   { return Iterator(raw_loader_.end(),   raw_loader_.end(), &model_, use_swa_, debug_); }

private:
  RawLoader& raw_loader_;
  EmbeddingModel_t&     model_;
  bool       use_swa_;
  bool       debug_;
};

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
