#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "jkimyei/api/training_spec.h"
#include "jkimyei/training/inference/mdn_trainer.h"
#include "kikijyeba/protocol/config_bundle.h"
#include "piaabo/parse/simple_kv_block.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "wikimyei/inference/expected_value/mdn/assembly.h"
#include "wikimyei/inference/expected_value/mdn/mdn_spec.h"
#include "wikimyei/representation/encoding/vicreg/assembly.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_spec.h"

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
  if (bundle.wave_settings.wave_id.empty() ||
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
  if (!mdn_training_spec.input_representation_checkpoint_path.empty() ||
      !mdn_training_spec.input_mdn_checkpoint_path.empty()) {
    throw std::runtime_error(
        "repository MDN training config must not carry checkpoint paths");
  }
  if (!mdn_training_spec.allow_untrained_representation) {
    throw std::runtime_error(
        "repository MDN training config should remain smoke-valid without "
        "checkpoint paths");
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

void test_runtime_checkpoint_inputs_do_not_change_protocol_contract_identity() {
  namespace protocol = cuwacunu::kikijyeba::protocol;

  const auto bundle =
      protocol::load_channel_graph_first_config_bundle_from_config(
          "/cuwacunu/src/config/.config");
  const auto base_fingerprint =
      protocol::channel_graph_first_protocol_contract_fingerprint(bundle);
  if (base_fingerprint.empty()) {
    throw std::runtime_error("protocol contract fingerprint missing");
  }

  auto checkpoint_input_bundle = bundle;
  checkpoint_input_bundle.channel_mdn_training
      .input_representation_checkpoint_path =
      "/cuwacunu/.runtime/checkpoints/vicreg_acceptance.pt";
  checkpoint_input_bundle.channel_mdn_training.input_mdn_checkpoint_path =
      "/cuwacunu/.runtime/checkpoints/mdn_acceptance.pt";
  protocol::validate_channel_graph_first_config_bundle(checkpoint_input_bundle);
  const auto checkpoint_input_fingerprint =
      protocol::channel_graph_first_protocol_contract_fingerprint(
          checkpoint_input_bundle);
  if (checkpoint_input_fingerprint != base_fingerprint) {
    throw std::runtime_error(
        "runtime checkpoint inputs changed protocol contract fingerprint");
  }

  auto contract_selector_bundle = bundle;
  ++contract_selector_bundle.channel_mdn_training.batch_size;
  const auto selector_fingerprint =
      protocol::channel_graph_first_protocol_contract_fingerprint(
          contract_selector_bundle);
  if (selector_fingerprint == base_fingerprint) {
    throw std::runtime_error(
        "contract training selector did not change protocol contract "
        "fingerprint");
  }
}

void test_channel_protocol_contract_identity_binds_strict_channel_config() {
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  namespace protocol = cuwacunu::kikijyeba::protocol;

  const auto bundle =
      protocol::load_channel_graph_first_config_bundle_from_config(
          "/cuwacunu/src/config/.config");
  const auto base_fingerprint =
      protocol::channel_graph_first_protocol_contract_fingerprint(bundle);
  if (base_fingerprint.empty()) {
    throw std::runtime_error("channel protocol contract fingerprint missing");
  }

  auto checkpoint_input_bundle = bundle;
  checkpoint_input_bundle.channel_mdn_training
      .input_representation_checkpoint_path =
      "/cuwacunu/.runtime/checkpoints/vicreg_acceptance.pt";
  checkpoint_input_bundle.channel_mdn_training.input_mdn_checkpoint_path =
      "/cuwacunu/.runtime/checkpoints/channel_mdn_acceptance.pt";
  protocol::validate_channel_graph_first_config_bundle(checkpoint_input_bundle);
  const auto checkpoint_input_fingerprint =
      protocol::channel_graph_first_protocol_contract_fingerprint(
          checkpoint_input_bundle);
  if (checkpoint_input_fingerprint != base_fingerprint) {
    throw std::runtime_error(
        "channel runtime checkpoint inputs changed protocol contract "
        "fingerprint");
  }

  const auto require_changed =
      [&](protocol::channel_graph_first_config_bundle_t candidate,
          const std::string &label) {
        protocol::validate_channel_graph_first_config_bundle(candidate);
        const auto fingerprint =
            protocol::channel_graph_first_protocol_contract_fingerprint(
                candidate);
        if (fingerprint == base_fingerprint) {
          throw std::runtime_error(label +
                                   " did not change channel protocol contract "
                                   "fingerprint");
        }
      };

  auto feature_stem_bundle = bundle;
  ++feature_stem_bundle.vicreg.feature_hidden_dim;
  require_changed(feature_stem_bundle, "channel VICReg feature hidden dim");

  auto cell_policy_bundle = bundle;
  cell_policy_bundle.vicreg.cell_valid_policy =
      vicreg::cell_valid_policy_t::min_valid_fraction;
  cell_policy_bundle.vicreg.min_valid_fraction = 0.5;
  require_changed(cell_policy_bundle, "channel VICReg cell-valid policy");

  auto projector_bundle = bundle;
  ++projector_bundle.vicreg.vicreg_projector_dim;
  require_changed(projector_bundle, "channel VICReg projector dim");

  auto augmentation_bundle = bundle;
  augmentation_bundle.vicreg.feature_dropout_prob = 0.25;
  require_changed(augmentation_bundle, "channel VICReg augmentation policy");

  auto mdn_hidden_bundle = bundle;
  ++mdn_hidden_bundle.channel_mdn.hidden_width;
  require_changed(mdn_hidden_bundle, "channel MDN hidden width");

  auto mdn_nll_bundle = bundle;
  mdn_nll_bundle.channel_mdn.sigma_min *= 2.0;
  require_changed(mdn_nll_bundle, "channel MDN NLL bounds");

  auto training_bundle = bundle;
  training_bundle.channel_mdn_training.learning_rate *= 2.0;
  require_changed(training_bundle, "channel MDN training optimizer config");
}

void test_channel_specs_decode_and_validate() {
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  namespace protocol = cuwacunu::kikijyeba::protocol;
  namespace training = cuwacunu::jkimyei::training;

  const auto paths = read_config_paths("/cuwacunu/src/config/.config");
  const auto vicreg_dsl_bnf =
      read_text(paths.at("wikimyei_representation_vicreg_dsl_bnf_path"));
  const auto vicreg_net_bnf =
      read_text(paths.at("wikimyei_representation_vicreg_net_bnf_path"));
  const auto vicreg_jkimyei_bnf =
      read_text(paths.at("wikimyei_representation_vicreg_jkimyei_bnf_path"));
  const auto channel_mdn_dsl_bnf =
      read_text(paths.at("wikimyei_inference_expected_value_mdn_dsl_bnf_path"));
  const auto channel_mdn_net_bnf =
      read_text(paths.at("wikimyei_inference_expected_value_mdn_net_bnf_path"));
  const auto channel_mdn_jkimyei_bnf = read_text(
      paths.at("wikimyei_inference_expected_value_mdn_jkimyei_bnf_path"));
  if (vicreg_dsl_bnf.find("VICREG") == std::string::npos ||
      vicreg_net_bnf.find("VICREG_NET") == std::string::npos ||
      vicreg_net_bnf.find("JITTER_STD") == std::string::npos ||
      vicreg_net_bnf.find("FEATURE_DROPOUT_PROB") == std::string::npos ||
      vicreg_net_bnf.find("HISTORY_DROPOUT_PROB") == std::string::npos ||
      vicreg_jkimyei_bnf.find("FREEZE_REPRESENTATION") == std::string::npos ||
      channel_mdn_dsl_bnf.find("MDN") == std::string::npos ||
      channel_mdn_net_bnf.find("MDN_NET") == std::string::npos ||
      channel_mdn_net_bnf.find("GLOBAL_CONTEXT_DIM") == std::string::npos ||
      channel_mdn_jkimyei_bnf.find("INPUT_MDN_CHECKPOINT") ==
          std::string::npos) {
    throw std::runtime_error("channel config BNF surface mismatch");
  }
  const auto representation_spec = vicreg::decode_vicreg_spec_from_split_dsl(
      read_text(paths.at("wikimyei_representation_vicreg_dsl_path")),
      read_text(paths.at("wikimyei_representation_vicreg_net_path")));
  const auto mdn_spec = mdn::decode_channel_mdn_spec_from_split_dsl(
      read_text(paths.at("wikimyei_inference_expected_value_mdn_dsl_path")),
      read_text(paths.at("wikimyei_inference_expected_value_mdn_net_path")));
  const auto representation_training =
      training::decode_training_run_spec_from_dsl(
          read_text(paths.at("wikimyei_representation_vicreg_jkimyei_path")));
  const auto mdn_training =
      training::decode_training_run_spec_from_dsl(read_text(
          paths.at("wikimyei_inference_expected_value_mdn_jkimyei_path")));

  if (representation_spec.component_id != "vicreg_v1") {
    throw std::runtime_error("channel VICReg component id mismatch");
  }
  if (mdn_spec.input_representation_id != representation_spec.component_id) {
    throw std::runtime_error("channel MDN representation reference mismatch");
  }
  if (mdn_spec.context_mode !=
          mdn::channel_mdn_context_mode_t::channel_context_strict ||
      mdn_spec.global_context_dim != 0) {
    throw std::runtime_error("channel MDN strict context spec mismatch");
  }
  if (representation_training.task !=
          training::training_task_t::vicreg_representation ||
      representation_training.component_id !=
          representation_spec.component_id) {
    throw std::runtime_error("channel VICReg training spec mismatch");
  }
  if (mdn_training.task !=
          training::training_task_t::mdn_expected_value_inference ||
      mdn_training.component_id != mdn_spec.component_id ||
      !mdn_training.freeze_representation) {
    throw std::runtime_error("channel MDN training spec mismatch");
  }

  auto channel_bundle =
      protocol::load_channel_graph_first_config_bundle_from_config(
          "/cuwacunu/src/config/.config");
  protocol::validate_channel_graph_first_config_bundle(channel_bundle);
  if (channel_bundle.wikimyei_representation_vicreg_dsl_bnf_path.empty() ||
      channel_bundle.wikimyei_representation_vicreg_net_bnf_path.empty() ||
      channel_bundle.wikimyei_inference_expected_value_mdn_dsl_bnf_path
          .empty() ||
      channel_bundle.wikimyei_inference_expected_value_mdn_net_bnf_path
          .empty() ||
      channel_bundle.wikimyei_representation_vicreg_jkimyei_bnf_path.empty() ||
      channel_bundle.wikimyei_inference_expected_value_mdn_jkimyei_bnf_path
          .empty()) {
    throw std::runtime_error(
        "channel graph-first BNF paths missing from bundle");
  }
  if (channel_bundle.vicreg.component_id != representation_spec.component_id ||
      channel_bundle.channel_mdn.component_id != mdn_spec.component_id ||
      channel_bundle.channel_mdn.input_representation_id !=
          representation_spec.component_id) {
    throw std::runtime_error("channel graph-first bundle component mismatch");
  }
  if (channel_bundle.vicreg_assembly.family !=
          "wikimyei.representation.encoding.vicreg" ||
      channel_bundle.channel_mdn_assembly.family !=
          "wikimyei.inference.expected_value.mdn") {
    throw std::runtime_error("channel graph-first assembly family mismatch");
  }
  cuwacunu::kikijyeba::topology::validate_node_value_assembly_chain(
      channel_bundle.nodelift_assembly, channel_bundle.vicreg_assembly,
      channel_bundle.channel_mdn_assembly);
  if (!channel_bundle.dock_binding_report.warnings.empty()) {
    throw std::runtime_error("channel dock binding unexpectedly has warnings");
  }
  if (protocol::channel_graph_first_protocol_contract_fingerprint(
          channel_bundle)
          .empty() ||
      protocol::channel_graph_first_protocol_contract_token(channel_bundle)
              .find("kikijyeba.protocol.channel_graph_first.contract.v1/") !=
          0) {
    throw std::runtime_error("channel graph-first contract token missing");
  }
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      channel_bundle.dock_binding, "C",
      protocol::active_channel_count(channel_bundle.source_dock),
      "channel graph-first active channel count");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      channel_bundle.dock_binding, "Hx",
      protocol::max_input_length(channel_bundle.source_dock),
      "channel graph-first input length");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      channel_bundle.dock_binding, "Hf",
      protocol::max_future_length(channel_bundle.source_dock),
      "channel graph-first future length");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      channel_bundle.dock_binding, "De", representation_spec.encoding_dim,
      "channel graph-first encoding dimension");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      channel_bundle.dock_binding, "Df",
      static_cast<int64_t>(mdn_spec.target_coords.size()),
      "channel graph-first MDN target width");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      channel_bundle.dock_binding, "K", mdn_spec.mixture_count,
      "channel graph-first MDN mixture count");

  const auto encoder_options =
      vicreg::channel_encoder_options_from_spec(representation_spec);
  if (encoder_options.channel_count != representation_spec.channel_count ||
      encoder_options.history_length != representation_spec.history_length ||
      encoder_options.encoding_dim != representation_spec.encoding_dim ||
      encoder_options.cell_valid_policy !=
          vicreg::cell_valid_policy_t::required_features) {
    throw std::runtime_error("channel encoder options mismatch");
  }

  const auto projector_options =
      vicreg::channel_projector_options_from_spec(representation_spec);
  if (projector_options.input_dim != representation_spec.encoding_dim ||
      projector_options.projector_dim !=
          representation_spec.vicreg_projector_dim) {
    throw std::runtime_error("channel projector options mismatch");
  }
  const auto vicreg_train_options =
      vicreg::vicreg_train_options_from_spec(representation_spec);
  if (vicreg_train_options.vicreg.global_aux_weight !=
          representation_spec.global_aux_weight ||
      vicreg_train_options.jitter_std != representation_spec.jitter_std ||
      vicreg_train_options.feature_dropout_prob !=
          representation_spec.feature_dropout_prob ||
      vicreg_train_options.history_dropout_prob !=
          representation_spec.history_dropout_prob) {
    throw std::runtime_error("channel VICReg train options mismatch");
  }

  const auto mdn_adapter_options =
      mdn::channel_mdn_adapter_options_from_spec(mdn_spec);
  if (mdn_adapter_options.target_coords != mdn_spec.target_coords ||
      mdn_adapter_options.target_mask_policy !=
          mdn::stream::channel_target_mask_policy_t::
              all_target_features_valid) {
    throw std::runtime_error("channel MDN adapter options mismatch");
  }
  const auto nll_options = mdn::channel_mdn_nll_options_from_spec(mdn_spec);
  if (nll_options.sigma_min != mdn_spec.sigma_min ||
      nll_options.sigma_max != mdn_spec.sigma_max ||
      nll_options.eps != mdn_spec.eps) {
    throw std::runtime_error("channel MDN NLL options mismatch");
  }

  const auto representation_assembly =
      vicreg::make_vicreg_assembly(representation_spec.component_id);
  const auto mdn_assembly =
      mdn::make_channel_context_mdn_assembly(mdn_spec.component_id);
  if (!cuwacunu::wikimyei::assembly::dock_domain_compatible(
          representation_assembly.docks.at(1), mdn_assembly.docks.at(0))) {
    throw std::runtime_error("channel assemblies are not compatible");
  }
  const auto old_fused_producer = cuwacunu::wikimyei::assembly::make_dock(
      "old_fused_node_representation",
      cuwacunu::wikimyei::assembly::dock_direction_t::produces,
      cuwacunu::wikimyei::assembly::dock_role_t::output,
      cuwacunu::wikimyei::assembly::dock_domain_t::node_representation,
      "graph_order.node_representation.v1", "[B,N,De]", "[B,N]",
      /*required=*/true, /*target_side_only=*/false, {"B", "N", "De"});
  if (cuwacunu::wikimyei::assembly::dock_domain_compatible(
          old_fused_producer, mdn_assembly.docks.at(0))) {
    throw std::runtime_error("old fused representation bound channel MDN");
  }

  auto bad_representation = representation_spec;
  bad_representation.history_length = 0;
  expect_throw([&] { vicreg::validate_vicreg_spec(bad_representation); },
               "channel VICReg history length");

  auto bad_augmentation = representation_spec;
  bad_augmentation.feature_dropout_prob = 1.5;
  expect_throw([&] { vicreg::validate_vicreg_spec(bad_augmentation); },
               "channel VICReg augmentation bounds");

  auto bad_mdn = mdn_spec;
  bad_mdn.target_coords = {0, 0};
  expect_throw([&] { mdn::validate_channel_mdn_spec(bad_mdn); },
               "channel MDN duplicate target coord");

  auto bad_strict_global = mdn_spec;
  bad_strict_global.global_context_dim = 4;
  expect_throw([&] { mdn::validate_channel_mdn_spec(bad_strict_global); },
               "channel MDN strict rejects global context dim");

  auto plus_global_mdn = mdn_spec;
  plus_global_mdn.context_mode =
      mdn::channel_mdn_context_mode_t::channel_context_plus_global;
  plus_global_mdn.global_context_dim = 4;
  mdn::validate_channel_mdn_spec(plus_global_mdn);
  const auto plus_global_dsl =
      std::string("MDN {\n"
                  "  VERSION = wikimyei.inference.expected_value.mdn.v1;\n"
                  "  COMPONENT_ID = channel_context_plus_global_mdn_v1;\n"
                  "  INPUT_REPRESENTATION_ID = vicreg_v1;\n"
                  "  CONTEXT_MODE = channel_context_plus_global;\n"
                  "  TARGET_DOMAIN = channel_node_future;\n"
                  "  TARGET_COORDS = 0,1;\n"
                  "  TARGET_MASK_POLICY = all_target_features_valid;\n"
                  "  ACTIVITY_TARGET = node_feature_support_mean;\n"
                  "  SIGMA_MIN = 0.001;\n"
                  "  SIGMA_MAX = 0.0;\n"
                  "  EPS = 0.000001;\n"
                  "};\n");
  const auto plus_global_net = std::string("MDN_NET {\n"
                                           "  CHANNEL_COUNT = 3;\n"
                                           "  FUTURE_HORIZON = 1;\n"
                                           "  MIXTURE_COUNT = 2;\n"
                                           "  HIDDEN_WIDTH = 16;\n"
                                           "  RESIDUAL_DEPTH = 1;\n"
                                           "  GLOBAL_CONTEXT_DIM = 4;\n"
                                           "};\n");
  const auto decoded_plus_global = mdn::decode_channel_mdn_spec_from_split_dsl(
      plus_global_dsl, plus_global_net);
  if (decoded_plus_global.context_mode !=
          mdn::channel_mdn_context_mode_t::channel_context_plus_global ||
      decoded_plus_global.global_context_dim != 4) {
    throw std::runtime_error("channel MDN plus-global spec decode mismatch");
  }
  auto bad_plus_global = decoded_plus_global;
  bad_plus_global.global_context_dim = 0;
  expect_throw([&] { mdn::validate_channel_mdn_spec(bad_plus_global); },
               "channel MDN plus-global requires global context dim");
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
  namespace protocol = cuwacunu::kikijyeba::protocol;

  auto channel_bundle =
      protocol::load_channel_graph_first_config_bundle_from_config(
          "/cuwacunu/src/config/.config");
  channel_bundle.channel_mdn.input_representation_id = "other_channel_rep";
  expect_throw(
      [&] {
        protocol::validate_channel_graph_first_config_bundle(channel_bundle);
      },
      "channel MDN representation mismatch");

  channel_bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      "/cuwacunu/src/config/.config");
  channel_bundle.channel_mdn_assembly.docks.front().coordinate_space =
      "graph_order.node_representation.v1";
  expect_throw(
      [&] {
        protocol::validate_channel_graph_first_config_bundle(channel_bundle);
      },
      "channel assembly coordinate mismatch");

  channel_bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      "/cuwacunu/src/config/.config");
  channel_bundle.vicreg.channel_count =
      cuwacunu::kikijyeba::protocol::active_channel_count(
          channel_bundle.source_dock) +
      1;
  expect_throw(
      [&] {
        protocol::validate_channel_graph_first_config_bundle(channel_bundle);
      },
      "channel source count mismatch");
}

} // namespace

int main() {
  test_runtime_checkpoint_inputs_do_not_change_protocol_contract_identity();
  test_channel_protocol_contract_identity_binds_strict_channel_config();
  test_channel_specs_decode_and_validate();
  test_invalid_specs_fail_fast();
  test_cross_reference_failures();
  std::cout << "[test_wikimyei_graph_first_specs] all checks passed\n";
  return 0;
}
