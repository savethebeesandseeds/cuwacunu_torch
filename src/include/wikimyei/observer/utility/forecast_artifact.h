// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/observer/belief/types.h"
#include "wikimyei/observer/utility/nodelift_return_projection.h"
#include "wikimyei/observer/utility/scenario_bank.h"

namespace cuwacunu::wikimyei::observer {

struct forecast_artifact_identity_t {
  std::int64_t schema_version{1};
  std::string artifact_family{"wikimyei.observer.forecast_artifact.v1"};

  belief::anchor_key_t anchor_key{};
  belief::timestamp_ms_t timestamp_ms{0};
  int horizon{1};

  std::string graph_order_fingerprint{};
  belief::GraphNodeAxisBinding graph_node_axis{};
  std::string source_feature_semantics_id{};
  std::string source_feature_semantics_fingerprint{};
  std::string node_ids_fingerprint{};
  std::string target_coords_hash{};
  std::string normalization_hash{};
  std::string model_version{};

  std::vector<belief::node_id_t> graph_node_ids{};
  std::vector<belief::node_id_t> node_ids{};
  std::vector<std::int64_t> node_graph_indices{};

  belief::BasePolicy base_policy{};
  std::int64_t projection_reference_graph_index{-1};
  std::string return_origin{"base_relative_nodelift_projection"};
};

struct marginal_forecast_artifact_t {
  forecast_artifact_identity_t identity{};

  torch::Tensor log_weight{};                 // [A,Kc], log-return mixture
  torch::Tensor mu{};                         // [A,Kc], log-return mixture
  torch::Tensor sigma{};                      // [A,Kc], log-return mixture
  torch::Tensor active_mask{};                // [A], bool
  torch::Tensor expected_log_return{};        // [A]
  torch::Tensor expected_arithmetic_return{}; // [A]

  torch::Tensor scenarios{};   // optional [S,A], arithmetic returns
  torch::Tensor covariance{};  // optional [A,A]
  torch::Tensor correlation{}; // optional [A,A]
  torch::Tensor confidence{};  // optional [A]

