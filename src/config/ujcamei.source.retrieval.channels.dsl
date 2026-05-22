/*
  ujcamei.source.retrieval.channels.dsl
  ====================
  Purpose:
    Ujcamei retrieval channel table. Each row defines how one Ujcamei source
    family is sampled and normalized into tensors when a protocol asks Ujcamei
    for graph-anchor batches.

  Table schema:
    interval            : timeframe key that must exist in Ujcamei sources
    active              : enable/disable this channel in the assembled payload
    record_type         : source decoder family expected for this interval
    input_length        : history window size (past context length)
    future_length       : target-side future window size for aligned features
    channel_weight      : reserved scalar weight; v1 requires 1.0
    normalization_policy: binary preprocessing preset applied during CSV -> BIN

  Semantics:
    - Active rows contribute features to graph-anchor source batch
      construction.
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
    - v1 runtime supports active kline retrieval channels only. Other
      record_type values may exist in the source registry for future/fallback
      authoring, but activating them must fail fast until their tensor contract
      is implemented.
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
    - This file is Ujcamei retrieval/channel configuration. It is not graph
      topology wiring and does not define node/edge topology.

  Supported values:
    active:
      true | false

    record_type:
      kline is the only currently supported active graph-first NodeLift record
      type. trade/basic may exist in source registry rows but should not be
      activated for this v1 path.

    normalization_policy:
      log_returns
        Required for the current NodeLift-compatible kline path.

      none
        Grammar-supported storage mode, but not valid for the current
        NodeLift-compatible graph-first path.

    channel_weight:
      1.0 or 1 only. Weighted channel contribution is not implemented.
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
