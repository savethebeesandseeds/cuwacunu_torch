/* observation_sample.h */
#pragma once

#include <torch/torch.h>
#include <vector>
#include <string>
#include <stdexcept>

#include "piaabo/dutils.h"

namespace cuwacunu {
namespace camahjucunu {
namespace data {

struct observation_sample_t {
  // past (ends at t)
  torch::Tensor features;        // [B,C,T,D] or [C,T,D] or [T,D] if unbatched/single
  torch::Tensor mask;            // [B,C,T]   or [C,T]   or [T]

  // future (starts at t+1) — same channeling as past, different time length Tf
  torch::Tensor future_features; // [B,C,Tf,D] or [C,Tf,D] or [Tf,D]
  torch::Tensor future_mask;     // [B,C,Tf]   or [C,Tf]   or [Tf]

  // encoder output
  torch::Tensor encoding;        // [B,De] or [B,T',De] (or undefined)

  // ---------- normalization toggle ----------
  // Whether 'features' / 'future_features' are currently in normalized space.
  bool normalized = false;

  // Per-feature stats used for (de)normalization. Keep broadcastable shapes.
  // Typical usage: shape [D]. Will broadcast over [*,*,T,D].
  torch::Tensor feature_mean;    // same dtype/device as features
  torch::Tensor feature_std;     // same dtype/device as features

  // ---------- time keys ----------
  // Keys/timestamps aligned with past/future sequences.
  // Single dataset: [T] / [Tf]
  // Concat (C channels): [C,T] / [C,Tf]
  // Batched variants broadcast a leading [B] if collated externally.
  torch::Tensor past_keys;
  torch::Tensor future_keys;

private:
  /* ---- helpers ----------------------------------------------------------- */

  static inline bool is_batched_past(const observation_sample_t& s) {
    if (s.features.defined()) return s.features.dim() >= 4; // [B,C,T,D]
    if (s.mask.defined())     return s.mask.dim()     >= 3; // [B,C,T]
    return false;
  }

  // Future is considered "batched" iff past is batched AND future has a leading B
  // that matches the past's B. Otherwise (including unbatched past), treat future
  // as UNBATCHED ([C,Tf,D]/[C,Tf] or [Tf,D]/[Tf]).
  static inline bool is_batched_future(const observation_sample_t& s) {
    const bool past_batched = is_batched_past(s);
    if (!past_batched) {
      return false; // unbatched futures are [C,Tf,D]/[C,Tf] or [Tf,D]/[Tf]
    }

    int64_t B = 0;
    if (s.features.defined() && s.features.dim() >= 4) B = s.features.size(0);
    else if (s.mask.defined() && s.mask.dim() >= 3)    B = s.mask.size(0);
    if (B == 0) return false;

    if (s.future_features.defined()) {
      const auto d = s.future_features.dim();        // expect 4 when batched
      if (d >= 4) return s.future_features.size(0) == B;
    }
    if (s.future_mask.defined()) {
      const auto d = s.future_mask.dim();            // expect 3 when batched
      if (d >= 3) return s.future_mask.size(0) == B;
    }
    return false;
  }

  static inline bool is_batched_encoding(const observation_sample_t& s) {
    return s.encoding.defined() && s.encoding.dim() >= 2; // [B,De] or [B,T',De]
  }

  // Keys should mirror the non-feature dims of their aligned tensor:
  // features [..,D] -> keys [..], mask [..] -> keys [..].
  static inline int64_t expected_key_dim_from_past(const observation_sample_t& s) {
    if (s.features.defined()) {
      TORCH_CHECK(s.features.dim() >= 2, "[observation_sample_t] invalid past features dim");
      return s.features.dim() - 1;
    }
    if (s.mask.defined()) {
      TORCH_CHECK(s.mask.dim() >= 1, "[observation_sample_t] invalid past mask dim");
      return s.mask.dim();
    }
    return -1;
  }

  static inline int64_t expected_key_dim_from_future(const observation_sample_t& s) {
    if (s.future_features.defined()) {
      TORCH_CHECK(s.future_features.dim() >= 2, "[observation_sample_t] invalid future_features dim");
      return s.future_features.dim() - 1;
    }
    if (s.future_mask.defined()) {
      TORCH_CHECK(s.future_mask.dim() >= 1, "[observation_sample_t] invalid future_mask dim");
      return s.future_mask.dim();
    }
    return -1;
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
      if (tensors.size() == 1) return tensors[0];
      return torch::cat(tensors, /*dim=*/0);     // merge along B
    } else {
      return torch::stack(tensors, /*dim=*/0);   // create B
    }
  }

