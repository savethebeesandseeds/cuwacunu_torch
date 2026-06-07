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
#include "kikijyeba/environment/replay/spec.h"
#include "kikijyeba/protocol/config_bundle.h"
#include "piaabo/parse/simple_kv_block.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "wikimyei/inference/expected_value/mdn/assembly.h"
#include "wikimyei/inference/expected_value/mdn/mdn_spec.h"
#include "wikimyei/observer/belief/assembly.h"
#include "wikimyei/observer/belief/spec.h"
#include "wikimyei/policy/portfolio/graph_node_allocation/assembly.h"
#include "wikimyei/policy/portfolio/graph_node_allocation/spec.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/assembly.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/spec.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg_spec.h"
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
  COMPONENT_ASSEMBLY_ID = nodelift_srl_v1;
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
      "/cuwacunu/.runtime/cuwacunu_exec/checkpoints/vicreg_acceptance.pt";
  checkpoint_input_bundle.channel_mdn_training.input_mdn_checkpoint_path =
      "/cuwacunu/.runtime/cuwacunu_exec/checkpoints/mdn_acceptance.pt";
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
      "/cuwacunu/.runtime/cuwacunu_exec/checkpoints/vicreg_acceptance.pt";
  checkpoint_input_bundle.channel_mdn_training.input_mdn_checkpoint_path =
      "/cuwacunu/.runtime/cuwacunu_exec/checkpoints/channel_mdn_acceptance.pt";
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
  require_changed(feature_stem_bundle, "VICReg feature hidden dim");

  auto cell_policy_bundle = bundle;
  cell_policy_bundle.vicreg.cell_valid_policy =
      vicreg::cell_valid_policy_t::min_valid_fraction;
  cell_policy_bundle.vicreg.min_valid_fraction = 0.5;
  require_changed(cell_policy_bundle, "VICReg cell-valid policy");

  auto projector_bundle = bundle;
  ++projector_bundle.vicreg.vicreg_projector_dim;
  require_changed(projector_bundle, "VICReg projector dim");

  auto augmentation_bundle = bundle;
  augmentation_bundle.vicreg.feature_dropout_prob = 0.25;
  require_changed(augmentation_bundle, "VICReg augmentation policy");

  auto vicreg_loss_bundle = bundle;
  vicreg_loss_bundle.vicreg.vicreg_variance_floor *= 1.5;
  require_changed(vicreg_loss_bundle, "VICReg objective policy");

  auto mdn_hidden_bundle = bundle;
  ++mdn_hidden_bundle.channel_mdn.hidden_width;
  require_changed(mdn_hidden_bundle, "MDN hidden width");

  auto mdn_nll_bundle = bundle;
  mdn_nll_bundle.channel_mdn.sigma_min *= 2.0;
  require_changed(mdn_nll_bundle, "MDN NLL bounds");

  auto training_bundle = bundle;
  training_bundle.channel_mdn_training.learning_rate *= 2.0;
  require_changed(training_bundle, "MDN training optimizer config");

  auto protocol_variant_bundle = bundle;
  protocol_variant_bundle.protocol_variant =
      protocol::decode_protocol_variant_from_dsl(
          read_text("/cuwacunu/src/config/kikijyeba.protocol.cwu_01v.dsl"));
  protocol_variant_bundle.dock_binding =
      protocol::make_channel_graph_first_dock_binding(protocol_variant_bundle);
  protocol_variant_bundle.dock_binding_report =
      protocol::make_channel_graph_first_dock_binding_report(
          protocol_variant_bundle);
  protocol_variant_bundle.wave_settings.compatible_protocol_ids = {"cwu_01v"};
  require_changed(protocol_variant_bundle, "active protocol variant");
}

