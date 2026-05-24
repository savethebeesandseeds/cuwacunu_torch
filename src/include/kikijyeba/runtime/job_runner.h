// SPDX-License-Identifier: MIT
#pragma once

#include <cctype>
#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

#include "jkimyei/training/inference/channel_graph_first_inference_launcher.h"
#include "jkimyei/training/representation/channel_graph_first_representation_launcher.h"
#include "kikijyeba/lattice/exposure/exposure_ledger.h"
#include "kikijyeba/protocol/config_bundle.h"
#include "kikijyeba/protocol/config_provenance.h"
#include "kikijyeba/protocol/pipeline_builder.h"
#include "kikijyeba/runtime/job_manifest.h"
#include "kikijyeba/runtime/job_state.h"
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
  std::filesystem::path config = config_path.empty()
                                     ? std::filesystem::current_path()
                                     : std::filesystem::absolute(config_path);
  if (config.filename() == ".config") {
    config = config.parent_path();
  }
  if (config.filename() == "config" &&
      config.parent_path().filename() == "src") {
    return config.parent_path().parent_path() / ".runtime" / "cuwacunu_exec";
  }
  return std::filesystem::current_path() / ".runtime" / "cuwacunu_exec";
}

[[nodiscard]] inline std::filesystem::path
default_job_dir_for_manifest(const std::string &config_path,
                             const job_manifest_t &manifest) {
  return default_job_root_for_config(config_path) /
         sanitize_job_dir_leaf(manifest.job_id);
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
    return job_dir / "channel_representation.report";
  case runtime_job_kind_t::channel_inference_mdn:
    return job_dir / "channel_inference.report";
  }
  return job_dir / "job.report";
}

[[nodiscard]] inline runtime_job_kind_t runtime_job_kind_from_target(
    cuwacunu::kikijyeba::settings::wave_target_t target) {
  switch (target) {
  case cuwacunu::kikijyeba::settings::wave_target_t::vicreg_representation:
    return runtime_job_kind_t::channel_representation_vicreg;
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
    return std::filesystem::current_path() / ".runtime" / "cuwacunu_exec";
  }
  const auto parent = job_dir.parent_path();
  return parent.empty() ? std::filesystem::current_path() : parent;
}

[[nodiscard]] inline std::string
component_assembly_fingerprint_for_manifest(const job_manifest_t &manifest) {
  if (manifest.target_component_family_id ==
      "wikimyei.representation.encoding.vicreg") {
    return manifest.vicreg_assembly_fingerprint;
  }
  if (manifest.target_component_family_id ==
      "wikimyei.inference.expected_value.mdn") {
    return manifest.mdn_assembly_fingerprint;
  }
  return {};
}

inline void
populate_manifest_config_provenance(const std::filesystem::path &job_dir,
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

  manifest->component_family_id = manifest->target_component_family_id;
  manifest->component_spawn_schema = "kikijyeba.component_spawn.v1";
  const auto runtime_root = runtime_root_for_job_dir(job_dir);
  manifest->component_spawn_registry_id =
      provenance::component_spawn_registry_id_for_runtime_root(runtime_root);
  provenance::component_spawn_tuple_t spawn{};
  spawn.component_family_id = manifest->component_family_id;
  spawn.protocol_contract_fingerprint = manifest->protocol_contract_fingerprint;
  spawn.graph_order_fingerprint = manifest->graph_order_fingerprint;
  spawn.source_cursor_token = manifest->source_cursor_token;
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

    const auto wave_settings =
        cuwacunu::kikijyeba::protocol::load_wave_settings_from_config(
            config_path_);
    const auto resolved_job_kind =
        options_.job_kind_from_target
            ? job_runner_detail::runtime_job_kind_from_target(
                  wave_settings.target)
            : options_.job_kind;

    switch (resolved_job_kind) {
    case runtime_job_kind_t::channel_representation_vicreg:
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
    auto job_dir = options_.job_dir.empty()
                       ? job_runner_detail::default_job_dir_for_manifest(
                             config_path_, manifest)
                       : options_.job_dir;
    job_runner_detail::ensure_job_dir(job_dir);
    job_runner_detail::populate_manifest_config_provenance(job_dir, &manifest);

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
      job_runner_detail::write_lattice_fact_sidecars(job_dir, &result.state);
      write_job_state_file(result.state_path, result.state);
      return result;
    } catch (const std::exception &ex) {
      result.state = make_failed_job_state(manifest, wave_plan, ex.what());
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
