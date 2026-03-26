/*
  vicreg.solo big_span_v1 observation profile.

  This file freezes the observation surface for the current objective so
  architecture and jkimyei comparisons happen against one stable input dock.

  Current scope:
    - long-horizon representation learning
    - low data-rate multiscale bars
    - no minute-resolution expansion yet

  Active rows define the public VICReg docking boundary used by
  iitepi.contract.base.dsl:
    C = 3
    T = 30
    D = 9
*/
/---------------------------------------------------------------------------------------------------------\
|  interval   |  active   |  record_type |  seq_length | future_seq_length | channel_weight | normalization_policy |
|---------------------------------------------------------------------------------------------------------|
|    1w       |   true    |    kline     |     4       |         1         |      1.0       |  log_returns         |
|    3d       |   true    |    kline     |     10      |         1         |      1.0       |  log_returns         |
|    1d       |   true    |    kline     |     30      |         1         |      1.0       |  log_returns         |
|    12h      |   false   |    kline     |     60      |         1         |      1.0       |  log_returns         |
|    8h       |   false   |    kline     |     90      |         1         |      1.0       |  log_returns         |
|    6h       |   false   |    kline     |     100     |         1         |      1.0       |  log_returns         |
|    4h       |   false   |    kline     |     100     |         1         |      1.0       |  log_returns         |
|    2h       |   false   |    kline     |     100     |         1         |      1.0       |  log_returns         |
|    1h       |   false   |    kline     |     100     |         1         |      1.0       |  log_returns         |
|    30m      |   false   |    kline     |     100     |         1         |      1.0       |  log_returns         |
|    15m      |   false   |    kline     |     100     |         1         |      1.0       |  log_returns         |
|    5m       |   false   |    kline     |     100     |         1         |      1.0       |  log_returns         |
|    3m       |   false   |    kline     |     100     |         1         |      1.0       |  log_returns         |
|    1m       |   false   |    kline     |     100     |         1         |      1.0       |  log_returns         |
\---------------------------------------------------------------------------------------------------------/
