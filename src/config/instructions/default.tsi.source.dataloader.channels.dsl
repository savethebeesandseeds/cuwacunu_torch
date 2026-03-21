/*
  default.tsi.source.dataloader.channels.dsl
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
    normalization_policy: binary preprocessing preset applied during CSV -> BIN

  Semantics:
    - Active rows contribute features to model payload construction.
    - Tensor rank convention:
      past features are [B,C,T,D] when batched and [C,T,D] when unbatched.
      future features are [B,C,Hf,D] when batched and [C,Hf,D] when unbatched.
      C is active channel count, T comes from seq_length, Hf comes from
      future_seq_length, and D is the per-timestep feature width emitted by the
      record_type decoder.
    - Active rows also define the contract-facing observation docking shape.
      C equals the number of active rows.
      T equals the maximum seq_length across active rows.
      future_seq_length does not participate in current VICReg docking.
    - Contract registration fails fast if decoded active-row C/T disagree with
      the contract docking __variables consumed by VICReg network_design.
    - future_seq_length is still decoded and available for future target-side
      docking, but it is not enforced as part of the current VICReg input dock.
    - seq_length/future_seq_length control temporal slicing and label alignment.
    - normalization_policy controls per-channel binary preprocessing.
    - none keeps raw values in the cached .bin.
    - log_returns is the strict policy for cached normalization.
    - log_returns stores price fields as causal log-returns against the
      immediate previous on-grid valid record and masks rows that do not have
      that context.
    - volume/count-like fields use log1p(...). Out-of-domain numeric inputs are
      warned and clamped to finite boundaries.
    - This file is declarative channel configuration, not topology wiring.
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
