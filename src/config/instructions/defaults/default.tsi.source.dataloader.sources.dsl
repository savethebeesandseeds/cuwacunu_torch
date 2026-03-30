/*
  default.tsi.source.dataloader.sources.dsl
  =======================
  Purpose:
    Source registry for observation ingestion. This table maps
    (instrument, interval, record_type) to concrete dataset paths.

  Table schema:
    instrument   : symbol or logical source family (e.g., BTCUSDT, UTILITIES)
    interval     : timeframe key (e.g., 1m, 1h, 1d, 1w, constant, sine)
    record_type  : parser/decoder family (e.g., kline, basic)
    source       : absolute/relative file path to raw source file

  Semantics:
    - Rows are canonical source definitions consumed by observation pipeline.
    - interval + record_type must align with channel declarations.
    - Missing files or invalid paths are rejected during config/loader setup.
    - This file declares availability, not model training intent.
    - DATA_ANALYTICS_POLICY is required (no silent runtime defaults).
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
/---------------------------------------------------------------------------------------------------------\
|  instrument  |  interval  |  record_type  |  source                                                     |
|---------------------------------------------------------------------------------------------------------|
|   BTCUSDT    |    1w      |     kline     |  /cuwacunu/.data/raw/BTCUSDT/1w/BTCUSDT-1w-all-years.csv     |
|   BTCUSDT    |    3d      |     kline     |  /cuwacunu/.data/raw/BTCUSDT/3d/BTCUSDT-3d-all-years.csv     |
|   BTCUSDT    |    1d      |     kline     |  /cuwacunu/.data/raw/BTCUSDT/1d/BTCUSDT-1d-all-years.csv     |
|   ETHUSDT    |    1w      |     kline     |  /cuwacunu/.data/raw/ETHUSDT/1w/ETHUSDT-1w-all-years.csv     |
|   ETHUSDT    |    3d      |     kline     |  /cuwacunu/.data/raw/ETHUSDT/3d/ETHUSDT-3d-all-years.csv     |
|   ETHUSDT    |    1d      |     kline     |  /cuwacunu/.data/raw/ETHUSDT/1d/ETHUSDT-1d-all-years.csv     |
|   ETHUSDT    |    12h     |     kline     |  /cuwacunu/.data/raw/ETHUSDT/12h/ETHUSDT-12h-all-years.csv   |
|   ETHUSDT    |    8h      |     kline     |  /cuwacunu/.data/raw/ETHUSDT/8h/ETHUSDT-8h-all-years.csv     |
|   ETHUSDT    |    6h      |     kline     |  /cuwacunu/.data/raw/ETHUSDT/6h/ETHUSDT-6h-all-years.csv     |
|   ETHUSDT    |    4h      |     kline     |  /cuwacunu/.data/raw/ETHUSDT/4h/ETHUSDT-4h-all-years.csv     |
|   ETHUSDT    |    2h      |     kline     |  /cuwacunu/.data/raw/ETHUSDT/2h/ETHUSDT-2h-all-years.csv     |
|   ETHUSDT    |    1h      |     kline     |  /cuwacunu/.data/raw/ETHUSDT/1h/ETHUSDT-1h-all-years.csv     |
|   ETHUSDT    |    30m     |     kline     |  /cuwacunu/.data/raw/ETHUSDT/30m/ETHUSDT-30m-all-years.csv   |
|   ETHUSDT    |    15m     |     kline     |  /cuwacunu/.data/raw/ETHUSDT/15m/ETHUSDT-15m-all-years.csv   |
|   ETHUSDT    |    5m      |     kline     |  /cuwacunu/.data/raw/ETHUSDT/5m/ETHUSDT-5m-all-years.csv     |
|   ETHUSDT    |    3m      |     kline     |  /cuwacunu/.data/raw/ETHUSDT/3m/ETHUSDT-3m-all-years.csv     |
|   ETHUSDT    |    1m      |     kline     |  /cuwacunu/.data/raw/ETHUSDT/1m/ETHUSDT-1m-all-years.csv     |
|   BTCUSDT    |    12h     |     kline     |  /cuwacunu/.data/raw/BTCUSDT/12h/BTCUSDT-12h-all-years.csv   |
|   BTCUSDT    |    8h      |     kline     |  /cuwacunu/.data/raw/BTCUSDT/8h/BTCUSDT-8h-all-years.csv     |
|   BTCUSDT    |    6h      |     kline     |  /cuwacunu/.data/raw/BTCUSDT/6h/BTCUSDT-6h-all-years.csv     |
|   BTCUSDT    |    4h      |     kline     |  /cuwacunu/.data/raw/BTCUSDT/4h/BTCUSDT-4h-all-years.csv     |
|   BTCUSDT    |    2h      |     kline     |  /cuwacunu/.data/raw/BTCUSDT/2h/BTCUSDT-2h-all-years.csv     |
|   BTCUSDT    |    1h      |     kline     |  /cuwacunu/.data/raw/BTCUSDT/1h/BTCUSDT-1h-all-years.csv     |
|   BTCUSDT    |    30m     |     kline     |  /cuwacunu/.data/raw/BTCUSDT/30m/BTCUSDT-30m-all-years.csv   |
|   BTCUSDT    |    15m     |     kline     |  /cuwacunu/.data/raw/BTCUSDT/15m/BTCUSDT-15m-all-years.csv   |
|   BTCUSDT    |    5m      |     kline     |  /cuwacunu/.data/raw/BTCUSDT/5m/BTCUSDT-5m-all-years.csv     |
|   BTCUSDT    |    3m      |     kline     |  /cuwacunu/.data/raw/BTCUSDT/3m/BTCUSDT-3m-all-years.csv     |
|   BTCUSDT    |    1m      |     kline     |  /cuwacunu/.data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv     |
|   UTILITIES  |    1w      |     kline     |  /cuwacunu/.data/raw/UTILITIES/1w/UTILITIES-1w-all-years.csv |
|   UTILITIES  |    3d      |     kline     |  /cuwacunu/.data/raw/UTILITIES/3d/UTILITIES-3d-all-years.csv |
|   UTILITIES  |    1d      |     kline     |  /cuwacunu/.data/raw/UTILITIES/1d/UTILITIES-1d-all-years.csv |
|   UTILITIES  |    12h     |     kline     |  /cuwacunu/.data/raw/UTILITIES/12h/UTILITIES-12h-all-years.csv |
|   UTILITIES  |    8h      |     kline     |  /cuwacunu/.data/raw/UTILITIES/8h/UTILITIES-8h-all-years.csv |
|   UTILITIES  |    6h      |     kline     |  /cuwacunu/.data/raw/UTILITIES/6h/UTILITIES-6h-all-years.csv |
|   UTILITIES  |    4h      |     kline     |  /cuwacunu/.data/raw/UTILITIES/4h/UTILITIES-4h-all-years.csv |
|   UTILITIES  |    2h      |     kline     |  /cuwacunu/.data/raw/UTILITIES/2h/UTILITIES-2h-all-years.csv |
|   UTILITIES  |    1h      |     kline     |  /cuwacunu/.data/raw/UTILITIES/1h/UTILITIES-1h-all-years.csv |
|   UTILITIES  |    30m     |     kline     |  /cuwacunu/.data/raw/UTILITIES/30m/UTILITIES-30m-all-years.csv |
|   UTILITIES  |    15m     |     kline     |  /cuwacunu/.data/raw/UTILITIES/15m/UTILITIES-15m-all-years.csv |
|   UTILITIES  |    5m      |     kline     |  /cuwacunu/.data/raw/UTILITIES/5m/UTILITIES-5m-all-years.csv |
|   UTILITIES  |    3m      |     kline     |  /cuwacunu/.data/raw/UTILITIES/3m/UTILITIES-3m-all-years.csv |
|   UTILITIES  |    1m      |     kline     |  /cuwacunu/.data/raw/UTILITIES/1m/UTILITIES-1m-all-years.csv |
|   UTILITIES  |  constant  |     basic     |  /cuwacunu/.data/raw/UTILITIES/constant/constant_value.csv   |
|   UTILITIES  |    sine    |     basic     |  /cuwacunu/.data/raw/UTILITIES/sine/sine_wave.csv            |
|   UTILITIES  | triangular |     basic     |  /cuwacunu/.data/raw/UTILITIES/triangular/triangular_wave.csv |
\---------------------------------------------------------------------------------------------------------/
