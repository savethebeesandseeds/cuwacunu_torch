/* observation_sample.h */
#pragma once

#include <torch/torch.h>
#include <vector>
#include <string>
#include <stdexcept>

namespace cuwacunu {
namespace camahjucunu {
namespace data {

struct observation_sample_t {
  // past (ends at t)
  torch::Tensor features;        // [B,C,T,D] or [C,T,D] if unbatched
  torch::Tensor mask;            // [B,C,T]   or [C,T]
  // future (starts at t+1)
  torch::Tensor future_features; // [B,Nf,D]  or [Nf,D]
  torch::Tensor future_mask;     // [B,Nf]    or [Nf]
  // optional encoder output
  torch::Tensor encoding;        // [B,De] or [B,T',De] (or undefined)

private:
  /* ---- helpers ----------------------------------------------------------- */
  static inline bool is_batched_past(const observation_sample_t& s) {
    if (s.features.defined()) return s.features.dim() >= 4; // [B,C,T,D]
    if (s.mask.defined())     return s.mask.dim()     >= 3; // [B,C,T]
    return false;
  }
  static inline bool is_batched_future(const observation_sample_t& s) {
    if (s.future_features.defined()) return s.future_features.dim() >= 3; // [B,Nf,D]
    if (s.future_mask.defined())     return s.future_mask.dim()     >= 2; // [B,Nf]
    return false;
  }
  static inline bool is_batched_encoding(const observation_sample_t& s) {
    // Our encoding is documented as batched when defined; be conservative:
    return s.encoding.defined() && s.encoding.dim() >= 2; // [B,De] or [B,T',De]
  }
  static inline bool all_same(bool v0, const std::vector<observation_sample_t>& batch,
                              bool (*pred)(const observation_sample_t&)) {
    for (size_t i = 1; i < batch.size(); ++i) {
      if (pred(batch[i]) != v0) return false;
    }
    return true;
  }
  template <typename V>
  static inline torch::Tensor smart_stack_or_cat(V&& tensors, bool already_batched) {
    if (tensors.empty()) return torch::Tensor();
    if (already_batched) {
      if (tensors.size() == 1) return tensors[0];              // pass-through
      return torch::cat(tensors, /*dim=*/0);                   // merge batches
    } else {
      return torch::stack(tensors, /*dim=*/0);                 // create batch dim
    }
  }

public:
  /* -------- collate: past fields only (features/mask) -------- */
  static inline observation_sample_t collate_fn_past(const std::vector<observation_sample_t>& batch) {
    TORCH_CHECK(!batch.empty(), "[collate_fn_past] empty batch");
    const bool already_batched = is_batched_past(batch.front());
    TORCH_CHECK(all_same(already_batched, batch, &is_batched_past),
                "[collate_fn_past] mix of batched/unbatched samples");

    const auto fsz = batch.front().features.sizes();
    const auto msz = batch.front().mask.sizes();

    std::vector<torch::Tensor> feats; feats.reserve(batch.size());
    std::vector<torch::Tensor> masks; masks.reserve(batch.size());
    for (const auto& s : batch) {
      TORCH_CHECK(s.features.sizes() == fsz, "[collate_fn_past] features mismatch");
      TORCH_CHECK(s.mask.sizes()     == msz, "[collate_fn_past] mask mismatch");
      feats.emplace_back(s.features);
      masks.emplace_back(s.mask);
    }

    return observation_sample_t{
      smart_stack_or_cat(feats, already_batched),  // features
      smart_stack_or_cat(masks, already_batched),  // mask
      torch::Tensor(),                             // future_features
      torch::Tensor(),                             // future_mask
      torch::Tensor()                              // encoding
    };
  }

  /* -------- collate: future fields only (targets) -------- */
  static inline observation_sample_t collate_fn_future(const std::vector<observation_sample_t>& batch) {
    TORCH_CHECK(!batch.empty(), "[collate_fn_future] empty batch");
    const bool already_batched = is_batched_future(batch.front());
    TORCH_CHECK(all_same(already_batched, batch, &is_batched_future),
                "[collate_fn_future] mix of batched/unbatched samples");

    const auto ffsz = batch.front().future_features.sizes();
    const auto fmsz = batch.front().future_mask.sizes();

    std::vector<torch::Tensor> fut_feats; fut_feats.reserve(batch.size());
    std::vector<torch::Tensor> fut_masks; fut_masks.reserve(batch.size());
    for (const auto& s : batch) {
      TORCH_CHECK(s.future_features.sizes() == ffsz, "[collate_fn_future] future_features mismatch");
      TORCH_CHECK(s.future_mask.sizes()     == fmsz, "[collate_fn_future] future_mask mismatch");
      fut_feats.emplace_back(s.future_features);
      fut_masks.emplace_back(s.future_mask);
    }

    return observation_sample_t{
      torch::Tensor(),                                  // features
      torch::Tensor(),                                  // mask
      smart_stack_or_cat(fut_feats, already_batched),   // future_features
      smart_stack_or_cat(fut_masks, already_batched),   // future_mask
      torch::Tensor()                                   // encoding
    };
  }

