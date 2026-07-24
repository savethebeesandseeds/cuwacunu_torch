/*
  Synthetic kline source registry for synthetic_continuous_graph_v2.

  The v2 identities are disjoint from v1 so graph and checkpoint identity
  checks fail closed across benchmark generations. Source files and caches are
  benchmark-local under data/raw.
*/
CSV_POLICY {
  CSV_BOOTSTRAP_DELTAS = 128;
  CSV_STEP_ABS_TOL = 1e-7;
  CSV_STEP_REL_TOL = 1e-9;
};
DATA_ANALYTICS_POLICY {
  MAX_SAMPLES = 4096;
  MAX_FEATURES = 2048;
  MASK_EPSILON = 1e-12;
  STANDARDIZE_EPSILON = 1e-8;
};
SOURCE_DEFAULTS {
  SOURCE_ROOT = data/raw;
  KLINE_INTERVALS = 1w,3d,1d;
};
KLINE_SOURCE_SET {
  INSTRUMENT = SYN2ALPHASYN2USD;
  MARKET_TYPE = synthetic;
  VENUE = local;
  BASE_ASSET = SYN2ALPHA;
  QUOTE_ASSET = SYN2USD;
  SOURCE_KIND = synthetic;
};
KLINE_SOURCE_SET {
  INSTRUMENT = SYN2BETASYN2USD;
  MARKET_TYPE = synthetic;
  VENUE = local;
  BASE_ASSET = SYN2BETA;
  QUOTE_ASSET = SYN2USD;
  SOURCE_KIND = synthetic;
};
KLINE_SOURCE_SET {
  INSTRUMENT = SYN2GAMMASYN2USD;
  MARKET_TYPE = synthetic;
  VENUE = local;
  BASE_ASSET = SYN2GAMMA;
  QUOTE_ASSET = SYN2USD;
  SOURCE_KIND = synthetic;
};
/---------------------------------------------------------------------------------------------------------------------------------------\
|  instrument      |  interval  |  record_type  |  market_type  |  venue  |  base_asset  |  quote_asset  |  source_kind  |  source  |
|---------------------------------------------------------------------------------------------------------------------------------------|
\---------------------------------------------------------------------------------------------------------------------------------------/
