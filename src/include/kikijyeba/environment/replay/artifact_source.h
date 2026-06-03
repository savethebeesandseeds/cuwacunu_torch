// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/environment/replay/source.h"
#include "wikimyei/observer/utility/forecast_artifact.h"

namespace cuwacunu::kikijyeba::environment::replay {

namespace forecast = cuwacunu::wikimyei::observer;

struct replay_observation_artifacts_t {
  std::string anchor_key{};
  std::optional<std::int64_t> observation_anchor_index{std::nullopt};
  portfolio::timestamp_ms_t artifact_timestamp_ms{0};

  std::optional<artifact_ref_t> mdn_artifact{};
  std::optional<belief::NodeLiftPotentialBelief> nodelift_potential_belief{};
  std::optional<belief::AllocationBelief> allocation_belief{};
  std::optional<portfolio::MarketState> market_state{};
  std::optional<execution::spot_edge_market_state_t> edge_market_state{};

  torch::Tensor projected_log_return_scenarios{}; // optional [S,A]
  torch::Tensor active_projection_mask{};         // optional [A], bool
  torch::Tensor nodelift_residual_energy{};       // optional [...,Df]
  torch::Tensor nodelift_residual_mask{};         // optional same shape, bool
};

struct replay_observation_artifact_options_t {
  bool require_artifacts_per_frame{false};
  bool require_anchor_key_match{true};
  bool require_anchor_index_match{true};
  bool require_artifact_timestamp{true};
  bool require_artifact_time_not_after_observation{true};
  bool validate_allocation_belief{true};
  bool validate_market_state{true};
  bool validate_edge_market_state{true};
  bool require_allocation_belief_anchor_match{true};
  bool require_allocation_belief_node_match{true};
  bool require_allocation_belief_base_policy_match{true};
};

inline constexpr const char *kReplayObservationArtifactIndexSchema =
    "kikijyeba.environment.replay.observation_artifact_index.v1";

struct replay_observation_artifact_index_entry_t {
  std::string anchor_key{};
  std::optional<std::int64_t> observation_anchor_index{std::nullopt};
  portfolio::timestamp_ms_t artifact_timestamp_ms{0};

  std::filesystem::path forecast_artifact_path{};

  artifact_ref_t mdn_artifact{};
};

namespace replay_artifact_detail {

[[nodiscard]] inline bool
has_payload(const replay_observation_artifacts_t &artifacts) {
  return artifacts.mdn_artifact.has_value() ||
         artifacts.nodelift_potential_belief.has_value() ||
         artifacts.allocation_belief.has_value() ||
         artifacts.market_state.has_value() ||
         artifacts.edge_market_state.has_value() ||
         artifacts.projected_log_return_scenarios.defined() ||
         artifacts.active_projection_mask.defined() ||
         artifacts.nodelift_residual_energy.defined() ||
         artifacts.nodelift_residual_mask.defined();
}

inline void
validate_artifact_time(const replay_observation_artifacts_t &artifacts,
                       const observation_t &observation,
                       const replay_observation_artifact_options_t &options) {
  if (options.require_artifact_timestamp &&
      artifacts.artifact_timestamp_ms <= 0) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact_timestamp_ms is required");
  }
  if (artifacts.artifact_timestamp_ms > 0 &&
      options.require_artifact_time_not_after_observation &&
      artifacts.artifact_timestamp_ms > observation.knowledge_timestamp_ms) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact timestamp is after observation "
        "knowledge timestamp");
  }
}

