/*
  kikijyeba.protocol.cwu_01v.settings.dock.channels.dsl
  ===================================
  Purpose:
    CWU-01v source dock table. Each row defines how one Ujcamei source family
    is sampled and normalized into tensors for this Kikijyeba protocol.

  Table schema:
    interval            : timeframe key that must exist in Ujcamei sources
    active              : enable/disable this channel in the assembled payload
    record_type         : source decoder family expected for this interval
    input_length        : history window size (past context length)
    future_length       : target-side future window size for aligned features
    channel_weight      : reserved scalar weight; v1 requires 1.0
    normalization_policy: binary preprocessing preset applied during CSV -> BIN

  Semantics:
    - Active rows contribute features to CWU-01v source batch construction.
    - Tensor rank convention:
      past features are [B,C,Hx,Dx] when batched and [C,Hx,Dx] when unbatched.
      future features are [B,C,Hf,Dx] when batched and [C,Hf,Dx] when unbatched.
      C is active channel count, Hx comes from input_length, Hf comes from
      future_length, and Dx is the per-timestep feature width emitted by the
      record_type decoder.
    - Active rows also define the source-facing tensor shape.
      C equals the number of active rows.
      Hx equals the maximum input_length across active rows.
      future_length does not participate in current VICReg docking.
    - future_length is decoded and available for future feature batches.
    - input_length/future_length control temporal slicing and future alignment.
    - normalization_policy controls per-channel binary preprocessing.
    - none keeps raw values in the cached .bin.
    - log_returns is the strict policy for cached normalization.
    - log_returns stores price fields as causal log-returns against the
      immediate previous on-grid valid record and masks rows that do not have
      that context.
    - volume/count-like fields use log1p(...). Out-of-domain numeric inputs are
      warned and clamped to finite boundaries.
    - channel_weight is reserved for weighted channel contribution. The v1
      runtime rejects values other than 1.0 instead of silently ignoring them.
    - This file is Kikijyeba protocol dock configuration, not Ujcamei source
      identity and not topology wiring.
*/
/------------------------------------------------------------------------------------------------------------------\
|  interval   |  active   |  record_type | input_length | future_length | channel_weight | normalization_policy |
|------------------------------------------------------------------------------------------------------------------|
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
\------------------------------------------------------------------------------------------------------------------/