  torch::Tensor high_correlation_scenarios{};    // optional [S,A]
  torch::Tensor volatility_inflated_scenarios{}; // optional [S,A]
  torch::Tensor left_tail_shifted_scenarios{};   // optional [S,A]
  torch::Tensor tail_thickened_scenarios{};      // optional [S,A]
};

namespace detail {

[[nodiscard]] inline torch::Tensor
int64_tensor_from_vector(const std::vector<std::int64_t> &values) {
  const auto options = torch::TensorOptions().dtype(torch::kInt64);
  if (values.empty()) {
    return torch::empty({0}, options);
  }
  return torch::tensor(values, options);
}

[[nodiscard]] inline std::vector<std::int64_t>
string_to_bytes(const std::string &value) {
  std::vector<std::int64_t> bytes;
  bytes.reserve(value.size());
  for (const unsigned char ch : value) {
    bytes.push_back(static_cast<std::int64_t>(ch));
  }
  return bytes;
}

[[nodiscard]] inline std::string
bytes_to_string(const std::vector<std::int64_t> &bytes) {
  std::string out;
  out.reserve(bytes.size());
  for (const auto byte : bytes) {
    if (byte < 0 || byte > 255) {
      throw std::runtime_error(
          "[forecast_artifact] invalid encoded string byte");
    }
    out.push_back(static_cast<char>(byte));
  }
  return out;
}

[[nodiscard]] inline std::pair<std::vector<std::int64_t>,
                               std::vector<std::int64_t>>
string_list_to_lengths_and_bytes(const std::vector<std::string> &values) {
  std::vector<std::int64_t> lengths;
  std::vector<std::int64_t> bytes;
  lengths.reserve(values.size());
  for (const auto &value : values) {
    lengths.push_back(static_cast<std::int64_t>(value.size()));
    for (const unsigned char ch : value) {
      bytes.push_back(static_cast<std::int64_t>(ch));
    }
  }
  return {std::move(lengths), std::move(bytes)};
}

[[nodiscard]] inline std::vector<std::string>
string_list_from_tensors(const torch::Tensor &lengths_tensor,
                         const torch::Tensor &bytes_tensor) {
  auto lengths_cpu =
      lengths_tensor.to(torch::kCPU).to(torch::kInt64).contiguous().view({-1});
  auto bytes_cpu =
      bytes_tensor.to(torch::kCPU).to(torch::kInt64).contiguous().view({-1});
  auto lengths = lengths_cpu.accessor<std::int64_t, 1>();
  auto bytes = bytes_cpu.accessor<std::int64_t, 1>();

  std::vector<std::string> out;
  out.reserve(lengths_cpu.numel());
  std::int64_t cursor = 0;
  for (std::int64_t i = 0; i < lengths_cpu.numel(); ++i) {
    const auto length = lengths[i];
    if (length < 0 || cursor + length > bytes_cpu.numel()) {
      throw std::runtime_error(
          "[forecast_artifact] invalid encoded string-list length");
    }
    std::vector<std::int64_t> slice;
    slice.reserve(static_cast<std::size_t>(length));
    for (std::int64_t j = 0; j < length; ++j) {
      slice.push_back(bytes[cursor + j]);
    }
    out.push_back(bytes_to_string(slice));
    cursor += length;
  }
  if (cursor != bytes_cpu.numel()) {
    throw std::runtime_error(
        "[forecast_artifact] unused bytes in encoded string list");
  }
  return out;
}

[[nodiscard]] inline std::vector<std::int64_t>
tensor_to_int64_vector(const torch::Tensor &tensor) {
  if (!tensor.defined()) {
    return {};
  }
  auto cpu = tensor.to(torch::kCPU).to(torch::kInt64).contiguous().view({-1});
  std::vector<std::int64_t> out;
  out.reserve(static_cast<std::size_t>(cpu.numel()));
  auto accessor = cpu.accessor<std::int64_t, 1>();
  for (std::int64_t i = 0; i < cpu.numel(); ++i) {
    out.push_back(accessor[i]);
  }
  return out;
}

[[nodiscard]] inline std::int64_t scalar_i64(const torch::Tensor &tensor,
                                             const char *name) {
  TORCH_CHECK(tensor.defined() && tensor.numel() == 1, "[forecast_artifact] ",
              name, " must be scalar");
  return tensor.to(torch::kCPU).to(torch::kInt64).item<std::int64_t>();
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

inline void write_string(torch::serialize::OutputArchive &root,
                         const std::string &key, const std::string &value) {
  root.write(key, int64_tensor_from_vector(string_to_bytes(value)));
}

inline void read_string(torch::serialize::InputArchive &root,
                        const std::string &key, std::string *out) {
  torch::Tensor bytes{};
  root.read(key, bytes);
  *out = bytes_to_string(tensor_to_int64_vector(bytes));
}

inline void write_string_list(torch::serialize::OutputArchive &root,
                              const std::string &key_prefix,
                              const std::vector<std::string> &values) {
  auto [lengths, bytes] = string_list_to_lengths_and_bytes(values);
  root.write(key_prefix + "/lengths", int64_tensor_from_vector(lengths));
  root.write(key_prefix + "/bytes", int64_tensor_from_vector(bytes));
}

inline void read_string_list(torch::serialize::InputArchive &root,
                             const std::string &key_prefix,
                             std::vector<std::string> *out) {
  torch::Tensor lengths{};
  torch::Tensor bytes{};
  root.read(key_prefix + "/lengths", lengths);
  root.read(key_prefix + "/bytes", bytes);
  *out = string_list_from_tensors(lengths, bytes);
}

inline void require_vector_shape(const torch::Tensor &tensor, std::int64_t A,
                                 const char *name, bool required) {
  if (!tensor.defined() || tensor.numel() == 0) {
    TORCH_CHECK(!required, "[forecast_artifact] missing ", name);
    return;
  }
  TORCH_CHECK(tensor.dim() == 1 && tensor.size(0) == A, "[forecast_artifact] ",
              name, " must be [A]");
}

inline void require_matrix_shape(const torch::Tensor &tensor, std::int64_t A,
                                 const char *name, bool required) {
  if (!tensor.defined() || tensor.numel() == 0) {
    TORCH_CHECK(!required, "[forecast_artifact] missing ", name);
    return;
  }
  TORCH_CHECK(tensor.dim() == 2 && tensor.size(0) == A && tensor.size(1) == A,
              "[forecast_artifact] ", name, " must be [A,A]");
}

inline void require_scenario_shape(const torch::Tensor &tensor, std::int64_t A,
                                   const char *name, bool required) {
  if (!tensor.defined() || tensor.numel() == 0) {
    TORCH_CHECK(!required, "[forecast_artifact] missing ", name);
    return;
  }
  TORCH_CHECK(tensor.dim() == 2 && tensor.size(1) == A, "[forecast_artifact] ",
              name, " must be [S,A]");
  TORCH_CHECK(torch::isfinite(tensor).all().item<bool>(),
              "[forecast_artifact] ", name, " contains non-finite values");
}

} // namespace detail

[[nodiscard]] inline std::string
node_ids_fingerprint(const std::vector<belief::node_id_t> &node_ids) {
  std::string out = std::to_string(node_ids.size());
  for (const auto &id : node_ids) {
    out += "|" + std::to_string(id.size()) + ":" + id;
  }
  return out;
}

[[nodiscard]] inline std::int64_t
asset_count(const marginal_forecast_artifact_t &artifact) {
  return static_cast<std::int64_t>(artifact.identity.node_ids.size());
}

inline void
validate_forecast_artifact(const marginal_forecast_artifact_t &artifact,
                           bool require_scenarios = false) {
  const auto &id = artifact.identity;
  TORCH_CHECK(id.schema_version == 1,
              "[forecast_artifact] unsupported schema_version");
  TORCH_CHECK(id.artifact_family == "wikimyei.observer.forecast_artifact.v1",
              "[forecast_artifact] unsupported artifact_family");
  TORCH_CHECK(id.horizon == 1, "[forecast_artifact] V1 requires horizon=1");
  TORCH_CHECK(!id.anchor_key.empty(),
              "[forecast_artifact] anchor_key required");
  TORCH_CHECK(id.return_origin == "base_relative_nodelift_projection",
              "[forecast_artifact] unsupported return_origin");
  const auto graph_axis = belief::detail::effective_graph_node_axis_binding(
      id.graph_node_axis, id.graph_order_fingerprint, id.graph_node_ids,
      "[forecast_artifact]");
  TORCH_CHECK(!id.source_feature_semantics_id.empty() &&
                  !id.source_feature_semantics_fingerprint.empty(),
              "[forecast_artifact] source feature semantics required");
  TORCH_CHECK(!id.base_policy.accounting_numeraire_id.empty() &&
                  !id.base_policy.settlement_asset_id.empty() &&
                  !id.base_policy.reserve_asset_id.empty() &&
                  !id.base_policy.projection_reference_node_id.empty(),
              "[forecast_artifact] BasePolicy fields required");
  TORCH_CHECK(id.node_graph_indices.size() == id.node_ids.size(),
              "[forecast_artifact] node_graph_indices size mismatch");
  TORCH_CHECK(id.node_ids_fingerprint.empty() ||
                  id.node_ids_fingerprint == node_ids_fingerprint(id.node_ids),
              "[forecast_artifact] node_ids_fingerprint mismatch");
  for (std::size_t i = 0; i < id.node_ids.size(); ++i) {
    TORCH_CHECK(id.node_ids[i] != id.base_policy.reserve_asset_id &&
                    id.node_ids[i] !=
                        id.base_policy.projection_reference_node_id,
                "[forecast_artifact] base/reserve node appears in risky "
                "node_ids");
    TORCH_CHECK(id.node_graph_indices[i] >= 0 &&
                    id.node_graph_indices[i] <
                        static_cast<std::int64_t>(graph_axis.node_ids.size()),
                "[forecast_artifact] node_graph_indices out of range");
  }
  if (id.projection_reference_graph_index >= 0) {
    TORCH_CHECK(id.projection_reference_graph_index <
                    static_cast<std::int64_t>(graph_axis.node_ids.size()),
                "[forecast_artifact] projection_reference_graph_index out of "
                "range");
    TORCH_CHECK(graph_axis.node_ids[static_cast<std::size_t>(
                    id.projection_reference_graph_index)] ==
                    id.base_policy.projection_reference_node_id,
                "[forecast_artifact] projection_reference_graph_index does not "
                "match BasePolicy projection_reference_node_id");
  }

  const auto A = asset_count(artifact);
  TORCH_CHECK(artifact.log_weight.defined() && artifact.log_weight.dim() == 2,
              "[forecast_artifact] log_weight must be [A,Kc]");
  TORCH_CHECK(artifact.log_weight.size(0) == A,
              "[forecast_artifact] log_weight asset axis mismatch");
  TORCH_CHECK(artifact.mu.sizes() == artifact.log_weight.sizes() &&
                  artifact.sigma.sizes() == artifact.log_weight.sizes(),
              "[forecast_artifact] mixture tensor shape mismatch");
  detail::require_vector_shape(artifact.active_mask, A, "active_mask", true);
  TORCH_CHECK(artifact.active_mask.scalar_type() == torch::kBool,
              "[forecast_artifact] active_mask must be bool");
  detail::require_vector_shape(artifact.expected_log_return, A,
                               "expected_log_return", true);
  detail::require_vector_shape(artifact.expected_arithmetic_return, A,
                               "expected_arithmetic_return", true);
  detail::require_vector_shape(artifact.confidence, A, "confidence", false);
  detail::require_matrix_shape(artifact.covariance, A, "covariance", false);
  detail::require_matrix_shape(artifact.correlation, A, "correlation", false);
  detail::require_scenario_shape(artifact.scenarios, A, "scenarios",
                                 require_scenarios);
  detail::require_scenario_shape(artifact.high_correlation_scenarios, A,
                                 "high_correlation_scenarios", false);
  detail::require_scenario_shape(artifact.volatility_inflated_scenarios, A,
                                 "volatility_inflated_scenarios", false);
  detail::require_scenario_shape(artifact.left_tail_shifted_scenarios, A,
                                 "left_tail_shifted_scenarios", false);
  detail::require_scenario_shape(artifact.tail_thickened_scenarios, A,
                                 "tail_thickened_scenarios", false);
  if (artifact.scenarios.defined() && artifact.scenarios.numel() > 0) {
    auto require_same_scenario_shape = [&](const torch::Tensor &tensor,
                                           const char *name) {
      if (tensor.defined() && tensor.numel() > 0) {
        TORCH_CHECK(tensor.sizes() == artifact.scenarios.sizes(),
                    "[forecast_artifact] ", name,
                    " must match scenarios shape");
      }
    };
    require_same_scenario_shape(artifact.high_correlation_scenarios,
                                "high_correlation_scenarios");
    require_same_scenario_shape(artifact.volatility_inflated_scenarios,
                                "volatility_inflated_scenarios");
    require_same_scenario_shape(artifact.left_tail_shifted_scenarios,
                                "left_tail_shifted_scenarios");
    require_same_scenario_shape(artifact.tail_thickened_scenarios,
                                "tail_thickened_scenarios");
  }

  TORCH_CHECK(torch::isfinite(artifact.log_weight).all().item<bool>(),
              "[forecast_artifact] log_weight contains non-finite values");
  TORCH_CHECK(torch::isfinite(artifact.mu).all().item<bool>(),
              "[forecast_artifact] mu contains non-finite values");
  TORCH_CHECK(torch::isfinite(artifact.sigma).all().item<bool>(),
              "[forecast_artifact] sigma contains non-finite values");
  auto active = artifact.active_mask.to(torch::kBool);
  if (active.any().item<bool>()) {
    TORCH_CHECK(artifact.sigma.index({active}).gt(0.0).all().item<bool>(),
                "[forecast_artifact] active sigma must be positive");
  }
  if (active.any().item<bool>()) {
    auto weights = artifact.log_weight.exp().sum(/*dim=*/1);
    auto err = (weights.index({active}) - 1.0).abs().max().item<double>();
    TORCH_CHECK(
        err < 1.0e-5,
        "[forecast_artifact] active mixture weights are not normalized");
  }
}

[[nodiscard]] inline marginal_forecast_artifact_t
make_from_allocation_belief_and_projection(
    const belief::AllocationBelief &allocation_belief,
    const nodelift_return_projection_t &projection,
    std::string model_version = {}, std::string target_coords_hash = {},
    std::string normalization_hash = {},
    const scenario_bank_t *stress_bank = nullptr) {
  belief::validate_allocation_belief_contract(
      allocation_belief,
      /*require_portfolio_fields=*/true);
  TORCH_CHECK(projection.projected_log_weight.defined() &&
                  projection.projected_log_weight.dim() == 2,
              "[forecast_artifact] projection mixture must be [A,Kp]");
  TORCH_CHECK(projection.projected_log_return_mu.sizes() ==
                      projection.projected_log_weight.sizes() &&
                  projection.projected_log_return_sigma.sizes() ==
                      projection.projected_log_weight.sizes(),
              "[forecast_artifact] projection mixture shape mismatch");

  marginal_forecast_artifact_t artifact{};
  artifact.identity.anchor_key = allocation_belief.anchor_key;
  artifact.identity.timestamp_ms = allocation_belief.timestamp_ms;
  artifact.identity.horizon = allocation_belief.horizon;
  artifact.identity.graph_order_fingerprint =
      allocation_belief.graph_order_fingerprint;
  artifact.identity.graph_node_axis =
      belief::detail::effective_graph_node_axis_binding(
          allocation_belief.graph_node_axis,
          allocation_belief.graph_order_fingerprint,
          allocation_belief.graph_node_ids, "[forecast_artifact]");
  artifact.identity.source_feature_semantics_id =
      allocation_belief.source_feature_semantics_id;
  artifact.identity.source_feature_semantics_fingerprint =
      allocation_belief.source_feature_semantics_fingerprint;
  artifact.identity.node_ids = allocation_belief.node_ids;
  artifact.identity.node_ids_fingerprint =
      node_ids_fingerprint(allocation_belief.node_ids);
  artifact.identity.graph_node_ids = allocation_belief.graph_node_ids;
  artifact.identity.node_graph_indices = allocation_belief.node_graph_indices;
  artifact.identity.base_policy = allocation_belief.base_policy;
  artifact.identity.projection_reference_graph_index =
      allocation_belief.projection_reference_graph_index.value_or(-1);
  artifact.identity.return_origin = "base_relative_nodelift_projection";
  artifact.identity.model_version = std::move(model_version);
  artifact.identity.target_coords_hash = std::move(target_coords_hash);
  artifact.identity.normalization_hash = std::move(normalization_hash);

  artifact.log_weight = detail::tensor_or_empty_cpu(
      projection.projected_log_weight, torch::kFloat64);
  artifact.mu = detail::tensor_or_empty_cpu(projection.projected_log_return_mu,
                                            torch::kFloat64);
  artifact.sigma = detail::tensor_or_empty_cpu(
      projection.projected_log_return_sigma, torch::kFloat64);
  artifact.active_mask =
      detail::tensor_or_empty_cpu(projection.active_mask, torch::kBool);
  artifact.expected_log_return = detail::tensor_or_empty_cpu(
      allocation_belief.expected_log_return, torch::kFloat64);
  artifact.expected_arithmetic_return = detail::tensor_or_empty_cpu(
      allocation_belief.expected_arithmetic_return, torch::kFloat64);
  artifact.scenarios =
      detail::tensor_or_empty_cpu(allocation_belief.scenarios, torch::kFloat64);
  artifact.covariance = detail::tensor_or_empty_cpu(
      allocation_belief.covariance, torch::kFloat64);
  artifact.correlation = detail::tensor_or_empty_cpu(
      allocation_belief.correlation, torch::kFloat64);
  artifact.confidence = detail::tensor_or_empty_cpu(
      allocation_belief.confidence, torch::kFloat64);
  if (stress_bank != nullptr) {
    validate_scenario_bank(*stress_bank);
    artifact.high_correlation_scenarios = detail::tensor_or_empty_cpu(
        stress_bank->high_correlation_scenarios, torch::kFloat64);
    artifact.volatility_inflated_scenarios = detail::tensor_or_empty_cpu(
        stress_bank->volatility_inflated_scenarios, torch::kFloat64);
    artifact.left_tail_shifted_scenarios = detail::tensor_or_empty_cpu(
        stress_bank->left_tail_shifted_scenarios, torch::kFloat64);
    artifact.tail_thickened_scenarios = detail::tensor_or_empty_cpu(
        stress_bank->tail_thickened_scenarios, torch::kFloat64);
  }

  validate_forecast_artifact(artifact);
  return artifact;
}

inline void save_forecast_artifact(const marginal_forecast_artifact_t &artifact,
                                   const std::filesystem::path &path) {
  TORCH_CHECK(!path.empty(), "[forecast_artifact] path must not be empty");
  validate_forecast_artifact(artifact);
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }

  torch::serialize::OutputArchive root;
  const auto i64 = torch::TensorOptions().dtype(torch::kInt64);
  const auto graph_axis = belief::detail::effective_graph_node_axis_binding(
      artifact.identity.graph_node_axis,
      artifact.identity.graph_order_fingerprint,
      artifact.identity.graph_node_ids, "[forecast_artifact]");
  root.write("meta/schema_version",
             torch::tensor({artifact.identity.schema_version}, i64));
  detail::write_string(root, "meta/artifact_family_bytes",
                       artifact.identity.artifact_family);
  detail::write_string(root, "meta/anchor_key_bytes",
                       artifact.identity.anchor_key);
  root.write("meta/timestamp_ms",
             torch::tensor({artifact.identity.timestamp_ms}, i64));
  root.write("meta/horizon", torch::tensor({artifact.identity.horizon}, i64));
  detail::write_string(root, "meta/graph_order_fingerprint_bytes",
                       artifact.identity.graph_order_fingerprint);
  detail::write_string(root, "meta/graph_node_axis/dock_name_bytes",
                       graph_axis.dock_name);
  detail::write_string(root, "meta/graph_node_axis/axis_name_bytes",
                       graph_axis.axis_name);
  detail::write_string(root, "meta/graph_node_axis/coordinate_space_bytes",
                       graph_axis.coordinate_space);
  detail::write_string(root, "meta/source_feature_semantics_id_bytes",
                       artifact.identity.source_feature_semantics_id);
  detail::write_string(root, "meta/source_feature_semantics_fingerprint_bytes",
                       artifact.identity.source_feature_semantics_fingerprint);
  detail::write_string(root, "meta/node_ids_fingerprint_bytes",
                       artifact.identity.node_ids_fingerprint);
  detail::write_string(root, "meta/target_coords_hash_bytes",
                       artifact.identity.target_coords_hash);
  detail::write_string(root, "meta/normalization_hash_bytes",
                       artifact.identity.normalization_hash);
  detail::write_string(root, "meta/model_version_bytes",
                       artifact.identity.model_version);
  detail::write_string(root, "meta/return_origin_bytes",
                       artifact.identity.return_origin);
  detail::write_string(root, "meta/base_policy/accounting_numeraire_id_bytes",
                       artifact.identity.base_policy.accounting_numeraire_id);
  detail::write_string(root, "meta/base_policy/settlement_asset_id_bytes",
                       artifact.identity.base_policy.settlement_asset_id);
  detail::write_string(root, "meta/base_policy/reserve_asset_id_bytes",
                       artifact.identity.base_policy.reserve_asset_id);
  detail::write_string(
      root, "meta/base_policy/projection_reference_node_id_bytes",
      artifact.identity.base_policy.projection_reference_node_id);
  root.write(
      "meta/projection_reference_graph_index",
      torch::tensor({artifact.identity.projection_reference_graph_index}, i64));
  root.write(
      "meta/node_graph_indices",
      detail::int64_tensor_from_vector(artifact.identity.node_graph_indices));
  detail::write_string_list(root, "meta/graph_node_ids",
                            artifact.identity.graph_node_ids);
  detail::write_string_list(root, "meta/node_ids", artifact.identity.node_ids);

