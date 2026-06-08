#include "hero/lattice_hero/lattice/exposure/exposure_ledger.h"
#include "hero/lattice_hero/lattice/target/lattice_target_evaluator.h"
#include "hero/marshal_hero/marshal/digest.h"
#include "hero/runtime_hero/hero_runtime_tools.h"
#include "hero/runtime_hero/runtime/policy_training_causal_schedule.h"
#include "hero/runtime_hero/runtime/wave_settings.h"
#include "wikimyei/assembly.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <unistd.h>

namespace hero_runtime = cuwacunu::hero::runtime;
namespace lattice_exposure = cuwacunu::hero::lattice::exposure;
namespace lattice_target = cuwacunu::hero::lattice::target;
namespace wave_settings = cuwacunu::hero::runtime::settings;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::string join_issue_tokens(const std::vector<std::string> &issues) {
  std::ostringstream out;
  for (std::size_t i = 0; i < issues.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << issues[i];
  }
  return out.str();
}

void replace_all(std::string &text, const std::string &from,
                 const std::string &to) {
  check(!from.empty(), "replace_all requires a non-empty search string");
  std::size_t pos = 0;
  while ((pos = text.find(from, pos)) != std::string::npos) {
    text.replace(pos, from.size(), to);
    pos += to.size();
  }
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  static int counter = 0;
  auto dir = std::filesystem::temp_directory_path() /
             ("cuwacunu_hero_runtime_wave_" + label + "_" +
              std::to_string(static_cast<long long>(::getpid())) + "_" +
              std::to_string(counter++));
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir;
}

void write_text(const std::filesystem::path &path, const std::string &text) {
  std::ofstream out(path, std::ios::trunc);
  check(out.is_open(), "failed to open text output");
  out << text;
}

std::string read_text(const std::filesystem::path &path) {
  std::ifstream in(path);
  check(in.is_open(), "failed to open text input: " + path.string());
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

std::string json_string_field(const std::string &json,
                              const std::string &field) {
  const std::string needle = "\"" + field + "\":\"";
  const auto pos = json.find(needle);
  check(pos != std::string::npos, "missing JSON string field: " + field);
  const auto begin = pos + needle.size();
  const auto end = json.find("\"", begin);
  check(end != std::string::npos, "unterminated JSON string field: " + field);
  return json.substr(begin, end - begin);
}

std::string digest_file_for_handoff(const std::filesystem::path &path,
                                    const std::string &domain) {
  std::ifstream in(path, std::ios::binary);
  check(in.is_open(), "failed to open digest input");
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return cuwacunu::hero::marshal::marshal_digest_for_text(domain, buffer.str());
}

std::string replay_report_digest_for_text(const std::string &text) {
  std::uint64_t hash =
      cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, "kikijyeba.lattice.exposure.v1");
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash,
      std::string("kikijyeba.environment.replay.report_digest.v1\n") + text);
  return cuwacunu::wikimyei::assembly::assembly_detail::hash_hex(hash);
}

struct ppo_rehearsal_parent_fact_bundle_t {
  lattice_exposure::lattice_exposure_ledger_t ledger{};
  lattice_exposure::lattice_exposure_fact_t parent_fact{};
  std::string forecast_eval_digest{};
  std::string observer_belief_digest{};
  std::string allocation_engine_digest{};
  std::string replay_environment_digest{};
};

ppo_rehearsal_parent_fact_bundle_t make_ppo_rehearsal_parent_fact_bundle() {
  ppo_rehearsal_parent_fact_bundle_t bundle{};
  const lattice_exposure::anchor_interval_t rehearsal_range{0, 10};
  bundle.parent_fact.contract_fingerprint = "contract_1";
  bundle.parent_fact.protocol_id = "cwu_02v";
  bundle.parent_fact.graph_order_fingerprint = "graph_1";
  bundle.parent_fact.source_cursor_token = "cursor_1";
  bundle.parent_fact.split_policy_fingerprint = "split_policy_1";
  bundle.parent_fact.component_assembly_fingerprint = "policy_trainable_1";
  bundle.parent_fact.target_component_family_id = "wikimyei.policy.trainable";
  bundle.parent_fact.job_id = "ppo_on_policy_parent";
  bundle.parent_fact.wave_id = "policy_training_ppo_v0";
  bundle.parent_fact.job_status = "completed";
  bundle.parent_fact.wave_action = "policy_training";
  bundle.parent_fact.split_name = "train";
  bundle.parent_fact.split_role =
      lattice_exposure::exposure_split_role_t::train;
  bundle.parent_fact.anchor_range = rehearsal_range;
  bundle.parent_fact.completed_anchor_range = rehearsal_range;
  const auto parent_digest =
      lattice_exposure::exposure_fact_digest(bundle.parent_fact);

  const auto bind_common = [&](auto &fact, const std::string &component,
                               const std::string &assembly) {
    fact.parent_exposure_fact_digest = parent_digest;
    fact.contract_fingerprint = "contract_1";
    fact.protocol_id = "cwu_02v";
    fact.graph_order_fingerprint = "graph_1";
    fact.source_cursor_token = "cursor_1";
    fact.split_policy_fingerprint = "split_policy_1";
    fact.component_assembly_fingerprint = assembly;
    fact.target_component_family_id = component;
    fact.job_id = "ppo_on_policy_parent";
    fact.wave_id = "policy_training_ppo_v0";
    fact.job_status = "completed";
    fact.wave_action = "policy_training";
    fact.split_name = "train";
    fact.split_role = lattice_exposure::exposure_split_role_t::train;
    fact.anchor_range = rehearsal_range;
    fact.completed_anchor_range = rehearsal_range;
  };

  lattice_exposure::lattice_forecast_eval_fact_t forecast{};
  bind_common(forecast, "wikimyei.inference.expected_value.mdn",
              "mdn_assembly_1");
  forecast.target_feature_ids = {"log_return"};
  forecast.horizon = 1;
  forecast.support_count = 10;
  forecast.valid_count = 10;
  forecast.missing_count = 0;
  forecast.weakest_support_rows = 10;
  forecast.mean_nll = 1.0;
  forecast.mean_nll_per_horizon = {1.0};
  forecast.valid_target_count_per_horizon = {10};
  forecast.ev_mae = 0.10;
  forecast.ev_rmse = 0.15;
  forecast.signed_error = 0.01;
  forecast.directional_accuracy = 0.60;
  forecast.calibration_coverage = 0.90;
  forecast.pit_summary = "fixture_uniform";
  forecast.sigma_scale_sanity = "ok";
  forecast.support_by_node = "USDT:10,BTC:10,ETH:10";
  forecast.support_by_channel = "close:10";
  forecast.support_by_target_feature = "log_return:10";
  forecast.support_by_horizon = "h1:10";
  forecast.forecast_artifact_digest = "forecast_artifact_digest_v1";
  forecast.evaluated_representation_checkpoint_digest =
      "representation_checkpoint_digest_v1";
  forecast.evaluated_mdn_checkpoint_digest = "mdn_checkpoint_digest_v1";
  forecast.target_transform_fact_digest = "target_transform_digest_v1";
  bundle.ledger.add_forecast_eval(forecast);
  bundle.forecast_eval_digest =
      lattice_exposure::forecast_eval_fact_digest(forecast);

  lattice_exposure::lattice_observer_belief_fact_t observer{};
  bind_common(observer, "wikimyei.inference.expected_value.mdn",
              "observer_belief_assembly_1");
  observer.belief_kind = "allocation_belief";
  observer.channel_consensus = "USDT:0.80,BTC:0.62,ETH:0.58";
  observer.potential_surface_diagnostics = "stable_surface";
  observer.nodelift_return_projection = "allocation_ready";
  observer.covariance_coupling = "compact_risk_summary";
  observer.scenario_bank_digest = "scenario_bank_digest_v1";
  observer.nodelift_residual_quality = "ok";
  observer.projection_validation_scores = "rmse:0.15";
  observer.confidence = 0.70;
  observer.data_quality = 0.90;
  observer.liquidity = 0.80;
  observer.forecast_artifact_digest = forecast.forecast_artifact_digest;
  observer.forecast_artifact_lineage = bundle.forecast_eval_digest;
  observer.feature_semantics_fingerprint = "feature_semantics_digest_v1";
  observer.dock_binding_fingerprint = "dock_binding_digest_v1";
  bundle.ledger.add_observer_belief(observer);
  bundle.observer_belief_digest =
      lattice_exposure::observer_belief_fact_digest(observer);

  lattice_exposure::lattice_allocation_engine_fact_t allocation{};
  bind_common(allocation, "wikimyei.policy.portfolio.graph_node_allocation",
              "graph_node_allocation_assembly_1");
  allocation.target_node_weights = "USDT:0.25,BTC:0.45,ETH:0.30";
  allocation.accounting_numeraire_node_id = "USDT";
  allocation.accounting_numeraire_node_source = "global_config_accounting";
  allocation.base_policy_accounting_numeraire_node_id = "USDT";
  allocation.accounting_numeraire_node_graph_bound = true;
  allocation.numeraire_weight = 0.25;
  allocation.turnover = 0.14;
  allocation.objective_terms = "post_execution_log_growth,cost,drawdown";
  allocation.cvar_loss = 0.04;
  allocation.transaction_cost_estimate = 0.00042;
  allocation.constraint_diagnostics = "unified_target_node_weights_ok";
  allocation.cap_diagnostics = "node_caps_ok";
  allocation.scenario_growth_floor_status = "met";
  allocation.fallback_reasons = "none";
  allocation.derisk_reasons = "none";
  allocation.observer_belief_fact_digest = bundle.observer_belief_digest;
  allocation.forecast_artifact_digest = forecast.forecast_artifact_digest;
  allocation.base_policy_digest = "graph_node_allocation_base_policy_digest_v1";
  bundle.ledger.add_allocation_engine(allocation);
  bundle.allocation_engine_digest =
      lattice_exposure::allocation_engine_fact_digest(allocation);

  lattice_exposure::lattice_replay_environment_fact_t replay{};
  bind_common(replay, "wikimyei.inference.expected_value.mdn",
              "replay_environment_assembly_1");
  replay.batch_index_path =
      "/tmp/ppo_v0_rehearsal/runtime_replay_batches.index";
  replay.experiment_index_path =
      "/tmp/ppo_v0_rehearsal/runtime_replay_experiments.index";
  replay.experiment_report_path =
      "/tmp/ppo_v0_rehearsal/replay_validation.report";
  replay.runtime_replay_batch_index_schema =
      "kikijyeba.environment.replay.runtime_batch_index.v1";
  replay.runtime_replay_experiment_index_schema =
      "kikijyeba.environment.replay.runtime_experiment_index.v1";
  replay.replay_experiment_report_schema =
      lattice_exposure::replay_environment_experiment_report_schema_id();
  replay.experiment_index_report_digest = "replay_report_digest_v1";
  replay.experiment_report_digest = "replay_report_digest_v1";
  replay.experiment_id = "ppo_v0_rehearsal_replay";
  replay.runtime_run_id = "runtime_on_policy_fixture";
  replay.environment_run_id = "env_on_policy_fixture";
  replay.execution_profile_digest = "execution_profile_digest_v1";
  replay.policy_set_digest = "policy_set_digest_v1";
  replay.replay_environment_version = "kikijyeba.environment.replay.v1";
  replay.replay_environment_component_assembly_id = "replay_environment_v1";
  replay.replay_environment_world_mode = "historical_replay";
  replay.replay_environment_api_contract = "rl_compatible_reset_step";
  replay.replay_environment_spawn_model = "episode_parallel_step_sequential";
  replay.replay_environment_range_source = "ujcamei_component_stream_cursor";
  replay.replay_environment_source_range_policy = "anchor_index_or_source_key";
  replay.replay_environment_source_order_policy = "sequential";
  replay.replay_environment_range_resolution =
      "runtime_resolved_cursor_identity";
  replay.replay_environment_observation_time_law = "time_t_only";
  replay.replay_environment_realization_reveal = "after_action_execution";
  replay.replay_environment_realization_key_policy = "shared_key_per_frame";
  replay.replay_environment_action_kind = "target_node_weights";
  replay.replay_environment_action_time_policy =
      "decision_timestamp_after_knowledge_before_realization";
  replay.replay_environment_graph_node_universe_policy =
      "episode_spec_graph_node_ids";
  replay.replay_environment_reward_policy =
      "post_execution_ledger_log_growth_drawdown_cost_turnover_invalid";
  replay.replay_environment_projection_validation =
      "projected_log_return_vs_realized_asset_numeraire_return";
  replay.replay_environment_policy_surface = "policy_adapter";
  replay.replay_environment_action_policy_identity =
      "policy_adapter_must_match_action";
  replay.replay_environment_initial_policy_kind =
      "deterministic_allocator_or_baseline";
  replay.replay_environment_experiment_task_identity =
      "bundle_policy_task_indices";
  replay.replay_environment_experiment_run_identity =
      "single_runtime_environment_run";
  replay.replay_environment_step_artifact_identity =
      "episode_run_policy_cursor";
  replay.replay_environment_experiment_report_count_policy =
      "counts_match_evidence";
  replay.replay_environment_artifact_schema = "cajtucu_ready_replay_artifacts";
  replay.replay_environment_lattice_fact_family = "replay_environment";
  replay.replay_environment_lattice_target =
      "replay_environment_artifact_ready";
  replay.replay_environment_require_resolved_cursor = true;
  replay.replay_environment_require_no_future_leakage = true;
  replay.replay_environment_require_projection_validation = true;
  replay.replay_environment_default_max_parallel_jobs = 1;
  replay.experiment_requested_max_parallel_jobs = 1;
  replay.experiment_resolved_parallelism = 1;
  replay.batch_entry_count = 1;
  replay.experiment_entry_count = 1;
  replay.replay_bundle_count = 1;
  replay.policy_count = 1;
  replay.policy_summary_count = 1;
  replay.attempted_count = 1;
  replay.completed_count = 1;
  replay.failed_count = 0;
  replay.episode_count = 1;
  replay.episode_requested_range_bound_count = 1;
  replay.episode_cursor_bound_count = 1;
  replay.episode_anchor_interval_bound_count = 1;
  replay.episode_anchor_keys_bound_count = 1;
  replay.time_law_expected_step_count = 2;
  replay.time_law_observation_step_count = 2;
  replay.time_law_action_step_count = 2;
  replay.time_law_execution_step_count = 2;
  replay.time_law_realization_after_action_count = 2;
  replay.time_law_future_observation_violation_count = 0;
  replay.mixed_future_realization_key_count = 0;
  replay.projection_validation_step_count = 2;
  replay.cajtucu_valid_trace_count = 2;
  replay.cajtucu_invalid_trace_count = 0;
  replay.cajtucu_missing_direct_pair_count = 0;
  replay.cajtucu_numeraire_fallback_pair_count = 0;
  replay.cajtucu_synthetic_market_step_count = 0;
  replay.requested_order_count = 2;
  replay.executed_order_count = 2;
  replay.rejected_order_count = 0;
  replay.partial_order_count = 0;
  replay.mean_total_reward = 0.001;
  replay.mean_total_log_growth = 0.00175;
  replay.mean_final_equity_numeraire = 100.10;
  replay.mean_max_drawdown = 0.01;
  replay.mean_total_turnover = 0.28;
  replay.mean_total_transaction_cost_numeraire = 0.00042;
  replay.requested_notional_numeraire = 1.0;
  replay.executed_notional_numeraire = 1.0;
  replay.rejected_notional_numeraire = 0.0;
  replay.fill_ratio = 1.0;
  replay.fee_cost_numeraire = 0.00010;
  replay.spread_cost_numeraire = 0.00014;
  replay.slippage_cost_numeraire = 0.00018;
  replay.mean_target_weight_error_l1 = 0.0;
  replay.mean_target_weight_error_linf = 0.0;
  replay.mean_projection_mae = 0.01;
  replay.mean_projection_signed_bias = 0.001;
  replay.mean_projection_directional_accuracy = 0.60;
  replay.mean_projection_interval_coverage = 0.80;
  bundle.ledger.add_replay_environment(replay);
  bundle.replay_environment_digest =
      lattice_exposure::replay_environment_fact_digest(replay);
  return bundle;
}

std::string run_wave_preview(const std::string &target, const std::string &mode,
                             const std::string &source_order = {},
                             const std::string &protocol_text = {},
                             const std::string &compatible_protocols = {}) {
  const auto dir = make_tmp_dir(target);
  const auto wave_path = dir / "hero.runtime.wave.dsl";
  const auto protocol_path = dir / "kikijyeba.protocol.dsl";
  const auto config_path = dir / ".config";

  std::ostringstream wave;
  wave << "WAVE_SETTINGS {\n"
       << "  WAVE_ID = test_wave;\n"
       << (compatible_protocols.empty()
               ? ""
               : "  COMPATIBLE_PROTOCOLS = " + compatible_protocols + ";\n")
       << "  TARGET = " << target << ";\n"
       << "  MODE = " << mode << ";\n"
       << "  SOURCE_CURSOR_KIND = graph_anchor;\n"
       << "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
       << "  SOURCE_RANGE = anchor_index;\n"
       << (source_order.empty() ? ""
                                : "  SOURCE_ORDER = " + source_order + ";\n")
       << "  ANCHOR_INDEX_BEGIN = 1;\n"
       << "  ANCHOR_INDEX_END = 3;\n"
       << "};\n";
  write_text(wave_path, wave.str());
  if (!protocol_text.empty()) {
    write_text(protocol_path, protocol_text);
  }

  std::ostringstream config;
  config << "[HERO]\n"
         << "runtime_wave_dsl_path = " << wave_path.string() << "\n";
  if (!protocol_text.empty()) {
    config << "[KIKIJYEBA]\n"
           << "kikijyeba_protocol_dsl_path = " << protocol_path.string()
           << "\n";
  }
  write_text(config_path, config.str());

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;

  std::string result;
  std::string error;
  const bool ok = hero_runtime::execute_tool_json(
      "hero.runtime.inspect",
      "{\"subject\":\"wave\",\"config_path\":\"" + config_path.string() + "\"}",
      &ctx, &result, &error);
  check(ok, "hero.runtime.inspect subject=wave failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "hero.runtime.inspect subject=wave returned error: " + result);
  std::filesystem::remove_all(dir);
  return result;
}

void require_contains(const std::string &text, std::string_view needle,
                      const std::string &message) {
  check(text.find(needle) != std::string::npos,
        message + " missing needle: " + std::string(needle) + "\n" + text);
}

bool has_issue_containing(const std::vector<std::string> &issues,
                          const std::string &needle) {
  return std::any_of(issues.begin(), issues.end(), [&](const auto &issue) {
    return issue.find(needle) != std::string::npos;
  });
}

std::string cwu02_mtf_protocol_text() {
  return std::string("PROTOCOL {\n"
                     "  PROTOCOL_ID = cwu_02v;\n"
                     "  PROTOCOL_KIND = channel_graph_first;\n"
                     "  GRAPH_TOPOLOGY = kikijyeba.topology.graph;\n"
                     "  NODELIFT = wikimyei.expression.nodelift.srl;\n"
                     "  REPRESENTATION = "
                     "wikimyei.representation.encoding.mtf_jepa_mae_vicreg;\n"
                     "  INFERENCE = wikimyei.inference.expected_value.mdn;\n"
                     "  OBSERVER = wikimyei.observer.belief;\n"
                     "  ALLOCATION_POLICY = "
                     "wikimyei.policy.portfolio.spot_distributional_utility;\n"
                     "  REPRESENTATION_CONTRACT = "
                     "graph_order.channel_node_representation.v1;\n"
                     "};\n");
}

void test_wave_settings_train_defaults_to_random_source_order() {
  const auto implicit_train = wave_settings::decode_wave_settings_from_dsl(
      "WAVE_SETTINGS {\n"
      "  WAVE_ID = train_default;\n"
      "  COMPATIBLE_PROTOCOLS = cwu_01v,cwu_02v;\n"
      "  TARGET = wikimyei.representation.encoding.vicreg;\n"
      "  MODE = train;\n"
      "  SOURCE_RANGE = all;\n"
      "};\n");
  check(implicit_train.source_order_policy ==
            wave_settings::wave_source_order_policy_t::random_per_epoch,
        "train waves must default to random_per_epoch SOURCE_ORDER");
  check(implicit_train.compatible_protocol_ids.size() == 2 &&
            implicit_train.compatible_protocol_ids[0] == "cwu_01v" &&
            implicit_train.compatible_protocol_ids[1] == "cwu_02v",
        "COMPATIBLE_PROTOCOLS should parse as ordered protocol ids");
  check(wave_settings::wave_supports_protocol(implicit_train, "cwu_02v"),
        "parsed compatible protocols should include cwu_02v");
  check(!wave_settings::wave_supports_protocol(implicit_train, "cwu_03v"),
        "parsed compatible protocols should reject unknown protocol");
  check(!implicit_train.source_order_policy_explicit,
        "omitted train SOURCE_ORDER must remain implicit");
  check(std::string(wave_settings::source_order_warning_token(implicit_train))
            .empty(),
        "implicit train random SOURCE_ORDER must not warn");

  const auto explicit_train = wave_settings::decode_wave_settings_from_dsl(
      "WAVE_SETTINGS {\n"
      "  WAVE_ID = train_sequential;\n"
      "  TARGET = wikimyei.representation.encoding.vicreg;\n"
      "  MODE = train;\n"
      "  SOURCE_RANGE = all;\n"
      "  SOURCE_ORDER = sequential;\n"
      "};\n");
  check(explicit_train.source_order_policy ==
            wave_settings::wave_source_order_policy_t::sequential,
        "explicit sequential train SOURCE_ORDER must be preserved");
  check(explicit_train.source_order_policy_explicit,
        "explicit train SOURCE_ORDER must be marked explicit");
  check(std::string(wave_settings::source_order_warning_token(
            explicit_train)) == "train_wave_explicit_sequential_source_order",
        "explicit sequential train SOURCE_ORDER must warn");
}

void test_multi_wave_catalog_selection() {
  const std::string catalog =
      "WAVE_SELECTION {\n"
      "  ACTIVE_WAVE_ID = eval_wave;\n"
      "};\n"
      "WAVE_SETTINGS {\n"
      "  WAVE_ID = eval_wave;\n"
      "  TARGET = wikimyei.inference.expected_value.mdn;\n"
      "  MODE = run|debug;\n"
      "  SOURCE_RANGE = all;\n"
      "};\n"
      "WAVE_SETTINGS {\n"
      "  WAVE_ID = train_wave;\n"
      "  TARGET = wikimyei.representation.encoding.mtf_jepa_mae_vicreg;\n"
      "  MODE = train|debug;\n"
      "  SOURCE_RANGE = all;\n"
      "  SOURCE_ORDER = random_per_epoch;\n"
      "};\n";
  const auto fallback = wave_settings::decode_wave_settings_from_dsl(catalog);
  check(fallback.wave_id == "eval_wave",
        "WAVE_SELECTION should select the default wave");
  check(fallback.action == wave_settings::wave_action_t::run,
        "selected fallback wave should be the eval/run profile");

  const auto selected =
      wave_settings::decode_wave_settings_from_dsl(catalog, "train_wave");
  check(selected.wave_id == "train_wave",
        "config wave id should select matching WAVE_SETTINGS block");
  check(selected.action == wave_settings::wave_action_t::train,
        "selected train wave should decode train action");

  bool refused_missing_selection = false;
  try {
    (void)wave_settings::decode_wave_settings_from_dsl(
        "WAVE_SETTINGS {\n"
        "  WAVE_ID = a;\n"
        "  TARGET = wikimyei.inference.expected_value.mdn;\n"
        "  MODE = run;\n"
        "};\n"
        "WAVE_SETTINGS {\n"
        "  WAVE_ID = b;\n"
        "  TARGET = wikimyei.inference.expected_value.mdn;\n"
        "  MODE = run;\n"
        "};\n");
  } catch (const std::exception &ex) {
    refused_missing_selection =
        std::string(ex.what()).find("multiple WAVE_SETTINGS") !=
        std::string::npos;
  }
  check(refused_missing_selection,
        "multiple WAVE_SETTINGS blocks must require explicit selection");

  const auto dir = make_tmp_dir("multi_wave_catalog_selection");
  const auto wave_path = dir / "hero.runtime.wave.dsl";
  const auto config_path = dir / ".config";
  write_text(wave_path, catalog);
  write_text(config_path, "[HERO]\n"
                          "runtime_wave_dsl_path = " +
                              wave_path.string() +
                              "\n"
                              "runtime_wave_id = train_wave\n");

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;
  std::string result;
  std::string error;
  check(hero_runtime::execute_tool_json(
            "hero.runtime.inspect",
            "{\"subject\":\"wave\",\"config_path\":\"" + config_path.string() +
                "\"}",
            &ctx, &result, &error),
        "Runtime wave inspect should decode selected catalog wave: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "Runtime wave inspect should not return error: " + result);
  require_contains(result, "\"selected_wave_id\":\"train_wave\"",
                   "Runtime inspect should expose selected wave id");
  require_contains(result, "\"selected_wave_source\":\"global_config\"",
                   "Runtime inspect should expose config selection source");
  require_contains(result,
                   "\"target_component_family_id\":\"wikimyei.representation."
                   "encoding.mtf_jepa_mae_vicreg\"",
                   "Runtime inspect should expose selected wave target");

  std::filesystem::remove_all(dir);
}

void test_runtime_policy_profiles() {
  const auto dir = make_tmp_dir("runtime_policy_profiles");
  const auto config_path = dir / ".config";
  const auto policy_path = dir / "hero.runtime.dsl";
  const auto runtime_root = dir / "runtime";
  std::filesystem::create_directories(runtime_root);

  write_text(config_path, "[HERO]\n"
                          "runtime_hero_dsl_path = " +
                              policy_path.string() +
                              "\n"
                              "runtime_hero_profile = long_train_operator\n");
  write_text(policy_path, "protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO\n"
                          "runtime_profile:enum = operator_default\n"
                          "runtime_exec_path:path = /bin/true\n"
                          "default_config_path:path = " +
                              config_path.string() +
                              "\n"
                              "runtime_root:path = " +
                              runtime_root.string() +
                              "\n"
                              "allowed_job_roots:path_list = " +
                              runtime_root.string() +
                              "\n"
                              "require_confirm_execute:bool = true\n"
                              "require_confirm_dev_nuke:bool = true\n"
                              "max_capture_bytes:int = 4096\n"
                              "RUNTIME_PROFILE operator_default {\n"
                              "  default_dry_run:bool = true\n"
                              "  allow_execute:bool = true\n"
                              "  allow_train_execute:bool = true\n"
                              "  allow_dev_nuke:bool = true\n"
                              "  max_runtime_seconds:int = 5\n"
                              "}\n"
                              "RUNTIME_PROFILE long_train_operator {\n"
                              "  default_dry_run:bool = true\n"
                              "  allow_execute:bool = true\n"
                              "  allow_train_execute:bool = true\n"
                              "  allow_dev_nuke:bool = false\n"
                              "  max_runtime_seconds:int = 21600\n"
                              "}\n");

  hero_runtime::runtime_policy_t policy{};
  std::string error;
  check(hero_runtime::load_runtime_policy(policy_path, config_path, &policy,
                                          &error),
        "profile policy should load: " + error);
  check(policy.profile_id == "long_train_operator",
        "global config runtime_hero_profile should select long_train_operator");
  check(policy.values.at("allow_execute") == "true",
        "long_train_operator should allow execute");
  check(policy.values.at("allow_train_execute") == "true",
        "long_train_operator should allow train execute");
  check(policy.values.at("allow_dev_nuke") == "false",
        "long_train_operator should disable developer reset");

  policy = {};
  error.clear();
  check(hero_runtime::load_runtime_policy(policy_path, config_path, &policy,
                                          &error, "operator_default"),
        "profile override should load operator_default: " + error);
  check(policy.profile_id == "operator_default",
        "explicit profile override should win");
  check(policy.values.at("allow_execute") == "true",
        "operator_default should allow confirmed non-live execute");
  check(policy.values.at("allow_train_execute") == "true",
        "operator_default should allow confirmed non-live train execute");
  check(policy.values.at("allow_dev_nuke") == "true",
        "operator_default should keep guarded developer reset available");

  policy = {};
  error.clear();
  check(!hero_runtime::load_runtime_policy(policy_path, config_path, &policy,
                                           &error, "missing_profile"),
        "unknown runtime profile should fail closed");
  check(error.find("unknown runtime profile") != std::string::npos,
        "unknown profile failure should be explicit");

  std::filesystem::remove_all(dir);
}

void test_execute_expected_wave_binding() {
  const auto dir = make_tmp_dir("expected_wave_execute");
  const auto runtime_root = dir / "runtime";
  const auto wave_path = dir / "hero.runtime.wave.dsl";
  const auto config_path = dir / ".config";
  const auto policy_path = dir / "hero.runtime.dsl";
  std::filesystem::create_directories(runtime_root);

  write_text(wave_path, "WAVE_SETTINGS {\n"
                        "  WAVE_ID = test_wave;\n"
                        "  TARGET = wikimyei.inference.expected_value.mdn;\n"
                        "  MODE = run|debug;\n"
                        "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                        "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
                        "  SOURCE_RANGE = anchor_index;\n"
                        "  ANCHOR_INDEX_BEGIN = 1;\n"
                        "  ANCHOR_INDEX_END = 3;\n"
                        "  INPUT_MDN_CHECKPOINT = /tmp/mdn.pt;\n"
                        "  INPUT_REPRESENTATION_CHECKPOINT = /tmp/rep.pt;\n"
                        "};\n");

  write_text(config_path, "[HERO]\n"
                          "runtime_hero_dsl_path = " +
                              policy_path.string() +
                              "\n"
                              "runtime_wave_dsl_path = " +
                              wave_path.string() + "\n");
  write_text(policy_path, "protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO\n"
                          "runtime_exec_path:path = /bin/true\n"
                          "default_config_path:path = " +
                              config_path.string() +
                              "\n"
                              "runtime_root:path = " +
                              runtime_root.string() +
                              "\n"
                              "allowed_job_roots:path_list = " +
                              runtime_root.string() +
                              ",/tmp\n"
                              "default_dry_run:bool = true\n"
                              "allow_execute:bool = false\n"
                              "allow_train_execute:bool = false\n"
                              "allow_force_rebuild_cache:bool = false\n"
                              "require_confirm_execute:bool = true\n"
                              "allow_dev_nuke:bool = false\n"
                              "require_confirm_dev_nuke:bool = true\n"
                              "dev_nuke_backup_enabled:bool = true\n"
                              "dev_nuke_backup_root:path = " +
                              (dir / "backups").string() +
                              "\n"
                              "allowed_dev_nuke_roots:path_list = " +
                              runtime_root.string() +
                              "\n"
                              "max_capture_bytes:int = 4096\n"
                              "max_runtime_seconds:int = 5\n");

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;
  ctx.policy_path = policy_path;
  std::string error;
  check(hero_runtime::load_runtime_policy(policy_path, config_path, &ctx.policy,
                                          &error),
        "failed to load runtime policy: " + error);

  const std::string config_digest = digest_file_for_handoff(
      config_path, "kikijyeba.runtime.handoff.base_config_file.v1");
  const std::string policy_digest = digest_file_for_handoff(
      policy_path, "kikijyeba.runtime.handoff.runtime_policy_file.v1");
  const std::string good_handoff_digest = "test";
  const std::string good_handoff_id = "runtime_handoff_" + good_handoff_digest;
  const std::string stale_handoff_digest = "stale_digest";
  const std::string stale_handoff_id =
      "runtime_handoff_" + stale_handoff_digest;
  const std::string symbolic_handoff_digest = "symbolic";
  const std::string symbolic_handoff_id =
      "runtime_handoff_" + symbolic_handoff_digest;

  const std::string good_args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\",\"marshal_expected_wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"run|debug\","
      "\"source_range\":\"anchor_index\","
      "\"source_order\":\"sequential\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\","
      "\"model_state_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/mdn.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"}}}";
  std::string result;
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", good_args, &ctx,
                                        &result, &error),
        "expected-wave-bound execute failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "expected-wave-bound execute returned error: " + result);
  require_contains(result, "\"ok\":true",
                   "expected-wave-bound execute should run when fields match");

  const std::string handoff_args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\",\"confirm_execute\":false,\"runtime_handoff\":{"
      "\"handoff_schema_version\":\"1\","
      "\"handoff_id\":\"" +
      good_handoff_id +
      "\","
      "\"handoff_digest\":\"" +
      good_handoff_digest +
      "\","
      "\"created_by\":\"marshal\","
      "\"created_at\":\"2026-05-25T00:00:00Z\","
      "\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"base_config\":{\"path\":\"" +
      config_path.string() + "\",\"digest\":\"" + config_digest +
      "\"},"
      "\"wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"run|debug\","
      "\"source_range\":\"anchor_index\","
      "\"source_order\":\"sequential\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\","
      "\"model_state_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/mdn.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"}},"
      "\"checkpoint_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/mdn.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"},"
      "\"checkpoint_artifact_digests\":{},"
      "\"lattice_certificate_refs\":{},"
      "\"runtime_policy\":{\"path\":\"" +
      policy_path.string() + "\",\"digest\":\"" + policy_digest +
      "\"},"
      "\"intent\":{\"dry_run\":true,\"confirm_execute\":false,"
      "\"force_rebuild_cache\":false},"
      "\"unresolved_symbols\":[]}}";
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", handoff_args, &ctx,
                                        &result, &error),
        "runtime_handoff execute failed: error=" + error + " result=" + result);
  check(!hero_runtime::tool_result_is_error(result),
        "runtime_handoff execute returned error: " + result);
  require_contains(result, "\"ok\":true",
                   "runtime_handoff execute should run when fields match");
  require_contains(result, "\"--runtime-handoff-id\"",
                   "runtime_handoff execute should pass handoff id to runtime");
  require_contains(result, "\"" + good_handoff_id + "\"",
                   "runtime_handoff execute should pass concrete handoff id");
  require_contains(result, "\"--runtime-handoff-digest\"",
                   "runtime_handoff execute should pass handoff digest to "
                   "runtime");
  require_contains(result, "\"" + good_handoff_digest + "\"",
                   "runtime_handoff execute should pass concrete handoff "
                   "digest");

  const std::string stale_hash_handoff_args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\",\"confirm_execute\":false,\"runtime_handoff\":{"
      "\"handoff_schema_version\":\"1\","
      "\"handoff_id\":\"" +
      stale_handoff_id +
      "\","
      "\"handoff_digest\":\"" +
      stale_handoff_digest +
      "\","
      "\"created_by\":\"marshal\","
      "\"created_at\":\"2026-05-25T00:00:00Z\","
      "\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"base_config\":{\"path\":\"" +
      config_path.string() +
      "\",\"digest\":\"stale_config_digest\"},"
      "\"wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"run|debug\","
      "\"source_range\":\"anchor_index\","
      "\"source_order\":\"sequential\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\","
      "\"model_state_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/mdn.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"}},"
      "\"checkpoint_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/mdn.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"},"
      "\"checkpoint_artifact_digests\":{},"
      "\"lattice_certificate_refs\":{},"
      "\"runtime_policy\":{\"path\":\"" +
      policy_path.string() + "\",\"digest\":\"" + policy_digest +
      "\"},"
      "\"intent\":{\"dry_run\":true,\"confirm_execute\":false,"
      "\"force_rebuild_cache\":false},"
      "\"unresolved_symbols\":[]}}";
  result.clear();
  error.clear();
  const bool stale_hash_ok = hero_runtime::execute_tool_json(
      "hero.runtime.run", stale_hash_handoff_args, &ctx, &result, &error);
  check(
      !stale_hash_ok,
      "runtime_handoff stale base config digest should fail before execution: "
      "result=" +
          result + " error=" + error);
  check(
      error.find("base_config path/digest") != std::string::npos,
      "runtime_handoff stale digest should report base_config digest failure");

  const std::string unresolved_handoff_args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\",\"confirm_execute\":false,\"runtime_handoff\":{"
      "\"handoff_schema_version\":\"1\","
      "\"handoff_id\":\"" +
      symbolic_handoff_id +
      "\","
      "\"handoff_digest\":\"" +
      symbolic_handoff_digest +
      "\","
      "\"created_by\":\"marshal\","
      "\"created_at\":\"2026-05-25T00:00:00Z\","
      "\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"base_config\":{\"path\":\"" +
      config_path.string() + "\",\"digest\":\"" + config_digest +
      "\"},"
      "\"wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"run|debug\","
      "\"source_range\":\"anchor_index\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\","
      "\"model_state_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"latest_satisfying:"
      "channel_mdn_train_core_ready\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"}},"
      "\"checkpoint_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"latest_satisfying:"
      "channel_mdn_train_core_ready\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"},"
      "\"checkpoint_artifact_digests\":{},"
      "\"lattice_certificate_refs\":{},"
      "\"runtime_policy\":{\"path\":\"" +
      policy_path.string() + "\",\"digest\":\"" + policy_digest +
      "\"},"
      "\"intent\":{\"dry_run\":true,\"confirm_execute\":false,"
      "\"force_rebuild_cache\":false},"
      "\"unresolved_symbols\":[\"PLAN_INPUT_MDN_CHECKPOINT=latest_satisfying:"
      "channel_mdn_train_core_ready\"]}}";
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json(
            "hero.runtime.run", unresolved_handoff_args, &ctx, &result, &error),
        "runtime_handoff unresolved selector should fail before execution");
  check(error.find("E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS") != std::string::npos,
        "runtime_handoff unresolved selector should report unresolved symbols");

  const std::string bad_args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\",\"marshal_expected_wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"train|debug\","
      "\"source_range\":\"anchor_index\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\","
      "\"model_state_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/mdn.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"}}}";
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run", bad_args, &ctx,
                                         &result, &error),
        "expected-wave mismatch should fail before execution");
  check(error.find("E_RUNTIME_EXPECTED_WAVE_MISMATCH") != std::string::npos,
        "expected-wave mismatch should report a specific error");

  const std::string bad_checkpoint_args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\",\"marshal_expected_wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"run|debug\","
      "\"source_range\":\"anchor_index\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\","
      "\"model_state_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/other.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"}}}";
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json(
            "hero.runtime.run", bad_checkpoint_args, &ctx, &result, &error),
        "expected-wave checkpoint mismatch should fail before execution");
  check(error.find("model_state_inputs differs") != std::string::npos,
        "checkpoint mismatch should report model_state_inputs failure");

  std::filesystem::remove_all(dir);
}

void test_handoff_checkpoint_inputs_launch_without_static_config_inputs() {
  const auto dir = make_tmp_dir("handoff_checkpoint_inputs_launch");
  const auto runtime_root = dir / "runtime";
  const auto wave_path = dir / "hero.runtime.wave.dsl";
  const auto config_path = dir / ".config";
  const auto policy_path = dir / "hero.runtime.dsl";
  std::filesystem::create_directories(runtime_root);

  write_text(wave_path, "WAVE_SETTINGS {\n"
                        "  WAVE_ID = handoff_wave;\n"
                        "  TARGET = wikimyei.inference.expected_value.mdn;\n"
                        "  MODE = run|debug;\n"
                        "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                        "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
                        "  SOURCE_RANGE = anchor_index;\n"
                        "  ANCHOR_INDEX_BEGIN = 1;\n"
                        "  ANCHOR_INDEX_END = 3;\n"
                        "};\n");

  write_text(config_path, "[HERO]\n"
                          "runtime_hero_dsl_path = " +
                              policy_path.string() +
                              "\n"
                              "runtime_wave_dsl_path = " +
                              wave_path.string() + "\n");
  write_text(policy_path, "protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO\n"
                          "runtime_exec_path:path = /bin/true\n"
                          "default_config_path:path = " +
                              config_path.string() +
                              "\n"
                              "runtime_root:path = " +
                              runtime_root.string() +
                              "\n"
                              "allowed_job_roots:path_list = " +
                              runtime_root.string() +
                              ",/tmp\n"
                              "default_dry_run:bool = true\n"
                              "allow_execute:bool = false\n"
                              "allow_train_execute:bool = false\n"
                              "allow_force_rebuild_cache:bool = false\n"
                              "require_confirm_execute:bool = true\n"
                              "allow_dev_nuke:bool = false\n"
                              "require_confirm_dev_nuke:bool = true\n"
                              "dev_nuke_backup_enabled:bool = true\n"
                              "dev_nuke_backup_root:path = " +
                              (dir / "backups").string() +
                              "\n"
                              "allowed_dev_nuke_roots:path_list = " +
                              runtime_root.string() +
                              "\n"
                              "max_capture_bytes:int = 4096\n"
                              "max_runtime_seconds:int = 5\n");

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;
  ctx.policy_path = policy_path;
  std::string error;
  check(hero_runtime::load_runtime_policy(policy_path, config_path, &ctx.policy,
                                          &error),
        "failed to load runtime policy: " + error);

  const std::string config_digest = digest_file_for_handoff(
      config_path, "kikijyeba.runtime.handoff.base_config_file.v1");
  const std::string policy_digest = digest_file_for_handoff(
      policy_path, "kikijyeba.runtime.handoff.runtime_policy_file.v1");
  const std::string handoff_digest = "materialized";
  const std::string handoff_id = "runtime_handoff_" + handoff_digest;

  const std::string args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\",\"confirm_execute\":false,\"runtime_handoff\":{"
      "\"handoff_schema_version\":\"1\","
      "\"handoff_id\":\"" +
      handoff_id +
      "\","
      "\"handoff_digest\":\"" +
      handoff_digest +
      "\","
      "\"created_by\":\"marshal\","
      "\"created_at\":\"2026-06-05T00:00:00Z\","
      "\"target_id\":\"channel_mdn_validation_eval_ready\","
      "\"base_config\":{\"path\":\"" +
      config_path.string() + "\",\"digest\":\"" + config_digest +
      "\"},"
      "\"wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"run|debug\","
      "\"source_range\":\"anchor_index\","
      "\"source_order\":\"sequential\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\","
      "\"model_state_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/materialized_mdn.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/materialized_rep.pt\"}},"
      "\"checkpoint_inputs\":{"
      "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/materialized_mdn.pt\","
      "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/materialized_rep.pt\"},"
      "\"checkpoint_artifact_digests\":{},"
      "\"lattice_certificate_refs\":{},"
      "\"runtime_policy\":{\"path\":\"" +
      policy_path.string() + "\",\"digest\":\"" + policy_digest +
      "\"},"
      "\"intent\":{\"dry_run\":true,\"confirm_execute\":false,"
      "\"force_rebuild_cache\":false},"
      "\"unresolved_symbols\":[]}}";

  std::string result;
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", args, &ctx, &result,
                                        &error),
        "materialized runtime_handoff execute failed: result=" + result +
            " error=" + error);
  check(!hero_runtime::tool_result_is_error(result),
        "materialized runtime_handoff execute returned error: " + result);
  require_contains(
      result, "\"--input-representation-checkpoint\"",
      "Runtime should pass materialized representation checkpoint");
  require_contains(result, "\"/tmp/materialized_rep.pt\"",
                   "Runtime should pass concrete representation checkpoint");
  require_contains(result, "\"--input-mdn-checkpoint\"",
                   "Runtime should pass materialized MDN checkpoint");
  require_contains(result, "\"/tmp/materialized_mdn.pt\"",
                   "Runtime should pass concrete MDN checkpoint");

  std::filesystem::remove_all(dir);
}

void test_execute_wave_overlay() {
  const auto dir = make_tmp_dir("execute_wave_overlay");
  const auto runtime_root = dir / "runtime";
  const auto wave_path = dir / "hero.runtime.wave.dsl";
  const auto config_path = dir / ".config";
  const auto policy_path = dir / "hero.runtime.dsl";
  std::filesystem::create_directories(runtime_root);

  write_text(wave_path, "WAVE_SETTINGS {\n"
                        "  WAVE_ID = profile_wave;\n"
                        "  TARGET = wikimyei.inference.expected_value.mdn;\n"
                        "  MODE = run|debug;\n"
                        "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                        "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
                        "  SOURCE_RANGE = all;\n"
                        "};\n");
  write_text(config_path, "[HERO]\n"
                          "runtime_hero_dsl_path = " +
                              policy_path.string() +
                              "\n"
                              "runtime_wave_dsl_path = " +
                              wave_path.string() + "\n");
  write_text(policy_path, "protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO\n"
                          "runtime_exec_path:path = /bin/true\n"
                          "default_config_path:path = " +
                              config_path.string() +
                              "\n"
                              "runtime_root:path = " +
                              runtime_root.string() +
                              "\n"
                              "allowed_job_roots:path_list = " +
                              runtime_root.string() +
                              ",/tmp\n"
                              "default_dry_run:bool = true\n"
                              "allow_execute:bool = false\n"
                              "allow_train_execute:bool = false\n"
                              "allow_force_rebuild_cache:bool = false\n"
                              "require_confirm_execute:bool = true\n"
                              "allow_dev_nuke:bool = false\n"
                              "require_confirm_dev_nuke:bool = true\n"
                              "dev_nuke_backup_enabled:bool = false\n"
                              "dev_nuke_backup_root:path = " +
                              (dir / "backups").string() +
                              "\n"
                              "allowed_dev_nuke_roots:path_list = " +
                              runtime_root.string() +
                              "\n"
                              "max_capture_bytes:int = 4096\n"
                              "max_runtime_seconds:int = 5\n");

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;
  ctx.policy_path = policy_path;
  std::string error;
  check(hero_runtime::load_runtime_policy(policy_path, config_path, &ctx.policy,
                                          &error),
        "failed to load runtime policy: " + error);

  const std::string args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\","
      "\"wave_overlay\":{\"source_range\":\"anchor_index\","
      "\"anchor_index_begin\":\"1\",\"anchor_index_end\":\"3\"},"
      "\"marshal_expected_wave\":{"
      "\"target_component_family_id\":\"wikimyei.inference.expected_value."
      "mdn\","
      "\"mode\":\"run|debug\","
      "\"source_range\":\"anchor_index\","
      "\"source_order\":\"sequential\","
      "\"anchor_index_begin\":\"1\","
      "\"anchor_index_end\":\"3\"}}";
  std::string result;
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", args, &ctx, &result,
                                        &error),
        "wave-overlay execute failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "wave-overlay execute returned error: " + result);
  require_contains(result, "\"--source-range\"",
                   "wave overlay should be passed to runtime process");
  require_contains(result, "\"anchor_index\"",
                   "wave overlay should pass anchor_index policy");
  require_contains(result, "\"--anchor-index-begin\"",
                   "wave overlay should pass anchor begin flag");
  require_contains(result, "\"1\"", "wave overlay should pass anchor begin");
  require_contains(result, "\"--anchor-index-end\"",
                   "wave overlay should pass anchor end flag");
  require_contains(result, "\"3\"", "wave overlay should pass anchor end");

  const std::string invalid_args =
      "{\"operation\":\"wave\",\"requested_mode\":\"dry_run\","
      "\"config_path\":\"" +
      config_path.string() +
      "\","
      "\"wave_overlay\":{\"source_range\":\"all\","
      "\"anchor_index_begin\":\"1\",\"anchor_index_end\":\"3\"}}";
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run", invalid_args, &ctx,
                                         &result, &error),
        "invalid wave overlay should fail");
  check(error.find("E_RUNTIME_WAVE_OVERLAY_INVALID") != std::string::npos,
        "invalid wave overlay reports overlay error");

  std::filesystem::remove_all(dir);
}

void test_replay_operator_tool() {
  const auto dir = make_tmp_dir("replay_tool");
  const auto runtime_root = dir / "runtime";
  const auto job_dir = runtime_root / "jobs" / "completed_job";
  const auto replay_artifact_dir =
      job_dir / "artifacts" / "kikijyeba.environment.replay.v1";
  const auto config_path = dir / ".config";
  const auto policy_path = dir / "hero.runtime.dsl";
  const auto fake_exec = dir / "fake_cuwacunu_exec.sh";
  std::filesystem::create_directories(replay_artifact_dir);

  write_text(config_path, "[ACCOUNTING]\n"
                          "accounting_numeraire_node_id = USDT\n"
                          "\n"
                          "[HERO]\n"
                          "runtime_hero_dsl_path = " +
                              policy_path.string() + "\n");
  write_text(job_dir / "job.manifest",
             "job_id=completed_job\n"
             "config_path=" +
                 config_path.string() +
                 "\n"
                 "target_component_family_id=wikimyei.inference.expected_"
                 "value.mdn\n");
  write_text(job_dir / "job.state", "status=completed\n");
  write_text(replay_artifact_dir / "runtime_replay_batches.index",
             "schema=kikijyeba.environment.replay.runtime_batch_index.v1\n"
             "entry_count=1\n");
  const std::string hero_replay_report_text =
      "schema=kikijyeba.environment.replay.cajtucu_ready_experiment_"
      "artifact.v1\n"
      "experiment_id=hero_replay\n"
      "completed_count=2\n"
      "execution_profile_digest=profile_digest_fixture\n"
      "policy_set_digest=policy_set_digest_fixture\n"
      "cajtucu_invalid_trace_count=0\n"
      "cajtucu_numeraire_fallback_pair_count=0\n"
      "cajtucu_synthetic_market_step_count=0\n";
  const std::string hero_replay_report_digest =
      replay_report_digest_for_text(hero_replay_report_text);
  write_text(replay_artifact_dir / "runtime_replay_experiments.index",
             "schema=kikijyeba.environment.replay.runtime_experiment_index."
             "v1\n"
             "entry_count=1\n"
             "entry_0_experiment_id=hero_replay\n"
             "entry_0_policy_count=2\n"
             "entry_0_replay_bundle_count=1\n"
             "entry_0_attempted_count=2\n"
             "entry_0_completed_count=2\n"
             "entry_0_report_path=hero_replay.report\n"
             "entry_0_report_digest=" +
                 hero_replay_report_digest + "\n");
  write_text(replay_artifact_dir / "hero_replay.report",
             hero_replay_report_text);
  write_text(fake_exec,
             "#!/bin/sh\n"
             "printf 'replay_experiment_id=hero_replay\\n'\n"
             "printf 'replay_completed_count=2\\n'\n"
             "printf 'replay_report_path=%s\\n' \"" +
                 (replay_artifact_dir / "hero_replay.report").string() +
                 "\"\n");
  std::filesystem::permissions(fake_exec,
                               std::filesystem::perms::owner_exec |
                                   std::filesystem::perms::owner_read |
                                   std::filesystem::perms::owner_write);
  write_text(policy_path, "protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO\n"
                          "runtime_exec_path:path = " +
                              fake_exec.string() +
                              "\n"
                              "default_config_path:path = " +
                              config_path.string() +
                              "\n"
                              "runtime_root:path = " +
                              runtime_root.string() +
                              "\n"
                              "allowed_job_roots:path_list = " +
                              runtime_root.string() +
                              ",/tmp\n"
                              "default_dry_run:bool = true\n"
                              "allow_execute:bool = false\n"
                              "allow_train_execute:bool = false\n"
                              "allow_force_rebuild_cache:bool = false\n"
                              "require_confirm_execute:bool = true\n"
                              "allow_dev_nuke:bool = false\n"
                              "require_confirm_dev_nuke:bool = true\n"
                              "dev_nuke_backup_enabled:bool = false\n"
                              "dev_nuke_backup_root:path = " +
                              (dir / "backups").string() +
                              "\n"
                              "allowed_dev_nuke_roots:path_list = " +
                              runtime_root.string() +
                              "\n"
                              "max_capture_bytes:int = 4096\n"
                              "max_runtime_seconds:int = 5\n");

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;
  ctx.policy_path = policy_path;
  std::string error;
  check(hero_runtime::load_runtime_policy(policy_path, config_path, &ctx.policy,
                                          &error),
        "failed to load replay runtime policy: " + error);

  const std::string dry_args =
      "{\"operation\":\"replay\",\"requested_mode\":\"plan\","
      "\"job_dir\":\"" +
      job_dir.string() +
      "\","
      "\"accounting_numeraire_node_id\":\"USDT\","
      "\"target_node_ids\":\"BTC,ETH\","
      "\"experiment_id\":\"hero_replay\","
      "\"max_steps\":8,"
      "\"include_equal_weight\":true,"
      "\"allow_synthetic_direct_edges\":true,"
      "\"linear_transaction_cost_rate\":0.001,"
      "\"execution_profile_digest\":\"profile_digest_fixture\","
      "\"policy_set_digest\":\"policy_set_digest_fixture\"}";
  std::string result;
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", dry_args, &ctx,
                                        &result, &error),
        "replay dry-run failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "replay dry-run returned error: " + result);
  require_contains(result, "\"dry_run\":true",
                   "replay dry-run should not execute process");
  require_contains(result, "\"--replay-from-job-dir\"",
                   "replay should target job-dir replay mode");
  require_contains(result, "\"--replay-accounting-numeraire-node\"",
                   "replay should pass accounting numeraire override");
  require_contains(result, "\"--replay-target-nodes\"",
                   "replay should pass target-node override");
  require_contains(result, "\"--replay-max-steps\"",
                   "replay should pass max steps");
  require_contains(result, "\"--replay-include-equal-weight\"",
                   "replay should pass baseline policy flag");
  require_contains(result, "\"--replay-allow-synthetic-direct-edges\"",
                   "replay should pass synthetic-edge replay flag");
  require_contains(result, "\"--replay-linear-transaction-cost-rate\"",
                   "replay should pass transaction-cost replay flag");
  require_contains(result, "\"--replay-execution-profile-digest\"",
                   "replay should pass execution profile identity digest");
  require_contains(result, "\"profile_digest_fixture\"",
                   "replay should bind execution profile digest value");
  require_contains(result, "\"--replay-policy-set-digest\"",
                   "replay should pass policy set identity digest");
  require_contains(result, "\"policy_set_digest_fixture\"",
                   "replay should bind policy set digest value");

  const std::string validation_args =
      "{\"operation\":\"replay\",\"requested_mode\":\"plan\","
      "\"job_dir\":\"" +
      job_dir.string() +
      "\","
      "\"accounting_numeraire_node_id\":\"USDT\","
      "\"target_node_ids\":\"BTC,ETH\","
      "\"experiment_id\":\"hero_validation_replay\","
      "\"max_steps\":8,"
      "\"max_parallel_jobs\":2,"
      "\"include_equal_weight\":true,"
      "\"validation_rollout\":true,"
      "\"linear_transaction_cost_rate\":0.001,"
      "\"execution_profile_digest\":\"profile_digest_fixture\","
      "\"policy_set_digest\":\"policy_set_digest_fixture\"}";
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", validation_args,
                                        &ctx, &result, &error),
        "validation replay dry-run failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "validation replay dry-run returned error: " + result);
  require_contains(result, "\"dry_run\":true",
                   "validation replay plan should not execute process");
  require_contains(result, "\"--replay-linear-transaction-cost-rate\"",
                   "validation replay should pass nonzero transaction cost");
  require_contains(result, "\"--replay-execution-profile-digest\"",
                   "validation replay should bind execution profile digest");
  require_contains(result, "\"--replay-policy-set-digest\"",
                   "validation replay should bind policy set digest");
  check(result.find("\"--replay-allow-synthetic-direct-edges\"") ==
            std::string::npos,
        "validation replay must not pass synthetic edge CLI flag");

  const auto expect_validation_replay_rejection =
      [&](const std::string &bad_args, const std::string &error_needle,
          const std::string &message) {
        result.clear();
        error.clear();
        check(!hero_runtime::execute_tool_json("hero.runtime.run", bad_args,
                                               &ctx, &result, &error),
              message);
        require_contains(error, error_needle, message + " error");
      };

  expect_validation_replay_rejection(
      "{\"operation\":\"replay\",\"requested_mode\":\"plan\",\"job_dir\":\"" +
          job_dir.string() +
          "\",\"max_steps\":8,\"max_parallel_jobs\":1,"
          "\"validation_rollout\":true,"
          "\"linear_transaction_cost_rate\":0.001,"
          "\"policy_set_digest\":\"policy_set_digest_fixture\"}",
      "E_RUNTIME_REPLAY_VALIDATION_PROFILE_DIGEST_MISSING",
      "validation replay should reject missing execution profile digest");
  expect_validation_replay_rejection(
      "{\"operation\":\"replay\",\"requested_mode\":\"plan\",\"job_dir\":\"" +
          job_dir.string() +
          "\",\"max_steps\":8,\"max_parallel_jobs\":1,"
          "\"validation_rollout\":true,"
          "\"linear_transaction_cost_rate\":0.001,"
          "\"execution_profile_digest\":\"profile_digest_fixture\"}",
      "E_RUNTIME_REPLAY_VALIDATION_POLICY_SET_DIGEST_MISSING",
      "validation replay should reject missing policy set digest");
  expect_validation_replay_rejection(
      "{\"operation\":\"replay\",\"requested_mode\":\"plan\",\"job_dir\":\"" +
          job_dir.string() +
          "\",\"max_steps\":8,\"max_parallel_jobs\":1,"
          "\"validation_rollout\":true,"
          "\"linear_transaction_cost_rate\":0.0,"
          "\"execution_profile_digest\":\"profile_digest_fixture\","
          "\"policy_set_digest\":\"policy_set_digest_fixture\"}",
      "E_RUNTIME_REPLAY_VALIDATION_NONZERO_COST_REQUIRED",
      "validation replay should reject zero transaction cost");
  expect_validation_replay_rejection(
      "{\"operation\":\"replay\",\"requested_mode\":\"plan\",\"job_dir\":\"" +
          job_dir.string() +
          "\",\"max_steps\":8,\"max_parallel_jobs\":1,"
          "\"validation_rollout\":true,"
          "\"allow_synthetic_direct_edges\":true,"
          "\"linear_transaction_cost_rate\":0.001,"
          "\"execution_profile_digest\":\"profile_digest_fixture\","
          "\"policy_set_digest\":\"policy_set_digest_fixture\"}",
      "E_RUNTIME_REPLAY_VALIDATION_SYNTHETIC_EDGES_FORBIDDEN",
      "validation replay should reject synthetic execution markets");
  expect_validation_replay_rejection(
      "{\"operation\":\"replay\",\"requested_mode\":\"plan\",\"job_dir\":\"" +
          job_dir.string() +
          "\",\"max_parallel_jobs\":1,"
          "\"validation_rollout\":true,"
          "\"linear_transaction_cost_rate\":0.001,"
          "\"execution_profile_digest\":\"profile_digest_fixture\","
          "\"policy_set_digest\":\"policy_set_digest_fixture\"}",
      "E_RUNTIME_REPLAY_VALIDATION_MAX_STEPS_REQUIRED",
      "validation replay should reject missing max_steps");
  expect_validation_replay_rejection(
      "{\"operation\":\"replay\",\"requested_mode\":\"plan\",\"job_dir\":\"" +
          job_dir.string() +
          "\",\"max_steps\":8,"
          "\"validation_rollout\":true,"
          "\"linear_transaction_cost_rate\":0.001,"
          "\"execution_profile_digest\":\"profile_digest_fixture\","
          "\"policy_set_digest\":\"policy_set_digest_fixture\"}",
      "E_RUNTIME_REPLAY_VALIDATION_PARALLELISM_INVALID",
      "validation replay should reject missing max_parallel_jobs");

  const std::string run_args =
      "{\"operation\":\"replay\",\"requested_mode\":\"execute\","
      "\"job_dir\":\"" +
      job_dir.string() +
      "\","
      "\"accounting_numeraire_node_id\":\"USDT\","
      "\"target_node_ids\":\"BTC,ETH\","
      "\"experiment_id\":\"hero_replay\","
      "\"max_steps\":8}";
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", run_args, &ctx,
                                        &result, &error),
        "replay execution failed with allow_execute=false: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "replay execution returned error: " + result);
  require_contains(result, "\"ok\":true", "replay execution should succeed");
  require_contains(result, "\"replay_completed_count\":\"2\"",
                   "replay should expose replay stdout fields");

  const std::string validation_run_args =
      "{\"operation\":\"replay\",\"requested_mode\":\"execute\","
      "\"job_dir\":\"" +
      job_dir.string() +
      "\","
      "\"accounting_numeraire_node_id\":\"USDT\","
      "\"target_node_ids\":\"BTC,ETH\","
      "\"experiment_id\":\"hero_validation_replay\","
      "\"max_steps\":8,"
      "\"max_parallel_jobs\":2,"
      "\"validation_rollout\":true,"
      "\"linear_transaction_cost_rate\":0.001,"
      "\"execution_profile_digest\":\"profile_digest_fixture\","
      "\"policy_set_digest\":\"policy_set_digest_fixture\"}";
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", validation_run_args,
                                        &ctx, &result, &error),
        "validation replay execution should accept clean Cajtucu report: " +
            error);
  check(!hero_runtime::tool_result_is_error(result),
        "validation replay execution returned error: " + result);
  require_contains(result, "\"ok\":true",
                   "validation replay execution should succeed with clean "
                   "Cajtucu counters");

  const auto expect_validation_execute_rejection =
      [&](const std::string &report_text, const std::string &error_needle,
          const std::string &message) {
        write_text(replay_artifact_dir / "hero_replay.report", report_text);
        result.clear();
        error.clear();
        check(!hero_runtime::execute_tool_json("hero.runtime.run",
                                               validation_run_args, &ctx,
                                               &result, &error),
              message);
        require_contains(error, error_needle, message + " error");
      };
  expect_validation_execute_rejection(
      "schema=kikijyeba.environment.replay.cajtucu_ready_experiment_"
      "artifact.v1\n"
      "experiment_id=hero_replay\n"
      "completed_count=2\n"
      "execution_profile_digest=profile_digest_fixture\n"
      "policy_set_digest=policy_set_digest_fixture\n"
      "cajtucu_invalid_trace_count=0\n"
      "cajtucu_numeraire_fallback_pair_count=0\n"
      "cajtucu_synthetic_market_step_count=3\n",
      "E_RUNTIME_REPLAY_VALIDATION_SYNTHETIC_MARKET_USED",
      "validation replay should reject synthetic Cajtucu market evidence");
  expect_validation_execute_rejection(
      "schema=kikijyeba.environment.replay.cajtucu_ready_experiment_"
      "artifact.v1\n"
      "experiment_id=hero_replay\n"
      "completed_count=2\n"
      "execution_profile_digest=profile_digest_fixture\n"
      "policy_set_digest=policy_set_digest_fixture\n"
      "cajtucu_invalid_trace_count=4\n"
      "cajtucu_numeraire_fallback_pair_count=0\n"
      "cajtucu_synthetic_market_step_count=0\n",
      "E_RUNTIME_REPLAY_VALIDATION_INVALID_CAJTUCU_TRACE",
      "validation replay should reject invalid Cajtucu traces");
  expect_validation_execute_rejection(
      "schema=kikijyeba.environment.replay.cajtucu_ready_experiment_"
      "artifact.v1\n"
      "experiment_id=hero_replay\n"
      "completed_count=2\n"
      "execution_profile_digest=profile_digest_fixture\n"
      "policy_set_digest=policy_set_digest_fixture\n"
      "cajtucu_invalid_trace_count=0\n"
      "cajtucu_numeraire_fallback_pair_count=1\n"
      "cajtucu_synthetic_market_step_count=0\n",
      "E_RUNTIME_REPLAY_VALIDATION_NUMERAIRE_FALLBACK_USED",
      "validation replay should reject accounting numeraire fallback "
      "execution evidence");
  write_text(replay_artifact_dir / "hero_replay.report",
             hero_replay_report_text);

  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json(
            "hero.runtime.inspect",
            "{\"subject\":\"artifact\",\"job_dir\":\"" + job_dir.string() +
                "\",\"artifact\":\"replay_experiment_index\"}",
            &ctx, &result, &error),
        "read replay experiment index failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "read replay experiment index returned error: " + result);
  require_contains(result, "runtime_replay_experiments.index",
                   "read_artifact should resolve replay experiment index");
  require_contains(result, "\"entry_0_experiment_id\":\"hero_replay\"",
                   "read_artifact should parse replay experiment index fields");

  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json(
            "hero.runtime.inspect",
            "{\"subject\":\"artifact\",\"job_dir\":\"" + job_dir.string() +
                "\",\"artifact\":\"replay_experiment_report\"}",
            &ctx, &result, &error),
        "read replay experiment report failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "read replay experiment report returned error: " + result);
  require_contains(result, "hero_replay.report",
                   "read_artifact should resolve latest replay report");
  require_contains(result, "\"completed_count\":\"2\"",
                   "read_artifact should parse replay report fields");
  require_contains(result, "\"replay_report_integrity\"",
                   "read_artifact should expose replay report integrity");
  require_contains(result, "\"report_digest_bound\":true",
                   "read_artifact should expose declared digest binding");
  require_contains(result, "\"report_digest_match\":true",
                   "read_artifact should verify matching report digest");
  require_contains(result, hero_replay_report_digest,
                   "read_artifact should expose replay report digest");

  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.inspect",
                                        "{\"subject\":\"job\",\"job_dir\":\"" +
                                            job_dir.string() +
                                            "\",\"include_text\":false}",
                                        &ctx, &result, &error),
        "get_job replay artifact summary failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "get_job replay artifact summary returned error: " + result);
  require_contains(result, "\"replay_experiment_index\"",
                   "get_job should summarize replay experiment index");
  require_contains(result, "\"replay_experiment_report\"",
                   "get_job should summarize replay experiment report");
  require_contains(result, "\"replay_experiment_report_integrity\"",
                   "get_job should summarize replay report integrity");
  require_contains(result, "\"report_digest_match\":true",
                   "get_job should expose matching replay report digest");

  const std::string escaped_replay_report_text =
      "schema=kikijyeba.environment.replay.cajtucu_ready_experiment_"
      "artifact.v1\n"
      "experiment_id=escaped_replay\n"
      "completed_count=999\n";
  const std::string escaped_replay_report_digest =
      replay_report_digest_for_text(escaped_replay_report_text);
  write_text(job_dir / "artifacts" / "outside_replay.report",
             escaped_replay_report_text);
  write_text(replay_artifact_dir / "runtime_replay_experiments.index",
             "schema=kikijyeba.environment.replay.runtime_experiment_index."
             "v1\n"
             "entry_count=1\n"
             "entry_0_experiment_id=escaped_replay\n"
             "entry_0_policy_count=1\n"
             "entry_0_replay_bundle_count=1\n"
             "entry_0_attempted_count=1\n"
             "entry_0_completed_count=1\n"
             "entry_0_report_path=../outside_replay.report\n"
             "entry_0_report_digest=" +
                 escaped_replay_report_digest + "\n");
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json(
            "hero.runtime.inspect",
            "{\"subject\":\"artifact\",\"job_dir\":\"" + job_dir.string() +
                "\",\"artifact\":\"replay_experiment_report\"}",
            &ctx, &result, &error),
        "read replay experiment report with unsafe index path failed: " +
            error);
  check(!hero_runtime::tool_result_is_error(result),
        "unsafe replay report index should not crash artifact read: " + result);
  check(result.find("completed_count=999") == std::string::npos &&
            result.find("outside_replay.report") == std::string::npos,
        "Runtime Hero must not follow parent-directory replay report paths "
        "from runtime_replay_experiments.index");
  require_contains(result, "\"report_path_rejected\":true",
                   "unsafe replay report path should be surfaced");
  require_contains(result, "\"report_digest_bound\":true",
                   "unsafe replay report digest binding should remain visible");

  write_text(replay_artifact_dir / "runtime_replay_experiments.index",
             "schema=kikijyeba.environment.replay.runtime_experiment_index."
             "v1\n"
             "entry_count=1\n"
             "entry_0_experiment_id=hero_replay\n"
             "entry_0_policy_count=2\n"
             "entry_0_replay_bundle_count=1\n"
             "entry_0_attempted_count=2\n"
             "entry_0_completed_count=2\n"
             "entry_0_report_path=hero_replay.report\n"
             "entry_0_report_digest=mutated_digest\n");
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json(
            "hero.runtime.inspect",
            "{\"subject\":\"artifact\",\"job_dir\":\"" + job_dir.string() +
                "\",\"artifact\":\"replay_experiment_report\"}",
            &ctx, &result, &error),
        "read replay experiment report with mismatched digest failed: " +
            error);
  check(!hero_runtime::tool_result_is_error(result),
        "mismatched replay report digest should remain readable: " + result);
  require_contains(result, "\"completed_count\":\"2\"",
                   "digest mismatch should not hide replay report content");
  require_contains(result, "\"declared_report_digest\":\"mutated_digest\"",
                   "read_artifact should expose mismatched declared digest");
  require_contains(result, "\"report_digest_bound\":true",
                   "read_artifact should expose mismatched digest binding");
  require_contains(result, "\"report_digest_match\":false",
                   "read_artifact should expose replay digest mismatch");

  write_text(job_dir / "job.state", "status=failed\n");
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run", dry_args, &ctx,
                                         &result, &error),
        "replay should reject non-completed jobs");
  require_contains(error, "E_RUNTIME_REPLAY_JOB_NOT_COMPLETED",
                   "replay should report non-completed job error");

  std::filesystem::remove_all(dir);
}

void test_canonical_mdn_previews_channel_job() {
  const auto result =
      run_wave_preview("wikimyei.inference.expected_value.mdn", "run|debug");
  require_contains(result, "\"protocol_id\":\"cwu_01v\"",
                   "default protocol id should be cwu_01v");
  require_contains(result, "\"compatible_protocols\":[]",
                   "missing COMPATIBLE_PROTOCOLS is represented as empty");
  require_contains(result, "\"wave_protocol_compatible\":true",
                   "missing COMPATIBLE_PROTOCOLS remains backward compatible");
  require_contains(result,
                   "\"active_representation_family\":\"wikimyei."
                   "representation.encoding.vicreg\"",
                   "default active representation should be VICReg");
  require_contains(result,
                   "\"target_component_family_id\":\"wikimyei.inference."
                   "expected_value.mdn\"",
                   "canonical MDN target field should use component family id");
  require_contains(result, "\"job_kind\":\"channel_inference_mdn\"",
                   "canonical MDN target previews channel job");
  require_contains(result, "\"train_target\":false",
                   "run mode is non-mutating");
  require_contains(result, "\"source_order\":\"sequential\"",
                   "run waves default to sequential source order");
  require_contains(result, "\"source_order_explicit\":false",
                   "omitted run SOURCE_ORDER is reported as implicit");
  require_contains(result, "\"source_order_warning_level\":\"none\"",
                   "run waves do not warn about sequential source order");
}

void test_cwu02_mdn_preview_uses_mtf_representation_chain() {
  const auto result =
      run_wave_preview("wikimyei.inference.expected_value.mdn", "run|debug", "",
                       cwu02_mtf_protocol_text());
  require_contains(result, "\"protocol_id\":\"cwu_02v\"",
                   "cwu_02v protocol id should be surfaced");
  require_contains(result,
                   "\"active_representation_family\":\"wikimyei."
                   "representation.encoding.mtf_jepa_mae_vicreg\"",
                   "cwu_02v active representation should be MTF");
  require_contains(
      result, "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:run_frozen",
      "cwu_02v MDN preview should route through frozen MTF representation");
}

void test_canonical_vicreg_previews_channel_job() {
  const auto result = run_wave_preview(
      "wikimyei.representation.encoding.vicreg", "train|debug");
  require_contains(result, "\"job_kind\":\"channel_representation_vicreg\"",
                   "canonical VICReg target previews channel job");
  require_contains(result, "\"train_target\":true",
                   "train mode mutates target");
  require_contains(result, "\"source_order\":\"random_per_epoch\"",
                   "train waves default to RandomSampler source order");
  require_contains(result, "\"source_order_explicit\":false",
                   "omitted train SOURCE_ORDER is reported as implicit");
  require_contains(result, "\"source_order_warning_level\":\"none\"",
                   "implicit train source order should not warn");
}

void test_canonical_mtf_previews_separate_representation_job() {
  const auto result =
      run_wave_preview("wikimyei.representation.encoding.mtf_jepa_mae_vicreg",
                       "train|debug", "", cwu02_mtf_protocol_text());
  require_contains(result, "\"protocol_id\":\"cwu_02v\"",
                   "MTF representation preview should use cwu_02v protocol");
  require_contains(result, "\"protocol_target_compatible\":true",
                   "MTF representation target is compatible with cwu_02v");
  require_contains(
      result, "\"job_kind\":\"channel_representation_mtf_jepa_mae_vicreg\"",
      "canonical MTF target previews separate representation job");
  require_contains(result,
                   "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:train",
                   "MTF preview reports MTF representation execution chain");
  require_contains(result, "\"train_target\":true",
                   "train mode mutates MTF target");
}

void test_mismatched_protocol_representation_preview_warns() {
  const auto result = run_wave_preview(
      "wikimyei.representation.encoding.mtf_jepa_mae_vicreg", "train|debug");
  require_contains(result, "\"protocol_id\":\"cwu_01v\"",
                   "default preview protocol remains cwu_01v");
  require_contains(result, "\"protocol_target_compatible\":false",
                   "MTF target should warn under cwu_01v");
  require_contains(result,
                   "\"protocol_target_warning\":\"wave_target_not_docked_by_"
                   "active_protocol\"",
                   "MTF target under cwu_01v reports protocol warning");
}

void test_wave_profile_protocol_preview_warns() {
  const auto result = run_wave_preview("wikimyei.inference.expected_value.mdn",
                                       "run|debug", "", "", "cwu_02v");
  require_contains(result, "\"protocol_id\":\"cwu_01v\"",
                   "default preview protocol remains cwu_01v");
  require_contains(result, "\"compatible_protocols\":[\"cwu_02v\"]",
                   "preview reports declared compatible protocols");
  require_contains(result, "\"wave_protocol_compatible\":false",
                   "active cwu_01v should not match cwu_02v-only profile");
  require_contains(result,
                   "\"wave_protocol_warning\":\"active_protocol_not_declared_by"
                   "_wave_profile\"",
                   "profile protocol mismatch reports warning");
}

void test_train_wave_explicit_sequential_source_order_warns() {
  const auto result = run_wave_preview(
      "wikimyei.representation.encoding.vicreg", "train|debug", "sequential");
  require_contains(result, "\"source_order\":\"sequential\"",
                   "explicit sequential train SOURCE_ORDER is preserved");
  require_contains(result, "\"source_order_explicit\":true",
                   "explicit train SOURCE_ORDER is reported");
  require_contains(result, "\"source_order_warning_level\":\"warning\"",
                   "explicit sequential train SOURCE_ORDER warns loudly");
  require_contains(result,
                   "\"source_order_warnings\":\"train_wave_explicit_"
                   "sequential_source_order\"",
                   "explicit sequential train SOURCE_ORDER warning token");
}

void test_source_key_wave_preview_reports_key_bounds() {
  const auto dir = make_tmp_dir("source_key_wave");
  const auto wave_path = dir / "hero.runtime.wave.dsl";
  const auto config_path = dir / ".config";

  write_text(wave_path, "WAVE_SETTINGS {\n"
                        "  WAVE_ID = source_key_wave;\n"
                        "  TARGET = wikimyei.inference.expected_value.mdn;\n"
                        "  MODE = run|debug;\n"
                        "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                        "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
                        "  SOURCE_RANGE = source_key;\n"
                        "  SOURCE_KEY_BEGIN = 1002;\n"
                        "  SOURCE_KEY_END = 1004;\n"
                        "};\n");
  write_text(config_path, "[HERO]\n"
                          "runtime_wave_dsl_path = " +
                              wave_path.string() + "\n");

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;
  std::string result;
  std::string error;
  const bool ok = hero_runtime::execute_tool_json(
      "hero.runtime.inspect",
      "{\"subject\":\"wave\",\"config_path\":\"" + config_path.string() + "\"}",
      &ctx, &result, &error);
  check(ok, "hero.runtime.inspect subject=wave source-key preview failed: " +
                error);
  check(
      !hero_runtime::tool_result_is_error(result),
      "hero.runtime.inspect subject=wave source-key preview returned error: " +
          result);
  require_contains(result, "\"source_range\":\"source_key\"",
                   "source-key preview reports source range");
  require_contains(result, "\"source_key_begin\":\"1002\"",
                   "source-key preview reports begin");
  require_contains(result, "\"source_key_end\":\"1004\"",
                   "source-key preview reports end");
  std::filesystem::remove_all(dir);
}

void test_mdn_wave_preview_reads_jkimyei_model_state_inputs() {
  const auto dir = make_tmp_dir("mdn_jkimyei_inputs_preview");
  const auto wave_path = dir / "hero.runtime.wave.dsl";
  const auto jkimyei_path =
      dir / "wikimyei.inference.expected_value.mdn.jkimyei";
  const auto config_path = dir / ".config";

  write_text(wave_path, "WAVE_SETTINGS {\n"
                        "  WAVE_ID = mdn_train;\n"
                        "  TARGET = wikimyei.inference.expected_value.mdn;\n"
                        "  MODE = train|debug;\n"
                        "  SOURCE_CURSOR_KIND = graph_anchor;\n"
                        "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
                        "  SOURCE_RANGE = all;\n"
                        "};\n");
  write_text(jkimyei_path, "TRAINING {\n"
                           "  TRAINING_ID = mdn_train;\n"
                           "  TASK = mdn_expected_value_inference;\n"
                           "  COMPONENT_ASSEMBLY_ID = mdn_v1;\n"
                           "  LEARNING_RATE = 0.001;\n"
                           "  MAX_STEPS = 250;\n"
                           "  BATCH_SIZE = 64;\n"
                           "  INPUT_REPRESENTATION_CHECKPOINT = /tmp/rep.pt;\n"
                           "  INPUT_MDN_CHECKPOINT = /tmp/mdn.pt;\n"
                           "};\n");
  write_text(config_path, "[HERO]\n"
                          "runtime_wave_dsl_path = " +
                              wave_path.string() +
                              "\n"
                              "[JKIMYEI]\n"
                              "wikimyei_inference_expected_value_mdn_"
                              "jkimyei_path = " +
                              jkimyei_path.string() + "\n");

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;
  std::string result;
  std::string error;
  const bool ok = hero_runtime::execute_tool_json(
      "hero.runtime.inspect",
      "{\"subject\":\"wave\",\"config_path\":\"" + config_path.string() + "\"}",
      &ctx, &result, &error);
  check(ok, "hero.runtime.inspect subject=wave jkimyei-input preview failed: " +
                error);
  check(!hero_runtime::tool_result_is_error(result),
        "hero.runtime.inspect subject=wave jkimyei-input preview returned "
        "error: " +
            result);
  require_contains(
      result, "\"PLAN_INPUT_REPRESENTATION_CHECKPOINT\":\"/tmp/rep.pt\"",
      "MDN wave preview should expose jkimyei representation input");
  require_contains(result, "\"PLAN_INPUT_MDN_CHECKPOINT\":\"/tmp/mdn.pt\"",
                   "MDN wave preview should expose jkimyei MDN input");
  std::filesystem::remove_all(dir);
}

void test_policy_training_causal_schedule_contract() {
  namespace schedule_contract = cuwacunu::hero::runtime;
  schedule_contract::causal_policy_training_schedule_t schedule{};
  schedule.schedule_id = "causal_policy_training_schedule_1";
  schedule.cursor_key_kind = "numeric_anchor_index";
  schedule.training_range_digest = "range_train_digest";
  schedule.validation_range_digest = "range_validation_digest";
  schedule.test_range_digest = "range_test_digest";

  const auto make_artifact =
      [](std::string digest,
         std::string kind) -> schedule_contract::artifact_fit_ledger_t {
    schedule_contract::artifact_fit_ledger_t artifact{};
    artifact.artifact_digest = std::move(digest);
    artifact.artifact_kind = std::move(kind);
    artifact.artifact_schema_id = "artifact_schema_v1";
    artifact.producer_job_id = "producer_job";
    artifact.fit_cutoff_key = "100";
    artifact.fit_input_cutoff_key = "90";
    artifact.fit_target_cutoff_key = "100";
    artifact.normalization_cutoff_key = "100";
    artifact.calibration_cutoff_key = "100";
    artifact.covariance_fit_cutoff_key = "100";
    artifact.replay_buffer_cutoff_key = "100";
    artifact.target_label_available_from_key = "120";
    artifact.reward_available_from_key = "120";
    artifact.trajectory_usable_from_key = "120";
    artifact.usable_from_key = "120";
    artifact.embargo_policy_id = "target_horizon_embargo.v1";
    artifact.training_block_id = "B0";
    artifact.snapshot_generation = "0";
    return artifact;
  };
  schedule.artifact_fits.push_back(
      make_artifact("rep_snapshot_0", "representation"));
  schedule.artifact_fits.push_back(make_artifact("mdn_snapshot_0", "mdn"));
  schedule.artifact_fits.push_back(
      make_artifact("observer_snapshot_0", "observer_belief"));
  schedule.artifact_fits.push_back(
      make_artifact("policy_snapshot_0", "policy"));
  schedule.artifact_fits.push_back(
      make_artifact("normalization_snapshot_0", "normalization"));
  schedule.artifact_fits.push_back(
      make_artifact("calibration_snapshot_0", "calibration"));
  schedule.artifact_fits.push_back(
      make_artifact("covariance_snapshot_0", "covariance_coupler"));
  schedule.artifact_fits.push_back(
      make_artifact("replay_buffer_snapshot_0", "replay_buffer"));
  schedule.artifact_fits.push_back(
      make_artifact("reward_baseline_snapshot_0", "reward_baseline"));

  schedule_contract::snapshot_family_t family{};
  family.snapshot_family_digest = "snapshot_family_0";
  family.representation_snapshot_digest = "rep_snapshot_0";
  family.mdn_snapshot_digest = "mdn_snapshot_0";
  family.observer_belief_snapshot_digest = "observer_snapshot_0";
  family.policy_snapshot_digest = "policy_snapshot_0";
  family.normalization_snapshot_digest = "normalization_snapshot_0";
  family.calibration_snapshot_digest = "calibration_snapshot_0";
  family.covariance_coupler_snapshot_digest = "covariance_snapshot_0";
  family.replay_buffer_snapshot_digest = "replay_buffer_snapshot_0";
  family.reward_baseline_snapshot_digest = "reward_baseline_snapshot_0";
  schedule.snapshot_families.push_back(family);

  schedule_contract::causal_training_block_t block{};
  block.block_id = "B1";
  block.block_cursor_begin = "200";
  block.block_cursor_end = "300";
  block.block_digest = "block_digest_1";
  block.observation_count = 64;
  block.artifact_use.block_id = block.block_id;
  block.artifact_use.block_cursor_begin = block.block_cursor_begin;
  block.artifact_use.block_cursor_end = block.block_cursor_end;
  block.artifact_use.snapshot_family_digest_used =
      family.snapshot_family_digest;
  block.artifact_use.representation_snapshot_digest_used =
      family.representation_snapshot_digest;
  block.artifact_use.mdn_snapshot_digest_used = family.mdn_snapshot_digest;
  block.artifact_use.observer_belief_snapshot_digest_used =
      family.observer_belief_snapshot_digest;
  block.artifact_use.policy_snapshot_digest_used =
      family.policy_snapshot_digest;
  block.artifact_use.normalization_snapshot_digest_used =
      family.normalization_snapshot_digest;
  block.artifact_use.calibration_snapshot_digest_used =
      family.calibration_snapshot_digest;
  block.artifact_use.covariance_coupler_snapshot_digest_used =
      family.covariance_coupler_snapshot_digest;
  block.artifact_use.replay_buffer_snapshot_digest_used =
      family.replay_buffer_snapshot_digest;
  block.artifact_use.reward_baseline_snapshot_digest_used =
      family.reward_baseline_snapshot_digest;
  block.artifact_use.cajtucu_execution_profile_digest =
      "execution_profile_digest_v1";
  block.artifact_use.reward_contract_digest =
      "kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_"
      "drawdown.v1";
  block.artifact_use.no_future_snapshot_use = true;
  schedule.blocks.push_back(block);

  auto issues =
      schedule_contract::validate_causal_policy_training_schedule(schedule);
  check(issues.empty(), "causal walk-forward schedule should validate");
  check(!schedule_contract::policy_training_causal_schedule_digest(schedule)
             .empty(),
        "causal walk-forward schedule has a stable digest");
  check(
      schedule_contract::causal_policy_training_schedule_no_future_snapshot_use(
          schedule),
      "no-future-snapshot proof is derived from fit/use ledgers");

  auto declared_false = schedule;
  declared_false.blocks.front().artifact_use.no_future_snapshot_use = false;
  issues = schedule_contract::validate_causal_policy_training_schedule(
      declared_false);
  check(issues.empty(),
        "schedule validity derives no-future-snapshot use from ledgers rather "
        "than trusting the declaration flag");

  auto same_block = schedule;
  same_block.artifact_fits.front().training_block_id = "B1";
  issues =
      schedule_contract::validate_causal_policy_training_schedule(same_block);
  check(has_issue_containing(issues, "same_block_snapshot_use"),
        "schedule rejects snapshots trained on the same block they serve");

  auto future_use = schedule;
  future_use.artifact_fits.front().usable_from_key = "250";
  issues =
      schedule_contract::validate_causal_policy_training_schedule(future_use);
  check(has_issue_containing(issues, "future_snapshot_use"),
        "schedule rejects snapshots unavailable at block start");

  for (const auto &pair :
       {std::pair<std::size_t, std::string>{4, "normalization"},
        std::pair<std::size_t, std::string>{5, "calibration"},
        std::pair<std::size_t, std::string>{6, "covariance_coupler"},
        std::pair<std::size_t, std::string>{7, "replay_buffer"},
        std::pair<std::size_t, std::string>{8, "reward_baseline"}}) {
    auto future_support = schedule;
    future_support.artifact_fits[pair.first].usable_from_key = "250";
    issues = schedule_contract::validate_causal_policy_training_schedule(
        future_support);
    check(has_issue_containing(issues, "future_snapshot_use:" + pair.second),
          "schedule rejects future " + pair.second + " snapshot use");
  }

  auto late_target_label = schedule;
  late_target_label.artifact_fits.front().target_label_available_from_key =
      "130";
  issues = schedule_contract::validate_causal_policy_training_schedule(
      late_target_label);
  check(
      has_issue_containing(issues, "target_label_available_after_usable_from"),
      "schedule rejects labels unavailable by artifact usable_from_key");

  auto late_reward = schedule;
  late_reward.artifact_fits.front().reward_available_from_key = "130";
  issues =
      schedule_contract::validate_causal_policy_training_schedule(late_reward);
  check(has_issue_containing(issues, "reward_available_after_usable_from"),
        "schedule rejects rewards unavailable by artifact usable_from_key");

  auto late_trajectory = schedule;
  late_trajectory.artifact_fits.front().trajectory_usable_from_key = "130";
  issues = schedule_contract::validate_causal_policy_training_schedule(
      late_trajectory);
  check(
      has_issue_containing(issues, "trajectory_available_after_usable_from"),
      "schedule rejects trajectories unavailable by artifact usable_from_key");

  auto opaque_keys = schedule;
  opaque_keys.cursor_key_kind = "opaque_unsortable";
  issues =
      schedule_contract::validate_causal_policy_training_schedule(opaque_keys);
  check(has_issue_containing(issues, "opaque_or_unsupported_cursor_key_kind"),
        "schedule rejects opaque cursor keys without an ordering map");

  auto offline = schedule;
  offline.training_schedule_mode =
      schedule_contract::k_policy_training_schedule_mode_offline_research;
  issues = schedule_contract::validate_causal_policy_training_schedule(offline);
  check(has_issue_containing(
            issues, "offline_full_window_research_not_readiness_eligible"),
        "full-window research cannot satisfy readiness");
}

std::string valid_policy_training_args(
    std::string requested_mode = "plan", std::string policy_kind = "ppo",
    std::string policy_id = "wikimyei.policy.rl.ppo_portfolio.v0",
    std::string config_path = {}, bool include_wave_identity_fields = true) {
  std::ostringstream out;
  out << "{\"operation\":\"wave\",\"requested_mode\":\"" << requested_mode
      << "\"";
  if (!config_path.empty()) {
    out << ",\"config_path\":\"" << config_path << "\"";
  }
  out << ",\"protocol_id\":\"cwu_02v\","
         "\"protocol_contract_fingerprint\":\"contract_1\","
         "\"graph_order_fingerprint\":\"graph_1\","
         "\"source_cursor_token\":\"cursor_1\","
         "\"split_policy_fingerprint\":\"split_policy_1\","
         "\"component_assembly_fingerprint\":\"policy_trainable_1\",";
  if (include_wave_identity_fields) {
    out << "\"policy_id\":\"" << policy_id << "\","
        << "\"policy_kind\":\"" << policy_kind << "\",";
  }
  out << "\"policy_architecture_digest\":\"arch_digest_v0\","
         "\"training_config_digest\":\"train_config_digest_v0\","
         "\"training_range_digest\":\"range_train_digest\","
         "\"validation_range_digest\":\"range_validation_digest\","
         "\"test_range_digest\":\"range_test_digest\","
         "\"environment_contract_id\":\"kikijyeba.environment.replay.v1\","
         "\"observation_schema_digest\":\"kikijyeba.environment.policy_input."
         "v1\","
         "\"action_schema_digest\":\"kikijyeba.environment.action."
         "target_node_weights.v1\","
         "\"reward_contract_digest\":\"kikijyeba.environment.reward."
         "post_execution_ledger_log_growth_cost_drawdown.v1\","
         "\"policy_input_schema_id\":\"kikijyeba.environment.policy_input."
         "v1\","
         "\"action_adapter_id\":\"target_node_weights_simplex.v1\","
         "\"action_distribution_id\":\"masked_dirichlet_simplex.v1\","
         "\"reward_contract_id\":\"kikijyeba.environment.reward."
         "post_execution_ledger_log_growth_cost_drawdown.v1\","
         "\"execution_profile_digest\":\"execution_profile_digest_v1\",";
  if (policy_kind.find("ppo") != std::string::npos ||
      policy_kind.find("PPO") != std::string::npos) {
    out << "\"ppo_policy_artifact_contract_id\":\"kikijyeba.runtime."
           "ppo_policy_artifact_contract.v1\","
           "\"policy_family_id\":\"wikimyei.policy.portfolio."
           "graph_node_allocation\","
           "\"policy_checkpoint_schema_id\":\"wikimyei.policy.portfolio."
           "graph_node_allocation.ppo_checkpoint.v1\","
           "\"policy_input_feature_manifest_digest\":\"policy_input_manifest_"
           "digest_v1\","
           "\"action_distribution_config_digest\":\"action_distribution_config_"
           "digest_v1\","
           "\"snapshot_family_digest\":\"snapshot_family_digest_v1\","
           "\"actor_architecture_digest\":\"actor_arch_digest_v1\","
           "\"critic_architecture_digest\":\"critic_arch_digest_v1\","
           "\"actor_checkpoint_digest\":\"actor_checkpoint_digest_v1\","
           "\"critic_checkpoint_digest\":\"critic_checkpoint_digest_v1\","
           "\"optimizer_state_digest\":\"optimizer_state_digest_v1\","
           "\"ppo_config_digest\":\"ppo_config_digest_v1\","
           "\"advantage_estimator_id\":\"gae.v1\","
           "\"advantage_normalization_policy\":\"per_rollout_standardize_v1\","
           "\"rollout_collection_schema_id\":\"kikijyeba.runtime."
           "ppo_rollout_collection.v1\","
           "\"rollout_collection_digest\":\"rollout_collection_digest_v1\","
           "\"ppo_update_report_schema_id\":\"kikijyeba.runtime."
           "ppo_update_report.v1\","
           "\"ppo_update_report_digest\":\"ppo_update_report_digest_v1\","
           "\"validation_rollout_report_digest\":\"validation_rollout_report_"
           "digest_v1\","
           "\"ppo_gamma\":0.99,"
           "\"ppo_gae_lambda\":0.95,"
           "\"ppo_clip_epsilon\":0.2,"
           "\"ppo_target_kl\":0.02,"
           "\"ppo_entropy_coeff\":0.01,"
           "\"ppo_value_loss_coeff\":0.5,"
           "\"ppo_max_grad_norm\":0.5,"
           "\"ppo_minibatch_size\":32,"
           "\"ppo_epochs_per_rollout\":4,";
  }
  if (include_wave_identity_fields) {
    out << "\"training_schedule_mode\":\"causal_walk_forward_training.v1\",";
  }
  out << "\"causal_schedule_schema_id\":\"kikijyeba.runtime.policy_training_"
         "causal_schedule.v1\","
         "\"causal_schedule_digest\":\"causal_schedule_digest_v1\","
         "\"causal_schedule_cursor_key_kind\":\"numeric_anchor_index\","
         "\"causal_schedule_no_future_snapshot_use_source\":\"derived_from_"
         "artifact_fit_use_ledgers\","
         "\"normalization_fit_range_digest\":\"range_train_digest\","
         "\"replay_buffer_source_range_digest\":\"range_train_digest\","
         "\"early_stopping_policy_digest\":\"early_stop_digest_v1\","
         "\"hyperparameter_selection_policy_digest\":\"selector_digest_v1\","
         "\"selector_split\":\"validation\","
         "\"selector_policy_digest\":\"selector_policy_digest_v1\","
         "\"parent_checkpoint_digest\":\"parent_policy_checkpoint_digest\","
         "\"parent_forecast_eval_fact_digest\":\"forecast_eval_fact_digest\","
         "\"parent_observer_belief_fact_digest\":\"observer_belief_fact_"
         "digest\","
         "\"parent_allocation_engine_fact_digest\":\"allocation_engine_fact_"
         "digest\","
         "\"parent_replay_environment_fact_digest\":\"replay_env_fact_digest\","
         "\"random_seed\":0,"
         "\"max_episodes\":4,"
         "\"max_steps\":64,"
         "\"max_parallel_jobs\":1,"
         "\"max_wall_clock_seconds\":120,"
         "\"causal_schedule_readiness_eligible\":true,"
         "\"causal_schedule_no_future_snapshot_use\":true,"
         "\"offline_full_window_research_allowed\":false,"
         "\"final_refit_uses_validation\":false,"
         "\"validation_no_longer_proof\":false,"
         "\"sealed_test_required\":false,"
         "\"live_execution_allowed\":false}";
  return out.str();
}