inline void
validate_projection_tensors(const replay_observation_artifacts_t &artifacts,
                            std::int64_t A) {
  if (artifacts.projected_log_return_scenarios.defined()) {
    TORCH_CHECK(artifacts.projected_log_return_scenarios.dim() == 2 &&
                    artifacts.projected_log_return_scenarios.size(1) == A,
                "[replay_artifact_source] projected_log_return_scenarios must "
                "be [S,A]");
    TORCH_CHECK(artifacts.projected_log_return_scenarios.is_floating_point(),
                "[replay_artifact_source] projected_log_return_scenarios must "
                "be floating point");
    TORCH_CHECK(artifacts.projected_log_return_scenarios.size(0) > 0,
                "[replay_artifact_source] projected_log_return_scenarios must "
                "have at least one scenario");
    auto scenarios =
        artifacts.projected_log_return_scenarios.to(torch::kFloat64);
    TORCH_CHECK(torch::isfinite(scenarios).all().item<bool>(),
                "[replay_artifact_source] projected_log_return_scenarios "
                "contain non-finite values");
  }
  if (artifacts.active_projection_mask.defined()) {
    TORCH_CHECK(artifacts.projected_log_return_scenarios.defined(),
                "[replay_artifact_source] active_projection_mask requires "
                "projected_log_return_scenarios");
    TORCH_CHECK(artifacts.active_projection_mask.dim() == 1 &&
                    artifacts.active_projection_mask.size(0) == A,
                "[replay_artifact_source] active_projection_mask must be [A]");
    TORCH_CHECK(artifacts.active_projection_mask.scalar_type() == torch::kBool,
                "[replay_artifact_source] active_projection_mask must be bool");
    TORCH_CHECK(artifacts.active_projection_mask.any().item<bool>(),
                "[replay_artifact_source] active_projection_mask must contain "
                "at least one active asset");
  }
  if (artifacts.nodelift_residual_energy.defined()) {
    TORCH_CHECK(artifacts.nodelift_residual_energy.dim() >= 1 &&
                    artifacts.nodelift_residual_energy.numel() > 0,
                "[replay_artifact_source] nodelift_residual_energy must be "
                "nonempty");
    TORCH_CHECK(artifacts.nodelift_residual_energy.is_floating_point(),
                "[replay_artifact_source] nodelift_residual_energy must be "
                "floating point");
    auto energy = artifacts.nodelift_residual_energy.to(torch::kFloat64);
    TORCH_CHECK(torch::isfinite(energy).all().item<bool>(),
                "[replay_artifact_source] nodelift_residual_energy contains "
                "non-finite values");
  }
  if (artifacts.nodelift_residual_mask.defined()) {
    TORCH_CHECK(artifacts.nodelift_residual_energy.defined(),
                "[replay_artifact_source] nodelift_residual_mask requires "
                "nodelift_residual_energy");
    TORCH_CHECK(artifacts.nodelift_residual_mask.sizes() ==
                    artifacts.nodelift_residual_energy.sizes(),
                "[replay_artifact_source] nodelift_residual_mask shape must "
                "match nodelift_residual_energy");
    TORCH_CHECK(artifacts.nodelift_residual_mask.scalar_type() == torch::kBool,
                "[replay_artifact_source] nodelift_residual_mask must be bool");
  }
}

inline void validate_allocation_belief_for_frame(
    const belief::AllocationBelief &belief_state, const replay_frame_t &frame,
    const episode_spec_t &spec,
    const replay_observation_artifact_options_t &options) {
  if (options.validate_allocation_belief) {
    belief::validate_allocation_belief_contract(belief_state);
  }
  if (options.require_allocation_belief_anchor_match &&
      belief_state.anchor_key != frame.observation.anchor_key) {
    throw std::runtime_error(
        "[replay_artifact_source] AllocationBelief anchor_key mismatch");
  }
  if (belief_state.timestamp_ms > frame.observation.knowledge_timestamp_ms) {
    throw std::runtime_error(
        "[replay_artifact_source] AllocationBelief timestamp is after "
        "observation knowledge timestamp");
  }
  if (belief_state.graph_order_fingerprint != spec.graph_order_fingerprint) {
    throw std::runtime_error(
        "[replay_artifact_source] AllocationBelief graph fingerprint "
        "mismatch");
  }
  if (options.require_allocation_belief_node_match &&
      belief_state.node_ids != spec.risky_node_ids) {
    throw std::runtime_error(
        "[replay_artifact_source] AllocationBelief risky node order mismatch");
  }
  if (options.require_allocation_belief_base_policy_match &&
      (belief_state.base_policy.accounting_numeraire_id !=
           spec.base_policy.accounting_numeraire_id ||
       belief_state.base_policy.settlement_asset_id !=
           spec.base_policy.settlement_asset_id ||
       belief_state.base_policy.reserve_asset_id !=
           spec.base_policy.reserve_asset_id ||
       belief_state.base_policy.projection_reference_node_id !=
           spec.base_policy.projection_reference_node_id)) {
    throw std::runtime_error(
        "[replay_artifact_source] AllocationBelief BasePolicy mismatch");
  }
}

