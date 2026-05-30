/*
  Frozen optimized observation profile for vicreg.solo.

  The promoted baseline medium-dock result uses the 1d + 4h + 1h observation
  surface, so the optimized bundle freezes that channel profile as part of the
  artifact identity.
*/
/---------------------------------------------------------------------------------------------------------\
|  interval   |  active   |  record_type |  seq_length | future_seq_length | channel_weight | normalization_policy |
|---------------------------------------------------------------------------------------------------------|
|    1w       |   false   |    kline     |     4       |         1         |      1.0       |  log_returns         |
|    3d       |   false   |    kline     |     14      |         1         |      1.0       |  log_returns         |
|    1d       |   true    |    kline     |     30      |         1         |      1.0       |  log_returns         |
|    12h      |   false   |    kline     |     28      |         1         |      1.0       |  log_returns         |
|    8h       |   false   |    kline     |     90      |         1         |      1.0       |  log_returns         |
|    6h       |   false   |    kline     |     28      |         1         |      1.0       |  log_returns         |
|    4h       |   true    |    kline     |     28      |         1         |      1.0       |  log_returns         |
|    2h       |   false   |    kline     |     24      |         1         |      1.0       |  log_returns         |
|    1h       |   true    |    kline     |     24      |         1         |      1.0       |  log_returns         |
|    30m      |   false   |    kline     |     100     |         1         |      1.0       |  log_returns         |
|    15m      |   false   |    kline     |     100     |         1         |      1.0       |  log_returns         |
|    5m       |   false   |    kline     |     100     |         1         |      1.0       |  log_returns         |
|    3m       |   false   |    kline     |     100     |         1         |      1.0       |  log_returns         |
|    1m       |   false   |    kline     |     100     |         1         |      1.0       |  log_returns         |
\---------------------------------------------------------------------------------------------------------/
