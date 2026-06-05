// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/environment/replay/world.h"
#include "kikijyeba/topology/graph/graph.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "ujcamei/source/retrieval/dataloader/graph_anchor_edge_batch.h"

namespace cuwacunu::kikijyeba::environment::replay {

namespace replay_source_detail {

namespace topology_graph = cuwacunu::kikijyeba::topology::graph;
using topology_graph::edge_index_t;
using topology_graph::market_graph_t;
namespace dataloader = cuwacunu::ujcamei::source::retrieval::dataloader;
namespace kline = cuwacunu::ujcamei::source::registry::types;

template <typename T> [[nodiscard]] std::int64_t to_i64_checked(T value) {
  static_assert(std::is_integral_v<T>,
                "replay source keys and indices must be integral in V1");
  if constexpr (std::is_signed_v<T>) {
    if (value < 0) {
      throw std::runtime_error("[replay_source] negative integral value");
    }
  }
  using limit_t = std::numeric_limits<std::int64_t>;
  using unsigned_limit_t = std::make_unsigned_t<std::int64_t>;
  if constexpr (std::is_unsigned_v<T>) {
    if (value > static_cast<unsigned_limit_t>(limit_t::max())) {
      throw std::runtime_error(
          "[replay_source] integral value exceeds int64 range");
    }
  } else {
    if (value > limit_t::max()) {
      throw std::runtime_error(
          "[replay_source] integral value exceeds int64 range");
    }
  }
  return static_cast<std::int64_t>(value);
}

template <typename KeyT> [[nodiscard]] std::string key_to_string(KeyT key) {
  static_assert(std::is_integral_v<KeyT>,
                "replay source V1 requires integral timestamp-like keys");
  std::ostringstream out;
  out << key;
  return out.str();
}

template <typename KeyT>
[[nodiscard]] std::vector<std::string>
stringify_anchor_keys(const std::vector<KeyT> &keys) {
  std::vector<std::string> out;
  out.reserve(keys.size());
  for (const auto key : keys) {
    out.push_back(key_to_string(key));
  }
  return out;
}

template <typename KeyT>
[[nodiscard]] std::pair<std::int64_t, std::int64_t> accepted_anchor_interval(
    const dataloader::graph_anchor_cursor_t<KeyT> &cursor) {
  if (cursor.empty()) {
    throw std::runtime_error("[replay_source] graph anchor cursor is empty");
  }
  for (std::size_t i = 1; i < cursor.anchor_keys.size(); ++i) {
    if (cursor.anchor_keys[i] <= cursor.anchor_keys[i - 1]) {
      throw std::runtime_error(
          "[replay_source] V1 requires strictly increasing accepted anchor "
          "keys");
    }
  }
  if (!cursor.anchor_indices.empty()) {
    if (cursor.anchor_indices.size() != cursor.anchor_keys.size()) {
      throw std::runtime_error(
          "[replay_source] cursor anchor_indices/anchor_keys size mismatch");
    }
    const auto begin = cursor.anchor_indices.front();
    for (std::size_t i = 0; i < cursor.anchor_indices.size(); ++i) {
      if (cursor.anchor_indices[i] != begin + i) {
        throw std::runtime_error(
            "[replay_source] V1 requires contiguous accepted anchor indices");
      }
    }
    return {to_i64_checked(begin),
            to_i64_checked(begin + cursor.anchor_indices.size())};
  }
  if (cursor.end_anchor_index <= cursor.begin_anchor_index) {
    throw std::runtime_error(
        "[replay_source] cursor begin/end anchor indices are invalid");
  }
  const auto count = cursor.end_anchor_index - cursor.begin_anchor_index;
  if (count != cursor.anchor_keys.size()) {
    throw std::runtime_error(
        "[replay_source] V1 requires accepted cursor range to match key count");
  }
  return {to_i64_checked(cursor.begin_anchor_index),
          to_i64_checked(cursor.end_anchor_index)};
}

[[nodiscard]] inline std::int64_t close_log_return_coord() {
  const auto *descriptor =
      kline::find_kline_feature_descriptor(kline::kline_feature_e::close);
  if (descriptor == nullptr) {
    throw std::runtime_error(
        "[replay_source] kline close feature descriptor is missing");
  }
  if (descriptor->normalization_rule !=
      kline::kline_normalization_rule_e::log_return_to_previous_close) {
    throw std::runtime_error(
        "[replay_source] close feature is not declared as log return");
  }
  return descriptor->coord;
}

struct direct_base_edge_projection_t {
  edge_index_t graph_edge_index{-1};
  std::string edge_id{};
  std::string risky_node_id{};
  std::string reference_node_id{};
  double sign{0.0};
};

[[nodiscard]] inline direct_base_edge_projection_t
resolve_direct_base_edge(const market_graph_t &graph,
                         const std::string &risky_node_id,
                         const std::string &reference_node_id) {
  graph.validate();
  std::optional<direct_base_edge_projection_t> forward;
  std::optional<direct_base_edge_projection_t> reverse;
  for (edge_index_t e = 0; e < graph.num_edges(); ++e) {
    const auto edge = graph.directed_edge(e);
    if (edge.base_node_id == risky_node_id &&
        edge.quote_node_id == reference_node_id) {
      forward = direct_base_edge_projection_t{
          .graph_edge_index = e,
          .edge_id = edge.edge_id,
          .risky_node_id = risky_node_id,
          .reference_node_id = reference_node_id,
          .sign = 1.0,
      };
    }
    if (edge.base_node_id == reference_node_id &&
        edge.quote_node_id == risky_node_id) {
      reverse = direct_base_edge_projection_t{
          .graph_edge_index = e,
          .edge_id = edge.edge_id,
          .risky_node_id = risky_node_id,
          .reference_node_id = reference_node_id,
          .sign = -1.0,
      };
    }
  }
  if (forward.has_value()) {
    return *forward;
  }
  if (reverse.has_value()) {
    return *reverse;
  }
  throw std::runtime_error("[replay_source] missing direct edge for risky "
                           "asset/reference pair: " +
                           risky_node_id + "/" + reference_node_id);
}

[[nodiscard]] inline std::unordered_map<std::string, std::int64_t>
make_batch_edge_index(const std::vector<std::string> &edge_ids) {
  std::unordered_map<std::string, std::int64_t> out;
  out.reserve(edge_ids.size());
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(edge_ids.size());
       ++i) {
    const auto &edge_id = edge_ids[static_cast<std::size_t>(i)];
    if (edge_id.empty()) {
      throw std::runtime_error(
          "[replay_source] batch edge_ids must not contain empty ids");
    }
    const auto [_, inserted] = out.emplace(edge_id, i);
    if (!inserted) {
      throw std::runtime_error("[replay_source] duplicate batch edge_id: " +
                               edge_id);
    }
  }
  return out;
}

[[nodiscard]] inline std::int64_t require_batch_edge_index(
    const std::unordered_map<std::string, std::int64_t> &batch_edge_index,
    const std::string &edge_id) {
  const auto found = batch_edge_index.find(edge_id);
  if (found == batch_edge_index.end()) {
    throw std::runtime_error(
        "[replay_source] direct projection edge is absent from edge batch: " +
        edge_id);
  }
  return found->second;
}

inline void require_v1_unified_base_policy(const belief::BasePolicy &policy) {
  if (policy.accounting_numeraire_id != policy.reserve_asset_id ||
      policy.accounting_numeraire_id != policy.settlement_asset_id ||
      policy.accounting_numeraire_id != policy.projection_reference_node_id) {
    throw std::runtime_error(
        "[replay_source] V1 requires accounting, settlement, reserve, and "
        "projection reference nodes to be the same graph node");
  }
}

template <typename KeyT>
[[nodiscard]] KeyT tensor_key_at(const torch::Tensor &tensor,
                                 std::initializer_list<std::int64_t> index,
                                 const char *name) {
  static_assert(std::is_integral_v<KeyT>,
                "replay source V1 requires integral timestamp-like keys");
  TORCH_CHECK(tensor.defined(), "[replay_source] missing ", name);
  TORCH_CHECK(tensor.scalar_type() == torch::kInt64, "[replay_source] ", name,
              " must be int64");
  auto current = tensor.to(torch::kCPU).to(torch::kInt64);
  std::vector<torch::indexing::TensorIndex> indices;
  indices.reserve(index.size());
  for (const auto i : index) {
    indices.emplace_back(i);
  }
  const auto value = current.index(indices).template item<std::int64_t>();
  return static_cast<KeyT>(value);
}

template <typename KeyT>
[[nodiscard]] KeyT
anchor_key_at(const dataloader::graph_anchor_edge_batch_t<KeyT> &batch,
              std::int64_t b) {
  if (!batch.cursor.anchor_keys.empty()) {
    return batch.cursor.anchor_keys.at(static_cast<std::size_t>(b));
  }
  return tensor_key_at<KeyT>(batch.anchor_keys, {b}, "anchor_keys");
}

template <typename KeyT>
inline void validate_edge_batch_anchor_keys_match_spec(
    const dataloader::graph_anchor_edge_batch_t<KeyT> &batch,
    const episode_spec_t &spec, std::int64_t B) {
  static_assert(std::is_integral_v<KeyT>,
                "replay source V1 requires integral timestamp-like keys");
  TORCH_CHECK(batch.anchor_keys.defined(),
              "[replay_source] anchor_keys are required");
  TORCH_CHECK(batch.anchor_keys.dim() == 1 && batch.anchor_keys.size(0) == B,
              "[replay_source] anchor_keys must be [B]");
  for (std::int64_t b = 0; b < B; ++b) {
    const auto key = tensor_key_at<KeyT>(batch.anchor_keys, {b}, "anchor_keys");
    const auto expected =
        spec.accepted_range.anchor_keys.at(static_cast<std::size_t>(b));
    if (key_to_string(key) != expected) {
      throw std::runtime_error(
          "[replay_source] edge batch anchor_keys tensor does not match "
          "accepted cursor keys");
    }
  }
}

template <typename KeyT>
inline void validate_edge_batch_cursor_matches_spec(
    const dataloader::graph_anchor_edge_batch_t<KeyT> &batch,
    const episode_spec_t &spec) {
  static_assert(std::is_integral_v<KeyT>,
                "replay source V1 requires integral timestamp-like keys");
  if (batch.cursor.empty()) {
    throw std::runtime_error("[replay_source] edge batch cursor is required");
  }
  const auto batch_cursor_token = batch.cursor.cursor_token();
  if (batch_cursor_token != spec.accepted_range.cursor.batch_cursor_token) {
    throw std::runtime_error(
        "[replay_source] edge batch cursor token does not match accepted "
        "cursor identity");
  }
}

template <typename KeyT>
[[nodiscard]] std::vector<KeyT> shared_realization_keys_for_direct_edges(
    const dataloader::graph_anchor_edge_batch_t<KeyT> &batch,
    const std::vector<std::int64_t> &direct_batch_edges,
    std::int64_t future_horizon_index,
    std::int64_t realization_channel_index = -1) {
  static_assert(std::is_integral_v<KeyT>,
                "replay source V1 requires integral timestamp-like keys");
  TORCH_CHECK(batch.future_keys.defined(),
              "[replay_source] future_keys are required");
  TORCH_CHECK(batch.future_keys.dim() == 4,
              "[replay_source] future_keys must be [B,L,C,Hf]");
  TORCH_CHECK(batch.future_keys.scalar_type() == torch::kInt64,
              "[replay_source] future_keys must be int64");
  TORCH_CHECK(batch.future_mask.defined(),
              "[replay_source] future_mask is required");
  TORCH_CHECK(batch.future_mask.dim() == 4,
              "[replay_source] future_mask must be [B,L,C,Hf]");
  TORCH_CHECK(batch.future_mask.scalar_type() == torch::kBool,
              "[replay_source] future_mask must be bool");
  TORCH_CHECK(batch.future_keys.sizes() == batch.future_mask.sizes(),
              "[replay_source] future_keys/future_mask shape mismatch");
  const auto B = batch.future_mask.size(0);
  const auto L = batch.future_mask.size(1);
  const auto C = batch.future_mask.size(2);
  const auto Hf = batch.future_mask.size(3);
  TORCH_CHECK(batch.edge_present.defined(),
              "[replay_source] edge_present is required");
  TORCH_CHECK(batch.edge_present.dim() == 2 &&
                  batch.edge_present.size(0) == B &&
                  batch.edge_present.size(1) == L,
              "[replay_source] edge_present must be [B,L]");
  TORCH_CHECK(batch.edge_present.scalar_type() == torch::kBool,
              "[replay_source] edge_present must be bool");
  TORCH_CHECK(future_horizon_index >= 0 && future_horizon_index < Hf,
              "[replay_source] future_horizon_index out of range");
  TORCH_CHECK(realization_channel_index < C,
              "[replay_source] realization_channel_index out of range");

  const auto future_mask =
      batch.future_mask.to(torch::kCPU).to(torch::kBool).contiguous();
  const auto edge_present =
      batch.edge_present.to(torch::kCPU).to(torch::kBool).contiguous();
  const std::int64_t channel_begin =
      realization_channel_index == -2
          ? C - 1
          : (realization_channel_index >= 0 ? realization_channel_index : 0);
  const std::int64_t channel_end =
      realization_channel_index == -2
          ? C
          : (realization_channel_index >= 0 ? realization_channel_index + 1
                                            : C);
  std::vector<KeyT> out;
  out.reserve(static_cast<std::size_t>(B));
  for (std::int64_t b = 0; b < B; ++b) {
    const KeyT anchor_key = anchor_key_at<KeyT>(batch, b);
    std::optional<KeyT> realization_key{};
    for (const auto edge_index : direct_batch_edges) {
      TORCH_CHECK(edge_index >= 0 && edge_index < L,
                  "[replay_source] direct projection edge index out of range");
      if (!edge_present.index({b, edge_index}).template item<bool>()) {
        throw std::runtime_error(
            "[replay_source] direct projection edge is marked absent");
      }
      for (std::int64_t c = channel_begin; c < channel_end; ++c) {
        if (!future_mask.index({b, edge_index, c, future_horizon_index})
                 .template item<bool>()) {
          continue;
        }
        const KeyT key = tensor_key_at<KeyT>(
            batch.future_keys, {b, edge_index, c, future_horizon_index},
            "future_keys");
        if (key <= anchor_key) {
          throw std::runtime_error(
              "[replay_source] future realization key is not after anchor");
        }
        if (realization_key.has_value() && key != *realization_key) {
          throw std::runtime_error(
              "[replay_source] V1 requires one shared realization key per "
              "replay frame");
        }
        realization_key = key;
      }
    }
    if (!realization_key.has_value()) {
      throw std::runtime_error(
          "[replay_source] no realization key for replay frame");
    }
    out.push_back(*realization_key);
  }
  return out;
}

} // namespace replay_source_detail

