# Project Clear Signal - Exhaust the Sealed V2 Raw96 Affine Inventory

## Frozen identity and scope

Protocol and result schema:

    synthetic_v2_sealed_raw96_edge_channel_affine_re_evaluation_development_v1

Diagnostic phase:

    sealed_raw96_edge_channel_affine_re_evaluation

This is a one-shot, retrospective, development-only re-evaluation of the
already sealed compatible raw-96 train/validation probe pairs from the V2
serving-pool and representation-training ablation lineages. It asks only
whether the original nine-head edge-by-channel affine evaluator finds the
original validation strong gate in any pair.

The attempt performs no capture, representation or encoder forward, checkpoint
read, representation/MDN/policy model construction, optimizer training,
checkpoint write, refit, certified evaluation, final-holdout evaluation, or
policy evaluation. The only data operations are:

1. a fixed integrity-only coordinate-and-serialized-target projection over the
   seven sealed unique probe pairs; and
2. the frozen Phase 2A analytic affine evaluator.

The projection and all evaluator calls occur after publication of the sole
attempt, inside one foreground 900-second process-group timeout with a
10-second TERM-to-KILL grace. There is no retry, replay of the overall
attempt, resume, sweep, recapture, encoder execution, or replacement input.

## Frozen evaluator authority

The only scientific executable is the already sealed Phase 2A binary:

- source:
  /cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_encoder_channel_conditioned_affine_probe.cpp
  at SHA-256
  5103e594a6096a325ac33b115594a739a0c3e3f0ad8d36b9fcf38d8ac8114570;
- shared parser/metric source:
  /cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_representation_affine_probe.cpp
  at SHA-256
  45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939;
- compile-only wrapper:
  /cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_frozen_encoder_channel_conditioned_affine_probe.sh
  at SHA-256
  284b6cd4cef37b7cb965d9e92f1f55a5ab0aa02743d9553b13a71c25c21e0324;
- sealed Phase 2A receipt:
  /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_encoder_channel_conditioned_affine_development_v1/development.status
  at SHA-256
  b611d3d3d9e2d1e198a2764b928886b647d5ee95211a89e584a49c4e05b7fbe5;
- sealed binary:
  /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_encoder_channel_conditioned_affine_development_v1/bin/frozen_encoder_channel_conditioned_affine_probe
  at SHA-256
  efc2ece40bb0ab727447d12fe388060c9573e2dafebc9df2a9889ac510ba647d.

Preparation copies those exact binary bytes into the private protocol runtime;
it does not compile or open a probe. The copied binary must retain the same
hash. Each invocation is exactly:

    EVALUATOR --probe-kind representation --development-only \
      --train-input ABS_TRAIN --validation-input ABS_VALIDATION \
      --output ABS_EXCLUSIVE_OUTPUT

Protected, certified, holdout, policy, selection-lock, checkpoint, model, and
additional input flags are forbidden.

The all-six-ridges-valid evaluator report has exactly 234 unique,
newline-terminated key=value records with nonempty keys. The six valid
candidate rejection-reason values are intentionally empty. The SHA-256 of its C-locale sorted key
stream, with one LF after every key, is:

    67084d03ed7c441d539fe97c50114d8a13d1e90b42530e9c52eddd7476bef5f1

A smaller report means at least one numerically invalid ridge candidate and is
not science-complete for this protocol. Main and replay are independently
validated against the full schema and then must be byte-identical.

## Closed source authorities

The serving authority is:

    /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mtf_serving_pool_replay_development_v1/development.status
    SHA-256 9fd91759db1665f534f5a2a304b19399991673a9bcb3ca2c380857e72858aee5
    lock /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mtf_serving_pool_replay_development_v1/.execution.lock

Its split capture reports are fixed at SHA-256
5079c1beaa7efcfcb7dc4b519097b68e8a4222a4847f697735e5699205214d57
for train and
439df8fe37b548005b4b6c3f50fd8e8ce9b104bd96a09a280edc68c07328c4f3
for validation.

The representation-ablation authorities are:

    /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_ablation_isolated_v2_retry3/development.status
    SHA-256 6b4a422d5045b1e2a6d4ffb47e28d750cbdcabdf8216eded4f8fef00d41d012d

    /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_ablation_isolated_v2_retry3/selection.status
    SHA-256 4df2a2a5596c5d9139f6ce24a5b588e3f44e89577926c699b2ea6f0f957747bb

    lock /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_ablation_isolated_v2_retry3/.development.lock

