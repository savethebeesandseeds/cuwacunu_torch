// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility {

struct spot_distributional_utility_spec_t {
  std::string version_token{
      "wikimyei.policy.portfolio.spot_distributional_utility.v1"};
  std::string component_assembly_id{"spot_distributional_utility_v1"};
  std::string input_belief_assembly_id{"nodelift_allocation_belief_v1"};
  std::string optimizer{"projected_gradient"};
  std::string objective{
      "scenario_log_growth_cvar_cost_concentration_uncertainty"};
  std::string scenario_unit{"arithmetic_return"};
  std::int64_t iterations{160};
  double learning_rate{0.05};
  double cvar_alpha{0.95};
  double scenario_growth_floor{1.0e-6};
  bool long_only{true};
  bool spot_only{true};
  bool require_accounting_numeraire_node{true};
  bool projection_validation_required{true};
  bool live_capital_allowed{false};
};

inline void validate_spot_distributional_utility_spec(
    const spot_distributional_utility_spec_t &spec) {
  if (spec.version_token !=
      "wikimyei.policy.portfolio.spot_distributional_utility.v1") {
    throw std::runtime_error("[spot_distributional_utility_spec] unsupported "
                             "VERSION");
  }
  if (spec.component_assembly_id.empty()) {
    throw std::runtime_error("[spot_distributional_utility_spec] "
                             "COMPONENT_ASSEMBLY_ID is required");
  }
  if (spec.input_belief_assembly_id.empty()) {
    throw std::runtime_error("[spot_distributional_utility_spec] "
                             "INPUT_BELIEF_ASSEMBLY_ID is required");
  }
  if (spec.optimizer != "projected_gradient") {
    throw std::runtime_error(
        "[spot_distributional_utility_spec] OPTIMIZER must be "
        "projected_gradient");
  }
  if (spec.objective !=
      "scenario_log_growth_cvar_cost_concentration_uncertainty") {
    throw std::runtime_error(
        "[spot_distributional_utility_spec] unexpected OBJECTIVE");
  }
  if (spec.scenario_unit != "arithmetic_return") {
    throw std::runtime_error(
        "[spot_distributional_utility_spec] SCENARIO_UNIT must be "
        "arithmetic_return");
  }
  if (!spec.long_only || !spec.spot_only ||
      !spec.require_accounting_numeraire_node) {
    throw std::runtime_error(
        "[spot_distributional_utility_spec] v1 is long-only, spot-only, and "
        "requires an accounting numeraire graph node");
  }
  if (!spec.projection_validation_required) {
    throw std::runtime_error("[spot_distributional_utility_spec] "
                             "PROJECTION_VALIDATION_REQUIRED must be true");
  }
  if (spec.live_capital_allowed) {
    throw std::runtime_error("[spot_distributional_utility_spec] "
                             "LIVE_CAPITAL_ALLOWED must be false for V1");
  }
  if (spec.iterations <= 0 || spec.learning_rate <= 0.0 ||
      spec.cvar_alpha <= 0.0 || spec.cvar_alpha >= 1.0 ||
      spec.scenario_growth_floor <= 0.0) {
    throw std::runtime_error(
        "[spot_distributional_utility_spec] invalid optimizer/scenario "
        "numbers");
  }
}

[[nodiscard]] inline spot_distributional_utility_spec_t
decode_spot_distributional_utility_spec_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(dsl_text, "SPOT_DISTRIBUTIONAL_UTILITY");
  spot_distributional_utility_spec_t spec{};
  spec.version_token = kv::required(block, "VERSION");
  spec.component_assembly_id = kv::required(block, "COMPONENT_ASSEMBLY_ID");
  spec.input_belief_assembly_id =
      kv::required(block, "INPUT_BELIEF_ASSEMBLY_ID");
  spec.optimizer = kv::required(block, "OPTIMIZER");
  spec.objective = kv::required(block, "OBJECTIVE");
  spec.scenario_unit = kv::required(block, "SCENARIO_UNIT");
  spec.iterations = kv::parse_i64(kv::required(block, "ITERATIONS"));
  spec.learning_rate = kv::parse_double(kv::required(block, "LEARNING_RATE"));
  spec.cvar_alpha = kv::parse_double(kv::required(block, "CVAR_ALPHA"));
  spec.scenario_growth_floor =
      kv::parse_double(kv::required(block, "SCENARIO_GROWTH_FLOOR"));
  spec.long_only = kv::parse_bool(kv::required(block, "LONG_ONLY"));
  spec.spot_only = kv::parse_bool(kv::required(block, "SPOT_ONLY"));
  spec.require_accounting_numeraire_node = kv::parse_bool(
      kv::required(block, "REQUIRE_ACCOUNTING_NUMERAIRE_NODE"));
  spec.projection_validation_required =
      kv::parse_bool(kv::required(block, "PROJECTION_VALIDATION_REQUIRED"));
  spec.live_capital_allowed =
      kv::parse_bool(kv::required(block, "LIVE_CAPITAL_ALLOWED"));
  validate_spot_distributional_utility_spec(spec);
  return spec;
}

} // namespace
  // cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility
