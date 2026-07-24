# Synthetic continuous graph v2

This benchmark is the fresh-seed successor to
`synthetic_continuous_graph_v1`. It exists to separate three questions that
became entangled in v1:

1. Does the source contain a causally forecastable sequential signal?
2. Does the representation preserve enough of that signal?
3. Can the forecasting readout learn to use the preserved signal?

V2 starts from one deterministic daily latent market and derives its 3-day
and 1-week channels by real, non-overlapping causal OHLCV aggregation. The
channels therefore share a coherent clock without repeating the same
bar-index waveform at three different timestamp spacings.

The original preregistration declared 4,096 production graph anchors and the
following fixed disjoint ranges:

| Purpose | Anchor range |
|---|---:|
| train | `[0,2496)` |
| purge | `[2496,2560)` |
| validation | `[2560,2816)` |
| purge | `[2816,2880)` |
| certified development evaluation | `[2880,3264)` |
| purge | `[3264,3328)` |
| final holdout | `[3328,4096)` |

`data/raw` contains the complete source required by the production loader.
`data/development_prefix` was generated to cover the intended first 3,264
anchors. A later clean Runtime reconstruction established that timestamp
synchronization accepts only 3,261 anchors from those bytes: the last accepted
anchor closes on master day 3,289, while the weekly bar beginning on day 3,290
is the required future/coarse boundary rather than another anchor. The
operational certified-development range is therefore `[2880,3261)` (381
anchors / 3,429 probe rows). Development-only tools must refuse `data/raw`,
use the sealed isolated mirror of this prefix, and bind both the immutable
cursor correction and its metadata erratum.

The immutable experiment contract is
[`FRESH_SEED_PREREGISTRATION.md`](FRESH_SEED_PREREGISTRATION.md). Generated
manifests and closure receipts live under `artifacts/`. Creating the source,
building its caches, and sealing the closure are preparation steps; they do
not authorize final-holdout exposure. A future final run requires a separate
one-shot ledger created only after the candidate checkpoint and every input
hash have been fixed.

## Development evidence

The sealed raw-data gate passed. A train-only order-24 ridge autoregression
reached `1.0` direction and pairwise rank accuracy on both validation and
certified development, with correlation effectively `1.0` and an RMSE /
target-RMS ratio of about `0.000098`. The report is
`artifacts/synthetic_v2_data_predictability_baselines.v1.report`.

The representation-bypass isolation also passed the capability question. The
production `ChannelContextMdn` trained directly on causal raw-return history
reached about `0.98` direction/rank and `0.995` correlation through its
mixture expectation. Its primary K=3 direct output reached about
`0.97` direction/rank and `0.99` correlation, missing the deliberately strict
complete gate only on RMSE calibration (an RMSE / target-RMS ratio of
`0.269-0.271` versus the `0.25` cutoff). The report is
`artifacts/synthetic_v2_raw_history_supervised_isolation.v1.report`.

These two results rule out an unforecastable source and a generally incapable
production MDN. They do not yet establish representation quality.

## Representation/interface diagnosis

The remaining procedure is fixed by:

- [`REPRESENTATION_READOUT_DIAGNOSTIC_PREREGISTRATION.md`](REPRESENTATION_READOUT_DIAGNOSTIC_PREREGISTRATION.md)
- [`REPRESENTATION_READOUT_DIAGNOSTIC_PROTOCOL_AMENDMENT.md`](REPRESENTATION_READOUT_DIAGNOSTIC_PROTOCOL_AMENDMENT.md)
- [`REPRESENTATION_INTERFACE_LOCALIZATION_ADDENDUM.md`](REPRESENTATION_INTERFACE_LOCALIZATION_ADDENDUM.md)
- [`REPRESENTATION_ABLATION_PREREGISTRATION.md`](REPRESENTATION_ABLATION_PREREGISTRATION.md)
- [`REPRESENTATION_CONDITIONAL_CERTIFICATION_AMENDMENT.md`](REPRESENTATION_CONDITIONAL_CERTIFICATION_AMENDMENT.md)
- [`DEVELOPMENT_SOURCE_ISOLATION_AMENDMENT.md`](DEVELOPMENT_SOURCE_ISOLATION_AMENDMENT.md)
- [`ISOLATED_DEVELOPMENT_SOURCE_PROTOCOL.md`](ISOLATED_DEVELOPMENT_SOURCE_PROTOCOL.md)
- [`STAGED_EVALUATION_HARDENING_AMENDMENT.md`](STAGED_EVALUATION_HARDENING_AMENDMENT.md)
- [`DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_CORRECTION.md`](DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_CORRECTION.md)
- [`DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_METADATA_ERRATUM.md`](DEVELOPMENT_PREFIX_CURSOR_ALIGNMENT_METADATA_ERRATUM.md)

The first canonical representation completed 3,000 steps without geometric
collapse: effective rank was `25.7529 / 32`, latent standard deviation was
`0.362748`, and condition number was `58.1197`. Audit then found that its
production config inherited the preparation registry and constructed the
full canonical `data/raw` source domain, contrary to the physical-prefix
rule. That checkpoint and its associated production MDN run are quarantined;
they cannot supply the staged route or a certified result. Both must be rerun
from step zero against a newly sealed runtime mirror containing only the nine
development-prefix files.

Runtime exposes two distinct feature boundaries. The raw encoder probe has 96
values (base, quote, and difference at width 32). The post-MDN probe has 400
values after learned MDN preprocessing; the fixed primary affine uses its
first 384 values and excludes 16 edge-identity values. The evaluation reports
both trained boundaries plus a seed-17 untrained raw-encoder control, so an
encoder failure can be distinguished from damage in the MDN preprocessing
interface.

Evaluation is deliberately staged. Canonical train and validation probes are
captured first, and the raw-96 affine validation gate mechanically publishes
one immutable route. A strong pass permits one canonical certified capture;
a failure forbids that capture and instead invokes the predeclared four-arm
representation screen, whose validation-selected arm alone may receive the
single certified attempt. This ordering prevents certified development from
participating in representation selection.

No representation validation or certified feature capture has completed.
Because Runtime nevertheless constructed the 4,096-anchor raw source domain,
the V2 final interval is conservatively treated as operationally exposed and
cannot support independent final evidence. Clean train/validation/certified
development localization can continue on the isolated source with 3,261
accepted anchors; a future independent final claim requires a fresh dataset
and ledger.
