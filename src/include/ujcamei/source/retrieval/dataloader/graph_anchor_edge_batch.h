// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/topology/graph/graph.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "ujcamei/source/retrieval/dataloader/edge_sample.h"
#include "ujcamei/source/retrieval/dataloader/source_cursor.h"

namespace cuwacunu::ujcamei::source::retrieval::dataloader {

template <typename KeyT> struct graph_anchor_edge_sample_t {
  cuwacunu::kikijyeba::topology::graph::instrument_edge_id_t edge_id;
  KeyT anchor_key{};
  cuwacunu::ujcamei::source::retrieval::dataloader::edge_sample_t sample{};
};

enum class graph_anchor_order_policy_t {
  first_seen,
  sorted,
};

template <typename KeyT> struct graph_anchor_edge_batch_t {
  torch::Tensor edge_features{};   // [B,L,C,Hx,9]
  torch::Tensor edge_mask{};       // [B,L,C,Hx], bool
  torch::Tensor future_features{}; // [B,L,C,Hf,9], undefined if omitted
  torch::Tensor future_mask{};     // [B,L,C,Hf], bool, undefined if omitted
  torch::Tensor past_keys{};       // [B,L,C,Hx], undefined if omitted
  torch::Tensor future_keys{};     // [B,L,C,Hf], undefined if omitted
  torch::Tensor anchor_keys{};     // [B]
  torch::Tensor edge_present{};    // [B,L], bool
  std::vector<cuwacunu::kikijyeba::topology::graph::instrument_edge_id_t>
      edge_ids{};
  std::string graph_order_fingerprint{};
  graph_anchor_cursor_t<KeyT> cursor{};
};

struct graph_anchor_edge_alignment_diagnostics_t {
  torch::Tensor max_past_lag_by_anchor{};   // [B], float64
  torch::Tensor max_past_lag_by_edge{};     // [B,L], float64
  torch::Tensor max_future_gap_by_anchor{}; // [B], float64, undefined if no
                                            // future keys/masks
};

struct graph_anchor_edge_batch_options_t {
  std::vector<cuwacunu::kikijyeba::topology::graph::instrument_edge_id_t>
      edge_ids{};
  bool require_all_edges{false};
  bool require_normalized{true};
  bool include_future{true};
  bool require_future{true};
  std::optional<int64_t> future_length{std::nullopt};
  bool include_keys{true};
  bool validate_keys_against_anchor{false};
  int64_t feature_width{
      cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth};
  graph_anchor_order_policy_t anchor_order{
      graph_anchor_order_policy_t::first_seen};
};

namespace detail {

template <typename KeyT> [[nodiscard]] c10::ScalarType key_scalar_type() {
  if constexpr (std::is_integral_v<KeyT>) {
    return torch::kInt64;
  } else {
    return torch::kFloat64;
  }
}

template <typename KeyT>
[[nodiscard]] torch::Tensor make_key_tensor(const std::vector<KeyT> &values,
                                            const torch::Device &device) {
  if constexpr (std::is_integral_v<KeyT>) {
    std::vector<int64_t> converted;
    converted.reserve(values.size());
    for (const KeyT value : values) {
      converted.push_back(static_cast<int64_t>(value));
    }
    return torch::tensor(converted, torch::TensorOptions().dtype(torch::kInt64))
        .to(device);
  } else {
    std::vector<double> converted;
    converted.reserve(values.size());
    for (const KeyT value : values) {
      converted.push_back(static_cast<double>(value));
    }
    return torch::tensor(converted,
                         torch::TensorOptions().dtype(torch::kFloat64))
        .to(device);
  }
}

template <typename KeyT>
[[nodiscard]] torch::Tensor key_tensor_like(const torch::Tensor &tensor,
                                            const torch::Device &device) {
  return tensor.to(device, key_scalar_type<KeyT>()).contiguous();
}

[[nodiscard]] inline torch::Tensor bool_mask_like(const torch::Tensor &tensor,
                                                  const torch::Device &device) {
  return tensor.to(device, torch::kBool).contiguous();
}

[[nodiscard]] inline bool has_future_tensors(
    const cuwacunu::ujcamei::source::retrieval::dataloader::edge_sample_t
        &sample) {
  const bool has_features = sample.future_features.defined();
  const bool has_mask = sample.future_mask.defined();
  TORCH_CHECK(has_features == has_mask,
              "[GraphAnchorEdgeBatch] future_features/future_mask defined "
              "mismatch");
  return has_features && has_mask;
}

template <typename KeyT>
[[nodiscard]] double key_value_as_double(const torch::Tensor &keys, int64_t c,
                                         int64_t h) {
  return keys.index({c, h})
      .to(torch::kCPU)
      .to(torch::kFloat64)
      .template item<double>();
}

template <typename KeyT>
void validate_key_alignment(const graph_anchor_edge_sample_t<KeyT> &wrapped,
                            const torch::Tensor &past_mask,
                            const std::optional<torch::Tensor> &future_mask) {
  const double anchor = static_cast<double>(wrapped.anchor_key);
  const auto C = past_mask.size(0);
  const auto Hx = past_mask.size(1);
  const auto past_keys =
      wrapped.sample.past_keys.to(torch::kCPU).to(key_scalar_type<KeyT>());
  const auto past_mask_cpu = past_mask.to(torch::kCPU).to(torch::kBool);

  for (int64_t c = 0; c < C; ++c) {
    for (int64_t h = Hx - 1; h >= 0; --h) {
      if (!past_mask_cpu.index({c, h}).template item<bool>()) {
        continue;
      }
      const double key = key_value_as_double<KeyT>(past_keys, c, h);
      TORCH_CHECK(key <= anchor,
                  "[GraphAnchorEdgeBatch] terminal valid past key exceeds "
                  "anchor_key");
      break;
    }
  }

  if (!future_mask.has_value() || !future_mask->defined()) {
    return;
  }
  const auto Hf = future_mask->size(1);
  const auto future_keys =
      wrapped.sample.future_keys.to(torch::kCPU).to(key_scalar_type<KeyT>());
  const auto future_mask_cpu = future_mask->to(torch::kCPU).to(torch::kBool);
  for (int64_t c = 0; c < C; ++c) {
    for (int64_t h = 0; h < Hf; ++h) {
      if (!future_mask_cpu.index({c, h}).template item<bool>()) {
        continue;
      }
      const double key = key_value_as_double<KeyT>(future_keys, c, h);
      TORCH_CHECK(key > anchor,
                  "[GraphAnchorEdgeBatch] first valid future key is not after "
                  "anchor_key");
      break;
    }
  }
}

} // namespace detail

template <typename KeyT>
[[nodiscard]] graph_anchor_edge_batch_t<KeyT> make_graph_anchor_edge_batch(
    const std::vector<graph_anchor_edge_sample_t<KeyT>> &samples,
    const graph_anchor_edge_batch_options_t &options) {
  static_assert(
      std::is_integral_v<KeyT>,
      "GraphAnchorEdgeBatch v1 requires integral timestamp-like KeyT");

  TORCH_CHECK(!samples.empty(), "[GraphAnchorEdgeBatch] empty sample list");
  TORCH_CHECK(!options.edge_ids.empty(),
              "[GraphAnchorEdgeBatch] options.edge_ids must not be empty");
  TORCH_CHECK(
      options.feature_width ==
          cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth,
      "[GraphAnchorEdgeBatch] v1 requires feature_width=9 for "
      "kline/SRL input");
  TORCH_CHECK(!options.validate_keys_against_anchor || options.include_keys,
              "[GraphAnchorEdgeBatch] validate_keys_against_anchor=true "
              "requires include_keys=true");
  if (options.future_length.has_value()) {
    TORCH_CHECK(*options.future_length > 0,
                "[GraphAnchorEdgeBatch] future_length must be positive");
  }

  std::unordered_map<std::string, int64_t> edge_to_index;
  edge_to_index.reserve(options.edge_ids.size());
  for (int64_t i = 0; i < static_cast<int64_t>(options.edge_ids.size()); ++i) {
    TORCH_CHECK(!options.edge_ids[static_cast<std::size_t>(i)].empty(),
                "[GraphAnchorEdgeBatch] edge_ids must not contain empty ids");
    const auto [_, inserted] =
        edge_to_index.emplace(options.edge_ids[static_cast<std::size_t>(i)], i);
    TORCH_CHECK(inserted,
                "[GraphAnchorEdgeBatch] duplicate edge_id in options: ",
                options.edge_ids[static_cast<std::size_t>(i)]);
  }

  std::vector<KeyT> anchors;
  anchors.reserve(samples.size());
  std::unordered_set<KeyT> seen_anchors;
  for (const auto &wrapped : samples) {
    TORCH_CHECK(
        edge_to_index.find(wrapped.edge_id) != edge_to_index.end(),
        "[GraphAnchorEdgeBatch] unknown sample edge_id: ", wrapped.edge_id);
    if (seen_anchors.insert(wrapped.anchor_key).second) {
      anchors.push_back(wrapped.anchor_key);
    }
  }
  if (options.anchor_order == graph_anchor_order_policy_t::sorted) {
    std::sort(anchors.begin(), anchors.end());
  }

  std::unordered_map<KeyT, int64_t> anchor_to_index;
  anchor_to_index.reserve(anchors.size());
  for (int64_t i = 0; i < static_cast<int64_t>(anchors.size()); ++i) {
    anchor_to_index.emplace(anchors[static_cast<std::size_t>(i)], i);
  }

  const auto &first = samples.front().sample;
  TORCH_CHECK(first.features.defined(),
              "[GraphAnchorEdgeBatch] sample.features must be defined");
  TORCH_CHECK(first.mask.defined(),
              "[GraphAnchorEdgeBatch] sample.mask must be defined");
  TORCH_CHECK(first.features.dim() == 3,
              "[GraphAnchorEdgeBatch] v1 requires features [C,Hx,D]");
  TORCH_CHECK(first.mask.dim() == 2,
              "[GraphAnchorEdgeBatch] v1 requires mask [C,Hx]");
  const int64_t C = first.features.size(0);
  const int64_t Hx = first.features.size(1);
  const int64_t D = first.features.size(2);
  TORCH_CHECK(D == options.feature_width,
              "[GraphAnchorEdgeBatch] feature width mismatch");
  TORCH_CHECK(first.mask.sizes() == torch::IntArrayRef({C, Hx}),
              "[GraphAnchorEdgeBatch] mask shape mismatch");

  const torch::Device device = first.features.device();
  const c10::ScalarType dtype = first.features.scalar_type();
  const bool normalized0 = first.normalized;
  TORCH_CHECK(first.features.is_floating_point(),
              "[GraphAnchorEdgeBatch] features must be floating normalized "
              "tensors");

  int64_t Hf = options.future_length.value_or(0);
  bool saw_future = false;
  for (const auto &wrapped : samples) {
    const auto &s = wrapped.sample;
    TORCH_CHECK(s.features.defined(),
                "[GraphAnchorEdgeBatch] sample.features must be defined");
    TORCH_CHECK(s.mask.defined(),
                "[GraphAnchorEdgeBatch] sample.mask must be defined");
    TORCH_CHECK(s.features.dim() == 3,
                "[GraphAnchorEdgeBatch] v1 accepts only unbatched [C,Hx,D] "
                "features");
    TORCH_CHECK(
        s.mask.dim() == 2,
        "[GraphAnchorEdgeBatch] v1 accepts only unbatched [C,Hx] masks");
    TORCH_CHECK(s.features.sizes() == torch::IntArrayRef({C, Hx, D}),
                "[GraphAnchorEdgeBatch] feature shape mismatch");
    TORCH_CHECK(s.mask.sizes() == torch::IntArrayRef({C, Hx}),
                "[GraphAnchorEdgeBatch] mask shape mismatch");
    TORCH_CHECK(s.features.device() == device,
                "[GraphAnchorEdgeBatch] feature device mismatch");
    TORCH_CHECK(s.mask.device() == device,
                "[GraphAnchorEdgeBatch] mask device mismatch");
    TORCH_CHECK(s.features.scalar_type() == dtype,
                "[GraphAnchorEdgeBatch] feature dtype mismatch");
    TORCH_CHECK(s.features.is_floating_point(),
                "[GraphAnchorEdgeBatch] features must be floating normalized "
                "tensors");
    TORCH_CHECK(s.normalized == normalized0,
                "[GraphAnchorEdgeBatch] normalized flag mismatch");
    if (options.require_normalized) {
      TORCH_CHECK(s.normalized,
                  "[GraphAnchorEdgeBatch] normalized sample required");
    }

    if (options.include_keys) {
      TORCH_CHECK(s.past_keys.defined(),
                  "[GraphAnchorEdgeBatch] past_keys required");
      TORCH_CHECK(s.past_keys.dim() == 2,
                  "[GraphAnchorEdgeBatch] past_keys must be [C,Hx]");
      TORCH_CHECK(s.past_keys.sizes() == torch::IntArrayRef({C, Hx}),
                  "[GraphAnchorEdgeBatch] past_keys shape mismatch");
    }

    if (!options.include_future) {
      continue;
    }
    const bool has_future = detail::has_future_tensors(s);
    if (!has_future) {
      TORCH_CHECK(!options.require_future,
                  "[GraphAnchorEdgeBatch] future tensors required");
      continue;
    }
    TORCH_CHECK(s.future_features.dim() == 3,
                "[GraphAnchorEdgeBatch] future_features must be [C,Hf,D]");
    TORCH_CHECK(s.future_mask.dim() == 2,
                "[GraphAnchorEdgeBatch] future_mask must be [C,Hf]");
    TORCH_CHECK(s.future_features.size(0) == C,
                "[GraphAnchorEdgeBatch] future C mismatch");
    TORCH_CHECK(s.future_features.size(2) == D,
                "[GraphAnchorEdgeBatch] future feature width mismatch");
    TORCH_CHECK(s.future_mask.sizes() ==
                    torch::IntArrayRef({C, s.future_features.size(1)}),
                "[GraphAnchorEdgeBatch] future_mask shape mismatch");
    TORCH_CHECK(s.future_features.device() == device,
                "[GraphAnchorEdgeBatch] future feature device mismatch");
    TORCH_CHECK(s.future_mask.device() == device,
                "[GraphAnchorEdgeBatch] future mask device mismatch");
    TORCH_CHECK(s.future_features.scalar_type() == dtype,
                "[GraphAnchorEdgeBatch] future feature dtype mismatch");
    if (!saw_future) {
      Hf = s.future_features.size(1);
      saw_future = true;
      if (options.future_length.has_value()) {
        TORCH_CHECK(*options.future_length == Hf,
                    "[GraphAnchorEdgeBatch] future_length does not match "
                    "present future tensors");
      }
    } else {
      TORCH_CHECK(s.future_features.size(1) == Hf,
                  "[GraphAnchorEdgeBatch] future length mismatch");
    }
    if (options.include_keys) {
      TORCH_CHECK(s.future_keys.defined(),
                  "[GraphAnchorEdgeBatch] future_keys required");
      TORCH_CHECK(s.future_keys.dim() == 2,
                  "[GraphAnchorEdgeBatch] future_keys must be [C,Hf]");
      TORCH_CHECK(s.future_keys.sizes() == torch::IntArrayRef({C, Hf}),
                  "[GraphAnchorEdgeBatch] future_keys shape mismatch");
    }
  }

  if (options.include_future && !saw_future) {
    TORCH_CHECK(!options.require_future,
                "[GraphAnchorEdgeBatch] future tensors required");
    TORCH_CHECK(Hf > 0,
                "[GraphAnchorEdgeBatch] future_length required when all "
                "futures are missing");
  }

  const int64_t Bg = static_cast<int64_t>(anchors.size());
  const int64_t L = static_cast<int64_t>(options.edge_ids.size());

  auto feature_opts = torch::TensorOptions().dtype(dtype).device(device);
  auto bool_opts = torch::TensorOptions().dtype(torch::kBool).device(device);
  auto key_opts = torch::TensorOptions()
                      .dtype(detail::key_scalar_type<KeyT>())
                      .device(device);

  graph_anchor_edge_batch_t<KeyT> out{};
  out.edge_ids = options.edge_ids;
  out.edge_features = torch::zeros({Bg, L, C, Hx, D}, feature_opts);
  out.edge_mask = torch::zeros({Bg, L, C, Hx}, bool_opts);
  out.edge_present = torch::zeros({Bg, L}, bool_opts);
  out.anchor_keys = detail::make_key_tensor<KeyT>(anchors, device);
  if (options.include_keys) {
    out.past_keys = torch::zeros({Bg, L, C, Hx}, key_opts);
  }
  if (options.include_future) {
    out.future_features = torch::zeros({Bg, L, C, Hf, D}, feature_opts);
    out.future_mask = torch::zeros({Bg, L, C, Hf}, bool_opts);
    if (options.include_keys) {
      out.future_keys = torch::zeros({Bg, L, C, Hf}, key_opts);
    }
  }

  auto seen_pairs = torch::zeros({Bg, L}, bool_opts);
  for (const auto &wrapped : samples) {
    const int64_t b = anchor_to_index.at(wrapped.anchor_key);
    const int64_t e = edge_to_index.at(wrapped.edge_id);
    TORCH_CHECK(!seen_pairs.index({b, e}).template item<bool>(),
                "[GraphAnchorEdgeBatch] duplicate (anchor_key, edge_id)");
    seen_pairs.index_put_({b, e}, true);
    out.edge_present.index_put_({b, e}, true);

    const auto &s = wrapped.sample;
    out.edge_features.index_put_({b, e, torch::indexing::Slice(),
                                  torch::indexing::Slice(),
                                  torch::indexing::Slice()},
                                 s.features.contiguous());
    const auto mask = detail::bool_mask_like(s.mask, device);
    out.edge_mask.index_put_(
        {b, e, torch::indexing::Slice(), torch::indexing::Slice()}, mask);
    if (options.include_keys) {
      out.past_keys.index_put_(
          {b, e, torch::indexing::Slice(), torch::indexing::Slice()},
          detail::key_tensor_like<KeyT>(s.past_keys, device));
    }

    std::optional<torch::Tensor> future_mask_for_validation{};
    if (options.include_future && detail::has_future_tensors(s)) {
      out.future_features.index_put_({b, e, torch::indexing::Slice(),
                                      torch::indexing::Slice(),
                                      torch::indexing::Slice()},
                                     s.future_features.contiguous());
      const auto future_mask = detail::bool_mask_like(s.future_mask, device);
      out.future_mask.index_put_(
          {b, e, torch::indexing::Slice(), torch::indexing::Slice()},
          future_mask);
      future_mask_for_validation = future_mask;
      if (options.include_keys) {
        out.future_keys.index_put_(
            {b, e, torch::indexing::Slice(), torch::indexing::Slice()},
            detail::key_tensor_like<KeyT>(s.future_keys, device));
      }
    }

    if (options.include_keys && options.validate_keys_against_anchor) {
      detail::validate_key_alignment<KeyT>(wrapped, mask,
                                           future_mask_for_validation);
    }
  }

  if (options.require_all_edges) {
    TORCH_CHECK(
        out.edge_present.all().template item<bool>(),
        "[GraphAnchorEdgeBatch] require_all_edges=true but at least one "
        "anchor is missing an edge");
  }

  return out;
}

template <typename KeyT>
[[nodiscard]] graph_anchor_edge_alignment_diagnostics_t
compute_graph_anchor_edge_alignment_diagnostics(
    const graph_anchor_edge_batch_t<KeyT> &batch) {
  static_assert(std::is_integral_v<KeyT>,
                "GraphAnchorEdgeBatch diagnostics v1 requires integral KeyT");

  graph_anchor_edge_alignment_diagnostics_t out{};
  TORCH_CHECK(batch.anchor_keys.defined(),
              "[GraphAnchorEdgeBatch diagnostics] anchor_keys must be defined");
  TORCH_CHECK(batch.edge_mask.defined(),
              "[GraphAnchorEdgeBatch diagnostics] edge_mask must be defined");
  TORCH_CHECK(batch.past_keys.defined(),
              "[GraphAnchorEdgeBatch diagnostics] past_keys must be defined");

  const auto anchors = batch.anchor_keys.to(torch::kCPU).to(torch::kFloat64);
  const auto past_keys = batch.past_keys.to(torch::kCPU).to(torch::kFloat64);
  const auto edge_mask = batch.edge_mask.to(torch::kCPU).to(torch::kBool);
  const int64_t Bg = batch.edge_mask.size(0);
  const int64_t L = batch.edge_mask.size(1);
  const int64_t C = batch.edge_mask.size(2);
  const int64_t Hx = batch.edge_mask.size(3);

  out.max_past_lag_by_anchor =
      torch::zeros({Bg}, torch::TensorOptions().dtype(torch::kFloat64));
  out.max_past_lag_by_edge =
      torch::zeros({Bg, L}, torch::TensorOptions().dtype(torch::kFloat64));

  for (int64_t b = 0; b < Bg; ++b) {
    const double anchor = anchors.index({b}).template item<double>();
    double max_anchor_lag = 0.0;
    for (int64_t e = 0; e < L; ++e) {
      double max_edge_lag = 0.0;
      for (int64_t c = 0; c < C; ++c) {
        for (int64_t h = Hx - 1; h >= 0; --h) {
          if (!edge_mask.index({b, e, c, h}).template item<bool>()) {
            continue;
          }
          const double key =
              past_keys.index({b, e, c, h}).template item<double>();
          max_edge_lag = std::max(max_edge_lag, anchor - key);
          break;
        }
      }
      out.max_past_lag_by_edge.index_put_({b, e}, max_edge_lag);
      max_anchor_lag = std::max(max_anchor_lag, max_edge_lag);
    }
    out.max_past_lag_by_anchor.index_put_({b}, max_anchor_lag);
  }

  if (batch.future_keys.defined() && batch.future_mask.defined()) {
    const auto future_keys =
        batch.future_keys.to(torch::kCPU).to(torch::kFloat64);
    const auto future_mask = batch.future_mask.to(torch::kCPU).to(torch::kBool);
    const int64_t Hf = batch.future_mask.size(3);
    out.max_future_gap_by_anchor =
        torch::zeros({Bg}, torch::TensorOptions().dtype(torch::kFloat64));
    for (int64_t b = 0; b < Bg; ++b) {
      const double anchor = anchors.index({b}).template item<double>();
      double max_gap = 0.0;
      for (int64_t e = 0; e < L; ++e) {
        for (int64_t c = 0; c < C; ++c) {
          for (int64_t h = 0; h < Hf; ++h) {
            if (!future_mask.index({b, e, c, h}).template item<bool>()) {
              continue;
            }
            const double key =
                future_keys.index({b, e, c, h}).template item<double>();
            max_gap = std::max(max_gap, key - anchor);
            break;
          }
        }
      }
      out.max_future_gap_by_anchor.index_put_({b}, max_gap);
    }
  }

  return out;
}

} // namespace cuwacunu::ujcamei::source::retrieval::dataloader