inline void validate_nodelift_potential_belief_for_frame(
    const belief::NodeLiftPotentialBelief &belief_state,
    const replay_frame_t &frame, const episode_spec_t &spec) {
  if (belief_state.anchor_key != frame.observation.anchor_key) {
    throw std::runtime_error(
        "[replay_artifact_source] NodeLiftPotentialBelief anchor_key "
        "mismatch");
  }
  if (belief_state.timestamp_ms > frame.observation.knowledge_timestamp_ms) {
    throw std::runtime_error(
        "[replay_artifact_source] NodeLiftPotentialBelief timestamp is after "
        "observation knowledge timestamp");
  }
  if (!belief_state.graph_order_fingerprint.empty() &&
      belief_state.graph_order_fingerprint != spec.graph_order_fingerprint) {
    throw std::runtime_error(
        "[replay_artifact_source] NodeLiftPotentialBelief graph fingerprint "
        "mismatch");
  }
  if (!belief_state.graph_node_ids.empty() &&
      belief_state.graph_node_ids != spec.graph_node_ids) {
    throw std::runtime_error(
        "[replay_artifact_source] NodeLiftPotentialBelief graph node order "
        "mismatch");
  }
  const auto &axis = belief_state.graph_node_axis;
  if (belief::graph_node_axis_binding_has_payload(axis)) {
    if (!axis.graph_order_fingerprint.empty() &&
        axis.graph_order_fingerprint != spec.graph_order_fingerprint) {
      throw std::runtime_error(
          "[replay_artifact_source] NodeLiftPotentialBelief graph-node-axis "
          "fingerprint mismatch");
    }
    if (!axis.node_ids.empty() && axis.node_ids != spec.graph_node_ids) {
      throw std::runtime_error(
          "[replay_artifact_source] NodeLiftPotentialBelief graph-node-axis "
          "node order mismatch");
    }
  }
}

[[nodiscard]] inline bool edge_market_graph_contains_node(
    const execution::spot_edge_market_state_t &market,
    const portfolio::node_id_t &node_id) {
  for (const auto &market_node_id : market.graph.node_ids) {
    if (market_node_id == node_id) {
      return true;
    }
  }
  return false;
}

inline void validate_edge_market_state_for_frame(
    const execution::spot_edge_market_state_t &market_state,
    const episode_spec_t &spec,
    const replay_observation_artifact_options_t &options) {
  if (options.validate_edge_market_state) {
    execution::validate_spot_edge_market_state(market_state);
  }
  for (const auto &node_id : spec.risky_node_ids) {
    if (!edge_market_graph_contains_node(market_state, node_id)) {
      throw std::runtime_error(
          "[replay_artifact_source] edge market graph missing risky node: " +
          node_id);
    }
  }
  const auto &reserve_node_id = spec.base_policy.reserve_asset_id;
  if (!edge_market_graph_contains_node(market_state, reserve_node_id)) {
    throw std::runtime_error("[replay_artifact_source] edge market graph "
                             "missing base reserve node: " +
                             reserve_node_id);
  }
}

inline void validate_market_state_for_frame(
    const portfolio::MarketState &market_state,
    const observation_t &observation, std::int64_t A,
    const replay_observation_artifact_options_t &options) {
  if (options.validate_market_state) {
    portfolio::validate_market_state(market_state, A);
  }
  if (market_state.timestamp_ms > 0 &&
      market_state.timestamp_ms > observation.knowledge_timestamp_ms) {
    throw std::runtime_error(
        "[replay_artifact_source] MarketState timestamp is after observation "
        "knowledge timestamp");
  }
}

[[nodiscard]] inline double tensor_mean_or_zero(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return 0.0;
  }
  return tensor.to(torch::kFloat64).mean().item<double>();
}

[[nodiscard]] inline torch::Tensor
covariance_from_scenarios(const torch::Tensor &scenarios) {
  TORCH_CHECK(scenarios.defined() && scenarios.dim() == 2,
              "[replay_artifact_source] scenarios must be [S,A]");
  const auto S = scenarios.size(0);
  const auto A = scenarios.size(1);
  auto x = scenarios.to(torch::kFloat64);
  if (S <= 1) {
    return torch::zeros({A, A}, torch::TensorOptions().dtype(torch::kFloat64));
  }
  auto centered = x - x.mean(/*dim=*/0, /*keepdim=*/true);
  return centered.transpose(0, 1).matmul(centered) / static_cast<double>(S - 1);
}

