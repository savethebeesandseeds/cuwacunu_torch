# Fresh-seed forecast-isolation preregistration

Status: fixed before source generation, baseline scoring, training, or model
evaluation on 2026-07-16. No v2 outcome was inspected when this contract was
written.

## Purpose

The primary question is whether forecast failure on the synthetic benchmark
comes from the data, the representation, or the learned forecast readout.
V1 cannot answer that cleanly: its nominal 1d, 3d, and 1w files reused the
same bar-index waveform, and its final interval is conservatively consumed
after an infrastructure failure. V2 uses new source and graph identities, a
new deterministic process, and a new untouched final interval. No v1
checkpoint is eligible.

The ordered evidence ladder is:

1. raw-history model-free and fitted autoregressive controls;
2. supervised raw-history forecast head with no learned representation;
3. fixed probes on an untrained versus trained representation;
4. the production MDN/direct readout on the same trained representation.

Only train, validation, and certified-development ranges may be used while
choosing models or diagnosing failures. The final holdout is not part of this
work.

## Deterministic process commitment

Seed label:
`cuwacunu.synthetic_continuous_graph_v2.process.seed.2026-07-16.01`

SHA-256 commitment:
`7d22bae27c203eeec9fb2147d63d1cbe55d9de056db7c048f74e4798849ee227`

The first 64 commitment bits, `0x7d22bae27c203eee`, initialize SplitMix64.
Exactly four outputs are mapped from their top 53 bits into `[0,1)` and then
to phases in `[0,2*pi)`. The process has no stochastic innovations and no
drift.

For master day `t`,

```text
P = (17*sqrt(2), 19*sqrt(3), 23*sqrt(5), 25*sqrt(7)) days
s_k(t) = sin(2*pi*t/P_k + phase_k)
r_i(t) = sigma_i * (1 + 0.12*s_3(t))
         * sum_{k=0..2}(M[i,k] * s_k(t))

M = [[ 0.66,  0.25, -0.09],
     [-0.18,  0.63,  0.19],
     [ 0.24, -0.16,  0.60]]
sigma = (0.0070, 0.0065, 0.0060)
```

The instruments are `SYN2ALPHASYN2USD`, `SYN2BETASYN2USD`, and
`SYN2GAMMASYN2USD`, with initial prices 100, 112, and 87. Prices integrate the
daily log returns. High, low, volume, trades, and taker fields may depend only
on the current and preceding completed daily return. Three-day and weekly
bars are exact non-overlapping aggregates of the daily master path; they are
not separately generated waveforms.

The generator creates 4,137 master days and publishes 4,126 daily rows,
1,376 complete 3-day rows, and 591 complete weekly rows. With channel history
and future lengths `(30,1)`, `(10,1)`, and `(4,1)`, production timestamp
synchronization must yield exactly 4,096 accepted anchors. Anchor zero is
master day 29; anchor 4,095 is master day 4,124.

## Frozen exposure boundaries

| Range | Bounds | Permitted use |
|---|---:|---|
| train core | `[0,2496)` | fitting |
| purge 1 | `[2496,2560)` | none |
| validation | `[2560,2816)` | selection and debugging |
| purge 2 | `[2816,2880)` | none |
| certified development eval | `[2880,3264)` | one confirmation per frozen candidate; no refit |
| purge 3 | `[3264,3328)` | none |
| final holdout | `[3328,4096)` | prohibited until a separate one-shot ledger exists |

The generated development-prefix view contains exactly 3,294 1d rows, 1,098
3d rows, and 471 1w rows per instrument. Its maximum visible anchor is 3,263.
Baseline and diagnostic programs must report their maximum anchor and source
row read, reject canonical `data/raw` paths, reject symlinks, and reject an
evaluation end above 3,264.

Generating the full raw source is not an exposure: the generator computes no
forecast metric and makes no candidate decision. After source validation and
production cache construction, a closure receipt must bind all source,
prefix, configuration, generator, validator, cache, and executable hashes.
The nine cache chains must pass the strict freshness guard before and after
the source tree becomes read-only. No generator invocation is permitted after
that closure.

## Predeclared data-predictability gates

The primary raw-data capability arm is an order-24 ridge autoregression fitted
only on train-core daily log returns, with regularization chosen using train
data only. It is evaluated unchanged on validation and then certified
development evaluation. The zero, fitted-mean, lag-1, and train-selected-lag
arms are controls; an analytic process oracle is reported as an upper-bound
implementation check and is not the primary gate.

For each of validation and certified development evaluation, the order-24
arm must satisfy all of:

- finite predictions for every instrument and anchor;
- aggregate sign-direction accuracy at least 0.95;
- cross-instrument pairwise rank accuracy at least 0.95;
- aggregate Pearson correlation at least 0.95;
- RMSE no greater than 0.25 times the target RMS.

Failure means this fresh process is not an adequate forecast-isolation
benchmark under the declared history and fitting method. The representation
and MDN must not be blamed or trained on v2 until that failure is understood.

## Interpretation ladder after the raw gate

If the raw gate passes, subsequent experiments use the same fixed split and
report direction, rank, correlation, RMSE, target scale, prediction scale,
coverage, and exact maximum exposure.

- If a supervised raw-history head fails, investigate target construction or
  forecast optimization before the representation.
- If raw history succeeds but a fixed probe on the trained representation
  fails, the representation/interface discarded usable information.
- If a fixed probe succeeds but the learned production readout fails, the
  failure is in readout parameterization, objective, scaling, or optimizer
  dynamics.
- If both succeed but remain below the raw gate, representation capacity or
  invariance remains the upstream ceiling.

No result on validation or certified development evaluation is independent
final evidence. A final claim requires one frozen candidate, a separately
published exactly-once ledger for `[3328,4096)`, a cache-freshness pass before
ledger creation, and no retry if capture starts or the ledger is published.

