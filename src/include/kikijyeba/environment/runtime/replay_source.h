// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "hero/runtime_hero/runtime/job_layout.h"
#include "hero/runtime_hero/runtime/job_manifest.h"
#include "hero/runtime_hero/runtime/wave_plan.h"
#include "kikijyeba/environment/replay/bundle_source.h"
#include "kikijyeba/topology/graph/graph.h"
#include "wikimyei/inference/expected_value/mdn/stream/mdn_adapter.h"
#include "wikimyei/observer/belief/builder.h"
#include "wikimyei/observer/belief/spec.h"
#include "wikimyei/representation/encoding/vicreg/stream/channel_representation_batch.h"

namespace cuwacunu::kikijyeba::environment::replay {

namespace replay_runtime_detail {

namespace runtime = cuwacunu::hero::runtime;
namespace dataloader = cuwacunu::ujcamei::source::retrieval::dataloader;
namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
namespace mdn_stream =
    cuwacunu::wikimyei::inference::expected_value::mdn::stream;

[[nodiscard]] inline bool blank(const std::string &text) {
  return cuwacunu::kikijyeba::environment::detail::blank(text);
}

template <typename KeyT>
inline void
require_key_match(const std::string &actual, const char *actual_name,
                  std::optional<KeyT> expected, const char *expected_name) {
  if (actual.empty() || !expected.has_value()) {
    return;
  }
  const auto expected_text = replay_source_detail::key_to_string(*expected);
  if (actual != expected_text) {
    throw std::runtime_error(std::string("[replay_runtime_source] ") +
                             actual_name + " does not match " + expected_name);
  }
}

inline void require_same_if_present(const std::string &lhs,
                                    const char *lhs_name,
                                    const std::string &rhs,
                                    const char *rhs_name) {
  if (lhs.empty() || rhs.empty() || lhs == rhs) {
    return;
  }
  throw std::runtime_error(std::string("[replay_runtime_source] ") + lhs_name +
                           " does not match " + rhs_name);
}

[[nodiscard]] inline std::size_t
parse_size_or(const std::unordered_map<std::string, std::string> &map,
              const std::string &key, std::size_t fallback = 0) {
  const auto value = runtime::job_layout::map_get(map, key);
  if (value.empty()) {
    return fallback;
  }
  return static_cast<std::size_t>(std::stoull(value));
}

[[nodiscard]] inline std::int64_t
parse_i64_or(const std::unordered_map<std::string, std::string> &map,
             const std::string &key, std::int64_t fallback = 0) {
  const auto value = runtime::job_layout::map_get(map, key);
  if (value.empty()) {
    return fallback;
  }
  return static_cast<std::int64_t>(std::stoll(value));
}

[[nodiscard]] inline double
parse_double_or(const std::unordered_map<std::string, std::string> &map,
                const std::string &key, double fallback = 0.0) {
  const auto value = runtime::job_layout::map_get(map, key);
  if (value.empty()) {
    return fallback;
  }
  return std::stod(value);
}

[[nodiscard]] inline std::optional<std::int64_t>
parse_optional_i64(const std::string &value) {
  if (blank(value)) {
    return std::nullopt;
  }
  return static_cast<std::int64_t>(std::stoll(value));
}

[[nodiscard]] inline std::vector<std::string>
split_csv(const std::string &csv) {
  std::vector<std::string> out;
  std::stringstream stream(csv);
  std::string item;
  while (std::getline(stream, item, ',')) {
    item = runtime::job_layout::trim_ascii(item);
    if (!item.empty()) {
      out.push_back(std::move(item));
    }
  }
  return out;
}

[[nodiscard]] inline std::string
target_coords_fingerprint(const std::vector<std::int64_t> &target_coords) {
  std::ostringstream out;
  out << "target_coords";
  for (const auto coord : target_coords) {
    out << ":" << coord;
  }
  return out.str();
}

[[nodiscard]] inline torch::Tensor
tensor_or_empty_cpu(const torch::Tensor &tensor,
                    torch::ScalarType dtype = torch::ScalarType::Undefined) {
  if (!tensor.defined()) {
    return torch::empty({0}, torch::TensorOptions().dtype(torch::kFloat64));
  }
  auto out = tensor.detach().to(torch::kCPU);
  if (dtype != torch::ScalarType::Undefined) {
    out = out.to(dtype);
  }
  return out.contiguous();
}

[[nodiscard]] inline mdn::MdnOut mdn_out_to_cpu(const mdn::MdnOut &out) {
  mdn::MdnOut cpu{};
  cpu.log_pi = tensor_or_empty_cpu(out.log_pi, torch::kFloat64);
  cpu.mu = tensor_or_empty_cpu(out.mu, torch::kFloat64);
  cpu.sigma = tensor_or_empty_cpu(out.sigma, torch::kFloat64);
  return cpu;
}

inline void write_string(torch::serialize::OutputArchive &root,
                         const std::string &key, const std::string &value) {
  forecast::detail::write_string(root, key, value);
}

inline void read_string(torch::serialize::InputArchive &root,
                        const std::string &key, std::string *out) {
  forecast::detail::read_string(root, key, out);
}

inline void write_string_list(torch::serialize::OutputArchive &root,
                              const std::string &key_prefix,
                              const std::vector<std::string> &values) {
  forecast::detail::write_string_list(root, key_prefix, values);
}

inline void read_string_list(torch::serialize::InputArchive &root,
                             const std::string &key_prefix,
                             std::vector<std::string> *out) {
  forecast::detail::read_string_list(root, key_prefix, out);
}

template <typename KeyT>
[[nodiscard]] torch::Tensor
key_vector_to_tensor(const std::vector<KeyT> &values) {
  std::vector<std::int64_t> converted;
  converted.reserve(values.size());
  for (const auto value : values) {
    converted.push_back(static_cast<std::int64_t>(value));
  }
  return torch::tensor(converted, torch::TensorOptions().dtype(torch::kInt64));
}

template <typename KeyT>
[[nodiscard]] std::vector<KeyT>
tensor_to_key_vector(const torch::Tensor &tensor, const char *name) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return {};
  }
  auto cpu = tensor.to(torch::kCPU).to(torch::kInt64).contiguous().view({-1});
  std::vector<KeyT> out;
  out.reserve(static_cast<std::size_t>(cpu.numel()));
  auto accessor = cpu.accessor<std::int64_t, 1>();
  for (std::int64_t i = 0; i < cpu.numel(); ++i) {
    const auto value = accessor[i];
    if constexpr (std::is_unsigned_v<KeyT>) {
      if (value < 0) {
        throw std::runtime_error(std::string("[replay_runtime_source] ") +
                                 name + " contains negative value");
      }
    }
    out.push_back(static_cast<KeyT>(value));
  }
  return out;
}

[[nodiscard]] inline std::vector<std::size_t>
tensor_to_size_vector(const torch::Tensor &tensor, const char *name) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return {};
  }
  auto cpu = tensor.to(torch::kCPU).to(torch::kInt64).contiguous().view({-1});
  std::vector<std::size_t> out;
  out.reserve(static_cast<std::size_t>(cpu.numel()));
  auto accessor = cpu.accessor<std::int64_t, 1>();
  for (std::int64_t i = 0; i < cpu.numel(); ++i) {
    if (accessor[i] < 0) {
      throw std::runtime_error(std::string("[replay_runtime_source] ") + name +
                               " contains negative value");
    }
    out.push_back(static_cast<std::size_t>(accessor[i]));
  }
  return out;
}

[[nodiscard]] inline torch::Tensor
size_vector_to_tensor(const std::vector<std::size_t> &values) {
  std::vector<std::int64_t> converted;
  converted.reserve(values.size());
  for (const auto value : values) {
    converted.push_back(static_cast<std::int64_t>(value));
  }
  return torch::tensor(converted, torch::TensorOptions().dtype(torch::kInt64));
}

} // namespace replay_runtime_detail

struct runtime_replay_job_evidence_read_options_t {
  std::string source_cursor_kind{"graph_anchor"};
  std::string source_cursor_scope{"episode"};
  bool use_resolved_range_as_requested_anchor_range{true};
};

struct runtime_replay_job_evidence_t {
  std::filesystem::path job_dir{};
  std::filesystem::path manifest_path{};
  replay_runtime_detail::runtime::job_manifest_t manifest{};
  replay_runtime_detail::runtime::wave_plan_t wave_plan{};
};

inline constexpr const char *kGraphAnchorEdgeBatchArtifactSchema =
    "kikijyeba.environment.replay.graph_anchor_edge_batch_artifact.v1";
inline constexpr const char *kRuntimeReplayArtifactPathIndexSchema =
    "kikijyeba.environment.replay.runtime_artifact_paths.v1";
inline constexpr const char *kRuntimeReplayBatchIndexSchema =
    "kikijyeba.environment.replay.runtime_batch_index.v1";

struct runtime_replay_artifact_paths_t {
  std::filesystem::path graph_anchor_edge_batch_artifact_path{};
  std::filesystem::path observation_artifact_index_path{};
};

struct runtime_replay_batch_index_entry_t {
  std::size_t wave_pulse_index{0};
  std::size_t begin_anchor_index{0};
  std::size_t end_anchor_index{0};
  std::size_t anchor_count{0};
  std::string batch_cursor_token{};
  std::filesystem::path artifact_path_index_path{};
};

struct runtime_replay_forecast_artifact_record_t {
  forecast::marginal_forecast_artifact_t forecast_artifact{};
  std::optional<std::int64_t> observation_anchor_index{std::nullopt};
  artifact_ref_t mdn_artifact{};
  std::filesystem::path forecast_artifact_path{};
};

struct runtime_allocation_belief_observer_options_t {
  std::string accounting_numeraire_node_id{};
  std::vector<std::string> target_node_ids{};
  bool require_direct_accounting_numeraire_valuation_edges{true};
  bool use_identity_potential_correlation{true};
  double default_linear_cost{0.0};
  double default_quadratic_impact{0.0};
  double default_capacity_weight_limit{1.0};
  double default_data_quality_score{1.0};
  double channel_disagreement_confidence_scale{1.0};
};

[[nodiscard]] inline std::optional<std::size_t>
node_index_by_id(const std::vector<std::string> &node_ids,
                 const std::string &node_id) {
  for (std::size_t i = 0; i < node_ids.size(); ++i) {
    if (node_ids[i] == node_id) {
      return i;
    }
  }
  return std::nullopt;
}

