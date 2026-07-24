/*
  Synthetic deterministic graph for synthetic_continuous_graph_v2.

  Active directed edges are discovered from the benchmark-local synthetic
  kline registry. Node order deliberately keeps the accounting numeraire first.
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
|  SYN2USD    |    asset    |   true   |
|  SYN2ALPHA  |    asset    |   true   |
|  SYN2BETA   |    asset    |   true   |
|  SYN2GAMMA  |    asset    |   true   |
\----------------------------------------/
