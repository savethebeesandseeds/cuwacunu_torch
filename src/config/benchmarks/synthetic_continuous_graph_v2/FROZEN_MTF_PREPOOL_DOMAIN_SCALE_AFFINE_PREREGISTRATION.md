# Project Clear Signal — Test the Frozen Pre-Pool Domain×Scale Surface

Protocol/schema identity:
`synthetic_v2_frozen_mtf_prepool_domain_scale_affine_development_v1`.

This is a single, development-only causal diagnostic. It asks whether the
already frozen canonical MTF encoder contains the missing predictive signal in
its pre-serving-pool token surface when tokens are summarized separately by
channel, domain, and scale, and a fixed nine-head edge-by-channel affine map is
then fitted. It is not benchmark acceptance, certification, production
authorization, or policy evidence.

## Frozen scientific question

For each sample-node and channel, group the frozen encoder's token embeddings by
the exact `(domain_id, scale_id)` metadata cell, take the masked mean in each
cell, and flatten in this exact order:

`time_s0,time_s1,time_s2,time_s3,frequency_s0,frequency_s1,frequency_s2,frequency_s3`,

with latent coordinate `0..31` contiguous inside each group. The frozen shape is
`3 channels × 2 domains × 4 scales × 32 latent coordinates`, or 256 values per
node/channel. The generic edge serializer then emits
`base_256,quote_256,base_minus_quote_256`, or 768 float features per row.

The fixed evaluator fits one weight row and bias for each of the nine
`edge × channel` heads. It evaluates the exact ridge grid
`1e-12,1e-10,1e-8,1e-6,1e-4,1e-2` and selects one global ridge using the frozen
Phase 2A comparator: validation direction, rank, and correlation descending;
validation RMSE ascending; then smallest alpha, with tolerance `1e-12`.

## Immutable implementation authorities

The following exact regular, non-symlinked, root-owned, single-link files are
frozen before this preregistration is sealed:

- Capture source:
  `/cuwacunu/src/main/exec/cuwacunu_mtf_prepool_domain_scale_capture.cpp`,
  SHA-256
  `4d7c961129723f3983de17c2212a8ca4f1550327f472104d6369071d921aee54`,
  mode `0444`.
- Capture compile-only wrapper:
  `/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_mtf_prepool_domain_scale_capture.sh`,
  SHA-256
  `99fc145011c36671b6b8d55c1b546ec255454232a8c13ece5ea56fcb572a418f`,
  mode `0555`.
- Evaluator source:
  `/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/mtf_prepool_domain_scale_channel_conditioned_affine_probe.cpp`,
  SHA-256
  `ba13d95c4d33347cf4840f8eaa30616e095cf1c7dc3b0fa85de6a6c8f7c6f718`,
  mode `0444`.
- Evaluator compile-only wrapper:
  `/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_mtf_prepool_domain_scale_channel_conditioned_affine_probe.sh`,
  SHA-256
  `22d9047dfd8248fc65453744f1b4363ab5c4584b88574d7fa42e294754b9aaf6`,
  mode `0555`.

The evaluator binds the frozen nine-head Phase 2A source SHA
`5103e594a6096a325ac33b115594a739a0c3e3f0ad8d36b9fcf38d8ac8114570`
and canonical parser source SHA
`45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939`.

The compile wrappers bind the resolved GCC 12 driver, the capture wrapper binds
the sealed common and Torch wrapper archives, and both wrappers bind the eight
explicit LibTorch/CUDA link inputs before and after compilation. Their declared
scope is intentionally finite. Every preparation receipt and final result must
state:

`full_transitive_system_toolchain_or_elf_closure=false`.

The prepared binary hashes, not an overclaimed transitive build closure, are the
execution authority. Preparation is compile/link only and may not open the
canonical config, checkpoint, probes, source data, policy artifacts, or model.
The capture wrapper and evaluator wrapper run sequentially in the foreground,
each under its own 900-second timeout and 10-second TERM grace. Preparation
signals/timeouts stop and reap the active compile process group. Preparation
failure is non-attempt, publishes no scientific terminal receipt, and leaves no
partial prepared executable; successful build logs and the preparation receipt
are sealed read-only.

## Predecessor and canonical-input authorities

The immediate predecessor is the completed sealed-inventory V2 result:

`/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_sealed_raw96_edge_channel_affine_re_evaluation_development_v2/development.status`

with SHA-256
`c9c1f6248f99b00fd50b2b61d2c161abe321057690c37368f3e811568221484d`.
It must validate as complete with all seven unique pairs and all fourteen
evaluator calls complete, zero original-strong-gate passes, no terminal receipt,
and zero capture, encoder, checkpoint, model, MDN, or optimizer work. Its
execution lock is held shared throughout this attempt.

