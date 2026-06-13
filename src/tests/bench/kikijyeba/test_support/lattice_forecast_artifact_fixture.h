// SPDX-License-Identifier: MIT
#pragma once

#include "hero/lattice_hero/lattice/exposure/exposure_ledger.h"
#include "hero/lattice_hero/lattice/split/split_policy.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace cuwacunu::tests::kikijyeba::lattice_fixture {

namespace exposure = cuwacunu::hero::lattice::exposure;
namespace split = cuwacunu::hero::lattice::split;

struct scanned_forecast_artifact_fixture_options_t {
  bool forecast_baseline_digest_mismatch{false};
  bool load_split_policy{true};
  std::filesystem::path split_policy_path{
      "/cuwacunu/src/config/hero.lattice.splits.dsl"};
  std::string split_id{"validation_holdout"};

  std::string contract_fingerprint{"contract_1"};
  std::string protocol_id{"cwu_02v"};
  std::string graph_order_fingerprint{"graph_1"};
  std::string source_cursor_token{"cursor_1"};
  std::string component_assembly_fingerprint{"mdn_1"};
  std::string target_component_family_id{
      "wikimyei.inference.expected_value.mdn"};
  std::string job_id{"artifact_job"};
  std::string wave_id{"artifact_wave"};
  std::string wave_action{"evaluate"};
  std::string job_status{"completed"};

  std::string cursor_domain{"ujcamei.graph_anchor"};
  std::string split_policy_fingerprint{"split_policy_1"};
  std::string split_name{"train_core"};
  exposure::exposure_split_role_t split_role{
      exposure::exposure_split_role_t::train};
  exposure::anchor_interval_t anchor_range{0, 10};
  std::string normalization_contract{};
};

[[nodiscard]] inline scanned_forecast_artifact_fixture_options_t
validation_holdout_forecast_artifact_options(
    bool forecast_baseline_digest_mismatch = false) {
  scanned_forecast_artifact_fixture_options_t options{};
  options.forecast_baseline_digest_mismatch = forecast_baseline_digest_mismatch;
  options.load_split_policy = true;
  options.split_id = "validation_holdout";
  return options;
}

[[nodiscard]] inline scanned_forecast_artifact_fixture_options_t
anchor_range_forecast_artifact_options(
    bool forecast_baseline_digest_mismatch = false) {
  scanned_forecast_artifact_fixture_options_t options{};
  options.forecast_baseline_digest_mismatch = forecast_baseline_digest_mismatch;
  options.load_split_policy = false;
  options.cursor_domain = "ujcamei.graph_anchor";
  options.split_policy_fingerprint = "split_policy_1";
  options.split_name = "train_core";
  options.split_role = exposure::exposure_split_role_t::train;
  options.anchor_range = exposure::anchor_interval_t{0, 10};
  return options;
}

inline void write_fixture_text(const std::filesystem::path &path,
                               const std::string &text) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error("failed to write fixture: " + path.string());
  }
  out << text;
}

inline void apply_split_identity(
    exposure::lattice_exposure_fact_t &parent_seed,
    const scanned_forecast_artifact_fixture_options_t &options) {
  if (!options.load_split_policy) {
    parent_seed.cursor_domain = options.cursor_domain;
    parent_seed.split_policy_fingerprint = options.split_policy_fingerprint;
    parent_seed.split_name = options.split_name;
    parent_seed.split_role = options.split_role;
    parent_seed.anchor_range = options.anchor_range;
    return;
  }

  const auto policy =
      split::load_lattice_split_policy_from_file(options.split_policy_path);
  const auto *selected_split = policy.find_split(options.split_id);
  if (selected_split == nullptr) {
    throw std::runtime_error("missing lattice split: " + options.split_id);
  }
  parent_seed.cursor_domain = policy.cursor_domain;
  parent_seed.split_policy_fingerprint =
      split::lattice_split_policy_fingerprint(policy);
  parent_seed.split_name = selected_split->split_id;
  parent_seed.split_role = selected_split->role;
  parent_seed.anchor_range = selected_split->anchor_range;
}