  static inline torch::Tensor ensure_like(const torch::Tensor& src, const torch::Tensor& ref) {
    if (!src.defined()) return src;
    auto t = src;
    if (t.dtype() != ref.dtype()) t = t.to(ref.dtype());
    if (t.device() != ref.device()) t = t.to(ref.device());
    return t;
  }

public:
  /* =====================  (B) normalization helpers  ===================== */

  // Return true if future observations have *any* valid values (mask==true).
  // Useful in realtime, where future may be unknown (empty or fully masked).
  bool has_future_values() const {
    if (future_mask.defined()) {
      // any() is cheap on small tensors used for viz/inspection
      return future_mask.numel() > 0 && future_mask.any().item<bool>();
    }
    // Fallback if mask is not present but future_features exist
    return future_features.defined() && future_features.numel() > 0;
  }

  // In-place normalization using stored stats.
  observation_sample_t& normalize_inplace(double eps = 1e-12) {
    if (normalized) return *this;
    if (!feature_mean.defined() || !feature_std.defined()) return *this;

    auto use_on = [&](torch::Tensor& x) {
      if (!x.defined()) return;
      auto mu  = ensure_like(feature_mean, x);
      auto sig = ensure_like(feature_std,  x).clamp_min(eps);
      x = (x - mu) / sig;
    };
    use_on(features);
    use_on(future_features);
    normalized = true;
    return *this;
  }

  // In-place de-normalization using stored stats.
  observation_sample_t& denormalize_inplace() {
    if (!normalized) return *this;
    if (!feature_mean.defined() || !feature_std.defined()) return *this;

    auto use_on = [&](torch::Tensor& x) {
      if (!x.defined()) return;
      auto mu  = ensure_like(feature_mean, x);
      auto sig = ensure_like(feature_std,  x);
      x = x * sig + mu;
    };
    use_on(features);
    use_on(future_features);
    normalized = false;
    return *this;
  }

  /* =====================  existing collate utilities  ==================== */

  /* -------- collate: past fields only -------- */
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
      smart_stack_or_cat(feats, already_batched),
      smart_stack_or_cat(masks, already_batched),
      torch::Tensor(), torch::Tensor(), torch::Tensor(),
      /* normalized */ false,
      /* mean/std  */ torch::Tensor(), torch::Tensor(),
      /* keys      */ torch::Tensor(), torch::Tensor()
    };
  }

