/*
  kikijyeba.topology.graph.dsl
  =================================
  Purpose:
    Kikijyeba protocol active topology for graph-collated source evidence.

  Semantics:
    - Nodes are active asset nodes.
    - Edges are directed traded instruments from base_node to quote_node.
    - source_instrument must resolve to exact source rows with EDGE_SOURCE_KIND
      provenance for every active NodeLift channel.
    - For v1, edge_id must equal source_instrument.
    - This file declares the Kikijyeba protocol active market graph. Source
      availability lives in ujcamei.source.registry.dsl, runtime cursors live in
      dataloader/reporting code, and active temporal channel policy lives in
      ujcamei.source.retrieval.channels.dsl.
    - EDGE_RESOLUTION_POLICY = explicit_only uses the edge table as authored.
      EDGE_RESOLUTION_POLICY = edge_discovery resolves active directed edges
      from exact Ujcamei source rows for all active channels.
    - EDGE_SOURCE_KIND = real keeps discovery on real exchange evidence.
    - FETCH_MODE controls graph-anchor edge fetch. parallel_by_edge preserves
      graph edge order while reading staged edge datasets concurrently once the
      batch has enough edge-anchor work items.

  GRAPH_POLICY fields:
    EDGE_RESOLUTION_POLICY:
      explicit_only
        Use only the edge table below. Every active edge must resolve to exact
        Ujcamei source rows for every active retrieval channel.

      edge_discovery
        Ignore any missing edge table and discover active directed edges from
        exact Ujcamei source rows matching the active nodes/channels.

    EDGE_SOURCE_KIND:
      real
        Source rows must be real exchange/venue evidence.

      synthetic | derived
        Accepted by grammar for future policy work, but not the current
        NodeLift production path.

    FETCH_MODE:
      serial
        Fetch graph edges sequentially.

      parallel_by_edge
        Fetch edge-anchor work items concurrently while preserving graph edge
        order in the resulting batch.

    MAX_FETCH_WORKERS:
      0 means runtime default/auto. Positive values cap worker count.

    PARALLEL_MIN_WORK_ITEMS:
      Minimum edge-anchor work items before parallel fetch is used.
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
