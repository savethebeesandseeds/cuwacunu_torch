/*
  kikijyeba.protocol.cwu_02v.dsl
  =================================

  Default channel graph-first protocol.

  cwu_02v docks the MTF-JEPA-MAE-VICReg representation family as the active
  representation provider. It is the preferred protocol for new training and
  evaluation runs; cwu_01v remains available as the VICReg baseline.
*/
PROTOCOL {
  PROTOCOL_ID = cwu_02v;
  PROTOCOL_KIND = channel_graph_first;
  PROTOCOL_STATUS = active;
  SUCCESSOR_PROTOCOL = ;
  PROTOCOL_WARNING = ;
  GRAPH_TOPOLOGY = kikijyeba.topology.graph;
  NODELIFT = wikimyei.expression.nodelift.srl;
  REPRESENTATION = wikimyei.representation.encoding.mtf_jepa_mae_vicreg;
  INFERENCE = wikimyei.inference.expected_value.mdn;
  OBSERVER = wikimyei.observer.belief;
  ALLOCATION_POLICY = wikimyei.policy.portfolio.spot_distributional_utility;
  POLICY_COMPONENT = wikimyei.policy.portfolio.graph_node_allocation;
  REPRESENTATION_CONTRACT = graph_order.channel_node_representation.v1;
};

NO_LOOKAHEAD_CONTRACT {
  CONTRACT_ID = cwu_02v_no_lookahead_artifact_provenance.anchor_v1;
  CERTIFICATE_SCHEMA = no_lookahead_artifact_provenance.v1;
  INFLUENCE_SCHEMA = no_lookahead_artifact_provenance.anchor_v1;
  FRONTIER_UNIT = accepted_anchor_index;
  SERVING_ORDER = representation,mdn,policy;
  VISIBILITY_POLICY = prior_generation_per_slice;
  DERIVED_ARTIFACT_RULE = inherit_parent_influence;
  CHECKPOINT_RULE = generation_manifest_required;
  PUBLISH_RULE = valid_from_anchor_gte_fit_end;
  BOOTSTRAP_POLICY = explicit_lane_only;
  RESEARCH_POLICY = smoke_or_research_not_promotable;
};