The canonical capture authority is
`synthetic_v2_frozen_feature_capture_isolated_v2`; its execution lock is held
shared. The exact capture config is

`/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/synthetic_benchmark.frozen_feature_capture.isolated.config`

with SHA-256
`eeea5620f1b271c0bd4527db6764c8f7b66eef5aced7b72d9d1b28d89443c9b3`.

The exact frozen encoder checkpoint is

`/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_train_isolated_v2/job/channel_representation.report.mtf_jepa_mae_vicreg.pt`

with SHA-256
`70919a6f76a1b461d5e46d91a936d2b94ffbc154b44c157e745653e1c460aa6d`.
Its representation-training execution lock is held shared.

The frozen isolated-source execution lock is also held shared. The worker binds
the exact development-source closure, cursor erratum, source manifest, and the
frozen config/DSL inputs already sealed by the canonical capture lineage before
it opens the config or checkpoint.

The historical all-token comparator probes are exact frozen regular,
non-symlinked, root-owned, single-link files under the canonical-capture shared
lock:

- train path
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/train/representation_edge_features.probe`,
  SHA-256
  `d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75`;
- validation path
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/validation/representation_edge_features.probe`,
  SHA-256
  `8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd`.

Their coordinate-plus-serialized-target projection digests are respectively
`f7a935fe83bb5e72388c63a4ab6e063d7908913f4cc52a5fcb652ead5e7dd08d`
and
`c1575a9936cca22f24c2e40908c8196c64bdfc7f89cdf15f70e822e92b16ec22`.

No config, checkpoint, probe, or source-data content may be opened by `--plan`,
`--prepare`, or the pre-attempt parent. Those reads begin only in the private
worker after the immutable attempt receipt has been published and are covered by
the single global timeout.

## Exact capture contract

There are exactly two capture processes and no capture replay:

1. train anchors `[0,2496)`, 2,496 anchors and 22,464 rows;
2. validation anchors `[2560,2816)`, 256 anchors and 2,304 rows.

Maximum anchor read is 2815. Each process loads the checkpoint once and performs
one encoder forward per fixed 64-anchor stream batch: 39 train calls and 4
validation calls. Each anchor participates in exactly one encoder pass.

The two exact command vectors are, with every flag appearing exactly once and no
additional arguments:

`CAPTURE --config CONFIG_ABS --input-representation-checkpoint CHECKPOINT_ABS --output-dir ABSENT_TRAIN_DIR_ABS --anchor-index-begin 0 --anchor-index-end 2496`

and

`CAPTURE --config CONFIG_ABS --input-representation-checkpoint CHECKPOINT_ABS --output-dir ABSENT_VALIDATION_DIR_ABS --anchor-index-begin 2560 --anchor-index-end 2816`.

The two output directories are distinct, absolute, newline-free, and absent at
launch. Launch receipts bind the literal argument vector before each process.

Each capture invocation emits exactly these three files in an initially absent,
private split directory:

- `all_tokens_reference.probe`;
- `prepool_domain_scale.probe`;
- `capture.report`.

The capture report schema is
`synthetic_v2_frozen_mtf_prepool_domain_scale_capture.v1`. A complete report has
exactly 65 unique newline-terminated `key=value` records and sorted-key SHA-256
`a66a0a942371694b7374e6be109b0581d87b13e795190b7191a9529f3f0b3ecd`.
All counters, paths, ranges, dimensions, access booleans, and formulas are
recomputed by the runner; report booleans are never trusted.

For each split, the same encode output must generate both artifacts. The emitted
all-token reference must be byte-identical to the exact historical comparator
probe. The coordinate-plus-serialized-target projection
`tail -n +2 -- PROBE | cut -d, -f2-10` must have the known split digest for the
historical probe, the emitted all-token reference, and the emitted pre-pool
probe. No projection payload is persisted.

The capture parses policy configuration only as an inert config dependency. It
constructs no policy or MDN model, opens no policy or MDN checkpoint, executes no
policy or MDN model, performs no optimization, writes no checkpoint, and does
not mutate any model parameter or buffer value after checkpoint loading.

## Exact affine contract

After both capture reports, both historical byte-parity checks, and all six
projection checks validate, run the prepared evaluator exactly twice:

`EVAL --probe-kind prepool_domain_scale --development-only --train-input TRAIN_ABS --validation-input VALIDATION_ABS --output ABSENT_ABS`

The first invocation is `main`; the second is `replay`. All flags occur exactly
once. All paths are absolute, distinct, and newline-free. Certified, final,
selection-lock, checkpoint, model, representation-forward, and policy flags are
forbidden. The evaluator creates its output exclusively.