struct replay_frame_build_options_t {
  std::int64_t future_horizon_index{0};
  // -1 preserves all-channel legacy behavior. -2 selects the last active
  // channel, which is the current runtime replay default for ordered
  // multi-timeframe Ujcamei channels. Nonnegative values select an explicit
  // channel.
  std::int64_t realization_channel_index{-1};
  bool require_v1_unified_base_policy{true};
  bool require_graph_fingerprint_match{true};
};

inline void validate_replay_frame_build_options(
    const replay_frame_build_options_t &options) {
  if (options.future_horizon_index < 0) {
    throw std::runtime_error(
        "[replay_source] future_horizon_index must be nonnegative");
  }
  if (options.realization_channel_index < -2) {
    throw std::runtime_error(
        "[replay_source] realization_channel_index must be -2, -1, or "
        "nonnegative");
  }
}

struct direct_edge_projection_plan_t {
  std::string truth_id{kDirectEdgeRealizedReturnTruthV1};
  std::vector<std::string> risky_node_ids{};
  std::string reference_node_id{};
  std::vector<std::string> edge_ids{};
  torch::Tensor signs{}; // [A], +1 for asset/reference, -1 for reference/asset
};

struct replay_episode_bundle_t {
  episode_spec_t spec{};
  std::vector<replay_frame_t> frames{};
  replay_frame_build_options_t frame_options{};
  replay_world_options_t world_options{};
  direct_edge_projection_plan_t realized_return_projection{};

