# Matched nonlinear sufficiency corrected-control preregistration

Status: fixed on 2026-08-13 before any corrected-control source capture,
model fit, metric, or report.

```text
protocol_id=synthetic_v2_matched_nonlinear_sufficiency_corrected_control_development_v1
diagnostic_authority=development_only
benchmark_acceptance_authority=false
```

This is a new one-shot protocol. It does not resume or retry the consumed V1
attempt. Its only scientific change is correction of the raw NodeLift validity
rule. Representation inputs, splits, coordinates, targets, model, seeds,
optimizer schedule, metrics, gates, and interpretation remain fixed.

## Predecessor and entry authority

The predecessor is terminally invalid:

```text
path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/
     synthetic_v2_matched_nonlinear_sufficiency_development_v1/
     terminal.invalid.status
sha256=a83154f71bee7eda29d3de8e221e55c03b4a42742cdb485a01ebb00ac1f41237
classification=invalid_pre_fit_raw_control_capture_contract_failure
fits_completed=0
optimizer_steps=0
scientific_result_available=false
same_protocol_retry_allowed=false
new_protocol_required=true
```

The corrected protocol must verify that exact immutable receipt before any
work. The unchanged predecessor scientific contract is
`MATCHED_NONLINEAR_SUFFICIENCY_PREREGISTRATION.md` at SHA-256
`cbbf1d837aa741ed157beb2fbab5b01d6c6e004376e865b1f71f2732b46fa348`.
This document supersedes only its erroneous exact-mask-count sentence and its
consumed attempt identity. It must also verify the sealed Phase 2A receipt
`synthetic_v2_frozen_encoder_channel_conditioned_affine_development_v1/development.status`
at SHA-256
`b611d3d3d9e2d1e198a2764b928886b647d5ee95211a89e584a49c4e05b7fbe5`,
including `classification=edge_channel_affine_sufficiency_not_established`
and `rung_b_authorized=true`. Rung A is not repeated.

## Fixed inputs, splits, and exposure boundary

The representation arm uses the same sealed `all_tokens` probes byte for byte:

```text
train [0,2496)
path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/
     synthetic_v2_frozen_feature_capture_isolated_v2/jobs/train/
     representation_edge_features.probe
sha256=d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75
rows=22464

validation [2560,2816)
path=/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/
     synthetic_v2_frozen_feature_capture_isolated_v2/jobs/validation/
     representation_edge_features.probe
sha256=8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd
rows=2304
```

Their schema remains
`kikijyeba.synthetic.representation_edge_feature_probe.v1`, their feature
layout remains `base[0:32],quote[32:64],base_minus_quote[64:96]`, and the
frozen representation checkpoint identity remains
`70919a6f76a1b461d5e46d91a936d2b94ffbc154b44c157e745653e1c460aa6d`.
The representation is not constructed or executed.

Only the physically isolated development source may supply the corrected raw
control. The train and validation ranges above are exhaustive. Source order is
sequential and the maximum anchor read is 2815. Canonical `data/raw`, certified
development `[2880,3264)`, final holdout `[3328,4096)`, policy data, and all
policy/checkpoint artifacts are forbidden. No representation, MLP, optimizer,
MDN, or policy checkpoint may be written.

Every row remains keyed by the exact tuple
`(record_schema,anchor_key,anchor_index,anchor_local_index,edge_index,edge_id,`
`base_node_id,quote_node_id,channel_index)`. Coordinate columns and serialized
`target_edge_close_return` must be byte-identical between the corrected raw
probe and canonical representation probe. Both arms consume one joined table,
the same row order, and the same parsed target tensor. Independent target
reconstruction from the isolated source must agree within absolute error
`1e-9`; otherwise the protocol is terminally invalid.

The unchanged scalar target for row channel `c` is close coordinate 3 at the
single future step:

```text
target_edge_close_return =
  future[base_node,c,0,3] - future[quote_node,c,0,3]
```

Both future close masks must be true and both values finite. Target generation,
float serialization, and row omission policy are unchanged; neither arm may
reconstruct or independently modify the tensor used by the evaluator.

## Corrected raw-history rule

The retrieval capacities remain 4, 10, and 30 for channels 0, 1, and 2. They
define structural support inside the common NodeLift history axis `Hx=30`; they
do not define a guaranteed count of true NodeLift cells.

