/*
  wikimyei.policy.portfolio.graph_node_allocation.dsl
  =======================================================
  Contract-only trainable policy surface for future graph-node allocation
  policies.

  V1 consumes a curated, causal policy_input_t and emits one unified target
  weight vector over the ordered graph-node action universe. The accounting
  numeraire is an ordinary graph node inside that vector, not a separate
  reserve output. PPO execution is intentionally not implemented here.
*/
GRAPH_NODE_ALLOCATION {
  VERSION = wikimyei.policy.portfolio.graph_node_allocation.v1;
  COMPONENT_ASSEMBLY_ID = graph_node_allocation_v1;
  POLICY_KIND = trainable_graph_node_allocation;
  POLICY_INPUT_SCHEMA = kikijyeba.environment.policy_input.v1;
  ACTION_ADAPTER = target_node_weights_simplex.v1;
  ACTION_SCHEMA = kikijyeba.environment.action.target_node_weights.v1;
  REWARD_CONTRACT = kikijyeba.environment.reward.post_execution_ledger_log_growth_cost_drawdown.v1;
  GRAPH_NODE_UNIVERSE_POLICY = fixed_ordered_target_node_universe;
  SCENARIO_INPUT_POLICY = allocation_belief_distributional_summaries_v1;
  RAW_MDN_INPUT_ALLOWED = false;
  PPO_IMPLEMENTED = false;
  LIVE_CAPITAL_ALLOWED = false;
};
