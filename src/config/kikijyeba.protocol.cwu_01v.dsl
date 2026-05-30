/*
  kikijyeba.protocol.cwu_01v.dsl
  =================================

  Baseline channel graph-first protocol.

  cwu_01v docks:

    Ujcamei graph source
      -> NodeLift SRL
      -> channel-preserving VICReg representation
      -> strict channel-context ExpectedValue MDN

  LEGACY WARNING:
    cwu_01v is retained as the VICReg baseline protocol. New training and
    evaluation should prefer cwu_02v unless an explicit baseline comparison is
    being run.
*/
PROTOCOL {
  PROTOCOL_ID = cwu_01v;
  PROTOCOL_KIND = channel_graph_first;
  PROTOCOL_STATUS = legacy;
  SUCCESSOR_PROTOCOL = cwu_02v;
  PROTOCOL_WARNING = cwu_01v is legacy baseline, prefer cwu_02v for new training and evaluation;
  GRAPH_TOPOLOGY = kikijyeba.topology.graph;
  NODELIFT = wikimyei.expression.nodelift.srl;
  REPRESENTATION = wikimyei.representation.encoding.vicreg;
  INFERENCE = wikimyei.inference.expected_value.mdn;
  REPRESENTATION_CONTRACT = graph_order.channel_node_representation.v1;
};