  [[nodiscard]] std::size_t frame_count() const { return frames.size(); }
};

[[nodiscard]] inline direct_edge_projection_plan_t
make_direct_edge_projection_plan(
    const replay_source_detail::market_graph_t &graph,
    const episode_spec_t &spec, replay_frame_build_options_t options = {}) {
  validate_episode_spec(spec);
  validate_replay_frame_build_options(options);
  if (options.require_v1_unified_base_policy) {
    replay_source_detail::require_v1_unified_base_policy(spec.base_policy);
  }
  if (options.require_graph_fingerprint_match &&
      graph.computed_graph_order_fingerprint() !=
          spec.graph_order_fingerprint) {
    throw std::runtime_error(
        "[replay_source] EpisodeSpec graph fingerprint does not match graph");
  }

  direct_edge_projection_plan_t out{};
  out.risky_node_ids = spec.risky_node_ids;
  out.reference_node_id = spec.base_policy.projection_reference_node_id;
  out.edge_ids.reserve(spec.risky_node_ids.size());
  std::vector<double> signs;
  signs.reserve(spec.risky_node_ids.size());
  for (const auto &risky_node_id : spec.risky_node_ids) {
    const auto projection = replay_source_detail::resolve_direct_base_edge(
        graph, risky_node_id, spec.base_policy.projection_reference_node_id);
    out.edge_ids.push_back(projection.edge_id);
    signs.push_back(projection.sign);
  }
  out.signs =
      torch::tensor(signs, torch::TensorOptions().dtype(torch::kFloat64));
  return out;
}

