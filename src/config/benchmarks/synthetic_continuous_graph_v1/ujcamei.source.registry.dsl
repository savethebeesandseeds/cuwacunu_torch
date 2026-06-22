/*
  Synthetic kline source registry for synthetic_continuous_graph_v1.

  The first benchmark uses kline-shaped synthetic rows because graph-first
  Runtime currently supports active kline channels. The underlying close prices
  are deterministic continuous functions implemented by the benchmark generator
  script.
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
  INSTRUMENT = SYNALPHASYNUSD;
  MARKET_TYPE = synthetic;
  VENUE = local;
  BASE_ASSET = SYNALPHA;
  QUOTE_ASSET = SYNUSD;
  SOURCE_KIND = synthetic;
};
KLINE_SOURCE_SET {
  INSTRUMENT = SYNBETASYNUSD;
  MARKET_TYPE = synthetic;
  VENUE = local;
  BASE_ASSET = SYNBETA;
  QUOTE_ASSET = SYNUSD;
  SOURCE_KIND = synthetic;
};
KLINE_SOURCE_SET {
  INSTRUMENT = SYNGAMMASYNUSD;
  MARKET_TYPE = synthetic;
  VENUE = local;
  BASE_ASSET = SYNGAMMA;
  QUOTE_ASSET = SYNUSD;
  SOURCE_KIND = synthetic;
};
/---------------------------------------------------------------------------------------------------------------------------------------\
|  instrument      |  interval  |  record_type  |  market_type  |  venue  |  base_asset  |  quote_asset  |  source_kind  |  source  |
|---------------------------------------------------------------------------------------------------------------------------------------|
\---------------------------------------------------------------------------------------------------------------------------------------/
