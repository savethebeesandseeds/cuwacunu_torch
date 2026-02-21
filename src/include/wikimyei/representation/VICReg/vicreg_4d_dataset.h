/* vicreg_4d_encoded_dataset.h */
#pragma once
#include <torch/torch.h>
#include <vector>
#include <type_traits>

#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_dataset.h"

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

/**
 * @brief A view/wrapper over a memory-mapped dataset that ensures
 *        observation_sample_t::encoding is filled using a VICReg_4D model.
 *
 * Works with both:
 *   - camahjucunu::data::MemoryMappedDataset<Datatype_t>
 *   - camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>
 *
 * Assumptions:
 *   - .features  is [C,T,D] (CPU)
 *   - .mask      is [C,T]   (CPU)
 *   - model.encode accepts [B,C,T,D],[B,C,T] and returns [B,De] or [B,T',De]
 */
template <typename EmbeddingModel_t, typename Dataset_t, typename Datasample_t, typename Datatype_t>
class RepresentationDatasetView {
public:
  RepresentationDatasetView(Dataset_t& base,
                            EmbeddingModel_t&       model,
                            torch::Device device,
                            bool use_swa       = true,
                            bool detach_to_cpu = true)
  : base_(&base), model_(&model), device_(device),
    use_swa_(use_swa), detach_cpu_(detach_to_cpu)
  {
    // Copy static metadata from base (handy for UI)
    leftmost_key_value_  = base.leftmost_key_value_;
    rightmost_key_value_ = base.rightmost_key_value_;
    key_value_span_      = base.key_value_span_;
    key_value_step_      = base.key_value_step_;
    num_records_         = base.size().value_or(0);
    // Optional padding params if Dataset_t is MemoryMappedConcatDataset<Datatype_t>
    if constexpr (requires(Dataset_t b){ b.max_N_past_; b.max_N_future_; }) {
      max_N_past_   = base.max_N_past_;
      max_N_future_ = base.max_N_future_;
    }
  }

  // ------- torch Dataset API -------
  torch::optional<std::size_t> size() const override { return base_->size(); }

  Datasample_t get(std::size_t index) override {
    Datasample_t s = base_->get(index);
    ensure_encoding(s);
    return s;
  }

  // ------- Convenience pass-throughs (non-standard Dataset API) -------

  // get_by_key_value : only exists on our memory-mapped datasets; forward when available
  template <typename KeyT>
  Datasample_t get_by_key_value(KeyT key) {
    if constexpr (requires(Dataset_t b, KeyT k){ b.get_by_key_value(k); }) {
      Datasample_t s = base_->get_by_key_value(key);
      ensure_encoding(s);
      return s;
    } else {
      // Fallback: compute anchor index by interpolation
      auto sz = base_->size().value_or(0);
      if (sz == 0) throw std::out_of_range("Empty dataset");
      // Rough index
      double j = double(key - leftmost_key_value_) / double(key_value_step_);
      std::size_t idx = (j <= 0.0) ? 0 : (j >= double(sz-1) ? (sz-1) : (std::size_t)std::llround(j));
      return get(idx);
    }
  }

  // range_samples_by_keys : forward and fill encodings in bulk
  template <typename KeyT>
  std::vector<Datasample_t> range_samples_by_keys(KeyT left, KeyT right) {
    if constexpr (requires(Dataset_t b, KeyT a, KeyT c){ b.range_samples_by_keys(a,c); }) {
      auto v = base_->range_samples_by_keys(left, right);
      for (auto& s : v) ensure_encoding(s);
      return v;
    } else {
      // Fallback: naive linear sweep
      std::vector<Datasample_t> out;
      if (num_records_ == 0) return out;
      if (right < left) std::swap(left, right);
      auto key = leftmost_key_value_;
      for (std::size_t i=0;i<num_records_;++i, key += key_value_step_) {
        if (key < left || key > right) continue;
        out.emplace_back(get(i));
      }
      return out;
    }
  }

  // Public metadata mirror (useful for UI/tests)
  decltype(Dataset_t::leftmost_key_value_)  leftmost_key_value_{};
  decltype(Dataset_t::rightmost_key_value_) rightmost_key_value_{};
  decltype(Dataset_t::key_value_span_)      key_value_span_{};
  decltype(Dataset_t::key_value_step_)      key_value_step_{};
  std::size_t num_records_{0};
  std::size_t max_N_past_{0};
  std::size_t max_N_future_{0};

private:
  void ensure_encoding(Datasample_t& s) {
    if (s.encoding.defined()) return;         // already present

    TORCH_CHECK(s.features.defined() && s.mask.defined(),
                "[RepDatasetView] features/mask undefined");
    TORCH_CHECK((s.features.dim()==3) && (s.mask.dim()==2),
                "[RepDatasetView] expecting unbatched [C,T,D] / [C,T]");

    // Prepare a batch-of-1 on model device
    auto feats_b = s.features.unsqueeze(0).to(device_, /*non_blocking=*/false); // [1,C,T,D]
    auto mask_b  = s.mask.unsqueeze(0).to(device_, /*non_blocking=*/false);     // [1,C,T]

    torch::NoGradGuard ng;
    auto enc_b = model_->encode(feats_b, mask_b, /*use_swa=*/use_swa_, /*detach_to_cpu=*/false);
    auto enc_1 = enc_b.squeeze(0); // [De] or [T',De]

    if (detach_cpu_) enc_1 = enc_1.to(torch::kCPU).contiguous();
    s.encoding = std::move(enc_1);
  }

private:
  Dataset_t* base_;
  EmbeddingModel_t*       model_;
  torch::Device device_;
  bool use_swa_{true};
  bool detach_cpu_{true};
};

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