void test_policy_training_contract_and_pre_ppo_execute() {
  hero_runtime::runtime_context_t ctx{};
  std::string result;
  std::string error;
  const bool plan_ok = hero_runtime::execute_tool_json(
      "hero.runtime.run", valid_policy_training_args("plan"), &ctx, &result,
      &error);
  check(plan_ok, "policy-training plan failed: " + error + " result=" + result);
  check(!hero_runtime::tool_result_is_error(result),
        "policy-training plan returned error: " + result);
  require_contains(result,
                   "\"schema_version\":\"kikijyeba.runtime.policy_training_"
                   "job_contract_packet.v1\"",
                   "policy-training plan reports contract packet schema");
  require_contains(result, "\"contract_digest\":",
                   "policy-training plan reports contract digest");
  require_contains(result, "\"policy_training_execution_supported\":true",
                   "PPO policy-training plan is executable through Runtime V0");
  require_contains(result,
                   "\"supported_execute_policy_kinds\":[\"noop_policy_"
                   "training.v1\",\"ppo_policy_adapter.v1\"]",
                   "policy-training plan reports PPO V0 execute support");
  require_contains(result,
                   "\"normalization_fit_range_digest\":\"range_train_digest\"",
                   "policy-training plan binds normalization fit range");
  require_contains(result,
                   "\"training_schedule_mode\":\"causal_walk_forward_"
                   "training.v1\"",
                   "policy-training plan binds causal schedule mode");
  require_contains(result,
                   "\"causal_schedule_digest\":\"causal_schedule_digest_v1\"",
                   "policy-training plan binds causal schedule digest");
  require_contains(result,
                   "\"policy_input_schema_id\":\"kikijyeba.environment."
                   "policy_input.v1\"",
                   "policy-training plan binds policy input schema");
  require_contains(result,
                   "\"action_adapter_id\":\"target_node_weights_simplex.v1\"",
                   "policy-training plan binds action adapter");
  require_contains(result,
                   "\"action_distribution_id\":\"masked_dirichlet_simplex.v1\"",
                   "policy-training plan binds action distribution");
  require_contains(result,
                   "\"ppo_policy_artifact_contract_id\":\"kikijyeba.runtime."
                   "ppo_policy_artifact_contract.v1\"",
                   "PPO policy-training plan binds artifact contract id");
  require_contains(result,
                   "\"actor_architecture_digest\":\"actor_arch_digest_v1\"",
                   "PPO policy-training plan binds actor architecture digest");
  require_contains(result,
                   "\"critic_architecture_digest\":\"critic_arch_digest_v1\"",
                   "PPO policy-training plan binds critic architecture digest");
  require_contains(result, "\"ppo_config_digest\":\"ppo_config_digest_v1\"",
                   "PPO policy-training plan binds PPO config digest");
  require_contains(result,
                   "\"rollout_collection_digest\":\"rollout_collection_digest_"
                   "v1\"",
                   "PPO policy-training plan binds rollout collection digest");
  require_contains(result,
                   "\"ppo_update_report_digest\":\"ppo_update_report_digest_"
                   "v1\"",
                   "PPO policy-training plan binds PPO update report digest");
  require_contains(result,
                   "\"reward_contract_id\":\"kikijyeba.environment.reward."
                   "post_execution_ledger_log_growth_cost_drawdown.v1\"",
                   "policy-training plan binds post-execution reward contract");
  require_contains(
      result, "\"causal_schedule_cursor_key_kind\":\"numeric_anchor_index\"",
      "policy-training plan binds causal cursor key ordering");
  require_contains(result,
                   "\"causal_schedule_no_future_snapshot_use_source\":"
                   "\"derived_from_artifact_fit_use_ledgers\"",
                   "policy-training plan binds derived no-future proof source");
  require_contains(result, "\"runtime_trains_policy\":true",
                   "policy-training plan exposes Runtime train authority for "
                   "supported PPO V0");

  const auto wave_defaults_root = make_tmp_dir("policy_training_wave_defaults");
  const auto wave_path = wave_defaults_root / "hero.runtime.wave.dsl";
  const auto config_path = wave_defaults_root / ".config";
  write_text(wave_path,
             "WAVE_SELECTION {\n"
             "  ACTIVE_WAVE_ID = policy_training_pre_ppo_noop;\n"
             "};\n"
             "WAVE_SETTINGS {\n"
             "  WAVE_ID = policy_training_pre_ppo_noop;\n"
             "  COMPATIBLE_PROTOCOLS = cwu_02v;\n"
             "  TARGET = wikimyei.policy.trainable;\n"
             "  MODE = train;\n"
             "  JOB_KIND = policy_training;\n"
             "  POLICY_ID = wikimyei.policy.trainable.noop_pre_ppo;\n"
             "  POLICY_KIND = noop_policy_training.v1;\n"
             "  TRAINING_SCHEDULE_MODE = causal_walk_forward_training.v1;\n"
             "  LIVE_EXECUTION_ALLOWED = false;\n"
             "  SOURCE_CURSOR_KIND = graph_anchor;\n"
             "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
             "  SOURCE_RANGE = all;\n"
             "  SOURCE_ORDER = random_per_epoch;\n"
             "};\n");
  write_text(config_path,
             "[HERO]\n"
             "runtime_wave_dsl_path = " +
                 wave_path.string() +
                 "\n"
                 "runtime_wave_id = policy_training_pre_ppo_noop\n");
  hero_runtime::runtime_context_t wave_ctx{};
  wave_ctx.global_config_path = config_path;
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json(
            "hero.runtime.inspect",
            "{\"subject\":\"wave\",\"config_path\":\"" + config_path.string() +
                "\"}",
            &wave_ctx, &result, &error),
        "policy-training wave inspect failed: " + error);
  require_contains(result, "\"job_kind\":\"policy_training\"",
                   "policy-training wave exposes policy_training job kind");
  require_contains(result, "\"policy_training_wave\":true",
                   "policy-training wave is recognized by Runtime");
  require_contains(result, "\"policy_kind\":\"noop_policy_training.v1\"",
                   "policy-training wave exposes policy kind");
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json(
            "hero.runtime.run",
            valid_policy_training_args("plan", "unused", "unused",
                                       config_path.string(), false),
            &wave_ctx, &result, &error),
        "policy-training wave-default plan failed: " + error +
            " result=" + result);
  require_contains(result,
                   "\"policy_id\":\"wikimyei.policy.trainable.noop_pre_ppo\"",
                   "policy-training contract should take policy_id from wave");
  require_contains(
      result, "\"policy_kind\":\"noop_policy_training.v1\"",
      "policy-training contract should take policy_kind from wave");
  require_contains(result,
                   "\"training_schedule_mode\":\"causal_walk_forward_"
                   "training.v1\"",
                   "policy-training contract should take schedule mode from "
                   "wave");
  std::filesystem::remove_all(wave_defaults_root);

  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run",
                                        valid_policy_training_args("dry_run"),
                                        &ctx, &result, &error),
        "policy-training dry_run failed: " + error);
  require_contains(result, "\"requested_mode\":\"dry_run\"",
                   "policy-training dry_run reports requested mode");

  std::string leaking_args = valid_policy_training_args("plan");
  const std::string validation_digest_field =
      "\"validation_range_digest\":\"range_validation_digest\"";
  const auto validation_pos = leaking_args.find(validation_digest_field);
  check(validation_pos != std::string::npos,
        "test fixture should contain validation range digest");
  leaking_args.replace(validation_pos, validation_digest_field.size(),
                       "\"validation_range_digest\":\"range_train_digest\"");
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run", leaking_args, &ctx,
                                         &result, &error),
        "policy-training contract should reject train/validation overlap");
  require_contains(error, "training_validation_range_overlap",
                   "policy-training overlap refusal should name the issue");

  std::string scheduleless_args = valid_policy_training_args("plan");
  const std::string schedule_digest_field =
      "\"causal_schedule_digest\":\"causal_schedule_digest_v1\",";
  const auto schedule_digest_pos =
      scheduleless_args.find(schedule_digest_field);
  check(schedule_digest_pos != std::string::npos,
        "test fixture should contain causal schedule digest");
  scheduleless_args.erase(schedule_digest_pos, schedule_digest_field.size());
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run", scheduleless_args,
                                         &ctx, &result, &error),
        "policy-training contract should reject missing causal schedule");
  require_contains(error, "missing required field: causal_schedule_digest",
                   "missing causal schedule refusal should be stable");

  std::string missing_source_args = valid_policy_training_args("plan");
  const std::string schedule_source_field =
      "\"causal_schedule_no_future_snapshot_use_source\":\"derived_from_"
      "artifact_fit_use_ledgers\",";
  const auto schedule_source_pos =
      missing_source_args.find(schedule_source_field);
  check(schedule_source_pos != std::string::npos,
        "test fixture should contain causal schedule no-future source");
  missing_source_args.erase(schedule_source_pos, schedule_source_field.size());
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json(
            "hero.runtime.run", missing_source_args, &ctx, &result, &error),
        "policy-training contract should reject missing causal schedule "
        "no-future source");
  require_contains(
      error,
      "missing required field: causal_schedule_no_future_snapshot_use_source",
      "missing causal schedule no-future source refusal should be stable");

  std::string missing_policy_input_args = valid_policy_training_args("plan");
  const std::string policy_input_field =
      "\"policy_input_schema_id\":\"kikijyeba.environment.policy_input.v1\",";
  const auto policy_input_pos =
      missing_policy_input_args.find(policy_input_field);
  check(policy_input_pos != std::string::npos,
        "test fixture should contain policy input schema id");
  missing_policy_input_args.erase(policy_input_pos, policy_input_field.size());
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run",
                                         missing_policy_input_args, &ctx,
                                         &result, &error),
        "policy-training contract should reject missing policy input schema");
  require_contains(error, "missing required field: policy_input_schema_id",
                   "missing policy input schema refusal should be stable");

  std::string old_action_schema_args = valid_policy_training_args("plan");
  const std::string action_schema_field =
      "\"action_schema_digest\":\"kikijyeba.environment.action."
      "target_node_weights.v1\",";
  const auto action_schema_pos =
      old_action_schema_args.find(action_schema_field);
  check(action_schema_pos != std::string::npos,
        "test fixture should contain unified action schema");
  old_action_schema_args.replace(
      action_schema_pos, action_schema_field.size(),
      "\"action_schema_digest\":\"kikijyeba.environment.action."
      "target_weights.v1\",");
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json(
            "hero.runtime.run", old_action_schema_args, &ctx, &result, &error),
        "policy-training contract should reject old target_weights action "
        "schema");
  require_contains(error, "unsupported_action_schema_digest",
                   "old action schema refusal should be stable");

  std::string bad_distribution_args = valid_policy_training_args("plan");
  const std::string action_distribution_field =
      "\"action_distribution_id\":\"masked_dirichlet_simplex.v1\",";
  const auto action_distribution_pos =
      bad_distribution_args.find(action_distribution_field);
  check(action_distribution_pos != std::string::npos,
        "test fixture should contain action distribution id");
  bad_distribution_args.replace(
      action_distribution_pos, action_distribution_field.size(),
      "\"action_distribution_id\":\"unsupported_simplex_policy.v1\",");
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json(
            "hero.runtime.run", bad_distribution_args, &ctx, &result, &error),
        "policy-training contract should reject unsupported action "
        "distribution");
  check(error.find("unsupported_action_distribution_id") != std::string::npos ||
            error.find("action_distribution_id") != std::string::npos,
        "unsupported action-distribution refusal should name the field or "
        "contract issue: " +
            error);

  std::string asserted_source_args = valid_policy_training_args("plan");
  const auto asserted_source_pos =
      asserted_source_args.find(schedule_source_field);
  check(asserted_source_pos != std::string::npos,
        "test fixture should contain causal schedule no-future source");
  asserted_source_args.replace(
      asserted_source_pos, schedule_source_field.size(),
      "\"causal_schedule_no_future_snapshot_use_source\":\"asserted_by_"
      "caller\",");
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json(
            "hero.runtime.run", asserted_source_args, &ctx, &result, &error),
        "policy-training contract should reject caller-asserted no-future "
        "source");
  require_contains(error, "unsupported_no_future_snapshot_use_source",
                   "caller-asserted no-future source refusal should be stable");

  std::string opaque_cursor_args = valid_policy_training_args("plan");
  const std::string cursor_kind_field =
      "\"causal_schedule_cursor_key_kind\":\"numeric_anchor_index\",";
  const auto cursor_kind_pos = opaque_cursor_args.find(cursor_kind_field);
  check(cursor_kind_pos != std::string::npos,
        "test fixture should contain causal schedule cursor kind");
  opaque_cursor_args.replace(
      cursor_kind_pos, cursor_kind_field.size(),
      "\"causal_schedule_cursor_key_kind\":\"opaque_unsortable\",");
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run", opaque_cursor_args,
                                         &ctx, &result, &error),
        "policy-training contract should reject opaque cursor key ordering");
  require_contains(error,
                   "opaque_or_unsupported_causal_schedule_cursor_key_kind",
                   "opaque cursor key refusal should be stable");

  const auto expect_missing_ppo_contract_field =
      [&](const std::string &field_text, const std::string &expected_issue,
          const std::string &label) {
        std::string args = valid_policy_training_args("plan");
        const auto pos = args.find(field_text);
        check(pos != std::string::npos, "PPO fixture should contain " + label);
        args.erase(pos, field_text.size());
        result.clear();
        error.clear();
        check(!hero_runtime::execute_tool_json("hero.runtime.run", args, &ctx,
                                               &result, &error),
              "policy-training contract should reject missing " + label);
        require_contains(error, expected_issue,
                         "missing " + label + " refusal should be stable");
      };
  expect_missing_ppo_contract_field(
      "\"actor_architecture_digest\":\"actor_arch_digest_v1\",",
      "missing_actor_architecture_digest", "actor architecture digest");
  expect_missing_ppo_contract_field(
      "\"critic_architecture_digest\":\"critic_arch_digest_v1\",",
      "missing_critic_architecture_digest", "critic architecture digest");
  expect_missing_ppo_contract_field(
      "\"actor_checkpoint_digest\":\"actor_checkpoint_digest_v1\",",
      "missing_actor_checkpoint_digest", "actor checkpoint digest");
  expect_missing_ppo_contract_field(
      "\"critic_checkpoint_digest\":\"critic_checkpoint_digest_v1\",",
      "missing_critic_checkpoint_digest", "critic checkpoint digest");
  expect_missing_ppo_contract_field(
      "\"ppo_config_digest\":\"ppo_config_digest_v1\",",
      "missing_ppo_config_digest", "PPO config digest");
  expect_missing_ppo_contract_field(
      "\"rollout_collection_digest\":\"rollout_collection_digest_v1\",",
      "missing_rollout_collection_digest", "rollout collection digest");
  expect_missing_ppo_contract_field(
      "\"ppo_update_report_digest\":\"ppo_update_report_digest_v1\",",
      "missing_ppo_update_report_digest", "PPO update report digest");

  std::string offline_args = valid_policy_training_args("plan");
  const std::string causal_mode =
      "\"training_schedule_mode\":\"causal_walk_forward_training.v1\"";
  const auto causal_mode_pos = offline_args.find(causal_mode);
  check(causal_mode_pos != std::string::npos,
        "test fixture should contain causal schedule mode");
  offline_args.replace(causal_mode_pos, causal_mode.size(),
                       "\"training_schedule_mode\":\"offline_full_window_"
                       "research\"");
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run", offline_args, &ctx,
                                         &result, &error),
        "policy-training contract should reject full-window research as "
        "readiness evidence");
  require_contains(error, "offline_full_window_research_not_readiness_eligible",
                   "full-window research refusal should name readiness leak");

  result.clear();
  error.clear();
  const auto ppo_runtime_root = make_tmp_dir("policy_training_ppo_execute");
  hero_runtime::runtime_context_t ppo_ctx{};
  ppo_ctx.global_config_path = ppo_runtime_root / ".config";
  ppo_ctx.policy.policy_path = ppo_runtime_root / "hero.runtime.dsl";
  ppo_ctx.policy.global_config_path = ppo_ctx.global_config_path;
  ppo_ctx.policy.values["runtime_root"] = ppo_runtime_root.string();
  ppo_ctx.policy.values["allowed_job_roots"] = ppo_runtime_root.string();
  ppo_ctx.policy.values["allow_execute"] = "true";
  ppo_ctx.policy.values["allow_train_execute"] = "true";
  ppo_ctx.policy.values["require_confirm_execute"] = "true";
  ppo_ctx.policy.values["allow_dev_nuke"] = "true";
  ppo_ctx.policy.values["require_confirm_dev_nuke"] = "true";
  ppo_ctx.policy.values["dev_nuke_backup_enabled"] = "false";
  ppo_ctx.policy.values["dev_nuke_backup_root"] =
      (ppo_runtime_root.parent_path() / "ppo_dev_nuke_backups").string();
  ppo_ctx.policy.values["allowed_dev_nuke_roots"] = ppo_runtime_root.string();

  const auto ppo_stale_job_dir = ppo_runtime_root / "stale_previous_run";
  std::filesystem::create_directories(ppo_stale_job_dir);
  write_text(ppo_stale_job_dir / "stale.marker",
             "previous_runtime_state=true\n");
  std::string ppo_reset_args =
      "{\"requested_mode\":\"plan\",\"runtime_root\":\"" +
      ppo_runtime_root.string() + "\"}";
  check(hero_runtime::execute_tool_json("hero.runtime.reset", ppo_reset_args,
                                        &ppo_ctx, &result, &error),
        "PPO fresh-run reset plan failed: " + error);
  require_contains(result, "\"dry_run\":true",
                   "PPO fresh-run reset plan stays dry-run");
  require_contains(result, "\"would_clear\":true",
                   "PPO fresh-run reset plan detects stale runtime state");
  ppo_reset_args = "{\"requested_mode\":\"execute\",\"runtime_root\":\"" +
                   ppo_runtime_root.string() + "\",\"confirm_dev_nuke\":true}";
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.reset", ppo_reset_args,
                                        &ppo_ctx, &result, &error),
        "PPO fresh-run reset execute failed: " + error);
  require_contains(result, "\"dry_run\":false",
                   "PPO fresh-run reset execute is non-dry-run");
  require_contains(result, "\"cleared\":true",
                   "PPO fresh-run reset clears stale runtime state");
  check(!std::filesystem::exists(ppo_stale_job_dir),
        "PPO fresh-run reset should remove stale previous run state");

  result.clear();
  error.clear();
  std::string ppo_execute_args = valid_policy_training_args("execute");
  const std::string ppo_live_forbidden = "\"live_execution_allowed\":false}";
  const auto ppo_live_forbidden_pos = ppo_execute_args.find(ppo_live_forbidden);
  check(ppo_live_forbidden_pos != std::string::npos,
        "PPO test fixture should contain live execution denial");
  ppo_execute_args.replace(ppo_live_forbidden_pos, ppo_live_forbidden.size(),
                           "\"live_execution_allowed\":false,"
                           "\"confirm_execute\":true}");
  check(hero_runtime::execute_tool_json("hero.runtime.run", ppo_execute_args,
                                        &ppo_ctx, &result, &error),
        "PPO policy-training execute failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "PPO policy-training execute returned error: " + result);
  require_contains(result, "\"trainer_kind\":\"ppo_policy_adapter.v1\"",
                   "PPO execute binds trainer kind");
  require_contains(result, "\"ppo_implemented\":true",
                   "PPO execute reports PPO implementation");
  require_contains(result, "\"runtime_trains_ppo\":true",
                   "PPO execute declares Runtime PPO training authority");
  require_contains(result, "\"runtime_executes_live_capital\":false",
                   "PPO execute denies live capital");
  require_contains(result, "\"actor_checkpoint_digest\":",
                   "PPO execute reports actor checkpoint digest");
  require_contains(result, "\"critic_checkpoint_digest\":",
                   "PPO execute reports critic checkpoint digest");
  require_contains(result, "\"rollout_collection_digest\":",
                   "PPO execute reports rollout collection digest");
  require_contains(result, "\"ppo_update_report_digest\":",
                   "PPO execute reports PPO update report digest");
  require_contains(
      result, "\"kikijyeba_cajtucu_replay_integration\":false",
      "PPO smoke execute still declares missing replay integration");
  const auto ppo_manifest_path =
      std::filesystem::path(json_string_field(result, "manifest_path"));
  const auto ppo_manifest_text = read_text(ppo_manifest_path);
  require_contains(ppo_manifest_text,
                   "protocol_status=ppo_v0_runtime_smoke_execution",
                   "PPO manifest records PPO smoke status");
  require_contains(ppo_manifest_text, "wave_id=policy_training_ppo_v0",
                   "PPO manifest records PPO wave id");
  require_contains(ppo_manifest_text,
                   "runtime.policy_training.ppo_v0:"
                   "write_checkpoint_rollout_update",
                   "PPO manifest records PPO execution chain");
  const auto ppo_rollout_collection_path = std::filesystem::path(
      json_string_field(result, "rollout_collection_path"));
  const auto ppo_rollout_text = read_text(ppo_rollout_collection_path);
  require_contains(ppo_rollout_text, "replay_step_schema_compatible=true",
                   "PPO rollout collection declares replay-step compatibility");
  require_contains(ppo_rollout_text, "collection_source=runtime_smoke_fixture",
                   "PPO rollout collection keeps fixture source explicit");
  require_contains(ppo_rollout_text, "replay_backed_step_count=0",
                   "PPO rollout collection does not claim replay-backed steps");
  require_contains(
      ppo_rollout_text, "step_0_policy_input_digest=",
      "PPO rollout collection records per-step policy input digest");
  require_contains(
      ppo_rollout_text,
      "step_0_action_distribution_id=masked_dirichlet_simplex.v1",
      "PPO rollout collection records action distribution evidence");
  require_contains(ppo_rollout_text, "step_0_old_log_prob=",
                   "PPO rollout collection records old log probability");
  require_contains(ppo_rollout_text, "step_0_prob_ratio=",
                   "PPO rollout collection records PPO probability ratio");
  require_contains(
      ppo_rollout_text, "step_0_cajtucu_trace_digest_bound=false",
      "PPO rollout collection does not fake Cajtucu trace binding");
  require_contains(ppo_rollout_text, "step_0_transaction_cost_numeraire=",
                   "PPO rollout collection records transaction cost field");
  require_contains(ppo_rollout_text, "step_0_reward_total=",
                   "PPO rollout collection records reward field");
  require_contains(ppo_rollout_text, "step_3_done=true",
                   "PPO rollout collection records terminal step flag");
  const auto ppo_update_report_path = std::filesystem::path(
      json_string_field(result, "ppo_update_report_path"));
  const auto ppo_update_text = read_text(ppo_update_report_path);
  require_contains(ppo_update_text, "ppo_algorithm=ppo_clip",
                   "PPO update report records PPO-Clip");
  require_contains(ppo_update_text, "advantage_estimator_id=gae.v1",
                   "PPO update report records GAE");
  require_contains(ppo_update_text, "update_sample_count=4",
                   "PPO update report records per-sample update count");
  require_contains(ppo_update_text, "sample_0_old_log_prob=",
                   "PPO update report records old log probability");
  require_contains(ppo_update_text, "sample_0_new_log_prob=",
                   "PPO update report records new log probability");
  require_contains(ppo_update_text, "sample_0_advantage=",
                   "PPO update report records sample advantage");
  require_contains(ppo_update_text, "sample_0_normalized_advantage=",
                   "PPO update report records normalized advantage");
  require_contains(ppo_update_text, "sample_0_return_target=",
                   "PPO update report records return target");
  require_contains(ppo_update_text, "sample_0_policy_loss_final=",
                   "PPO update report records clipped policy loss");
  require_contains(ppo_update_text, "sample_0_approx_kl=",
                   "PPO update report records sample KL estimate");
  require_contains(ppo_update_text,
                   "parameter_update_mode=bounded_ppo_v0_module_head_delta_"
                   "update",
                   "PPO update report records bounded parameter update mode");
  require_contains(ppo_update_text, "parameter_update_applied=true",
                   "PPO update report records applied parameter update");
  require_contains(ppo_update_text, "updated_node_weight_logits=",
                   "PPO update report records learned actor logits");
  require_contains(ppo_update_text, "updated_value_estimate=",
                   "PPO update report records learned value estimate");
  require_contains(ppo_update_text, "finite_loss_check=true",
                   "PPO update report records finite loss check");
  const auto ppo_optimizer_state_path =
      std::filesystem::path(json_string_field(result, "optimizer_state_path"));
  const auto ppo_optimizer_state_text = read_text(ppo_optimizer_state_path);
  require_contains(ppo_optimizer_state_text,
                   "state_kind=bounded_ppo_v0_update_state",
                   "PPO optimizer state records update-state kind");
  require_contains(ppo_optimizer_state_text, "parameter_update_applied=true",
                   "PPO optimizer state records applied parameter update");
  require_contains(ppo_optimizer_state_text, "learning_rate_actor=",
                   "PPO optimizer state records actor learning rate");
  const auto ppo_fact_path = std::filesystem::path(
      json_string_field(result, "policy_training_fact_path"));
  const auto ppo_fact_text = read_text(ppo_fact_path);
  require_contains(ppo_fact_text, "runtime_trainer=false",
                   "PPO fact remains artifact evidence, not trainer authority");
  require_contains(ppo_fact_text, "policy_training_authority=false",
                   "PPO fact denies policy-training authority");
  require_contains(ppo_fact_text, "quality_authority=false",
                   "PPO fact still denies quality authority");
  require_contains(ppo_fact_text, "actor_checkpoint_digest=",
                   "PPO fact carries emitted actor checkpoint digest");
  require_contains(ppo_fact_text, "input_policy_checkpoint_path=",
                   "PPO fact carries collection actor checkpoint path");
  require_contains(ppo_fact_text, "input_policy_checkpoint_digest=",
                   "PPO fact carries collection actor checkpoint digest");
  require_contains(ppo_fact_text, "ppo_update_report_digest=",
                   "PPO fact carries emitted update report digest");
  lattice_exposure::lattice_exposure_fact_t parent_fact{};
  const auto parsed_ppo_facts =
      lattice_exposure::make_policy_training_facts_from_job_dir(
          ppo_fact_path.parent_path(), parent_fact);
  check(parsed_ppo_facts.size() == 1U,
        "Lattice exposure should parse emitted PPO policy-training fact");
  check(parsed_ppo_facts.front().policy_kind == "ppo",
        "parsed PPO fact preserves policy kind");
  check(!parsed_ppo_facts.front().actor_checkpoint_digest.empty(),
        "parsed PPO fact carries actor checkpoint digest");
  check(!parsed_ppo_facts.front().ppo_update_report_digest.empty(),
        "parsed PPO fact carries PPO update report digest");

  const auto ppo_replay_report_path =
      ppo_runtime_root / "ppo_input_replay.report";
  write_text(ppo_replay_report_path,
             "schema=kikijyeba.environment.replay.report.v1\n"
             "runtime_run_id=runtime_replay_fixture\n"
             "environment_run_id=env_replay_fixture\n"
             "episode_count=1\n"
             "episode_0_step_count=2\n"
             "episode_0_step_0_step_index=0\n"
             "episode_0_step_0_episode_id=episode_replay_fixture\n"
             "episode_0_step_0_anchor_key=anchor_0000\n"
             "episode_0_step_0_observation_anchor_index=100\n"
             "episode_0_step_0_knowledge_timestamp_ms=100000\n"
             "episode_0_step_0_accepted_anchor_index_begin=100\n"
             "episode_0_step_0_accepted_anchor_index_end=101\n"
             "episode_0_step_0_policy_input_digest=policy_input_digest_0\n"
             "episode_0_step_0_action_distribution_id="
             "masked_dirichlet_simplex.v1\n"
             "episode_0_step_0_active_node_indices=0,1\n"
             "episode_0_step_0_active_count=2\n"
             "episode_0_step_0_old_log_prob=-0.812\n"
             "episode_0_step_0_old_entropy=0.71\n"
             "episode_0_step_0_old_value_estimate=0.006\n"
             "episode_0_step_0_action_distribution_evidence_bound=true\n"
             "episode_0_step_0_target_node_weights=BTC:0.60,ETH:0.40\n"
             "episode_0_step_0_reward_total=0.004\n"
             "episode_0_step_0_reward_log_growth=0.005\n"
             "episode_0_step_0_reward_transaction_cost_penalty=-0.001\n"
             "episode_0_step_0_transaction_cost_numeraire=0.00021\n"
             "episode_0_step_0_turnover=0.18\n"
             "episode_0_step_0_cajtucu_execution_trace_available=true\n"
             "episode_0_step_0_cajtucu_trace_valid=true\n"
             "episode_0_step_0_cajtucu_trace_id=trace_fixture_0\n"
             "episode_0_step_0_cajtucu_total_fee_numeraire=0.00005\n"
             "episode_0_step_0_cajtucu_total_spread_cost_numeraire=0.00007\n"
             "episode_0_step_0_cajtucu_total_slippage_numeraire=0.00009\n"
             "episode_0_step_0_cajtucu_rejected_notional_numeraire=0.0\n"
             "episode_0_step_0_cajtucu_partial_notional_numeraire=0.0\n"
             "episode_0_step_0_cajtucu_missing_direct_pair_count=0\n"
             "episode_0_step_0_cajtucu_numeraire_fallback_pair_count=0\n"
             "episode_0_step_0_cajtucu_rejected_fill_count=0\n"
             "episode_0_step_0_cajtucu_partial_fill_count=0\n"
             "episode_0_step_0_invalid_action=false\n"
             "episode_0_step_0_done=false\n"
             "episode_0_step_1_step_index=1\n"
             "episode_0_step_1_episode_id=episode_replay_fixture\n"
             "episode_0_step_1_anchor_key=anchor_0001\n"
             "episode_0_step_1_observation_anchor_index=101\n"
             "episode_0_step_1_knowledge_timestamp_ms=101000\n"
             "episode_0_step_1_accepted_anchor_index_begin=101\n"
             "episode_0_step_1_accepted_anchor_index_end=102\n"
             "episode_0_step_1_policy_input_digest=policy_input_digest_1\n"
             "episode_0_step_1_action_distribution_id="
             "masked_dirichlet_simplex.v1\n"
             "episode_0_step_1_active_node_indices=0,1\n"
             "episode_0_step_1_active_count=2\n"
             "episode_0_step_1_old_log_prob=-0.744\n"
             "episode_0_step_1_old_entropy=0.69\n"
             "episode_0_step_1_old_value_estimate=0.002\n"
             "episode_0_step_1_action_distribution_evidence_bound=true\n"
             "episode_0_step_1_target_node_weights=BTC:0.55,ETH:0.45\n"
             "episode_0_step_1_reward_total=-0.002\n"
             "episode_0_step_1_reward_log_growth=-0.001\n"
             "episode_0_step_1_reward_turnover_penalty=-0.001\n"
             "episode_0_step_1_transaction_cost_numeraire=0.00013\n"
             "episode_0_step_1_turnover=0.05\n"
             "episode_0_step_1_cajtucu_execution_trace_available=true\n"
             "episode_0_step_1_cajtucu_trace_valid=true\n"
             "episode_0_step_1_cajtucu_trace_id=trace_fixture_1\n"
             "episode_0_step_1_cajtucu_total_fee_numeraire=0.00003\n"
             "episode_0_step_1_cajtucu_total_spread_cost_numeraire=0.00004\n"
             "episode_0_step_1_cajtucu_total_slippage_numeraire=0.00006\n"
             "episode_0_step_1_cajtucu_rejected_notional_numeraire=0.2\n"
             "episode_0_step_1_cajtucu_partial_notional_numeraire=0.1\n"
             "episode_0_step_1_cajtucu_missing_direct_pair_count=1\n"
             "episode_0_step_1_cajtucu_numeraire_fallback_pair_count=0\n"
             "episode_0_step_1_cajtucu_rejected_fill_count=1\n"
             "episode_0_step_1_cajtucu_partial_fill_count=1\n"
             "episode_0_step_1_invalid_action=false\n"
             "episode_0_step_1_done=true\n");
  std::string ppo_replay_args = valid_policy_training_args("execute");
  const auto ppo_replay_live_pos = ppo_replay_args.find(ppo_live_forbidden);
  check(ppo_replay_live_pos != std::string::npos,
        "PPO replay fixture should contain live execution denial");
  ppo_replay_args.replace(ppo_replay_live_pos, ppo_live_forbidden.size(),
                          "\"live_execution_allowed\":false,"
                          "\"confirm_execute\":true,"
                          "\"job_id\":\"ppo_replay_ingest\","
                          "\"report_path\":\"" +
                              ppo_replay_report_path.string() + "\"}");
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", ppo_replay_args,
                                        &ppo_ctx, &result, &error),
        "PPO replay-report execute failed: " + error);
  require_contains(result,
                   "\"rollout_collection_source\":\"kikijyeba_replay_"
                   "report\"",
                   "PPO replay execute reports replay collection source");
  require_contains(result, "\"replay_backed_step_count\":2",
                   "PPO replay execute reports replay-backed sample count");
  require_contains(result, "\"kikijyeba_cajtucu_replay_integration\":true",
                   "PPO replay execute reports replay integration");
  require_contains(result, "\"policy_distribution_evidence_bound\":true",
                   "PPO replay execute requires policy distribution evidence");
  const auto ppo_replay_manifest_path =
      std::filesystem::path(json_string_field(result, "manifest_path"));
  const auto ppo_replay_manifest_text = read_text(ppo_replay_manifest_path);
  require_contains(ppo_replay_manifest_text,
                   "protocol_status=ppo_v0_runtime_replay_report_ingestion",
                   "PPO replay manifest records replay ingestion status");
  require_contains(ppo_replay_manifest_text,
                   "ingest_replay_report_write_checkpoint_rollout_update",
                   "PPO replay manifest records replay ingestion chain");
  const auto ppo_replay_rollout_path = std::filesystem::path(
      json_string_field(result, "rollout_collection_path"));
  const auto ppo_replay_rollout_text = read_text(ppo_replay_rollout_path);
  require_contains(ppo_replay_rollout_text,
                   "collection_source=kikijyeba_replay_report",
                   "PPO replay rollout records replay source");
  require_contains(ppo_replay_rollout_text, "replay_backed_step_count=2",
                   "PPO replay rollout records replay-backed step count");
  require_contains(ppo_replay_rollout_text, "fixture_step_count=0",
                   "PPO replay rollout does not mix smoke fixture steps");
  require_contains(ppo_replay_rollout_text,
                   "policy_distribution_evidence_bound=true",
                   "PPO replay rollout binds distribution evidence");
  require_contains(ppo_replay_rollout_text,
                   "step_0_source=kikijyeba_replay_report",
                   "PPO replay rollout records per-step replay source");
  require_contains(ppo_replay_rollout_text, "step_0_replay_backed=true",
                   "PPO replay rollout marks step replay-backed");
  require_contains(ppo_replay_rollout_text,
                   "step_0_policy_input_digest=policy_input_digest_0",
                   "PPO replay rollout preserves replay policy input digest");
  require_contains(ppo_replay_rollout_text, "step_0_old_log_prob=-0.812",
                   "PPO replay rollout preserves old policy log probability");
  require_contains(ppo_replay_rollout_text, "step_0_old_value_estimate=0.006",
                   "PPO replay rollout preserves old value estimate");
  require_contains(ppo_replay_rollout_text,
                   "step_0_cajtucu_trace_id=trace_fixture_0",
                   "PPO replay rollout records Cajtucu trace id");
  require_contains(ppo_replay_rollout_text,
                   "step_0_cajtucu_trace_digest_bound=true",
                   "PPO replay rollout binds Cajtucu trace id into a digest");
  require_contains(ppo_replay_rollout_text,
                   "step_0_transaction_cost_numeraire=0.00021",
                   "PPO replay rollout preserves transaction cost");
  require_contains(ppo_replay_rollout_text,
                   "step_1_missing_direct_pair_count=1",
                   "PPO replay rollout preserves missing-pair counters");
  require_contains(ppo_replay_rollout_text,
                   "step_1_rejected_notional_numeraire=0.2",
                   "PPO replay rollout preserves rejected notional");
  require_contains(ppo_replay_rollout_text, "step_1_done=true",
                   "PPO replay rollout records terminal replay step");

  const auto ppo_legacy_replay_report_path =
      ppo_runtime_root / "ppo_legacy_input_replay_missing_distribution.report";
  write_text(ppo_legacy_replay_report_path,
             "schema=kikijyeba.environment.replay.report.v1\n"
             "runtime_run_id=runtime_legacy_replay_fixture\n"
             "environment_run_id=env_legacy_replay_fixture\n"
             "episode_count=1\n"
             "episode_0_step_count=1\n"
             "episode_0_step_0_step_index=0\n"
             "episode_0_step_0_episode_id=episode_legacy_fixture\n"
             "episode_0_step_0_anchor_key=anchor_legacy_0000\n"
             "episode_0_step_0_observation_anchor_index=100\n"
             "episode_0_step_0_knowledge_timestamp_ms=100000\n"
             "episode_0_step_0_policy_input_digest=legacy_policy_input\n"
             "episode_0_step_0_action_distribution_id="
             "masked_dirichlet_simplex.v1\n"
             "episode_0_step_0_target_node_weights=BTC:0.60,ETH:0.40\n"
             "episode_0_step_0_reward_total=0.004\n"
             "episode_0_step_0_reward_log_growth=0.005\n"
             "episode_0_step_0_cajtucu_execution_trace_available=true\n"
             "episode_0_step_0_cajtucu_trace_valid=true\n"
             "episode_0_step_0_cajtucu_trace_id=trace_legacy_fixture_0\n"
             "episode_0_step_0_done=true\n");
  std::string ppo_legacy_replay_args = valid_policy_training_args("execute");
  const auto ppo_legacy_live_pos =
      ppo_legacy_replay_args.find(ppo_live_forbidden);
  check(ppo_legacy_live_pos != std::string::npos,
        "PPO legacy replay fixture should contain live execution denial");
  ppo_legacy_replay_args.replace(
      ppo_legacy_live_pos, ppo_live_forbidden.size(),
      "\"live_execution_allowed\":false,"
      "\"confirm_execute\":true,"
      "\"job_id\":\"ppo_legacy_replay_missing_distribution\","
      "\"report_path\":\"" +
          ppo_legacy_replay_report_path.string() + "\"}");
  result.clear();
  error.clear();
  check(!hero_runtime::execute_tool_json("hero.runtime.run",
                                         ppo_legacy_replay_args, &ppo_ctx,
                                         &result, &error),
        "PPO replay ingestion should reject missing distribution evidence");
  require_contains(
      error, "E_RUNTIME_POLICY_TRAINING_PPO_DISTRIBUTION_EVIDENCE_MISSING",
      "PPO missing distribution evidence refusal should be stable");

  const auto ppo_source_job_dir = ppo_runtime_root / "source_completed_job";
  const auto ppo_source_replay_artifact_dir =
      ppo_source_job_dir / "artifacts" / "kikijyeba.environment.replay.v1";
  std::filesystem::create_directories(ppo_source_replay_artifact_dir);
  write_text(ppo_source_job_dir / "job.manifest",
             "job_id=source_completed_job\n"
             "target_component_family_id=wikimyei.inference.expected_value."
             "mdn\n");
  write_text(ppo_source_job_dir / "job.state", "status=completed\n");
  write_text(ppo_source_replay_artifact_dir / "runtime_replay_batches.index",
             "schema=kikijyeba.environment.replay.runtime_batch_index.v1\n"
             "entry_count=1\n");
  const auto fake_ppo_replay_exec =
      ppo_runtime_root / "fake_ppo_replay_exec.sh";
  write_text(fake_ppo_replay_exec,
             "#!/bin/sh\n"
             "report_path=''\n"
             "policy_checkpoint_path=''\n"
             "policy_artifact_digest=''\n"
             "linear_cost_rate=''\n"
             "on_policy_sample='false'\n"
             "quality_replay='false'\n"
             "no_numeraire_policy='false'\n"
             "no_sdu_policy='false'\n"
             "while [ \"$#\" -gt 0 ]; do\n"
             "  if [ \"$1\" = '--replay-report-path' ]; then\n"
             "    shift\n"
             "    report_path=\"$1\"\n"
             "  elif [ \"$1\" = '--replay-policy-checkpoint-path' ]; then\n"
             "    shift\n"
             "    policy_checkpoint_path=\"$1\"\n"
             "  elif [ \"$1\" = '--replay-policy-artifact-digest' ]; then\n"
             "    shift\n"
             "    policy_artifact_digest=\"$1\"\n"
             "  elif [ \"$1\" = '--replay-linear-transaction-cost-rate' ]; "
             "then\n"
             "    shift\n"
             "    linear_cost_rate=\"$1\"\n"
             "  elif [ \"$1\" = '--replay-on-policy-sample' ]; then\n"
             "    on_policy_sample='true'\n"
             "  elif [ \"$1\" = '--replay-no-numeraire-only-policy' ]; then\n"
             "    no_numeraire_policy='true'\n"
             "  elif [ \"$1\" = '--replay-no-sdu-policy' ]; then\n"
             "    no_sdu_policy='true'\n"
             "  fi\n"
             "  shift\n"
             "done\n"
             "if [ -z \"$report_path\" ]; then\n"
             "  echo missing replay report path >&2\n"
             "  exit 2\n"
             "fi\n"
             "case \"$report_path\" in\n"
             "  *policy_quality_replay.report) quality_replay='true' ;;\n"
             "esac\n"
             "if [ \"$quality_replay\" = 'true' ]; then\n"
             "  if [ \"$no_numeraire_policy\" = 'true' ] || "
             "[ \"$no_sdu_policy\" = 'true' ]; then\n"
             "    echo quality replay unexpectedly disabled a required "
             "baseline >&2\n"
             "    exit 5\n"
             "  fi\n"
             "fi\n"
             "if [ -z \"$policy_checkpoint_path\" ] || [ ! -s "
             "\"$policy_checkpoint_path\" ]; then\n"
             "  echo missing replay policy checkpoint path >&2\n"
             "  exit 3\n"
             "fi\n"
             "if [ \"$on_policy_sample\" = 'true' ]; then\n"
             "  expected_stage='collection_policy'\n"
             "  mode='on_policy_sample'\n"
             "  runtime_run_id='runtime_on_policy_fixture'\n"
             "  environment_run_id='env_on_policy_fixture'\n"
             "  episode_id='episode_on_policy_fixture'\n"
             "  anchor0='anchor_on_policy_0000'\n"
             "  anchor1='anchor_on_policy_0001'\n"
             "  input0='on_policy_input_digest_0'\n"
             "  input1='on_policy_input_digest_1'\n"
             "  trace_prefix='trace_on_policy'\n"
             "  old_log0='-1.234'\n"
             "  old_log1='-1.111'\n"
             "  entropy0='0.88'\n"
             "  entropy1='0.83'\n"
             "  value0='0.015'\n"
             "  value1='0.010'\n"
             "  weights0='USDT:0.20,BTC:0.50,ETH:0.30'\n"
             "  weights1='USDT:0.25,BTC:0.45,ETH:0.30'\n"
             "  reward0='0.003'\n"
             "  reward1='-0.001'\n"
             "  log_growth0='0.004'\n"
             "  log_growth1='-0.0005'\n"
             "  cost0='0.00031'\n"
             "  cost1='0.00011'\n"
             "  turnover0='0.21'\n"
             "  turnover1='0.07'\n"
             "  fee0='0.00007'\n"
             "  fee1='0.00003'\n"
             "  spread0='0.00010'\n"
             "  spread1='0.00004'\n"
             "  slippage0='0.00014'\n"
             "  slippage1='0.00004'\n"
             "  final_equity='100.08'\n"
             "  total_log_growth='0.0035'\n"
             "  max_drawdown='0.010'\n"
             "  realized_volatility='0.00318'\n"
             "  target_l1='0.000'\n"
             "  target_linf='0.000'\n"
             "else\n"
             "  expected_stage='post_update_policy'\n"
             "  mode='deterministic_action'\n"
             "  runtime_run_id='runtime_validation_fixture'\n"
             "  environment_run_id='env_validation_fixture'\n"
             "  episode_id='episode_validation_fixture'\n"
             "  anchor0='anchor_validation_0000'\n"
             "  anchor1='anchor_validation_0001'\n"
             "  input0='validation_input_digest_0'\n"
             "  input1='validation_input_digest_1'\n"
             "  trace_prefix='trace_validation'\n"
             "  old_log0='-0.900'\n"
             "  old_log1='-0.905'\n"
             "  entropy0='0.55'\n"
             "  entropy1='0.54'\n"
             "  value0='0.020'\n"
             "  value1='0.018'\n"
             "  weights0='USDT:0.24,BTC:0.46,ETH:0.30'\n"
             "  weights1='USDT:0.24,BTC:0.46,ETH:0.30'\n"
             "  reward0='0.0015'\n"
             "  reward1='-0.0003'\n"
             "  log_growth0='0.0018'\n"
             "  log_growth1='-0.0006'\n"
             "  cost0='0.00024'\n"
             "  cost1='0.00006'\n"
             "  turnover0='0.09'\n"
             "  turnover1='0.02'\n"
             "  fee0='0.00006'\n"
             "  fee1='0.00002'\n"
             "  spread0='0.00008'\n"
             "  spread1='0.00002'\n"
             "  slippage0='0.00010'\n"
             "  slippage1='0.00002'\n"
             "  final_equity='100.12'\n"
             "  total_log_growth='0.0012'\n"
             "  max_drawdown='0.012'\n"
             "  realized_volatility='0.00170'\n"
             "  target_l1='0.010'\n"
             "  target_linf='0.005'\n"
             "fi\n"
             "if ! grep -q \"checkpoint_stage=$expected_stage\" "
             "\"$policy_checkpoint_path\"; then\n"
             "  echo wrong checkpoint stage for replay mode >&2\n"
             "  exit 4\n"
             "fi\n"
             "mkdir -p \"$(dirname \"$report_path\")\"\n"
             "cat > \"$report_path\" <<REPORT\n"
             "schema=kikijyeba.environment.replay.cajtucu_ready_experiment_"
             "artifact.v1\n"
             "runtime_run_id=$runtime_run_id\n"
             "environment_run_id=$environment_run_id\n"
             "mean_final_equity_numeraire=$final_equity\n"
             "mean_total_log_growth=$total_log_growth\n"
             "mean_max_drawdown=$max_drawdown\n"
             "mean_realized_volatility=$realized_volatility\n"
             "mean_target_weight_error_l1=$target_l1\n"
             "mean_target_weight_error_linf=$target_linf\n"
             "policy_artifact_digest=$policy_artifact_digest\n"
             "linear_transaction_cost_rate=$linear_cost_rate\n"
             "episode_count=1\n"
             "episode_0_policy_id=wikimyei.policy.portfolio.graph_node_"
             "allocation.v1\n"
             "episode_0_policy_kind=reinforcement_learning\n"
             "episode_0_policy_action_mode=$mode\n"
             "episode_0_step_count=2\n"
             "episode_0_step_0_step_index=0\n"
             "episode_0_step_0_episode_id=$episode_id\n"
             "episode_0_step_0_policy_action_mode=$mode\n"
             "episode_0_step_0_anchor_key=$anchor0\n"
             "episode_0_step_0_observation_anchor_index=200\n"
             "episode_0_step_0_knowledge_timestamp_ms=200000\n"
             "episode_0_step_0_accepted_anchor_index_begin=200\n"
             "episode_0_step_0_accepted_anchor_index_end=201\n"
             "episode_0_step_0_policy_input_digest=$input0\n"
             "episode_0_step_0_action_distribution_id="
             "masked_dirichlet_simplex.v1\n"
             "episode_0_step_0_active_node_indices=0,1,2\n"
             "episode_0_step_0_active_count=3\n"
             "episode_0_step_0_old_log_prob=$old_log0\n"
             "episode_0_step_0_old_entropy=$entropy0\n"
             "episode_0_step_0_old_value_estimate=$value0\n"
             "episode_0_step_0_action_distribution_evidence_bound=true\n"
             "episode_0_step_0_target_node_weights=$weights0\n"
             "episode_0_step_0_reward_total=$reward0\n"
             "episode_0_step_0_reward_log_growth=$log_growth0\n"
             "episode_0_step_0_reward_transaction_cost_penalty=-0.001\n"
             "episode_0_step_0_transaction_cost_numeraire=$cost0\n"
             "episode_0_step_0_turnover=$turnover0\n"
             "episode_0_step_0_cajtucu_execution_trace_available=true\n"
             "episode_0_step_0_cajtucu_trace_valid=true\n"
             "episode_0_step_0_cajtucu_trace_id=${trace_prefix}_0\n"
             "episode_0_step_0_cajtucu_total_fee_numeraire=$fee0\n"
             "episode_0_step_0_cajtucu_total_spread_cost_numeraire=$spread0\n"
             "episode_0_step_0_cajtucu_total_slippage_numeraire=$slippage0\n"
             "episode_0_step_0_cajtucu_missing_direct_pair_count=0\n"
             "episode_0_step_0_cajtucu_numeraire_fallback_pair_count=0\n"
             "episode_0_step_0_cajtucu_rejected_fill_count=0\n"
             "episode_0_step_0_cajtucu_partial_fill_count=0\n"
             "episode_0_step_0_invalid_action=false\n"
             "episode_0_step_0_done=false\n"
             "episode_0_step_1_step_index=1\n"
             "episode_0_step_1_episode_id=$episode_id\n"
             "episode_0_step_1_policy_action_mode=$mode\n"
             "episode_0_step_1_anchor_key=$anchor1\n"
             "episode_0_step_1_observation_anchor_index=201\n"
             "episode_0_step_1_knowledge_timestamp_ms=201000\n"
             "episode_0_step_1_accepted_anchor_index_begin=201\n"
             "episode_0_step_1_accepted_anchor_index_end=202\n"
             "episode_0_step_1_policy_input_digest=$input1\n"
             "episode_0_step_1_action_distribution_id="
             "masked_dirichlet_simplex.v1\n"
             "episode_0_step_1_active_node_indices=0,1,2\n"
             "episode_0_step_1_active_count=3\n"
             "episode_0_step_1_old_log_prob=$old_log1\n"
             "episode_0_step_1_old_entropy=$entropy1\n"
             "episode_0_step_1_old_value_estimate=$value1\n"
             "episode_0_step_1_action_distribution_evidence_bound=true\n"
             "episode_0_step_1_target_node_weights=$weights1\n"
             "episode_0_step_1_reward_total=$reward1\n"
             "episode_0_step_1_reward_log_growth=$log_growth1\n"
             "episode_0_step_1_reward_turnover_penalty=-0.0005\n"
             "episode_0_step_1_transaction_cost_numeraire=$cost1\n"
             "episode_0_step_1_turnover=$turnover1\n"
             "episode_0_step_1_cajtucu_execution_trace_available=true\n"
             "episode_0_step_1_cajtucu_trace_valid=true\n"
             "episode_0_step_1_cajtucu_trace_id=${trace_prefix}_1\n"
             "episode_0_step_1_cajtucu_total_fee_numeraire=$fee1\n"
             "episode_0_step_1_cajtucu_total_spread_cost_numeraire=$spread1\n"
             "episode_0_step_1_cajtucu_total_slippage_numeraire=$slippage1\n"
             "episode_0_step_1_cajtucu_missing_direct_pair_count=0\n"
             "episode_0_step_1_cajtucu_numeraire_fallback_pair_count=0\n"
             "episode_0_step_1_cajtucu_rejected_fill_count=0\n"
             "episode_0_step_1_cajtucu_partial_fill_count=0\n"
             "episode_0_step_1_invalid_action=false\n"
             "episode_0_step_1_done=true\n"
             "REPORT\n"
             "if [ \"$quality_replay\" = 'true' ]; then\n"
             "cat >> \"$report_path\" <<REPORT\n"
             "policy_summary_count=5\n"
             "policy_0_id=numeraire_only.v1\n"
             "policy_0_kind=baseline\n"
             "policy_0_attempted_count=1\n"
             "policy_0_completed_count=1\n"
             "policy_0_failed_count=0\n"
             "policy_0_mean_total_reward=0.0\n"
             "policy_0_mean_total_log_growth=0.0\n"
             "policy_0_mean_final_equity_numeraire=100.00\n"
             "policy_0_mean_max_drawdown=0.0\n"
             "policy_0_realized_volatility=0.0\n"
             "policy_0_mean_total_turnover=0.0\n"
             "policy_0_mean_total_transaction_cost_numeraire=0.0\n"
             "policy_0_requested_notional_numeraire=0.0\n"
             "policy_0_executed_notional_numeraire=0.0\n"
             "policy_0_rejected_notional_numeraire=0.0\n"
             "policy_0_partial_notional_numeraire=0.0\n"
             "policy_0_fill_ratio=1.0\n"
             "policy_0_fee_cost_numeraire=0.0\n"
             "policy_0_spread_cost_numeraire=0.0\n"
             "policy_0_slippage_cost_numeraire=0.0\n"
             "policy_0_mean_target_weight_error_l1=0.0\n"
             "policy_0_mean_target_weight_error_linf=0.0\n"
             "policy_0_mean_projection_mae=0.020\n"
             "policy_0_mean_projection_signed_bias=0.000\n"
             "policy_0_mean_projection_directional_accuracy=0.50\n"
             "policy_0_mean_projection_interval_coverage=0.75\n"
             "policy_0_cajtucu_valid_trace_count=2\n"
             "policy_0_cajtucu_invalid_trace_count=0\n"
             "policy_0_cajtucu_missing_direct_pair_count=0\n"
             "policy_0_cajtucu_numeraire_fallback_pair_count=0\n"
             "policy_0_rejected_order_count=0\n"
             "policy_0_partial_order_count=0\n"
             "policy_1_id=current_weight_no_trade.v1\n"
             "policy_1_kind=baseline\n"
             "policy_1_attempted_count=1\n"
             "policy_1_completed_count=1\n"
             "policy_1_failed_count=0\n"
             "policy_1_mean_total_reward=0.0004\n"
             "policy_1_mean_total_log_growth=0.0004\n"
             "policy_1_mean_final_equity_numeraire=100.04\n"
             "policy_1_mean_max_drawdown=0.010\n"
             "policy_1_realized_volatility=0.0012\n"
             "policy_1_mean_total_turnover=0.0\n"
             "policy_1_mean_total_transaction_cost_numeraire=0.0\n"
             "policy_1_requested_notional_numeraire=0.0\n"
             "policy_1_executed_notional_numeraire=0.0\n"
             "policy_1_rejected_notional_numeraire=0.0\n"
             "policy_1_partial_notional_numeraire=0.0\n"
             "policy_1_fill_ratio=1.0\n"
             "policy_1_fee_cost_numeraire=0.0\n"
             "policy_1_spread_cost_numeraire=0.0\n"
             "policy_1_slippage_cost_numeraire=0.0\n"
             "policy_1_mean_target_weight_error_l1=0.0\n"
             "policy_1_mean_target_weight_error_linf=0.0\n"
             "policy_1_mean_projection_mae=0.018\n"
             "policy_1_mean_projection_signed_bias=0.001\n"
             "policy_1_mean_projection_directional_accuracy=0.55\n"
             "policy_1_mean_projection_interval_coverage=0.78\n"
             "policy_1_cajtucu_valid_trace_count=2\n"
             "policy_1_cajtucu_invalid_trace_count=0\n"
             "policy_1_cajtucu_missing_direct_pair_count=0\n"
             "policy_1_cajtucu_numeraire_fallback_pair_count=0\n"
             "policy_1_rejected_order_count=0\n"
             "policy_1_partial_order_count=0\n"
             "policy_2_id=equal_weight.v1\n"
             "policy_2_kind=baseline\n"
             "policy_2_attempted_count=1\n"
             "policy_2_completed_count=1\n"
             "policy_2_failed_count=0\n"
             "policy_2_mean_total_reward=0.0006\n"
             "policy_2_mean_total_log_growth=0.0006\n"
             "policy_2_mean_final_equity_numeraire=100.06\n"
             "policy_2_mean_max_drawdown=0.018\n"
             "policy_2_realized_volatility=0.0019\n"
             "policy_2_mean_total_turnover=0.15\n"
             "policy_2_mean_total_transaction_cost_numeraire=0.00040\n"
             "policy_2_requested_notional_numeraire=1.10\n"
             "policy_2_executed_notional_numeraire=1.10\n"
             "policy_2_rejected_notional_numeraire=0.0\n"
             "policy_2_partial_notional_numeraire=0.0\n"
             "policy_2_fill_ratio=1.0\n"
             "policy_2_fee_cost_numeraire=0.00010\n"
             "policy_2_spread_cost_numeraire=0.00013\n"
             "policy_2_slippage_cost_numeraire=0.00017\n"
             "policy_2_mean_target_weight_error_l1=0.004\n"
             "policy_2_mean_target_weight_error_linf=0.002\n"
             "policy_2_mean_projection_mae=0.015\n"
             "policy_2_mean_projection_signed_bias=0.001\n"
             "policy_2_mean_projection_directional_accuracy=0.56\n"
             "policy_2_mean_projection_interval_coverage=0.80\n"
             "policy_2_cajtucu_valid_trace_count=2\n"
             "policy_2_cajtucu_invalid_trace_count=0\n"
             "policy_2_cajtucu_missing_direct_pair_count=0\n"
             "policy_2_cajtucu_numeraire_fallback_pair_count=0\n"
             "policy_2_rejected_order_count=0\n"
             "policy_2_partial_order_count=0\n"
             "policy_3_id=kikijyeba.environment.policy."
             "spot_distributional_utility.v1\n"
             "policy_3_kind=baseline\n"
             "policy_3_attempted_count=1\n"
             "policy_3_completed_count=1\n"
             "policy_3_failed_count=0\n"
             "policy_3_mean_total_reward=0.0010\n"
             "policy_3_mean_total_log_growth=0.0010\n"
             "policy_3_mean_final_equity_numeraire=100.10\n"
             "policy_3_mean_max_drawdown=0.013\n"
             "policy_3_realized_volatility=0.0016\n"
             "policy_3_mean_total_turnover=0.10\n"
             "policy_3_mean_total_transaction_cost_numeraire=0.00025\n"
             "policy_3_requested_notional_numeraire=0.90\n"
             "policy_3_executed_notional_numeraire=0.90\n"
             "policy_3_rejected_notional_numeraire=0.0\n"
             "policy_3_partial_notional_numeraire=0.0\n"
             "policy_3_fill_ratio=1.0\n"
             "policy_3_fee_cost_numeraire=0.00006\n"
             "policy_3_spread_cost_numeraire=0.00008\n"
             "policy_3_slippage_cost_numeraire=0.00011\n"
             "policy_3_mean_target_weight_error_l1=0.006\n"
             "policy_3_mean_target_weight_error_linf=0.003\n"
             "policy_3_mean_projection_mae=0.012\n"
             "policy_3_mean_projection_signed_bias=0.001\n"
             "policy_3_mean_projection_directional_accuracy=0.59\n"
             "policy_3_mean_projection_interval_coverage=0.82\n"
             "policy_3_cajtucu_valid_trace_count=2\n"
             "policy_3_cajtucu_invalid_trace_count=0\n"
             "policy_3_cajtucu_missing_direct_pair_count=0\n"
             "policy_3_cajtucu_numeraire_fallback_pair_count=0\n"
             "policy_3_rejected_order_count=0\n"
             "policy_3_partial_order_count=0\n"
             "policy_4_id=wikimyei.policy.portfolio.graph_node_allocation.v1\n"
             "policy_4_kind=reinforcement_learning\n"
             "policy_4_attempted_count=1\n"
             "policy_4_completed_count=1\n"
             "policy_4_failed_count=0\n"
             "policy_4_mean_total_reward=0.0012\n"
             "policy_4_mean_total_log_growth=0.0012\n"
             "policy_4_mean_final_equity_numeraire=100.12\n"
             "policy_4_mean_max_drawdown=0.012\n"
             "policy_4_realized_volatility=0.0017\n"
             "policy_4_mean_total_turnover=0.11\n"
             "policy_4_mean_total_transaction_cost_numeraire=0.00030\n"
             "policy_4_requested_notional_numeraire=1.0\n"
             "policy_4_executed_notional_numeraire=1.0\n"
             "policy_4_rejected_notional_numeraire=0.0\n"
             "policy_4_partial_notional_numeraire=0.0\n"
             "policy_4_fill_ratio=1.0\n"
             "policy_4_fee_cost_numeraire=0.00008\n"
             "policy_4_spread_cost_numeraire=0.00010\n"
             "policy_4_slippage_cost_numeraire=0.00012\n"
             "policy_4_mean_target_weight_error_l1=0.010\n"
             "policy_4_mean_target_weight_error_linf=0.005\n"
             "policy_4_mean_projection_mae=0.010\n"
             "policy_4_mean_projection_signed_bias=0.001\n"
             "policy_4_mean_projection_directional_accuracy=0.60\n"
             "policy_4_mean_projection_interval_coverage=0.80\n"
             "policy_4_cajtucu_valid_trace_count=2\n"
             "policy_4_cajtucu_invalid_trace_count=0\n"
             "policy_4_cajtucu_missing_direct_pair_count=0\n"
             "policy_4_cajtucu_numeraire_fallback_pair_count=0\n"
             "policy_4_rejected_order_count=0\n"
             "policy_4_partial_order_count=0\n"
             "REPORT\n"
             "fi\n"
             "printf 'replay_experiment_id=ppo_on_policy_fixture\\n'\n"
             "printf 'replay_completed_count=1\\n'\n"
             "printf 'replay_report_path=%s\\n' \"$report_path\"\n");
  std::filesystem::permissions(fake_ppo_replay_exec,
                               std::filesystem::perms::owner_exec |
                                   std::filesystem::perms::owner_read |
                                   std::filesystem::perms::owner_write);
  ppo_ctx.policy.values["runtime_exec_path"] = fake_ppo_replay_exec.string();
  auto ppo_parent_bundle = make_ppo_rehearsal_parent_fact_bundle();
  std::string ppo_on_policy_args = valid_policy_training_args("execute");
  replace_all(ppo_on_policy_args,
              "\"parent_forecast_eval_fact_digest\":\"forecast_eval_fact_"
              "digest\"",
              "\"parent_forecast_eval_fact_digest\":\"" +
                  ppo_parent_bundle.forecast_eval_digest + "\"");
  replace_all(ppo_on_policy_args,
              "\"parent_observer_belief_fact_digest\":\"observer_belief_fact_"
              "digest\"",
              "\"parent_observer_belief_fact_digest\":\"" +
                  ppo_parent_bundle.observer_belief_digest + "\"");
  replace_all(ppo_on_policy_args,
              "\"parent_allocation_engine_fact_digest\":\"allocation_engine_"
              "fact_digest\"",
              "\"parent_allocation_engine_fact_digest\":\"" +
                  ppo_parent_bundle.allocation_engine_digest + "\"");
  replace_all(ppo_on_policy_args,
              "\"parent_replay_environment_fact_digest\":\"replay_env_fact_"
              "digest\"",
              "\"parent_replay_environment_fact_digest\":\"" +
                  ppo_parent_bundle.replay_environment_digest + "\"");
  const auto ppo_on_policy_max_steps_pos =
      ppo_on_policy_args.find("\"max_steps\":64");
  check(ppo_on_policy_max_steps_pos != std::string::npos,
        "PPO on-policy fixture should contain default max_steps");
  ppo_on_policy_args.replace(ppo_on_policy_max_steps_pos,
                             std::string("\"max_steps\":64").size(),
                             "\"max_steps\":2");
  const auto ppo_on_policy_live_pos =
      ppo_on_policy_args.find(ppo_live_forbidden);
  check(ppo_on_policy_live_pos != std::string::npos,
        "PPO on-policy fixture should contain live execution denial");
  ppo_on_policy_args.replace(ppo_on_policy_live_pos, ppo_live_forbidden.size(),
                             "\"live_execution_allowed\":false,"
                             "\"confirm_execute\":true,"
                             "\"job_id\":\"ppo_on_policy_collect\","
                             "\"replay_job_dir\":\"" +
                                 ppo_source_job_dir.string() +
                                 "\","
                                 "\"accounting_numeraire_node_id\":\"USDT\"}");
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", ppo_on_policy_args,
                                        &ppo_ctx, &result, &error),
        "PPO on-policy replay execute failed: " + error +
            "\nresult: " + result);
  require_contains(result,
                   "\"rollout_collection_source\":\"kikijyeba_on_policy_"
                   "replay\"",
                   "PPO on-policy execute reports fresh replay collection "
                   "source");
  require_contains(result, "\"replay_backed_step_count\":2",
                   "PPO on-policy execute reports replay-backed sample count");
  require_contains(result, "\"on_policy_runtime_collection_complete\":true",
                   "PPO on-policy execute marks Runtime collection complete");
  require_contains(result, "\"policy_distribution_evidence_bound\":true",
                   "PPO on-policy execute binds policy distribution evidence");
  require_contains(result, "\"input_policy_checkpoint_path\":",
                   "PPO on-policy execute reports collection checkpoint path");
  require_contains(
      result, "\"input_policy_checkpoint_digest\":",
      "PPO on-policy execute reports collection checkpoint digest");
  const auto ppo_on_policy_manifest_path =
      std::filesystem::path(json_string_field(result, "manifest_path"));
  const auto ppo_on_policy_manifest_text =
      read_text(ppo_on_policy_manifest_path);
  require_contains(ppo_on_policy_manifest_text,
                   "protocol_status=ppo_v0_runtime_on_policy_replay_"
                   "collection",
                   "PPO on-policy manifest records fresh collection status");
  require_contains(ppo_on_policy_manifest_text,
                   "dispatch_kikijyeba_replay_collect_on_policy_report_"
                   "write_checkpoint_rollout_update",
                   "PPO on-policy manifest records fresh collection chain");
  const auto ppo_on_policy_rollout_path = std::filesystem::path(
      json_string_field(result, "rollout_collection_path"));
  const auto ppo_on_policy_rollout_text = read_text(ppo_on_policy_rollout_path);
  require_contains(ppo_on_policy_rollout_text,
                   "collection_source=kikijyeba_on_policy_replay",
                   "PPO on-policy rollout records fresh collection source");
  require_contains(ppo_on_policy_rollout_text, "input_policy_checkpoint_path=",
                   "PPO on-policy rollout records collection checkpoint path");
  require_contains(
      ppo_on_policy_rollout_text, "input_policy_checkpoint_digest=",
      "PPO on-policy rollout records collection checkpoint digest");
  require_contains(ppo_on_policy_rollout_text,
                   "on_policy_runtime_collection_complete=true",
                   "PPO on-policy rollout records completion flag");
  require_contains(ppo_on_policy_rollout_text,
                   "action_log_prob_source=action_distribution_evidence",
                   "PPO on-policy rollout uses action-distribution evidence");
  require_contains(ppo_on_policy_rollout_text,
                   "policy_distribution_evidence_bound=true",
                   "PPO on-policy rollout marks distribution evidence bound");
  require_contains(ppo_on_policy_rollout_text,
                   "step_0_source=kikijyeba_on_policy_replay",
                   "PPO on-policy rollout records per-step fresh source");
  require_contains(ppo_on_policy_rollout_text,
                   "step_0_policy_input_digest=on_policy_input_digest_0",
                   "PPO on-policy rollout preserves policy input digest");
  require_contains(ppo_on_policy_rollout_text, "step_0_old_log_prob=-1.234",
                   "PPO on-policy rollout preserves old log probability");
  require_contains(ppo_on_policy_rollout_text,
                   "step_0_cajtucu_trace_id=trace_on_policy_0",
                   "PPO on-policy rollout preserves Cajtucu trace id");
  require_contains(ppo_on_policy_rollout_text, "step_1_done=true",
                   "PPO on-policy rollout records terminal step");
  const auto ppo_on_policy_report_path =
      std::filesystem::path(json_string_field(result, "report_path"));
  const auto ppo_on_policy_report_text = read_text(ppo_on_policy_report_path);
  require_contains(
      ppo_on_policy_report_text,
      "run_data_kind=policy_training_ppo_v0_on_policy_replay_collected",
      "PPO on-policy report records fresh collection run-data kind");
  require_contains(ppo_on_policy_report_text, "input_policy_checkpoint_path=",
                   "PPO on-policy report records collection checkpoint path");
  require_contains(ppo_on_policy_report_text, "input_policy_checkpoint_digest=",
                   "PPO on-policy report records collection checkpoint digest");
  const auto ppo_on_policy_fact_path = std::filesystem::path(
      json_string_field(result, "policy_training_fact_path"));
  const auto ppo_on_policy_fact_text = read_text(ppo_on_policy_fact_path);
  require_contains(ppo_on_policy_fact_text, "input_policy_checkpoint_path=",
                   "PPO on-policy fact records collection checkpoint path");
  require_contains(ppo_on_policy_fact_text, "input_policy_checkpoint_digest=",
                   "PPO on-policy fact records collection checkpoint digest");
  const auto parsed_ppo_on_policy_facts =
      lattice_exposure::make_policy_training_facts_from_job_dir(
          ppo_on_policy_fact_path.parent_path(), ppo_parent_bundle.parent_fact);
  check(parsed_ppo_on_policy_facts.size() == 1U,
        "Lattice exposure should parse emitted on-policy PPO fact");
  const auto &parsed_ppo_on_policy_fact = parsed_ppo_on_policy_facts.front();
  check(!parsed_ppo_on_policy_fact.input_policy_checkpoint_digest.empty(),
        "parsed on-policy PPO fact carries input policy checkpoint digest");
  check(parsed_ppo_on_policy_fact.parent_forecast_eval_fact_digest ==
            ppo_parent_bundle.forecast_eval_digest,
        "parsed on-policy PPO fact binds concrete forecast-eval parent digest");
  check(parsed_ppo_on_policy_fact.parent_observer_belief_fact_digest ==
            ppo_parent_bundle.observer_belief_digest,
        "parsed on-policy PPO fact binds concrete observer-belief parent "
        "digest");
  check(parsed_ppo_on_policy_fact.parent_allocation_engine_fact_digest.empty(),
        "parsed on-policy PPO fact does not require legacy allocation-engine "
        "parent lineage");
  check(parsed_ppo_on_policy_fact.parent_replay_environment_fact_digest.empty(),
        "parsed on-policy PPO fact does not depend on a pre-derived replay "
        "environment fact digest");
  const auto ppo_policy_quality_replay_path = std::filesystem::path(
      json_string_field(result, "policy_quality_replay_report_path"));
  const auto ppo_policy_quality_replay_digest =
      lattice_exposure::replay_environment_report_digest_for_text(
          read_text(ppo_policy_quality_replay_path));
  check(parsed_ppo_on_policy_fact.parent_replay_environment_report_digest ==
            ppo_policy_quality_replay_digest,
        "parsed on-policy PPO fact binds post-derived replay report lineage "
        "digest");
  const auto ppo_on_policy_fact_issues =
      lattice_exposure::policy_training_fact_issues(parsed_ppo_on_policy_fact);
  check(ppo_on_policy_fact_issues.empty(),
        "parsed on-policy PPO fact is Lattice-ready artifact evidence: " +
            join_issue_tokens(ppo_on_policy_fact_issues));
  auto generated_policy_quality_replay_parent =
      ppo_parent_bundle.ledger.replay_environment_facts().front();
  generated_policy_quality_replay_parent.experiment_report_path =
      ppo_policy_quality_replay_path;
  generated_policy_quality_replay_parent.experiment_report_digest =
      ppo_policy_quality_replay_digest;
  ppo_parent_bundle.ledger.add_replay_environment(
      std::move(generated_policy_quality_replay_parent));
  ppo_parent_bundle.ledger.add_policy_training(parsed_ppo_on_policy_fact);
  const auto ppo_generated_specs =
      lattice_target::decode_lattice_targets_from_dsl(R"DSL(
LATTICE_TARGET {
  TARGET_ID = generated_ppo_v0_policy_training_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  PROOF_KIND = policy_training_artifact_bound;
  SUBJECT_FACT_FAMILY = policy_training;
  SUBJECT_COMPONENT = wikimyei.policy.trainable;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  ANCHOR_INDEX_BEGIN = 0;
  ANCHOR_INDEX_END = 10;
  REQUIRE_CONTRACT_MATCH = true;
};
)DSL");
  lattice_target::lattice_target_evaluator_options_t ppo_artifact_options{};
  ppo_artifact_options.exposure_ledger = &ppo_parent_bundle.ledger;
  ppo_artifact_options.auto_build_exposure_ledger = false;
  ppo_artifact_options.active_identity.protocol_id = "cwu_02v";
  ppo_artifact_options.active_identity.protocol_contract_fingerprint =
      "contract_1";
  ppo_artifact_options.active_identity.graph_order_fingerprint = "graph_1";
  ppo_artifact_options.active_identity.source_cursor_token = "cursor_1";
  lattice_target::lattice_target_evaluator_t ppo_artifact_evaluator(
      ppo_generated_specs, ppo_artifact_options);
  const auto ppo_artifact_eval = ppo_artifact_evaluator.evaluate(
      "generated_ppo_v0_policy_training_artifact_ready");
  check(ppo_artifact_eval.status ==
            lattice_target::lattice_target_status_t::satisfied,
        "generated on-policy PPO fact should satisfy "
        "policy_training_artifact_ready with concrete parent evidence: " +
            join_issue_tokens(ppo_artifact_eval.reasons));
  check(ppo_artifact_eval.proof_kind == "policy_training_artifact_bound",
        "generated PPO Lattice target uses policy-training artifact proof");
  check(ppo_artifact_eval.subject_fact_family == "policy_training",
        "generated PPO Lattice target remains scoped to policy_training facts");
  check(!ppo_artifact_eval.plan_ready &&
            ppo_artifact_eval.suggested_wave.empty(),
        "generated PPO artifact readiness does not suggest training or "
        "selection work");
  check(ppo_artifact_eval.fact_integrity_summary_available,
        "generated PPO artifact readiness exposes fact-integrity summary");
  check(ppo_artifact_eval.fact_integrity_summary.relation_declared_count == 3,
        "generated PPO artifact readiness declares three parent evidence "
        "relations");
  check(ppo_artifact_eval.fact_integrity_summary.relation_bound_count == 3,
        "generated PPO artifact readiness binds all three parent evidence "
        "relations");
  check(ppo_artifact_eval.fact_integrity_summary.relation_integrity_clean,
        "generated PPO artifact readiness has clean parent relation "
        "integrity");
  check(ppo_artifact_eval.proof_certificate.artifacts.size() == 1,
        "generated PPO artifact readiness emits one proof artifact");
  check(ppo_artifact_eval.proof_certificate.artifacts.front().passed,
        "generated PPO proof artifact passes");
  check(ppo_artifact_eval.proof_certificate.artifacts.front().lineage_bound,
        "generated PPO proof artifact binds parent lineage");
  check(ppo_artifact_eval.proof_certificate.artifacts.front()
            .proof_template_bound,
        "generated PPO proof artifact binds policy-training proof template");
  check(ppo_artifact_eval.proof_certificate_check.passed,
        "generated PPO proof certificate check passes");
  const auto &ppo_artifact_proof =
      ppo_artifact_eval.proof_certificate.artifacts.front();
  check(ppo_artifact_proof.authority_clean &&
            !ppo_artifact_proof.readiness_authority &&
            !ppo_artifact_proof.quality_authority &&
            !ppo_artifact_proof.performance_authority &&
            !ppo_artifact_proof.checkpoint_selector &&
            !ppo_artifact_proof.allocation_authority &&
            !ppo_artifact_proof.execution_authority &&
            !ppo_artifact_proof.market_readiness_authority &&
            !ppo_artifact_proof.deployment_authority,
        "generated PPO Lattice artifact proof remains evidence-only and "
        "does not claim policy quality, selection, execution, or market "
        "readiness authority");
  const auto ppo_on_policy_collection_checkpoint_path = std::filesystem::path(
      json_string_field(result, "input_policy_checkpoint_path"));
  const auto ppo_on_policy_collection_checkpoint_text =
      read_text(ppo_on_policy_collection_checkpoint_path);
  require_contains(ppo_on_policy_collection_checkpoint_text,
                   "checkpoint_stage=collection_policy",
                   "PPO on-policy collection checkpoint records stage");
  require_contains(ppo_on_policy_collection_checkpoint_text,
                   "forward_mode=graph_node_allocation_torch_policy_module_v0."
                   "v1",
                   "PPO on-policy collection checkpoint records forward mode");
  require_contains(ppo_on_policy_collection_checkpoint_text,
                   "module_contract_id=wikimyei.policy.portfolio."
                   "graph_node_allocation.torch_policy_module.v0",
                   "PPO on-policy collection checkpoint records module "
                   "contract");
  require_contains(ppo_on_policy_collection_checkpoint_text,
                   "node_weight_logit_bias_mode=zero",
                   "PPO on-policy collection checkpoint records logit bias "
                   "mode");
  require_contains(ppo_on_policy_collection_checkpoint_text,
                   "ppo_update_bound=false",
                   "PPO on-policy collection checkpoint is pre-update");
  const auto ppo_on_policy_update_path = std::filesystem::path(
      json_string_field(result, "ppo_update_report_path"));
  const auto ppo_on_policy_update_text = read_text(ppo_on_policy_update_path);
  require_contains(ppo_on_policy_update_text, "update_sample_count=2",
                   "PPO on-policy update report records sample count");
  require_contains(ppo_on_policy_update_text,
                   "sample_0_policy_input_digest=on_policy_input_digest_0",
                   "PPO on-policy update preserves policy input digest");
  require_contains(ppo_on_policy_update_text, "sample_0_old_log_prob=-1.234",
                   "PPO on-policy update preserves old log probability");
  require_contains(ppo_on_policy_update_text, "sample_0_return_target=",
                   "PPO on-policy update records return target");
  require_contains(ppo_on_policy_update_text, "sample_0_policy_loss_final=",
                   "PPO on-policy update records clipped policy loss");
  const auto ppo_on_policy_actor_checkpoint_path =
      std::filesystem::path(json_string_field(result, "actor_checkpoint_path"));
  const auto ppo_on_policy_actor_checkpoint_text =
      read_text(ppo_on_policy_actor_checkpoint_path);
  require_contains(ppo_on_policy_actor_checkpoint_text,
                   "checkpoint_stage=post_update_policy",
                   "PPO on-policy actor checkpoint records post-update stage");
  require_contains(ppo_on_policy_actor_checkpoint_text, "ppo_update_bound=true",
                   "PPO on-policy actor checkpoint binds update evidence");
  require_contains(ppo_on_policy_actor_checkpoint_text,
                   "node_weight_logit_bias_mode=learned_bounded_ppo_v0",
                   "PPO on-policy actor checkpoint records learned logit "
                   "bias");
  require_contains(ppo_on_policy_actor_checkpoint_text,
                   "parameter_update_mode=bounded_ppo_v0_module_head_delta_"
                   "update",
                   "PPO on-policy actor checkpoint records update mode");
  require_contains(ppo_on_policy_actor_checkpoint_text,
                   "parameter_update_applied=true",
                   "PPO on-policy actor checkpoint records applied update");
  require_contains(ppo_on_policy_actor_checkpoint_text,
                   "parent_actor_checkpoint_digest=",
                   "PPO on-policy actor checkpoint records collection parent");
  const auto ppo_on_policy_critic_checkpoint_path = std::filesystem::path(
      json_string_field(result, "critic_checkpoint_path"));
  const auto ppo_on_policy_critic_checkpoint_text =
      read_text(ppo_on_policy_critic_checkpoint_path);
  require_contains(ppo_on_policy_critic_checkpoint_text,
                   "checkpoint_stage=post_update_value",
                   "PPO on-policy critic checkpoint records post-update stage");
  require_contains(ppo_on_policy_critic_checkpoint_text,
                   "parameter_update_mode=bounded_ppo_v0_value_update",
                   "PPO on-policy critic checkpoint records value update");
  require_contains(ppo_on_policy_critic_checkpoint_text,
                   "ppo_update_bound=true",
                   "PPO on-policy critic checkpoint binds update evidence");
  require_contains(ppo_on_policy_critic_checkpoint_text, "value_estimate=",
                   "PPO on-policy critic checkpoint records value estimate");
  const auto ppo_on_policy_optimizer_state_path =
      std::filesystem::path(json_string_field(result, "optimizer_state_path"));
  const auto ppo_on_policy_optimizer_state_text =
      read_text(ppo_on_policy_optimizer_state_path);
  require_contains(ppo_on_policy_optimizer_state_text,
                   "state_kind=bounded_ppo_v0_update_state",
                   "PPO on-policy optimizer state records update-state kind");
  require_contains(ppo_on_policy_optimizer_state_text,
                   "parameter_update_applied=true",
                   "PPO on-policy optimizer state records applied update");
  require_contains(ppo_on_policy_optimizer_state_text, "learning_rate_actor=",
                   "PPO on-policy optimizer state records actor learning rate");
  const auto ppo_on_policy_validation_rollout_path = std::filesystem::path(
      json_string_field(result, "validation_rollout_report_path"));
  const auto ppo_on_policy_validation_rollout_text =
      read_text(ppo_on_policy_validation_rollout_path);
  require_contains(ppo_on_policy_validation_rollout_text,
                   "schema=kikijyeba.runtime.ppo_cost_aware_validation_"
                   "rollout.v1",
                   "PPO on-policy validation rollout artifact records cost-"
                   "aware schema");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "validation_rollout_ready=true",
                   "PPO on-policy validation rollout marks evaluation ready");
  require_contains(
      ppo_on_policy_validation_rollout_text,
      "reason=post_update_deterministic_cost_aware_replay_complete",
      "PPO on-policy validation rollout records deterministic cost-aware "
      "completion reason");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "action_selection_mode=deterministic_action",
                   "PPO validation rollout uses deterministic action mode");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "post_update_actor_checkpoint_used=true",
                   "PPO validation rollout uses the post-update checkpoint");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "collection_source=kikijyeba_cost_aware_validation_replay",
                   "PPO validation rollout records validation replay source");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "raw_validation_replay_report_path=",
                   "PPO validation rollout records raw replay report path");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "raw_validation_replay_report_digest=",
                   "PPO validation rollout records raw replay report digest");
  require_contains(ppo_on_policy_validation_rollout_text, "sample_count=2",
                   "PPO validation rollout records sample count");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "replay_backed_step_count=2",
                   "PPO validation rollout records replay-backed count");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "final_equity_numeraire=100.12",
                   "PPO validation rollout records final equity");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "total_log_growth=0.0012",
                   "PPO validation rollout records total log growth");
  require_contains(ppo_on_policy_validation_rollout_text, "max_drawdown=0.012",
                   "PPO validation rollout records max drawdown");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "transaction_cost_numeraire=0.000",
                   "PPO validation rollout records transaction cost");
  require_contains(ppo_on_policy_validation_rollout_text, "fee_cost_numeraire=",
                   "PPO validation rollout records fee cost");
  require_contains(
      ppo_on_policy_validation_rollout_text,
      "spread_cost_numeraire=", "PPO validation rollout records spread cost");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "slippage_cost_numeraire=",
                   "PPO validation rollout records slippage cost");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "missing_direct_pair_count=0",
                   "PPO validation rollout records missing-pair count");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "numeraire_fallback_pair_count=0",
                   "PPO validation rollout records numeraire fallback count");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "target_tracking_error_l1=0.01",
                   "PPO validation rollout records target tracking error");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "step_0_policy_input_digest=validation_input_digest_0",
                   "PPO validation rollout preserves validation policy input");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "step_0_cajtucu_trace_id=trace_validation_0",
                   "PPO validation rollout records validation Cajtucu trace");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "lattice_quality_claim=false",
                   "PPO on-policy validation rollout denies quality claim");
  require_contains(ppo_on_policy_validation_rollout_text,
                   "market_readiness_claimed=false",
                   "PPO on-policy validation rollout denies market readiness");
  require_contains(result, "\"validation_rollout_ready\":true",
                   "PPO on-policy result marks validation rollout ready");
  require_contains(result, "\"cost_aware_validation_rollout\":true",
                   "PPO on-policy result marks cost-aware validation complete");
  require_contains(result, "\"validation_rollout_report_digest\":",
                   "PPO on-policy result reports validation rollout digest");
  require_contains(result, "\"policy_quality_report_digest\":",
                   "PPO on-policy result reports policy-quality digest");
  require_contains(result, "\"policy_quality_report_ready\":true",
                   "PPO on-policy result marks policy-quality report ready");
  require_contains(result, "\"inspect_policy_quality_report\"",
                   "PPO on-policy result suggests quality-report inspection");
  require_contains(ppo_on_policy_fact_text, "policy_quality_report_digest=",
                   "PPO on-policy fact records quality-report digest");
  check(!parsed_ppo_on_policy_fact.policy_quality_report_digest.empty(),
        "parsed on-policy PPO fact carries policy-quality report digest");
  const auto ppo_policy_quality_report_path = std::filesystem::path(
      json_string_field(result, "policy_quality_report_path"));
  const auto ppo_policy_quality_report_text =
      read_text(ppo_policy_quality_report_path);
  require_contains(ppo_policy_quality_report_text,
                   "schema=kikijyeba.runtime.policy_quality_report.v1",
                   "PPO policy-quality report records schema");
  require_contains(ppo_policy_quality_report_text,
                   "policy_quality_report_ready=true",
                   "PPO policy-quality report marks comparison evidence ready");
  require_contains(
      ppo_policy_quality_report_text,
      "reason=post_update_cost_aware_policy_quality_replay_complete",
      "PPO policy-quality report records comparison replay completion");
  require_contains(ppo_policy_quality_report_text,
                   "ppo_policy_id=wikimyei.policy.portfolio.graph_node_"
                   "allocation.v1",
                   "PPO policy-quality report identifies PPO policy");
  require_contains(ppo_policy_quality_report_text, "baseline_required_count=4",
                   "PPO policy-quality report requires four baselines");
  require_contains(ppo_policy_quality_report_text, "baseline_bound_count=4",
                   "PPO policy-quality report binds four baselines");
  require_contains(ppo_policy_quality_report_text,
                   "comparison_policy_set_complete=true",
                   "PPO policy-quality report binds complete comparison set");
  require_contains(ppo_policy_quality_report_text,
                   "same_replay_source_job=true",
                   "PPO policy-quality report records replay-source parity");
  require_contains(ppo_policy_quality_report_text,
                   "same_execution_profile_digest=true",
                   "PPO policy-quality report records execution-profile "
                   "parity");
  require_contains(ppo_policy_quality_report_text,
                   "same_transaction_cost_profile=true",
                   "PPO policy-quality report records transaction-cost parity");
  require_contains(ppo_policy_quality_report_text, "same_action_universe=true",
                   "PPO policy-quality report records action-universe parity");
  require_contains(ppo_policy_quality_report_text,
                   "policy_0_id=numeraire_only.v1",
                   "PPO policy-quality report includes numeraire baseline");
  require_contains(ppo_policy_quality_report_text,
                   "policy_1_id=current_weight_no_trade.v1",
                   "PPO policy-quality report includes no-trade baseline");
  require_contains(ppo_policy_quality_report_text,
                   "policy_2_id=equal_weight.v1",
                   "PPO policy-quality report includes equal-weight baseline");
  require_contains(ppo_policy_quality_report_text,
                   "policy_3_id=kikijyeba.environment.policy."
                   "spot_distributional_utility.v1",
                   "PPO policy-quality report includes SDU baseline");
  require_contains(ppo_policy_quality_report_text,
                   "policy_4_id=wikimyei.policy.portfolio.graph_node_"
                   "allocation.v1",
                   "PPO policy-quality report includes graph allocation PPO");
  require_contains(
      ppo_policy_quality_report_text,
      "comparison_0_ppo_minus_baseline_total_log_growth=0.0012",
      "PPO policy-quality report records PPO-vs-numeraire log-growth delta");
  require_contains(ppo_policy_quality_report_text, "policy_selected=false",
                   "PPO policy-quality report denies policy selection");
  require_contains(ppo_policy_quality_report_text, "comparison_ranked=false",
                   "PPO policy-quality report denies ranking");
  require_contains(ppo_policy_quality_report_text, "winner_declared=false",
                   "PPO policy-quality report denies winner declaration");
  require_contains(ppo_policy_quality_report_text,
                   "policy_quality_claimed=false",
                   "PPO policy-quality report denies quality claim");
  require_contains(ppo_policy_quality_report_text,
                   "lattice_policy_selector=false",
                   "PPO policy-quality report denies Lattice selection");
  require_contains(ppo_policy_quality_report_text, "report_integrity_only=true",
                   "PPO policy-quality report is integrity evidence only");
  std::filesystem::remove_all(ppo_runtime_root);

  const auto runtime_root = make_tmp_dir("policy_training_noop_execute");
  hero_runtime::runtime_context_t exec_ctx{};
  exec_ctx.global_config_path = runtime_root / ".config";
  exec_ctx.policy.policy_path = runtime_root / "hero.runtime.dsl";
  exec_ctx.policy.global_config_path = exec_ctx.global_config_path;
  exec_ctx.policy.values["runtime_root"] = runtime_root.string();
  exec_ctx.policy.values["allowed_job_roots"] = runtime_root.string();
  exec_ctx.policy.values["allow_execute"] = "true";
  exec_ctx.policy.values["allow_train_execute"] = "true";
  exec_ctx.policy.values["require_confirm_execute"] = "true";
  exec_ctx.policy.values["max_capture_bytes"] = "65536";
  std::string noop_execute_args =
      valid_policy_training_args("execute", "noop_policy_training.v1",
                                 "wikimyei.policy.trainable.noop_pre_ppo.v1");
  const std::string live_forbidden = "\"live_execution_allowed\":false}";
  const auto live_forbidden_pos = noop_execute_args.find(live_forbidden);
  check(live_forbidden_pos != std::string::npos,
        "test fixture should contain live execution denial");
  noop_execute_args.replace(live_forbidden_pos, live_forbidden.size(),
                            "\"live_execution_allowed\":false,"
                            "\"confirm_execute\":true}");
  result.clear();
  error.clear();
  check(hero_runtime::execute_tool_json("hero.runtime.run", noop_execute_args,
                                        &exec_ctx, &result, &error),
        "noop policy-training execute failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "noop policy-training execute returned error: " + result);
  require_contains(result,
                   "\"schema_version\":\"kikijyeba.runtime.policy_training_"
                   "execution_packet.v1\"",
                   "noop execute reports execution packet schema");
  require_contains(result, "\"trainer_kind\":\"noop_policy_training.v1\"",
                   "noop execute binds trainer kind");
  require_contains(result, "\"ppo_implemented\":false",
                   "noop execute does not implement PPO");
  require_contains(result, "\"policy_training_fact_path\":",
                   "noop execute writes policy-training fact");
  require_contains(result, "\"checkpoint_digest\":",
                   "noop execute reports checkpoint digest");
  require_contains(result, "\"runtime_executes_policy_training_smoke\":true",
                   "noop execute declares bounded smoke execution");
  require_contains(result, "\"runtime_trains_ppo\":false",
                   "noop execute denies PPO training authority");

  std::string inspect_result;
  std::string inspect_error;
  check(hero_runtime::execute_tool_json(
            "hero.runtime.inspect",
            "{\"subject\":\"jobs\",\"root\":\"" + runtime_root.string() +
                "\",\"include_artifacts\":true}",
            &exec_ctx, &inspect_result, &inspect_error),
        "noop policy-training job inspection failed: " + inspect_error);
  require_contains(inspect_result, "\"job_kind\":\"policy_training\"",
                   "noop execute creates an inspectable Runtime job");
  bool policy_fact_found = false;
  for (const auto &entry :
       std::filesystem::recursive_directory_iterator(runtime_root)) {
    if (entry.path().filename() == "runtime.policy_training.fact") {
      policy_fact_found = true;
      break;
    }
  }
  check(policy_fact_found,
        "noop execute should write runtime.policy_training.fact");

  const auto policy_training_scan =
      lattice_exposure::scan_exposure_ledger_from_runtime_root(runtime_root);
  check(policy_training_scan.ledger.policy_training_facts().size() == 1,
        "Lattice exposure scan derives one policy-training fact from noop "
        "Runtime job");
  const auto &policy_training_fact =
      policy_training_scan.ledger.policy_training_facts().front();
  check(lattice_exposure::policy_training_fact_issues(policy_training_fact)
            .empty(),
        "noop Runtime policy-training fact is issue-clean for the declared "
        "pre-PPO contract");
  check(
      policy_training_fact.policy_input_schema_id ==
              "kikijyeba.environment.policy_input.v1" &&
          policy_training_fact.target_component_family_id ==
              "wikimyei.policy.trainable" &&
          policy_training_fact.action_schema_digest ==
              "kikijyeba.environment.action.target_node_weights.v1" &&
          policy_training_fact.action_adapter_id ==
              "target_node_weights_simplex.v1" &&
          policy_training_fact.action_distribution_id ==
              "masked_dirichlet_simplex.v1" &&
          policy_training_fact.reward_contract_id ==
              "kikijyeba.environment.reward."
              "post_execution_ledger_log_growth_cost_drawdown.v1" &&
          policy_training_fact.training_schedule_mode ==
              "causal_walk_forward_training.v1" &&
          policy_training_fact.causal_schedule_no_future_snapshot_use_source ==
              "derived_from_artifact_fit_use_ledgers",
      "noop Runtime policy-training fact binds policy input, unified action, "
      "post-execution reward, and derived causal schedule identity");
  std::filesystem::remove_all(runtime_root);
}

} // namespace

int main() {
  try {
    test_canonical_mdn_previews_channel_job();
    test_cwu02_mdn_preview_uses_mtf_representation_chain();
    test_canonical_vicreg_previews_channel_job();
    test_canonical_mtf_previews_separate_representation_job();
    test_mismatched_protocol_representation_preview_warns();
    test_wave_profile_protocol_preview_warns();
    test_train_wave_explicit_sequential_source_order_warns();
    test_runtime_policy_profiles();
    test_wave_settings_train_defaults_to_random_source_order();
    test_multi_wave_catalog_selection();
    test_source_key_wave_preview_reports_key_bounds();
    test_mdn_wave_preview_reads_jkimyei_model_state_inputs();
    test_policy_training_causal_schedule_contract();
    test_policy_training_contract_and_pre_ppo_execute();
    test_execute_expected_wave_binding();
    test_handoff_checkpoint_inputs_launch_without_static_config_inputs();
    test_execute_wave_overlay();
    test_replay_operator_tool();
    std::cout << "hero runtime wave preview tests passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "test failed: " << ex.what() << "\n";
    return 1;
  }
}
