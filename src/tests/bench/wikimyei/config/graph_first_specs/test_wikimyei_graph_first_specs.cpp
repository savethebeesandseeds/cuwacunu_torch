#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "jkimyei/training/inference/mdn_trainer.h"
#include "kikijyeba/protocol/config_bundle.h"
#include "piaabo/parse/simple_kv_block.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"

namespace {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

std::string read_text(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("failed to open " + path);
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

std::unordered_map<std::string, std::string>
read_config_paths(const std::string &path) {
  std::unordered_map<std::string, std::string> out;
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("failed to open " + path);
  }
  std::string line;
  while (std::getline(in, line)) {
    line = kv::trim(line);
    if (line.empty() || line[0] == '[' || line[0] == '#') {
      continue;
    }
    const auto comment = line.find('#');
    if (comment != std::string::npos) {
      line = kv::trim(line.substr(0, comment));
    }
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    auto key = kv::trim(line.substr(0, eq));
    auto value = kv::trim(line.substr(eq + 1));
    if (!key.empty() && !value.empty()) {
      out.emplace(std::move(key), std::move(value));
    }
  }
  return out;
}

template <typename Fn> void expect_throw(Fn &&fn, const char *label) {
  bool threw = false;
  try {
    fn();
  } catch (const std::exception &) {
    threw = true;
  }
  if (!threw) {
    throw std::runtime_error(std::string("expected throw: ") + label);
  }
}

template <std::size_t N>
std::vector<int64_t>
coords_from_array(const std::array<std::int64_t, N> &coords) {
  std::vector<int64_t> out;
  out.reserve(N);
  for (const auto coord : coords) {
    out.push_back(static_cast<int64_t>(coord));
  }
  return out;
}

std::string valid_nodelift_srl_dsl() {
  return R"(
NODELIFT_SRL {
  VERSION = wikimyei.expression.nodelift.srl.v1;
  COMPONENT_ID = nodelift_srl_v1;
  FEATURE_WIDTH = 9;
  PRICE_COORDS = kline_price;
  ACTIVITY_COORDS = kline_activity;
  GAUGE_POLICY = uniform_per_component;
  PRECISION_POLICY = identity;
  ACTIVITY_MODE = support_mean;
  LIFT_FUTURE = target_side;
};
)";
}

std::string replace_first(std::string text, const std::string &from,
                          const std::string &to) {
  const auto pos = text.find(from);
  if (pos == std::string::npos) {
    throw std::runtime_error("replacement token not found: " + from);
  }
  text.replace(pos, from.size(), to);
  return text;
}