[[nodiscard]] inline bool has_direct_node_pair_edge(
    const cuwacunu::kikijyeba::topology::graph::market_graph_t &graph,
    const std::string &lhs_node_id, const std::string &rhs_node_id) {
  for (std::size_t e = 0; e < graph.edge_ids.size(); ++e) {
    const auto base_index = graph.base_index[e];
    const auto quote_index = graph.quote_index[e];
    if (base_index < 0 || quote_index < 0 ||
        static_cast<std::size_t>(base_index) >= graph.node_ids.size() ||
        static_cast<std::size_t>(quote_index) >= graph.node_ids.size()) {
      continue;
    }
    const auto &base_node =
        graph.node_ids[static_cast<std::size_t>(base_index)];
    const auto &quote_node =
        graph.node_ids[static_cast<std::size_t>(quote_index)];
    if ((base_node == lhs_node_id && quote_node == rhs_node_id) ||
        (base_node == rhs_node_id && quote_node == lhs_node_id)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] inline std::vector<std::string> default_target_node_ids(
    const cuwacunu::kikijyeba::topology::graph::market_graph_t &graph) {
  if (graph.node_ids.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] graph node universe is empty");
  }
  return graph.node_ids;
}

[[nodiscard]] inline belief::allocation_belief_builder_options_t
make_runtime_allocation_belief_builder_options(
    const cuwacunu::kikijyeba::topology::graph::market_graph_t &graph,
    const belief::belief_observer_spec_t &spec,
    runtime_allocation_belief_observer_options_t options = {}) {
  belief::validate_belief_observer_spec(spec);
  graph.validate();
  if (options.accounting_numeraire_node_id.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] accounting numeraire node is required; set "
        "[ACCOUNTING].accounting_numeraire_node_id in .config or pass an "
        "explicit replay accounting numeraire override");
  }
  const std::string accounting_numeraire_node_id =
      options.accounting_numeraire_node_id;
  const auto numeraire_index =
      node_index_by_id(graph.node_ids, accounting_numeraire_node_id);
  if (!numeraire_index.has_value()) {
    throw std::runtime_error("[replay_runtime_source] accounting numeraire "
                             "node must appear in graph");
  }

  auto target_node_ids = options.target_node_ids.empty()
                             ? default_target_node_ids(graph)
                             : std::move(options.target_node_ids);
  std::vector<std::int64_t> target_indices;
  target_indices.reserve(target_node_ids.size());
  for (const auto &node_id : target_node_ids) {
    const auto index = node_index_by_id(graph.node_ids, node_id);
    if (!index.has_value()) {
      throw std::runtime_error(
          "[replay_runtime_source] target node is not present in graph: " +
          node_id);
    }
    if (options.require_direct_accounting_numeraire_valuation_edges &&
        node_id != accounting_numeraire_node_id &&
        !has_direct_node_pair_edge(graph, node_id,
                                   accounting_numeraire_node_id)) {
      throw std::runtime_error(
          "[replay_runtime_source] V1 replay allocation requires a direct "
          "accounting-numeraire valuation edge for target node: " +
          node_id + " / " + accounting_numeraire_node_id);
    }
    target_indices.push_back(static_cast<std::int64_t>(*index));
  }

  if (!options.use_identity_potential_correlation) {
    throw std::runtime_error(
        "[replay_runtime_source] runtime empirical potential correlation "
        "sources are not wired yet; V1 uses identity correlation unless an "
        "explicit builder option is supplied");
  }
  const auto A = static_cast<std::int64_t>(target_node_ids.size());
  auto tensor_options = torch::TensorOptions().dtype(torch::kFloat64);

  belief::allocation_belief_builder_options_t out{};
  out.graph_order_fingerprint = graph.computed_graph_order_fingerprint();
  out.graph_node_ids = graph.node_ids;
  out.graph_node_axis = belief::make_graph_node_axis_binding(
      out.graph_order_fingerprint, graph.node_ids,
      "future_channel_node_distribution");
  out.node_ids = std::move(target_node_ids);
  out.node_graph_indices = std::move(target_indices);
  out.base_policy = {
      .accounting_numeraire_id = accounting_numeraire_node_id,
      .settlement_asset_id = accounting_numeraire_node_id,
      .projection_reference_node_id = accounting_numeraire_node_id,
  };
  out.projection_reference_graph_index =
      static_cast<std::int64_t>(*numeraire_index);
  const bool target_nodes_include_projection_reference =
      std::find(out.node_graph_indices.begin(), out.node_graph_indices.end(),
                *numeraire_index) != out.node_graph_indices.end();
  const auto projection_universe_count =
      A + (target_nodes_include_projection_reference ? 0 : 1);
  out.empirical_potential_correlation =
      torch::eye(projection_universe_count, tensor_options);
  out.tradable_mask =
      torch::ones({A}, torch::TensorOptions().dtype(torch::kBool));
  out.data_quality_score =
      torch::full({A}, options.default_data_quality_score, tensor_options)
          .clamp(0.0, 1.0);
  out.linear_cost =
      torch::full({A}, options.default_linear_cost, tensor_options)
          .clamp_min(0.0);
  out.quadratic_impact =
      torch::full({A}, options.default_quadratic_impact, tensor_options)
          .clamp_min(0.0);
  out.capacity_weight_limit =
      torch::full({A}, options.default_capacity_weight_limit, tensor_options)
          .clamp(0.0, 1.0);
  out.projection_options.coupling_options.sample_count = spec.scenario_count;
  out.flow_liquidity_options.neutral_when_missing = true;
  out.flow_liquidity_options.neutral_liquidity_score = 1.0;
  out.flow_liquidity_options.neutral_capacity_weight_limit = 1.0;
  out.channel_disagreement_confidence_scale =
      options.channel_disagreement_confidence_scale;
  return out;
}

[[nodiscard]] inline runtime_replay_forecast_artifact_record_t
make_runtime_replay_forecast_record_from_observer_build(
    const belief::allocation_belief_build_result_t &build_result,
    std::optional<std::int64_t> observation_anchor_index = std::nullopt,
    artifact_ref_t mdn_artifact = {}, std::string model_version = {},
    std::string target_coords_hash = {}, std::string normalization_hash = {},
    std::filesystem::path forecast_artifact_path = {}) {
  auto artifact = forecast::make_from_allocation_belief_and_projection(
      build_result.allocation_belief, build_result.return_projection,
      std::move(model_version), std::move(target_coords_hash),
      std::move(normalization_hash), &build_result.scenario_bank);
  return {
      .forecast_artifact = std::move(artifact),
      .observation_anchor_index = observation_anchor_index,
      .mdn_artifact = std::move(mdn_artifact),
      .forecast_artifact_path = std::move(forecast_artifact_path),
  };
}

template <typename KeyT>
[[nodiscard]] std::vector<runtime_replay_forecast_artifact_record_t>
make_runtime_replay_forecast_records_from_mdn_batch(
    const replay_runtime_detail::mdn::MdnOut &out,
    const replay_runtime_detail::mdn_stream::channel_mdn_input_batch_t<KeyT>
        &batch,
    belief::allocation_belief_builder_options_t common_options,
    artifact_ref_t mdn_artifact = {}, std::string model_version = {},
    std::string target_coords_hash = {}, std::string normalization_hash = {}) {
  static_assert(std::is_integral_v<KeyT>,
                "Runtime replay MDN batch records require integral keys");
  const auto cpu_out = replay_runtime_detail::mdn_out_to_cpu(out);
  TORCH_CHECK(cpu_out.log_pi.defined() && cpu_out.log_pi.dim() == 5,
              "[replay_runtime_source] MdnOut must be [B,N,C,Df,K]");
  TORCH_CHECK(cpu_out.mu.sizes() == cpu_out.log_pi.sizes() &&
                  cpu_out.sigma.sizes() == cpu_out.log_pi.sizes(),
              "[replay_runtime_source] MdnOut tensor shapes must match");
  const auto B = cpu_out.log_pi.size(0);
  TORCH_CHECK(batch.context_mask.defined() && batch.context_mask.dim() == 3,
              "[replay_runtime_source] channel MDN context_mask must be "
              "[B,N,C]");
  TORCH_CHECK(batch.context_mask.size(0) == B &&
                  batch.context_mask.size(1) == cpu_out.log_pi.size(1) &&
                  batch.context_mask.size(2) == cpu_out.log_pi.size(2),
              "[replay_runtime_source] context_mask shape does not match "
              "MdnOut");
  TORCH_CHECK(!batch.node_ids.empty(),
              "[replay_runtime_source] channel MDN batch node_ids are "
              "required");
  TORCH_CHECK(static_cast<std::int64_t>(batch.node_ids.size()) ==
                  cpu_out.log_pi.size(1),
              "[replay_runtime_source] node_ids size must match MdnOut N");
  TORCH_CHECK(batch.anchor_keys.defined() && batch.anchor_keys.dim() == 1 &&
                  batch.anchor_keys.size(0) == B,
              "[replay_runtime_source] channel MDN anchor_keys must be [B]");

  auto anchor_keys = replay_runtime_detail::tensor_to_key_vector<KeyT>(
      batch.anchor_keys, "anchor_keys");
  if (common_options.graph_order_fingerprint.empty()) {
    common_options.graph_order_fingerprint = batch.graph_order_fingerprint;
  } else if (!batch.graph_order_fingerprint.empty() &&
             common_options.graph_order_fingerprint !=
                 batch.graph_order_fingerprint) {
    throw std::runtime_error(
        "[replay_runtime_source] common graph fingerprint does not match "
        "channel MDN batch");
  }
  if (common_options.graph_node_ids.empty()) {
    common_options.graph_node_ids = batch.node_ids;
  } else if (common_options.graph_node_ids != batch.node_ids) {
    throw std::runtime_error(
        "[replay_runtime_source] common graph_node_ids do not match channel "
        "MDN batch node_ids");
  }
  if (!common_options.channel_mask.defined()) {
    common_options.channel_mask =
        batch.context_mask.detach().to(torch::kCPU).to(torch::kBool);
  }
  if (target_coords_hash.empty()) {
    target_coords_hash =
        replay_runtime_detail::target_coords_fingerprint(batch.target_coords);
  }

  std::vector<runtime_replay_forecast_artifact_record_t> records;
  records.reserve(static_cast<std::size_t>(B));
  for (std::int64_t b = 0; b < B; ++b) {
    auto options = common_options;
    options.anchor_slot = b;
    options.anchor_key = replay_source_detail::key_to_string(
        anchor_keys[static_cast<std::size_t>(b)]);
    options.timestamp_ms = replay_source_detail::to_i64_checked(
        anchor_keys[static_cast<std::size_t>(b)]);
    auto build_result =
        belief::build_single_anchor_allocation_belief(cpu_out, options);
    std::optional<std::int64_t> observation_anchor_index{};
    if (batch.cursor.anchor_indices.size() == static_cast<std::size_t>(B)) {
      observation_anchor_index = replay_source_detail::to_i64_checked(
          batch.cursor.anchor_indices[static_cast<std::size_t>(b)]);
    } else if (!batch.cursor.empty()) {
      observation_anchor_index = replay_source_detail::to_i64_checked(
                                     batch.cursor.begin_anchor_index) +
                                 b;
    }
    records.push_back(make_runtime_replay_forecast_record_from_observer_build(
        build_result, observation_anchor_index, mdn_artifact, model_version,
        target_coords_hash, normalization_hash));
  }
  return records;
}