The evaluator report schema is
`synthetic_v2_frozen_mtf_prepool_domain_scale_channel_conditioned_affine_development_v1`.
The all-six-candidates-valid complete report has exactly 249 unique
newline-terminated `key=value` records and sorted-key SHA-256
`cf47e8483e193eee90cd3fd52b90e30e63aa5e4a136bea4706969a4af6732811`.
The runner recomputes candidate selection, all selected metric copies, all
counters, the strong and partial gates, and classification. Main and replay must
both validate independently and then be byte-identical.

Per evaluator invocation the complete path has one train standardization, nine
cached ridge-invariant systems, six ridge attempts, 54 Cholesky factorizations,
54 solves, and no refit. Across main plus replay these are 18 systems, 12 ridge
attempts, 108 factorizations, and 108 solves.

## Gate, classification, and stop rule

The original validation strong gate is exact:

`direction>=0.95,rank>=0.95,correlation>=0.95,rmse_target_rms_ratio<=0.25`.

The partial gate `direction>=0.80,rank>=0.78` is diagnostic only.

After validated main/replay parity, classify exactly:

- strong gate true:
  `prepool_domain_scale_affine_strong_gate_observed_development_only`;
- otherwise:
  `prepool_domain_scale_affine_strong_gate_not_observed`.

The run stops after this single frozen surface. A strong result is development
evidence only; it does not authorize certified/final data, benchmark acceptance,
policy use, production use, threshold changes, a new ridge grid, or post-hoc
tuning. A non-strong result closes this preregistered surface without silently
trying another summary.

The preregistered causal reading is deliberately narrow. A strong result is
evidence that the current cross-domain/cross-scale serving mean is the exposed
information-loss boundary, because the encoder, checkpoint, targets, splits,
and evaluator family are frozen and only the pre-pool domain-by-scale
separation changes. A non-strong result closes only this fixed domain-by-scale
masked-mean affine surface. It does not establish encoder failure: the summary
still collapses within-cell window order, and the downstream head remains
affine.

## One-shot lifecycle and counters

The scientific attempt is exactly one private foreground worker under one
5,400-second timeout and 30-second TERM grace. It runs, in order:

1. in-attempt authority/config/checkpoint/source validation;
2. train capture and report validation;
3. validation capture and report validation;
4. historical byte parity and projection validation;
5. evaluator main and complete-report validation;
6. evaluator replay and complete-report validation;
7. report byte parity, science-complete receipt, result candidate validation,
   and one final atomic result publication.

There is no retry, replay of the whole attempt, resume, checkpoint write,
capture replay, refit, early stopping, seed selection, hyperparameter search, or
validation-driven change to capture. The evaluator's fixed six-ridge validation
selection is the sole preregistered development selection.

The successful result records these exact current-attempt counters:

- worker invocations: 1;
- capture process starts/completions: 2/2;
- capture replay invocations: 0;
- checkpoint loads: 2;
- encoder forward calls: 43;
- encoder anchor participations: 2,752;
- emitted capture artifacts: 4 probes plus 2 reports;
- historical byte-parity checks: 2;
- projection split/artifact checks: 6;
- evaluator starts/completions/reports: 2/2/2;
- main/replay parity checks: 1;
- ridge-system builds: 18;
- analytic ridge attempts: 12;
- Cholesky factorizations/solves: 108/108;
- conditioned head solves: 108;
- optimizer fits/steps, refits, retries, early stops: 0;
- certified/final/policy-model/MDN-model access or execution: 0.

Before evaluator call 1, evaluator counters are zero. After a process is started
but before its immutable returned/report receipt is fully validated, terminal
actual counters for that process's internal work are `not_available`; planned
counts are never reported as actual. After a validated science-complete receipt
but before final result publication, exact science counters remain recoverable
from the immutable reports and receipt while `scientific_result_available=false`.

Signals and timeouts stop the whole worker process group, wait for reaping, and
then terminalize. A stale consumed attempt is terminalized under the exclusive
lock and is never restarted. Rejected report/result evidence is frozen and
hash-closed before terminal publication. The final development result is the
worker's literal last semantic commit.

## Modes

- `--plan`: read-only, non-consuming, and content-free with respect to config,
  checkpoint, probes, and source data.
- `--prepare`: compile/link only; creates and seals the two prepared binaries and
  preparation receipt, with zero scientific-input/model access.
- `--run-development`: requires valid preparation, publishes the sole attempt,
  and executes the bounded worker once.
- `--verify-development`: read-only verification of an already committed result
  or terminal bundle. It never compiles, captures, evaluates, retries, or
  repairs.