[[nodiscard]] inline torch::Tensor
correlation_from_covariance(const torch::Tensor &covariance,
                            double eps = 1.0e-12) {
  auto cov = covariance.to(torch::kFloat64);
  auto diag = cov.diag().clamp_min(eps).sqrt();
  auto denom = diag.unsqueeze(1).matmul(diag.unsqueeze(0)).clamp_min(eps);
  auto corr = (cov / denom).clamp(-1.0, 1.0);
  corr.diagonal().fill_(1.0);
  return corr;
}

[[nodiscard]] inline torch::Tensor arithmetic_variance_from_log_mixture(
    const cuwacunu::wikimyei::observer::marginal_forecast_artifact_t
        &artifact) {
  auto log_weight = artifact.log_weight.to(torch::kFloat64);
  auto mu = artifact.mu.to(torch::kFloat64);
  auto sigma = artifact.sigma.to(torch::kFloat64);
  auto first =
      (log_weight.exp() * (mu + 0.5 * sigma.pow(2)).exp()).sum(/*dim=*/1);
  auto second =
      (log_weight.exp() * (2.0 * mu + 2.0 * sigma.pow(2)).exp()).sum(/*dim=*/1);
  return (second - first.pow(2)).clamp_min(0.0);
}

[[nodiscard]] inline std::string
resolve_index_path(const std::filesystem::path &path,
                   const std::filesystem::path &index_dir) {
  if (path.empty()) {
    return {};
  }
  return path.is_absolute() ? path.string() : (index_dir / path).string();
}

inline void validate_index_entry_matches_forecast_artifact(
    const replay_observation_artifact_index_entry_t &entry,
    const forecast::marginal_forecast_artifact_t &artifact) {
  if (entry.anchor_key != artifact.identity.anchor_key) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact index anchor_key does not match "
        "forecast artifact identity");
  }
  if (entry.artifact_timestamp_ms != artifact.identity.timestamp_ms) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact index timestamp does not match "
        "forecast artifact identity");
  }
}

[[nodiscard]] inline std::unordered_map<std::string, std::string>
parse_index_line(const std::string &line) {
  std::unordered_map<std::string, std::string> out;
  std::size_t cursor = 0;
  while (cursor <= line.size()) {
    const auto end = line.find('|', cursor);
    const auto token = line.substr(
        cursor, end == std::string::npos ? std::string::npos : end - cursor);
    const auto eq = token.find('=');
    if (eq != std::string::npos) {
      const auto key = token.substr(0, eq);
      const auto value = token.substr(eq + 1U);
      if (!key.empty()) {
        out[key] = value;
      }
    }
    if (end == std::string::npos) {
      break;
    }
    cursor = end + 1U;
  }
  return out;
}

[[nodiscard]] inline std::optional<std::int64_t>
optional_i64_from_string(const std::string &text) {
  if (cuwacunu::kikijyeba::environment::detail::blank(text)) {
    return std::nullopt;
  }
  std::size_t parsed_chars = 0;
  try {
    const auto value = std::stoll(text, &parsed_chars, 10);
    if (parsed_chars != text.size()) {
      throw std::runtime_error("trailing characters");
    }
    return static_cast<std::int64_t>(value);
  } catch (const std::exception &) {
    throw std::runtime_error(
        "[replay_artifact_source] malformed artifact index integer value");
  }
}

[[nodiscard]] inline std::int64_t
required_i64_from_string(const std::string &text, const char *field_name) {
  if (cuwacunu::kikijyeba::environment::detail::blank(text)) {
    throw std::runtime_error(std::string("[replay_artifact_source] malformed "
                                         "artifact index integer field: ") +
                             field_name);
  }
  std::size_t parsed_chars = 0;
  try {
    const auto value = std::stoll(text, &parsed_chars, 10);
    if (parsed_chars != text.size()) {
      throw std::runtime_error("trailing characters");
    }
    return static_cast<std::int64_t>(value);
  } catch (const std::exception &) {
    throw std::runtime_error(std::string("[replay_artifact_source] malformed "
                                         "artifact index integer field: ") +
                             field_name);
  }
}

} // namespace replay_artifact_detail

