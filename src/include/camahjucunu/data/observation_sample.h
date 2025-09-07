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
  torch::Tensor features;        // [B,C,T,D] or [C,T,D] if unbatched
  torch::Tensor mask;            // [B,C,T]   or [C,T]
  // future (starts at t+1) — same channeling as past, different time length Tf
  torch::Tensor future_features; // [B,C,Tf,D] or [C,Tf,D] if unbatched
  torch::Tensor future_mask;     // [B,C,Tf]   or [C,Tf]
  // optional encoder output
  torch::Tensor encoding;        // [B,De] or [B,T',De] (or undefined)

private:
  /* ---- helpers ----------------------------------------------------------- */

  static inline bool is_batched_past(const observation_sample_t& s) {
    if (s.features.defined()) return s.features.dim() >= 4; // [B,C,T,D]
    if (s.mask.defined())     return s.mask.dim()     >= 3; // [B,C,T]
    return false;
  }

  // Future is considered "batched" iff past is batched AND future has a leading B
  // that matches the past's B. Otherwise (including unbatched past), treat future
  // as UNBATCHED ([C,Tf,D]/[C,Tf]).
  static inline bool is_batched_future(const observation_sample_t& s) {
    const bool past_batched = is_batched_past(s);
    if (!past_batched) {
      return false; // unbatched futures are [C,Tf,D]/[C,Tf]
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

public:
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
      torch::Tensor(), torch::Tensor(), torch::Tensor()
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
      torch::Tensor()
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

    // Strong invariants to avoid silent B*C collapses and enforce channel-parity.
    if (!batched_past) {
      // Unbatched past => unbatched futures must be [C,Tf,D]/[C,Tf] (NO leading B).
      if (batch.front().future_features.defined()) {
        TORCH_CHECK(batch.front().future_features.dim() == 3,
                    "[collate_fn] Unbatched past but future_features dim!=3; expected [C,Tf,D], got ",
                    batch.front().future_features.sizes());
      }
      if (batch.front().future_mask.defined()) {
        TORCH_CHECK(batch.front().future_mask.dim() == 2,
            "[collate_fn] Unbatched past but future_mask dim!=2; expected [C,Tf], got ",
            batch.front().future_mask.sizes());
      }
    } else {
      // Batched past with B => batched futures (if batched) must start with B.
      int64_t B = 0, C = 0;
      if (batch.front().features.defined() && batch.front().features.dim() >= 4) {
        B = batch.front().features.size(0);
        C = batch.front().features.size(1);
      } else if (batch.front().mask.defined() && batch.front().mask.dim() >= 3) {
        B = batch.front().mask.size(0);
        C = batch.front().mask.size(1);
      }
      if (batch.front().future_features.defined()) {
        if (batched_future) {
          TORCH_CHECK(batch.front().future_features.dim() >= 4 &&
                      batch.front().future_features.size(0) == B &&
                      batch.front().future_features.size(1) == C,
                      "[collate_fn] Batched future_features must be [B,C,Tf,D]; got ",
                      batch.front().future_features.sizes(), " while B=", B, " C=", C);
        } else {
          TORCH_CHECK(batch.front().future_features.dim() == 3 &&
                      batch.front().future_features.size(0) == C,
                      "[collate_fn] Unbatched future_features with batched past must be [C,Tf,D]; got ",
                      batch.front().future_features.sizes(), " with C=", C);
        }
      }
      if (batch.front().future_mask.defined()) {
        if (batched_future) {
          TORCH_CHECK(batch.front().future_mask.dim() >= 3 &&
                      batch.front().future_mask.size(0) == B &&
                      batch.front().future_mask.size(1) == C,
                      "[collate_fn] Batched future_mask must be [B,C,Tf]; got ",
                      batch.front().future_mask.sizes(), " while B=", B, " C=", C);
        } else {
          TORCH_CHECK(batch.front().future_mask.dim() == 2 &&
                      batch.front().future_mask.size(0) == C,
                      "[collate_fn] Unbatched future_mask with batched past must be [C,Tf]; got ",
                      batch.front().future_mask.sizes(), " with C=", C);
        }
      }
    }

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
        TORCH_CHECK(s.encoding.defined(), "[collate_fn] encoding undefined while first was defined");
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
    printf("\n");
  }
};

} /* namespace data */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
