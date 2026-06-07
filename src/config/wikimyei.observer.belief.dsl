/*
  wikimyei.observer.belief.dsl
  ============================
  Deterministic observer settings for converting the current channel-context
  MDN output into a NodeLift-aware AllocationBelief.

  This component consumes marginal future NodeLift potential distributions. It
  must not treat raw node potentials as asset returns. The return projection is
  numeraire-relative: phi_asset - phi_reference, where the accounting
  numeraire is a graph node supplied by BasePolicy.
*/
OBSERVER_BELIEF {
  VERSION = wikimyei.observer.belief.nodelift_allocation_belief.v1;
  COMPONENT_ASSEMBLY_ID = nodelift_allocation_belief_v1;
  INPUT_MDN_ASSEMBLY_ID = mdn_v1;
  FEATURE_SEMANTICS = ujcamei_kline_registry;
  FEATURE_SEMANTICS_SOURCE = dock_coordinate_space;
  GRAPH_NODE_AXIS_POLICY = graph_node_axis_binding;
  BATCH_POLICY = single_anchor;
  CHANNEL_CONSENSUS = uniform_valid_channels;
  POTENTIAL_SEMANTICS = edge_log_return_lifted_potential;
  RETURN_PROJECTION = numeraire_relative_nodelift_projection;
  SCENARIO_UNIT = arithmetic_return;
  ACCOUNTING_NUMERAIRE_POLICY = graph_node_from_base_policy;
  COVARIANCE_COUPLER = gaussian_copula_shrinkage;
  SCENARIO_COUNT = 1024;
  PROJECTION_VALIDATION_REQUIRED = true;
  LIVE_CAPITAL_ALLOWED = false;
  EPS = 0.000000000001;
};
