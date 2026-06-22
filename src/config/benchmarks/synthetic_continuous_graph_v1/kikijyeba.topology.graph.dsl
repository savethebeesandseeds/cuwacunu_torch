/*
  Synthetic deterministic graph for benchmark policy experiments.

  Nodes are synthetic assets plus an accounting numeraire. Active directed
  edges are discovered from Ujcamei synthetic kline source rows for the active
  retrieval channels. The functions that generate the source prices are
  implemented by the benchmark generator script.
*/
GRAPH_POLICY {
  EDGE_RESOLUTION_POLICY = edge_discovery;
  EDGE_SOURCE_KIND = synthetic;
  FETCH_MODE = serial;
  MAX_FETCH_WORKERS = 0;
  PARALLEL_MIN_WORK_ITEMS = 16;
};
/----------------------------------------\
|  node_id    |  node_kind  |  active  |
|----------------------------------------|
|  SYNUSD     |    asset    |   true   |
|  SYNALPHA   |    asset    |   true   |
|  SYNBETA    |    asset    |   true   |
|  SYNGAMMA   |    asset    |   true   |
\----------------------------------------/