  /* -------- collate: future fields only -------- */
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
      torch::Tensor(), torch::Tensor(),
      smart_stack_or_cat(fut_feats, already_batched),
      smart_stack_or_cat(fut_masks, already_batched),
      torch::Tensor(),
      /* normalized */ false,
      /* mean/std  */ torch::Tensor(), torch::Tensor(),
      /* keys      */ torch::Tensor(), torch::Tensor()
    };
  }

  /* -------- collate: encodings only -------- */
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
    return smart_stack_or_cat(encs, already_batched);  // [B,De] or [B,T',De]
  }

  /* -------- all fields collator -------- */
  static inline observation_sample_t collate_fn(const std::vector<observation_sample_t>& batch) {
    TORCH_CHECK(!batch.empty(), "[observation_sample_t::collate_fn] empty batch");

    const bool batched_past   = is_batched_past(batch.front());
    const bool batched_future = is_batched_future(batch.front());
    const bool batched_enc    = is_batched_encoding(batch.front());

    TORCH_CHECK(all_same(batched_past,   batch, &is_batched_past),     "[collate_fn] mix in past fields");
    TORCH_CHECK(all_same(batched_future, batch, &is_batched_future),   "[collate_fn] mix in future fields");
    TORCH_CHECK(all_same(batched_enc,    batch, &is_batched_encoding), "[collate_fn] mix in encoding fields");

    const auto fsz  = batch.front().features.sizes();
    const auto msz  = batch.front().mask.sizes();
    const auto ffsz = batch.front().future_features.sizes();
    const auto fmsz = batch.front().future_mask.sizes();

    const bool have_enc = batch.front().encoding.defined();
    const auto esz = have_enc ? batch.front().encoding.sizes() : torch::IntArrayRef{};
    const bool have_past_keys   = batch.front().past_keys.defined();
    const bool have_future_keys = batch.front().future_keys.defined();
    const bool past_keys_batched = have_past_keys ? batched_past : false;
    const bool future_keys_batched = have_future_keys ? batched_future : false;
    const int64_t past_key_dim = have_past_keys ? expected_key_dim_from_past(batch.front()) : -1;
    const int64_t future_key_dim = have_future_keys ? expected_key_dim_from_future(batch.front()) : -1;
    const auto pksz = have_past_keys ? batch.front().past_keys.sizes() : torch::IntArrayRef{};
    const auto fksz = have_future_keys ? batch.front().future_keys.sizes() : torch::IntArrayRef{};

    const bool have_mean = batch.front().feature_mean.defined();
    const bool have_std  = batch.front().feature_std.defined();
    const bool mean_batched = have_mean ? (batch.front().feature_mean.dim() >= 2) : false;
    const bool std_batched  = have_std  ? (batch.front().feature_std.dim()  >= 2) : false;
    const auto mean_sz = have_mean ? batch.front().feature_mean.sizes() : torch::IntArrayRef{};
    const auto std_sz  = have_std  ? batch.front().feature_std.sizes()  : torch::IntArrayRef{};
    const bool normalized0 = batch.front().normalized;
    bool normalized_consistent = true;

    std::vector<torch::Tensor> feats, masks, fut_feats, fut_masks, encs;
    std::vector<torch::Tensor> keys_past, keys_future, means, stds;
    feats.reserve(batch.size()); masks.reserve(batch.size());
    fut_feats.reserve(batch.size()); fut_masks.reserve(batch.size());
    if (have_enc) encs.reserve(batch.size());
    if (have_past_keys) keys_past.reserve(batch.size());
    if (have_future_keys) keys_future.reserve(batch.size());
    if (have_mean) means.reserve(batch.size());
    if (have_std) stds.reserve(batch.size());

    for (const auto& s : batch) {
      TORCH_CHECK(s.features.sizes()        == fsz,  "[collate_fn] features mismatch");
      TORCH_CHECK(s.mask.sizes()            == msz,  "[collate_fn] mask mismatch");
      TORCH_CHECK(s.future_features.sizes() == ffsz, "[collate_fn] future_features mismatch");
      TORCH_CHECK(s.future_mask.sizes()     == fmsz, "[collate_fn] future_mask mismatch");
      normalized_consistent = normalized_consistent && (s.normalized == normalized0);
      feats.emplace_back(s.features);
      masks.emplace_back(s.mask);
      fut_feats.emplace_back(s.future_features);
      fut_masks.emplace_back(s.future_mask);
      if (have_enc) {
        TORCH_CHECK(s.encoding.defined(), "[collate_fn] encoding undefined while first was defined");
        TORCH_CHECK(s.encoding.sizes() == esz, "[collate_fn] encoding mismatch");
        encs.emplace_back(s.encoding);
      }

      TORCH_CHECK(s.past_keys.defined() == have_past_keys, "[collate_fn] past_keys defined mismatch");
      TORCH_CHECK(s.future_keys.defined() == have_future_keys, "[collate_fn] future_keys defined mismatch");
      if (have_past_keys) {
        TORCH_CHECK(s.past_keys.dim() == past_key_dim, "[collate_fn] past_keys dim mismatch");
        TORCH_CHECK(s.past_keys.sizes() == pksz, "[collate_fn] past_keys shape mismatch");
        keys_past.emplace_back(s.past_keys);
      }
      if (have_future_keys) {
        TORCH_CHECK(s.future_keys.dim() == future_key_dim, "[collate_fn] future_keys dim mismatch");
        TORCH_CHECK(s.future_keys.sizes() == fksz, "[collate_fn] future_keys shape mismatch");
        keys_future.emplace_back(s.future_keys);
      }

      TORCH_CHECK(s.feature_mean.defined() == have_mean, "[collate_fn] feature_mean defined mismatch");
      TORCH_CHECK(s.feature_std.defined()  == have_std,  "[collate_fn] feature_std defined mismatch");
      if (have_mean) {
        TORCH_CHECK((s.feature_mean.dim() >= 2) == mean_batched, "[collate_fn] feature_mean batched mismatch");
        TORCH_CHECK(s.feature_mean.sizes() == mean_sz, "[collate_fn] feature_mean shape mismatch");
        means.emplace_back(s.feature_mean);
      }
      if (have_std) {
        TORCH_CHECK((s.feature_std.dim() >= 2) == std_batched, "[collate_fn] feature_std batched mismatch");
        TORCH_CHECK(s.feature_std.sizes() == std_sz, "[collate_fn] feature_std shape mismatch");
        stds.emplace_back(s.feature_std);
      }
    }

    observation_sample_t out{
      smart_stack_or_cat(feats,     batched_past),
      smart_stack_or_cat(masks,     batched_past),
      smart_stack_or_cat(fut_feats, batched_future),
      smart_stack_or_cat(fut_masks, batched_future),
      torch::Tensor(),  // encoding set below if present
      /* normalized */ false,
      /* mean/std  */ torch::Tensor(), torch::Tensor(),
      /* keys      */ torch::Tensor(), torch::Tensor()
    };
    if (!encs.empty()) out.encoding = smart_stack_or_cat(encs, batched_enc);
    if (!keys_past.empty()) out.past_keys = smart_stack_or_cat(keys_past, past_keys_batched);
    if (!keys_future.empty()) out.future_keys = smart_stack_or_cat(keys_future, future_keys_batched);
    if (!means.empty()) out.feature_mean = smart_stack_or_cat(means, mean_batched);
    if (!stds.empty()) out.feature_std = smart_stack_or_cat(stds, std_batched);
    out.normalized = normalized_consistent ? normalized0 : false;
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
    auto maybe_split_or_broadcast = [&](const torch::Tensor& t) -> std::vector<torch::Tensor> {
      if (!t.defined()) return {};
      if (t.dim() >= 1 && t.size(0) == B) return torch::unbind(t, /*dim=*/0);
      std::vector<torch::Tensor> out(static_cast<size_t>(B));
      for (int64_t i = 0; i < B; ++i) out[static_cast<size_t>(i)] = t;
      return out;
    };

    auto feats     = maybe_split(batched.features);
    auto masks     = maybe_split(batched.mask);
    auto fut_feats = maybe_split(batched.future_features);
    auto fut_masks = maybe_split(batched.future_mask);
    auto encs      = maybe_split(batched.encoding);
    auto pkeys     = maybe_split_or_broadcast(batched.past_keys);
    auto fkeys     = maybe_split_or_broadcast(batched.future_keys);
    auto means     = maybe_split_or_broadcast(batched.feature_mean);
    auto stds      = maybe_split_or_broadcast(batched.feature_std);

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
        encs.empty()      ? torch::Tensor() : maybe_clone(encs[i]),
        /* normalized */ batched.normalized,
        /* mean/std  */ means.empty() ? torch::Tensor() : maybe_clone(means[i]),
                         stds.empty() ? torch::Tensor() : maybe_clone(stds[i]),
        /* keys      */ pkeys.empty() ? torch::Tensor() : maybe_clone(pkeys[i]),
                         fkeys.empty() ? torch::Tensor() : maybe_clone(fkeys[i])
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
    feature_mean.reset();
    feature_std.reset();
    past_keys.reset();
    future_keys.reset();
    normalized = false;
  }

  void print() const {
    auto show = [&](const char* name, const torch::Tensor& t, const char* color) {
      printf("  %s%-16s%s : ", color, name, ANSI_COLOR_RESET);
      if (!t.defined()) {
        printf("%sundef%s\n", ANSI_COLOR_Red, ANSI_COLOR_RESET);
      } else {
        printf("[");
        for (int64_t i = 0; i < t.dim(); ++i) {
          if (i) printf(",");
          printf("%lld", static_cast<long long>(t.size(i)));
        }
        printf("]\n");
      }
    };

    printf("%s[observation_sample_t]%s\n", ANSI_COLOR_Blue, ANSI_COLOR_RESET);
    show("features",        features,        ANSI_COLOR_Cyan);
    show("mask",            mask,            ANSI_COLOR_Yellow);
    show("future_features", future_features, ANSI_COLOR_Green);
    show("future_mask",     future_mask,     ANSI_COLOR_Magenta);
    show("encoding",        encoding,        ANSI_COLOR_White);
    show("past_keys",       past_keys,       ANSI_COLOR_Cyan);
    show("future_keys",     future_keys,     ANSI_COLOR_Green);
    show("feat_mean",       feature_mean,    ANSI_COLOR_Dim_Gray);
    show("feat_std",        feature_std,     ANSI_COLOR_Dim_Gray);
    printf("  normalized       : %s%s%s\n", normalized ? ANSI_COLOR_Bright_Green : ANSI_COLOR_Red,
           normalized ? "true" : "false", ANSI_COLOR_RESET);
    printf("\n");
  }
};

} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