[[nodiscard]] inline realized_return_projection_spec_t
realized_return_projection_spec_from_plan(
    const direct_edge_projection_plan_t &plan) {
  realized_return_projection_spec_t out{};
  out.truth_id = plan.truth_id;
  out.reference_node_id = plan.reference_node_id;
  out.edge_ids = plan.edge_ids;
  out.signs = plan.signs.defined() ? plan.signs.clone() : torch::Tensor{};
  return out;
}

inline void
validate_replay_episode_bundle(const replay_episode_bundle_t &bundle) {
  validate_episode_spec(bundle.spec);
  validate_replay_frame_build_options(bundle.frame_options);
  validate_replay_world_options(bundle.world_options);
  if (bundle.spec.require_projection_validation &&
      !bundle.world_options.require_projection_validation) {
    throw std::runtime_error(
        "[replay_source] EpisodeSpec requires projection validation but "
        "replay_world_options disabled it");
  }
  const auto expected_frames =
      static_cast<std::size_t>(bundle.spec.accepted_range.anchor_index_end -
                               bundle.spec.accepted_range.anchor_index_begin);
  if (bundle.frames.size() != expected_frames) {
    throw std::runtime_error(
        "[replay_source] replay episode bundle frame count mismatch");
  }
  auto frame_validation_options = bundle.world_options;
  frame_validation_options.require_projection_validation = false;
  for (std::size_t i = 0; i < bundle.frames.size(); ++i) {
    detail::validate_frame(bundle.frames[i], bundle.spec, i,
                           frame_validation_options);
    if (i + 1 < bundle.frames.size()) {
      detail::validate_frame_sequence(bundle.frames[i], bundle.frames[i + 1]);
    }
  }
  const auto A = bundle.spec.risky_node_ids.size();
  if (bundle.realized_return_projection.risky_node_ids !=
      bundle.spec.risky_node_ids) {
    throw std::runtime_error(
        "[replay_source] replay episode bundle projection node mismatch");
  }
  if (bundle.realized_return_projection.truth_id !=
      kDirectEdgeRealizedReturnTruthV1) {
    throw std::runtime_error(
        "[replay_source] replay episode bundle projection truth id mismatch");
  }
  if (bundle.realized_return_projection.reference_node_id !=
      bundle.spec.base_policy.projection_reference_node_id) {
    throw std::runtime_error(
        "[replay_source] replay episode bundle projection reference mismatch");
  }
  if (bundle.realized_return_projection.edge_ids.size() != A) {
    throw std::runtime_error(
        "[replay_source] replay episode bundle projection edge count mismatch");
  }
  std::unordered_map<std::string, bool> seen_projection_edges;
  seen_projection_edges.reserve(
      bundle.realized_return_projection.edge_ids.size());
  for (const auto &edge_id : bundle.realized_return_projection.edge_ids) {
    if (edge_id.empty()) {
      throw std::runtime_error(
          "[replay_source] replay episode bundle projection edge_id is empty");
    }
    const auto [_, inserted] = seen_projection_edges.emplace(edge_id, true);
    if (!inserted) {
      throw std::runtime_error(
          "[replay_source] replay episode bundle projection edge_id duplicate");
    }
  }
  if (!bundle.realized_return_projection.signs.defined() ||
      bundle.realized_return_projection.signs.dim() != 1 ||
      bundle.realized_return_projection.signs.size(0) !=
          static_cast<std::int64_t>(A)) {
    throw std::runtime_error(
        "[replay_source] replay episode bundle projection signs must be [A]");
  }
  auto signs = bundle.realized_return_projection.signs.to(torch::kFloat64);
  if (!torch::isfinite(signs).all().item<bool>()) {
    throw std::runtime_error("[replay_source] replay episode bundle projection "
                             "signs are non-finite");
  }
  for (std::int64_t a = 0; a < signs.size(0); ++a) {
    const auto sign = signs.index({a}).item<double>();
    if (sign != 1.0 && sign != -1.0) {
      throw std::runtime_error("[replay_source] replay episode bundle "
                               "projection signs must be +/-1");
    }
  }
}

