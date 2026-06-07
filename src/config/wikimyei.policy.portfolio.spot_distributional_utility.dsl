/*
  wikimyei.policy.portfolio.spot_distributional_utility.dsl
  =========================================================
  Deterministic V1 portfolio allocation method.

  The method consumes AllocationBelief only. The accounting numeraire is not an
  invented scalar bucket: it must be a graph node supplied by the belief
  BasePolicy and included in the unified target-node weight vector.
*/
SPOT_DISTRIBUTIONAL_UTILITY {
  VERSION = wikimyei.policy.portfolio.spot_distributional_utility.v1;
  COMPONENT_ASSEMBLY_ID = spot_distributional_utility_v1;
  INPUT_BELIEF_ASSEMBLY_ID = nodelift_allocation_belief_v1;
  OPTIMIZER = projected_gradient;
  OBJECTIVE = scenario_log_growth_cvar_cost_concentration_uncertainty;
  SCENARIO_UNIT = arithmetic_return;
  ITERATIONS = 160;
  LEARNING_RATE = 0.05;
  CVAR_ALPHA = 0.95;
  SCENARIO_GROWTH_FLOOR = 0.000001;
  LONG_ONLY = true;
  SPOT_ONLY = true;
  REQUIRE_ACCOUNTING_NUMERAIRE_NODE = true;
  PROJECTION_VALIDATION_REQUIRED = true;
  LIVE_CAPITAL_ALLOWED = false;
};