void test_channel_specs_decode_and_validate() {
  namespace belief = cuwacunu::wikimyei::observer::belief;
  namespace policy =
      cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility;
  namespace graph_allocation =
      cuwacunu::wikimyei::policy::portfolio::graph_node_allocation;
  namespace environment = cuwacunu::kikijyeba::environment;
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  namespace mtf =
      cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  namespace protocol = cuwacunu::kikijyeba::protocol;
  namespace training = cuwacunu::jkimyei::training;

  const auto paths = read_config_paths("/cuwacunu/src/config/.config");
  const auto protocol_bnf =
      read_text(paths.at("kikijyeba_protocol_dsl_bnf_path"));
  const auto vicreg_dsl_bnf =
      read_text(paths.at("wikimyei_representation_vicreg_dsl_bnf_path"));
  const auto vicreg_net_bnf =
      read_text(paths.at("wikimyei_representation_vicreg_net_bnf_path"));
  const auto vicreg_jkimyei_bnf =
      read_text(paths.at("wikimyei_representation_vicreg_jkimyei_bnf_path"));
  const auto mtf_dsl_bnf = read_text(
      paths.at("wikimyei_representation_mtf_jepa_mae_vicreg_dsl_bnf_path"));
  const auto mtf_net_bnf = read_text(
      paths.at("wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path"));
  const auto mtf_jkimyei_bnf = read_text(
      paths.at("wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_bnf_path"));
  const auto channel_mdn_dsl_bnf =
      read_text(paths.at("wikimyei_inference_expected_value_mdn_dsl_bnf_path"));
  const auto channel_mdn_net_bnf =
      read_text(paths.at("wikimyei_inference_expected_value_mdn_net_bnf_path"));
  const auto belief_observer_bnf =
      read_text(paths.at("wikimyei_observer_belief_dsl_bnf_path"));
  const auto spot_distributional_utility_bnf = read_text(paths.at(
      "wikimyei_policy_portfolio_spot_distributional_utility_dsl_bnf_path"));
  const auto graph_node_allocation_dsl_bnf = read_text(
      paths.at("wikimyei_policy_portfolio_graph_node_allocation_dsl_bnf_path"));
  const auto graph_node_allocation_net_bnf = read_text(
      paths.at("wikimyei_policy_portfolio_graph_node_allocation_net_bnf_path"));
  const auto replay_environment_bnf =
      read_text(paths.at("kikijyeba_environment_replay_dsl_bnf_path"));
  const auto channel_mdn_jkimyei_bnf = read_text(
      paths.at("wikimyei_inference_expected_value_mdn_jkimyei_bnf_path"));
  const auto graph_node_allocation_jkimyei_bnf = read_text(paths.at(
      "wikimyei_policy_portfolio_graph_node_allocation_jkimyei_bnf_path"));
  if (protocol_bnf.find("PROTOCOL") == std::string::npos ||
      protocol_bnf.find("REPRESENTATION_CONTRACT") == std::string::npos ||
      protocol_bnf.find("OBSERVER") == std::string::npos ||
      protocol_bnf.find("ALLOCATION_POLICY") == std::string::npos ||
      vicreg_dsl_bnf.find("VICREG") == std::string::npos ||
      vicreg_net_bnf.find("VICREG_NET") == std::string::npos ||
      vicreg_net_bnf.find("VICREG_INVARIANCE_WEIGHT") == std::string::npos ||
      vicreg_net_bnf.find("MIN_VALID_ROWS") == std::string::npos ||
      vicreg_net_bnf.find("JITTER_STD") == std::string::npos ||
      vicreg_net_bnf.find("FEATURE_DROPOUT_PROB") == std::string::npos ||
      vicreg_net_bnf.find("HISTORY_DROPOUT_PROB") == std::string::npos ||
      vicreg_jkimyei_bnf.find("FREEZE_REPRESENTATION") == std::string::npos ||
      mtf_dsl_bnf.find("MTF_JEPA_MAE_VICREG") == std::string::npos ||
      mtf_net_bnf.find("MTF_JEPA_MAE_VICREG_NET") == std::string::npos ||
      mtf_net_bnf.find("TARGET_EMA_TAU") == std::string::npos ||
      mtf_net_bnf.find("LAMBDA_GLOBAL_VICREG") == std::string::npos ||
      mtf_jkimyei_bnf.find("FREEZE_REPRESENTATION") == std::string::npos ||
      channel_mdn_dsl_bnf.find("MDN") == std::string::npos ||
      channel_mdn_net_bnf.find("MDN_NET") == std::string::npos ||
      channel_mdn_net_bnf.find("GLOBAL_CONTEXT_DIM") == std::string::npos ||
      channel_mdn_net_bnf.find("FEATURE_EMBEDDING_DIM") == std::string::npos ||
      channel_mdn_net_bnf.find("CHANNEL_ADAPTER_RANK") == std::string::npos ||
      belief_observer_bnf.find("OBSERVER_BELIEF") == std::string::npos ||
      belief_observer_bnf.find("ACCOUNTING_NUMERAIRE_POLICY") ==
          std::string::npos ||
      belief_observer_bnf.find("PROJECTION_VALIDATION_REQUIRED") ==
          std::string::npos ||
      belief_observer_bnf.find("LIVE_CAPITAL_ALLOWED") == std::string::npos ||
      spot_distributional_utility_bnf.find("SPOT_DISTRIBUTIONAL_UTILITY") ==
          std::string::npos ||
      spot_distributional_utility_bnf.find(
          "REQUIRE_ACCOUNTING_NUMERAIRE_NODE") == std::string::npos ||
      spot_distributional_utility_bnf.find("PROJECTION_VALIDATION_REQUIRED") ==
          std::string::npos ||
      spot_distributional_utility_bnf.find("LIVE_CAPITAL_ALLOWED") ==
          std::string::npos ||
      graph_node_allocation_dsl_bnf.find("GRAPH_NODE_ALLOCATION") ==
          std::string::npos ||
      graph_node_allocation_dsl_bnf.find("POLICY_INPUT_SCHEMA") ==
          std::string::npos ||
      graph_node_allocation_dsl_bnf.find("ACTION_ADAPTER") ==
          std::string::npos ||
      graph_node_allocation_net_bnf.find("GRAPH_NODE_ALLOCATION_NET") ==
          std::string::npos ||
      graph_node_allocation_net_bnf.find("OUTPUT_HEAD") == std::string::npos ||
      replay_environment_bnf.find("REPLAY_ENVIRONMENT") == std::string::npos ||
      replay_environment_bnf.find("OBSERVATION_TIME_LAW") ==
          std::string::npos ||
      replay_environment_bnf.find("REQUIRE_NO_FUTURE_LEAKAGE") ==
          std::string::npos ||
      replay_environment_bnf.find("LIVE_CAPITAL_ALLOWED") ==
          std::string::npos ||
      channel_mdn_jkimyei_bnf.find("INPUT_MDN_CHECKPOINT") ==
          std::string::npos ||
      graph_node_allocation_jkimyei_bnf.find("TRAINING_SCHEDULE_MODE") ==
          std::string::npos ||
      graph_node_allocation_jkimyei_bnf.find("PPO_EXECUTION_ALLOWED") ==
          std::string::npos) {
    throw std::runtime_error("channel config BNF surface mismatch");
  }
  const auto representation_dsl_text =
      read_text(paths.at("wikimyei_representation_vicreg_dsl_path"));
  const auto protocol_dsl_text =
      read_text(paths.at("kikijyeba_protocol_dsl_path"));
  const auto representation_net_text =
      read_text(paths.at("wikimyei_representation_vicreg_net_path"));
  const auto mtf_dsl_text = read_text(
      paths.at("wikimyei_representation_mtf_jepa_mae_vicreg_dsl_path"));
  const auto mtf_net_text = read_text(
      paths.at("wikimyei_representation_mtf_jepa_mae_vicreg_net_path"));
  const auto mdn_dsl_text =
      read_text(paths.at("wikimyei_inference_expected_value_mdn_dsl_path"));
  const auto mdn_net_text =
      read_text(paths.at("wikimyei_inference_expected_value_mdn_net_path"));
  const auto belief_observer_dsl_text =
      read_text(paths.at("wikimyei_observer_belief_dsl_path"));
  const auto spot_distributional_utility_dsl_text = read_text(paths.at(
      "wikimyei_policy_portfolio_spot_distributional_utility_dsl_path"));
  const auto graph_node_allocation_dsl_text = read_text(
      paths.at("wikimyei_policy_portfolio_graph_node_allocation_dsl_path"));
  const auto graph_node_allocation_net_text = read_text(
      paths.at("wikimyei_policy_portfolio_graph_node_allocation_net_path"));
  const auto replay_environment_dsl_text =
      read_text(paths.at("kikijyeba_environment_replay_dsl_path"));
  const auto representation_spec = vicreg::decode_vicreg_spec_from_split_dsl(
      representation_dsl_text, representation_net_text);
  const auto protocol_variant =
      protocol::decode_protocol_variant_from_dsl(protocol_dsl_text);
  const auto mtf_spec = mtf::decode_mtf_jepa_mae_vicreg_spec_from_split_dsl(
      mtf_dsl_text, mtf_net_text);
  const auto mdn_spec =
      mdn::decode_channel_mdn_spec_from_split_dsl(mdn_dsl_text, mdn_net_text);
  const auto belief_spec =
      belief::decode_belief_observer_spec_from_dsl(belief_observer_dsl_text);
  const auto policy_spec =
      policy::decode_spot_distributional_utility_spec_from_dsl(
          spot_distributional_utility_dsl_text);
  const auto allocation_spec =
      graph_allocation::decode_graph_node_allocation_spec_from_dsl(
          graph_node_allocation_dsl_text);
  const auto allocation_net_spec =
      graph_allocation::decode_graph_node_allocation_net_spec_from_dsl(
          graph_node_allocation_net_text);
  const auto replay_environment_spec =
      environment::decode_replay_environment_spec_from_dsl(
          replay_environment_dsl_text);
  const auto representation_training =
      training::decode_training_run_spec_from_dsl(
          read_text(paths.at("wikimyei_representation_vicreg_jkimyei_path")));
  const auto mtf_training =
      training::decode_training_run_spec_from_dsl(read_text(paths.at(
          "wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path")));
  const auto mdn_training =
      training::decode_training_run_spec_from_dsl(read_text(
          paths.at("wikimyei_inference_expected_value_mdn_jkimyei_path")));
  const auto allocation_training =
      training::decode_training_run_spec_from_dsl(read_text(paths.at(
          "wikimyei_policy_portfolio_graph_node_allocation_jkimyei_path")));

  if (representation_spec.component_assembly_id != "vicreg_v1") {
    throw std::runtime_error("VICReg component id mismatch");
  }
  if (protocol_variant.protocol_id != "cwu_02v" ||
      protocol::protocol_representation_family_name(
          protocol_variant.representation_family) !=
          std::string{"wikimyei.representation.encoding.mtf_jepa_mae_vicreg"} ||
      protocol_variant.observer_family != "wikimyei.observer.belief" ||
      protocol_variant.allocation_policy_family !=
          "wikimyei.policy.portfolio.spot_distributional_utility") {
    throw std::runtime_error("active protocol variant mismatch");
  }
  if (mtf_spec.component_assembly_id != "mtf_jepa_mae_vicreg_v1" ||
      mtf_spec.config.time_scales.size() != 4 ||
      mtf_spec.config.latent_dim != 32 || !mtf_spec.config.use_global_vicreg) {
    throw std::runtime_error("MTF-JEPA-MAE-VICReg config mismatch");
  }
  if (mdn_spec.input_representation_assembly_id !=
      representation_spec.component_assembly_id) {
    throw std::runtime_error("MDN representation reference mismatch");
  }
  if (belief_spec.component_assembly_id != "nodelift_allocation_belief_v1" ||
      belief_spec.input_mdn_assembly_id != mdn_spec.component_assembly_id ||
      belief_spec.accounting_numeraire_policy !=
          "graph_node_from_base_policy" ||
      !belief_spec.projection_validation_required ||
      belief_spec.live_capital_allowed) {
    throw std::runtime_error("belief observer spec mismatch");
  }
  if (policy_spec.component_assembly_id != "spot_distributional_utility_v1" ||
      policy_spec.input_belief_assembly_id !=
          belief_spec.component_assembly_id ||
      policy_spec.optimizer != "projected_gradient" ||
      !policy_spec.require_accounting_numeraire_node ||
      !policy_spec.projection_validation_required ||
      policy_spec.live_capital_allowed) {
    throw std::runtime_error("spot distributional utility spec mismatch");
  }
  if (allocation_spec.component_assembly_id != "graph_node_allocation_v1" ||
      allocation_spec.policy_input_schema !=
          "kikijyeba.environment.policy_input.v1" ||
      allocation_spec.action_adapter != "target_node_weights_simplex.v1" ||
      allocation_spec.action_schema !=
          "kikijyeba.environment.action.target_node_weights.v1" ||
      allocation_spec.reward_contract !=
          "kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_"
          "drawdown.v1" ||
      allocation_spec.scenario_input_policy !=
          "allocation_belief_distributional_summaries_v1" ||
      allocation_spec.raw_mdn_input_allowed ||
      allocation_spec.ppo_implemented || allocation_spec.live_capital_allowed) {
    throw std::runtime_error("graph-node allocation policy spec mismatch");
  }
  if (allocation_net_spec.input_node_feature_dim != 27 ||
      allocation_net_spec.input_global_feature_dim != 8 ||
      allocation_net_spec.output_head != "node_weight_logits" ||
      allocation_net_spec.value_head != "reserved_for_ppo" ||
      allocation_net_spec.action_adapter != "target_node_weights_simplex.v1" ||
      allocation_net_spec.ppo_execution_allowed) {
    throw std::runtime_error("graph-node allocation policy net spec mismatch");
  }
  if (replay_environment_spec.component_assembly_id !=
          "replay_environment_v1" ||
      replay_environment_spec.range_source !=
          "ujcamei_component_stream_cursor" ||
      replay_environment_spec.observation_time_law != "time_t_only" ||
      replay_environment_spec.action_kind != "target_node_weights" ||
      replay_environment_spec.action_schema_id !=
          "kikijyeba.environment.action.target_node_weights.v1" ||
      !replay_environment_spec.require_no_future_leakage ||
      replay_environment_spec.live_capital_allowed) {
    throw std::runtime_error("replay environment spec mismatch");
  }
  expect_throw(
      [&] {
        const auto ignored = vicreg::decode_vicreg_spec_from_split_dsl(
            replace_first(representation_dsl_text,
                          "  CELL_VALID_POLICY = required_features;\n", ""),
            representation_net_text);
        (void)ignored;
      },
      "VICReg DSL must not use implicit cell-valid default");
  expect_throw(
      [&] {
        const auto ignored = vicreg::decode_vicreg_spec_from_split_dsl(
            representation_dsl_text,
            replace_first(representation_net_text,
                          "  VICREG_VARIANCE_FLOOR = 1.0;\n", ""));
        (void)ignored;
      },
      "VICReg NET must not use implicit loss default");
  expect_throw(
      [&] {
        const auto ignored = mdn::decode_channel_mdn_spec_from_split_dsl(
            replace_first(mdn_dsl_text,
                          "  TARGET_MASK_POLICY = per_target_feature_valid;\n",
                          ""),
            mdn_net_text);
        (void)ignored;
      },
      "MDN DSL must not use implicit target-mask default");
  expect_throw(
      [&] {
        const auto ignored = mdn::decode_channel_mdn_spec_from_split_dsl(
            mdn_dsl_text,
            replace_first(mdn_net_text, "  GLOBAL_CONTEXT_DIM = 0;\n", ""));
        (void)ignored;
      },
      "MDN NET must not use implicit global-context default");
  if (mdn_spec.context_mode !=
          mdn::channel_mdn_context_mode_t::channel_context_strict ||
      mdn_spec.global_context_dim != 0) {
    throw std::runtime_error("MDN strict context spec mismatch");
  }
  if (representation_training.task !=
          training::training_task_t::vicreg_representation ||
      representation_training.component_assembly_id !=
          representation_spec.component_assembly_id) {
    throw std::runtime_error("VICReg training spec mismatch");
  }
  if (mtf_training.task !=
          training::training_task_t::mtf_jepa_mae_vicreg_representation ||
      mtf_training.component_assembly_id != mtf_spec.component_assembly_id) {
    throw std::runtime_error("MTF-JEPA-MAE-VICReg training spec mismatch");
  }
  if (mdn_training.task !=
          training::training_task_t::mdn_expected_value_inference ||
      mdn_training.component_assembly_id != mdn_spec.component_assembly_id ||
      !mdn_training.freeze_representation) {
    throw std::runtime_error("MDN training spec mismatch");
  }
  if (allocation_training.task !=
          training::training_task_t::
              policy_graph_node_allocation_contract_smoke ||
      allocation_training.component_assembly_id !=
          allocation_spec.component_assembly_id ||
      allocation_training.optimizer != training::training_optimizer_t::noop ||
      allocation_training.training_schedule_mode !=
          "causal_walk_forward_training.v1" ||
      !allocation_training.require_causal_schedule ||
      allocation_training.ppo_execution_allowed ||
      allocation_training.checkpoint_kind != "policy_contract_fixture") {
    throw std::runtime_error(
        "graph-node allocation policy training spec mismatch");
  }

  auto channel_bundle =
      protocol::load_channel_graph_first_config_bundle_from_config(
          "/cuwacunu/src/config/.config");
  protocol::validate_channel_graph_first_config_bundle(channel_bundle);
  if (channel_bundle.kikijyeba_protocol_dsl_bnf_path.empty() ||
      channel_bundle.kikijyeba_protocol_dsl_path.empty() ||
      channel_bundle.wikimyei_representation_vicreg_dsl_bnf_path.empty() ||
      channel_bundle.wikimyei_representation_vicreg_net_bnf_path.empty() ||
      channel_bundle.wikimyei_representation_mtf_jepa_mae_vicreg_dsl_bnf_path
          .empty() ||
      channel_bundle.wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path
          .empty() ||
      channel_bundle.wikimyei_inference_expected_value_mdn_dsl_bnf_path
          .empty() ||
      channel_bundle.wikimyei_inference_expected_value_mdn_net_bnf_path
          .empty() ||
      channel_bundle.wikimyei_observer_belief_dsl_bnf_path.empty() ||
      channel_bundle
          .wikimyei_policy_portfolio_spot_distributional_utility_dsl_bnf_path
          .empty() ||
      channel_bundle
          .wikimyei_policy_portfolio_graph_node_allocation_dsl_bnf_path
          .empty() ||
      channel_bundle
          .wikimyei_policy_portfolio_graph_node_allocation_net_bnf_path
          .empty() ||
      channel_bundle.kikijyeba_environment_replay_dsl_bnf_path.empty() ||
      channel_bundle.wikimyei_representation_vicreg_jkimyei_bnf_path.empty() ||
      channel_bundle
          .wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_bnf_path
          .empty() ||
      channel_bundle.wikimyei_inference_expected_value_mdn_jkimyei_bnf_path
          .empty() ||
      channel_bundle
          .wikimyei_policy_portfolio_graph_node_allocation_jkimyei_bnf_path
          .empty()) {
    throw std::runtime_error(
        "channel graph-first BNF paths missing from bundle");
  }
  if (channel_bundle.vicreg.component_assembly_id !=
          representation_spec.component_assembly_id ||
      channel_bundle.protocol_variant.protocol_id != "cwu_02v" ||
      protocol::active_representation_family_id(channel_bundle) !=
          "wikimyei.representation.encoding.mtf_jepa_mae_vicreg" ||
      protocol::active_representation_component_assembly_id(channel_bundle) !=
          mtf_spec.component_assembly_id ||
      channel_bundle.mtf_jepa_mae_vicreg.component_assembly_id !=
          mtf_spec.component_assembly_id ||
      channel_bundle.channel_mdn.component_assembly_id !=
          mdn_spec.component_assembly_id ||
      channel_bundle.channel_mdn.input_representation_assembly_id !=
          representation_spec.component_assembly_id ||
      channel_bundle.belief_observer.component_assembly_id !=
          belief_spec.component_assembly_id ||
      channel_bundle.belief_observer.input_mdn_assembly_id !=
          mdn_spec.component_assembly_id ||
      channel_bundle.spot_distributional_utility.component_assembly_id !=
          policy_spec.component_assembly_id ||
      channel_bundle.spot_distributional_utility.input_belief_assembly_id !=
          belief_spec.component_assembly_id ||
      channel_bundle.graph_node_allocation.component_assembly_id !=
          allocation_spec.component_assembly_id ||
      channel_bundle.graph_node_allocation_net.output_head !=
          allocation_net_spec.output_head ||
      channel_bundle.replay_environment.component_assembly_id !=
          replay_environment_spec.component_assembly_id ||
      !channel_bundle.replay_environment.require_projection_validation) {
    throw std::runtime_error("channel graph-first bundle component mismatch");
  }
  if (channel_bundle.vicreg_assembly.family !=
          "wikimyei.representation.encoding.vicreg" ||
      channel_bundle.mtf_jepa_mae_vicreg_assembly.family !=
          "wikimyei.representation.encoding.mtf_jepa_mae_vicreg" ||
      channel_bundle.channel_mdn_assembly.family !=
          "wikimyei.inference.expected_value.mdn" ||
      channel_bundle.belief_observer_assembly.family !=
          "wikimyei.observer.belief" ||
      channel_bundle.spot_distributional_utility_assembly.family !=
          "wikimyei.policy.portfolio.spot_distributional_utility" ||
      channel_bundle.graph_node_allocation_assembly.family !=
          "wikimyei.policy.portfolio.graph_node_allocation") {
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
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      channel_bundle.dock_binding, "G",
      static_cast<int64_t>(
          channel_bundle.source_plan.market_graph.node_ids.size()),
      "channel graph-first graph node count");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      channel_bundle.dock_binding, "Kc",
      protocol::active_channel_count(channel_bundle.source_dock) *
          mdn_spec.mixture_count,
      "channel graph-first channel-consensus mixture count");
  cuwacunu::kikijyeba::topology::require_static_i64_binding_value(
      channel_bundle.dock_binding, "S", belief_spec.scenario_count,
      "channel graph-first observer scenario count");
  cuwacunu::kikijyeba::topology::require_runtime_binding(
      channel_bundle.dock_binding, "A",
      "channel graph-first allocatable asset count");

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
  if (vicreg_train_options.vicreg.invariance_weight !=
          representation_spec.vicreg_invariance_weight ||
      vicreg_train_options.vicreg.variance_weight !=
          representation_spec.vicreg_variance_weight ||
      vicreg_train_options.vicreg.covariance_weight !=
          representation_spec.vicreg_covariance_weight ||
      vicreg_train_options.vicreg.variance_floor !=
          representation_spec.vicreg_variance_floor ||
      vicreg_train_options.vicreg.eps != representation_spec.vicreg_eps ||
      vicreg_train_options.vicreg.global_aux_weight !=
          representation_spec.global_aux_weight ||
      vicreg_train_options.min_valid_rows !=
          representation_spec.min_valid_rows ||
      vicreg_train_options.skip_non_finite_loss !=
          representation_spec.skip_non_finite_loss ||
      vicreg_train_options.jitter_std != representation_spec.jitter_std ||
      vicreg_train_options.feature_dropout_prob !=
          representation_spec.feature_dropout_prob ||
      vicreg_train_options.history_dropout_prob !=
          representation_spec.history_dropout_prob) {
    throw std::runtime_error("VICReg train options mismatch");
  }

  const auto mdn_adapter_options =
      mdn::channel_mdn_adapter_options_from_spec(mdn_spec);
  if (mdn_adapter_options.target_coords != mdn_spec.target_coords ||
      mdn_adapter_options.target_mask_policy !=
          mdn::stream::channel_target_mask_policy_t::per_target_feature_valid) {
    throw std::runtime_error("MDN adapter options mismatch");
  }
  const auto nll_options = mdn::channel_mdn_nll_options_from_spec(mdn_spec);
  if (nll_options.sigma_min != mdn_spec.sigma_min ||
      nll_options.sigma_max != mdn_spec.sigma_max ||
      nll_options.eps != mdn_spec.eps) {
    throw std::runtime_error("MDN NLL options mismatch");
  }

  const auto representation_assembly =
      vicreg::make_vicreg_assembly(representation_spec.component_assembly_id);
  const auto mdn_assembly =
      mdn::make_channel_context_mdn_assembly(mdn_spec.component_assembly_id);
  const auto belief_assembly = belief::make_nodelift_allocation_belief_assembly(
      belief_spec.component_assembly_id, belief_spec.version_token);
  const auto policy_assembly =
      policy::make_spot_distributional_utility_assembly(
          policy_spec.component_assembly_id, policy_spec.version_token);
  const auto allocation_assembly =
      graph_allocation::make_graph_node_allocation_assembly(
          allocation_spec.component_assembly_id, allocation_spec.version_token);
  if (!cuwacunu::wikimyei::assembly::dock_domain_compatible(
          representation_assembly.docks.at(1), mdn_assembly.docks.at(0))) {
    throw std::runtime_error("channel assemblies are not compatible");
  }
  if (!cuwacunu::wikimyei::assembly::dock_domain_compatible(
          mdn_assembly.docks.at(2), belief_assembly.docks.at(0)) ||
      !cuwacunu::wikimyei::assembly::dock_domain_compatible(
          belief_assembly.docks.at(2), policy_assembly.docks.at(0))) {
    throw std::runtime_error("observer/policy assemblies are not compatible");
  }
  if (allocation_assembly.docks.at(1).coordinate_space !=
          "kikijyeba.environment.action.target_node_weights.v1" ||
      allocation_assembly.docks.at(1).value_shape != "TargetNodeWeights[A]") {
    throw std::runtime_error(
        "graph-node allocation policy action dock mismatch");
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
    throw std::runtime_error("old fused representation bound MDN");
  }

  auto bad_representation = representation_spec;
  bad_representation.history_length = 0;
  expect_throw([&] { vicreg::validate_vicreg_spec(bad_representation); },
               "VICReg history length");

  auto bad_augmentation = representation_spec;
  bad_augmentation.feature_dropout_prob = 1.5;
  expect_throw([&] { vicreg::validate_vicreg_spec(bad_augmentation); },
               "VICReg augmentation bounds");

  auto bad_mdn = mdn_spec;
  bad_mdn.target_coords = {0, 0};
  expect_throw([&] { mdn::validate_channel_mdn_spec(bad_mdn); },
               "MDN duplicate target coord");

  auto bad_strict_global = mdn_spec;
  bad_strict_global.global_context_dim = 4;
  expect_throw([&] { mdn::validate_channel_mdn_spec(bad_strict_global); },
               "MDN strict rejects global context dim");

  auto plus_global_mdn = mdn_spec;
  plus_global_mdn.context_mode =
      mdn::channel_mdn_context_mode_t::channel_context_plus_global;
  plus_global_mdn.global_context_dim = 4;
  mdn::validate_channel_mdn_spec(plus_global_mdn);
  const auto plus_global_dsl =
      std::string("MDN {\n"
                  "  VERSION = wikimyei.inference.expected_value.mdn.v1;\n"
                  "  COMPONENT_ASSEMBLY_ID = mdn_plus_global_v1;\n"
                  "  INPUT_REPRESENTATION_ASSEMBLY_ID = vicreg_v1;\n"
                  "  CONTEXT_MODE = channel_context_plus_global;\n"
                  "  TARGET_DOMAIN = channel_node_future;\n"
                  "  TARGET_COORDS = 0,1;\n"
                  "  TARGET_MASK_POLICY = per_target_feature_valid;\n"
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
                                           "  FEATURE_EMBEDDING_DIM = 4;\n"
                                           "  CHANNEL_ADAPTER_RANK = 4;\n"
                                           "  GLOBAL_CONTEXT_DIM = 4;\n"
                                           "};\n");
  const auto decoded_plus_global = mdn::decode_channel_mdn_spec_from_split_dsl(
      plus_global_dsl, plus_global_net);
  if (decoded_plus_global.context_mode !=
          mdn::channel_mdn_context_mode_t::channel_context_plus_global ||
      decoded_plus_global.global_context_dim != 4) {
    throw std::runtime_error("MDN plus-global spec decode mismatch");
  }
  auto bad_plus_global = decoded_plus_global;
  bad_plus_global.global_context_dim = 0;
  expect_throw([&] { mdn::validate_channel_mdn_spec(bad_plus_global); },
               "MDN plus-global requires global context dim");
}

