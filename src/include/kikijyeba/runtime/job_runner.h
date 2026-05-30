// SPDX-License-Identifier: MIT
#pragma once

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "jkimyei/training/inference/channel_graph_first_inference_launcher.h"
#include "jkimyei/training/representation/channel_graph_first_representation_launcher.h"
#include "jkimyei/training/representation/mtf_jepa_mae_vicreg_graph_first_launcher.h"
#include "kikijyeba/lattice/exposure/exposure_ledger.h"
#include "kikijyeba/protocol/config_bundle.h"
#include "kikijyeba/protocol/config_provenance.h"
#include "kikijyeba/protocol/pipeline_builder.h"
#include "kikijyeba/runtime/job_layout.h"
#include "kikijyeba/runtime/job_manifest.h"
#include "kikijyeba/runtime/job_state.h"
#include "kikijyeba/runtime/terminal_facts.h"
#include "kikijyeba/runtime/wave_plan.h"

namespace cuwacunu::kikijyeba::runtime {

struct job_runner_options_t {
  runtime_job_kind_t job_kind{runtime_job_kind_t::channel_inference_mdn};
  bool job_kind_from_target{true};
  bool dry_run{false};
  bool resume{false};
  bool force_rebuild_cache{false};
  bool compute_alignment_diagnostics{true};
  bool display_mdn_model{false};
  bool write_report{true};
  std::size_t batch_size{0};
  std::string job_id{};
  std::string runtime_handoff_id{};
  std::string runtime_handoff_digest{};
  std::string marshal_target_driver_run_id{};
  bool source_range_override_enabled{false};
  std::string source_range_override{};
  std::optional<std::size_t> anchor_index_begin_override{std::nullopt};
  std::optional<std::size_t> anchor_index_end_override{std::nullopt};
  std::optional<std::int64_t> source_key_begin_override{std::nullopt};
  std::optional<std::int64_t> source_key_end_override{std::nullopt};
  std::filesystem::path job_dir{};
  cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t
      runtime_report_mode{cuwacunu::kikijyeba::lattice::runtime_report::
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
  (void)config_path;
  return std::filesystem::path("/cuwacunu/.runtime/cuwacunu_exec");
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
    cuwacunu::kikijyeba::settings::wave_target_t target) {
  switch (target) {
  case cuwacunu::kikijyeba::settings::wave_target_t::vicreg_representation:
    return runtime_job_kind_t::channel_representation_vicreg;
  case cuwacunu::kikijyeba::settings::wave_target_t::
      mtf_jepa_mae_vicreg_representation:
    return runtime_job_kind_t::channel_representation_mtf_jepa_mae_vicreg;
  case cuwacunu::kikijyeba::settings::wave_target_t::inference_channel_mdn:
    return runtime_job_kind_t::channel_inference_mdn;
  }
  throw std::runtime_error("[kikijyeba_job_runner] unknown wave target");
}

[[nodiscard]] inline bool train_target_from_wave_action(
    cuwacunu::kikijyeba::settings::wave_action_t action) {
  switch (action) {
  case cuwacunu::kikijyeba::settings::wave_action_t::run:
    return false;
  case cuwacunu::kikijyeba::settings::wave_action_t::train:
    return true;
  }
  throw std::runtime_error("[kikijyeba_job_runner] unknown wave action");
}

inline void apply_source_range_override(
    cuwacunu::kikijyeba::settings::wave_settings_t *settings,
    const job_runner_options_t &options) {
  if (settings == nullptr || !options.source_range_override_enabled) {
    return;
  }
  settings->source_range_policy =
      cuwacunu::kikijyeba::settings::parse_source_range_policy(
          options.source_range_override);
  settings->anchor_index_begin = options.anchor_index_begin_override;
  settings->anchor_index_end = options.anchor_index_end_override;
  settings->source_key_begin = options.source_key_begin_override;
  settings->source_key_end = options.source_key_end_override;
  cuwacunu::kikijyeba::settings::validate_wave_settings(*settings);
}

inline void write_lattice_fact_sidecars(const std::filesystem::path &job_dir,
                                        job_state_t *state) {
  if (state == nullptr) {
    return;
  }
  namespace exposure = cuwacunu::kikijyeba::lattice::exposure;
  try {
    const auto fact = exposure::make_exposure_fact_from_job_dir(job_dir);
    const auto exposure_path =
        exposure::exposure_fact_path_for_job_dir(job_dir);
    exposure::write_exposure_fact_sidecar(exposure_path, fact);
    state->lattice_exposure_fact_written = true;
    state->lattice_exposure_fact_path = exposure_path.string();
    if (!fact.output_checkpoint.empty() &&
        std::filesystem::exists(fact.output_checkpoint)) {
      auto checkpoint_fact =
          exposure::make_checkpoint_fact_from_exposure_fact(fact);
      const auto checkpoint_path =
          exposure::checkpoint_fact_path_for_job_dir(job_dir);
      exposure::write_checkpoint_fact_sidecar(checkpoint_path,
                                              std::move(checkpoint_fact));
      state->lattice_checkpoint_fact_written = true;
      state->lattice_checkpoint_fact_path = checkpoint_path.string();
    }
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
    auto wave_plan = cuwacunu::kikijyeba::runtime::make_wave_plan(
        bundle.wave_settings, graph_source);
    const auto train_target = job_runner_detail::train_target_from_wave_action(
        bundle.wave_settings.action);
    auto manifest = make_job_manifest(builder, wave_plan, resolved_job_kind,
                                      options_.job_id);
    manifest.runtime_handoff_id = options_.runtime_handoff_id;
    manifest.runtime_handoff_digest = options_.runtime_handoff_digest;
    manifest.marshal_target_driver_run_id =
        options_.marshal_target_driver_run_id;
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

    try {
      if (resolved_job_kind == runtime_job_kind_t::channel_inference_mdn &&
          !train_target && !options_.dry_run &&
          bundle.channel_mdn_training.input_mdn_checkpoint_path.empty()) {
        throw std::runtime_error(
            "[kikijyeba_job_runner] Channel MDN run/eval requires "
            "INPUT_MDN_CHECKPOINT");
      }
      result.state = run_channel_delegate(
          std::move(builder), manifest, wave_plan, result.delegated_report_path,
          resolved_job_kind, train_target);
      write_job_state_file(result.state_path, result.state);
      terminal_facts::write_terminal_fact_sidecars(job_dir, manifest,
                                                   &result.state);
      job_runner_detail::write_lattice_fact_sidecars(job_dir, &result.state);
      write_job_state_file(result.state_path, result.state);
      return result;
    } catch (const std::exception &ex) {
      result.state = make_failed_job_state(manifest, wave_plan, ex.what());
      terminal_facts::write_terminal_fact_sidecars(job_dir, manifest,
                                                   &result.state);
      write_job_state_file(result.state_path, result.state);
      throw;
    }
  }

  [[nodiscard]] job_state_t
  run_channel_delegate(channel_builder_t builder,
                       const job_manifest_t &manifest,
                       const wave_plan_t &wave_plan,
                       const std::filesystem::path &delegated_report_path,
                       runtime_job_kind_t job_kind, bool train_target) const {
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
      cuwacunu::jkimyei::training::inference::
          channel_graph_first_inference_launcher_t<DatatypeT>
              launcher(std::move(builder), launcher_options);
      const auto report = launcher.run();
      return make_completed_job_state(manifest, wave_plan, report,
                                      options_.dry_run);
    }
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

} // namespace cuwacunu::kikijyeba::runtime