inline void write_scanned_forecast_artifact_fixture(
    const std::filesystem::path &runtime_root,
    const scanned_forecast_artifact_fixture_options_t &options) {
  std::filesystem::remove_all(runtime_root);
  const auto job_dir = runtime_root / options.job_id;
  std::filesystem::create_directories(job_dir);
  std::ostringstream manifest;
  manifest << "job_id=" << options.job_id << "\n"
           << "job_kind=channel_inference_mdn\n"
           << "protocol_id=" << options.protocol_id << "\n"
           << "protocol_contract_fingerprint=" << options.contract_fingerprint
           << "\n"
           << "graph_order_fingerprint=" << options.graph_order_fingerprint
           << "\n"
           << "source_cursor_token=" << options.source_cursor_token << "\n"
           << "target_component_family_id="
           << options.target_component_family_id << "\n"
           << "component_family_id=" << options.target_component_family_id
           << "\n"
           << "mdn_assembly_fingerprint="
           << options.component_assembly_fingerprint << "\n"
           << "wave_id=" << options.wave_id << "\n"
           << "wave_action=" << options.wave_action << "\n";
  write_fixture_text(job_dir / "job.manifest", manifest.str());

  std::ostringstream state;
  state << "status=" << options.job_status << "\n"
        << "wave_id=" << options.wave_id << "\n"
        << "wave_action=" << options.wave_action << "\n"
        << "source_cursor_token=" << options.source_cursor_token << "\n";
  write_fixture_text(job_dir / "job.state", state.str());

  exposure::lattice_exposure_fact_t parent_seed{};
  parent_seed.fact_type = "exposure";
  parent_seed.contract_fingerprint = options.contract_fingerprint;
  parent_seed.protocol_id = options.protocol_id;
  parent_seed.graph_order_fingerprint = options.graph_order_fingerprint;
  parent_seed.source_cursor_token = options.source_cursor_token;
  parent_seed.component_assembly_fingerprint =
      options.component_assembly_fingerprint;
  parent_seed.target_component_family_id = options.target_component_family_id;
  parent_seed.job_id = options.job_id;
  parent_seed.wave_id = options.wave_id;
  parent_seed.wave_action = options.wave_action;
  parent_seed.job_status = options.job_status;
  apply_split_identity(parent_seed, options);
  parent_seed.completed_anchor_range = parent_seed.anchor_range;
  parent_seed.use.selection_signal = true;

  const auto exposure_sidecar =
      exposure::exposure_fact_path_for_job_dir(job_dir);
  exposure::write_exposure_fact_sidecar(exposure_sidecar, parent_seed);
  const auto parent =
      exposure::make_exposure_fact_from_sidecar_file(exposure_sidecar, job_dir);
  const auto selection_signal =
      exposure::make_selection_signal_fact_from_exposure_fact(parent);
  const auto selection_signal_digest =
      exposure::selection_signal_fact_digest(selection_signal);

  exposure::lattice_target_transform_fact_t transform{};
  exposure::populate_artifact_fact_identity(transform, parent);
  transform.target_feature_ids = {"ev_close", "ev_return"};
  transform.horizon = 3;
  transform.target_mode = "log_return";
  transform.normalization_contract = options.normalization_contract.empty()
                                         ? "zscore." + parent.split_name + ".v1"
                                         : options.normalization_contract;
  transform.inverse_transform_contract = "exp_return.v1";
  transform.units = "log_return";
  transform.target_mask_policy_digest = "mask_policy_1";
  transform.support_surface_identity = "BTC_ETH:h3";
  transform.support_surface_digest = "support_surface_1";
  const auto transform_digest =
      exposure::target_transform_fact_digest(transform);

  exposure::lattice_forecast_baseline_fact_t baseline{};
  exposure::populate_artifact_fact_identity(baseline, parent);
  baseline.target_feature_ids = transform.target_feature_ids;
  baseline.horizon = transform.horizon;
  baseline.baseline_kind = "previous_value";
  baseline.baseline_parameters = "lag=1";
  baseline.target_transform_fact_digest = transform_digest;
  baseline.support_count = 42;
  baseline.valid_count = 40;
  baseline.missing_count = 2;
  baseline.baseline_mean_nll = 1.50;
  baseline.baseline_ev_mae = 0.20;
  baseline.baseline_ev_rmse = 0.30;
  baseline.baseline_signed_error = -0.01;
  baseline.baseline_directional_accuracy = 0.55;
  baseline.metric_status = "computed";
  const auto baseline_digest =
      exposure::forecast_baseline_fact_digest(baseline);

  exposure::lattice_forecast_eval_fact_t forecast{};
  exposure::populate_artifact_fact_identity(forecast, parent);
  forecast.target_feature_ids = transform.target_feature_ids;
  forecast.horizon = transform.horizon;
  forecast.support_count = 42;
  forecast.valid_count = 40;
  forecast.missing_count = 2;
  forecast.weakest_support_rows = 8;
  forecast.mean_nll = 1.10;
  forecast.mean_nll_per_horizon = {1.10};
  forecast.valid_target_count_per_horizon = {40};
  forecast.ev_mae = 0.16;
  forecast.ev_rmse = 0.24;
  forecast.signed_error = -0.02;
  forecast.directional_accuracy = 0.62;
  forecast.calibration_coverage = 0.91;
  forecast.pit_summary = "uniform-ish";
  forecast.sigma_scale_sanity = "ok";
  forecast.support_by_node = "BTC:20,ETH:20";
  forecast.support_by_channel = "close:40";
  forecast.support_by_target_feature = "ev_close:40,ev_return:40";
  forecast.support_by_horizon = "h3:40";
  forecast.forecast_artifact_digest = "forecast_artifact_1";
  forecast.evaluated_representation_checkpoint_digest = "rep_checkpoint_1";
  forecast.evaluated_mdn_checkpoint_digest = "mdn_checkpoint_1";
  forecast.target_transform_fact_digest = transform_digest;
  forecast.baseline_fact_digests =
      options.forecast_baseline_digest_mismatch
          ? std::vector<std::string>{"missing_baseline_digest"}
          : std::vector<std::string>{baseline_digest};
  forecast.selection_signal_fact_digests = {selection_signal_digest};
  const auto forecast_eval_digest =
      exposure::forecast_eval_fact_digest(forecast);

  exposure::lattice_observer_belief_fact_t observer{};
  exposure::populate_artifact_fact_identity(observer, parent);
  observer.belief_kind = "raw_nodelift_potential";
  observer.channel_consensus = "aligned";
  observer.potential_surface_diagnostics = "smooth";
  observer.nodelift_return_projection = "projection_v1";
  observer.covariance_coupling = "diagonal_scenario_bank";
  observer.scenario_bank_digest = "scenario_bank_1";
  observer.nodelift_residual_quality = "acceptable";
  observer.projection_validation_scores = "mae=0.02";
  observer.confidence = 0.64;
  observer.data_quality = 0.88;
  observer.liquidity = 0.72;
  observer.forecast_artifact_digest = forecast.forecast_artifact_digest;
  observer.forecast_artifact_lineage = forecast_eval_digest;
  observer.feature_semantics_fingerprint = "feature_semantics_1";
  observer.dock_binding_fingerprint = "dock_binding_1";
  const auto observer_digest = exposure::observer_belief_fact_digest(observer);

  exposure::lattice_allocation_engine_fact_t allocation{};
  exposure::populate_artifact_fact_identity(allocation, parent);
  allocation.target_node_weights = "BTC:0.40,ETH:0.25";
  allocation.accounting_numeraire_node_id = "USD_CASH";
  allocation.accounting_numeraire_node_source = "base_policy";
  allocation.base_policy_accounting_numeraire_node_id = "USD_CASH";
  allocation.accounting_numeraire_node_graph_bound = true;
  allocation.numeraire_weight = 0.35;
  allocation.turnover = 0.12;
  allocation.objective_terms = "growth:0.70,cvar:0.20,cost:0.10";
  allocation.cvar_loss = 0.08;
  allocation.transaction_cost_estimate = 0.003;
  allocation.constraint_diagnostics = "all_constraints_satisfied";
  allocation.cap_diagnostics = "caps_clear";
  allocation.scenario_growth_floor_status = "satisfied";
  allocation.fallback_reasons = "none";
  allocation.derisk_reasons = "none";
  allocation.observer_belief_fact_digest = observer_digest;
  allocation.forecast_artifact_digest = forecast.forecast_artifact_digest;
  allocation.base_policy_digest = "base_policy_1";

  exposure::write_target_transform_fact_sidecar(
      job_dir / "lattice.target_transform.fact", transform);
  exposure::write_forecast_baseline_fact_sidecar(
      job_dir / "lattice.forecast_baseline.fact", baseline);
  exposure::write_forecast_eval_fact_sidecar(
      job_dir / "lattice.forecast_eval.fact", forecast);
  exposure::write_observer_belief_fact_sidecar(
      job_dir / "lattice.observer_belief.fact", observer);
  exposure::write_allocation_engine_fact_sidecar(
      job_dir / "lattice.allocation_engine.fact", allocation);
}

inline void write_scanned_forecast_artifact_fixture(
    const std::filesystem::path &runtime_root,
    bool forecast_baseline_digest_mismatch) {
  write_scanned_forecast_artifact_fixture(
      runtime_root, validation_holdout_forecast_artifact_options(
                        forecast_baseline_digest_mismatch));
}

} // namespace cuwacunu::tests::kikijyeba::lattice_fixture
