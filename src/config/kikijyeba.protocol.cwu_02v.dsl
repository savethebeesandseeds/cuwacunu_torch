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
  REPRESENTATION_CONTRACT = graph_order.channel_node_representation.v1;
};