  root.write("marginal/log_weight",
             detail::tensor_or_empty_cpu(artifact.log_weight, torch::kFloat64));
  root.write("marginal/mu",
             detail::tensor_or_empty_cpu(artifact.mu, torch::kFloat64));
  root.write("marginal/sigma",
             detail::tensor_or_empty_cpu(artifact.sigma, torch::kFloat64));
  root.write("marginal/active_mask",
             detail::tensor_or_empty_cpu(artifact.active_mask, torch::kBool));
  root.write("marginal/expected_log_return",
             detail::tensor_or_empty_cpu(artifact.expected_log_return,
                                         torch::kFloat64));
  root.write("marginal/expected_arithmetic_return",
             detail::tensor_or_empty_cpu(artifact.expected_arithmetic_return,
                                         torch::kFloat64));
  root.write("belief/scenarios",
             detail::tensor_or_empty_cpu(artifact.scenarios, torch::kFloat64));
  root.write("belief/covariance",
             detail::tensor_or_empty_cpu(artifact.covariance, torch::kFloat64));
  root.write("belief/correlation", detail::tensor_or_empty_cpu(
                                       artifact.correlation, torch::kFloat64));
  root.write("belief/confidence",
             detail::tensor_or_empty_cpu(artifact.confidence, torch::kFloat64));
  root.write("scenario_bank/high_correlation",
             detail::tensor_or_empty_cpu(artifact.high_correlation_scenarios,
                                         torch::kFloat64));
  root.write("scenario_bank/volatility_inflated",
             detail::tensor_or_empty_cpu(artifact.volatility_inflated_scenarios,
                                         torch::kFloat64));
  root.write("scenario_bank/left_tail_shifted",
             detail::tensor_or_empty_cpu(artifact.left_tail_shifted_scenarios,
                                         torch::kFloat64));
  root.write("scenario_bank/tail_thickened",
             detail::tensor_or_empty_cpu(artifact.tail_thickened_scenarios,
                                         torch::kFloat64));
  root.save_to(path.string());
}

[[nodiscard]] inline marginal_forecast_artifact_t load_forecast_artifact(
    const std::filesystem::path &path,
    const torch::Device &device = torch::Device(torch::kCPU)) {
  TORCH_CHECK(!path.empty(), "[forecast_artifact] path must not be empty");
  TORCH_CHECK(std::filesystem::exists(path),
              "[forecast_artifact] path does not exist: ", path.string());

  torch::serialize::InputArchive root;
  root.load_from(path.string(), device);

  marginal_forecast_artifact_t artifact{};
  torch::Tensor schema_version{};
  torch::Tensor timestamp_ms{};
  torch::Tensor horizon{};
  torch::Tensor projection_reference_graph_index{};
  torch::Tensor node_graph_indices{};
  root.read("meta/schema_version", schema_version);
  artifact.identity.schema_version =
      detail::scalar_i64(schema_version, "schema_version");
  detail::read_string(root, "meta/artifact_family_bytes",
                      &artifact.identity.artifact_family);
  detail::read_string(root, "meta/anchor_key_bytes",
                      &artifact.identity.anchor_key);
  root.read("meta/timestamp_ms", timestamp_ms);
  artifact.identity.timestamp_ms =
      detail::scalar_i64(timestamp_ms, "timestamp_ms");
  root.read("meta/horizon", horizon);
  artifact.identity.horizon =
      static_cast<int>(detail::scalar_i64(horizon, "horizon"));
  detail::read_string(root, "meta/graph_order_fingerprint_bytes",
                      &artifact.identity.graph_order_fingerprint);
  detail::read_string(root, "meta/graph_node_axis/dock_name_bytes",
                      &artifact.identity.graph_node_axis.dock_name);
  detail::read_string(root, "meta/graph_node_axis/axis_name_bytes",
                      &artifact.identity.graph_node_axis.axis_name);
  detail::read_string(root, "meta/graph_node_axis/coordinate_space_bytes",
                      &artifact.identity.graph_node_axis.coordinate_space);
  detail::read_string(root, "meta/source_feature_semantics_id_bytes",
                      &artifact.identity.source_feature_semantics_id);
  detail::read_string(root, "meta/source_feature_semantics_fingerprint_bytes",
                      &artifact.identity.source_feature_semantics_fingerprint);
  detail::read_string(root, "meta/node_ids_fingerprint_bytes",
                      &artifact.identity.node_ids_fingerprint);
  detail::read_string(root, "meta/target_coords_hash_bytes",
                      &artifact.identity.target_coords_hash);
  detail::read_string(root, "meta/normalization_hash_bytes",
                      &artifact.identity.normalization_hash);
  detail::read_string(root, "meta/model_version_bytes",
                      &artifact.identity.model_version);
  detail::read_string(root, "meta/return_origin_bytes",
                      &artifact.identity.return_origin);
  detail::read_string(root, "meta/base_policy/accounting_numeraire_id_bytes",
                      &artifact.identity.base_policy.accounting_numeraire_id);
  detail::read_string(root, "meta/base_policy/settlement_asset_id_bytes",
                      &artifact.identity.base_policy.settlement_asset_id);
  detail::read_string(root, "meta/base_policy/reserve_asset_id_bytes",
                      &artifact.identity.base_policy.reserve_asset_id);
  detail::read_string(
      root, "meta/base_policy/projection_reference_node_id_bytes",
      &artifact.identity.base_policy.projection_reference_node_id);
  root.read("meta/projection_reference_graph_index",
            projection_reference_graph_index);
  artifact.identity.projection_reference_graph_index = detail::scalar_i64(
      projection_reference_graph_index, "projection_reference_graph_index");
  root.read("meta/node_graph_indices", node_graph_indices);
  artifact.identity.node_graph_indices =
      detail::tensor_to_int64_vector(node_graph_indices);
  detail::read_string_list(root, "meta/graph_node_ids",
                           &artifact.identity.graph_node_ids);
  detail::read_string_list(root, "meta/node_ids", &artifact.identity.node_ids);
  artifact.identity.graph_node_axis.graph_order_fingerprint =
      artifact.identity.graph_order_fingerprint;
  artifact.identity.graph_node_axis.node_ids = artifact.identity.graph_node_ids;

  root.read("marginal/log_weight", artifact.log_weight);
  root.read("marginal/mu", artifact.mu);
  root.read("marginal/sigma", artifact.sigma);
  root.read("marginal/active_mask", artifact.active_mask);
  root.read("marginal/expected_log_return", artifact.expected_log_return);
  root.read("marginal/expected_arithmetic_return",
            artifact.expected_arithmetic_return);
  root.read("belief/scenarios", artifact.scenarios);
  root.read("belief/covariance", artifact.covariance);
  root.read("belief/correlation", artifact.correlation);
  root.read("belief/confidence", artifact.confidence);
  root.read("scenario_bank/high_correlation",
            artifact.high_correlation_scenarios);
  root.read("scenario_bank/volatility_inflated",
            artifact.volatility_inflated_scenarios);
  root.read("scenario_bank/left_tail_shifted",
            artifact.left_tail_shifted_scenarios);
  root.read("scenario_bank/tail_thickened", artifact.tail_thickened_scenarios);

  validate_forecast_artifact(artifact);
  return artifact;
}

} // namespace cuwacunu::wikimyei::observer