inline void validate_replay_episode_bundle_ready_for_world(
    const replay_episode_bundle_t &bundle) {
  validate_replay_episode_bundle(bundle);
  const auto expected_frames =
      static_cast<std::size_t>(bundle.spec.accepted_range.anchor_index_end -
                               bundle.spec.accepted_range.anchor_index_begin);
  if (bundle.frames.size() != expected_frames) {
    throw std::runtime_error(
        "[replay_source] replay episode bundle frame count mismatch");
  }
  for (std::size_t i = 0; i < bundle.frames.size(); ++i) {
    detail::validate_frame(bundle.frames[i], bundle.spec, i,
                           bundle.world_options);
    if (i + 1 < bundle.frames.size()) {
      detail::validate_frame_sequence(bundle.frames[i], bundle.frames[i + 1]);
    }
  }
}

[[nodiscard]] inline requested_episode_range_t
requested_episode_range_from_wave(
    const protocol::component_stream_wave_t &wave) {
  requested_episode_range_t out{};
  if (wave.anchor_index_begin.has_value()) {
    out.anchor_index_begin =
        replay_source_detail::to_i64_checked(*wave.anchor_index_begin);
  }
  if (wave.anchor_index_end.has_value()) {
    out.anchor_index_end =
        replay_source_detail::to_i64_checked(*wave.anchor_index_end);
  }
  out.source_key_begin = wave.source_key_begin;
  out.source_key_end = wave.source_key_end;
  return out;
}

template <typename KeyT>
[[nodiscard]] accepted_episode_range_t accepted_episode_range_from_cursor(
    const protocol::component_stream_wave_t &wave,
    const replay_source_detail::dataloader::graph_anchor_cursor_t<KeyT>
        &cursor) {
  static_assert(std::is_integral_v<KeyT>,
                "replay source V1 requires integral timestamp-like keys");
  auto [begin, end] = replay_source_detail::accepted_anchor_interval(cursor);
  accepted_episode_range_t out{};
  out.cursor = protocol::make_component_stream_cursor(wave, cursor);
  out.anchor_index_begin = begin;
  out.anchor_index_end = end;
  out.anchor_keys =
      replay_source_detail::stringify_anchor_keys(cursor.anchor_keys);
  return out;
}

template <typename KeyT>
[[nodiscard]] episode_spec_t bind_episode_spec_to_graph_cursor(
    episode_spec_t spec, const protocol::component_stream_wave_t &wave,
    const replay_source_detail::dataloader::graph_anchor_cursor_t<KeyT>
        &cursor) {
  if (spec.graph_order_fingerprint.empty()) {
    spec.graph_order_fingerprint = cursor.graph_order_fingerprint;
  } else if (spec.graph_order_fingerprint != cursor.graph_order_fingerprint) {
    throw std::runtime_error(
        "[replay_source] EpisodeSpec graph fingerprint does not match cursor");
  }
  spec.requested_wave = wave;
  spec.requested_range = requested_episode_range_from_wave(wave);
  spec.accepted_range = accepted_episode_range_from_cursor(wave, cursor);
  return spec;
}