template <typename KeyT>
inline void save_graph_anchor_edge_batch_artifact(
    const replay_source_detail::dataloader::graph_anchor_edge_batch_t<KeyT>
        &batch,
    const std::filesystem::path &path);

template <typename KeyT>
[[nodiscard]] replay_source_detail::dataloader::graph_anchor_edge_batch_t<KeyT>
make_runtime_replay_edge_batch_from_channel_representation(
    const cuwacunu::wikimyei::representation::encoding::vicreg::stream::
        channel_representation_batch_t<KeyT> &batch) {
  static_assert(std::is_integral_v<KeyT>,
                "Replay edge batch V1 requires integral keys");
  TORCH_CHECK(batch.future_edge_features.defined(),
              "[replay_runtime_source] future_edge_features are required");
  TORCH_CHECK(batch.future_edge_mask.defined(),
              "[replay_runtime_source] future_edge_mask is required");
  TORCH_CHECK(batch.edge_features.defined(),
              "[replay_runtime_source] edge_features are required");
  TORCH_CHECK(batch.edge_mask.defined(),
              "[replay_runtime_source] edge_mask is required");
  TORCH_CHECK(batch.past_keys.defined(),
              "[replay_runtime_source] past_keys are required");
  TORCH_CHECK(batch.future_keys.defined(),
              "[replay_runtime_source] future_keys are required");
  TORCH_CHECK(batch.anchor_keys.defined(),
              "[replay_runtime_source] anchor_keys are required");
  TORCH_CHECK(batch.future_edge_features.dim() == 5,
              "[replay_runtime_source] future_edge_features must be "
              "[B,L,C,Hf,9]");
  TORCH_CHECK(batch.future_edge_mask.dim() == 4,
              "[replay_runtime_source] future_edge_mask must be [B,L,C,Hf]");
  TORCH_CHECK(batch.edge_features.dim() == 5,
              "[replay_runtime_source] edge_features must be [B,L,C,Hx,9]");
  TORCH_CHECK(batch.edge_mask.dim() == 4,
              "[replay_runtime_source] edge_mask must be [B,L,C,Hx]");
  TORCH_CHECK(batch.past_keys.dim() == 4,
              "[replay_runtime_source] past_keys must be [B,L,C,Hx]");
  TORCH_CHECK(batch.future_keys.dim() == 4,
              "[replay_runtime_source] future_keys must be [B,L,C,Hf]");
  const auto B = batch.future_edge_features.size(0);
  const auto L = batch.future_edge_features.size(1);
  const auto C = batch.future_edge_features.size(2);
  const auto Hf = batch.future_edge_features.size(3);
  const auto Hx = batch.edge_features.size(3);
  TORCH_CHECK(
      batch.edge_features.sizes() ==
          torch::IntArrayRef(
              {B, L, C, Hx,
               cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth}),
      "[replay_runtime_source] edge_features shape mismatch");
  TORCH_CHECK(batch.edge_mask.sizes() == torch::IntArrayRef({B, L, C, Hx}),
              "[replay_runtime_source] edge_mask shape mismatch");
  TORCH_CHECK(batch.past_keys.sizes() == torch::IntArrayRef({B, L, C, Hx}),
              "[replay_runtime_source] past_keys shape mismatch");
  TORCH_CHECK(batch.future_edge_mask.sizes() ==
                  torch::IntArrayRef({B, L, C, Hf}),
              "[replay_runtime_source] future_edge_mask shape mismatch");
  TORCH_CHECK(batch.future_keys.sizes() == torch::IntArrayRef({B, L, C, Hf}),
              "[replay_runtime_source] future_keys shape mismatch");
  TORCH_CHECK(static_cast<std::int64_t>(batch.edge_ids.size()) == L,
              "[replay_runtime_source] edge_ids size must match future L");
  TORCH_CHECK(batch.anchor_keys.dim() == 1 && batch.anchor_keys.size(0) == B,
              "[replay_runtime_source] anchor_keys must be [B]");

  replay_source_detail::dataloader::graph_anchor_edge_batch_t<KeyT> out{};
  out.edge_features = batch.edge_features;
  out.edge_mask = batch.edge_mask;
  out.past_keys = batch.past_keys;
  out.future_features = batch.future_edge_features;
  out.future_mask = batch.future_edge_mask;
  out.future_keys = batch.future_keys;
  out.anchor_keys = batch.anchor_keys;
  out.edge_ids = batch.edge_ids;
  out.graph_order_fingerprint = batch.graph_order_fingerprint;
  out.cursor = batch.cursor;
  if (out.cursor.empty()) {
    auto anchor_keys = replay_runtime_detail::tensor_to_key_vector<KeyT>(
        batch.anchor_keys, "anchor_keys");
    out.cursor = replay_source_detail::dataloader::make_graph_anchor_cursor(
        batch.graph_order_fingerprint, 0,
        static_cast<std::size_t>(anchor_keys.size()), anchor_keys.size(),
        std::move(anchor_keys));
  }
  if (out.cursor.graph_order_fingerprint.empty()) {
    out.cursor.graph_order_fingerprint = batch.graph_order_fingerprint;
  }
  out.edge_present = batch.future_edge_mask.any(/*dim=*/3).any(/*dim=*/2);
  return out;
}

template <typename KeyT> struct runtime_graph_anchor_replay_record_t {
  replay_runtime_detail::runtime::job_manifest_t manifest{};
  replay_runtime_detail::runtime::wave_plan_t wave_plan{};

  episode_spec_t base_spec{};
  replay_source_detail::dataloader::graph_anchor_cursor_t<KeyT> cursor{};
  replay_source_detail::dataloader::graph_anchor_edge_batch_t<KeyT> batch{};

  replay_frame_build_options_t frame_options{};
  replay_world_options_t world_options{};

  std::string artifact_source_id{};
  std::vector<replay_observation_artifacts_t> observation_artifacts{};
  replay_observation_artifact_options_t artifact_options{};
  bool require_observation_artifacts{false};
};

[[nodiscard]] inline std::filesystem::path
runtime_replay_artifact_dir(const std::filesystem::path &job_dir) {
  return job_dir / "artifacts" / kReplayEnvironmentAssemblyId;
}

[[nodiscard]] inline std::filesystem::path
runtime_replay_artifact_path_index_path(const std::filesystem::path &job_dir) {
  return runtime_replay_artifact_dir(job_dir) /
         "runtime_replay_artifacts.index";
}

[[nodiscard]] inline std::filesystem::path
runtime_replay_batch_index_path(const std::filesystem::path &job_dir) {
  return runtime_replay_artifact_dir(job_dir) / "runtime_replay_batches.index";
}

[[nodiscard]] inline std::string
runtime_replay_pulse_leaf(std::size_t wave_pulse_index,
                          std::string batch_cursor_token = {}) {
  std::ostringstream out;
  out << "pulse_" << std::setw(6) << std::setfill('0') << wave_pulse_index;
  if (!batch_cursor_token.empty()) {
    out << "_"
        << replay_runtime_detail::runtime::job_layout::sanitize_path_component(
               std::move(batch_cursor_token));
  }
  return out.str();
}

[[nodiscard]] inline std::filesystem::path
runtime_replay_pulse_artifact_dir(const std::filesystem::path &job_dir,
                                  std::size_t wave_pulse_index,
                                  std::string batch_cursor_token = {}) {
  return runtime_replay_artifact_dir(job_dir) / "pulses" /
         runtime_replay_pulse_leaf(wave_pulse_index,
                                   std::move(batch_cursor_token));
}

[[nodiscard]] inline runtime_replay_artifact_paths_t
default_runtime_replay_artifact_paths(const std::filesystem::path &job_dir) {
  const auto dir = runtime_replay_artifact_dir(job_dir);
  return {
      .graph_anchor_edge_batch_artifact_path =
          dir / "graph_anchor_edge_batch.pt",
      .observation_artifact_index_path =
          dir / "replay_observation_artifacts.index",
  };
}

[[nodiscard]] inline runtime_replay_artifact_paths_t
default_runtime_replay_artifact_paths_for_dir(
    const std::filesystem::path &artifact_dir) {
  return {
      .graph_anchor_edge_batch_artifact_path =
          artifact_dir / "graph_anchor_edge_batch.pt",
      .observation_artifact_index_path =
          artifact_dir / "replay_observation_artifacts.index",
  };
}

[[nodiscard]] inline std::filesystem::path
default_runtime_replay_forecast_artifact_relative_path(
    const std::string &anchor_key, std::size_t index) {
  const auto sanitized_anchor =
      replay_runtime_detail::runtime::job_layout::sanitize_path_component(
          anchor_key.empty() ? std::string{"anchor"} : anchor_key);
  return std::filesystem::path("forecast") /
         (std::to_string(index) + "_" + sanitized_anchor + ".forecast.pt");
}

[[nodiscard]] inline std::filesystem::path
resolve_runtime_replay_path(const std::filesystem::path &path,
                            const std::filesystem::path &index_dir) {
  if (path.empty()) {
    return {};
  }
  return path.is_absolute() ? path : index_dir / path;
}

inline void validate_runtime_replay_artifact_paths(
    const runtime_replay_artifact_paths_t &paths) {
  if (paths.graph_anchor_edge_batch_artifact_path.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] graph-anchor batch artifact path is required");
  }
  if (cuwacunu::kikijyeba::environment::detail::path_contains_parent_reference(
          paths.graph_anchor_edge_batch_artifact_path)) {
    throw std::runtime_error(
        "[replay_runtime_source] graph-anchor batch artifact path must not "
        "contain parent-directory components");
  }
  if (!paths.observation_artifact_index_path.empty() &&
      cuwacunu::kikijyeba::environment::detail::path_contains_parent_reference(
          paths.observation_artifact_index_path)) {
    throw std::runtime_error(
        "[replay_runtime_source] observation artifact index path must not "
        "contain parent-directory components");
  }
}

