/*
  observation_channels.dsl
  ========================
  Purpose:
    Channel policy table for observation feature assembly. Each row defines
    how one interval/record stream is sampled and normalized into tensors.

  Table schema:
    interval            : timeframe key that must exist in sources table
    active              : enable/disable this channel in the assembled payload
    record_type         : source decoder family expected for this interval
    seq_length          : history window size (past context length)
    future_seq_length   : forecast window size for aligned targets
    channel_weight      : scalar weight for channel contribution
    norm_window         : normalization window length (0 disables rolling norm)

  Semantics:
    - Active rows contribute features to model payload construction.
    - seq_length/future_seq_length control temporal slicing and label alignment.
    - norm_window controls per-channel normalization policy.
    - This file is declarative channel configuration, not topology wiring.
*/
/-----------------------------------------------------------------------------------------------------------\
|  interval   |  active   |  record_type |  seq_length | future_seq_length | channel_weight | norm_window |
|-----------------------------------------------------------------------------------------------------------|
|    1w       |   true    |    kline     |     4       |         1         |      1.0       |      0      |
|    3d       |   true    |    kline     |     10      |         1         |      1.0       |      0      |
|    1d       |   true    |    kline     |     30      |         1         |      1.0       |      0      |
|    12h      |   false   |    kline     |     60      |         1         |      1.0       |     60      |
|    8h       |   false   |    kline     |     90      |         1         |      1.0       |     90      |
|    6h       |   false   |    kline     |     100     |         1         |      1.0       |     100     |
|    4h       |   false   |    kline     |     100     |         1         |      1.0       |     100     |
|    2h       |   false   |    kline     |     100     |         1         |      1.0       |     100     |
|    1h       |   false   |    kline     |     100     |         1         |      1.0       |     100     |
|    30m      |   false   |    kline     |     100     |         1         |      1.0       |     100     |
|    15m      |   false   |    kline     |     100     |         1         |      1.0       |     100     |
|    5m       |   false   |    kline     |     100     |         1         |      1.0       |     100     |
|    3m       |   false   |    kline     |     100     |         1         |      1.0       |     100     |
|    1m       |   false   |    kline     |     100     |         1         |      1.0       |     100     |
\-----------------------------------------------------------------------------------------------------------/