void test_invalid_specs_fail_fast() {
  namespace belief = cuwacunu::wikimyei::observer::belief;
  namespace policy =
      cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility;
  namespace graph_allocation =
      cuwacunu::wikimyei::policy::portfolio::graph_node_allocation;
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

  auto bad_vicreg = vicreg::vicreg_spec_t{};
  bad_vicreg.component_assembly_id = "bad";
  bad_vicreg.channel_count = 1;
  bad_vicreg.history_length = 2;
  bad_vicreg.input_width = 8;
  bad_vicreg.encoding_dim = 1;
  bad_vicreg.feature_hidden_dim = 1;
  bad_vicreg.temporal_depth = 1;
  bad_vicreg.vicreg_projector_dim = 1;
  bad_vicreg.vicreg_projector_hidden_dim = 1;
  bad_vicreg.vicreg_projector_depth = 1;
  expect_throw([&] { vicreg::validate_vicreg_spec(bad_vicreg); },
               "VICReg width");

  bad_vicreg.input_width = 9;
  bad_vicreg.dtype = "float16";
  expect_throw([&] { vicreg::validate_vicreg_spec(bad_vicreg); },
               "VICReg dtype");

  bad_vicreg.dtype = "float32";
  bad_vicreg.device = "gpu:0";
  expect_throw([&] { vicreg::validate_vicreg_spec(bad_vicreg); },
               "VICReg device");

  auto bad_mdn = mdn::channel_mdn_spec_t{};
  bad_mdn.component_assembly_id = "mdn";
  bad_mdn.input_representation_assembly_id = "rep";
  bad_mdn.target_coords = {99};
  bad_mdn.channel_count = 1;
  bad_mdn.future_horizon = 1;
  bad_mdn.mixture_count = 2;
  bad_mdn.hidden_width = 4;
  bad_mdn.feature_embedding_dim = 2;
  bad_mdn.channel_adapter_rank = 2;
  expect_throw([&] { mdn::validate_channel_mdn_spec(bad_mdn); },
               "MDN target coord");

  auto bad_training = training::training_run_spec_t{};
  bad_training.training_id = "train";
  bad_training.component_assembly_id = "mdn";
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

  auto bad_belief = belief::belief_observer_spec_t{};
  bad_belief.accounting_numeraire_policy = "external_cash_bucket";
  expect_throw([&] { belief::validate_belief_observer_spec(bad_belief); },
               "accounting numeraire must be");

  bad_belief = belief::belief_observer_spec_t{};
  bad_belief.live_capital_allowed = true;
  expect_throw([&] { belief::validate_belief_observer_spec(bad_belief); },
               "belief observer cannot allow live capital");

  auto bad_policy = policy::spot_distributional_utility_spec_t{};
  bad_policy.require_accounting_numeraire_node = false;
  expect_throw(
      [&] { policy::validate_spot_distributional_utility_spec(bad_policy); },
      "accounting numeraire graph node");

  bad_policy = policy::spot_distributional_utility_spec_t{};
  bad_policy.projection_validation_required = false;
  expect_throw(
      [&] { policy::validate_spot_distributional_utility_spec(bad_policy); },
      "policy projection validation required");

  auto bad_allocation = graph_allocation::graph_node_allocation_spec_t{};
  bad_allocation.ppo_implemented = true;
  expect_throw(
      [&] {
        graph_allocation::validate_graph_node_allocation_spec(bad_allocation);
      },
      "graph-node allocation PPO disabled");

  auto bad_allocation_net =
      graph_allocation::graph_node_allocation_net_spec_t{};
  bad_allocation_net.ppo_execution_allowed = true;
  expect_throw(
      [&] {
        graph_allocation::validate_graph_node_allocation_net_spec(
            bad_allocation_net);
      },
      "graph-node allocation net PPO disabled");

  auto bad_policy_training = training::training_run_spec_t{};
  bad_policy_training.version_token =
      "wikimyei.policy.portfolio.graph_node_allocation.jkimyei.v1";
  bad_policy_training.training_id = "policy_contract";
  bad_policy_training.task =
      training::training_task_t::policy_graph_node_allocation_contract_smoke;
  bad_policy_training.component_assembly_id = "graph_node_allocation_v1";
  bad_policy_training.optimizer = training::training_optimizer_t::adam;
  bad_policy_training.batch_size = 1;
  bad_policy_training.training_schedule_mode =
      "causal_walk_forward_training.v1";
  bad_policy_training.require_causal_schedule = true;
  bad_policy_training.checkpoint_kind = "policy_contract_fixture";
  expect_throw(
      [&] { training::validate_training_run_spec(bad_policy_training); },
      "policy contract smoke rejects adam");
}

void test_cross_reference_failures() {
  namespace protocol = cuwacunu::kikijyeba::protocol;

  auto channel_bundle =
      protocol::load_channel_graph_first_config_bundle_from_config(
          "/cuwacunu/src/config/.config");
  channel_bundle.protocol_variant = protocol::decode_protocol_variant_from_dsl(
      read_text("/cuwacunu/src/config/kikijyeba.protocol.cwu_01v.dsl"));
  channel_bundle.wave_settings.compatible_protocol_ids = {"cwu_01v"};
  channel_bundle.channel_mdn.input_representation_assembly_id =
      "other_channel_rep";
  expect_throw(
      [&] {
        protocol::validate_channel_graph_first_config_bundle(channel_bundle);
      },
      "MDN representation mismatch");

  channel_bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      "/cuwacunu/src/config/.config");
  channel_bundle.protocol_variant = protocol::decode_protocol_variant_from_dsl(
      read_text("/cuwacunu/src/config/kikijyeba.protocol.cwu_01v.dsl"));
  channel_bundle.wave_settings.compatible_protocol_ids = {"cwu_01v"};
  channel_bundle.channel_mdn_assembly.docks.front().coordinate_space =
      "graph_order.node_representation.v1";
  expect_throw(
      [&] {
        protocol::validate_channel_graph_first_config_bundle(channel_bundle);
      },
      "channel assembly coordinate mismatch");

  channel_bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      "/cuwacunu/src/config/.config");
  channel_bundle.belief_observer.input_mdn_assembly_id = "other_mdn";
  expect_throw(
      [&] {
        protocol::validate_channel_graph_first_config_bundle(channel_bundle);
      },
      "belief observer MDN mismatch");

  channel_bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      "/cuwacunu/src/config/.config");
  channel_bundle.spot_distributional_utility.input_belief_assembly_id =
      "other_belief";
  expect_throw(
      [&] {
        protocol::validate_channel_graph_first_config_bundle(channel_bundle);
      },
      "spot policy belief mismatch");

  channel_bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      "/cuwacunu/src/config/.config");
  channel_bundle.belief_observer_assembly.docks.front().coordinate_space =
      "wrong.future_distribution.v1";
  expect_throw(
      [&] {
        protocol::validate_channel_graph_first_config_bundle(channel_bundle);
      },
      "belief observer input dock mismatch");

  channel_bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      "/cuwacunu/src/config/.config");
  channel_bundle.protocol_variant = protocol::decode_protocol_variant_from_dsl(
      read_text("/cuwacunu/src/config/kikijyeba.protocol.cwu_01v.dsl"));
  channel_bundle.wave_settings.compatible_protocol_ids = {"cwu_01v"};
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

