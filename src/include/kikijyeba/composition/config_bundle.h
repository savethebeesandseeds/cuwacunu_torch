// SPDX-License-Identifier: MIT
#pragma once

#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "jkimyei/api/training_spec.h"
#include "kikijyeba/composition/source_dock.h"
#include "piaabo/parse/simple_kv_block.h"
#include "ujcamei/source/contract/runtime/decode.h"
#include "wikimyei/inference/expected_value/mdn/mdn_spec.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_spec.h"

namespace cuwacunu::kikijyeba::composition {

struct graph_first_config_bundle_t {
  std::string config_path{};
  cuwacunu::ujcamei::source::contract::source_config_paths_t source_paths{};
  graph_first_source_dock_paths_t source_dock_paths{};
  ujcamei_source_universe_t source_universe{};
  graph_first_source_dock_t source_dock{};
  graph_first_source_resolution_report_t source_resolution_report{};
  resolved_graph_first_source_plan_t source_plan{};
  // Transitional compatibility shape for Ujcamei storage/validation surfaces
  // that still accept the historical merged source_spec_t.
  cuwacunu::ujcamei::source::contract::source_spec_t source_spec{};
  std::string wikimyei_representation_vicreg_bnf_path{};
  std::string wikimyei_representation_vicreg_dsl_path{};
  std::string wikimyei_inference_expected_value_mdn_bnf_path{};
  std::string wikimyei_inference_expected_value_mdn_dsl_path{};
  std::string jkimyei_representation_vicreg_bnf_path{};
  std::string jkimyei_representation_vicreg_dsl_path{};
  std::string jkimyei_inference_expected_value_mdn_bnf_path{};
  std::string jkimyei_inference_expected_value_mdn_dsl_path{};

  cuwacunu::wikimyei::representation::encoding::vicreg::
      vicreg_node_representation_spec_t vicreg{};
  cuwacunu::wikimyei::inference::expected_value::mdn::mdn_spec_t mdn{};
  cuwacunu::jkimyei::training::training_run_spec_t vicreg_training{};
  cuwacunu::jkimyei::training::training_run_spec_t mdn_training{};
};

inline void
populate_graph_first_source_plan(graph_first_config_bundle_t &bundle) {
  if (bundle.source_universe.empty() &&
      !bundle.source_spec.source_forms.empty()) {
    bundle.source_universe = make_ujcamei_source_universe(bundle.source_spec);
  }
  if (bundle.source_dock.empty() &&
      (!bundle.source_spec.channel_forms.empty() ||
       !bundle.source_spec.graph_node_forms.empty() ||
       !bundle.source_spec.graph_edge_forms.empty())) {
    bundle.source_dock = make_graph_first_source_dock(bundle.source_spec);
  }
  if (bundle.source_plan.compat_source_spec.source_forms.empty() &&
      !bundle.source_universe.empty() && !bundle.source_dock.empty()) {
    bundle.source_dock = resolve_graph_first_source_dock_edges(
        bundle.source_universe, bundle.source_dock);
    bundle.source_resolution_report = analyze_graph_first_source_resolution(
        bundle.source_universe, bundle.source_dock);
    bundle.source_plan = resolve_graph_first_source_plan(bundle.source_universe,
                                                         bundle.source_dock);
  }
  if (bundle.source_spec.source_forms.empty() &&
      !bundle.source_plan.compat_source_spec.source_forms.empty()) {
    bundle.source_spec = bundle.source_plan.compat_source_spec;
  }
}

inline void
validate_graph_first_config_bundle(const graph_first_config_bundle_t &bundle) {
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;

  vicreg::validate_vicreg_node_representation_spec(bundle.vicreg);
  mdn::validate_mdn_spec(bundle.mdn);
  cuwacunu::jkimyei::training::validate_training_run_spec(
      bundle.vicreg_training);
  cuwacunu::jkimyei::training::validate_training_run_spec(bundle.mdn_training);
  if (bundle.source_universe.empty()) {
    throw std::runtime_error(
        "[graph_first_config] Ujcamei source universe is empty");
  }
  if (bundle.source_dock.empty()) {
    throw std::runtime_error("[graph_first_config] source dock is empty");
  }
  if (bundle.source_plan.compat_source_spec.source_forms.empty()) {
    throw std::runtime_error(
        "[graph_first_config] resolved source plan is empty");
  }

  if (bundle.mdn.input_representation_id != bundle.vicreg.component_id) {
    throw std::runtime_error(
        "[graph_first_config] mdn input representation id does not match "
        "VICReg component id");
  }
  if (bundle.vicreg_training.task !=
      cuwacunu::jkimyei::training::training_task_t::
          vicreg_node_representation) {
    throw std::runtime_error(
        "[graph_first_config] VICReg training spec must use "
        "vicreg_node_representation task");
  }
  if (bundle.vicreg_training.component_id != bundle.vicreg.component_id) {
    throw std::runtime_error(
        "[graph_first_config] VICReg training component id does not match "
        "VICReg component id");
  }
  if (bundle.mdn_training.task != cuwacunu::jkimyei::training::training_task_t::
                                      mdn_expected_value_inference) {
    throw std::runtime_error("[graph_first_config] MDN training spec must use "
                             "mdn_expected_value_inference task");
  }
  if (bundle.mdn_training.component_id != bundle.mdn.component_id) {
    throw std::runtime_error(
        "[graph_first_config] MDN training component id does not match MDN "
        "component id");
  }
}

namespace graph_first_config_detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline std::string
read_text_file_or_throw(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("[graph_first_config] unable to open file: " +
                             path);
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] inline std::unordered_map<std::string, std::string>
parse_assignment_config(const std::string &config_path) {
  std::unordered_map<std::string, std::string> out;
  std::istringstream lines(read_text_file_or_throw(config_path));
  std::string line;
  std::size_t line_number = 0;
  while (std::getline(lines, line)) {
    ++line_number;
    const auto comment = line.find('#');
    if (comment != std::string::npos) {
      line.resize(comment);
    }
    line = kv::trim(std::move(line));
    if (line.empty() || (line.front() == '[' && line.back() == ']')) {
      continue;
    }
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      throw std::runtime_error("[graph_first_config] invalid config line " +
                               std::to_string(line_number) + ": missing '='");
    }
    auto key = kv::trim(line.substr(0, eq));
    auto value = kv::trim(line.substr(eq + 1));
    if (key.empty() || value.empty()) {
      throw std::runtime_error("[graph_first_config] invalid config line " +
                               std::to_string(line_number) +
                               ": empty key or value");
    }
    out[std::move(key)] = std::move(value);
  }
  return out;
}