[[nodiscard]] inline belief::AllocationBelief allocation_belief_from_forecast(
    const forecast::marginal_forecast_artifact_t &artifact) {
  forecast::validate_forecast_artifact(artifact, /*require_scenarios=*/true);
  const auto &id = artifact.identity;
  const auto A = forecast::asset_count(artifact);
  belief::AllocationBelief belief_state{};
  belief_state.anchor_key = id.anchor_key;
  belief_state.timestamp_ms = id.timestamp_ms;
  belief_state.graph_node_axis = id.graph_node_axis;
  belief_state.graph_order_fingerprint = id.graph_order_fingerprint;
  belief_state.source_feature_semantics_id = id.source_feature_semantics_id;
  belief_state.source_feature_semantics_fingerprint =
      id.source_feature_semantics_fingerprint;
  belief_state.graph_node_ids = id.graph_node_ids;
  belief_state.node_ids = id.node_ids;
  belief_state.node_graph_indices = id.node_graph_indices;
  belief_state.base_policy = id.base_policy;
  if (id.projection_reference_graph_index >= 0) {
    belief_state.projection_reference_graph_index =
        id.projection_reference_graph_index;
  }
  belief_state.horizon = id.horizon;
  belief_state.valid_mask = artifact.active_mask.to(torch::kBool);
  belief_state.tradable_mask = belief_state.valid_mask.clone();
  belief_state.expected_log_return =
      artifact.expected_log_return.to(torch::kFloat64);
  belief_state.expected_arithmetic_return =
      artifact.expected_arithmetic_return.to(torch::kFloat64);
  belief_state.scenarios = artifact.scenarios.to(torch::kFloat64);

  if (artifact.covariance.defined() && artifact.covariance.numel() > 0) {
    belief_state.covariance = artifact.covariance.to(torch::kFloat64);
  } else {
    belief_state.covariance = replay_artifact_detail::covariance_from_scenarios(
        belief_state.scenarios);
  }
  if (artifact.correlation.defined() && artifact.correlation.numel() > 0) {
    belief_state.correlation = artifact.correlation.to(torch::kFloat64);
  } else {
    belief_state.correlation =
        replay_artifact_detail::correlation_from_covariance(
            belief_state.covariance);
  }
  belief_state.marginal_variance =
      belief_state.covariance.diag().clamp_min(0.0);
  if ((!belief_state.marginal_variance.defined() ||
       belief_state.marginal_variance.numel() == 0 ||
       (belief_state.marginal_variance <= 0.0).all().item<bool>()) &&
      artifact.log_weight.defined() && artifact.log_weight.size(0) == A) {
    belief_state.marginal_variance =
        replay_artifact_detail::arithmetic_variance_from_log_mixture(artifact);
  }
  belief_state.marginal_volatility = belief_state.marginal_variance.sqrt();
  belief_state.confidence =
      artifact.confidence.defined() && artifact.confidence.numel() > 0
          ? artifact.confidence.to(torch::kFloat64)
          : torch::ones({A}, torch::TensorOptions().dtype(torch::kFloat64));
  belief_state.projection_validation_required = true;
  belief_state.projection_validated = false;
  belief_state.live_capital_allowed = false;
  belief_state.valid = true;
  belief::validate_allocation_belief_contract(belief_state);
  return belief_state;
}

[[nodiscard]] inline replay_observation_artifacts_t
make_replay_observation_artifacts_from_forecast_artifact(
    const forecast::marginal_forecast_artifact_t &forecast_artifact,
    artifact_ref_t mdn_artifact = {}) {
  forecast::validate_forecast_artifact(forecast_artifact,
                                       /*require_scenarios=*/true);
  replay_observation_artifacts_t out{};
  out.anchor_key = forecast_artifact.identity.anchor_key;
  out.artifact_timestamp_ms = forecast_artifact.identity.timestamp_ms;
  if (!mdn_artifact.artifact_id.empty() ||
      !mdn_artifact.artifact_path.empty() ||
      !mdn_artifact.artifact_digest.empty() ||
      !mdn_artifact.schema_id.empty()) {
    out.mdn_artifact = std::move(mdn_artifact);
  }
  out.allocation_belief = allocation_belief_from_forecast(forecast_artifact);
  out.projected_log_return_scenarios =
      forecast_artifact.scenarios.to(torch::kFloat64).log1p();
  out.active_projection_mask = forecast_artifact.active_mask.to(torch::kBool);
  return out;
}