void test_protocol_policy_identity_required() {
  namespace protocol = cuwacunu::kikijyeba::protocol;

  expect_throw(
      [] {
        (void)protocol::decode_protocol_variant_from_dsl(
            "PROTOCOL {\n"
            "  PROTOCOL_ID = cwu_bad;\n"
            "  PROTOCOL_KIND = channel_graph_first;\n"
            "  GRAPH_TOPOLOGY = kikijyeba.topology.graph;\n"
            "  NODELIFT = wikimyei.expression.nodelift.srl;\n"
            "  REPRESENTATION = "
            "wikimyei.representation.encoding.mtf_jepa_mae_vicreg;\n"
            "  INFERENCE = wikimyei.inference.expected_value.mdn;\n"
            "  OBSERVER = wikimyei.observer.belief;\n"
            "  REPRESENTATION_CONTRACT = "
            "graph_order.channel_node_representation.v1;\n"
            "};\n");
      },
      "missing allocation policy");

  expect_throw(
      [] {
        (void)protocol::decode_protocol_variant_from_dsl(
            "PROTOCOL {\n"
            "  PROTOCOL_ID = cwu_bad;\n"
            "  PROTOCOL_KIND = channel_graph_first;\n"
            "  GRAPH_TOPOLOGY = kikijyeba.topology.graph;\n"
            "  NODELIFT = wikimyei.expression.nodelift.srl;\n"
            "  REPRESENTATION = "
            "wikimyei.representation.encoding.mtf_jepa_mae_vicreg;\n"
            "  INFERENCE = wikimyei.inference.expected_value.mdn;\n"
            "  OBSERVER = wikimyei.observer.other;\n"
            "  ALLOCATION_POLICY = "
            "wikimyei.policy.portfolio.spot_distributional_utility;\n"
            "  REPRESENTATION_CONTRACT = "
            "graph_order.channel_node_representation.v1;\n"
            "};\n");
      },
      "invalid observer");

  auto bundle = protocol::load_channel_graph_first_config_bundle_from_config(
      "/cuwacunu/src/config/.config");
  const std::string contract_text =
      protocol::canonical_channel_graph_first_protocol_contract_text(bundle);
  if (contract_text.find("protocol_observer=wikimyei.observer.belief") ==
          std::string::npos ||
      contract_text.find("protocol_allocation_policy=wikimyei.policy.portfolio."
                         "spot_distributional_utility") == std::string::npos) {
    throw std::runtime_error(
        "protocol contract text missing observer/allocation policy identity");
  }
}

} // namespace

int main() {
  test_runtime_checkpoint_inputs_do_not_change_protocol_contract_identity();
  test_channel_protocol_contract_identity_binds_strict_channel_config();
  test_channel_specs_decode_and_validate();
  test_invalid_specs_fail_fast();
  test_cross_reference_failures();
  test_protocol_policy_identity_required();
  std::cout << "[test_wikimyei_graph_first_specs] all checks passed\n";
  return 0;
}
