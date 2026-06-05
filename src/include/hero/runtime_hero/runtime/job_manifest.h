// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "kikijyeba/protocol/pipeline_builder.h"
#include "hero/runtime_hero/runtime/wave_plan.h"
#include "kikijyeba/topology/dock_binding.h"
#include "wikimyei/assembly.h"

namespace cuwacunu::hero::runtime {

enum class runtime_job_kind_t {
  channel_representation_vicreg,
  channel_representation_mtf_jepa_mae_vicreg,
  channel_inference_mdn,
  policy_training,
};

[[nodiscard]] inline const char *
runtime_job_kind_name(runtime_job_kind_t kind) {
  switch (kind) {
  case runtime_job_kind_t::channel_representation_vicreg:
    return "channel_representation_vicreg";
  case runtime_job_kind_t::channel_representation_mtf_jepa_mae_vicreg:
    return "channel_representation_mtf_jepa_mae_vicreg";
  case runtime_job_kind_t::channel_inference_mdn:
    return "channel_inference_mdn";
  case runtime_job_kind_t::policy_training:
    return "policy_training";
  }
  return "unknown";
}

struct job_manifest_t {
  std::string job_id{};
  std::string job_stable_id{};
  std::string job_attempt_id{};
  std::size_t job_attempt_index{0};
  std::string job_attempt_policy{"unset"};
  std::string job_kind{};
  std::string config_path{};
  std::string config_bundle_id{};
  std::string config_receipt_id{};
  std::string component_spawn_registry_id{};
  std::string component_family_id{};
  std::string component_spawn_schema{"kikijyeba.component_spawn.v2"};
  std::string component_spawn_fingerprint{};
  std::string component_spawn_id{};
  std::string component_spawn_label{};
  std::string topology_id{"kikijyeba.topology.graph.v1"};
  std::string protocol_id{};
  std::string protocol_kind{};
  std::string protocol_status{};
  std::string successor_protocol{};
  std::string protocol_warning{};
  std::string active_representation_family{};
  std::string active_representation_component_assembly_id{};
  std::string protocol_observer_family{};
  std::string protocol_allocation_policy_family{};
  std::string protocol_representation_contract{};
  std::string wave_id{};
  std::string target_component_family_id{};
  std::string wave_action{};
  std::string wave_mode{};
  std::string execution_chain{};
  std::string mutated_components{};
  std::string frozen_components{};
  std::string source_range_policy{};
  std::string source_order_policy{};
  bool source_order_policy_explicit{false};
  std::string source_order_warning_level{"none"};
  std::string source_order_warnings{};
  std::string requested_source_key_begin{};
  std::string requested_source_key_end{};
  std::size_t resolved_anchor_index_begin{0};
  std::size_t resolved_anchor_index_end{0};
  std::size_t accepted_anchor_count{0};
  std::size_t candidate_anchor_count{0};
  double accepted_anchor_fraction{0.0};
  std::size_t skipped_outside_common_range{0};
  std::size_t skipped_missing_edge_coverage{0};
  std::size_t skipped_failed_fetch_probe{0};
  std::size_t duplicate_anchor_count{0};
  std::string anchor_domain_warning_level{"none"};
  std::string anchor_domain_warnings{};
  std::string source_cursor_token{};
  std::size_t source_input_length{0};
  std::size_t source_future_length{0};
  std::int64_t observed_source_row_begin{0};
  std::int64_t observed_source_row_end{0};
  std::int64_t target_source_row_begin{0};
  std::int64_t target_source_row_end{0};
  std::string source_footprint_precision{"graph_anchor_row_index_v1"};
  std::string first_anchor_key{};
  std::string last_anchor_key{};
  std::string observed_source_key_begin{};
  std::string observed_source_key_end{};
  std::string target_source_key_begin{};
  std::string target_source_key_end{};
  std::string source_key_footprint_precision{};
  std::string source_file_receipts{};
  std::string common_left_key{};
  std::string common_right_key{};
  std::string reference_left_key{};
  std::string reference_right_key{};
  std::string protocol_contract_fingerprint{};
  std::string protocol_contract_token{};
  std::string graph_order_fingerprint{};
  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  std::string nodelift_assembly_fingerprint{};
  std::string vicreg_assembly_fingerprint{};
  std::string mtf_jepa_mae_vicreg_assembly_fingerprint{};
  std::string mdn_assembly_fingerprint{};
  std::string dock_binding_fingerprint{};
  std::string dock_binding_token{};
  std::string stream_plan{};
  std::string representation_training_id{};
  std::string inference_training_id{};
  std::string input_representation_checkpoint_path{};
  std::string input_mdn_checkpoint_path{};
  std::string runtime_handoff_id{};
  std::string runtime_handoff_digest{};
  std::string marshal_target_driver_run_id{};
  std::string policy_training_contract_schema{};
  std::string policy_training_contract_digest{};
  std::string policy_training_artifact_schema{};
  std::string manifest_format{"kikijyeba.runtime.job_manifest.v1"};

