/*
  ujcamei.source.registry.dsl
  ===================
  Purpose:
    Source registry for source ingestion. This table maps
    exact source signatures plus intervals to concrete dataset paths.

  Table schema:
    instrument   : symbol or logical source family (e.g., BTCUSDT, UTILITIES)
    interval     : timeframe key (e.g., 1m, 1h, 1d, 1w, constant, sine)
    record_type  : parser/decoder family (e.g., kline, basic)
    market_type  : market family (e.g., spot, synthetic)
    venue        : exchange/source venue (e.g., binance, local)
    base_asset   : base asset of the instrument signature
    quote_asset  : quote asset, or NONE for non-quoted synthetic sources
    source_kind  : provenance class: real, synthetic, or derived
    source       : absolute/relative file path to raw source file

  Semantics:
    - Rows are canonical source definitions consumed by the source dataloader.
    - KLINE_SOURCE_SET expands to canonical per-interval source rows using:
        SOURCE_ROOT/INSTRUMENT/INTERVAL/INSTRUMENT-INTERVAL-all-years.csv
    - interval + record_type are source facts. Graph-first channel docks may
      request a subset of these source families during pipeline assembly.
    - Missing files or invalid paths are rejected during loader setup.
    - DATA_ANALYTICS_POLICY is required (no silent runtime defaults).
    - source_kind is used by graph/NodeLift validation. Reverse edge pairs are
      accepted only when both directions are exact real source instruments.
    - The source table declares exact source facts and availability only.

  CSV_POLICY fields:
    CSV_BOOTSTRAP_DELTAS:
      Number of initial row deltas used to infer regular cadence.

    CSV_STEP_ABS_TOL / CSV_STEP_REL_TOL:
      Absolute/relative tolerances for cadence checks and rounded step
      validation during cache materialization.

  DATA_ANALYTICS_POLICY fields:
    MAX_SAMPLES:
      Maximum source samples considered by source-data analytics reports.

    MAX_FEATURES:
      Maximum feature columns considered by source-data analytics reports.

    MASK_EPSILON:
      Numeric threshold used when deciding whether masked/invalid values should
      be excluded from analytics.

    STANDARDIZE_EPSILON:
      Epsilon used by analytics standardization to avoid zero-variance division.

  Supported table values:
    record_type: kline | trade | basic
    source_kind: real | synthetic | derived
    interval: constant | sine | triangular | 1s | 1m | 3m | 5m | 15m |
      30m | 1h | 2h | 4h | 6h | 8h | 12h | 1d | 3d | 1w | 1M
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
  SOURCE_ROOT = ../../.data/raw;
  KLINE_INTERVALS = 1w,3d,1d,12h,8h,6h,4h,2h,1h,30m,15m,5m,3m,1m;
};
KLINE_SOURCE_SET {
  INSTRUMENT = BTCUSDT;
  MARKET_TYPE = spot;
  VENUE = binance;
  BASE_ASSET = BTC;
  QUOTE_ASSET = USDT;
  SOURCE_KIND = real;
};
KLINE_SOURCE_SET {
  INSTRUMENT = ETHUSDT;
  MARKET_TYPE = spot;
  VENUE = binance;
  BASE_ASSET = ETH;
  QUOTE_ASSET = USDT;
  SOURCE_KIND = real;
};
KLINE_SOURCE_SET {
  INSTRUMENT = ETHBTC;
  MARKET_TYPE = spot;
  VENUE = binance;
  BASE_ASSET = ETH;
  QUOTE_ASSET = BTC;
  SOURCE_KIND = real;
};
KLINE_SOURCE_SET {
  INSTRUMENT = UTILITIES;
  MARKET_TYPE = synthetic;
  VENUE = local;
  BASE_ASSET = UTILITIES;
  QUOTE_ASSET = NONE;
  SOURCE_KIND = synthetic;
};
/-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\
|  instrument  |  interval  |  record_type  |  market_type  |  venue  |  base_asset  |  quote_asset  |  source_kind  |  source                                                      |
|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|   UTILITIES  |  constant  |     basic     | synthetic     | local   | UTILITIES    | NONE          |  synthetic    |  ../../.data/raw/UTILITIES/constant/constant_value.csv   |
|   UTILITIES  |    sine    |     basic     | synthetic     | local   | UTILITIES    | NONE          |  synthetic    |  ../../.data/raw/UTILITIES/sine/sine_wave.csv            |
|   UTILITIES  | triangular |     basic     | synthetic     | local   | UTILITIES    | NONE          |  synthetic    |  ../../.data/raw/UTILITIES/triangular/triangular_wave.csv |
\-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------/