void test_config_backed_specs_decode_and_validate() {
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  namespace nodelift = cuwacunu::wikimyei::expression::nodelift::srl;
  namespace protocol = cuwacunu::kikijyeba::protocol;
  namespace training = cuwacunu::jkimyei::training;

  const auto paths = read_config_paths("/cuwacunu/src/config/.config");
  const auto nodelift_spec = nodelift::decode_nodelift_srl_spec_from_dsl(
      read_text(paths.at("wikimyei_expression_nodelift_srl_dsl_path")));
  const auto vicreg_spec =
      vicreg::decode_vicreg_node_representation_spec_from_split_dsl(
          read_text(paths.at("wikimyei_representation_vicreg_dsl_path")),
          read_text(paths.at("wikimyei_representation_vicreg_net_path")));
  const auto mdn_spec = mdn::decode_mdn_spec_from_split_dsl(
      read_text(paths.at("wikimyei_inference_expected_value_mdn_dsl_path")),
      read_text(paths.at("wikimyei_inference_expected_value_mdn_net_path")));
  const auto vicreg_training_spec = training::decode_training_run_spec_from_dsl(
      read_text(paths.at("wikimyei_representation_vicreg_jkimyei_path")));
  const auto mdn_training_spec =
      training::decode_training_run_spec_from_dsl(read_text(
          paths.at("wikimyei_inference_expected_value_mdn_jkimyei_path")));

  auto bundle = protocol::load_graph_first_config_bundle_from_config(
      "/cuwacunu/src/config/.config");
  bundle.nodelift = nodelift_spec;
  bundle.vicreg = vicreg_spec;
  bundle.mdn = mdn_spec;
  bundle.vicreg_training = vicreg_training_spec;
  bundle.mdn_training = mdn_training_spec;
  protocol::validate_graph_first_config_bundle(bundle);
  if (bundle.wave_settings.wave_id != "cwu_01v_smoke" ||
      bundle.wave_settings.source_cursor_kind != "graph_anchor" ||
      bundle.wave_settings.source_cursor_scope != "wave_batch") {
    throw std::runtime_error("Kikijyeba wave settings mismatch");
  }

  if (nodelift_spec.component_id != "nodelift_srl_v1") {
    throw std::runtime_error("unexpected NodeLift SRL component id");
  }
  if (nodelift_spec.price_coords !=
          coords_from_array(cuwacunu::ujcamei::source::registry::types::
                                kKlinePriceFeatureCoords) ||
      nodelift_spec.activity_coords !=
          coords_from_array(cuwacunu::ujcamei::source::registry::types::
                                kKlineActivityFeatureCoords)) {
    throw std::runtime_error("NodeLift SRL kline coordinate selector mismatch");
  }
  if (vicreg_spec.component_id != "node_vicreg_v1") {
    throw std::runtime_error("unexpected VICReg component id");
  }
  if (mdn_spec.input_representation_id != vicreg_spec.component_id) {
    throw std::runtime_error("MDN representation reference mismatch");
  }
  if (vicreg_training_spec.component_id != vicreg_spec.component_id) {
    throw std::runtime_error("VICReg training component reference mismatch");
  }
  if (mdn_training_spec.component_id != mdn_spec.component_id) {
    throw std::runtime_error("MDN training component reference mismatch");
  }
  if (bundle.nodelift_assembly.family != "wikimyei.expression.nodelift.srl" ||
      bundle.vicreg_assembly.family !=
          "wikimyei.representation.encoding.vicreg" ||
      bundle.mdn_assembly.family != "wikimyei.inference.expected_value.mdn") {
    throw std::runtime_error("Wikimyei assembly family mismatch");
  }
  cuwacunu::kikijyeba::topology::validate_node_value_assembly_chain(
      bundle.nodelift_assembly, bundle.vicreg_assembly, bundle.mdn_assembly);
  if (cuwacunu::wikimyei::assembly::assembly_fingerprint(
          bundle.nodelift_assembly)
          .empty() ||
      cuwacunu::wikimyei::assembly::assembly_fingerprint(bundle.vicreg_assembly)
          .empty() ||
      cuwacunu::wikimyei::assembly::assembly_fingerprint(bundle.mdn_assembly)
          .empty()) {
    throw std::runtime_error("Wikimyei assembly fingerprint missing");
  }
  if (bundle.wikimyei_registry.require_component(mdn_spec.component_id)
          .family != "wikimyei.inference.expected_value.mdn") {
    throw std::runtime_error("Wikimyei assembly registry mismatch");
  }
  if (cuwacunu::kikijyeba::topology::dock_binding_fingerprint(
          bundle.dock_binding)
          .empty() ||
      cuwacunu::kikijyeba::topology::dock_binding_token(bundle.dock_binding)
              .find("kikijyeba.topology.graph.dock_binding.v1/") != 0) {
    throw std::runtime_error("dock binding token missing");
  }
  if (!bundle.dock_binding_report.warnings.empty()) {
    throw std::runtime_error("dock binding unexpectedly has warnings");
  }
  cuwacunu::kikijyeba::topology::require_runtime_binding(
      bundle.dock_binding, "B", "test wave batch");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      bundle.dock_binding, "N",
      static_cast<int64_t>(bundle.source_plan.market_graph.node_ids.size()),
      "test graph node count");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      bundle.dock_binding, "L",
      static_cast<int64_t>(bundle.source_plan.market_graph.edge_ids.size()),
      "test graph edge count");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      bundle.dock_binding, "C",
      cuwacunu::kikijyeba::protocol::active_channel_count(bundle.source_dock),
      "test active channel count");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      bundle.dock_binding, "Hx",
      cuwacunu::kikijyeba::protocol::max_input_length(bundle.source_dock),
      "test input length");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      bundle.dock_binding, "Hf",
      cuwacunu::kikijyeba::protocol::max_future_length(bundle.source_dock),
      "test future length");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      bundle.dock_binding, "F",
      cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth,
      "test feature width");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      bundle.dock_binding, "De", vicreg_spec.encoding_dim,
      "test encoding dimension");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      bundle.dock_binding, "Df",
      static_cast<int64_t>(mdn_spec.target_coords.size()),
      "test MDN target width");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      bundle.dock_binding, "K", mdn_spec.mixture_count,
      "test MDN mixture count");
  const auto node_opts =
      vicreg::node_adapter_options_from_vicreg_spec(vicreg_spec,
                                                    /*training=*/true);
  if (!node_opts.compact_valid_nodes_for_training ||
      node_opts.required_feature_coords != std::vector<int64_t>({0, 1, 2, 3})) {
    throw std::runtime_error("VICReg node adapter options mismatch");
  }

  const auto mdn_adapter_opts = mdn::mdn_adapter_options_from_spec(mdn_spec);
  if (mdn_adapter_opts.context_reduction !=
      cuwacunu::wikimyei::inference::expected_value::mdn::stream::
          context_reduction_policy_t::last) {
    throw std::runtime_error("MDN context reduction mismatch");
  }
  if (mdn_adapter_opts.target_coords !=
      std::vector<int64_t>({0, 1, 2, 3, 4, 5, 6, 7, 8})) {
    throw std::runtime_error("MDN target coordinate mismatch");
  }

  const auto trainer_opts =
      training::mdn_training_options_from_specs(mdn_training_spec, mdn_spec);
  if (trainer_opts.target_coords !=
          std::vector<int64_t>({0, 1, 2, 3, 4, 5, 6, 7, 8}) ||
      trainer_opts.nll_options.sigma_min != 0.001) {
    throw std::runtime_error("training options mismatch");
  }
}