template <typename KeyT>
[[nodiscard]] torch::Tensor realized_log_returns_from_graph_anchor_edge_batch(
    const replay_source_detail::dataloader::graph_anchor_edge_batch_t<KeyT>
        &batch,
    const replay_source_detail::market_graph_t &graph,
    const episode_spec_t &spec, replay_frame_build_options_t options = {}) {
  static_assert(std::is_integral_v<KeyT>,
                "replay source V1 requires integral timestamp-like keys");
  validate_episode_spec(spec);
  if (options.require_v1_unified_base_policy) {
    replay_source_detail::require_v1_unified_base_policy(spec.base_policy);
  }

  const auto graph_fingerprint = graph.computed_graph_order_fingerprint();
  if (options.require_graph_fingerprint_match &&
      graph_fingerprint != spec.graph_order_fingerprint) {
    throw std::runtime_error(
        "[replay_source] EpisodeSpec graph fingerprint does not match graph");
  }
  if (!batch.graph_order_fingerprint.empty() &&
      batch.graph_order_fingerprint != spec.graph_order_fingerprint) {
    throw std::runtime_error(
        "[replay_source] edge batch graph fingerprint does not match spec");
  }
  if (!batch.cursor.graph_order_fingerprint.empty() &&
      batch.cursor.graph_order_fingerprint != spec.graph_order_fingerprint) {
    throw std::runtime_error(
        "[replay_source] edge batch cursor fingerprint does not match spec");
  }
  replay_source_detail::validate_edge_batch_cursor_matches_spec(batch, spec);
  if (batch.cursor.anchor_keys.size() !=
      spec.accepted_range.anchor_keys.size()) {
    throw std::runtime_error(
        "[replay_source] edge batch cursor key count does not match spec");
  }
  if (!batch.cursor.anchor_keys.empty() &&
      replay_source_detail::stringify_anchor_keys(batch.cursor.anchor_keys) !=
          spec.accepted_range.anchor_keys) {
    throw std::runtime_error(
        "[replay_source] edge batch cursor keys do not match spec");
  }

  TORCH_CHECK(batch.future_features.defined(),
              "[replay_source] future_features are required");
  TORCH_CHECK(batch.future_features.is_floating_point(),
              "[replay_source] future_features must be floating point");
  TORCH_CHECK(batch.future_mask.defined(),
              "[replay_source] future_mask is required");
  TORCH_CHECK(batch.future_features.dim() == 5,
              "[replay_source] future_features must be [B,L,C,Hf,D]");
  TORCH_CHECK(batch.future_mask.dim() == 4,
              "[replay_source] future_mask must be [B,L,C,Hf]");
  const auto B = batch.future_features.size(0);
  const auto L = batch.future_features.size(1);
  const auto C = batch.future_features.size(2);
  const auto Hf = batch.future_features.size(3);
  const auto D = batch.future_features.size(4);
  TORCH_CHECK(batch.future_mask.sizes() == torch::IntArrayRef({B, L, C, Hf}),
              "[replay_source] future_mask shape mismatch");
  TORCH_CHECK(static_cast<std::size_t>(B) ==
                  spec.accepted_range.anchor_keys.size(),
              "[replay_source] frame count must match accepted cursor keys");
  replay_source_detail::validate_edge_batch_anchor_keys_match_spec(batch, spec,
                                                                   B);
  TORCH_CHECK(static_cast<std::size_t>(L) == batch.edge_ids.size(),
              "[replay_source] edge_ids size must match L");
  TORCH_CHECK(options.future_horizon_index >= 0 &&
                  options.future_horizon_index < Hf,
              "[replay_source] future_horizon_index out of range");
  const auto close_coord = replay_source_detail::close_log_return_coord();
  TORCH_CHECK(close_coord >= 0 && close_coord < D,
              "[replay_source] close coordinate outside feature width");

  const auto batch_edge_index =
      replay_source_detail::make_batch_edge_index(batch.edge_ids);
  std::vector<replay_source_detail::direct_base_edge_projection_t> projections;
  std::vector<std::int64_t> batch_edge_indices;
  projections.reserve(spec.risky_node_ids.size());
  batch_edge_indices.reserve(spec.risky_node_ids.size());
  for (const auto &risky_node_id : spec.risky_node_ids) {
    auto projection = replay_source_detail::resolve_direct_base_edge(
        graph, risky_node_id, spec.base_policy.projection_reference_node_id);
    const auto batch_edge = replay_source_detail::require_batch_edge_index(
        batch_edge_index, projection.edge_id);
    projections.push_back(std::move(projection));
    batch_edge_indices.push_back(batch_edge);
  }
  (void)replay_source_detail::shared_realization_keys_for_direct_edges(
      batch, batch_edge_indices, options.future_horizon_index,
      options.realization_channel_index);

  const auto future_features =
      batch.future_features.to(torch::kCPU).to(torch::kFloat64).contiguous();
  const auto future_mask =
      batch.future_mask.to(torch::kCPU).to(torch::kBool).contiguous();
  const auto A = static_cast<std::int64_t>(spec.risky_node_ids.size());
  auto out =
      torch::zeros({B, A}, torch::TensorOptions().dtype(torch::kFloat64));
  for (std::int64_t b = 0; b < B; ++b) {
    for (std::int64_t a = 0; a < A; ++a) {
      const auto edge_index = batch_edge_indices[static_cast<std::size_t>(a)];
      double sum = 0.0;
      std::int64_t count = 0;
      const std::int64_t channel_begin =
          options.realization_channel_index == -2
              ? C - 1
              : (options.realization_channel_index >= 0
                     ? options.realization_channel_index
                     : 0);
      const std::int64_t channel_end =
          options.realization_channel_index == -2
              ? C
              : (options.realization_channel_index >= 0
                     ? options.realization_channel_index + 1
                     : C);
      TORCH_CHECK(channel_end <= C,
                  "[replay_source] realization_channel_index out of range");
      for (std::int64_t c = channel_begin; c < channel_end; ++c) {
        if (!future_mask.index({b, edge_index, c, options.future_horizon_index})
                 .template item<bool>()) {
          continue;
        }
        const double value =
            future_features
                .index({b, edge_index, c, options.future_horizon_index,
                        close_coord})
                .template item<double>();
        if (!std::isfinite(value)) {
          throw std::runtime_error(
              "[replay_source] non-finite future close log-return value");
        }
        sum += value;
        ++count;
      }
      if (count == 0) {
        throw std::runtime_error(
            "[replay_source] no active future close channels for direct edge");
      }
      out.index_put_({b, a}, projections[static_cast<std::size_t>(a)].sign *
                                 (sum / static_cast<double>(count)));
    }
  }
  return out;
}