inline void validate_runtime_replay_batch_index_entry(
    const runtime_replay_batch_index_entry_t &entry) {
  if (entry.anchor_count == 0 ||
      entry.end_anchor_index <= entry.begin_anchor_index) {
    throw std::runtime_error(
        "[replay_runtime_source] invalid replay batch index entry range");
  }
  if (entry.artifact_path_index_path.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] replay batch entry artifact index path is "
        "required");
  }
  if (cuwacunu::kikijyeba::environment::detail::path_contains_parent_reference(
          entry.artifact_path_index_path)) {
    throw std::runtime_error(
        "[replay_runtime_source] replay batch entry artifact index path must "
        "not contain parent-directory components");
  }
}

inline void write_runtime_replay_artifact_path_index_at(
    const std::filesystem::path &index_path,
    const runtime_replay_artifact_paths_t &paths) {
  if (index_path.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] runtime replay artifact path index path is "
        "required");
  }
  validate_runtime_replay_artifact_paths(paths);
  std::ostringstream out;
  out << "schema=" << kRuntimeReplayArtifactPathIndexSchema << "\n";
  out << "graph_anchor_edge_batch_artifact_path="
      << paths.graph_anchor_edge_batch_artifact_path.generic_string() << "\n";
  out << "observation_artifact_index_path="
      << paths.observation_artifact_index_path.generic_string() << "\n";
  replay_runtime_detail::runtime::job_layout::write_text_file_atomically(
      index_path, out.str());
}

inline void write_runtime_replay_artifact_path_index(
    const std::filesystem::path &job_dir,
    const runtime_replay_artifact_paths_t &paths) {
  if (job_dir.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] job_dir is required for runtime replay "
        "artifact path index");
  }
  write_runtime_replay_artifact_path_index_at(
      runtime_replay_artifact_path_index_path(job_dir), paths);
}

[[nodiscard]] inline runtime_replay_artifact_paths_t
read_runtime_replay_artifact_path_index_at(
    const std::filesystem::path &index_path) {
  namespace layout = replay_runtime_detail::runtime::job_layout;
  if (index_path.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] runtime replay artifact path index path is "
        "required");
  }
  const auto kv = layout::parse_kv_file(index_path);
  if (kv.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] unable to read runtime replay artifact path "
        "index: " +
        index_path.string());
  }
  if (layout::map_get(kv, "schema") != kRuntimeReplayArtifactPathIndexSchema) {
    throw std::runtime_error(
        "[replay_runtime_source] runtime replay artifact path index schema "
        "mismatch");
  }
  const auto index_dir = index_path.parent_path();
  runtime_replay_artifact_paths_t out{};
  out.graph_anchor_edge_batch_artifact_path = resolve_runtime_replay_path(
      layout::map_get(kv, "graph_anchor_edge_batch_artifact_path"), index_dir);
  out.observation_artifact_index_path = resolve_runtime_replay_path(
      layout::map_get(kv, "observation_artifact_index_path"), index_dir);
  validate_runtime_replay_artifact_paths(out);
  return out;
}

[[nodiscard]] inline runtime_replay_artifact_paths_t
read_runtime_replay_artifact_path_index(const std::filesystem::path &job_dir) {
  if (job_dir.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] job_dir is required for runtime replay "
        "artifact path index");
  }
  return read_runtime_replay_artifact_path_index_at(
      runtime_replay_artifact_path_index_path(job_dir));
}

inline void write_runtime_replay_batch_index(
    const std::filesystem::path &job_dir,
    const std::vector<runtime_replay_batch_index_entry_t> &entries) {
  if (job_dir.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] job_dir is required for replay batch index");
  }
  if (entries.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] replay batch index entries are required");
  }
  const auto index_path = runtime_replay_batch_index_path(job_dir);
  std::ostringstream out;
  out << "schema=" << kRuntimeReplayBatchIndexSchema << "\n";
  out << "entry_count=" << entries.size() << "\n";
  for (std::size_t i = 0; i < entries.size(); ++i) {
    const auto &entry = entries[i];
    validate_runtime_replay_batch_index_entry(entry);
    const auto prefix = std::string("entry_") + std::to_string(i) + "_";
    out << prefix << "wave_pulse_index=" << entry.wave_pulse_index << "\n";
    out << prefix << "begin_anchor_index=" << entry.begin_anchor_index << "\n";
    out << prefix << "end_anchor_index=" << entry.end_anchor_index << "\n";
    out << prefix << "anchor_count=" << entry.anchor_count << "\n";
    out << prefix << "batch_cursor_token=" << entry.batch_cursor_token << "\n";
    out << prefix << "artifact_path_index_path="
        << entry.artifact_path_index_path.generic_string() << "\n";
  }
  replay_runtime_detail::runtime::job_layout::write_text_file_atomically(
      index_path, out.str());
}

[[nodiscard]] inline std::vector<runtime_replay_batch_index_entry_t>
read_runtime_replay_batch_index(const std::filesystem::path &job_dir) {
  namespace layout = replay_runtime_detail::runtime::job_layout;
  if (job_dir.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] job_dir is required for replay batch index");
  }
  const auto index_path = runtime_replay_batch_index_path(job_dir);
  const auto kv = layout::parse_kv_file(index_path);
  if (kv.empty()) {
    return {};
  }
  const auto schema = layout::map_get(kv, "schema");
  if (schema != kRuntimeReplayBatchIndexSchema) {
    throw std::runtime_error(
        "[replay_runtime_source] runtime replay batch index schema mismatch");
  }
  const auto entry_count_text = layout::map_get(kv, "entry_count");
  if (entry_count_text.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] replay batch index missing entry_count");
  }
  const auto entry_count =
      static_cast<std::size_t>(std::stoull(entry_count_text));
  if (entry_count == 0) {
    throw std::runtime_error(
        "[replay_runtime_source] replay batch index entries are required");
  }
  std::vector<runtime_replay_batch_index_entry_t> entries;
  entries.reserve(entry_count);
  for (std::size_t i = 0; i < entry_count; ++i) {
    const auto prefix = std::string("entry_") + std::to_string(i) + "_";
    runtime_replay_batch_index_entry_t entry{};
    entry.wave_pulse_index = static_cast<std::size_t>(
        std::stoull(layout::map_get(kv, prefix + "wave_pulse_index")));
    entry.begin_anchor_index = static_cast<std::size_t>(
        std::stoull(layout::map_get(kv, prefix + "begin_anchor_index")));
    entry.end_anchor_index = static_cast<std::size_t>(
        std::stoull(layout::map_get(kv, prefix + "end_anchor_index")));
    entry.anchor_count = static_cast<std::size_t>(
        std::stoull(layout::map_get(kv, prefix + "anchor_count")));
    entry.batch_cursor_token =
        layout::map_get(kv, prefix + "batch_cursor_token");
    entry.artifact_path_index_path =
        layout::map_get(kv, prefix + "artifact_path_index_path");
    if (!entry.artifact_path_index_path.is_absolute()) {
      entry.artifact_path_index_path =
          index_path.parent_path() / entry.artifact_path_index_path;
    }
    validate_runtime_replay_batch_index_entry(entry);
    entries.push_back(std::move(entry));
  }
  return entries;
}

inline void append_runtime_replay_batch_index_entry(
    const std::filesystem::path &job_dir,
    runtime_replay_batch_index_entry_t entry) {
  auto entries = read_runtime_replay_batch_index(job_dir);
  entries.erase(std::remove_if(entries.begin(), entries.end(),
                               [&](const auto &candidate) {
                                 return candidate.wave_pulse_index ==
                                        entry.wave_pulse_index;
                               }),
                entries.end());
  entries.push_back(std::move(entry));
  std::sort(entries.begin(), entries.end(),
            [](const auto &lhs, const auto &rhs) {
              return lhs.wave_pulse_index < rhs.wave_pulse_index;
            });
  write_runtime_replay_batch_index(job_dir, entries);
}

template <typename KeyT>
[[nodiscard]] inline runtime_replay_artifact_paths_t
write_runtime_graph_anchor_replay_artifacts_to_paths(
    const std::filesystem::path &artifact_path_index_path,
    runtime_replay_artifact_paths_t paths,
    const replay_source_detail::dataloader::graph_anchor_edge_batch_t<KeyT>
        &batch,
    const std::vector<runtime_replay_forecast_artifact_record_t>
        &forecast_records) {
  static_assert(std::is_integral_v<KeyT>,
                "Graph-anchor replay artifact V1 requires integral keys");
  if (artifact_path_index_path.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] artifact path index is required for replay "
        "artifacts");
  }
  if (paths.graph_anchor_edge_batch_artifact_path.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] graph-anchor replay artifact path is "
        "required");
  }
  if (paths.observation_artifact_index_path.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] observation replay artifact index path is "
        "required");
  }
  if (forecast_records.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] forecast artifact records are required for "
        "replay artifacts");
  }
  validate_runtime_replay_artifact_paths(paths);

  const auto artifact_dir = artifact_path_index_path.parent_path();

  std::vector<replay_observation_artifact_index_entry_t> entries;
  entries.reserve(forecast_records.size());
  std::vector<std::filesystem::path> forecast_paths;
  forecast_paths.reserve(forecast_records.size());
  for (std::size_t i = 0; i < forecast_records.size(); ++i) {
    const auto &record = forecast_records[i];
    forecast::validate_forecast_artifact(record.forecast_artifact,
                                         /*require_scenarios=*/true);
    const auto &identity = record.forecast_artifact.identity;
    if (identity.anchor_key.empty()) {
      throw std::runtime_error(
          "[replay_runtime_source] forecast artifact anchor_key is required");
    }

    const auto relative_forecast_path =
        record.forecast_artifact_path.empty()
            ? default_runtime_replay_forecast_artifact_relative_path(
                  identity.anchor_key, i)
            : record.forecast_artifact_path;
    const auto forecast_path = relative_forecast_path.is_absolute()
                                   ? relative_forecast_path
                                   : artifact_dir / relative_forecast_path;
    replay_observation_artifact_index_entry_t entry{
        .anchor_key = identity.anchor_key,
        .observation_anchor_index = record.observation_anchor_index,
        .artifact_timestamp_ms = identity.timestamp_ms,
        .forecast_artifact_path = relative_forecast_path,
        .mdn_artifact = record.mdn_artifact,
    };
    validate_replay_observation_artifact_index_entry(entry);
    forecast_paths.push_back(forecast_path);
    entries.push_back(std::move(entry));
  }

  std::filesystem::create_directories(artifact_dir);
  save_graph_anchor_edge_batch_artifact(
      batch, paths.graph_anchor_edge_batch_artifact_path);
  for (std::size_t i = 0; i < forecast_records.size(); ++i) {
    forecast::save_forecast_artifact(forecast_records[i].forecast_artifact,
                                     forecast_paths[i]);
  }
  write_replay_observation_artifact_index(paths.observation_artifact_index_path,
                                          entries);
  write_runtime_replay_artifact_path_index_at(artifact_path_index_path, paths);
  return paths;
}