void test_invalid_specs_fail_fast() {
  namespace nodelift = cuwacunu::wikimyei::expression::nodelift::srl;
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  namespace training = cuwacunu::jkimyei::training;

  const auto valid_nodelift = valid_nodelift_srl_dsl();
  const auto decoded_nodelift =
      nodelift::decode_nodelift_srl_spec_from_dsl(valid_nodelift);
  if (decoded_nodelift.price_coords !=
          coords_from_array(cuwacunu::ujcamei::source::registry::types::
                                kKlinePriceFeatureCoords) ||
      decoded_nodelift.activity_coords !=
          coords_from_array(cuwacunu::ujcamei::source::registry::types::
                                kKlineActivityFeatureCoords)) {
    throw std::runtime_error("symbolic NodeLift SRL coordinates mismatch");
  }
  expect_throw(
      [&] {
        const auto ignored = nodelift::decode_nodelift_srl_spec_from_dsl(
            replace_first(valid_nodelift, "kline_price", "kline_activity"));
        (void)ignored;
      },
      "NodeLift price selector must be kline price");
  expect_throw(
      [&] {
        const auto ignored = nodelift::decode_nodelift_srl_spec_from_dsl(
            replace_first(valid_nodelift, "kline_activity", "kline_price"));
        (void)ignored;
      },
      "NodeLift activity selector must be kline activity");
  expect_throw(
      [&] {
        const auto ignored = nodelift::decode_nodelift_srl_spec_from_dsl(
            replace_first(valid_nodelift, "kline_price", "0,1,2,4"));
        (void)ignored;
      },
      "NodeLift price coords must match kline registry");

  auto bad_vicreg = vicreg::vicreg_node_representation_spec_t{};
  bad_vicreg.component_id = "bad";
  bad_vicreg.input_width = 8;
  bad_vicreg.encoding_dim = 1;
  bad_vicreg.channel_expansion_dim = 1;
  bad_vicreg.fused_feature_dim = 1;
  bad_vicreg.encoder_hidden_dim = 1;
  bad_vicreg.encoder_depth = 1;
  expect_throw(
      [&] { vicreg::validate_vicreg_node_representation_spec(bad_vicreg); },
      "VICReg width");

  bad_vicreg.input_width = 9;
  bad_vicreg.dtype = "float16";
  expect_throw(
      [&] { vicreg::validate_vicreg_node_representation_spec(bad_vicreg); },
      "VICReg dtype");

  bad_vicreg.dtype = "float32";
  bad_vicreg.device = "gpu:0";
  expect_throw(
      [&] { vicreg::validate_vicreg_node_representation_spec(bad_vicreg); },
      "VICReg device");

  auto bad_mdn = mdn::mdn_spec_t{};
  bad_mdn.component_id = "mdn";
  bad_mdn.input_representation_id = "rep";
  bad_mdn.target_coords = {99};
  bad_mdn.mixture_count = 2;
  bad_mdn.hidden_width = 4;
  expect_throw([&] { mdn::validate_mdn_spec(bad_mdn); }, "MDN target coord");

  auto bad_training = training::training_run_spec_t{};
  bad_training.training_id = "train";
  bad_training.component_id = "mdn";
  bad_training.learning_rate = 0.01;
  bad_training.batch_size = 1;
  bad_training.freeze_representation = false;
  expect_throw([&] { training::validate_training_run_spec(bad_training); },
               "MDN freeze representation");

  bad_training.freeze_representation = true;
  bad_training.validation_every = 1;
  expect_throw([&] { training::validate_training_run_spec(bad_training); },
               "validation cadence");

  bad_training.validation_every = 0;
  bad_training.seed = -1;
  expect_throw([&] { training::validate_training_run_spec(bad_training); },
               "negative seed");
}