template <typename KeyT>
[[nodiscard]] std::vector<execution::spot_edge_market_state_t>
relative_edge_market_states_from_graph_anchor_edge_batch(
    const replay_source_detail::dataloader::graph_anchor_edge_batch_t<KeyT>
        &batch,
    const replay_source_detail::market_graph_t &graph,
    replay_frame_build_options_t options = {}) {
  static_assert(std::is_integral_v<KeyT>,
                "replay source V1 requires integral timestamp-like keys");
  graph.validate();
  TORCH_CHECK(batch.edge_features.defined(),
              "[replay_source] edge_features are required for replay "
              "edge_market_state");
  TORCH_CHECK(batch.edge_features.is_floating_point(),
              "[replay_source] edge_features must be floating point");
  TORCH_CHECK(batch.edge_mask.defined(),
              "[replay_source] edge_mask is required for replay "
              "edge_market_state");
  TORCH_CHECK(batch.edge_features.dim() == 5,
              "[replay_source] edge_features must be [B,L,C,Hx,D]");
  TORCH_CHECK(batch.edge_mask.dim() == 4,
              "[replay_source] edge_mask must be [B,L,C,Hx]");

  const auto B = batch.edge_features.size(0);
  const auto L = batch.edge_features.size(1);
  const auto C = batch.edge_features.size(2);
  const auto Hx = batch.edge_features.size(3);
  const auto D = batch.edge_features.size(4);
  TORCH_CHECK(batch.edge_mask.sizes() == torch::IntArrayRef({B, L, C, Hx}),
              "[replay_source] edge_mask shape mismatch");
  TORCH_CHECK(static_cast<std::size_t>(L) == batch.edge_ids.size(),
              "[replay_source] edge_ids size must match L");
  TORCH_CHECK(Hx > 0, "[replay_source] observed edge history is empty");

  const auto close_coord = replay_source_detail::close_log_return_coord();
  TORCH_CHECK(close_coord >= 0 && close_coord < D,
              "[replay_source] close coordinate outside feature width");
  const auto batch_edge_index =
      replay_source_detail::make_batch_edge_index(batch.edge_ids);
  std::vector<std::int64_t> graph_to_batch_edge;
  graph_to_batch_edge.reserve(static_cast<std::size_t>(graph.num_edges()));
  for (const auto &edge_id : graph.edge_ids) {
    graph_to_batch_edge.push_back(
        replay_source_detail::require_batch_edge_index(batch_edge_index,
                                                       edge_id));
  }

  const auto edge_features =
      batch.edge_features.to(torch::kCPU).to(torch::kFloat64).contiguous();
  const auto edge_mask =
      batch.edge_mask.to(torch::kCPU).to(torch::kBool).contiguous();
  const auto E = static_cast<std::int64_t>(graph.num_edges());
  auto cumulative_log_price =
      torch::zeros({E}, torch::TensorOptions().dtype(torch::kFloat64));
  std::vector<execution::spot_edge_market_state_t> out;
  out.reserve(static_cast<std::size_t>(B));
  const std::int64_t terminal_h = Hx - 1;
  const std::int64_t channel_begin =
      options.realization_channel_index == -2
          ? C - 1
          : (options.realization_channel_index >= 0
                 ? options.realization_channel_index
                 : 0);
  const std::int64_t channel_end =
      options.realization_channel_index == -2
          ? C
          : (options.realization_channel_index >= 0
                 ? options.realization_channel_index + 1
                 : C);
  TORCH_CHECK(channel_begin >= 0 && channel_begin < C && channel_end <= C &&
                  channel_begin < channel_end,
              "[replay_source] realization_channel_index out of range");

  for (std::int64_t b = 0; b < B; ++b) {
    if (b > 0) {
      for (std::int64_t e = 0; e < E; ++e) {
        const auto batch_edge =
            graph_to_batch_edge[static_cast<std::size_t>(e)];
        double sum = 0.0;
        std::int64_t count = 0;
        for (std::int64_t c = channel_begin; c < channel_end; ++c) {
          if (!edge_mask.index({b, batch_edge, c, terminal_h})
                   .template item<bool>()) {
            continue;
          }
          const double value =
              edge_features.index({b, batch_edge, c, terminal_h, close_coord})
                  .template item<double>();
          if (!std::isfinite(value)) {
            throw std::runtime_error(
                "[replay_source] non-finite observed close log-return value");
          }
          sum += value;
          ++count;
        }
        if (count == 0) {
          throw std::runtime_error(
              "[replay_source] no active observed close channels for edge");
        }
        cumulative_log_price.index_put_(
            {e}, cumulative_log_price.index({e}).template item<double>() +
                     (sum / static_cast<double>(count)));
      }
    }

    execution::spot_edge_market_state_t market{};
    market.graph = graph;
    market.edge_mid_price = cumulative_log_price.exp().clone();
    execution::validate_spot_edge_market_state(market);
    out.push_back(std::move(market));
  }
  return out;
}