[[nodiscard]] inline std::string
required_config_value(const std::unordered_map<std::string, std::string> &cfg,
                      const std::string &key, const std::string &config_path) {
  const auto it = cfg.find(key);
  if (it == cfg.end() || kv::trim(it->second).empty()) {
    throw std::runtime_error("[graph_first_config] missing required key '" +
                             key + "' in " + config_path);
  }
  return it->second;
}

} // namespace graph_first_config_detail

[[nodiscard]] inline graph_first_config_bundle_t
load_graph_first_config_bundle_from_config(std::string config_path = {}) {
  if (cuwacunu::piaabo::parse::simple_kv::trim(config_path).empty()) {
    config_path =
        cuwacunu::ujcamei::source::contract::default_source_config_path();
  }
  const auto cfg =
      graph_first_config_detail::parse_assignment_config(config_path);

  graph_first_config_bundle_t out{};
  out.config_path = config_path;
  out.source_paths =
      cuwacunu::ujcamei::source::contract::load_source_config_paths_from_config(
          config_path);
  out.source_dock_paths = graph_first_source_dock_paths_t{
      .channels_bnf_path = graph_first_config_detail::required_config_value(
          cfg, "kikijyeba_protocol_cwu_01v_settings_dock_channels_bnf_path",
          config_path),
      .channels_dsl_path = graph_first_config_detail::required_config_value(
          cfg, "kikijyeba_protocol_cwu_01v_settings_dock_channels_dsl_path",
          config_path),
      .graph_bnf_path = graph_first_config_detail::required_config_value(
          cfg, "kikijyeba_protocol_cwu_01v_topology_graph_bnf_path",
          config_path),
      .graph_dsl_path = graph_first_config_detail::required_config_value(
          cfg, "kikijyeba_protocol_cwu_01v_topology_graph_dsl_path",
          config_path),
  };
  out.source_universe =
      cuwacunu::ujcamei::source::contract::decode_source_universe_from_config(
          config_path);
  out.source_dock = decode_graph_first_source_dock_from_split_dsl(
      graph_first_config_detail::read_text_file_or_throw(
          out.source_dock_paths.channels_bnf_path),
      graph_first_config_detail::read_text_file_or_throw(
          out.source_dock_paths.channels_dsl_path),
      graph_first_config_detail::read_text_file_or_throw(
          out.source_dock_paths.graph_bnf_path),
      graph_first_config_detail::read_text_file_or_throw(
          out.source_dock_paths.graph_dsl_path));
  out.source_dock = resolve_graph_first_source_dock_edges(out.source_universe,
                                                          out.source_dock);
  out.source_resolution_report = analyze_graph_first_source_resolution(
      out.source_universe, out.source_dock);
  out.source_plan =
      resolve_graph_first_source_plan(out.source_universe, out.source_dock);
  out.source_spec = out.source_plan.compat_source_spec;

  out.wikimyei_representation_vicreg_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_representation_vicreg_bnf_path", config_path);
  out.wikimyei_representation_vicreg_dsl_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_representation_vicreg_dsl_path", config_path);
  out.wikimyei_inference_expected_value_mdn_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_inference_expected_value_mdn_bnf_path", config_path);
  out.wikimyei_inference_expected_value_mdn_dsl_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_inference_expected_value_mdn_dsl_path", config_path);
  out.jkimyei_representation_vicreg_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "jkimyei_representation_vicreg_bnf_path", config_path);
  out.jkimyei_representation_vicreg_dsl_path =
      graph_first_config_detail::required_config_value(
          cfg, "jkimyei_representation_vicreg_dsl_path", config_path);
  out.jkimyei_inference_expected_value_mdn_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "jkimyei_inference_expected_value_mdn_bnf_path", config_path);
  out.jkimyei_inference_expected_value_mdn_dsl_path =
      graph_first_config_detail::required_config_value(
          cfg, "jkimyei_inference_expected_value_mdn_dsl_path", config_path);

  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_representation_vicreg_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_inference_expected_value_mdn_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.jkimyei_representation_vicreg_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.jkimyei_inference_expected_value_mdn_bnf_path);

  out.vicreg = cuwacunu::wikimyei::representation::encoding::vicreg::
      decode_vicreg_node_representation_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_representation_vicreg_dsl_path));
  out.mdn = cuwacunu::wikimyei::inference::expected_value::mdn::
      decode_mdn_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_inference_expected_value_mdn_dsl_path));
  out.vicreg_training =
      cuwacunu::jkimyei::training::decode_training_run_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.jkimyei_representation_vicreg_dsl_path));
  out.mdn_training =
      cuwacunu::jkimyei::training::decode_training_run_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.jkimyei_inference_expected_value_mdn_dsl_path));

  populate_graph_first_source_plan(out);
  validate_graph_first_config_bundle(out);
  return out;
}

} // namespace cuwacunu::kikijyeba::composition