inline void validate_replay_observation_artifact_index_entry(
    const replay_observation_artifact_index_entry_t &entry) {
  if (entry.anchor_key.empty() || entry.forecast_artifact_path.empty()) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact index entry requires anchor_key "
        "and forecast_artifact_path");
  }
  if (cuwacunu::kikijyeba::environment::detail::path_contains_parent_reference(
          entry.forecast_artifact_path)) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact index forecast_artifact_path must "
        "not contain parent-directory components");
  }
  if (!entry.mdn_artifact.artifact_path.empty() &&
      cuwacunu::kikijyeba::environment::detail::path_contains_parent_reference(
          std::filesystem::path(entry.mdn_artifact.artifact_path))) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact index mdn_artifact_path must not "
        "contain parent-directory components");
  }
  if (entry.artifact_timestamp_ms <= 0) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact index entry requires positive "
        "artifact_timestamp_ms");
  }
  if (entry.observation_anchor_index.has_value() &&
      *entry.observation_anchor_index < 0) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact index entry observation anchor "
        "index must be nonnegative");
  }
}

inline void write_replay_observation_artifact_index(
    const std::filesystem::path &path,
    const std::vector<replay_observation_artifact_index_entry_t> &entries) {
  if (path.empty()) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact index path is required");
  }
  if (entries.empty()) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact index entries are required");
  }
  std::unordered_set<std::string> seen_anchor_keys;
  seen_anchor_keys.reserve(entries.size());
  for (const auto &entry : entries) {
    validate_replay_observation_artifact_index_entry(entry);
    if (!seen_anchor_keys.insert(entry.anchor_key).second) {
      throw std::runtime_error(
          "[replay_artifact_source] duplicate artifact index anchor_key: " +
          entry.anchor_key);
    }
  }

  const auto parent_path = path.parent_path();
  if (!parent_path.empty()) {
    std::filesystem::create_directories(parent_path);
  }
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error(
        "[replay_artifact_source] unable to write artifact index: " +
        path.string());
  }
  out << "schema=" << kReplayObservationArtifactIndexSchema << "\n";
  for (const auto &entry : entries) {
    out << "entry|anchor_key=" << entry.anchor_key
        << "|artifact_timestamp_ms=" << entry.artifact_timestamp_ms
        << "|forecast_artifact_path="
        << entry.forecast_artifact_path.generic_string();
    if (entry.observation_anchor_index.has_value()) {
      out << "|observation_anchor_index=" << *entry.observation_anchor_index;
    }
    if (!entry.mdn_artifact.artifact_id.empty()) {
      out << "|mdn_artifact_id=" << entry.mdn_artifact.artifact_id;
    }
    if (!entry.mdn_artifact.artifact_path.empty()) {
      out << "|mdn_artifact_path=" << entry.mdn_artifact.artifact_path;
    }
    if (!entry.mdn_artifact.artifact_digest.empty()) {
      out << "|mdn_artifact_digest=" << entry.mdn_artifact.artifact_digest;
    }
    if (!entry.mdn_artifact.schema_id.empty()) {
      out << "|mdn_schema_id=" << entry.mdn_artifact.schema_id;
    }
    out << "\n";
  }
}

[[nodiscard]] inline std::vector<replay_observation_artifact_index_entry_t>
read_replay_observation_artifact_index(const std::filesystem::path &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error(
        "[replay_artifact_source] unable to read artifact index: " +
        path.string());
  }
  std::vector<replay_observation_artifact_index_entry_t> entries;
  std::unordered_set<std::string> seen_anchor_keys;
  std::string line;
  bool saw_schema = false;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }
    if (line.rfind("schema=", 0) == 0) {
      saw_schema = line.substr(7) == kReplayObservationArtifactIndexSchema;
      continue;
    }
    if (line.rfind("entry|", 0) != 0) {
      continue;
    }
    auto fields = replay_artifact_detail::parse_index_line(line.substr(6));
    replay_observation_artifact_index_entry_t entry{};
    entry.anchor_key = fields["anchor_key"];
    entry.observation_anchor_index =
        replay_artifact_detail::optional_i64_from_string(
            fields["observation_anchor_index"]);
    entry.artifact_timestamp_ms = static_cast<portfolio::timestamp_ms_t>(
        replay_artifact_detail::required_i64_from_string(
            fields["artifact_timestamp_ms"], "artifact_timestamp_ms"));
    entry.forecast_artifact_path = fields["forecast_artifact_path"];
    entry.mdn_artifact = artifact_ref_t{
        .artifact_id = fields["mdn_artifact_id"],
        .artifact_path = fields["mdn_artifact_path"],
        .artifact_digest = fields["mdn_artifact_digest"],
        .schema_id = fields["mdn_schema_id"],
    };
    validate_replay_observation_artifact_index_entry(entry);
    if (!seen_anchor_keys.insert(entry.anchor_key).second) {
      throw std::runtime_error(
          "[replay_artifact_source] duplicate artifact index anchor_key: " +
          entry.anchor_key);
    }
    entries.push_back(std::move(entry));
  }
  if (!saw_schema) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact index schema mismatch");
  }
  if (entries.empty()) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact index entries are required");
  }
  return entries;
}

