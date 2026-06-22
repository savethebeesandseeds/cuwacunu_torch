/*
  Minimal active graph-first retrieval channels for the synthetic benchmark.

  Active basic channels are intentionally not used in v1 because the current
  graph-first NodeLift path supports active kline retrieval only.
*/
/------------------------------------------------------------------------------------------------------------------\
|  interval   |  active   |  record_type | input_length | future_length | channel_weight | normalization_policy |
|------------------------------------------------------------------------------------------------------------------|
|    1w       |   true    |    kline     |     4        |         1     |      1.0       |  log_returns         |
|    3d       |   true    |    kline     |     10       |         1     |      1.0       |  log_returns         |
|    1d       |   true    |    kline     |     30       |         1     |      1.0       |  log_returns         |
\------------------------------------------------------------------------------------------------------------------/