For channel `c`, let `capacity[c]` be 4, 10, or 30. For every anchor/node row:

1. A true close-coordinate mask is forbidden outside history indices
   `[30-capacity[c],30)`.
2. Inside that structural interval, the actual pre-encoder NodeLift
   `feature_mask` is authoritative and may be true or false. No exact true
   count is required.
3. Every true cell must contain a finite float32 value. Every false cell must
   contain the adapter's exact zero value. A true non-finite value, a nonzero
   false value, or a true mask outside structural capacity makes the protocol
   terminally invalid before fitting.
4. Construct a 32-value node history initialized to zero. Map history index
   `h` to output index `2+h`; copy the NodeLift value only when its actual mask
   is true and otherwise retain zero. This preserves the fixed right alignment
   without pretending that a masked cell is observed.
5. For each canonical row's selected `channel_index`, concatenate
   `base_32,quote_32,base_minus_quote_32`. The difference is elementwise after
   masking and zero fill. The result has exactly 96 float values.

No mask bit, validity count, edge embedding, channel embedding, handcrafted
forecast, or additional coordinate enters the model input. Masks govern
construction and auditing only.

The corrected raw record schema is fixed as
`kikijyeba.synthetic.raw_nodelift_edge_feature_probe.corrected_control.v1`.
The capture-report schema is
`synthetic_v2_raw_nodelift_edge_feature_probe_corrected_control_capture_v1`.
The report and probe are exclusive-create outputs; an existing output is an
integrity failure, not permission to append or resume.

### Mandatory observed mask summary

Before either arm fits, the capture must report, separately for train and
validation and for channels 0, 1, and 2:

- the minimum and maximum number of true close-coordinate cells over every
  `(anchor_index,node_index)` row;
- the configured structural capacity;
- a SHA-256 of the complete actual close-mask stream.

The required keys are
`close_mask_count.<split>.channel_<c>.min`,
`close_mask_count.<split>.channel_<c>.max`,
`close_mask_count.<split>.channel_<c>.capacity`, and
`close_mask_sha256.<split>`. Every minimum and maximum must satisfy
`0 <= min <= max <= capacity[c]`.

The mask-hash byte stream is fixed as UTF-8/ASCII with LF newlines and begins:

```text
synthetic_v2_nodelift_close_mask_v1
split=<train|validation>
anchor_range=[0,2496)
```

For validation, the third line is instead exactly
`anchor_range=[2560,2816)`. Angle brackets in the `split` line denote the one
selected literal and are not serialized.

It then contains one line for every tensor row in
`anchor_index,node_index,channel_index` lexicographic order:

```text
<anchor_index>,<node_index>,<channel_index>,<30 ASCII bits>\n
```

Bits are history indices 0 through 29 in order, with `1` for true and `0` for
false. Decimal indices have no leading zeros. No whitespace, carriage return,
or terminal field separator is permitted. The reported hash is over the exact
complete byte stream. This summary is audit metadata only and cannot select,
filter, weight, or modify rows.

## Mandatory data-free self-test

Before source access and before publishing the one-shot attempt receipt, the
frozen capture implementation must pass a deterministic, data-free in-memory
self-test. It must not open any probe, source, checkpoint, model, or policy
artifact. The self-test must cover all three capacities and at least:

- false structural padding;
- a false cell inside structural capacity, including the oldest cell that a
  log-return series can mask;
- multiple true finite cells and their exact placement in the 32-value output;
- exact base, quote, and elementwise-difference serialization;
- deterministic mask-stream ordering, min/max counts, and SHA-256;
- rejection of a true cell outside capacity;
- rejection of a nonzero value under a false mask; and
- rejection of a non-finite value under a true mask.

The self-test must emit an immutable receipt binding its source, binary,
expected cases, observed canonical-output hash, and `status=passed`. That
receipt's SHA-256 must be bound into the attempt receipt. A missing, mutable,
or failed self-test receipt forbids the attempt.

Self-test failure is automatically sealed as `terminal.invalid.status` with
`attempt_consumed=false`, zero source access, and zero fits; this protocol is
then retired and a new reviewed protocol is required. This conservative rule
prevents repairing a frozen implementation under the same identity.

## Fixed matched MLP experiment

There are exactly two arms:

