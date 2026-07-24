/*
  Active graph-first retrieval channels for synthetic_continuous_graph_v2.

  The channel geometry is initially held equal to v1 so the fresh data and
  graph identities are the controlled changes.
*/
/------------------------------------------------------------------------------------------------------------------\
|  interval   |  active   |  record_type | input_length | future_length | channel_weight | normalization_policy |
|------------------------------------------------------------------------------------------------------------------|
|    1w       |   true    |    kline     |     4        |         1     |      1.0       |  log_returns         |
|    3d       |   true    |    kline     |     10       |         1     |      1.0       |  log_returns         |
|    1d       |   true    |    kline     |     30       |         1     |      1.0       |  log_returns         |
\------------------------------------------------------------------------------------------------------------------/