The canonical capture authority and alias import are:

    /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/development.status
    SHA-256 fce8c2383b5040d11ddc1ea9618d52316ea65803442b900d349327d431623fe6

    lock /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/.execution.lock

    /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_ablation_isolated_v2_retry3/arms/canonical/import.status
    SHA-256 dd7e07cd562bda53a2d48be60a19be169162f89dccc38feb9be1fee11f17ac25

All authority receipts, probe inputs, evaluator sources, copied binary, and
locks must be absolute, canonical, non-symlinked, root-owned, single-link, and
at their exact preregistered mode and hash. The three producer locks remain
shared-locked throughout the attempt.

## Closed inventory and exact order

Every input uses record schema
kikijyeba.synthetic.representation_edge_feature_probe.v1, the
base-32, quote-32, base-minus-quote-32 raw-96 layout, graph fingerprint
4133db527907a8e4, train anchors [0,2496) with 22,464 rows, and validation
anchors [2560,2816) with 2,304 rows. Maximum readable anchor is 2815.

The seven unique pairs are evaluated in exactly this order. No runtime
discovery, filtering, reordering, or metric-informed priority is permitted.

1. all_tokens, also attributed to logical alias ablation.canonical
   - train:
     /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mtf_serving_pool_replay_development_v1/capture/train/all_tokens.probe
     SHA d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75
   - validation:
     /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mtf_serving_pool_replay_development_v1/capture/validation/all_tokens.probe
     SHA 8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd
   - canonical alias train:
     /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/train/representation_edge_features.probe
     at the same train SHA
   - canonical alias validation:
     /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/validation/representation_edge_features.probe
     at the same validation SHA.

2. pool_time_tokens
   - train SHA 2c27b4983977bb6bfbf8f5449bd8f098d64bf825d12c0ff49ba82c59cee39656
     at serving capture/train/pool_time_tokens.probe;
   - validation SHA 1c9a4ef281d559d7cdd496ad71320ce131494ac36384713d9955a8f6f3cc8e1e
     at serving capture/validation/pool_time_tokens.probe.

3. pool_frequency_tokens
   - train SHA 58e0ab63547739156e1db4288126cf808adeb16f037153d9a61bf2268354ab52
     at serving capture/train/pool_frequency_tokens.probe;
   - validation SHA 27e79a43f8521d40a8011c97fce2d14e34d2b286a3473c8017867cdbb621f0b2
     at serving capture/validation/pool_frequency_tokens.probe.

4. pool_domain_balanced
   - train SHA 1a99c7f85978232b74138aa1cc17da56277396e7c222f730cbd9dac2b1ca6dd4
     at serving capture/train/pool_domain_balanced.probe;
   - validation SHA af790faff213f27076c9a6fc18e0ca5c8ecf55f0ec4eaf75b601031da1b6ef42
     at serving capture/validation/pool_domain_balanced.probe.

5. endpoint_scale
   - capture receipt SHA
     6db9e6cca7cdf60676af6e0af1dad66f5b8428a91a944e90a866b72af799b5b2;
   - train SHA d79a2ff6eb4c615ade6e8ab4d476a0f576d8e98ca69b940dacf154251472ee38;
   - validation SHA
     78c7e8139c6a3de5ea6d19fdfea2c50eb880c46ff406a7618835d6dc39e7c740.

6. time_only
   - capture receipt SHA
     1977584999c386c0eac025a244498f9a7b8da7e98e9607aa373ae1e741f1a433;
   - train SHA 601f493c2f4eb801ea963c30afccbc39b36c9d7ff256875eea0564873d4722ca;
   - validation SHA
     c44cd11c8e222197f970f15ccb7d373eb95ffcd1b7c6ae126ea7defae51275cc.

7. no_tf_alignment
   - capture receipt SHA
     0c9be7e9ccf0b8fe4e7cb2b5076f9fe430abdb57ea703a1637df05df8d425f52;
   - train SHA 4db8424b6deeea9bff4a96963ff54d3051fc6adf375cfa45aa4195b33e639af7;
   - validation SHA
     0517678e2c724a8ce4f47b95472b90809c8487770d917e87a39042c64d9e853f.

The last three probe paths are respectively under:

    /cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_ablation_isolated_v2_retry3/arms/ARM/capture/{train,validation}/representation_edge_features.probe

The all_tokens and canonical files must have the same respective hashes and
must be byte-identical. Canonical is a logical lineage alias only: it receives
the newly produced all_tokens main/replay report attribution and is not an
eighth execution.

## Fairness and integrity projection

Before evaluator call 1, all seven pairs must pass exact row-domain validation
and a frozen read-only projection. With LC_ALL=C and pipefail, for each probe:

    tail -n +2 -- PROBE | cut -d, -f2-10 | sha256sum