template <typename KeyT>
[[nodiscard]] runtime_replay_artifact_paths_t
write_runtime_graph_anchor_replay_artifacts(
    const std::filesystem::path &job_dir,
    const replay_source_detail::dataloader::graph_anchor_edge_batch_t<KeyT>
        &batch,
    const std::vector<runtime_replay_forecast_artifact_record_t>
        &forecast_records) {
  if (job_dir.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] job_dir is required for replay artifacts");
  }
  return write_runtime_graph_anchor_replay_artifacts_to_paths(
      runtime_replay_artifact_path_index_path(job_dir),
      default_runtime_replay_artifact_paths(job_dir), batch, forecast_records);
}

template <typename KeyT>
[[nodiscard]] runtime_replay_artifact_paths_t
write_runtime_graph_anchor_replay_artifacts_from_mdn_batch(
    const std::filesystem::path &job_dir,
    const replay_runtime_detail::mdn::MdnOut &out,
    const replay_runtime_detail::mdn_stream::channel_mdn_input_batch_t<KeyT>
        &mdn_batch,
    const cuwacunu::wikimyei::representation::encoding::vicreg::stream::
        channel_representation_batch_t<KeyT> &representation_batch,
    belief::allocation_belief_builder_options_t common_options,
    artifact_ref_t mdn_artifact = {}, std::string model_version = {},
    std::string target_coords_hash = {}, std::string normalization_hash = {}) {
  static_assert(std::is_integral_v<KeyT>,
                "Runtime replay MDN writer requires integral keys");
  replay_runtime_detail::require_same_if_present(
      mdn_batch.graph_order_fingerprint, "channel MDN graph fingerprint",
      representation_batch.graph_order_fingerprint,
      "representation graph fingerprint");
  if (!mdn_batch.node_ids.empty() && !representation_batch.node_ids.empty() &&
      mdn_batch.node_ids != representation_batch.node_ids) {
    throw std::runtime_error(
        "[replay_runtime_source] channel MDN node_ids do not match "
        "representation node_ids");
  }
  if (!mdn_batch.edge_ids.empty() && !representation_batch.edge_ids.empty() &&
      mdn_batch.edge_ids != representation_batch.edge_ids) {
    throw std::runtime_error(
        "[replay_runtime_source] channel MDN edge_ids do not match "
        "representation edge_ids");
  }
  if (!mdn_batch.cursor.empty() && !representation_batch.cursor.empty() &&
      mdn_batch.cursor.cursor_token() !=
          representation_batch.cursor.cursor_token()) {
    throw std::runtime_error(
        "[replay_runtime_source] channel MDN cursor does not match "
        "representation cursor");
  }

  auto edge_batch = make_runtime_replay_edge_batch_from_channel_representation(
      representation_batch);
  auto forecast_records = make_runtime_replay_forecast_records_from_mdn_batch(
      out, mdn_batch, std::move(common_options), std::move(mdn_artifact),
      std::move(model_version), std::move(target_coords_hash),
      std::move(normalization_hash));
  return write_runtime_graph_anchor_replay_artifacts(job_dir, edge_batch,
                                                     forecast_records);
}

template <typename KeyT>
[[nodiscard]] runtime_replay_artifact_paths_t
write_runtime_graph_anchor_replay_artifacts_from_mdn_batch_for_pulse(
    const std::filesystem::path &job_dir, std::size_t wave_pulse_index,
    const replay_runtime_detail::mdn::MdnOut &out,
    const replay_runtime_detail::mdn_stream::channel_mdn_input_batch_t<KeyT>
        &mdn_batch,
    const cuwacunu::wikimyei::representation::encoding::vicreg::stream::
        channel_representation_batch_t<KeyT> &representation_batch,
    belief::allocation_belief_builder_options_t common_options,
    artifact_ref_t mdn_artifact = {}, std::string model_version = {},
    std::string target_coords_hash = {}, std::string normalization_hash = {}) {
  auto edge_batch = make_runtime_replay_edge_batch_from_channel_representation(
      representation_batch);
  const auto cursor_token = edge_batch.cursor.cursor_token();
  const auto pulse_dir = runtime_replay_pulse_artifact_dir(
      job_dir, wave_pulse_index, cursor_token);
  const auto paths = default_runtime_replay_artifact_paths_for_dir(pulse_dir);
  auto forecast_records = make_runtime_replay_forecast_records_from_mdn_batch(
      out, mdn_batch, std::move(common_options), std::move(mdn_artifact),
      std::move(model_version), std::move(target_coords_hash),
      std::move(normalization_hash));
  const auto written_paths =
      write_runtime_graph_anchor_replay_artifacts_to_paths(
          pulse_dir / "runtime_replay_artifacts.index", paths, edge_batch,
          forecast_records);
  append_runtime_replay_batch_index_entry(
      job_dir, runtime_replay_batch_index_entry_t{
                   .wave_pulse_index = wave_pulse_index,
                   .begin_anchor_index = edge_batch.cursor.begin_anchor_index,
                   .end_anchor_index = edge_batch.cursor.end_anchor_index,
                   .anchor_count = edge_batch.cursor.anchor_count(),
                   .batch_cursor_token = cursor_token,
                   .artifact_path_index_path =
                       pulse_dir / "runtime_replay_artifacts.index",
               });
  return written_paths;
}

template <typename KeyT>
inline void save_graph_anchor_edge_batch_artifact(
    const replay_source_detail::dataloader::graph_anchor_edge_batch_t<KeyT>
        &batch,
    const std::filesystem::path &path) {
  static_assert(std::is_integral_v<KeyT>,
                "Graph-anchor batch artifact V1 requires integral keys");
  if (path.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] graph-anchor batch artifact path is required");
  }
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  torch::serialize::OutputArchive root;
  const auto i64 = torch::TensorOptions().dtype(torch::kInt64);
  replay_runtime_detail::write_string(root, "meta/schema",
                                      kGraphAnchorEdgeBatchArtifactSchema);
  replay_runtime_detail::write_string(root, "meta/graph_order_fingerprint",
                                      batch.graph_order_fingerprint);
  replay_runtime_detail::write_string_list(root, "meta/edge_ids",
                                           batch.edge_ids);
  root.write(
      "meta/cursor/begin_anchor_index",
      torch::tensor(
          {static_cast<std::int64_t>(batch.cursor.begin_anchor_index)}, i64));
  root.write(
      "meta/cursor/end_anchor_index",
      torch::tensor({static_cast<std::int64_t>(batch.cursor.end_anchor_index)},
                    i64));
  root.write(
      "meta/cursor/requested_batch_size",
      torch::tensor(
          {static_cast<std::int64_t>(batch.cursor.requested_batch_size)}, i64));
  root.write(
      "meta/cursor/anchor_keys",
      replay_runtime_detail::key_vector_to_tensor(batch.cursor.anchor_keys));
  root.write("meta/cursor/anchor_indices",
             replay_runtime_detail::size_vector_to_tensor(
                 batch.cursor.anchor_indices));

  root.write("tensor/edge_features", replay_runtime_detail::tensor_or_empty_cpu(
                                         batch.edge_features, torch::kFloat64));
  root.write("tensor/edge_mask", replay_runtime_detail::tensor_or_empty_cpu(
                                     batch.edge_mask, torch::kBool));
  root.write("tensor/future_features",
             replay_runtime_detail::tensor_or_empty_cpu(batch.future_features,
                                                        torch::kFloat64));
  root.write("tensor/future_mask", replay_runtime_detail::tensor_or_empty_cpu(
                                       batch.future_mask, torch::kBool));
  root.write("tensor/past_keys", replay_runtime_detail::tensor_or_empty_cpu(
                                     batch.past_keys, torch::kInt64));
  root.write("tensor/future_keys", replay_runtime_detail::tensor_or_empty_cpu(
                                       batch.future_keys, torch::kInt64));
  root.write("tensor/anchor_keys", replay_runtime_detail::tensor_or_empty_cpu(
                                       batch.anchor_keys, torch::kInt64));
  root.write("tensor/edge_present", replay_runtime_detail::tensor_or_empty_cpu(
                                        batch.edge_present, torch::kBool));
  root.save_to(path.string());
}

template <typename KeyT>
[[nodiscard]] replay_source_detail::dataloader::graph_anchor_edge_batch_t<KeyT>
load_graph_anchor_edge_batch_artifact(
    const std::filesystem::path &path,
    const torch::Device &device = torch::Device(torch::kCPU)) {
  static_assert(std::is_integral_v<KeyT>,
                "Graph-anchor batch artifact V1 requires integral keys");
  if (path.empty() || !std::filesystem::exists(path)) {
    throw std::runtime_error(
        "[replay_runtime_source] graph-anchor batch artifact missing: " +
        path.string());
  }

  torch::serialize::InputArchive root;
  root.load_from(path.string(), device);
  std::string schema{};
  std::string graph_order_fingerprint{};
  replay_runtime_detail::read_string(root, "meta/schema", &schema);
  if (schema != kGraphAnchorEdgeBatchArtifactSchema) {
    throw std::runtime_error(
        "[replay_runtime_source] graph-anchor batch artifact schema mismatch");
  }
  replay_runtime_detail::read_string(root, "meta/graph_order_fingerprint",
                                     &graph_order_fingerprint);
  std::vector<std::string> edge_ids;
  replay_runtime_detail::read_string_list(root, "meta/edge_ids", &edge_ids);

  torch::Tensor begin_anchor_index{};
  torch::Tensor end_anchor_index{};
  torch::Tensor requested_batch_size{};
  torch::Tensor cursor_anchor_keys{};
  torch::Tensor cursor_anchor_indices{};
  root.read("meta/cursor/begin_anchor_index", begin_anchor_index);
  root.read("meta/cursor/end_anchor_index", end_anchor_index);
  root.read("meta/cursor/requested_batch_size", requested_batch_size);
  root.read("meta/cursor/anchor_keys", cursor_anchor_keys);
  root.read("meta/cursor/anchor_indices", cursor_anchor_indices);

  replay_source_detail::dataloader::graph_anchor_edge_batch_t<KeyT> out{};
  out.graph_order_fingerprint = graph_order_fingerprint;
  out.edge_ids = std::move(edge_ids);
  out.cursor.graph_order_fingerprint = graph_order_fingerprint;
  out.cursor.begin_anchor_index =
      static_cast<std::size_t>(begin_anchor_index.to(torch::kCPU)
                                   .to(torch::kInt64)
                                   .item<std::int64_t>());
  out.cursor.end_anchor_index = static_cast<std::size_t>(
      end_anchor_index.to(torch::kCPU).to(torch::kInt64).item<std::int64_t>());
  out.cursor.requested_batch_size =
      static_cast<std::size_t>(requested_batch_size.to(torch::kCPU)
                                   .to(torch::kInt64)
                                   .item<std::int64_t>());
  out.cursor.anchor_keys = replay_runtime_detail::tensor_to_key_vector<KeyT>(
      cursor_anchor_keys, "cursor.anchor_keys");
  out.cursor.anchor_indices = replay_runtime_detail::tensor_to_size_vector(
      cursor_anchor_indices, "cursor.anchor_indices");

  root.read("tensor/edge_features", out.edge_features);
  root.read("tensor/edge_mask", out.edge_mask);
  root.read("tensor/future_features", out.future_features);
  root.read("tensor/future_mask", out.future_mask);
  root.read("tensor/past_keys", out.past_keys);
  root.read("tensor/future_keys", out.future_keys);
  root.read("tensor/anchor_keys", out.anchor_keys);
  root.read("tensor/edge_present", out.edge_present);
  if (out.future_features.defined() && out.future_features.numel() == 0) {
    out.future_features = torch::Tensor{};
  }
  if (out.future_mask.defined() && out.future_mask.numel() == 0) {
    out.future_mask = torch::Tensor{};
  }
  if (out.past_keys.defined() && out.past_keys.numel() == 0) {
    out.past_keys = torch::Tensor{};
  }
  if (out.future_keys.defined() && out.future_keys.numel() == 0) {
    out.future_keys = torch::Tensor{};
  }
  return out;
}

