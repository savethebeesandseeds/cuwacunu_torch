/*
  kikijyeba.protocol.cwu_01v.topology.graph.dsl
  =================================
  Purpose:
    CWU-01v active graph topology for graph-collated source evidence.

  Semantics:
    - Nodes are active asset nodes.
    - Edges are directed traded instruments from base_node to quote_node.
    - source_instrument must resolve to exact source rows with EDGE_SOURCE_KIND
      provenance for every active NodeLift channel.
    - For v1, edge_id must equal source_instrument.
    - This file declares the CWU-01v active market graph. Source
      availability lives in ujcamei.sources.dsl, runtime cursors live in
      dataloader/reporting code, and active temporal channel policy lives in
      kikijyeba.protocol.cwu_01v.settings.dock.channels.dsl.
    - EDGE_RESOLUTION_POLICY = explicit_only uses the edge table as authored.
      EDGE_RESOLUTION_POLICY = edge_discovery resolves active directed edges
      from exact Ujcamei source rows for all active channels.
    - EDGE_SOURCE_KIND = real keeps discovery on real exchange evidence.
    - FETCH_MODE controls graph-anchor edge fetch. parallel_by_edge preserves
      graph edge order while reading staged edge datasets concurrently once the
      batch has enough edge-anchor work items.
*/
GRAPH_POLICY {
  EDGE_RESOLUTION_POLICY = edge_discovery;
  EDGE_SOURCE_KIND = real;
  FETCH_MODE = parallel_by_edge;
  MAX_FETCH_WORKERS = 0;
  PARALLEL_MIN_WORK_ITEMS = 16;
};
/------------------------------------\
|  node_id  |  node_kind  |  active  |
|------------------------------------|
|   BTC     |    asset    |   true   |
|   USDT    |    asset    |   true   |
|   ETH     |    asset    |   true   |
\------------------------------------/