[[nodiscard]] inline std::vector<replay_observation_artifacts_t>
load_replay_observation_artifacts_from_index(
    const std::filesystem::path &path,
    const torch::Device &device = torch::Device(torch::kCPU)) {
  const auto entries = read_replay_observation_artifact_index(path);
  const auto index_dir = path.parent_path();
  std::vector<replay_observation_artifacts_t> out;
  out.reserve(entries.size());
  for (const auto &entry : entries) {
    const auto forecast_path =
        std::filesystem::path(replay_artifact_detail::resolve_index_path(
            entry.forecast_artifact_path, index_dir));
    auto forecast_artifact =
        forecast::load_forecast_artifact(forecast_path, device);
    replay_artifact_detail::validate_index_entry_matches_forecast_artifact(
        entry, forecast_artifact);
    auto artifacts = make_replay_observation_artifacts_from_forecast_artifact(
        forecast_artifact, entry.mdn_artifact);
    artifacts.anchor_key = entry.anchor_key;
    artifacts.observation_anchor_index = entry.observation_anchor_index;
    artifacts.artifact_timestamp_ms = entry.artifact_timestamp_ms;
    if (artifacts.mdn_artifact.has_value()) {
      auto &mdn = *artifacts.mdn_artifact;
      mdn.artifact_path = replay_artifact_detail::resolve_index_path(
          std::filesystem::path(mdn.artifact_path), index_dir);
    }
    out.push_back(std::move(artifacts));
  }
  return out;
}

class replay_observation_artifact_source_iface_t {
public:
  virtual ~replay_observation_artifact_source_iface_t() = default;

  [[nodiscard]] virtual std::string source_id() const = 0;
  [[nodiscard]] virtual std::optional<replay_observation_artifacts_t>
  artifacts_for(const observation_t &observation) = 0;
  virtual void reset() = 0;
};

class keyed_replay_observation_artifact_source_t final
    : public replay_observation_artifact_source_iface_t {
public:
  keyed_replay_observation_artifact_source_t(
      std::string source_id,
      std::vector<replay_observation_artifacts_t> artifacts)
      : source_id_(std::move(source_id)) {
    if (cuwacunu::kikijyeba::environment::detail::blank(source_id_)) {
      throw std::runtime_error(
          "[keyed_replay_observation_artifact_source] source_id is required");
    }
    for (auto &artifact : artifacts) {
      if (cuwacunu::kikijyeba::environment::detail::blank(
              artifact.anchor_key)) {
        throw std::runtime_error(
            "[keyed_replay_observation_artifact_source] artifact anchor_key is "
            "required");
      }
      if (!replay_artifact_detail::has_payload(artifact)) {
        throw std::runtime_error(
            "[keyed_replay_observation_artifact_source] artifact has no "
            "payload");
      }
      const auto key = artifact.anchor_key;
      const auto [_, inserted] =
          artifacts_by_anchor_.emplace(key, std::move(artifact));
      if (!inserted) {
        throw std::runtime_error(
            "[keyed_replay_observation_artifact_source] duplicate artifact "
            "anchor_key: " +
            key);
      }
    }
  }

  [[nodiscard]] std::string source_id() const override { return source_id_; }

  [[nodiscard]] std::optional<replay_observation_artifacts_t>
  artifacts_for(const observation_t &observation) override {
    const auto found = artifacts_by_anchor_.find(observation.anchor_key);
    if (found == artifacts_by_anchor_.end()) {
      return std::nullopt;
    }
    return found->second;
  }

  void reset() override {}

  [[nodiscard]] std::size_t size() const { return artifacts_by_anchor_.size(); }

private:
  std::string source_id_{};
  std::unordered_map<std::string, replay_observation_artifacts_t>
      artifacts_by_anchor_{};
};