[[nodiscard]] inline replay_runtime_detail::runtime::job_manifest_t
job_manifest_from_runtime_kv(
    const std::unordered_map<std::string, std::string> &kv) {
  namespace detail = replay_runtime_detail;
  namespace layout = replay_runtime_detail::runtime::job_layout;
  replay_runtime_detail::runtime::job_manifest_t out{};
  out.job_id = layout::map_get(kv, "job_id");
  out.job_stable_id = layout::map_get(kv, "job_stable_id");
  out.job_attempt_id = layout::map_get(kv, "job_attempt_id");
  out.job_attempt_index = detail::parse_size_or(kv, "job_attempt_index");
  out.job_attempt_policy = layout::map_get(kv, "job_attempt_policy");
  out.job_kind = layout::map_get(kv, "job_kind");
  out.config_path = layout::map_get(kv, "config_path");
  out.config_bundle_id = layout::map_get(kv, "config_bundle_id");
  out.config_receipt_id = layout::map_get(kv, "config_receipt_id");
  out.component_spawn_registry_id =
      layout::map_get(kv, "component_spawn_registry_id");
  out.component_family_id = layout::map_get(kv, "component_family_id");
  out.component_spawn_fingerprint =
      layout::map_get(kv, "component_spawn_fingerprint");
  out.component_spawn_id = layout::map_get(kv, "component_spawn_id");
  out.component_spawn_label = layout::map_get(kv, "component_spawn_label");
  out.protocol_id = layout::map_get(kv, "protocol_id");
  out.protocol_kind = layout::map_get(kv, "protocol_kind");
  out.protocol_status = layout::map_get(kv, "protocol_status");
  out.wave_id = layout::map_get(kv, "wave_id");
  out.target_component_family_id =
      layout::map_get(kv, "target_component_family_id");
  out.wave_action = layout::map_get(kv, "wave_action");
  out.wave_mode = layout::map_get(kv, "wave_mode");
  out.source_range_policy = layout::map_get(kv, "source_range_policy");
  out.source_order_policy = layout::map_get(kv, "source_order_policy");
  out.requested_source_key_begin =
      layout::map_get(kv, "requested_source_key_begin");
  out.requested_source_key_end =
      layout::map_get(kv, "requested_source_key_end");
  out.resolved_anchor_index_begin =
      detail::parse_size_or(kv, "resolved_anchor_index_begin");
  out.resolved_anchor_index_end =
      detail::parse_size_or(kv, "resolved_anchor_index_end");
  out.accepted_anchor_count =
      detail::parse_size_or(kv, "accepted_anchor_count");
  out.candidate_anchor_count =
      detail::parse_size_or(kv, "candidate_anchor_count");
  out.accepted_anchor_fraction =
      detail::parse_double_or(kv, "accepted_anchor_fraction");
  out.skipped_outside_common_range =
      detail::parse_size_or(kv, "skipped_outside_common_range");
  out.skipped_missing_edge_coverage =
      detail::parse_size_or(kv, "skipped_missing_edge_coverage");
  out.skipped_failed_fetch_probe =
      detail::parse_size_or(kv, "skipped_failed_fetch_probe");
  out.duplicate_anchor_count =
      detail::parse_size_or(kv, "duplicate_anchor_count");
  out.anchor_domain_warning_level =
      layout::map_get(kv, "anchor_domain_warning_level");
  out.anchor_domain_warnings = layout::map_get(kv, "anchor_domain_warnings");
  out.source_cursor_token = layout::map_get(kv, "source_cursor_token");
  out.source_input_length = detail::parse_size_or(kv, "source_input_length");
  out.source_future_length = detail::parse_size_or(kv, "source_future_length");
  out.observed_source_row_begin =
      detail::parse_i64_or(kv, "observed_source_row_begin");
  out.observed_source_row_end =
      detail::parse_i64_or(kv, "observed_source_row_end");
  out.target_source_row_begin =
      detail::parse_i64_or(kv, "target_source_row_begin");
  out.target_source_row_end = detail::parse_i64_or(kv, "target_source_row_end");
  out.source_footprint_precision =
      layout::map_get(kv, "source_footprint_precision");
  out.first_anchor_key = layout::map_get(kv, "first_anchor_key");
  out.last_anchor_key = layout::map_get(kv, "last_anchor_key");
  out.observed_source_key_begin =
      layout::map_get(kv, "observed_source_key_begin");
  out.observed_source_key_end = layout::map_get(kv, "observed_source_key_end");
  out.target_source_key_begin = layout::map_get(kv, "target_source_key_begin");
  out.target_source_key_end = layout::map_get(kv, "target_source_key_end");
  out.source_key_footprint_precision =
      layout::map_get(kv, "source_key_footprint_precision");
  out.protocol_contract_fingerprint =
      layout::map_get(kv, "protocol_contract_fingerprint");
  out.protocol_contract_token = layout::map_get(kv, "protocol_contract_token");
  out.graph_order_fingerprint = layout::map_get(kv, "graph_order_fingerprint");
  out.node_ids = detail::split_csv(layout::map_get(kv, "node_ids"));
  out.edge_ids = detail::split_csv(layout::map_get(kv, "edge_ids"));
  out.nodelift_assembly_fingerprint =
      layout::map_get(kv, "nodelift_assembly_fingerprint");
  out.vicreg_assembly_fingerprint =
      layout::map_get(kv, "vicreg_assembly_fingerprint");
  out.mtf_jepa_mae_vicreg_assembly_fingerprint =
      layout::map_get(kv, "mtf_jepa_mae_vicreg_assembly_fingerprint");
  out.mdn_assembly_fingerprint =
      layout::map_get(kv, "mdn_assembly_fingerprint");
  out.dock_binding_fingerprint =
      layout::map_get(kv, "dock_binding_fingerprint");
  out.dock_binding_token = layout::map_get(kv, "dock_binding_token");
  out.stream_plan = layout::map_get(kv, "stream_plan");
  out.representation_training_id =
      layout::map_get(kv, "representation_training_id");
  out.inference_training_id = layout::map_get(kv, "inference_training_id");
  out.input_representation_checkpoint_path =
      layout::map_get(kv, "input_representation_checkpoint_path");
  out.input_mdn_checkpoint_path =
      layout::map_get(kv, "input_mdn_checkpoint_path");
  out.runtime_handoff_id = layout::map_get(kv, "runtime_handoff_id");
  out.runtime_handoff_digest = layout::map_get(kv, "runtime_handoff_digest");
  out.marshal_target_driver_run_id =
      layout::map_get(kv, "marshal_target_driver_run_id");
  return out;
}

[[nodiscard]] inline replay_runtime_detail::runtime::wave_plan_t
wave_plan_from_runtime_job_manifest(
    const replay_runtime_detail::runtime::job_manifest_t &manifest,
    const runtime_replay_job_evidence_read_options_t &options = {}) {
  if (replay_runtime_detail::blank(options.source_cursor_kind) ||
      replay_runtime_detail::blank(options.source_cursor_scope)) {
    throw std::runtime_error(
        "[replay_runtime_source] source cursor kind/scope options are "
        "required");
  }
  replay_runtime_detail::runtime::wave_plan_t out{};
  out.wave_id = manifest.wave_id;
  out.target_component_family_id = manifest.target_component_family_id;
  out.action = manifest.wave_action;
  out.mode_text = manifest.wave_mode;
  out.source_cursor_kind = options.source_cursor_kind;
  out.source_cursor_scope = options.source_cursor_scope;
  out.source_range_policy = manifest.source_range_policy;
  out.source_order_policy = manifest.source_order_policy;
  out.requested_source_key_begin = replay_runtime_detail::parse_optional_i64(
      manifest.requested_source_key_begin);
  out.requested_source_key_end = replay_runtime_detail::parse_optional_i64(
      manifest.requested_source_key_end);
  const bool source_key_request = out.source_range_policy == "source_key" &&
                                  out.requested_source_key_begin.has_value() &&
                                  out.requested_source_key_end.has_value();
  if (options.use_resolved_range_as_requested_anchor_range &&
      !source_key_request) {
    out.requested_anchor_index_begin = manifest.resolved_anchor_index_begin;
    out.requested_anchor_index_end = manifest.resolved_anchor_index_end;
  }
  out.resolved_anchor_index_begin = manifest.resolved_anchor_index_begin;
  out.resolved_anchor_index_end = manifest.resolved_anchor_index_end;
  out.accepted_anchor_count = manifest.accepted_anchor_count;
  out.candidate_anchor_count = manifest.candidate_anchor_count;
  out.accepted_anchor_fraction = manifest.accepted_anchor_fraction;
  out.skipped_outside_common_range = manifest.skipped_outside_common_range;
  out.skipped_missing_edge_coverage = manifest.skipped_missing_edge_coverage;
  out.skipped_failed_fetch_probe = manifest.skipped_failed_fetch_probe;
  out.duplicate_anchor_count = manifest.duplicate_anchor_count;
  out.anchor_domain_warning_level = manifest.anchor_domain_warning_level;
  out.anchor_domain_warnings = manifest.anchor_domain_warnings;
  out.source_cursor_token = manifest.source_cursor_token;
  out.first_anchor_key = manifest.first_anchor_key;
  out.last_anchor_key = manifest.last_anchor_key;
  out.common_left_key = manifest.common_left_key;
  out.common_right_key = manifest.common_right_key;
  out.reference_left_key = manifest.reference_left_key;
  out.reference_right_key = manifest.reference_right_key;
  out.source_input_length = manifest.source_input_length;
  out.source_future_length = manifest.source_future_length;
  out.observed_source_row_begin = manifest.observed_source_row_begin;
  out.observed_source_row_end = manifest.observed_source_row_end;
  out.target_source_row_begin = manifest.target_source_row_begin;
  out.target_source_row_end = manifest.target_source_row_end;
  out.source_footprint_precision = manifest.source_footprint_precision;
  out.observed_source_key_begin = manifest.observed_source_key_begin;
  out.observed_source_key_end = manifest.observed_source_key_end;
  out.target_source_key_begin = manifest.target_source_key_begin;
  out.target_source_key_end = manifest.target_source_key_end;
  out.source_key_footprint_precision = manifest.source_key_footprint_precision;
  return out;
}

