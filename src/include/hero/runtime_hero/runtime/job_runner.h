// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "hero/config_path_defaults.h"
#include "hero/lattice_hero/lattice/exposure/exposure_ledger.h"
#include "hero/runtime_hero/runtime/job_events_probe_sidecar.h"
#include "hero/runtime_hero/runtime/job_layout.h"
#include "hero/runtime_hero/runtime/job_manifest.h"
#include "hero/runtime_hero/runtime/job_state.h"
#include "hero/runtime_hero/runtime/probe_settings.h"
#include "hero/runtime_hero/runtime/terminal_facts.h"
#include "hero/runtime_hero/runtime/wave_plan.h"
#include "jkimyei/training/inference/channel_graph_first_inference_launcher.h"
#include "jkimyei/training/representation/channel_graph_first_representation_launcher.h"
#include "jkimyei/training/representation/mtf_jepa_mae_vicreg_graph_first_launcher.h"
#include "kikijyeba/environment/runtime/replay_source.h"
#include "kikijyeba/protocol/config_bundle.h"
#include "kikijyeba/protocol/config_provenance.h"
#include "kikijyeba/protocol/pipeline_builder.h"

namespace cuwacunu::hero::runtime {

struct job_runner_options_t {
  runtime_job_kind_t job_kind{runtime_job_kind_t::channel_inference_mdn};
  bool job_kind_from_target{true};
  bool dry_run{false};
  bool resume{false};
  bool force_rebuild_cache{false};
  bool compute_alignment_diagnostics{true};
  bool display_mdn_model{false};
  bool write_report{true};
  bool write_probe_records{false};
  bool write_replay_artifacts{true};
  bool replay_require_direct_accounting_numeraire_valuation_edges{true};
  std::size_t batch_size{0};
  std::string job_id{};
  std::string replay_accounting_numeraire_node_id{};
  std::vector<std::string> replay_target_node_ids{};
  std::string runtime_handoff_id{};
  std::string runtime_handoff_digest{};
  std::string marshal_target_driver_run_id{};
  std::string input_representation_checkpoint_path{};
  std::string input_mdn_checkpoint_path{};
  bool source_range_override_enabled{false};
  std::string source_range_override{};
  std::optional<std::size_t> anchor_index_begin_override{std::nullopt};
  std::optional<std::size_t> anchor_index_end_override{std::nullopt};
  std::optional<std::int64_t> source_key_begin_override{std::nullopt};
  std::optional<std::int64_t> source_key_end_override{std::nullopt};
  std::filesystem::path job_dir{};
  cuwacunu::hero::lattice::runtime_report::runtime_report_mode_t
      runtime_report_mode{cuwacunu::hero::lattice::runtime_report::
                              runtime_report_mode_t::normal};
};

struct job_run_result_t {
  job_manifest_t manifest{};
  job_state_t state{};
  wave_plan_t wave_plan{};
  std::filesystem::path manifest_path{};
  std::filesystem::path state_path{};
  std::filesystem::path delegated_report_path{};
};

namespace job_runner_detail {

inline void ensure_job_dir(const std::filesystem::path &path) {
  if (path.empty()) {
    throw std::runtime_error("[kikijyeba_job_runner] job_dir is required");
  }
  std::filesystem::create_directories(path);
}

[[nodiscard]] inline std::string sanitize_job_dir_leaf(std::string value) {
  if (value.empty()) {
    return "job";
  }
  for (auto &ch : value) {
    const auto u = static_cast<unsigned char>(ch);
    if (!(std::isalnum(u) || ch == '_' || ch == '-' || ch == '.')) {
      ch = '_';
    }
  }
  return value;
}

[[nodiscard]] inline std::filesystem::path
default_job_root_for_config(const std::string &config_path) {
  return cuwacunu::hero::config_paths::default_runtime_root_path(
      std::filesystem::path(config_path));
}

[[nodiscard]] inline std::filesystem::path
default_job_dir_for_manifest(const std::string &config_path,
                             const job_manifest_t &manifest) {
  const auto runtime_root = default_job_root_for_config(config_path);
  const std::string component_family_id =
      manifest.component_family_id.empty() ? manifest.target_component_family_id
                                           : manifest.component_family_id;
  return job_layout::job_dir(runtime_root, component_family_id,
                             manifest.component_spawn_id,
                             manifest.component_spawn_fingerprint,
                             manifest.wave_action, manifest.job_id);
}

[[nodiscard]] inline std::filesystem::path
manifest_path_for_job_dir(const std::filesystem::path &job_dir) {
  return job_dir / "job.manifest";
}

[[nodiscard]] inline std::filesystem::path
state_path_for_job_dir(const std::filesystem::path &job_dir) {
  return job_dir / "job.state";
}

[[nodiscard]] inline std::filesystem::path
delegated_report_path_for_job(const std::filesystem::path &job_dir,
                              runtime_job_kind_t kind) {
  switch (kind) {
  case runtime_job_kind_t::channel_representation_vicreg:
  case runtime_job_kind_t::channel_representation_mtf_jepa_mae_vicreg:
    return job_dir / "channel_representation.report";
  case runtime_job_kind_t::channel_inference_mdn:
    return job_dir / "channel_inference.report";
  case runtime_job_kind_t::policy_training:
    return job_dir / "policy_training.contract";
  }
  return job_dir / "job.report";
}

[[nodiscard]] inline std::string zero_padded_attempt_index(std::size_t index) {
  std::string text = std::to_string(index);
  if (text.size() < 6U) {
    text.insert(text.begin(), 6U - text.size(), '0');
  }
  return text;
}

[[nodiscard]] inline std::string job_attempt_id_for_index(std::size_t index) {
  return "attempt_" + zero_padded_attempt_index(index);
}

[[nodiscard]] inline std::string
attempt_job_id(const std::string &stable_job_id, std::size_t index) {
  return stable_job_id + "." + job_attempt_id_for_index(index);
}

[[nodiscard]] inline bool
job_dir_has_runtime_artifacts(const std::filesystem::path &job_dir) {
  std::error_code ec;
  if (!std::filesystem::exists(job_dir, ec) || ec) {
    return false;
  }
  ec.clear();
  const std::filesystem::path artifacts[] = {
      job_dir / "job.manifest",
      job_dir / "job.state",
      job_dir / "channel_representation.report",
      job_dir / "channel_inference.report",
      job_dir / job_events_probe::k_job_events_probe_stream_leaf,
      job_dir / "runtime.result.fact",
      job_dir / "runtime.checkpoint_io.fact",
      job_dir / "runtime.health_measurement.fact",
  };
  for (const auto &artifact : artifacts) {
    if (std::filesystem::exists(artifact, ec) && !ec) {
      return true;
    }
    ec.clear();
  }
  return false;
}

inline void apply_attempt_identity(job_manifest_t *manifest,
                                   std::size_t attempt_index,
                                   std::string policy) {
  if (manifest == nullptr) {
    return;
  }
  if (manifest->job_stable_id.empty()) {
    manifest->job_stable_id = manifest->job_id;
  }
  manifest->job_attempt_index = attempt_index;
  manifest->job_attempt_id = job_attempt_id_for_index(attempt_index);
  manifest->job_attempt_policy = std::move(policy);
  manifest->job_id = attempt_job_id(manifest->job_stable_id, attempt_index);
}

[[nodiscard]] inline std::filesystem::path
reserve_immutable_attempt_job_dir(const std::filesystem::path &runtime_root,
                                  job_manifest_t *manifest) {
  if (manifest == nullptr) {
    throw std::runtime_error(
        "[kikijyeba_job_runner] manifest is required for job attempt");
  }
  if (manifest->job_stable_id.empty()) {
    manifest->job_stable_id = manifest->job_id;
  }
  if (manifest->job_stable_id.empty()) {
    throw std::runtime_error(
        "[kikijyeba_job_runner] job_stable_id is required for job attempt");
  }

  const std::string component_family_id =
      manifest->component_family_id.empty()
          ? manifest->target_component_family_id
          : manifest->component_family_id;
  const auto spawn_dir = job_layout::component_spawn_dir(
      runtime_root, component_family_id, manifest->component_spawn_id,
      manifest->component_spawn_fingerprint);
  const auto action_dir =
      spawn_dir / "jobs" /
      job_layout::sanitize_path_component(manifest->wave_action.empty()
                                              ? std::string{"unknown"}
                                              : manifest->wave_action);
  std::filesystem::create_directories(action_dir);

  for (std::size_t attempt = 0; attempt < 1000000U; ++attempt) {
    apply_attempt_identity(manifest, attempt, "immutable_attempt_v1");
    const auto candidate =
        action_dir / job_layout::sanitize_path_component(manifest->job_id);
    std::error_code ec;
    if (std::filesystem::create_directory(candidate, ec)) {
      return candidate;
    }
    if (ec && !std::filesystem::exists(candidate)) {
      throw std::runtime_error(
          "[kikijyeba_job_runner] failed to reserve job attempt directory: " +
          candidate.string() + ": " + ec.message());
    }
  }
  throw std::runtime_error(
      "[kikijyeba_job_runner] exhausted immutable job attempt ids for " +
      manifest->job_stable_id);
}

inline void prepare_explicit_job_dir(const std::filesystem::path &job_dir,
                                     job_manifest_t *manifest) {
  if (manifest == nullptr) {
    return;
  }
  if (job_dir_has_runtime_artifacts(job_dir)) {
    throw std::runtime_error(
        "[kikijyeba_job_runner] refusing to overwrite existing job_dir: " +
        job_dir.string());
  }
  if (manifest->job_stable_id.empty()) {
    manifest->job_stable_id = manifest->job_id;
  }
  manifest->job_attempt_index = 0;
  manifest->job_attempt_id = "explicit";
  manifest->job_attempt_policy = "explicit_job_dir_no_overwrite_v1";
}

[[nodiscard]] inline runtime_job_kind_t runtime_job_kind_from_target(
    cuwacunu::hero::runtime::settings::wave_target_t target) {
  switch (target) {
  case cuwacunu::hero::runtime::settings::wave_target_t::vicreg_representation:
    return runtime_job_kind_t::channel_representation_vicreg;
  case cuwacunu::hero::runtime::settings::wave_target_t::
      mtf_jepa_mae_vicreg_representation:
    return runtime_job_kind_t::channel_representation_mtf_jepa_mae_vicreg;
  case cuwacunu::hero::runtime::settings::wave_target_t::inference_channel_mdn:
    return runtime_job_kind_t::channel_inference_mdn;
  case cuwacunu::hero::runtime::settings::wave_target_t::
      graph_node_allocation_policy:
    return runtime_job_kind_t::policy_training;
  }
  throw std::runtime_error("[kikijyeba_job_runner] unknown wave target");
}

[[nodiscard]] inline bool train_target_from_wave_action(
    cuwacunu::hero::runtime::settings::wave_action_t action) {
  switch (action) {
  case cuwacunu::hero::runtime::settings::wave_action_t::run:
    return false;
  case cuwacunu::hero::runtime::settings::wave_action_t::train:
    return true;
  }
  throw std::runtime_error("[kikijyeba_job_runner] unknown wave action");
}

[[nodiscard]] inline double
finite_tensor_mean_or_nan(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  auto values = tensor.to(torch::kFloat64).contiguous().view({-1});
  if (!torch::isfinite(values).all().item<bool>()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return values.mean().item<double>();
}

[[nodiscard]] inline std::string
tensor_shape_text(const torch::Tensor &tensor) {
  if (!tensor.defined()) {
    return "undefined";
  }
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < tensor.sizes().size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << tensor.sizes()[i];
  }
  out << "]";
  return out.str();
}

[[nodiscard]] inline std::filesystem::path
resolve_replay_index_path(const std::filesystem::path &path,
                          const std::filesystem::path &index_dir) {
  if (path.empty()) {
    return {};
  }
  return path.is_absolute() ? path : index_dir / path;
}

inline void write_runtime_replay_observer_belief_fact_sidecar(
    const std::filesystem::path &job_dir,
    const cuwacunu::hero::lattice::exposure::lattice_exposure_fact_t &parent) {
  namespace exposure = cuwacunu::hero::lattice::exposure;
  namespace replay = cuwacunu::kikijyeba::environment::replay;

  const auto batch_entries = replay::read_runtime_replay_batch_index(job_dir);
  if (batch_entries.empty()) {
    return;
  }

  std::int64_t belief_count = 0;
  std::int64_t scenario_count_total = 0;
  std::int64_t covariance_count = 0;
  std::int64_t active_asset_count_total = 0;
  double confidence_sum = 0.0;
  std::int64_t confidence_count = 0;
  double liquidity_sum = 0.0;
  std::int64_t liquidity_count = 0;
  double channel_disagreement_sum = 0.0;
  std::int64_t channel_disagreement_count = 0;
  double expected_log_return_sum = 0.0;
  std::int64_t expected_log_return_count = 0;
  double expected_arithmetic_return_sum = 0.0;
  std::int64_t expected_arithmetic_return_count = 0;
  std::string feature_semantics_fingerprint{};
  bool feature_semantics_drift = false;
  std::ostringstream forecast_digest_basis;
  std::ostringstream scenario_digest_basis;
  std::ostringstream lineage;

  for (const auto &batch_entry : batch_entries) {
    const auto paths = replay::read_runtime_replay_artifact_path_index_at(
        batch_entry.artifact_path_index_path);
    if (paths.observation_artifact_index_path.empty()) {
      continue;
    }
    const auto observation_index_dir =
        paths.observation_artifact_index_path.parent_path();
    const auto index_entries = replay::read_replay_observation_artifact_index(
        paths.observation_artifact_index_path);
    const auto artifacts = replay::load_replay_observation_artifacts_from_index(
        paths.observation_artifact_index_path);
    const auto count = std::min(index_entries.size(), artifacts.size());
    for (std::size_t i = 0; i < count; ++i) {
      const auto &index_entry = index_entries[i];
      const auto &artifacts_for_anchor = artifacts[i];
      const auto forecast_path = resolve_replay_index_path(
          index_entry.forecast_artifact_path, observation_index_dir);
      const auto forecast_digest =
          exposure::file_content_digest_checked(forecast_path);
      if (forecast_digest.ok) {
        forecast_digest_basis << forecast_path.generic_string() << "="
                              << forecast_digest.digest << ";";
        scenario_digest_basis << forecast_digest.digest << ";";
      }
      lineage << "anchor=" << index_entry.anchor_key
              << ",observation_anchor_index="
              << (index_entry.observation_anchor_index.has_value()
                      ? std::to_string(*index_entry.observation_anchor_index)
                      : std::string("unknown"))
              << ",mdn_artifact_id=" << index_entry.mdn_artifact.artifact_id
              << ",mdn_artifact_digest="
              << index_entry.mdn_artifact.artifact_digest << ";";

      if (!artifacts_for_anchor.allocation_belief.has_value()) {
        continue;
      }
      const auto &belief = *artifacts_for_anchor.allocation_belief;
      ++belief_count;
      if (feature_semantics_fingerprint.empty()) {
        feature_semantics_fingerprint =
            belief.source_feature_semantics_fingerprint;
      } else if (!belief.source_feature_semantics_fingerprint.empty() &&
                 belief.source_feature_semantics_fingerprint !=
                     feature_semantics_fingerprint) {
        feature_semantics_drift = true;
      }
      if (belief.scenarios.defined() && belief.scenarios.dim() == 2) {
        scenario_count_total += belief.scenarios.size(0);
        scenario_digest_basis
            << "scenario_shape=" << tensor_shape_text(belief.scenarios) << ";";
      }
      if (belief.covariance.defined() && belief.covariance.numel() > 0) {
        ++covariance_count;
        scenario_digest_basis
            << "covariance_shape=" << tensor_shape_text(belief.covariance)
            << ";";
      }
      if (belief.valid_mask.defined()) {
        active_asset_count_total +=
            belief.valid_mask.to(torch::kBool).sum().item<std::int64_t>();
      }
      const auto confidence = finite_tensor_mean_or_nan(belief.confidence);
      if (std::isfinite(confidence)) {
        confidence_sum += confidence;
        ++confidence_count;
      }
      const auto liquidity = finite_tensor_mean_or_nan(belief.liquidity_score);
      if (std::isfinite(liquidity)) {
        liquidity_sum += liquidity;
        ++liquidity_count;
      }
      const auto channel_disagreement =
          finite_tensor_mean_or_nan(belief.channel_disagreement);
      if (std::isfinite(channel_disagreement)) {
        channel_disagreement_sum += channel_disagreement;
        ++channel_disagreement_count;
      }
      const auto expected_log =
          finite_tensor_mean_or_nan(belief.expected_log_return);
      if (std::isfinite(expected_log)) {
        expected_log_return_sum += expected_log;
        ++expected_log_return_count;
      }
      const auto expected_arithmetic =
          finite_tensor_mean_or_nan(belief.expected_arithmetic_return);
      if (std::isfinite(expected_arithmetic)) {
        expected_arithmetic_return_sum += expected_arithmetic;
        ++expected_arithmetic_return_count;
      }
    }
  }

  if (belief_count == 0) {
    return;
  }

  const auto manifest = job_layout::parse_kv_file(job_dir / "job.manifest");
  exposure::lattice_observer_belief_fact_t fact{};
  exposure::populate_artifact_fact_identity(fact, parent);
  fact.belief_kind = "allocation_belief";
  fact.channel_consensus =
      "belief_count=" + std::to_string(belief_count) +
      ";mean_channel_disagreement=" +
      std::to_string(channel_disagreement_count == 0
                         ? 0.0
                         : channel_disagreement_sum /
                               static_cast<double>(channel_disagreement_count));
  fact.potential_surface_diagnostics =
      "active_asset_count=" + std::to_string(active_asset_count_total) +
      ";feature_semantics_drift=" +
      std::string(feature_semantics_drift ? "true" : "false");
  fact.nodelift_return_projection =
      "return_origin=numeraire_relative_nodelift_projection;"
      "mean_expected_log_return=" +
      std::to_string(expected_log_return_count == 0
                         ? 0.0
                         : expected_log_return_sum /
                               static_cast<double>(expected_log_return_count)) +
      ";mean_expected_arithmetic_return=" +
      std::to_string(
          expected_arithmetic_return_count == 0
              ? 0.0
              : expected_arithmetic_return_sum /
                    static_cast<double>(expected_arithmetic_return_count));
  fact.covariance_coupling =
      "covariance_count=" + std::to_string(covariance_count) +
      ";scenario_count=" + std::to_string(scenario_count_total);
  fact.scenario_bank_digest =
      exposure::exposure_digest_for_text(scenario_digest_basis.str());
  fact.nodelift_residual_quality = "deferred_runtime_replay_v1";
  fact.projection_validation_scores =
      "projection_validation_required=true;live_capital_allowed=false";
  fact.confidence =
      confidence_count == 0
          ? std::numeric_limits<double>::quiet_NaN()
          : confidence_sum / static_cast<double>(confidence_count);
  fact.data_quality = 1.0;
  fact.liquidity = liquidity_count == 0
                       ? std::numeric_limits<double>::quiet_NaN()
                       : liquidity_sum / static_cast<double>(liquidity_count);
  const auto forecast_eval_facts =
      exposure::make_forecast_eval_facts_from_job_dir(job_dir, parent);
  if (!forecast_eval_facts.empty()) {
    fact.forecast_artifact_digest =
        forecast_eval_facts.front().forecast_artifact_digest;
    fact.forecast_artifact_lineage =
        exposure::forecast_eval_fact_digest(forecast_eval_facts.front());
  } else {
    fact.forecast_artifact_digest =
        exposure::exposure_digest_for_text(forecast_digest_basis.str());
    fact.forecast_artifact_lineage =
        exposure::exposure_digest_for_text(lineage.str());
  }
  fact.feature_semantics_fingerprint = feature_semantics_fingerprint;
  fact.dock_binding_fingerprint =
      job_layout::map_get(manifest, "dock_binding_fingerprint");
  if (fact.dock_binding_fingerprint.empty()) {
    fact.dock_binding_fingerprint = parent.component_assembly_fingerprint;
  }
  exposure::write_observer_belief_fact_sidecar(
      job_dir / "lattice.observer_belief.fact", fact);
}

inline void apply_source_range_override(
    cuwacunu::hero::runtime::settings::wave_settings_t *settings,
    const job_runner_options_t &options) {
  if (settings == nullptr || !options.source_range_override_enabled) {
    return;
  }
  settings->source_range_policy =
      cuwacunu::hero::runtime::settings::parse_source_range_policy(
          options.source_range_override);
  settings->anchor_index_begin = options.anchor_index_begin_override;
  settings->anchor_index_end = options.anchor_index_end_override;
  settings->source_key_begin = options.source_key_begin_override;
  settings->source_key_end = options.source_key_end_override;
  cuwacunu::hero::runtime::settings::validate_wave_settings(*settings);
}

inline void apply_model_state_input_overrides(
    cuwacunu::kikijyeba::protocol::channel_graph_first_config_bundle_t *bundle,
    const job_runner_options_t &options) {
  if (bundle == nullptr) {
    return;
  }
  if (!options.input_representation_checkpoint_path.empty()) {
    bundle->channel_mdn_training.input_representation_checkpoint_path =
        std::filesystem::path(options.input_representation_checkpoint_path)
            .lexically_normal()
            .string();
  }
  if (!options.input_mdn_checkpoint_path.empty()) {
    bundle->channel_mdn_training.input_mdn_checkpoint_path =
        std::filesystem::path(options.input_mdn_checkpoint_path)
            .lexically_normal()
            .string();
  }
}

inline void write_lattice_fact_sidecars(const std::filesystem::path &job_dir,
                                        job_state_t *state) {
  if (state == nullptr) {
    return;
  }
  namespace exposure = cuwacunu::hero::lattice::exposure;
  try {
    const auto fact = exposure::make_exposure_fact_from_job_dir(job_dir);
    const auto exposure_path =
        exposure::exposure_fact_path_for_job_dir(job_dir);
    exposure::write_exposure_fact_sidecar(exposure_path, fact);
    state->lattice_exposure_fact_written = true;
    state->lattice_exposure_fact_path = exposure_path.string();
    const auto source_analytics_path =
        job_dir / "lattice.source_analytics.fact";
    if (!std::filesystem::exists(source_analytics_path)) {
      const auto source_analytics =
          exposure::make_runtime_source_analytics_fact_from_exposure_fact(
              fact, job_dir);
      exposure::write_source_analytics_fact_sidecar(source_analytics_path,
                                                    source_analytics);
    }
    state->lattice_source_analytics_fact_written = true;
    state->lattice_source_analytics_fact_path = source_analytics_path.string();
    if (!fact.output_checkpoint.empty() &&
        std::filesystem::exists(fact.output_checkpoint)) {
      auto checkpoint_fact =
          exposure::make_checkpoint_fact_from_exposure_fact(fact);
      exposure::bind_checkpoint_generation_parents_from_sidecars(
          checkpoint_fact);
      auto update_fact =
          exposure::make_component_training_update_fact_from_checkpoint_fact(
              checkpoint_fact, fact.model_state_mutated ? std::string("train")
                                                        : std::string("smoke"));
      update_fact.split_role =
          exposure::exposure_split_role_name(fact.split_role);
      update_fact.optimizer_update_count = fact.optimizer_steps;
      update_fact.optimizer_update_count_bound = fact.optimizer_steps > 0;
      checkpoint_fact.producer_component_update_fact_digest =
          exposure::component_training_update_fact_digest(update_fact);
      const auto checkpoint_path =
          exposure::checkpoint_fact_path_for_job_dir(job_dir);
      exposure::write_component_training_update_fact_sidecar(
          exposure::component_training_update_fact_path_for_job_dir(job_dir),
          update_fact);
      exposure::write_checkpoint_fact_sidecar(checkpoint_path,
                                              std::move(checkpoint_fact));
      state->lattice_checkpoint_fact_written = true;
      state->lattice_checkpoint_fact_path = checkpoint_path.string();
    }
    exposure::write_channel_mdn_forecast_artifact_fact_sidecars(job_dir, fact);
    write_runtime_replay_observer_belief_fact_sidecar(job_dir, fact);
  } catch (const std::exception &ex) {
    state->lattice_fact_error = ex.what();
  }
}

[[nodiscard]] inline std::filesystem::path
runtime_root_for_job_dir(const std::filesystem::path &job_dir) {
  if (job_dir.empty()) {
    return default_job_root_for_config({});
  }
  const auto parent = job_dir.parent_path();
  const auto fallback =
      parent.empty() ? default_job_root_for_config({}) : parent;
  return job_layout::runtime_root_for_job_dir(job_dir, fallback);
}

[[nodiscard]] inline std::string
component_assembly_fingerprint_for_manifest(const job_manifest_t &manifest) {
  if (manifest.target_component_family_id ==
      "wikimyei.representation.encoding.vicreg") {
    return manifest.vicreg_assembly_fingerprint;
  }
  if (manifest.target_component_family_id ==
      "wikimyei.representation.encoding.mtf_jepa_mae_vicreg") {
    return manifest.mtf_jepa_mae_vicreg_assembly_fingerprint;
  }
  if (manifest.target_component_family_id ==
      "wikimyei.inference.expected_value.mdn") {
    return manifest.mdn_assembly_fingerprint;
  }
  if (manifest.target_component_family_id ==
      "wikimyei.policy.portfolio.graph_node_allocation") {
    if (!manifest.policy_operator_surface_digest.empty()) {
      return manifest.policy_operator_surface_digest;
    }
    if (!manifest.component_spawn_fingerprint.empty()) {
      return manifest.component_spawn_fingerprint;
    }
  }
  return {};
}

inline void populate_manifest_component_spawn_identity(
    const std::filesystem::path &runtime_root, job_manifest_t *manifest) {
  if (manifest == nullptr) {
    return;
  }
  namespace provenance = cuwacunu::kikijyeba::protocol::config_provenance;
  manifest->component_family_id = manifest->target_component_family_id;
  manifest->component_spawn_schema = "kikijyeba.component_spawn.v2";
  manifest->component_spawn_registry_id =
      provenance::component_spawn_registry_id_for_runtime_root(runtime_root);
  provenance::component_spawn_tuple_t spawn{};
  spawn.component_family_id = manifest->component_family_id;
  spawn.protocol_contract_fingerprint = manifest->protocol_contract_fingerprint;
  spawn.graph_order_fingerprint = manifest->graph_order_fingerprint;
  spawn.component_assembly_fingerprint =
      component_assembly_fingerprint_for_manifest(*manifest);
  manifest->component_spawn_fingerprint =
      provenance::component_spawn_fingerprint(spawn);

  try {
    provenance::component_spawn_registry_entry_t entry{};
    entry.component_family_id = manifest->component_family_id;
    entry.component_spawn_fingerprint = manifest->component_spawn_fingerprint;
    entry.config_bundle_id = manifest->config_bundle_id;
    entry.config_receipt_id = manifest->config_receipt_id;
    const auto registered = provenance::upsert_component_spawn_registry_entry(
        runtime_root, std::move(entry));
    manifest->component_spawn_id = registered.component_spawn_id;
    manifest->component_spawn_label = registered.component_spawn_label;
  } catch (...) {
    manifest->component_spawn_id.clear();
    manifest->component_spawn_label.clear();
  }
}

inline void
populate_manifest_config_provenance(const std::filesystem::path &runtime_root,
                                    const std::filesystem::path &job_dir,
                                    job_manifest_t *manifest) {
  if (manifest == nullptr || manifest->config_path.empty()) {
    return;
  }
  namespace provenance = cuwacunu::kikijyeba::protocol::config_provenance;
  try {
    const auto receipt = provenance::capture_config_bundle_receipt(
        manifest->config_path, manifest->job_id + "|" + job_dir.string());
    manifest->config_bundle_id = receipt.config_bundle_id;
    manifest->config_receipt_id = receipt.config_receipt_id;
  } catch (...) {
    return;
  }

  if (manifest->component_spawn_fingerprint.empty() ||
      manifest->component_spawn_id.empty()) {
    populate_manifest_component_spawn_identity(runtime_root, manifest);
  }

  try {
    provenance::component_spawn_registry_entry_t entry{};
    entry.component_family_id = manifest->component_family_id;
    entry.component_spawn_fingerprint = manifest->component_spawn_fingerprint;
    entry.config_bundle_id = manifest->config_bundle_id;
    entry.config_receipt_id = manifest->config_receipt_id;
    const auto registered = provenance::upsert_component_spawn_registry_entry(
        runtime_root, std::move(entry));
    manifest->component_spawn_id = registered.component_spawn_id;
    manifest->component_spawn_label = registered.component_spawn_label;
  } catch (...) {
    manifest->component_spawn_id.clear();
    manifest->component_spawn_label.clear();
  }
}

} // namespace job_runner_detail

template <typename DatatypeT> class job_runner_t {
public:
  using channel_builder_t =
      cuwacunu::kikijyeba::protocol::channel_graph_first_pipeline_builder_t<
          DatatypeT>;

  explicit job_runner_t(std::string config_path = {},
                        job_runner_options_t options = {})
      : config_path_(std::move(config_path)), options_(std::move(options)) {}

  [[nodiscard]] job_run_result_t run() const {
    if (options_.resume) {
      throw std::runtime_error(
          "[kikijyeba_job_runner] resume is not implemented in v1");
    }
    if (!options_.job_dir.empty() &&
        job_runner_detail::job_dir_has_runtime_artifacts(options_.job_dir)) {
      throw std::runtime_error(
          "[kikijyeba_job_runner] refusing to overwrite existing job_dir: " +
          options_.job_dir.string());
    }

    const auto wave_settings =
        cuwacunu::kikijyeba::protocol::load_wave_settings_from_config(
            config_path_);
    auto effective_wave_settings = wave_settings;
    job_runner_detail::apply_source_range_override(&effective_wave_settings,
                                                   options_);
    const auto resolved_job_kind =
        options_.job_kind_from_target
            ? job_runner_detail::runtime_job_kind_from_target(
                  effective_wave_settings.target)
            : options_.job_kind;

    switch (resolved_job_kind) {
    case runtime_job_kind_t::channel_representation_vicreg:
    case runtime_job_kind_t::channel_representation_mtf_jepa_mae_vicreg:
    case runtime_job_kind_t::channel_inference_mdn:
      return run_channel_graph_first(resolved_job_kind);
    case runtime_job_kind_t::policy_training:
      throw std::runtime_error(
          "[kikijyeba_job_runner] policy_training is handled by Runtime Hero "
          "as operation=wave with policy-training contract fields; direct "
          "graph job_runner execution is not supported");
    }
    throw std::runtime_error("[kikijyeba_job_runner] unknown runtime job kind");
  }

private:
  [[nodiscard]] job_run_result_t
  run_channel_graph_first(runtime_job_kind_t resolved_job_kind) const {
    auto bundle = cuwacunu::kikijyeba::protocol::
        load_channel_graph_first_protocol_contract_from_config(config_path_);
    job_runner_detail::apply_source_range_override(&bundle.wave_settings,
                                                   options_);
    job_runner_detail::apply_model_state_input_overrides(&bundle, options_);
    cuwacunu::kikijyeba::protocol::validate_channel_graph_first_config_bundle(
        bundle);
    cuwacunu::kikijyeba::protocol::graph_first_pipeline_builder_options_t
        builder_options{};
    builder_options.dry_run = false;
    builder_options.force_rebuild_cache = options_.force_rebuild_cache;
    builder_options.batch_size = options_.batch_size;
    builder_options.compute_alignment_diagnostics =
        options_.compute_alignment_diagnostics;
    builder_options.display_mdn_model = options_.display_mdn_model;
    builder_options.runtime_report_mode = options_.runtime_report_mode;

    channel_builder_t builder(bundle, builder_options);
    auto graph_source = builder.make_graph_source();
    auto wave_plan = cuwacunu::hero::runtime::make_wave_plan(
        bundle.wave_settings, graph_source);
    const auto train_target = job_runner_detail::train_target_from_wave_action(
        bundle.wave_settings.action);
    auto manifest = make_job_manifest(builder, wave_plan, resolved_job_kind,
                                      options_.job_id);
    manifest.runtime_handoff_id = options_.runtime_handoff_id;
    manifest.runtime_handoff_digest = options_.runtime_handoff_digest;
    manifest.marshal_target_driver_run_id =
        options_.marshal_target_driver_run_id;
    auto probe_stream_config =
        job_events_probe::job_events_probe_stream_config_t{};
    const auto probe_catalog =
        probe_settings::load_runtime_probe_catalog_from_config(config_path_);
    const bool probe_catalog_requests_job_events =
        probe_settings::select_enabled_job_events_probe(probe_catalog,
                                                        &probe_stream_config);
    const bool probe_records_requested =
        options_.write_probe_records || probe_catalog_requests_job_events;
    if (probe_records_requested && !bundle.wave_settings.debug) {
      throw std::runtime_error(
          "[kikijyeba_job_runner] Runtime probes require active "
          "WAVE_SETTINGS.MODE to include debug");
    }
    manifest.probe_sidecar_enabled = probe_records_requested;
    manifest.probe_record_schema = probe_stream_config.record_schema;
    manifest.probe_stream_leaf = probe_stream_config.stream_leaf;
    const auto runtime_root =
        options_.job_dir.empty()
            ? job_runner_detail::default_job_root_for_config(config_path_)
            : job_runner_detail::runtime_root_for_job_dir(options_.job_dir);
    job_layout::write_runtime_layout_marker(runtime_root);
    job_runner_detail::populate_manifest_component_spawn_identity(runtime_root,
                                                                  &manifest);
    auto job_dir = options_.job_dir;
    if (job_dir.empty()) {
      job_dir = job_runner_detail::reserve_immutable_attempt_job_dir(
          runtime_root, &manifest);
    } else {
      const auto probe_stream_path =
          job_events_probe::probe_stream_path_for_job_dir(job_dir,
                                                          probe_stream_config);
      if (manifest.probe_sidecar_enabled &&
          std::filesystem::exists(probe_stream_path)) {
        throw std::runtime_error(
            "[kikijyeba_job_runner] refusing to overwrite existing job_dir: " +
            job_dir.string());
      }
      job_runner_detail::prepare_explicit_job_dir(job_dir, &manifest);
    }
    job_runner_detail::ensure_job_dir(job_dir);
    job_runner_detail::populate_manifest_config_provenance(runtime_root,
                                                           job_dir, &manifest);
    job_layout::write_component_spawn_ref(
        runtime_root, manifest.component_family_id, manifest.component_spawn_id,
        manifest.component_spawn_label, manifest.component_spawn_fingerprint,
        manifest.component_spawn_registry_id, manifest.config_bundle_id,
        manifest.config_receipt_id);

    job_run_result_t result{};
    result.manifest = manifest;
    result.wave_plan = wave_plan;
    result.manifest_path =
        job_runner_detail::manifest_path_for_job_dir(job_dir);
    result.state_path = job_runner_detail::state_path_for_job_dir(job_dir);
    result.delegated_report_path =
        job_runner_detail::delegated_report_path_for_job(job_dir,
                                                         resolved_job_kind);
    write_job_manifest_file(result.manifest_path, manifest);
    auto probe_summary = job_events_probe::make_write_summary(
        manifest.probe_sidecar_enabled, job_dir, probe_stream_config);
    job_events_probe::write_job_started_event(manifest, &probe_summary);

    try {
      if (resolved_job_kind == runtime_job_kind_t::channel_inference_mdn &&
          !train_target && !options_.dry_run &&
          bundle.channel_mdn_training.input_mdn_checkpoint_path.empty()) {
        throw std::runtime_error(
            "[kikijyeba_job_runner] Channel MDN run/eval requires "
            "INPUT_MDN_CHECKPOINT");
      }
      job_events_probe::write_job_progress_event(
          manifest, &probe_summary, "delegate_start", "running",
          runtime_job_kind_name(resolved_job_kind));
      result.state = run_channel_delegate(
          std::move(builder), manifest, wave_plan, result.delegated_report_path,
          job_dir, resolved_job_kind, train_target, &probe_summary);
      job_events_probe::write_job_progress_event(
          manifest, &probe_summary, "delegate_complete", "completed",
          runtime_job_kind_name(resolved_job_kind));
      write_job_state_file(result.state_path, result.state);
      terminal_facts::write_terminal_fact_sidecars(job_dir, manifest,
                                                   &result.state);
      job_runner_detail::write_lattice_fact_sidecars(job_dir, &result.state);
      job_events_probe::write_job_final_events(job_dir, manifest, result.state,
                                               &probe_summary);
      job_events_probe::apply_summary_to_state(probe_summary, &result.state);
      write_job_state_file(result.state_path, result.state);
      return result;
    } catch (const std::exception &ex) {
      result.state = make_failed_job_state(manifest, wave_plan, ex.what());
      terminal_facts::write_terminal_fact_sidecars(job_dir, manifest,
                                                   &result.state);
      job_events_probe::write_job_final_events(job_dir, manifest, result.state,
                                               &probe_summary);
      job_events_probe::apply_summary_to_state(probe_summary, &result.state);
      write_job_state_file(result.state_path, result.state);
      throw;
    }
  }

  [[nodiscard]] job_state_t run_channel_delegate(
      channel_builder_t builder, const job_manifest_t &manifest,
      const wave_plan_t &wave_plan,
      const std::filesystem::path &delegated_report_path,
      const std::filesystem::path &job_dir, runtime_job_kind_t job_kind,
      bool train_target,
      job_events_probe::job_events_probe_write_summary_t *probe_summary) const {
    const auto probe_snapshot_fallback_state =
        make_initial_job_state(manifest, wave_plan);
    switch (job_kind) {
    case runtime_job_kind_t::channel_representation_vicreg: {
      cuwacunu::jkimyei::training::representation::
          channel_graph_first_representation_launcher_options_t
              launcher_options{};
      launcher_options.dry_run = options_.dry_run;
      launcher_options.train_target_from_wave = false;
      launcher_options.train_target = train_target;
      launcher_options.write_report = options_.write_report;
      launcher_options.report_path = delegated_report_path;
      launcher_options.runtime_report_mode = options_.runtime_report_mode;
      if (probe_summary != nullptr && probe_summary->enabled &&
          probe_summary->config.emit_report_metrics) {
        launcher_options.learning_probe_report_sink =
            [&manifest, &probe_snapshot_fallback_state,
             probe_summary](std::string_view report_text) {
              job_events_probe::write_learning_report_snapshot_events(
                  manifest, probe_snapshot_fallback_state, probe_summary,
                  report_text);
            };
      }
      cuwacunu::jkimyei::training::representation::
          channel_graph_first_representation_launcher_t<DatatypeT>
              launcher(std::move(builder), launcher_options);
      const auto report = launcher.run();
      return make_completed_job_state(manifest, wave_plan, report,
                                      options_.dry_run);
    }
    case runtime_job_kind_t::channel_representation_mtf_jepa_mae_vicreg: {
      cuwacunu::jkimyei::training::representation::
          mtf_jepa_mae_vicreg_graph_first_launcher_options_t launcher_options{};
      launcher_options.dry_run = options_.dry_run;
      launcher_options.train_target_from_wave = false;
      launcher_options.train_target = train_target;
      launcher_options.write_report = options_.write_report;
      launcher_options.report_path = delegated_report_path;
      launcher_options.runtime_report_mode = options_.runtime_report_mode;
      if (probe_summary != nullptr && probe_summary->enabled &&
          probe_summary->config.emit_report_metrics) {
        launcher_options.learning_probe_report_sink =
            [&manifest, &probe_snapshot_fallback_state,
             probe_summary](std::string_view report_text) {
              job_events_probe::write_learning_report_snapshot_events(
                  manifest, probe_snapshot_fallback_state, probe_summary,
                  report_text);
            };
      }
      cuwacunu::jkimyei::training::representation::
          mtf_jepa_mae_vicreg_graph_first_launcher_t<DatatypeT>
              launcher(std::move(builder), launcher_options);
      const auto report = launcher.run();
      return make_completed_job_state(manifest, wave_plan, report,
                                      options_.dry_run);
    }
    case runtime_job_kind_t::channel_inference_mdn: {
      cuwacunu::jkimyei::training::inference::
          channel_graph_first_inference_launcher_options_t launcher_options{};
      launcher_options.dry_run = options_.dry_run;
      launcher_options.train_target_from_wave = false;
      launcher_options.train_target = train_target;
      launcher_options.write_report = options_.write_report;
      launcher_options.report_path = delegated_report_path;
      launcher_options.runtime_report_mode = options_.runtime_report_mode;
      if (probe_summary != nullptr && probe_summary->enabled &&
          probe_summary->config.emit_report_metrics) {
        launcher_options.learning_probe_report_sink =
            [&manifest, &probe_snapshot_fallback_state,
             probe_summary](std::string_view report_text) {
              job_events_probe::write_learning_report_snapshot_events(
                  manifest, probe_snapshot_fallback_state, probe_summary,
                  report_text);
            };
      }
      if (probe_summary != nullptr && probe_summary->enabled &&
          probe_summary->config.emit_artifacts) {
        launcher_options.representation_edge_feature_probe_path =
            job_dir / "representation_edge_features.probe";
        launcher_options.mdn_edge_context_feature_probe_path =
            job_dir / "mdn_edge_context_features.probe";
      }
      namespace replay = cuwacunu::kikijyeba::environment::replay;
      using launcher_t = cuwacunu::jkimyei::training::inference::
          channel_graph_first_inference_launcher_t<DatatypeT>;
      typename launcher_t::inference_batch_observer_t
          replay_artifact_observer{};
      bool replay_artifacts_written = false;
      std::string replay_artifact_error{};
      replay::runtime_replay_artifact_paths_t last_replay_artifact_paths{};
      if (options_.write_replay_artifacts && !options_.dry_run &&
          !train_target) {
        try {
          replay::runtime_allocation_belief_observer_options_t
              observer_options{};
          observer_options.accounting_numeraire_node_id = cuwacunu::kikijyeba::
              protocol::resolve_accounting_numeraire_node_id(
                  options_.replay_accounting_numeraire_node_id, config_path_);
          observer_options.target_node_ids = options_.replay_target_node_ids;
          observer_options.require_direct_accounting_numeraire_valuation_edges =
              options_
                  .replay_require_direct_accounting_numeraire_valuation_edges;
          auto belief_options =
              replay::make_runtime_allocation_belief_builder_options(
                  builder.market_graph(), builder.bundle().belief_observer,
                  std::move(observer_options));
          const auto model_version = manifest.mdn_assembly_fingerprint.empty()
                                         ? manifest.inference_training_id
                                         : manifest.mdn_assembly_fingerprint;
          const auto normalization_fingerprint =
              manifest.dock_binding_fingerprint;
          replay_artifact_observer = [job_dir, delegated_report_path, manifest,
                                      belief_options, model_version,
                                      normalization_fingerprint,
                                      &replay_artifacts_written,
                                      &replay_artifact_error,
                                      &last_replay_artifact_paths](
                                         const auto &out, const auto &batch,
                                         const auto &representation_batch,
                                         int64_t wave_pulse_index) {
            if (!replay_artifact_error.empty()) {
              return;
            }
            try {
              if (wave_pulse_index <= 0) {
                throw std::runtime_error(
                    "[kikijyeba_job_runner] replay artifact observer "
                    "received invalid wave pulse index");
              }
              const auto pulse_index =
                  static_cast<std::size_t>(wave_pulse_index);
              auto mdn_artifact =
                  cuwacunu::kikijyeba::environment::artifact_ref_t{
                      .artifact_id = manifest.job_id + ".mdn.wave_pulse." +
                                     std::to_string(wave_pulse_index),
                      .artifact_path = delegated_report_path.string(),
                      .artifact_digest = manifest.mdn_assembly_fingerprint,
                      .schema_id =
                          "wikimyei.inference.expected_value.mdn.MdnOut.v1",
                  };
              last_replay_artifact_paths = replay::
                  write_runtime_graph_anchor_replay_artifacts_from_mdn_batch_for_pulse(
                      job_dir, pulse_index, out, batch, representation_batch,
                      belief_options, std::move(mdn_artifact), model_version,
                      "", normalization_fingerprint);
              replay_artifacts_written = true;
            } catch (const std::exception &ex) {
              replay_artifact_error = ex.what();
            }
          };
        } catch (const std::exception &ex) {
          replay_artifact_error = ex.what();
        }
      }
      cuwacunu::jkimyei::training::inference::
          channel_graph_first_inference_launcher_t<DatatypeT>
              launcher(std::move(builder), launcher_options,
                       std::move(replay_artifact_observer));
      const auto report = launcher.run();
      auto state = make_completed_job_state(manifest, wave_plan, report,
                                            options_.dry_run);
      state.representation_edge_feature_probe_written =
          report.representation_edge_feature_probe_written;
      state.representation_edge_feature_probe_path =
          report.representation_edge_feature_probe_path;
      state.representation_edge_feature_probe_row_count =
          report.representation_edge_feature_probe_row_count;
      state.representation_edge_feature_probe_error =
          report.representation_edge_feature_probe_error;
      state.mdn_edge_context_feature_probe_written =
          report.mdn_edge_context_feature_probe_written;
      state.mdn_edge_context_feature_probe_path =
          report.mdn_edge_context_feature_probe_path;
      state.mdn_edge_context_feature_probe_row_count =
          report.mdn_edge_context_feature_probe_row_count;
      state.mdn_edge_context_feature_probe_error =
          report.mdn_edge_context_feature_probe_error;
      if (replay_artifacts_written) {
        state.replay_artifacts_written = true;
        state.replay_batch_index_path =
            replay::runtime_replay_batch_index_path(job_dir).string();
        state.replay_artifact_path_index_path =
            (last_replay_artifact_paths.observation_artifact_index_path
                 .parent_path() /
             "runtime_replay_artifacts.index")
                .string();
        state.replay_graph_anchor_edge_batch_artifact_path =
            last_replay_artifact_paths.graph_anchor_edge_batch_artifact_path
                .string();
        state.replay_observation_artifact_index_path =
            last_replay_artifact_paths.observation_artifact_index_path.string();
      }
      state.replay_artifact_error = replay_artifact_error;
      return state;
    }
    case runtime_job_kind_t::policy_training:
      throw std::runtime_error(
          "[kikijyeba_job_runner] policy_training execution is not available "
          "through direct graph job_runner execution; use Runtime Hero "
          "operation=wave with the bounded policy-training contract");
    }
    throw std::runtime_error("[kikijyeba_job_runner] unknown runtime job kind");
  }

  std::string config_path_{};
  job_runner_options_t options_{};
};

template <typename DatatypeT>
[[nodiscard]] inline job_run_result_t
run_graph_first_job(std::string config_path,
                    job_runner_options_t options = {}) {
  return job_runner_t<DatatypeT>(std::move(config_path), std::move(options))
      .run();
}

} // namespace cuwacunu::hero::runtime