- `raw_history_96`, constructed only by the corrected rule above;
- `representation_raw96`, the unchanged canonical sealed 96-feature row.

Each arm standardizes its 96 coordinates from train rows only with standard
deviation floor `1e-8`. Targets are standardized per
`(edge_index,channel_index)` from train rows only with floor `1e-8`, and are
returned to original units before metrics.

The identical decoder for both arms remains:

```text
input[96]
  -> Linear(96,128) -> GELU
  -> Linear(128,128) -> GELU
  -> Linear(128,9)
  -> select head channel_index * 3 + edge_index
```

All linear layers include bias. There is no dropout, normalization, residual
branch, mixture distribution, auxiliary loss, or direct identity input. Loss
is unweighted mean squared error of the standardized scalar target.

Exactly six fits are permitted: two arms times seeds `31,47,73`. A single
bounded evaluator invocation performs all six; replay would be an unauthorized
additional six fits. Every fit uses CPU float32, deterministic algorithms,
Adam with learning rate `1e-3`, `beta1=0.9`, `beta2=0.999`, `eps=1e-8`, zero
weight decay, batch size 64, gradient-norm clipping at 5.0, and exactly 3,500
optimizer steps. For each seed, both arms start from byte-identical parameter
tensors and consume the same precomputed train-row index sequence. Validation
is not read by the trainer and is evaluated only after the final step. There
is no early stopping, seed selection, hyperparameter search, retry, refit, or
checkpoint write. The whole post-attempt run has hard timeout at most 5,400
seconds.

## Metrics, gates, and classification

Metric definitions, comparison directions, finite checks, aggregation, and
pairwise ordering are byte-for-contract unchanged from the predecessor
preregistration SHA above. Mandatory metrics remain `count`,
`pairwise_rank_count`, `mae`, `rmse`, `target_rms`, `prediction_rms`,
`rmse_target_rms_ratio`, `directional_accuracy`, `pairwise_rank_accuracy`,
`best_asset_agreement`, and `correlation`.

The unchanged strong validation gate is the conjunction, on globally
aggregated rows for one seed:

- directional accuracy `>= 0.95`;
- pairwise-rank accuracy `>= 0.95`;
- correlation `>= 0.95`;
- RMSE / target RMS `<= 0.25`.

Direction `>= 0.80` and rank `>= 0.78` are partial evidence only. An arm passes
only when at least two of its three seeds pass all four strong thresholds.
Per-seed, median, aggregate, and per-channel train and validation metrics are
mandatory. Medians are descriptive and never select a seed.

After all integrity checks and exactly six completed finite fits, classification
is mechanical:

- both arms pass: `nonlinear_decodability_established`;
- raw passes and representation fails:
  `information_not_established_at_frozen_raw96_interface`;
- representation passes and raw fails:
  `representation_decodable_raw_history_control_invalid`;
- both fail: `inconclusive_both_mlp_arms_failed`.

The third outcome establishes representation decodability but does not support
a raw-versus-representation causal claim. The second supports information loss
or distortion at the tested frozen interface and bounded decoder, but does not
prove that every token or possible nonlinear decoder has lost the signal.

## One-shot mechanics and automatic invalidation

The runtime root is fixed to the protocol ID and must be pristine, canonical,
non-symlinked, and protected by an exclusive lock. Sources, build scripts,
binaries, preregistration, predecessor receipt, representation probes, isolated
config/source closure, self-test receipt, and build receipts must be bound by
exact SHA-256 before the attempt. The attempt is published atomically and can
occur once only.

After attempt publication, any nonzero exit, timeout, signal, integrity or hash
failure, row/coordinate/target mismatch, mask-contract failure, capture failure,
non-finite fit, wrong optimizer-step count, wrong fit count, protected access,
or incomplete report automatically publishes an immutable terminal-invalid
receipt. It must state the failing stage, command ordinal, log hashes, actual
captures/evaluator invocations/fits/optimizer steps, maximum possible anchor
read, and every protected-access flag. No resume or retry is allowed; a new
reviewed protocol is required.

Success requires immutable capture reports, raw probes, evaluator report, logs,
hash manifest, and `development.status`, followed by byte/hash verification.
The result remains development-only. No outcome authorizes certified/final
access, representation retraining, production MDN work, serving-policy change,
policy work, or deployment.