[[nodiscard]] inline runtime_replay_job_evidence_t
read_runtime_replay_job_evidence(
    const std::filesystem::path &job_dir,
    const runtime_replay_job_evidence_read_options_t &options = {}) {
  namespace layout = replay_runtime_detail::runtime::job_layout;
  const auto manifest_path = job_dir / "job.manifest";
  auto kv = layout::parse_kv_file(manifest_path);
  if (kv.empty()) {
    throw std::runtime_error(
        "[replay_runtime_source] unable to read job.manifest: " +
        manifest_path.string());
  }
  runtime_replay_job_evidence_t out{};
  out.job_dir = job_dir;
  out.manifest_path = manifest_path;
  out.manifest = job_manifest_from_runtime_kv(kv);
  out.wave_plan = wave_plan_from_runtime_job_manifest(out.manifest, options);
  return out;
}

[[nodiscard]] inline protocol::component_stream_wave_t
component_stream_wave_from_runtime_wave_plan(
    const replay_runtime_detail::runtime::wave_plan_t &plan) {
  if (replay_runtime_detail::blank(plan.wave_id) ||
      replay_runtime_detail::blank(plan.mode_text) ||
      replay_runtime_detail::blank(plan.source_cursor_kind) ||
      replay_runtime_detail::blank(plan.source_cursor_scope) ||
      replay_runtime_detail::blank(plan.source_range_policy) ||
      replay_runtime_detail::blank(plan.source_order_policy)) {
    throw std::runtime_error(
        "[replay_runtime_source] wave_plan lacks component-stream fields");
  }
  return protocol::component_stream_wave_t{
      .wave_id = plan.wave_id,
      .mode_text = plan.mode_text,
      .source_cursor_kind = plan.source_cursor_kind,
      .source_cursor_scope = plan.source_cursor_scope,
      .source_range_policy = plan.source_range_policy,
      .source_order_policy = plan.source_order_policy,
      .anchor_index_begin = plan.requested_anchor_index_begin,
      .anchor_index_end = plan.requested_anchor_index_end,
      .source_key_begin = plan.requested_source_key_begin,
      .source_key_end = plan.requested_source_key_end,
  };
}

template <typename KeyT>
[[nodiscard]] replay_runtime_detail::runtime::wave_plan_t
wave_plan_for_batch_cursor(
    replay_runtime_detail::runtime::wave_plan_t plan,
    const replay_source_detail::dataloader::graph_anchor_cursor_t<KeyT>
        &cursor) {
  const auto [accepted_begin, accepted_end] =
      replay_source_detail::accepted_anchor_interval(cursor);
  const bool has_requested_anchor_range =
      plan.requested_anchor_index_begin.has_value() ||
      plan.requested_anchor_index_end.has_value();
  const bool has_requested_source_key_range =
      plan.requested_source_key_begin.has_value() ||
      plan.requested_source_key_end.has_value();
  if (!has_requested_anchor_range && !has_requested_source_key_range) {
    plan.requested_anchor_index_begin =
        static_cast<std::size_t>(accepted_begin);
    plan.requested_anchor_index_end = static_cast<std::size_t>(accepted_end);
  }
  plan.resolved_anchor_index_begin = static_cast<std::size_t>(accepted_begin);
  plan.resolved_anchor_index_end = static_cast<std::size_t>(accepted_end);
  plan.accepted_anchor_count = cursor.anchor_count();
  if (cursor.first_anchor_key().has_value()) {
    plan.first_anchor_key =
        replay_source_detail::key_to_string(*cursor.first_anchor_key());
  }
  if (cursor.last_anchor_key().has_value()) {
    plan.last_anchor_key =
        replay_source_detail::key_to_string(*cursor.last_anchor_key());
  }
  plan.source_cursor_token = cursor.cursor_token();
  return plan;
}

template <typename KeyT>
inline void validate_runtime_graph_anchor_replay_record(
    const runtime_graph_anchor_replay_record_t<KeyT> &record,
    const replay_source_detail::market_graph_t &graph) {
  static_assert(std::is_integral_v<KeyT>,
                "Runtime replay source V1 requires integral keys");
  const auto &manifest = record.manifest;
  const auto &plan = record.wave_plan;

  if (replay_runtime_detail::blank(manifest.job_id) ||
      replay_runtime_detail::blank(manifest.protocol_id) ||
      replay_runtime_detail::blank(manifest.graph_order_fingerprint)) {
    throw std::runtime_error(
        "[replay_runtime_source] manifest job_id, protocol_id, and graph "
        "fingerprint are required");
  }
  if (manifest.accepted_anchor_count == 0) {
    throw std::runtime_error(
        "[replay_runtime_source] manifest accepted_anchor_count is zero");
  }
  if (replay_runtime_detail::blank(plan.wave_id) ||
      replay_runtime_detail::blank(plan.source_cursor_kind)) {
    throw std::runtime_error(
        "[replay_runtime_source] wave_plan wave_id and source cursor kind are "
        "required");
  }
  if (plan.source_cursor_kind != "graph_anchor") {
    throw std::runtime_error(
        "[replay_runtime_source] V1 requires graph_anchor source cursor kind");
  }

  graph.validate();
  const auto graph_fingerprint = graph.computed_graph_order_fingerprint();
  if (manifest.graph_order_fingerprint != graph_fingerprint ||
      record.cursor.graph_order_fingerprint != graph_fingerprint ||
      (!record.batch.graph_order_fingerprint.empty() &&
       record.batch.graph_order_fingerprint != graph_fingerprint)) {
    throw std::runtime_error(
        "[replay_runtime_source] graph fingerprint evidence mismatch: graph=" +
        graph_fingerprint + "|manifest=" + manifest.graph_order_fingerprint +
        "|cursor=" + record.cursor.graph_order_fingerprint +
        "|batch=" + record.batch.graph_order_fingerprint);
  }
  if (!manifest.node_ids.empty() && manifest.node_ids != graph.node_ids) {
    throw std::runtime_error(
        "[replay_runtime_source] manifest node_ids do not match graph");
  }
  if (!manifest.edge_ids.empty() && manifest.edge_ids != graph.edge_ids) {
    throw std::runtime_error(
        "[replay_runtime_source] manifest edge_ids do not match graph");
  }

  const auto [accepted_begin, accepted_end] =
      replay_source_detail::accepted_anchor_interval(record.cursor);
  if (static_cast<std::size_t>(accepted_begin) !=
          plan.resolved_anchor_index_begin ||
      static_cast<std::size_t>(accepted_end) !=
          plan.resolved_anchor_index_end) {
    throw std::runtime_error(
        "[replay_runtime_source] wave_plan resolved range does not match "
        "cursor");
  }
  if (plan.resolved_anchor_index_begin < manifest.resolved_anchor_index_begin ||
      plan.resolved_anchor_index_end > manifest.resolved_anchor_index_end ||
      plan.accepted_anchor_count > manifest.accepted_anchor_count) {
    throw std::runtime_error(
        "[replay_runtime_source] replay batch range is outside manifest wave "
        "range");
  }
  if (plan.accepted_anchor_count != record.cursor.anchor_count()) {
    throw std::runtime_error(
        "[replay_runtime_source] wave_plan accepted_anchor_count does not "
        "match cursor");
  }
  replay_runtime_detail::require_key_match(
      plan.first_anchor_key, "wave_plan.first_anchor_key",
      record.cursor.first_anchor_key(), "cursor.first_anchor_key");
  replay_runtime_detail::require_key_match(
      plan.last_anchor_key, "wave_plan.last_anchor_key",
      record.cursor.last_anchor_key(), "cursor.last_anchor_key");
  if (plan.resolved_anchor_index_begin ==
      manifest.resolved_anchor_index_begin) {
    replay_runtime_detail::require_key_match(
        manifest.first_anchor_key, "manifest.first_anchor_key",
        record.cursor.first_anchor_key(), "cursor.first_anchor_key");
  }
  if (plan.resolved_anchor_index_end == manifest.resolved_anchor_index_end) {
    replay_runtime_detail::require_key_match(
        manifest.last_anchor_key, "manifest.last_anchor_key",
        record.cursor.last_anchor_key(), "cursor.last_anchor_key");
  }
  replay_runtime_detail::require_same_if_present(
      manifest.wave_id, "manifest.wave_id", plan.wave_id, "wave_plan.wave_id");
  replay_runtime_detail::require_same_if_present(
      manifest.wave_mode, "manifest.wave_mode", plan.mode_text,
      "wave_plan.mode_text");

  if (record.require_observation_artifacts &&
      record.observation_artifacts.size() != record.cursor.anchor_count()) {
    throw std::runtime_error(
        "[replay_runtime_source] required observation artifact count does not "
        "match cursor");
  }
}