template <typename KeyT>
[[nodiscard]] std::vector<replay_frame_t>
make_replay_frames_from_graph_anchor_edge_batch(
    const replay_source_detail::dataloader::graph_anchor_edge_batch_t<KeyT>
        &batch,
    const replay_source_detail::market_graph_t &graph,
    const episode_spec_t &spec, replay_frame_build_options_t options = {}) {
  const auto realized_log_return =
      realized_log_returns_from_graph_anchor_edge_batch(batch, graph, spec,
                                                        options);
  const auto edge_market_states =
      relative_edge_market_states_from_graph_anchor_edge_batch(batch, graph,
                                                               options);
  const auto B = realized_log_return.size(0);
  TORCH_CHECK(static_cast<std::int64_t>(edge_market_states.size()) == B,
              "[replay_source] edge market state count must match frames");
  TORCH_CHECK(batch.future_keys.defined(),
              "[replay_source] future_keys are required to build observations");
  TORCH_CHECK(batch.future_keys.dim() == 4,
              "[replay_source] future_keys must be [B,L,C,Hf]");

  const auto batch_edge_index =
      replay_source_detail::make_batch_edge_index(batch.edge_ids);
  std::vector<std::int64_t> direct_batch_edges;
  direct_batch_edges.reserve(spec.risky_node_ids.size());
  for (const auto &risky_node_id : spec.risky_node_ids) {
    auto projection = replay_source_detail::resolve_direct_base_edge(
        graph, risky_node_id, spec.base_policy.projection_reference_node_id);
    direct_batch_edges.push_back(replay_source_detail::require_batch_edge_index(
        batch_edge_index, projection.edge_id));
  }
  const auto realization_keys =
      replay_source_detail::shared_realization_keys_for_direct_edges(
          batch, direct_batch_edges, options.future_horizon_index,
          options.realization_channel_index);

  std::vector<replay_frame_t> frames;
  frames.reserve(static_cast<std::size_t>(B));
  for (std::int64_t b = 0; b < B; ++b) {
    const KeyT anchor_key = replay_source_detail::anchor_key_at<KeyT>(batch, b);
    const KeyT realization_key =
        realization_keys.at(static_cast<std::size_t>(b));

    observation_t observation{};
    observation.anchor_key = replay_source_detail::key_to_string(anchor_key);
    observation.timestamp_ms = replay_source_detail::to_i64_checked(anchor_key);
    observation.observation_anchor_index =
        spec.accepted_range.anchor_index_begin + b;
    observation.next_realization_anchor_index =
        observation.observation_anchor_index + 1;
    observation.knowledge_timestamp_ms = observation.timestamp_ms;
    observation.realization_available_after_timestamp_ms =
        replay_source_detail::to_i64_checked(realization_key);
    observation.portfolio_state.timestamp_ms = observation.timestamp_ms;
    observation.portfolio_state.accounting_node_id =
        spec.base_policy.accounting_numeraire_id;
    observation.portfolio_state.reserve_node_id =
        spec.base_policy.reserve_asset_id;
    observation.portfolio_state.current_weights =
        torch::zeros({static_cast<std::int64_t>(spec.risky_node_ids.size())},
                     torch::TensorOptions().dtype(torch::kFloat64));
    observation.portfolio_state.current_units =
        torch::zeros({static_cast<std::int64_t>(spec.risky_node_ids.size())},
                     torch::TensorOptions().dtype(torch::kFloat64));
    observation.portfolio_state.base_reserve_weight = 1.0;
    observation.portfolio_state.equity_value_base = spec.initial_equity_base;
    observation.market_state.timestamp_ms = observation.timestamp_ms;
    observation.edge_market_state =
        edge_market_states[static_cast<std::size_t>(b)];

    replay_frame_t frame{};
    frame.observation = std::move(observation);
    frame.realized_log_return = realized_log_return.index({b}).clone();
    frame.realized_arithmetic_return = frame.realized_log_return.exp() - 1.0;
    frames.push_back(std::move(frame));
  }
  return frames;
}

template <typename KeyT>
[[nodiscard]] replay_episode_bundle_t
make_replay_episode_bundle_from_graph_anchor_edge_batch(
    episode_spec_t base_spec, const protocol::component_stream_wave_t &wave,
    const replay_source_detail::dataloader::graph_anchor_cursor_t<KeyT> &cursor,
    const replay_source_detail::dataloader::graph_anchor_edge_batch_t<KeyT>
        &batch,
    const replay_source_detail::market_graph_t &graph,
    replay_frame_build_options_t frame_options = {},
    replay_world_options_t world_options = {}) {
  auto spec =
      bind_episode_spec_to_graph_cursor(std::move(base_spec), wave, cursor);
  if (spec.graph_node_ids.empty()) {
    spec.graph_node_ids = graph.node_ids;
  } else if (spec.graph_node_ids != graph.node_ids) {
    throw std::runtime_error(
        "[replay_source] EpisodeSpec graph_node_ids do not match graph");
  }
  auto projection_plan =
      make_direct_edge_projection_plan(graph, spec, frame_options);
  spec.realized_return_projection =
      realized_return_projection_spec_from_plan(projection_plan);
  auto frames = make_replay_frames_from_graph_anchor_edge_batch(
      batch, graph, spec, frame_options);
  replay_episode_bundle_t out{};
  out.spec = std::move(spec);
  out.frames = std::move(frames);
  out.frame_options = frame_options;
  out.world_options = world_options;
  out.realized_return_projection = std::move(projection_plan);
  validate_replay_episode_bundle(out);
  return out;
}

[[nodiscard]] inline std::unique_ptr<world_iface_t>
spawn_replay_world(const replay_episode_bundle_t &bundle) {
  validate_replay_episode_bundle_ready_for_world(bundle);
  return std::make_unique<replay_world_t>(bundle.frames, bundle.world_options);
}

[[nodiscard]] inline std::unique_ptr<world_iface_t>
spawn_replay_world(replay_episode_bundle_t &&bundle) {
  validate_replay_episode_bundle_ready_for_world(bundle);
  return std::make_unique<replay_world_t>(std::move(bundle.frames),
                                          bundle.world_options);
}

} // namespace cuwacunu::kikijyeba::environment::replay