inline void apply_replay_observation_artifacts(
    replay_frame_t &frame, const replay_observation_artifacts_t &artifacts,
    const episode_spec_t &spec,
    const replay_observation_artifact_options_t &options = {}) {
  validate_episode_spec(spec);
  if (!replay_artifact_detail::has_payload(artifacts)) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact has no payload");
  }
  if (options.require_anchor_key_match &&
      artifacts.anchor_key != frame.observation.anchor_key) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact anchor_key mismatch");
  }
  if (options.require_anchor_index_match &&
      artifacts.observation_anchor_index.has_value() &&
      *artifacts.observation_anchor_index !=
          frame.observation.observation_anchor_index) {
    throw std::runtime_error(
        "[replay_artifact_source] artifact observation anchor index mismatch");
  }
  replay_artifact_detail::validate_artifact_time(artifacts, frame.observation,
                                                 options);

  const auto A = static_cast<std::int64_t>(spec.risky_node_ids.size());
  replay_artifact_detail::validate_projection_tensors(artifacts, A);

  if (artifacts.allocation_belief.has_value()) {
    replay_artifact_detail::validate_allocation_belief_for_frame(
        *artifacts.allocation_belief, frame, spec, options);
    frame.observation.allocation_belief = artifacts.allocation_belief;
  }
  if (artifacts.nodelift_potential_belief.has_value()) {
    replay_artifact_detail::validate_nodelift_potential_belief_for_frame(
        *artifacts.nodelift_potential_belief, frame, spec);
    frame.observation.nodelift_potential_belief =
        artifacts.nodelift_potential_belief;
  }
  if (artifacts.mdn_artifact.has_value()) {
    frame.observation.mdn_artifact = artifacts.mdn_artifact;
  }
  if (artifacts.market_state.has_value()) {
    replay_artifact_detail::validate_market_state_for_frame(
        *artifacts.market_state, frame.observation, A, options);
    frame.observation.market_state = *artifacts.market_state;
  }
  if (artifacts.edge_market_state.has_value()) {
    replay_artifact_detail::validate_edge_market_state_for_frame(
        *artifacts.edge_market_state, spec, options);
    frame.observation.edge_market_state = *artifacts.edge_market_state;
  }
  if (artifacts.projected_log_return_scenarios.defined()) {
    frame.projected_log_return_scenarios =
        artifacts.projected_log_return_scenarios.to(torch::kFloat64)
            .contiguous();
  }
  if (artifacts.active_projection_mask.defined()) {
    frame.active_projection_mask =
        artifacts.active_projection_mask.to(torch::kBool).contiguous();
  }
  if (artifacts.nodelift_residual_energy.defined()) {
    frame.nodelift_residual_energy =
        artifacts.nodelift_residual_energy.to(torch::kFloat64).contiguous();
  }
  if (artifacts.nodelift_residual_mask.defined()) {
    frame.nodelift_residual_mask =
        artifacts.nodelift_residual_mask.to(torch::kBool).contiguous();
  }
}

inline void enrich_replay_episode_bundle_with_artifacts(
    replay_episode_bundle_t &bundle,
    replay_observation_artifact_source_iface_t &artifact_source,
    replay_observation_artifact_options_t options = {}) {
  validate_replay_episode_bundle(bundle);
  artifact_source.reset();
  for (auto &frame : bundle.frames) {
    auto artifacts = artifact_source.artifacts_for(frame.observation);
    if (!artifacts.has_value()) {
      if (options.require_artifacts_per_frame) {
        throw std::runtime_error(
            "[replay_artifact_source] missing artifacts for frame anchor: " +
            frame.observation.anchor_key);
      }
      continue;
    }
    apply_replay_observation_artifacts(frame, *artifacts, bundle.spec, options);
  }
  validate_replay_episode_bundle(bundle);
}

} // namespace cuwacunu::kikijyeba::environment::replay