template <typename KeyT>
[[nodiscard]] episode_spec_t make_episode_spec_from_runtime_replay_record(
    const runtime_graph_anchor_replay_record_t<KeyT> &record) {
  auto spec = record.base_spec;
  const auto &manifest = record.manifest;
  if (spec.episode_id.empty()) {
    const auto [accepted_begin, accepted_end] =
        replay_source_detail::accepted_anchor_interval(record.cursor);
    spec.episode_id = manifest.job_id + ".episode." +
                      std::to_string(accepted_begin) + "_" +
                      std::to_string(accepted_end);
  }
  if (spec.protocol_id.empty()) {
    spec.protocol_id = manifest.protocol_id;
  } else if (spec.protocol_id != manifest.protocol_id) {
    throw std::runtime_error(
        "[replay_runtime_source] EpisodeSpec protocol_id does not match "
        "manifest");
  }
  if (spec.runtime_run_id.empty()) {
    spec.runtime_run_id = manifest.job_id;
  } else if (spec.runtime_run_id != manifest.job_id) {
    throw std::runtime_error(
        "[replay_runtime_source] EpisodeSpec runtime_run_id must match "
        "manifest job_id");
  }
  if (spec.environment_run_id.empty()) {
    spec.environment_run_id = manifest.job_id + ".environment_replay";
  }
  if (spec.graph_order_fingerprint.empty()) {
    spec.graph_order_fingerprint = manifest.graph_order_fingerprint;
  } else if (spec.graph_order_fingerprint != manifest.graph_order_fingerprint) {
    throw std::runtime_error(
        "[replay_runtime_source] EpisodeSpec graph fingerprint does not match "
        "manifest");
  }
  if (spec.graph_node_ids.empty()) {
    spec.graph_node_ids = manifest.node_ids;
  } else if (!manifest.node_ids.empty() &&
             spec.graph_node_ids != manifest.node_ids) {
    throw std::runtime_error(
        "[replay_runtime_source] EpisodeSpec graph_node_ids do not match "
        "manifest");
  }
  spec.world_mode = world_mode_t::historical_replay;
  return spec;
}

template <typename KeyT>
[[nodiscard]] runtime_graph_anchor_replay_record_t<KeyT>
make_runtime_graph_anchor_replay_record_from_job_evidence(
    const runtime_replay_job_evidence_t &evidence, episode_spec_t base_spec,
    replay_source_detail::dataloader::graph_anchor_cursor_t<KeyT> cursor,
    replay_source_detail::dataloader::graph_anchor_edge_batch_t<KeyT> batch,
    replay_frame_build_options_t frame_options = {},
    replay_world_options_t world_options = {},
    std::vector<replay_observation_artifacts_t> observation_artifacts = {},
    replay_observation_artifact_options_t artifact_options = {},
    bool require_observation_artifacts = false) {
  runtime_graph_anchor_replay_record_t<KeyT> out{};
  out.manifest = evidence.manifest;
  out.wave_plan = wave_plan_for_batch_cursor(evidence.wave_plan, cursor);
  out.base_spec = std::move(base_spec);
  out.cursor = std::move(cursor);
  out.batch = std::move(batch);
  out.frame_options = frame_options;
  out.world_options = world_options;
  out.observation_artifacts = std::move(observation_artifacts);
  out.artifact_options = artifact_options;
  out.require_observation_artifacts = require_observation_artifacts;
  return out;
}

template <typename KeyT>
[[nodiscard]] runtime_graph_anchor_replay_record_t<KeyT>
make_runtime_graph_anchor_replay_record_from_artifact_paths(
    const runtime_replay_job_evidence_t &evidence, episode_spec_t base_spec,
    const std::filesystem::path &graph_anchor_edge_batch_artifact_path,
    const std::filesystem::path &observation_artifact_index_path,
    replay_frame_build_options_t frame_options = {},
    replay_world_options_t world_options = {},
    replay_observation_artifact_options_t artifact_options = {},
    bool require_observation_artifacts = true,
    const torch::Device &device = torch::Device(torch::kCPU)) {
  auto batch = load_graph_anchor_edge_batch_artifact<KeyT>(
      graph_anchor_edge_batch_artifact_path, device);
  auto cursor = batch.cursor;
  auto artifacts = observation_artifact_index_path.empty()
                       ? std::vector<replay_observation_artifacts_t>{}
                       : load_replay_observation_artifacts_from_index(
                             observation_artifact_index_path, device);
  return make_runtime_graph_anchor_replay_record_from_job_evidence(
      evidence, std::move(base_spec), std::move(cursor), std::move(batch),
      frame_options, world_options, std::move(artifacts), artifact_options,
      require_observation_artifacts);
}

template <typename KeyT>
[[nodiscard]] runtime_graph_anchor_replay_record_t<KeyT>
make_runtime_graph_anchor_replay_record_from_job_dir(
    const std::filesystem::path &job_dir, episode_spec_t base_spec,
    replay_frame_build_options_t frame_options = {},
    replay_world_options_t world_options = {},
    replay_observation_artifact_options_t artifact_options = {},
    bool require_observation_artifacts = true,
    const torch::Device &device = torch::Device(torch::kCPU)) {
  const auto evidence = read_runtime_replay_job_evidence(job_dir);
  const auto paths = read_runtime_replay_artifact_path_index(job_dir);
  return make_runtime_graph_anchor_replay_record_from_artifact_paths<KeyT>(
      evidence, std::move(base_spec),
      paths.graph_anchor_edge_batch_artifact_path,
      paths.observation_artifact_index_path, frame_options, world_options,
      artifact_options, require_observation_artifacts, device);
}

template <typename KeyT>
[[nodiscard]] std::vector<runtime_graph_anchor_replay_record_t<KeyT>>
make_runtime_graph_anchor_replay_records_from_job_dir(
    const std::filesystem::path &job_dir, episode_spec_t base_spec,
    replay_frame_build_options_t frame_options = {},
    replay_world_options_t world_options = {},
    replay_observation_artifact_options_t artifact_options = {},
    bool require_observation_artifacts = true,
    const torch::Device &device = torch::Device(torch::kCPU)) {
  const auto evidence = read_runtime_replay_job_evidence(job_dir);
  auto batch_entries = read_runtime_replay_batch_index(job_dir);
  if (batch_entries.empty()) {
    return {make_runtime_graph_anchor_replay_record_from_job_dir<KeyT>(
        job_dir, std::move(base_spec), frame_options, world_options,
        artifact_options, require_observation_artifacts, device)};
  }

  std::vector<runtime_graph_anchor_replay_record_t<KeyT>> records;
  records.reserve(batch_entries.size());
  for (const auto &entry : batch_entries) {
    const auto paths = read_runtime_replay_artifact_path_index_at(
        entry.artifact_path_index_path);
    auto record =
        make_runtime_graph_anchor_replay_record_from_artifact_paths<KeyT>(
            evidence, base_spec, paths.graph_anchor_edge_batch_artifact_path,
            paths.observation_artifact_index_path, frame_options, world_options,
            artifact_options, require_observation_artifacts, device);
    if (record.cursor.begin_anchor_index != entry.begin_anchor_index ||
        record.cursor.end_anchor_index != entry.end_anchor_index ||
        record.cursor.anchor_count() != entry.anchor_count) {
      throw std::runtime_error(
          "[replay_runtime_source] replay batch index entry does not match "
          "loaded cursor");
    }
    records.push_back(std::move(record));
  }
  return records;
}

template <typename KeyT>
[[nodiscard]] replay_episode_bundle_t
make_replay_episode_bundle_from_runtime_graph_anchor_record(
    const runtime_graph_anchor_replay_record_t<KeyT> &record,
    const replay_source_detail::market_graph_t &graph) {
  validate_runtime_graph_anchor_replay_record(record, graph);
  auto spec = make_episode_spec_from_runtime_replay_record(record);
  auto wave = component_stream_wave_from_runtime_wave_plan(record.wave_plan);
  auto bundle = make_replay_episode_bundle_from_graph_anchor_edge_batch(
      std::move(spec), wave, record.cursor, record.batch, graph,
      record.frame_options, record.world_options);

  if (!record.observation_artifacts.empty() ||
      record.require_observation_artifacts) {
    auto artifact_options = record.artifact_options;
    artifact_options.require_artifacts_per_frame =
        artifact_options.require_artifacts_per_frame ||
        record.require_observation_artifacts;
    auto artifact_source_id =
        record.artifact_source_id.empty()
            ? record.manifest.job_id + ".observation_artifacts"
            : record.artifact_source_id;
    keyed_replay_observation_artifact_source_t artifact_source(
        std::move(artifact_source_id), record.observation_artifacts);
    enrich_replay_episode_bundle_with_artifacts(bundle, artifact_source,
                                                artifact_options);
  }
  validate_replay_episode_bundle(bundle);
  return bundle;
}

template <typename KeyT>
class runtime_graph_anchor_replay_bundle_source_t final
    : public replay_bundle_source_iface_t {
public:
  runtime_graph_anchor_replay_bundle_source_t(
      std::string source_id, replay_source_detail::market_graph_t graph,
      std::vector<runtime_graph_anchor_replay_record_t<KeyT>> records)
      : source_id_(std::move(source_id)), graph_(std::move(graph)),
        records_(std::move(records)) {
    if (cuwacunu::kikijyeba::environment::detail::blank(source_id_)) {
      throw std::runtime_error(
          "[runtime_graph_anchor_replay_bundle_source] source_id is required");
    }
    graph_.validate();
    if (records_.empty()) {
      throw std::runtime_error(
          "[runtime_graph_anchor_replay_bundle_source] records are required");
    }
  }

  [[nodiscard]] std::string source_id() const override { return source_id_; }

  [[nodiscard]] std::optional<replay_episode_bundle_t> next_bundle() override {
    if (cursor_ >= records_.size()) {
      return std::nullopt;
    }
    const auto &record = records_[cursor_++];
    auto bundle = make_replay_episode_bundle_from_runtime_graph_anchor_record(
        record, graph_);
    validate_replay_episode_bundle(bundle);
    return bundle;
  }

  void reset() override { cursor_ = 0; }

  [[nodiscard]] std::size_t size() const { return records_.size(); }
  [[nodiscard]] std::size_t cursor() const { return cursor_; }

private:
  std::string source_id_{};
  replay_source_detail::market_graph_t graph_{};
  std::vector<runtime_graph_anchor_replay_record_t<KeyT>> records_{};
  std::size_t cursor_{0};
};

template <typename KeyT>
[[nodiscard]] runtime_graph_anchor_replay_bundle_source_t<KeyT>
make_runtime_graph_anchor_replay_bundle_source_from_job_dir(
    std::string source_id, const std::filesystem::path &job_dir,
    replay_source_detail::market_graph_t graph, episode_spec_t base_spec,
    replay_frame_build_options_t frame_options = {},
    replay_world_options_t world_options = {},
    replay_observation_artifact_options_t artifact_options = {},
    bool require_observation_artifacts = true,
    const torch::Device &device = torch::Device(torch::kCPU)) {
  if (cuwacunu::kikijyeba::environment::detail::blank(source_id)) {
    const auto evidence = read_runtime_replay_job_evidence(job_dir);
    source_id = evidence.manifest.job_id + ".runtime_replay_bundle_source";
  }
  auto records = make_runtime_graph_anchor_replay_records_from_job_dir<KeyT>(
      job_dir, std::move(base_spec), frame_options, world_options,
      artifact_options, require_observation_artifacts, device);
  return runtime_graph_anchor_replay_bundle_source_t<KeyT>(
      std::move(source_id), std::move(graph), std::move(records));
}

} // namespace cuwacunu::kikijyeba::environment::replay
