#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/engine/portfolio/spot_distributional_utility/assembly.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/decision_step.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/solver.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/utility/base_reserve_fallback.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/utility/belief_reporter.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/utility/cvar_baseline.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/utility/mean_variance_baseline.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/utility/portfolio_ledger.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/utility/risk_gate.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/utility/risk_parity_fallback.h"
#include "wikimyei/engine/portfolio/spot_distributional_utility/utility/spot_rebalance_router.h"
#include "wikimyei/inference/expected_value/mdn/assembly.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network_types.h"
#include "wikimyei/observer/belief/assembly.h"
#include "wikimyei/observer/belief/builder.h"
#include "wikimyei/observer/belief/health_update.h"
#include "wikimyei/observer/belief/types.h"
#include "wikimyei/observer/utility.h"

namespace {

namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
namespace assembly = cuwacunu::wikimyei::assembly;
namespace observer = cuwacunu::wikimyei::observer;
namespace belief = cuwacunu::wikimyei::observer::belief;
namespace portfolio = cuwacunu::wikimyei::engine::portfolio;
namespace sdu =
    cuwacunu::wikimyei::engine::portfolio::spot_distributional_utility;
namespace execution = sdu::execution;
namespace accounting = sdu::accounting;
namespace monitoring = sdu::monitoring;
namespace risk = sdu::risk;
namespace baseline = sdu::baseline;
namespace fallback = sdu::fallback;
namespace decision_step = cuwacunu::wikimyei::engine::portfolio::
    spot_distributional_utility::decision_step;

void check(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void close(double actual, double expected, double tol, const char *message) {
  if (std::abs(actual - expected) > tol) {
    throw std::runtime_error(std::string(message) +
                             " actual=" + std::to_string(actual) +
                             " expected=" + std::to_string(expected));
  }
}

mdn::MdnOut make_fixture_mdn() {
  const int64_t B = 1;
  const int64_t N = 3;
  const int64_t C = 3;
  const int64_t Df = 9;
  const int64_t K = 3;
  auto opts = torch::TensorOptions().dtype(torch::kFloat64);
  mdn::MdnOut out{};
  out.log_pi = torch::log(torch::full({B, N, C, Df, K}, 1.0 / K, opts));
  out.mu = torch::zeros({B, N, C, Df, K}, opts);
  out.sigma = torch::full({B, N, C, Df, K}, 0.10, opts);
  for (int64_t n = 0; n < N; ++n) {
    for (int64_t c = 0; c < C; ++c) {
      out.mu.index_put_({0, n, c, 3, 0}, 0.01 * (n + 1) + 0.005 * c);
      out.mu.index_put_({0, n, c, 3, 1}, 0.02 * (n + 1) + 0.005 * c);
      out.mu.index_put_({0, n, c, 3, 2}, 0.03 * (n + 1) + 0.005 * c);
      if (n == 2) {
        for (int64_t k = 0; k < K; ++k) {
          out.mu.index_put_({0, n, c, 3, k}, 0.0);
          out.sigma.index_put_({0, n, c, 3, k}, 0.02);
        }
      }
      out.mu.index_put_({0, n, c, 1, 0}, 0.05);
      out.mu.index_put_({0, n, c, 2, 0}, -0.04);
      out.mu.index_put_({0, n, c, 4, 0}, 1.0 + 0.1 * n);
      out.mu.index_put_({0, n, c, 5, 0}, 1.2 + 0.1 * n);
      out.mu.index_put_({0, n, c, 6, 0}, 0.8 + 0.1 * n);
      out.mu.index_put_({0, n, c, 7, 0}, 0.3);
      out.mu.index_put_({0, n, c, 8, 0}, 0.2);
    }
  }
  return out;
}

void test_feature_semantics_detection() {
  auto mdn_assembly = mdn::make_channel_context_mdn_assembly();
  auto distribution_dock =
      assembly::find_dock(mdn_assembly, assembly::dock_direction_t::produces,
                          assembly::dock_domain_t::future_node_distribution);
  check(distribution_dock.has_value(), "MDN distribution dock exists");

  auto belief_assembly = belief::make_nodelift_allocation_belief_assembly();
  auto belief_input_dock =
      assembly::find_dock(belief_assembly, assembly::dock_direction_t::consumes,
                          assembly::dock_domain_t::future_node_distribution);
  check(belief_input_dock.has_value(), "observer belief input dock exists");
  assembly::require_compatible_docks(
      mdn_assembly, assembly::dock_domain_t::future_node_distribution,
      belief_assembly, assembly::dock_domain_t::future_node_distribution,
      "mdn_to_observer_belief");
  check(belief_assembly.trainability ==
            assembly::assembly_trainability_t::deterministic,
        "observer belief assembly is deterministic");

  auto portfolio_assembly = portfolio::spot_distributional_utility::
      make_spot_distributional_utility_assembly();
  assembly::require_compatible_docks(
      belief_assembly, assembly::dock_domain_t::allocation_belief,
      portfolio_assembly, assembly::dock_domain_t::allocation_belief,
      "observer_belief_to_portfolio_engine");
  check(portfolio_assembly.family ==
            "wikimyei.engine.portfolio.spot_distributional_utility",
        "portfolio method assembly family is method-scoped");

  auto detected =
      observer::require_feature_semantics_from_dock(*distribution_dock);
  check(detected.semantics_id ==
            std::string(observer::kKlineFeatureSemanticsId),
        "dock detects kline feature semantics");
  check(detected.width == 9, "kline feature semantics width");

  observer::nodelift_potential_surface_options_t surface_options{};
  observer::range_risk_options_t range_options{};
  observer::flow_liquidity_options_t liquidity_options{};
  check(surface_options.close_coord ==
            observer::kline_coord(observer::registry::kline_feature_e::close),
        "potential surface close coord derives from registry");
  check(range_options.high_coord ==
            observer::kline_coord(observer::registry::kline_feature_e::high),
        "range risk high coord derives from registry");
  check(range_options.low_coord ==
            observer::kline_coord(observer::registry::kline_feature_e::low),
        "range risk low coord derives from registry");
  check(liquidity_options.quote_volume_coord ==
            observer::kline_coord(
                observer::registry::kline_feature_e::quote_volume),
        "liquidity quote volume coord derives from registry");
}

void test_moments_and_channel_consensus() {
  auto out = make_fixture_mdn();
  check(observer::check_mdn_output(out).valid, "MDN data quality valid");

  auto m = observer::compute_mdn_moments(out);
  check(m.mean.sizes() == torch::IntArrayRef({1, 3, 3, 9}),
        "MDN moments mean shape");
  close(m.mean.index({0, 0, 0, 3}).item<double>(), 0.02, 1e-12,
        "MDN close mean");
  check((m.variance >= 0.0).all().item<bool>(), "MDN variance nonnegative");

  auto e = observer::compute_mixture_entropy(out);
  check(e.normalized_entropy.sizes() == torch::IntArrayRef({1, 3, 3, 9}),
        "mixture entropy shape");
  check((e.dominance <= 1.0).all().item<bool>(), "mixture dominance bounded");

  auto mask = torch::tensor(
      {{{true, false, true}, {true, true, true}, {true, true, true}}},
      torch::TensorOptions().dtype(torch::kBool));
  auto cc = observer::compute_uniform_valid_channel_consensus(out, mask);
  check(cc.log_weight.sizes() == torch::IntArrayRef({1, 3, 9, 9}),
        "channel consensus flattened shape");
  close(cc.active_channel_count.index({0, 0}).item<double>(), 2.0, 1e-12,
        "mask-aware active channel count");
  close(cc.log_weight.index({0, 0, 3}).exp().sum().item<double>(), 1.0, 1e-10,
        "collapsed mixture weights normalize");
  check(cc.channel_disagreement.index({0, 0, 3}).item<double>() > 0.0,
        "channel disagreement captured");
}

void test_auxiliary_observers() {
  auto out = make_fixture_mdn();
  auto mask = torch::ones({1, 3, 3}, torch::kBool);
  auto cc = observer::compute_uniform_valid_channel_consensus(out, mask);

  auto rr =
      observer::compute_range_risk(cc, std::vector<int64_t>{0, 1},
                                   /*projection_reference_graph_index=*/2);
  check(rr.adverse_excursion_prob.sizes() == torch::IntArrayRef({1, 2}),
        "range risk adverse shape");
  check((rr.adverse_excursion_prob >= 0.0).all().item<bool>() &&
            (rr.adverse_excursion_prob <= 1.0).all().item<bool>(),
        "range risk adverse probability bounded");

  auto fl = observer::compute_flow_liquidity(cc);
  check(fl.liquidity_score.sizes() == torch::IntArrayRef({1, 3}),
        "flow liquidity shape");
  check((fl.capacity_weight_limit >= 0.0).all().item<bool>(),
        "flow liquidity capacity nonnegative");

  auto base_scenarios =
      torch::tensor({{-0.03, -0.01}, {-0.02, 0.0}, {0.01, 0.02}, {0.02, 0.03}},
                    torch::TensorOptions().dtype(torch::kFloat64));
  auto bank = observer::make_base_bank(base_scenarios);
  check(bank.base_scenarios.sizes() == torch::IntArrayRef({4, 2}),
        "scenario bank base shape");
  auto shifted = observer::left_tail_shift(bank.base_scenarios, 0.01);
  check((shifted <= bank.base_scenarios).all().item<bool>(),
        "scenario bank left shift");
  auto inflated = observer::inflate_volatility(bank.base_scenarios, 1.5);
  check(inflated.sizes() == bank.base_scenarios.sizes(),
        "scenario bank volatility inflation shape");
  auto stress_input =
      torch::tensor({{-0.04, 0.02, -0.01},
                     {0.03, -0.02, 0.00},
                     {-0.02, -0.01, 0.04},
                     {0.01, 0.03, -0.03},
                     {0.00, -0.04, 0.02},
                     {0.02, 0.01, -0.02}},
                    torch::TensorOptions().dtype(torch::kFloat64));
  observer::scenario_bank_options_t stress_options{};
  stress_options.high_correlation_loading = 1.0;
  stress_options.left_tail_shift = 0.005;
  auto stress_bank = observer::make_stress_bank(stress_input, stress_options);
  check(stress_bank.high_correlation_scenarios.sizes() == stress_input.sizes(),
        "scenario bank high-correlation stress shape");
  check(stress_bank.volatility_inflated_scenarios.sizes() ==
            stress_input.sizes(),
        "scenario bank volatility stress shape");
  check(stress_bank.left_tail_shifted_scenarios.sizes() == stress_input.sizes(),
        "scenario bank left-tail shift stress shape");
  check(stress_bank.tail_thickened_scenarios.sizes() == stress_input.sizes(),
        "scenario bank tail-thickened stress shape");
  auto base_corr = observer::covariance_to_correlation(
      observer::scenario_covariance(stress_input));
  auto high_corr = observer::covariance_to_correlation(
      observer::scenario_covariance(stress_bank.high_correlation_scenarios));
  check(high_corr.index({0, 1}).abs().item<double>() >
            base_corr.index({0, 1}).abs().item<double>(),
        "scenario bank high-correlation stress increases co-movement");
  check((stress_bank.left_tail_shifted_scenarios < stress_input)
            .all()
            .item<bool>(),
        "scenario bank left-tail shift moves all returns down");
  check(stress_bank.tail_thickened_scenarios.min().item<double>() <
            stress_input.min().item<double>(),
        "scenario bank tail-thickened stress worsens left tail");

  auto tail = observer::compute_node_tail_risk(bank.base_scenarios, 0.50);
  check(tail.var_down.sizes() == torch::IntArrayRef({2}),
        "tail risk VaR shape");
  check((tail.cvar_down <= tail.var_down).all().item<bool>(),
        "tail risk CVaR is left-tail mean");

  auto tc = observer::compute_transaction_cost(
      torch::tensor({0.10, -0.20}, torch::kFloat64),
      torch::full({2}, 0.01, torch::kFloat64),
      torch::full({2}, 0.10, torch::kFloat64));
  check(tc.per_asset_cost.sizes() == torch::IntArrayRef({2}),
        "transaction cost shape");
  check(tc.total_cost > 0.0, "transaction cost positive");

  auto score_options = torch::TensorOptions().dtype(torch::kFloat64);
  observer::confidence_inputs_t inputs{};
  inputs.liquidity_score = torch::tensor({0.8, 0.5}, score_options);
  inputs.calibration_score =
      observer::neutral_calibration_score(2, score_options);
  inputs.surprise_score = observer::neutral_surprise_score(2, score_options);
  inputs.entropy_score = torch::tensor({0.9, 0.7}, score_options);
  inputs.channel_score = torch::tensor({0.95, 0.6}, score_options);
  auto conf = observer::compute_confidence(inputs, 2, score_options);
  check(conf.sizes() == torch::IntArrayRef({2}), "confidence shape");
  check((conf >= 0.0).all().item<bool>() && (conf <= 1.0).all().item<bool>(),
        "confidence bounded");

  observer::value_projection_options_t projection_options{};
  projection_options.scale = std::vector<double>(9, 1.0);
  projection_options.loc = std::vector<double>(9, 0.0);
  projection_options.scale[3] = 2.0;
  projection_options.loc[3] = 1.0;
  projection_options.nonnegative_support_coords = {4, 5, 6};
  auto projected = observer::project_channel_consensus(cc, projection_options);
  check(projected.projected_mean.sizes() == torch::IntArrayRef({1, 3, 9}),
        "value projection mean shape");
  close(projected.projected_mean.index({0, 0, 3}).item<double>(), 1.05, 1e-12,
        "value projection affine close transform");
  check((projected.invalid_lower_support_probability >= 0.0).all().item<bool>(),
        "value projection support probability nonnegative");
  check((projected.invalid_lower_support_probability <= 1.0).all().item<bool>(),
        "value projection support probability bounded");

  auto vol = observer::blend_mdn_and_realized_variance(
      torch::tensor({0.04, 0.09}, torch::kFloat64),
      torch::tensor({0.16, 0.01}, torch::kFloat64), {.mdn_weight = 0.25});
  close(vol.final_variance.index({0}).item<double>(), 0.13, 1e-12,
        "volatility blended variance asset 0");
  close(vol.final_variance.index({1}).item<double>(), 0.03, 1e-12,
        "volatility blended variance asset 1");
}

void test_projection_validation_and_residual_quality() {
  auto residual_energy =
      torch::tensor({{{0.01, 0.02, 0.03}, {0.02, 0.03, 0.04}},
                     {{0.00, 0.01, 0.02}, {0.03, 0.02, 0.01}}},
                    torch::TensorOptions().dtype(torch::kFloat64));
  auto residual_quality =
      observer::compute_nodelift_residual_quality(residual_energy);
  check(residual_quality.available, "residual quality available");
  check(residual_quality.valid, "residual quality valid");
  close(residual_quality.finite_fraction, 1.0, 1e-12,
        "residual quality finite fraction");
  check(residual_quality.mean_residual_energy_by_coord.sizes() ==
            torch::IntArrayRef({3}),
        "residual quality coordinate shape");
  check((residual_quality.residual_quality_score_by_coord >= 0.0)
                .all()
                .item<bool>() &&
            (residual_quality.residual_quality_score_by_coord <= 1.0)
                .all()
                .item<bool>(),
        "residual quality score bounded");

  auto missing_residual =
      observer::compute_nodelift_residual_quality(torch::Tensor());
  check(!missing_residual.available, "missing residual quality unavailable");
  check(!missing_residual.diagnostics.warnings.empty(),
        "missing residual quality warns");

  auto projected_log_returns = torch::tensor(
      {{0.010, 0.020}, {0.012, 0.018}, {0.009, 0.021}, {0.011, 0.019}},
      torch::TensorOptions().dtype(torch::kFloat64));
  auto realized_log_return = torch::tensor(
      {0.011, 0.019}, torch::TensorOptions().dtype(torch::kFloat64));
  auto validation = observer::validate_projected_log_return(
      projected_log_returns, realized_log_return);
  check(validation.available, "projection validation available");
  check(validation.valid, "projection validation valid");
  check(validation.predicted_log_return.sizes() == torch::IntArrayRef({2}),
        "projection validation predicted shape");
  check(validation.mae < 0.01, "projection validation low MAE");
  check(validation.directional_accuracy > 0.99,
        "projection validation directional accuracy");
  check(validation.interval_coverage > 0.99,
        "projection validation interval coverage");
  check((validation.score >= 0.0).all().item<bool>() &&
            (validation.score <= 1.0).all().item<bool>(),
        "projection validation score bounded");
}

void test_nodelift_projection_and_coupler() {
  auto out = make_fixture_mdn();
  auto mask = torch::ones({1, 3, 3}, torch::kBool);
  auto cc = observer::compute_uniform_valid_channel_consensus(out, mask);
  auto surface = observer::from_channel_consensus(cc);
  check(surface.expected_potential.sizes() == torch::IntArrayRef({1, 3}),
        "NodeLift potential surface shape");

  auto corr =
      torch::tensor({{1.0, 0.35, 0.10}, {0.35, 1.0, 0.15}, {0.10, 0.15, 1.0}},
                    torch::TensorOptions().dtype(torch::kFloat64));
  observer::nodelift_return_projection_options_t options{};
  options.coupling_options.sample_count = 64;
  options.coupling_options.quantile_bisection_steps = 24;
  auto projected = observer::project_single_anchor(
      surface, /*anchor_slot=*/0, std::vector<int64_t>{0, 1},
      /*projection_reference_graph_index=*/2, corr, options);
  check(projected.potential_samples.sizes() == torch::IntArrayRef({64, 3}),
        "projection samples risky plus reference");
  check(projected.arithmetic_return_scenarios.sizes() ==
            torch::IntArrayRef({64, 2}),
        "projected return scenarios shape");
  check(
      torch::isfinite(projected.arithmetic_return_scenarios).all().item<bool>(),
      "projected return scenarios finite");
  check(projected.covariance.sizes() == torch::IntArrayRef({2, 2}),
        "projected scenario covariance shape");
  check(projected.projected_log_weight.sizes() == torch::IntArrayRef({2, 81}),
        "projected marginal combines asset and reference mixtures");
}

void test_allocation_belief_builder() {
  auto out = make_fixture_mdn();
  auto mask = torch::tensor(
      {{{true, false, true}, {true, true, true}, {true, true, true}}},
      torch::TensorOptions().dtype(torch::kBool));
  belief::allocation_belief_builder_options_t options{};
  options.anchor_slot = 0;
  options.anchor_key = "builder_anchor";
  options.timestamp_ms = 2000;
  options.graph_order_fingerprint = "fixture_graph";
  options.graph_node_ids = {"BTC", "ETH", "USDT"};
  options.node_ids = {"BTC", "ETH"};
  options.node_graph_indices = {0, 1};
  options.base_policy = {.accounting_numeraire_id = "USDT",
                         .settlement_asset_id = "USDT",
                         .reserve_asset_id = "USDT",
                         .projection_reference_node_id = "USDT"};
  options.channel_mask = mask;
  options.empirical_potential_correlation =
      torch::tensor({{1.0, 0.20, 0.10}, {0.20, 1.0, 0.15}, {0.10, 0.15, 1.0}},
                    torch::TensorOptions().dtype(torch::kFloat64));
  options.tradable_mask = torch::ones({2}, torch::kBool);
  options.realized_variance = torch::tensor({0.02, 0.03}, torch::kFloat64);
  options.linear_cost = torch::full({2}, 0.001, torch::kFloat64);
  options.quadratic_impact = torch::zeros({2}, torch::kFloat64);
  options.capacity_weight_limit = torch::full({2}, 0.40, torch::kFloat64);
  options.projection_options.coupling_options.sample_count = 64;
  options.projection_options.coupling_options.quantile_bisection_steps = 24;

  auto built = belief::build_single_anchor_allocation_belief(out, options);
  belief::validate_allocation_belief_contract(built.allocation_belief);
  check(built.allocation_belief.anchor_key == "builder_anchor",
        "builder preserves anchor key");
  check(built.allocation_belief.node_ids == options.node_ids,
        "builder preserves node ids");
  check(built.allocation_belief.base_policy.projection_reference_node_id ==
            "USDT",
        "builder preserves projection reference node");
  check(built.allocation_belief.projection_validation_required,
        "builder marks projection validation required");
  check(!built.allocation_belief.projection_validated,
        "builder leaves projection unvalidated");
  check(!built.allocation_belief.live_capital_allowed,
        "builder leaves live capital disabled");
  check(built.allocation_belief.projection_reference_graph_index.has_value() &&
            *built.allocation_belief.projection_reference_graph_index == 2,
        "builder derives projection reference graph index from node id");
  check(built.allocation_belief.graph_node_axis.axis_name == "G",
        "builder carries graph node axis binding");
  check(built.allocation_belief.graph_node_axis.node_ids ==
            options.graph_node_ids,
        "builder graph node axis binds node ids");
  check(built.allocation_belief.scenarios.sizes() ==
            torch::IntArrayRef({64, 2}),
        "builder scenario shape");
  check(built.scenario_bank.base_scenarios.sizes() ==
            torch::IntArrayRef({64, 2}),
        "builder scenario bank base shape");
  check(built.scenario_bank.high_correlation_scenarios.sizes() ==
            torch::IntArrayRef({64, 2}),
        "builder scenario bank high-correlation shape");
  check(built.scenario_bank.volatility_inflated_scenarios.sizes() ==
            torch::IntArrayRef({64, 2}),
        "builder scenario bank volatility shape");
  check(built.scenario_bank.left_tail_shifted_scenarios.sizes() ==
            torch::IntArrayRef({64, 2}),
        "builder scenario bank left-tail shape");
  check(built.scenario_bank.tail_thickened_scenarios.sizes() ==
            torch::IntArrayRef({64, 2}),
        "builder scenario bank tail-thickened shape");
  check(built.allocation_belief.confidence.sizes() == torch::IntArrayRef({2}),
        "builder confidence shape");
  check((built.allocation_belief.confidence >= 0.0).all().item<bool>() &&
            (built.allocation_belief.confidence <= 1.0).all().item<bool>(),
        "builder confidence bounded");
  check((built.allocation_belief.capacity_weight_limit <= 0.40 + 1e-12)
            .all()
            .item<bool>(),
        "builder capacity override applied");
  check(built.allocation_belief.adverse_excursion_prob.sizes() ==
            torch::IntArrayRef({2}),
        "builder adverse excursion shape");
  check(built.allocation_belief.var_down.sizes() == torch::IntArrayRef({2}),
        "builder tail risk shape");
  close(built.allocation_belief.linear_cost.index({0}).item<double>(), 0.001,
        1e-12, "builder linear cost");
  close(built.allocation_belief.channel_disagreement.index({0}).item<double>(),
        built.channel_consensus.channel_disagreement.index({0, 0, 3})
            .item<double>(),
        1e-12, "builder channel disagreement");

  mdn::MdnOut batch_out{};
  batch_out.log_pi = torch::cat({out.log_pi, out.log_pi}, /*dim=*/0);
  batch_out.mu = torch::cat({out.mu, out.mu + 0.001}, /*dim=*/0);
  batch_out.sigma = torch::cat({out.sigma, out.sigma}, /*dim=*/0);
  belief::allocation_belief_batch_builder_options_t batch_options{};
  batch_options.common = options;
  batch_options.common.channel_mask = torch::ones({2, 3, 3}, torch::kBool);
  batch_options.common.projection_options.coupling_options.sample_count = 32;
  batch_options.anchor_keys = {"batch_anchor_0", "batch_anchor_1"};
  batch_options.timestamps_ms = {3000, 4000};
  auto batch = belief::build_allocation_belief_batch(batch_out, batch_options);
  check(batch.beliefs.size() == 2, "belief batch builder size");
  check(batch.beliefs[0].anchor_key == "batch_anchor_0",
        "belief batch first anchor key");
  check(batch.beliefs[1].timestamp_ms == 4000, "belief batch second timestamp");
  const auto &selected =
      belief::select_decision_anchor(batch, "batch_anchor_1");
  check(selected.anchor_key == "batch_anchor_1",
        "belief batch explicit anchor selection");
  auto collated = belief::collate_allocation_belief_batch(batch);
  check(collated.anchor_keys[0] == "batch_anchor_0" &&
            collated.anchor_keys[1] == "batch_anchor_1",
        "belief batch collate preserves anchor order");
  check(collated.expected_arithmetic_return.sizes() ==
            torch::IntArrayRef({2, 2}),
        "belief batch collate expected return shape");
  check(collated.scenarios.sizes() == torch::IntArrayRef({2, 32, 2}),
        "belief batch collate scenario shape");
  check(collated.covariance.sizes() == torch::IntArrayRef({2, 2, 2}),
        "belief batch collate covariance shape");
  check(collated.confidence.sizes() == torch::IntArrayRef({2, 2}),
        "belief batch collate confidence shape");
  check(collated.node_ids == options.node_ids,
        "belief batch collate preserves risky universe");
  check(collated.base_policy.projection_reference_node_id == "USDT",
        "belief batch collate preserves projection reference node");

  auto incompatible = batch;
  incompatible.beliefs[1].node_ids[1] = "SOL";
  bool rejected_incompatible = false;
  try {
    (void)belief::collate_allocation_belief_batch(incompatible);
  } catch (const std::exception &) {
    rejected_incompatible = true;
  }
  check(rejected_incompatible,
        "belief batch collate rejects incompatible universes");
}

belief::AllocationBelief make_allocation_belief() {
  belief::allocation_belief_builder_options_t options{};
  options.anchor_slot = 0;
  options.anchor_key = "1000";
  options.timestamp_ms = 1000;
  options.graph_order_fingerprint = "fixture_graph";
  options.graph_node_ids = {"BTC", "ETH", "USDT"};
  options.node_ids = {"BTC", "ETH"};
  options.node_graph_indices = {0, 1};
  options.base_policy = {.accounting_numeraire_id = "USDT",
                         .settlement_asset_id = "USDT",
                         .reserve_asset_id = "USDT",
                         .projection_reference_node_id = "USDT"};
  options.channel_mask = torch::ones({1, 3, 3}, torch::kBool);
  options.empirical_potential_correlation =
      torch::tensor({{1.0, 0.25, 0.10}, {0.25, 1.0, 0.15}, {0.10, 0.15, 1.0}},
                    torch::TensorOptions().dtype(torch::kFloat64));
  options.tradable_mask = torch::ones({2}, torch::kBool);
  options.linear_cost = torch::full({2}, 0.001, torch::kFloat64);
  options.quadratic_impact = torch::zeros({2}, torch::kFloat64);
  options.capacity_weight_limit = torch::full({2}, 0.50, torch::kFloat64);
  options.projection_options.coupling_options.sample_count = 64;
  options.projection_options.coupling_options.quantile_bisection_steps = 24;
  auto built = belief::build_single_anchor_allocation_belief(make_fixture_mdn(),
                                                             options);
  built.allocation_belief.confidence =
      torch::tensor({0.80, 0.70}, torch::kFloat64);
  return built.allocation_belief;
}

execution::spot_rebalance_router::spot_edge_market_state_t
make_spot_edge_market_state() {
  execution::spot_rebalance_router::spot_edge_market_state_t market{};
  market.graph.node_ids = {"BTC", "ETH", "USDT"};
  market.graph.edge_ids = {"BTCUSDT", "ETHUSDT"};
  market.graph.base_index = {0, 1};
  market.graph.quote_index = {2, 2};
  market.edge_fee_rate = torch::full({2}, 0.001, torch::kFloat64);
  market.edge_spread_rate = torch::full({2}, 0.002, torch::kFloat64);
  market.edge_slippage_rate = torch::full({2}, 0.0005, torch::kFloat64);
  market.min_notional_base = torch::full({2}, 10.0, torch::kFloat64);
  market.max_notional_base = torch::full({2}, 10000.0, torch::kFloat64);
  market.edge_tradable_mask = torch::ones({2}, torch::kBool);
  return market;
}

struct persisted_fixture_t {
  belief::AllocationBelief allocation_belief{};
  observer::nodelift_return_projection_t projection{};
  observer::scenario_bank_t scenario_bank{};
};

persisted_fixture_t make_persisted_fixture() {
  belief::allocation_belief_builder_options_t options{};
  options.anchor_slot = 0;
  options.anchor_key = "1000";
  options.timestamp_ms = 1000;
  options.graph_order_fingerprint = "fixture_graph";
  options.graph_node_ids = {"BTC", "ETH", "USDT"};
  options.node_ids = {"BTC", "ETH"};
  options.node_graph_indices = {0, 1};
  options.base_policy = {.accounting_numeraire_id = "USDT",
                         .settlement_asset_id = "USDT",
                         .reserve_asset_id = "USDT",
                         .projection_reference_node_id = "USDT"};
  options.channel_mask = torch::ones({1, 3, 3}, torch::kBool);
  options.empirical_potential_correlation =
      torch::tensor({{1.0, 0.25, 0.10}, {0.25, 1.0, 0.15}, {0.10, 0.15, 1.0}},
                    torch::TensorOptions().dtype(torch::kFloat64));
  options.tradable_mask = torch::ones({2}, torch::kBool);
  options.linear_cost = torch::full({2}, 0.001, torch::kFloat64);
  options.quadratic_impact = torch::zeros({2}, torch::kFloat64);
  options.capacity_weight_limit = torch::full({2}, 0.50, torch::kFloat64);
  options.projection_options.coupling_options.sample_count = 64;
  options.projection_options.coupling_options.quantile_bisection_steps = 24;
  auto built = belief::build_single_anchor_allocation_belief(make_fixture_mdn(),
                                                             options);
  persisted_fixture_t fixture{};
  fixture.allocation_belief = std::move(built.allocation_belief);
  fixture.projection = std::move(built.return_projection);
  fixture.scenario_bank = std::move(built.scenario_bank);
  return fixture;
}

void test_forecast_persistence_surprise_and_calibration() {
  auto fixture = make_persisted_fixture();
  auto artifact = observer::make_from_allocation_belief_and_projection(
      fixture.allocation_belief, fixture.projection, "fixture_model_v1",
      "target_coords:0,1,2,3,4,5,6,7,8", "identity_norm",
      &fixture.scenario_bank);
  check(artifact.identity.node_ids_fingerprint ==
            observer::node_ids_fingerprint(fixture.allocation_belief.node_ids),
        "forecast artifact node id fingerprint");
  check(artifact.identity.source_feature_semantics_fingerprint ==
            fixture.allocation_belief.source_feature_semantics_fingerprint,
        "forecast artifact records feature semantics fingerprint");

  const auto path = std::filesystem::path("/tmp") /
                    "wikimyei_observer_forecast_artifact_test.pt";
  std::filesystem::remove(path);
  observer::save_forecast_artifact(artifact, path);
  auto loaded = observer::load_forecast_artifact(path);
  observer::validate_forecast_artifact(loaded,
                                       /*require_scenarios=*/true);
  check(loaded.identity.anchor_key == "1000", "forecast artifact anchor key");
  check(loaded.identity.node_ids == fixture.allocation_belief.node_ids,
        "forecast artifact node ids");
  check(loaded.identity.graph_node_axis.node_ids ==
            fixture.allocation_belief.graph_node_axis.node_ids,
        "forecast artifact graph node axis round-trip");
  check(loaded.identity.graph_node_axis.axis_name == "G",
        "forecast artifact graph node axis name");
  check(torch::allclose(loaded.log_weight, artifact.log_weight, 1e-12, 1e-12),
        "forecast artifact log weights round-trip");
  check(torch::allclose(loaded.scenarios, artifact.scenarios, 1e-12, 1e-12),
        "forecast artifact scenarios round-trip");
  check(torch::allclose(loaded.high_correlation_scenarios,
                        artifact.high_correlation_scenarios, 1e-12, 1e-12),
        "forecast artifact high-correlation scenarios round-trip");
  check(torch::allclose(loaded.volatility_inflated_scenarios,
                        artifact.volatility_inflated_scenarios, 1e-12, 1e-12),
        "forecast artifact volatility stress scenarios round-trip");
  check(torch::allclose(loaded.left_tail_shifted_scenarios,
                        artifact.left_tail_shifted_scenarios, 1e-12, 1e-12),
        "forecast artifact left-tail stress scenarios round-trip");
  check(torch::allclose(loaded.tail_thickened_scenarios,
                        artifact.tail_thickened_scenarios, 1e-12, 1e-12),
        "forecast artifact tail-thickened scenarios round-trip");

  auto realized_near = torch::tensor(
      {0.02, 0.04}, torch::TensorOptions().dtype(torch::kFloat64));
  auto realized_far = torch::tensor(
      {1.50, 1.50}, torch::TensorOptions().dtype(torch::kFloat64));
  auto near_surprise = observer::compute_from_artifact(loaded, realized_near);
  auto far_surprise = observer::compute_from_artifact(loaded, realized_far);
  check(near_surprise.surprise.sizes() == torch::IntArrayRef({2}),
        "surprise shape");
  check((near_surprise.valid_mask).all().item<bool>(), "surprise valid mask");
  check(far_surprise.surprise.mean().item<double>() >
            near_surprise.surprise.mean().item<double>(),
        "far realization has higher surprise");
  check(far_surprise.score.mean().item<double>() <
            near_surprise.score.mean().item<double>(),
        "far realization has lower surprise score");

  std::vector<observer::calibration_observation_t> observations;
  observations.push_back(observer::observe_from_artifact(
      loaded, torch::tensor({0.02, 0.04}, torch::kFloat64)));
  observations.push_back(observer::observe_from_artifact(
      loaded, torch::tensor({0.01, 0.03}, torch::kFloat64)));
  observations.push_back(observer::observe_from_artifact(
      loaded, torch::tensor({-0.01, 0.02}, torch::kFloat64)));
  auto summary = observer::summarize_observations(observations);
  check(summary.observation_count.sizes() == torch::IntArrayRef({2}),
        "calibration observation count shape");
  close(summary.observation_count.min().item<double>(), 3.0, 1e-12,
        "calibration all observations counted");
  check((summary.score >= 0.0).all().item<bool>() &&
            (summary.score <= 1.0).all().item<bool>(),
        "calibration score bounded");
  check((summary.pit_mean >= 0.0).all().item<bool>() &&
            (summary.pit_mean <= 1.0).all().item<bool>(),
        "calibration PIT bounded");

  auto surprise_adjusted =
      belief::apply_surprise(fixture.allocation_belief, far_surprise);
  belief::validate_allocation_belief_contract(surprise_adjusted);
  check(surprise_adjusted.confidence.mean().item<double>() <
            fixture.allocation_belief.confidence.mean().item<double>(),
        "belief surprise update lowers confidence");
  auto surprise_adjusted_again =
      belief::apply_surprise(surprise_adjusted, far_surprise);
  check(torch::allclose(surprise_adjusted_again.confidence,
                        surprise_adjusted.confidence, 1e-12, 1e-12),
        "belief surprise update is idempotent for same observation");

  auto health_adjusted = belief::apply_calibration(surprise_adjusted, summary);
  belief::validate_allocation_belief_contract(health_adjusted);
  check(torch::allclose(health_adjusted.calibration_score, summary.score, 1e-12,
                        1e-12),
        "belief calibration update stores score");
  check((health_adjusted.confidence <= surprise_adjusted.confidence + 1e-12)
            .all()
            .item<bool>(),
        "belief calibration update does not raise confidence above surprise "
        "adjusted confidence");

  std::filesystem::remove(path);
}

void test_belief_contract_and_portfolio_engine() {
  auto state = make_allocation_belief();
  belief::validate_allocation_belief_contract(state);
  check(observer::check_allocation_belief(state).valid,
        "belief data quality valid");

  auto bad_confidence = state;
  bad_confidence.confidence = torch::tensor(
      {1.20, 0.50}, torch::TensorOptions().dtype(torch::kFloat64));
  check(!observer::check_allocation_belief(bad_confidence).valid,
        "belief data quality rejects out-of-range confidence");

  auto bad_correlation = state;
  bad_correlation.correlation.index_put_({0, 0}, 0.50);
  check(!observer::check_allocation_belief(bad_correlation).valid,
        "belief data quality rejects invalid correlation diagonal");

  auto bad_covariance = state;
  bad_covariance.covariance = torch::tensor(
      {{1.0, 2.0}, {2.0, 1.0}}, torch::TensorOptions().dtype(torch::kFloat64));
  check(!observer::check_allocation_belief(bad_covariance).valid,
        "belief data quality rejects non-PSD covariance");

  portfolio::PortfolioState portfolio_state{};
  portfolio_state.timestamp_ms = 1000;
  portfolio_state.accounting_node_id = "USDT";
  portfolio_state.reserve_node_id = "USDT";
  portfolio_state.current_weights = torch::tensor(
      {0.10, 0.10}, torch::TensorOptions().dtype(torch::kFloat64));
  portfolio_state.current_units = torch::ones({2}, torch::kFloat64);
  portfolio_state.base_reserve_weight = 0.80;
  portfolio::validate_portfolio_state(portfolio_state, 2);

  auto invalid_portfolio_state = portfolio_state;
  invalid_portfolio_state.current_weights = torch::tensor(
      {0.30, 0.30}, torch::TensorOptions().dtype(torch::kFloat64));
  bool rejected_bad_portfolio_state = false;
  try {
    portfolio::validate_portfolio_state(invalid_portfolio_state, 2);
  } catch (const std::exception &) {
    rejected_bad_portfolio_state = true;
  }
  check(rejected_bad_portfolio_state,
        "portfolio state rejects weights plus base reserve above one");

  portfolio::MarketState market{};
  market.timestamp_ms = 1000;
  market.tradable_mask = torch::ones({2}, torch::kBool);
  market.executable_mid = torch::tensor({100.0, 50.0}, torch::kFloat64);
  market.fee_rate = torch::full({2}, 0.001, torch::kFloat64);
  portfolio::validate_market_state(market, 2);

  auto invalid_market = market;
  invalid_market.fee_rate = torch::tensor({0.001, -0.001}, torch::kFloat64);
  bool rejected_bad_market = false;
  try {
    portfolio::validate_market_state(invalid_market, 2);
  } catch (const std::exception &) {
    rejected_bad_market = true;
  }
  check(rejected_bad_market, "market state rejects negative fee");

  portfolio::PortfolioConstraints constraints{};
  constraints.min_base_reserve_weight = 0.20;
  constraints.max_weight = torch::full({2}, 0.60, torch::kFloat64);
  constraints.max_turnover_l1 = 0.50;
  constraints.lambda_cvar = 0.5;
  constraints.lambda_concentration = 0.01;
  constraints.lambda_uncertainty = 0.01;
  constraints.lambda_turnover = 0.001;
  portfolio::validate_portfolio_constraints(constraints, 2);

  auto invalid_constraints = constraints;
  invalid_constraints.min_weight = torch::tensor(
      {0.50, 0.40}, torch::TensorOptions().dtype(torch::kFloat64));
  bool rejected_bad_constraints = false;
  try {
    portfolio::validate_portfolio_constraints(invalid_constraints, 2);
  } catch (const std::exception &) {
    rejected_bad_constraints = true;
  }
  check(rejected_bad_constraints,
        "portfolio constraints reject infeasible minimum weights");

  portfolio::spot_distributional_utility::solver_options_t solver_options{};
  solver_options.iterations = 30;
  solver_options.learning_rate = 0.03;
  auto target = portfolio::spot_distributional_utility::solve(
      state, portfolio_state, market, constraints, solver_options);
  check(target.valid, "target portfolio valid");
  portfolio::validate_target_portfolio(target, 2,
                                       portfolio_state.current_weights);
  check(target.target_weights.sizes() == torch::IntArrayRef({2}),
        "target weights shape");
  check(target.target_weights.min().item<double>() >= -1e-10,
        "long-only target weights");
  check(target.target_weights.sum().item<double>() <= 0.80 + 1e-8,
        "base-reserve budget respected");
  close(target.target_base_reserve_weight,
        1.0 - target.target_weights.sum().item<double>(), 1e-10,
        "base reserve is implicit");
}

void test_method_support_allocators() {
  auto state = make_allocation_belief();

  portfolio::PortfolioState portfolio_state{};
  portfolio_state.timestamp_ms = 1000;
  portfolio_state.accounting_node_id = "USDT";
  portfolio_state.reserve_node_id = "USDT";
  portfolio_state.current_weights = torch::tensor(
      {0.05, 0.05}, torch::TensorOptions().dtype(torch::kFloat64));
  portfolio_state.current_units = torch::ones({2}, torch::kFloat64);
  portfolio_state.base_reserve_weight = 0.90;

  portfolio::MarketState market{};
  market.timestamp_ms = 1000;
  market.tradable_mask = torch::ones({2}, torch::kBool);

  portfolio::PortfolioConstraints constraints{};
  constraints.min_base_reserve_weight = 0.25;
  constraints.max_weight = torch::full({2}, 0.40, torch::kFloat64);
  constraints.max_turnover_l1 = 0.60;
  constraints.cvar_alpha = 0.90;
  constraints.lambda_cvar = 0.5;
  constraints.lambda_concentration = 0.01;
  constraints.lambda_uncertainty = 0.01;
  constraints.lambda_turnover = 0.001;

  baseline::mean_variance::solver_options_t mv_options{};
  mv_options.iterations = 25;
  mv_options.learning_rate = 0.03;
  auto mv = baseline::mean_variance::solve(state, portfolio_state, market,
                                           constraints, mv_options);
  check(mv.valid, "mean variance baseline valid");
  portfolio::validate_target_portfolio(mv, 2, portfolio_state.current_weights);
  check(mv.target_weights.sizes() == torch::IntArrayRef({2}),
        "mean variance target shape");
  check(mv.target_weights.sum().item<double>() <= 0.75 + 1e-8,
        "mean variance base-reserve budget");

  baseline::cvar::solver_options_t cvar_options{};
  cvar_options.iterations = 25;
  cvar_options.learning_rate = 0.03;
  auto cvar = baseline::cvar::solve(state, portfolio_state, market, constraints,
                                    cvar_options);
  check(cvar.valid, "CVaR allocator valid");
  portfolio::validate_target_portfolio(cvar, 2,
                                       portfolio_state.current_weights);
  check(cvar.cvar_loss >= 0.0, "CVaR allocator reports loss");
  check(cvar.target_weights.sum().item<double>() <= 0.75 + 1e-8,
        "CVaR allocator base-reserve budget");

  auto parity =
      fallback::risk_parity::solve(state, portfolio_state, market, constraints);
  check(parity.valid, "risk parity fallback valid");
  portfolio::validate_target_portfolio(parity, 2,
                                       portfolio_state.current_weights);
  check(parity.target_weights.min().item<double>() >= -1e-10,
        "risk parity long only");
  check(parity.target_weights.sum().item<double>() <= 0.75 + 1e-8,
        "risk parity base-reserve budget");
}

void test_risk_gate() {
  auto state = make_allocation_belief();

  portfolio::PortfolioState portfolio_state{};
  portfolio_state.timestamp_ms = 1000;
  portfolio_state.accounting_node_id = "USDT";
  portfolio_state.reserve_node_id = "USDT";
  portfolio_state.current_weights = torch::tensor(
      {0.10, 0.10}, torch::TensorOptions().dtype(torch::kFloat64));
  portfolio_state.current_units = torch::ones({2}, torch::kFloat64);
  portfolio_state.base_reserve_weight = 0.80;
  portfolio_state.equity_value_base = 1000.0;

  risk::risk_gate::risk_gate_thresholds_t thresholds{};
  thresholds.min_mean_confidence = 0.10;
  thresholds.min_mean_liquidity = 0.10;
  thresholds.max_drawdown = 0.25;
  thresholds.max_mean_abs_correlation = 0.99;
  auto gate = risk::risk_gate::evaluate(state, portfolio_state, thresholds);
  check(gate.allow_trading, "risk gate allows healthy state");
  check(!gate.force_base_reserve_fallback,
        "risk gate healthy state no base reserve fallback");

  auto low_confidence = state;
  low_confidence.confidence = torch::zeros({2}, torch::kFloat64);
  auto blocked =
      risk::risk_gate::evaluate(low_confidence, portfolio_state, thresholds);
  check(!blocked.allow_trading, "risk gate blocks low confidence");
  check(blocked.force_base_reserve_fallback,
        "risk gate low confidence base reserve fallback");
  check(!blocked.reasons.empty(), "risk gate reports reason");

  portfolio::PortfolioConstraints constraints{};
  constraints.max_turnover_l1 = 0.10;
  auto guarded = risk::risk_gate::enforce(low_confidence, portfolio_state,
                                          constraints, thresholds);
  check(guarded.valid, "risk gate base reserve fallback target valid");
  check(guarded.target_weights.sum().item<double>() <
            portfolio_state.current_weights.sum().item<double>(),
        "risk gate de-risks toward base reserve");
}

void test_spot_distributional_utility_decision_wrapper() {
  auto state = make_allocation_belief();

  portfolio::PortfolioState portfolio_state{};
  portfolio_state.timestamp_ms = 1000;
  portfolio_state.accounting_node_id = "USDT";
  portfolio_state.reserve_node_id = "USDT";
  portfolio_state.current_weights = torch::tensor(
      {0.10, 0.10}, torch::TensorOptions().dtype(torch::kFloat64));
  portfolio_state.current_units = torch::ones({2}, torch::kFloat64);
  portfolio_state.base_reserve_weight = 0.80;
  portfolio_state.equity_value_base = 1000.0;

  portfolio::MarketState market{};
  market.timestamp_ms = 1000;
  market.tradable_mask = torch::ones({2}, torch::kBool);

  portfolio::PortfolioConstraints constraints{};
  constraints.min_base_reserve_weight = 0.20;
  constraints.max_weight = torch::full({2}, 0.60, torch::kFloat64);
  constraints.max_turnover_l1 = 0.50;
  constraints.lambda_cvar = 0.5;
  constraints.lambda_concentration = 0.01;
  constraints.lambda_uncertainty = 0.01;
  constraints.lambda_turnover = 0.001;

  auto edge_market = make_spot_edge_market_state();
  decision_step::decision_step_options_t options{};
  options.risk_thresholds.min_mean_confidence = 0.10;
  options.risk_thresholds.min_mean_liquidity = 0.10;
  options.risk_thresholds.max_mean_abs_correlation = 0.99;
  options.solver_options.iterations = 20;
  options.solver_options.learning_rate = 0.03;

  auto result = decision_step::run(state, portfolio_state, market, constraints,
                                   edge_market, options);
  check(result.risk_gate.allow_trading, "decision step allows healthy belief");
  check(!result.risk_gate.force_base_reserve_fallback,
        "decision step healthy belief no base reserve fallback");
  check(result.target.valid, "decision step target valid");
  check(result.rebalance_plan.valid, "decision step rebalance valid");
  check(result.valid, "decision step result valid");
  check(result.report.text.find("target_present=true") != std::string::npos,
        "decision step report includes target");
  check(result.report.text.find("rebalance_present=true") != std::string::npos,
        "decision step report includes rebalance plan");

  auto blocked_state = state;
  blocked_state.confidence = torch::zeros({2}, torch::kFloat64);
  constraints.max_turnover_l1 = 0.10;
  auto blocked = decision_step::run(blocked_state, portfolio_state, market,
                                    constraints, edge_market, options);
  check(!blocked.risk_gate.allow_trading,
        "decision step blocks low-confidence belief");
  check(blocked.risk_gate.force_base_reserve_fallback,
        "decision step low-confidence belief uses base reserve fallback");
  check(blocked.target.valid,
        "decision step base reserve fallback target valid");
  check(blocked.rebalance_plan.valid,
        "decision step base reserve fallback rebalance valid");
  check(blocked.target.target_weights.sum().item<double>() <
            portfolio_state.current_weights.sum().item<double>(),
        "decision step base reserve fallback de-risks");
  check(!blocked.rebalance_plan.orders.empty(),
        "decision step base reserve fallback routes de-risking orders");
  check(blocked.report.text.find("target_valid=true") != std::string::npos,
        "decision step base reserve fallback report target valid");
}

void test_spot_rebalance_router() {
  portfolio::PortfolioState portfolio_state{};
  portfolio_state.timestamp_ms = 1000;
  portfolio_state.accounting_node_id = "USDT";
  portfolio_state.reserve_node_id = "USDT";
  portfolio_state.current_weights = torch::tensor(
      {0.10, 0.10}, torch::TensorOptions().dtype(torch::kFloat64));
  portfolio_state.current_units = torch::ones({2}, torch::kFloat64);
  portfolio_state.base_reserve_weight = 0.80;
  portfolio_state.equity_value_base = 1000.0;

  portfolio::TargetPortfolio target{};
  target.timestamp_ms = 1000;
  target.node_ids = {"BTC", "ETH"};
  target.base_reserve_node_id = "USDT";
  target.target_weights = torch::tensor(
      {0.25, 0.05}, torch::TensorOptions().dtype(torch::kFloat64));
  target.target_base_reserve_weight = 0.70;
  target.delta_weights = torch::tensor(
      {0.15, -0.05}, torch::TensorOptions().dtype(torch::kFloat64));
  target.turnover = 0.20;
  target.valid = true;

  execution::spot_rebalance_router::spot_edge_market_state_t market{};
  market.graph.node_ids = {"BTC", "ETH", "USDT"};
  market.graph.edge_ids = {"BTCUSDT", "ETHUSDT"};
  market.graph.base_index = {0, 1};
  market.graph.quote_index = {2, 2};
  market.edge_fee_rate = torch::full({2}, 0.001, torch::kFloat64);
  market.edge_spread_rate = torch::full({2}, 0.002, torch::kFloat64);
  market.edge_slippage_rate = torch::full({2}, 0.0005, torch::kFloat64);
  market.min_notional_base = torch::full({2}, 10.0, torch::kFloat64);
  market.max_notional_base = torch::full({2}, 10000.0, torch::kFloat64);
  market.edge_tradable_mask = torch::ones({2}, torch::kBool);
  execution::spot_rebalance_router::validate_spot_edge_market_state(market);

  auto bad_edge_fee_market = market;
  bad_edge_fee_market.edge_fee_rate = torch::tensor(
      {0.001, -0.001}, torch::TensorOptions().dtype(torch::kFloat64));
  bool rejected_bad_edge_fee = false;
  try {
    execution::spot_rebalance_router::validate_spot_edge_market_state(
        bad_edge_fee_market);
  } catch (const std::exception &) {
    rejected_bad_edge_fee = true;
  }
  check(rejected_bad_edge_fee, "spot edge market rejects negative fee");

  auto bad_edge_notional_market = market;
  bad_edge_notional_market.min_notional_base =
      torch::full({2}, 100.0, torch::kFloat64);
  bad_edge_notional_market.max_notional_base =
      torch::full({2}, 10.0, torch::kFloat64);
  bool rejected_bad_edge_notional = false;
  try {
    execution::spot_rebalance_router::validate_spot_edge_market_state(
        bad_edge_notional_market);
  } catch (const std::exception &) {
    rejected_bad_edge_notional = true;
  }
  check(rejected_bad_edge_notional,
        "spot edge market rejects max below min notional");

  auto bad_edge_mask_market = market;
  bad_edge_mask_market.edge_tradable_mask =
      torch::ones({2}, torch::TensorOptions().dtype(torch::kFloat64));
  bool rejected_bad_edge_mask = false;
  try {
    execution::spot_rebalance_router::validate_spot_edge_market_state(
        bad_edge_mask_market);
  } catch (const std::exception &) {
    rejected_bad_edge_mask = true;
  }
  check(rejected_bad_edge_mask, "spot edge market rejects non-bool mask");

  auto plan = execution::spot_rebalance_router::plan_rebalance(
      target, portfolio_state, market);
  check(plan.valid, "spot rebalance plan valid");
  check(plan.orders.size() == 2, "spot rebalance routes two orders");
  check(plan.orders[0].edge_id == "BTCUSDT", "BTC routes through real edge");
  check(plan.orders[0].side ==
            execution::spot_rebalance_router::order_side_t::buy_edge_base,
        "BTC increase buys edge base");
  check(plan.orders[1].side ==
            execution::spot_rebalance_router::order_side_t::sell_edge_base,
        "ETH decrease sells edge base");
  close(plan.requested_notional_base, 200.0, 1e-9,
        "spot rebalance requested notional");
  close(plan.routed_notional_base, 200.0, 1e-9,
        "spot rebalance routed notional");
  check(plan.estimated_transaction_cost_base > 0.0,
        "spot rebalance estimates costs");

  auto bad_base_reserve_target = target;
  bad_base_reserve_target.target_base_reserve_weight = 0.60;
  bool rejected_bad_target_base_reserve = false;
  try {
    (void)execution::spot_rebalance_router::plan_rebalance(
        bad_base_reserve_target, portfolio_state, market);
  } catch (const std::exception &) {
    rejected_bad_target_base_reserve = true;
  }
  check(rejected_bad_target_base_reserve,
        "spot rebalance rejects target base reserve mismatch");

  auto bad_delta_target = target;
  bad_delta_target.delta_weights = torch::tensor(
      {0.10, -0.05}, torch::TensorOptions().dtype(torch::kFloat64));
  bool rejected_bad_target_delta = false;
  try {
    (void)execution::spot_rebalance_router::plan_rebalance(
        bad_delta_target, portfolio_state, market);
  } catch (const std::exception &) {
    rejected_bad_target_delta = true;
  }
  check(rejected_bad_target_delta,
        "spot rebalance rejects inconsistent target delta");

  market.graph.edge_ids = {"BTCUSDT"};
  market.graph.base_index = {0};
  market.graph.quote_index = {2};
  market.edge_fee_rate = torch::full({1}, 0.001, torch::kFloat64);
  market.edge_spread_rate = torch::full({1}, 0.002, torch::kFloat64);
  market.edge_slippage_rate = torch::full({1}, 0.0005, torch::kFloat64);
  market.min_notional_base = torch::full({1}, 10.0, torch::kFloat64);
  market.max_notional_base = torch::full({1}, 10000.0, torch::kFloat64);
  market.edge_tradable_mask = torch::ones({1}, torch::kBool);
  auto missing = execution::spot_rebalance_router::plan_rebalance(
      target, portfolio_state, market);
  check(!missing.valid, "missing direct edge invalidates material route");
  check(!missing.skipped.empty() &&
            missing.skipped.back().reason == "no_direct_market_edge",
        "missing direct edge is reported");
}

void test_portfolio_ledger_and_belief_reporter() {
  auto ledger = accounting::portfolio_ledger::make_ledger(
      /*timestamp_ms=*/1000, "USDT", std::vector<std::string>{"BTC", "ETH"},
      /*base_reserve_units=*/1000.0);

  accounting::portfolio_ledger::executed_fill_t buy{};
  buy.timestamp_ms = 1001;
  buy.node_id = "BTC";
  buy.edge_id = "BTCUSDT";
  buy.side = accounting::portfolio_ledger::fill_side_t::buy_asset;
  buy.quantity_asset = 0.01;
  buy.gross_notional_base = 100.0;
  buy.fee_base = 1.0;
  accounting::portfolio_ledger::apply_fill(ledger, buy);
  close(ledger.base_reserve_units, 899.0, 1e-12, "ledger buy base reserve");
  close(ledger.average_cost_base.index({0}).item<double>(), 10100.0, 1e-9,
        "ledger average cost includes buy fee");

  accounting::portfolio_ledger::executed_fill_t sell{};
  sell.timestamp_ms = 1002;
  sell.node_id = "BTC";
  sell.edge_id = "BTCUSDT";
  sell.side = accounting::portfolio_ledger::fill_side_t::sell_asset;
  sell.quantity_asset = 0.005;
  sell.gross_notional_base = 60.0;
  sell.fee_base = 0.5;
  accounting::portfolio_ledger::apply_fill(ledger, sell);
  close(ledger.base_reserve_units, 958.5, 1e-12, "ledger sell base reserve");
  close(ledger.realized_pnl_base.index({0}).item<double>(), 9.0, 1e-9,
        "ledger realized pnl");
  close(ledger.cumulative_fees_base, 1.5, 1e-12, "ledger cumulative fees");

  auto marked = accounting::portfolio_ledger::mark_to_market(
      ledger, torch::tensor({12000.0, 1000.0}, torch::kFloat64),
      /*timestamp_ms=*/1003);
  close(marked.equity_value_base, 1018.5, 1e-9, "ledger marked equity");
  close(marked.current_units.index({0}).item<double>(), 0.005, 1e-12,
        "ledger marked units");
  check(marked.current_weights.index({0}).item<double>() > 0.0,
        "ledger marked risky weight");
  check(marked.base_reserve_weight < 1.0 && marked.base_reserve_weight > 0.0,
        "ledger marked base reserve weight");

  auto state = make_allocation_belief();
  portfolio::TargetPortfolio target{};
  target.timestamp_ms = 1003;
  target.node_ids = state.node_ids;
  target.base_reserve_node_id = state.base_policy.reserve_asset_id;
  target.target_weights = torch::tensor({0.20, 0.10}, torch::kFloat64);
  target.target_base_reserve_weight = 0.70;
  target.delta_weights = torch::tensor({0.10, 0.0}, torch::kFloat64);
  target.expected_log_growth = 0.01;
  target.expected_arithmetic_return = 0.012;
  target.cvar_loss = 0.03;
  target.turnover = 0.10;
  target.estimated_transaction_cost = 0.001;
  target.valid = true;

  execution::spot_rebalance_router::spot_rebalance_plan_t plan{};
  plan.timestamp_ms = 1003;
  plan.base_reserve_node_id = "USDT";
  plan.node_ids = state.node_ids;
  plan.requested_turnover_weight = 0.10;
  plan.routed_turnover_weight = 0.10;
  plan.requested_notional_base = 100.0;
  plan.routed_notional_base = 100.0;
  plan.estimated_transaction_cost_base = 0.20;
  plan.valid = true;
  plan.orders.push_back(
      {.node_id = "BTC",
       .edge_id = "BTCUSDT",
       .edge_base_node_id = "BTC",
       .edge_quote_node_id = "USDT",
       .side = execution::spot_rebalance_router::order_side_t::buy_edge_base,
       .delta_weight = 0.10,
       .requested_notional_base = 100.0,
       .routed_notional_base = 100.0,
       .estimated_cost_base = 0.20});

  auto report =
      monitoring::belief_reporter::make_report({.belief_state = &state,
                                                .portfolio_state = &marked,
                                                .target = &target,
                                                .rebalance_plan = &plan,
                                                .ledger = &ledger});
  check(report.text.find("report_schema_id=wikimyei.engine.portfolio."
                         "spot_distributional_utility.monitoring."
                         "belief_reporter.v1") != std::string::npos,
        "belief report schema id");
  check(report.text.find("anchor_key=1000") != std::string::npos,
        "belief report anchor key");
  check(report.text.find("rebalance_order_count=1") != std::string::npos,
        "belief report rebalance order count");
  check(report.text.find("ledger_fill_count=2") != std::string::npos,
        "belief report ledger fill count");
}

} // namespace

int main() {
  try {
    torch::manual_seed(7);
    test_feature_semantics_detection();
    test_moments_and_channel_consensus();
    test_auxiliary_observers();
    test_projection_validation_and_residual_quality();
    test_nodelift_projection_and_coupler();
    test_allocation_belief_builder();
    test_belief_contract_and_portfolio_engine();
    test_method_support_allocators();
    test_risk_gate();
    test_spot_distributional_utility_decision_wrapper();
    test_forecast_persistence_surprise_and_calibration();
    test_spot_rebalance_router();
    test_portfolio_ledger_and_belief_reporter();
    std::cout << "wikimyei observer portfolio tests passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "test failed: " << ex.what() << "\n";
    return 1;
  }
}