void test_cross_reference_failures() {
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  namespace protocol = cuwacunu::kikijyeba::protocol;
  namespace training = cuwacunu::jkimyei::training;

  auto bundle = protocol::load_graph_first_config_bundle_from_config(
      "/cuwacunu/src/config/.config");

  bundle.mdn.input_representation_id = "other_rep";
  expect_throw([&] { protocol::validate_graph_first_config_bundle(bundle); },
               "MDN representation mismatch");

  bundle.mdn.input_representation_id = bundle.vicreg.component_id;
  bundle.mdn_training.component_id = "other_mdn";
  expect_throw([&] { protocol::validate_graph_first_config_bundle(bundle); },
               "MDN training component mismatch");

  bundle.mdn_training.component_id = bundle.mdn.component_id;
  bundle.mdn_assembly.component_id = "other_mdn";
  expect_throw([&] { protocol::validate_graph_first_config_bundle(bundle); },
               "MDN stale assembly mismatch");

  bundle.mdn_assembly.component_id = bundle.mdn.component_id;
  for (auto &variable : bundle.dock_binding.variables) {
    if (variable.name == "N") {
      variable.i64_value = 999;
      break;
    }
  }
  expect_throw([&] { protocol::validate_graph_first_config_bundle(bundle); },
               "dock binding node count mismatch");

  auto missing_variable_bundle =
      protocol::load_graph_first_config_bundle_from_config(
          "/cuwacunu/src/config/.config");
  missing_variable_bundle.vicreg_assembly.docks.front().variables.push_back(
      "MissingDockVariable");
  expect_throw(
      [&] {
        protocol::validate_graph_first_config_bundle(missing_variable_bundle);
      },
      "dock variable missing from binding");

  auto unused_variable_bundle =
      protocol::load_graph_first_config_bundle_from_config(
          "/cuwacunu/src/config/.config");
  unused_variable_bundle.dock_binding.variables.push_back(
      cuwacunu::kikijyeba::topology::make_static_i64_variable(
          "UnusedDockVariable", "test.unused", 1));
  const auto unused_report =
      protocol::make_graph_first_dock_binding_report(unused_variable_bundle);
  if (unused_report.warnings.empty()) {
    throw std::runtime_error("unused dock binding variable warning missing");
  }
}

} // namespace

int main() {
  test_config_backed_specs_decode_and_validate();
  test_invalid_specs_fail_fast();
  test_cross_reference_failures();
  std::cout << "[test_wikimyei_graph_first_specs] all checks passed\n";
  return 0;
}