  /* -------- collate: encodings only (when already present) -------- */
  static inline torch::Tensor collate_fn_encoding(const std::vector<observation_sample_t>& batch) {
    TORCH_CHECK(!batch.empty(), "[collate_fn_encoding] empty batch");
    TORCH_CHECK(batch.front().encoding.defined(), "[collate_fn_encoding] first sample has undefined encoding");
    const bool already_batched = is_batched_encoding(batch.front());
    TORCH_CHECK(all_same(already_batched, batch, &is_batched_encoding),
                "[collate_fn_encoding] mix of batched/unbatched encodings");

    const auto esz = batch.front().encoding.sizes();
    std::vector<torch::Tensor> encs; encs.reserve(batch.size());
    for (const auto& s : batch) {
      TORCH_CHECK(s.encoding.defined(), "[collate_fn_encoding] encoding undefined in some sample");
      TORCH_CHECK(s.encoding.sizes() == esz, "[collate_fn_encoding] encoding shape mismatch");
      encs.emplace_back(s.encoding);
    }
    return smart_stack_or_cat(encs, already_batched);  // [B, De] or [B,T',De]
  }

  /* -------- all fields collator -------- */
  static inline observation_sample_t collate_fn(const std::vector<observation_sample_t>& batch) {
    TORCH_CHECK(!batch.empty(), "[observation_sample_t::collate_fn] empty batch");

    const bool batched_past   = is_batched_past(batch.front());
    const bool batched_future = is_batched_future(batch.front());
    const bool batched_enc    = is_batched_encoding(batch.front());

    TORCH_CHECK(all_same(batched_past,   batch, &is_batched_past),   "[collate_fn] mix in past fields");
    TORCH_CHECK(all_same(batched_future, batch, &is_batched_future), "[collate_fn] mix in future fields");
    TORCH_CHECK(all_same(batched_enc,    batch, &is_batched_encoding), "[collate_fn] mix in encoding fields");

    const auto fsz  = batch.front().features.sizes();
    const auto msz  = batch.front().mask.sizes();
    const auto ffsz = batch.front().future_features.sizes();
    const auto fmsz = batch.front().future_mask.sizes();

    const bool have_enc = batch.front().encoding.defined();
    const auto esz = have_enc ? batch.front().encoding.sizes() : torch::IntArrayRef{};

    std::vector<torch::Tensor> feats, masks, fut_feats, fut_masks, encs;
    feats.reserve(batch.size()); masks.reserve(batch.size());
    fut_feats.reserve(batch.size()); fut_masks.reserve(batch.size());
    if (have_enc) encs.reserve(batch.size());

    for (const auto& s : batch) {
      TORCH_CHECK(s.features.sizes()        == fsz,  "[collate_fn] features mismatch");
      TORCH_CHECK(s.mask.sizes()            == msz,  "[collate_fn] mask mismatch");
      TORCH_CHECK(s.future_features.sizes() == ffsz, "[collate_fn] future_features mismatch");
      TORCH_CHECK(s.future_mask.sizes()     == fmsz, "[collate_fn] future_mask mismatch");
      feats.emplace_back(s.features);
      masks.emplace_back(s.mask);
      fut_feats.emplace_back(s.future_features);
      fut_masks.emplace_back(s.future_mask);

      if (have_enc) {
        TORCH_CHECK(s.encoding.defined(),   "[collate_fn] encoding undefined while first was defined");
        TORCH_CHECK(s.encoding.sizes() == esz, "[collate_fn] encoding mismatch");
        encs.emplace_back(s.encoding);
      }
    }

    observation_sample_t out{
      smart_stack_or_cat(feats,     batched_past),
      smart_stack_or_cat(masks,     batched_past),
      smart_stack_or_cat(fut_feats, batched_future),
      smart_stack_or_cat(fut_masks, batched_future),
      torch::Tensor{} // set below if present
    };
    if (!encs.empty()) out.encoding = smart_stack_or_cat(encs, batched_enc);
    return out;
  }

  /* -------- decollate (unchanged) -------- */
  static inline std::vector<observation_sample_t> decollate_fn(
      const observation_sample_t& batched,
      bool clone_tensors = false) {
    const auto B = batched.features.defined() ? batched.features.size(0)
                 : batched.future_features.defined() ? batched.future_features.size(0)
                 : batched.encoding.defined() ? batched.encoding.size(0)
                 : 0;
    TORCH_CHECK(B > 0, "[decollate_fn] cannot infer batch size; at least one field must be batched");

    auto maybe_split = [&](const torch::Tensor& t) -> std::vector<torch::Tensor> {
      if (!t.defined()) return {};
      return torch::unbind(t, /*dim=*/0);
    };

    auto feats     = maybe_split(batched.features);
    auto masks     = maybe_split(batched.mask);
    auto fut_feats = maybe_split(batched.future_features);
    auto fut_masks = maybe_split(batched.future_mask);
    auto encs      = maybe_split(batched.encoding);

    auto maybe_clone = [&](const torch::Tensor& t) -> torch::Tensor {
      return clone_tensors ? t.clone() : t;
    };

    std::vector<observation_sample_t> out;
    out.reserve(B);
    for (int64_t i = 0; i < B; ++i) {
      observation_sample_t s{
        feats.empty()     ? torch::Tensor() : maybe_clone(feats[i]),
        masks.empty()     ? torch::Tensor() : maybe_clone(masks[i]),
        fut_feats.empty() ? torch::Tensor() : maybe_clone(fut_feats[i]),
        fut_masks.empty() ? torch::Tensor() : maybe_clone(fut_masks[i]),
        encs.empty()      ? torch::Tensor() : maybe_clone(encs[i])
      };
      out.emplace_back(std::move(s));
    }
    return out;
  }

  /* clear all fields */
  void reset() {
    features.reset();
    mask.reset();
    future_features.reset();
    future_mask.reset();
    encoding.reset();
  }
};

} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