  [[nodiscard]] std::string to_text() const {
    std::ostringstream out;
    out << "manifest_format=" << manifest_format << "\n";
    out << "job_id=" << job_id << "\n";
    out << "job_stable_id=" << job_stable_id << "\n";
    out << "job_attempt_id=" << job_attempt_id << "\n";
    out << "job_attempt_index=" << job_attempt_index << "\n";
    out << "job_attempt_policy=" << job_attempt_policy << "\n";
    out << "job_kind=" << job_kind << "\n";
    out << "config_path=" << config_path << "\n";
    out << "config_bundle_id=" << config_bundle_id << "\n";
    out << "config_receipt_id=" << config_receipt_id << "\n";
    out << "component_spawn_registry_id=" << component_spawn_registry_id
        << "\n";
    out << "component_family_id=" << component_family_id << "\n";
    out << "component_spawn_schema=" << component_spawn_schema << "\n";
    out << "component_spawn_fingerprint=" << component_spawn_fingerprint
        << "\n";
    out << "component_spawn_id=" << component_spawn_id << "\n";
    out << "component_spawn_label=" << component_spawn_label << "\n";
    out << "topology_id=" << topology_id << "\n";
    out << "protocol_id=" << protocol_id << "\n";
    out << "protocol_kind=" << protocol_kind << "\n";
    out << "protocol_status=" << protocol_status << "\n";
    out << "successor_protocol=" << successor_protocol << "\n";
    out << "protocol_warning=" << protocol_warning << "\n";
    out << "active_representation_family=" << active_representation_family
        << "\n";
    out << "active_representation_component_assembly_id="
        << active_representation_component_assembly_id << "\n";
    out << "protocol_observer_family=" << protocol_observer_family << "\n";
    out << "protocol_allocation_policy_family="
        << protocol_allocation_policy_family << "\n";
    out << "protocol_representation_contract="
        << protocol_representation_contract << "\n";
    out << "wave_id=" << wave_id << "\n";
    out << "target_component_family_id=" << target_component_family_id << "\n";
    out << "wave_action=" << wave_action << "\n";
    out << "wave_mode=" << wave_mode << "\n";
    out << "execution_chain=" << execution_chain << "\n";
    out << "mutated_components=" << mutated_components << "\n";
    out << "frozen_components=" << frozen_components << "\n";
    out << "source_range_policy=" << source_range_policy << "\n";
    out << "source_order_policy=" << source_order_policy << "\n";
    out << "source_order_policy_explicit="
        << (source_order_policy_explicit ? "true" : "false") << "\n";
    out << "source_order_warning_level=" << source_order_warning_level << "\n";
    out << "source_order_warnings=" << source_order_warnings << "\n";
    out << "requested_source_key_begin=" << requested_source_key_begin << "\n";
    out << "requested_source_key_end=" << requested_source_key_end << "\n";
    out << "resolved_anchor_index_begin=" << resolved_anchor_index_begin
        << "\n";
    out << "resolved_anchor_index_end=" << resolved_anchor_index_end << "\n";
    out << "accepted_anchor_count=" << accepted_anchor_count << "\n";
    out << "candidate_anchor_count=" << candidate_anchor_count << "\n";
    out << "accepted_anchor_fraction=" << accepted_anchor_fraction << "\n";
    out << "skipped_outside_common_range=" << skipped_outside_common_range
        << "\n";
    out << "skipped_missing_edge_coverage=" << skipped_missing_edge_coverage
        << "\n";
    out << "skipped_failed_fetch_probe=" << skipped_failed_fetch_probe << "\n";
    out << "duplicate_anchor_count=" << duplicate_anchor_count << "\n";
    out << "anchor_domain_warning_level=" << anchor_domain_warning_level
        << "\n";
    out << "anchor_domain_warnings=" << anchor_domain_warnings << "\n";
    out << "source_cursor_token=" << source_cursor_token << "\n";
    out << "source_input_length=" << source_input_length << "\n";
    out << "source_future_length=" << source_future_length << "\n";
    out << "observed_source_row_begin=" << observed_source_row_begin << "\n";
    out << "observed_source_row_end=" << observed_source_row_end << "\n";
    out << "target_source_row_begin=" << target_source_row_begin << "\n";
    out << "target_source_row_end=" << target_source_row_end << "\n";
    out << "source_footprint_precision=" << source_footprint_precision << "\n";
    out << "first_anchor_key=" << first_anchor_key << "\n";
    out << "last_anchor_key=" << last_anchor_key << "\n";
    out << "observed_source_key_begin=" << observed_source_key_begin << "\n";
    out << "observed_source_key_end=" << observed_source_key_end << "\n";
    out << "target_source_key_begin=" << target_source_key_begin << "\n";
    out << "target_source_key_end=" << target_source_key_end << "\n";
    out << "source_key_footprint_precision=" << source_key_footprint_precision
        << "\n";
    out << "source_file_receipts=" << source_file_receipts << "\n";
    out << "common_left_key=" << common_left_key << "\n";
    out << "common_right_key=" << common_right_key << "\n";
    out << "reference_left_key=" << reference_left_key << "\n";
    out << "reference_right_key=" << reference_right_key << "\n";
    out << "protocol_contract_fingerprint=" << protocol_contract_fingerprint
        << "\n";
    out << "protocol_contract_token=" << protocol_contract_token << "\n";
    out << "graph_order_fingerprint=" << graph_order_fingerprint << "\n";
    out << "node_ids=";
    for (std::size_t i = 0; i < node_ids.size(); ++i) {
      if (i != 0)
        out << ",";
      out << node_ids[i];
    }
    out << "\nedge_ids=";
    for (std::size_t i = 0; i < edge_ids.size(); ++i) {
      if (i != 0)
        out << ",";
      out << edge_ids[i];
    }
    out << "\n";
    out << "nodelift_assembly_fingerprint=" << nodelift_assembly_fingerprint
        << "\n";
    out << "vicreg_assembly_fingerprint=" << vicreg_assembly_fingerprint
        << "\n";
    out << "mtf_jepa_mae_vicreg_assembly_fingerprint="
        << mtf_jepa_mae_vicreg_assembly_fingerprint << "\n";
    out << "mdn_assembly_fingerprint=" << mdn_assembly_fingerprint << "\n";
    out << "dock_binding_fingerprint=" << dock_binding_fingerprint << "\n";
    out << "dock_binding_token=" << dock_binding_token << "\n";
    out << "stream_plan=" << stream_plan << "\n";
    out << "representation_training_id=" << representation_training_id << "\n";
    out << "inference_training_id=" << inference_training_id << "\n";
    out << "input_representation_checkpoint_path="
        << input_representation_checkpoint_path << "\n";
    out << "input_mdn_checkpoint_path=" << input_mdn_checkpoint_path << "\n";
    out << "runtime_handoff_id=" << runtime_handoff_id << "\n";
    out << "runtime_handoff_digest=" << runtime_handoff_digest << "\n";
    out << "marshal_target_driver_run_id=" << marshal_target_driver_run_id
        << "\n";
    out << "policy_training_contract_schema=" << policy_training_contract_schema
        << "\n";
    out << "policy_training_contract_digest=" << policy_training_contract_digest
        << "\n";
    out << "policy_training_artifact_schema=" << policy_training_artifact_schema
        << "\n";
    return out.str();
  }
};

[[nodiscard]] inline std::string
execution_chain_for_job(runtime_job_kind_t job_kind,
                        const std::string &wave_action,
                        const std::string &active_representation_family =
                            "wikimyei.representation.encoding.vicreg") {
  const bool active_mtf =
      active_representation_family ==
      "wikimyei.representation.encoding.mtf_jepa_mae_vicreg";
  switch (job_kind) {
  case runtime_job_kind_t::channel_representation_vicreg:
    return wave_action == "train"
               ? "ujcamei.source.registry:run -> "
                 "wikimyei.expression.nodelift.srl:run -> "
                 "wikimyei.representation.encoding.vicreg:train"
               : "ujcamei.source.registry:run -> "
                 "wikimyei.expression.nodelift.srl:run -> "
                 "wikimyei.representation.encoding.vicreg:run";
  case runtime_job_kind_t::channel_representation_mtf_jepa_mae_vicreg:
    return wave_action == "train"
               ? "ujcamei.source.registry:run -> "
                 "wikimyei.expression.nodelift.srl:run -> "
                 "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:train"
               : "ujcamei.source.registry:run -> "
                 "wikimyei.expression.nodelift.srl:run -> "
                 "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:run";
  case runtime_job_kind_t::channel_inference_mdn:
    if (active_mtf) {
      return wave_action == "train"
                 ? "ujcamei.source.registry:run -> "
                   "wikimyei.expression.nodelift.srl:run -> "
                   "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:"
                   "run_frozen -> wikimyei.inference.expected_value.mdn:train"
                 : "ujcamei.source.registry:run -> "
                   "wikimyei.expression.nodelift.srl:run -> "
                   "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:"
                   "run_frozen -> wikimyei.inference.expected_value.mdn:run";
    }
    return wave_action == "train"
               ? "ujcamei.source.registry:run -> "
                 "wikimyei.expression.nodelift.srl:run -> "
                 "wikimyei.representation.encoding.vicreg:run_frozen "
                 "-> wikimyei.inference.expected_value.mdn:train"
               : "ujcamei.source.registry:run -> "
                 "wikimyei.expression.nodelift.srl:run -> "
                 "wikimyei.representation.encoding.vicreg:run_frozen "
                 "-> wikimyei.inference.expected_value.mdn:run";
  case runtime_job_kind_t::policy_training:
    return "runtime.completed_replay_job:read -> "
           "kikijyeba.environment.replay.v1:rollout -> "
           "cajtucu.execution.paper.v1:paper_execute -> "
           "wikimyei.policy.trainable:train_contract";
  }
  return {};
}

[[nodiscard]] inline std::string
mutated_components_for_job(runtime_job_kind_t job_kind,
                           const std::string &wave_action) {
  if (wave_action != "train") {
    return {};
  }
  switch (job_kind) {
  case runtime_job_kind_t::channel_representation_vicreg:
    return "wikimyei.representation.encoding.vicreg";
  case runtime_job_kind_t::channel_representation_mtf_jepa_mae_vicreg:
    return "wikimyei.representation.encoding.mtf_jepa_mae_vicreg";
  case runtime_job_kind_t::channel_inference_mdn:
    return "wikimyei.inference.expected_value.mdn";
  case runtime_job_kind_t::policy_training:
    return "wikimyei.policy.trainable";
  }
  return {};
}

[[nodiscard]] inline std::string
frozen_components_for_job(runtime_job_kind_t job_kind) {
  switch (job_kind) {
  case runtime_job_kind_t::channel_representation_vicreg:
  case runtime_job_kind_t::channel_representation_mtf_jepa_mae_vicreg:
    return {};
  case runtime_job_kind_t::channel_inference_mdn:
    return "wikimyei.representation.encoding.vicreg";
  case runtime_job_kind_t::policy_training:
    return "wikimyei.inference.expected_value.mdn,"
           "wikimyei.observer.belief,"
           "kikijyeba.environment.replay.v1,"
           "cajtucu.execution.paper.v1";
  }
  return {};
}

[[nodiscard]] inline std::string
frozen_components_for_job(runtime_job_kind_t job_kind,
                          const std::string &active_representation_family) {
  if (job_kind == runtime_job_kind_t::policy_training) {
    return frozen_components_for_job(job_kind);
  }
  if (job_kind != runtime_job_kind_t::channel_inference_mdn) {
    return frozen_components_for_job(job_kind);
  }
  return active_representation_family.empty()
             ? std::string{"wikimyei.representation.encoding.vicreg"}
             : active_representation_family;
}

namespace job_manifest_detail {

[[nodiscard]] inline bool source_form_matches_signature(
    const cuwacunu::ujcamei::source::contract::source_form_t &source,
    const cuwacunu::ujcamei::source::registry::instrument_signature_t
        &signature) {
  return source.instrument == signature.symbol &&
         source.market_type == signature.market_type &&
         source.venue == signature.venue &&
         source.base_asset == signature.base_asset &&
         source.quote_asset == signature.quote_asset;
}

template <typename BundleT>
[[nodiscard]] inline std::string
source_file_receipts_for_bundle(const BundleT &bundle) {
  namespace types = cuwacunu::ujcamei::source::registry::types;
  std::vector<std::string> receipts;
  const auto active_channels = cuwacunu::kikijyeba::protocol::
      graph_first_source_resolution_detail::active_channels(bundle.source_dock);
  for (const auto &edge_id : bundle.source_plan.market_graph.edge_ids) {
    const auto edge_it = bundle.source_plan.edge_instruments.find(edge_id);
    if (edge_it == bundle.source_plan.edge_instruments.end()) {
      continue;
    }
    const auto &signature = edge_it->second;
    for (const auto *channel : active_channels) {
      for (const auto &source :
           bundle.source_plan.compat_source_spec.source_forms) {
        if (!source_form_matches_signature(source, signature) ||
            source.interval != channel->interval ||
            source.record_type != channel->record_type) {
          continue;
        }
        std::ostringstream receipt;
        receipt << "edge=" << edge_id << "|instrument=" << source.instrument
                << "|interval=" << types::enum_to_string(source.interval)
                << "|record_type=" << source.record_type
                << "|source=" << source.source;
        receipts.push_back(receipt.str());
      }
    }
  }
  std::sort(receipts.begin(), receipts.end());
  receipts.erase(std::unique(receipts.begin(), receipts.end()), receipts.end());
  std::ostringstream out;
  for (std::size_t i = 0; i < receipts.size(); ++i) {
    if (i != 0) {
      out << ";;";
    }
    out << receipts.at(i);
  }
  return out.str();
}

} // namespace job_manifest_detail

[[nodiscard]] inline const cuwacunu::jkimyei::training::training_run_spec_t &
representation_training_for_manifest(
    const cuwacunu::kikijyeba::protocol::channel_graph_first_config_bundle_t
        &bundle) {
  return cuwacunu::kikijyeba::protocol::
                 active_protocol_uses_mtf_jepa_mae_vicreg(bundle)
             ? bundle.mtf_jepa_mae_vicreg_training
             : bundle.vicreg_training;
}

[[nodiscard]] inline const cuwacunu::jkimyei::training::training_run_spec_t &
inference_training_for_manifest(
    const cuwacunu::kikijyeba::protocol::channel_graph_first_config_bundle_t
        &bundle) {
  return bundle.channel_mdn_training;
}

template <typename BuilderT>
[[nodiscard]] inline job_manifest_t
make_job_manifest(const BuilderT &builder, const wave_plan_t &wave_plan,
                  runtime_job_kind_t job_kind, std::string job_id = {}) {
  const auto dry_run = builder.dry_run_report();
  if (job_id.empty()) {
    job_id = wave_plan.wave_id + "." + wave_plan.action + "." +
             runtime_job_kind_name(job_kind);
  }
  job_manifest_t out{};
  out.job_id = std::move(job_id);
  out.job_stable_id = out.job_id;
  out.job_kind = runtime_job_kind_name(job_kind);
  out.config_path = builder.bundle().config_path;
  out.protocol_id = dry_run.protocol_id;
  out.protocol_kind = dry_run.protocol_kind;
  out.protocol_status = dry_run.protocol_status;
  out.successor_protocol = dry_run.successor_protocol;
  out.protocol_warning = dry_run.protocol_warning;
  out.active_representation_family = dry_run.active_representation_family;
  out.active_representation_component_assembly_id =
      dry_run.active_representation_component_assembly_id;
  out.protocol_observer_family = dry_run.protocol_observer_family;
  out.protocol_allocation_policy_family =
      dry_run.protocol_allocation_policy_family;
  out.protocol_representation_contract =
      dry_run.protocol_representation_contract;
  out.wave_id = wave_plan.wave_id;
  out.target_component_family_id = wave_plan.target_component_family_id;
  out.wave_action = wave_plan.action;
  out.wave_mode = wave_plan.mode_text;
  out.execution_chain = execution_chain_for_job(
      job_kind, wave_plan.action, dry_run.active_representation_family);
  out.mutated_components =
      mutated_components_for_job(job_kind, wave_plan.action);
  out.frozen_components =
      frozen_components_for_job(job_kind, dry_run.active_representation_family);
  out.source_range_policy = wave_plan.source_range_policy;
  out.source_order_policy = wave_plan.source_order_policy;
  out.source_order_policy_explicit = wave_plan.source_order_policy_explicit;
  out.source_order_warning_level = wave_plan.source_order_warning_level;
  out.source_order_warnings = wave_plan.source_order_warnings;
  if (wave_plan.requested_source_key_begin.has_value()) {
    out.requested_source_key_begin =
        std::to_string(*wave_plan.requested_source_key_begin);
  }
  if (wave_plan.requested_source_key_end.has_value()) {
    out.requested_source_key_end =
        std::to_string(*wave_plan.requested_source_key_end);
  }
  out.resolved_anchor_index_begin = wave_plan.resolved_anchor_index_begin;
  out.resolved_anchor_index_end = wave_plan.resolved_anchor_index_end;
  out.accepted_anchor_count = wave_plan.accepted_anchor_count;
  out.candidate_anchor_count = wave_plan.candidate_anchor_count;
  out.accepted_anchor_fraction = wave_plan.accepted_anchor_fraction;
  out.skipped_outside_common_range = wave_plan.skipped_outside_common_range;
  out.skipped_missing_edge_coverage = wave_plan.skipped_missing_edge_coverage;
  out.skipped_failed_fetch_probe = wave_plan.skipped_failed_fetch_probe;
  out.duplicate_anchor_count = wave_plan.duplicate_anchor_count;
  out.anchor_domain_warning_level = wave_plan.anchor_domain_warning_level;
  out.anchor_domain_warnings = wave_plan.anchor_domain_warnings;
  out.source_cursor_token = wave_plan.source_cursor_token;
  out.source_input_length = wave_plan.source_input_length;
  out.source_future_length = wave_plan.source_future_length;
  out.observed_source_row_begin = wave_plan.observed_source_row_begin;
  out.observed_source_row_end = wave_plan.observed_source_row_end;
  out.target_source_row_begin = wave_plan.target_source_row_begin;
  out.target_source_row_end = wave_plan.target_source_row_end;
  out.source_footprint_precision = wave_plan.source_footprint_precision;
  out.first_anchor_key = wave_plan.first_anchor_key;
  out.last_anchor_key = wave_plan.last_anchor_key;
  out.observed_source_key_begin = wave_plan.observed_source_key_begin;
  out.observed_source_key_end = wave_plan.observed_source_key_end;
  out.target_source_key_begin = wave_plan.target_source_key_begin;
  out.target_source_key_end = wave_plan.target_source_key_end;
  out.source_key_footprint_precision = wave_plan.source_key_footprint_precision;
  out.source_file_receipts =
      job_manifest_detail::source_file_receipts_for_bundle(builder.bundle());
  out.common_left_key = wave_plan.common_left_key;
  out.common_right_key = wave_plan.common_right_key;
  out.reference_left_key = wave_plan.reference_left_key;
  out.reference_right_key = wave_plan.reference_right_key;
  out.protocol_contract_fingerprint = dry_run.protocol_contract_fingerprint;
  out.protocol_contract_token = dry_run.protocol_contract_token;
  out.graph_order_fingerprint = dry_run.graph_order_fingerprint;
  out.node_ids = dry_run.node_ids;
  out.edge_ids = dry_run.edge_ids;
  out.nodelift_assembly_fingerprint = dry_run.nodelift_assembly_fingerprint;
  out.vicreg_assembly_fingerprint = dry_run.vicreg_assembly_fingerprint;
  out.mtf_jepa_mae_vicreg_assembly_fingerprint =
      dry_run.mtf_jepa_mae_vicreg_assembly_fingerprint;
  out.mdn_assembly_fingerprint = dry_run.mdn_assembly_fingerprint;
  out.dock_binding_fingerprint = dry_run.dock_binding_fingerprint;
  out.dock_binding_token = dry_run.dock_binding_token;
  out.stream_plan = dry_run.stream_plan.summary();
  const auto &representation_training =
      representation_training_for_manifest(builder.bundle());
  const auto &inference_training =
      inference_training_for_manifest(builder.bundle());
  out.representation_training_id = representation_training.training_id;
  out.inference_training_id = inference_training.training_id;
  out.input_representation_checkpoint_path =
      inference_training.input_representation_checkpoint_path;
  out.input_mdn_checkpoint_path = inference_training.input_mdn_checkpoint_path;
  return out;
}

inline void write_job_manifest_file(const std::filesystem::path &path,
                                    const job_manifest_t &manifest) {
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error(
        "[kikijyeba_job_runner] failed to open job manifest path: " +
        path.string());
  }
  out << manifest.to_text();
}

} // namespace cuwacunu::hero::runtime