The projection omits the header and preserves exactly one newline per ordered
row. Every train projection must equal:

    f7a935fe83bb5e72388c63a4ab6e063d7908913f4cc52a5fcb652ead5e7dd08d

Every validation projection must equal:

    c1575a9936cca22f24c2e40908c8196c64bdfc7f89cdf15f70e822e92b16ec22

Only digest receipts are persisted; projection payloads are never stored.
Failure, signal, or timeout before the complete projection stage receipt is an
integrity failure with zero evaluator calls and no scientific result.

## Per-arm evaluator and selection contract

Each unique pair receives main and then replay. An arm is complete only after
both reports independently pass the exact 234-key schema, every one of the six
ridge candidates is numerically valid, and the reports are byte-identical.

The frozen ridge grid is 1e-12, 1e-10, 1e-8, 1e-6, 1e-4, 1e-2. Each candidate
fits nine independent edge-by-channel rows using train-only global feature
standardization and the float64 centered-Cholesky solver. Candidate selection
is recomputed rather than trusted: direction higher by more than 1e-12, then
pairwise rank higher by more than 1e-12, then correlation higher by more than
1e-12, then RMSE lower by more than 1e-12, then smaller ridge. No refit follows
selection.

The original validation strong gate is:

- selected directional accuracy at least 0.95;
- selected pairwise-rank accuracy at least 0.95;
- selected correlation at least 0.95; and
- selected RMSE/target-RMS ratio at most 0.25.

The original partial gate, direction at least 0.80 and rank at least 0.78, is
reported only. It never stops, passes, or authorizes anything.

The first fixed-order arm whose recomputed strong gate passes after main/replay
parity terminates the loop. The remaining suffix is explicitly
not_evaluated_due_to_strong_gate. The result makes no best-arm or full-ranking
claim in that branch.

If no arm passes, all seven pairs and all fourteen evaluator calls must complete.
Only then are arms ranked descriptively by selected validation direction,
pairwise rank, correlation, and lower raw RMSE, using the same 1e-12 absolute
tolerance and fixed arm order as the final tie-break. Ranking is descriptive,
not selection, acceptance, authorization, or evidence from a fresh split.

## Result and interpretation

The only valid scientific classifications, in order, are:

1. sealed_v2_raw96_edge_channel_affine_strong_gate_observed_development_only;
2. sealed_v2_raw96_edge_channel_affine_strong_gate_not_observed.

The first classification records the first fixed-order passing arm. It does
not say that arm is best, does not rank an incomplete prefix, and grants no
certified or benchmark-acceptance authority. The second requires complete
evaluation and descriptive ranking of all seven unique pairs.

Every result binds the full closed inventory, projection receipts, every
completed main/replay report and hash, all six candidate summaries, recomputed
selected ridge and gates, exact actual counters, skipped suffix, and canonical
alias attribution. It must state retrospective_development_only=true,
benchmark_acceptance_authority=false, certified_authorization_eligible=false,
fresh_confirmation=false, and successful_result_authorizes_next_stage=false.

Maximum successful counters are:

- logical arms: 8;
- unique pairs: 7;
- evaluator calls: 14;
- analytic ridge-candidate fits: 84;
- edge-by-channel head solves: 756;
- main/replay parity checks: 7;
- projection split checks: 14;
- capture, encoder, checkpoint, representation/MDN/policy model, optimizer,
  refit, certified, final, and policy counts: 0.

For a strong stop at unique position K, actual evaluator calls are 2K,
candidate fits 12K, head solves 108K, and parity checks K. Canonical alias
attribution makes represented logical-arm count K+1.

## One-shot lifecycle and terminal truth

The runner offers plan, prepare, run-development, and verify-development
modes. Prepare copies only the exact sealed evaluator binary and publishes a
readiness receipt. Run-development cannot compile or replace it.

The attempt is atomically published before projection or evaluator access. A
pre-existing attempt without a result is sealed terminally and never resumed.
Signals and timeout stop the process group, wait and reap it, freeze partial
evidence, and publish one terminal receipt. Main/replay candidates and a
validated result candidate are never silently discarded.

Terminal receipts distinguish planned maxima from actual validated evidence.
Counts derived from complete 234-key reports are exact. A launched evaluator
without a fully validated report has unknown analytic fit/head-solve counts;
planned values must never be substituted. Projection and completed-arm counts
come only from fully validated immutable stage receipts. If the science-complete
receipt exists but final result publication was interrupted, terminal state
retains exact science counters while scientific_result_available remains false.

The fully validated development result candidate is the worker's literal final
atomic commit. No fallible scientific action follows it. Verification is
read-only. A terminal result, successful result, or stale attempt closes the
identity permanently.
