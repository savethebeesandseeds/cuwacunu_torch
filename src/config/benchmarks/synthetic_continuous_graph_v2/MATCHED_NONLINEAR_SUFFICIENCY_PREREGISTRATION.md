# Project Clear Signal Phase 2: matched nonlinear sufficiency preregistration

Status: fixed on 2026-08-13 after the sealed Phase 1 serving-pool result was
inspected and before any Phase 2 model, metric, or report was produced.

This development-only diagnostic asks two questions in order:

1. Did the canonical affine test fail because one edge model was shared across
   all three channels?
2. If not, can the frozen `all_tokens` raw-96 interface support the same
   bounded nonlinear decoder that succeeds on matched raw history?

Phase 2 does not retrain or execute the representation, forecast with a
production MDN, select a serving policy, or authorize deployment.

## Fixed inputs and exposure boundary

The canonical representation inputs are the sealed `all_tokens` probes:

```text
train [0,2496)
  /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/
  synthetic_v2_frozen_feature_capture_isolated_v2/jobs/train/
  representation_edge_features.probe
  sha256=d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75

validation [2560,2816)
  /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/
  synthetic_v2_frozen_feature_capture_isolated_v2/jobs/validation/
  representation_edge_features.probe
  sha256=8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd
```

Both use record schema
`kikijyeba.synthetic.representation_edge_feature_probe.v1`, 22,464 train
rows, 2,304 validation rows, and the fixed feature layout
`base[0:32],quote[32:64],base_minus_quote[64:96]`. The frozen representation
checkpoint identity is
`70919a6f76a1b461d5e46d91a936d2b94ffbc154b44c157e745653e1c460aa6d`.

Only the physically isolated development source may supply the raw-history
control. Canonical `data/raw`, certified development `[2880,3264)`, final
holdout `[3328,4096)`, policy data, and policy/checkpoint artifacts are
forbidden. The maximum graph anchor read is 2815 and source order is
sequential. Phase 2 writes reports only: no representation, MLP, optimizer,
MDN, or policy checkpoint may be written.

Every example is keyed by the exact tuple
`(record_schema,anchor_key,anchor_index,anchor_local_index,edge_index,edge_id,`
`base_node_id,quote_node_id,channel_index)`. One joined row table is built
before either nonlinear arm. Both arms consume the same row order and the
same parsed `target_edge_close_return` tensor; coordinate and serialized
target columns must therefore be byte-identical between arms. Independently
reconstructing the target from the isolated source must agree within `1e-9`
absolute error or the experiment is invalid.

## Rung A: edge-by-channel affine decoder

Rung A changes only the canonical affine fit structure. It uses the sealed
`all_tokens` raw-96 features, the existing train-only global feature
standardization, float64 centered Cholesky solver, numerical residual limit
`1e-7`, and ridge grid:

```text
1e-12, 1e-10, 1e-8, 1e-6, 1e-4, 1e-2
```

For each ridge value, nine independent weight rows and biases are fitted, one
for each `(edge_index, channel_index)` pair. A single ridge value is shared by
all nine fits. Each candidate is scored by metrics aggregated over all
validation rows. Selection remains lexicographic: highest directional
accuracy, then pairwise-rank accuracy, then correlation, then lower RMSE,
then smaller ridge, with comparison tolerance `1e-12`. There is no refit.
Aggregate and per-channel train and validation metrics are reported.

The unchanged strong gate is the conjunction:

- directional accuracy `>= 0.95`;
- pairwise-rank accuracy `>= 0.95`;
- correlation `>= 0.95`;
- RMSE / target RMS `<= 0.25`.

Direction `>= 0.80` and rank `>= 0.78` remains partial evidence only. If
Rung A passes the strong validation gate, publish
`edge_channel_affine_sufficiency_established` and stop. Rung B must not run.

## Rung B: matched 96-feature MLP

Rung B is authorized only when Rung A completes validly and fails its strong
gate. It has two arms:

- `raw_history_96`: for each row, use the exact pre-encoder NodeLift normalized
  close-coordinate rows from the same isolated three-channel pipeline that
  supplied the representation input, not a manually reconstructed daily
  series. Take the base-node and quote-node histories for that channel and
  their difference. The NodeLift validity mask determines the 4, 10, and 30
  active values for channels 0, 1, and 2. Right-align each masked history in
  32 positions, zero-fill only the masked leading positions, and concatenate
  `base_32,quote_32,base_minus_quote_32` in that order. Mask and right-alignment
  parity with the pre-encoder NodeLift tensor must be asserted before fitting.
- `representation_raw96`: use the canonical sealed `all_tokens` raw-96 row
  unchanged.

The existing daily 29-lag raw-history audit remains an external
forecastability control only. Its manually assembled input, model, and metrics
do not enter either Phase 2 arm and cannot substitute for `raw_history_96`.

No validity flag, edge embedding, channel embedding, handcrafted forecast,
or additional coordinate enters either 96-feature vector. The selected output
head below supplies only the declared edge/channel conditioning.

Each arm standardizes its 96 input coordinates from train rows only, using a
standard-deviation floor of `1e-8`. Targets are standardized per
`(edge_index,channel_index)` from train rows only, also with floor `1e-8`, and
are returned to original units before metrics are computed.

The decoder is fixed and identical in both arms:

```text
input[96]
  -> Linear(96,128) -> GELU
  -> Linear(128,128) -> GELU
  -> Linear(128,9)
  -> select head channel_index * 3 + edge_index
```

All linear layers include bias. There is no dropout, normalization, residual
branch, mixture distribution, auxiliary loss, or direct identity input. Loss
is the unweighted mean squared error of the standardized scalar target.

Exactly six fits are allowed: two arms times seeds `31,47,73`. Each fit uses
CPU float32, deterministic algorithms, Adam with learning rate `1e-3`,
`beta1=0.9`, `beta2=0.999`, `eps=1e-8`, zero weight decay, batch size 64,
gradient-norm clipping at 5.0, and exactly 3,500 optimizer steps. For a given
seed, both arms must start from byte-identical parameter tensors and consume
the same precomputed sequence of train-row indices. Validation is not read by
the trainer and is evaluated only at the final step. There is no early
stopping, seed selection, hyperparameter search, retry, or refit. A failed or
non-finite fit makes Phase 2 incomplete; changing this schedule requires a
new preregistration.

The original Phase 1 metric definitions and strong gate apply independently
to every seed on globally aggregated validation rows. An arm passes only when
at least two of its three seeds pass all four strong thresholds. Per-seed,
median, aggregate, and per-channel metrics must all be reported; medians are
descriptive and never select a seed.

## Fixed interpretation

- Rung A passes: the pooled representation was linearly sufficient once
  edge/channel slopes were allowed; the prior failure was a readout-sharing
  defect. Do not run or claim nonlinear evidence.
- Raw history and representation MLP arms pass: nonlinear decodability of the
  frozen `all_tokens` raw-96 interface is established under this bounded model.
- Raw history passes and representation fails: the data, target, optimizer,
  and fixed MLP are mechanically capable, but usable information is not
  established at the frozen raw-96 interface. This supports information loss
  or distortion in the encoder/interface under the tested decoder; it does
  not prove that every encoded token or every possible nonlinear decoder has
  lost the signal.
- Representation passes and raw history fails: representation nonlinear
  decodability is established, but the raw-history control construction is
  invalid for causal attribution.
- Both MLP arms fail, or any integrity/finite check fails: the result is
  inconclusive and does not authorize representation retraining.

Relative improvements, partial gates, train-only fit, or a single successful
seed are descriptive only. No Phase 2 outcome authorizes certified/final
access, a serving-policy change, production MDN work, policy work, or another
experiment without a separately reviewed durable protocol.
