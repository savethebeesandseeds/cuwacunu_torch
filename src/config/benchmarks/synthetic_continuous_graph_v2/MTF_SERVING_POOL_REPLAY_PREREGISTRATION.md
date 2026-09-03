# MTF serving-pool replay preregistration

Status: frozen scientific contract before Phase 1 scoring  
Goal: Project Clear Signal — Cuwacunu Forecast Recovery  
Scope: development-only, same-checkpoint representation-serving isolation

Contract schema:
`synthetic_v2_mtf_serving_pool_replay_development_v1`  
Fixed runtime root:
`/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mtf_serving_pool_replay_development_v1`

The public runner must freeze and hash this document, its own source, the
dedicated capture source and binary, and every evaluator artifact before it
publishes the attempt receipt. No anchor may be captured and no probe may be
parsed before those bindings exist.

## Question

Does the current all-token per-channel mean materially hide forecastable state
that is already present in the canonical frequency-enabled MTF representation?

This phase changes only the reduction applied after one encoded token tensor.
It does not optimize or modify representation weights, construct or execute an
MDN model, execute policy, or open a certified/final range.

## Immutable scientific authority

The runner must require the following exact regular, non-symlinked, immutable
development receipt:

- Path:
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/development.status`
- SHA-256:
  `fce8c2383b5040d11ddc1ea9618d52316ea65803442b900d349327d431623fe6`
- Schema:
  `synthetic_v2_frozen_feature_capture_isolated_v2.development.v1`
- Status: `complete`
- Required metadata: mode `0444`, owner `0`, link count `1`.

That receipt must contain and bind these exact authorities:

- Input receipt:
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/inputs.status`
  with SHA-256
  `fedbf63815d5806309ac4f6c469b379c685825e8ec83a3b9bf8250663f6e39b0`.
- Clean isolated-source closure:
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/development_source_closure.status`
  with SHA-256
  `0509045745e208493ca0d8ba44a2671a574166b87812af4511fe34951fa21cc7`.
- Cursor-alignment erratum receipt:
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/cursor_alignment_erratum.status`
  with SHA-256
  `c710a2bc35b3857d3f252ee8db52b1011d046194d1bc1b2d70c3d27d37404fc4`.
- Isolated source root:
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/source`.
- Frozen capture config:
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/synthetic_benchmark.frozen_feature_capture.isolated.config`
  with SHA-256
  `eeea5620f1b271c0bd4527db6764c8f7b66eef5aced7b72d9d1b28d89443c9b3`.
- Representation checkpoint:
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_representation_train_isolated_v2/job/channel_representation.report.mtf_jepa_mae_vicreg.pt`
  with SHA-256
  `70919a6f76a1b461d5e46d91a936d2b94ffbc154b44c157e745653e1c460aa6d`.
- Dedicated capture build receipt:
  `/cuwacunu/src/config/benchmarks/synthetic_continuous_graph_v2/MTF_SERVING_POOL_CAPTURE_BUILD_RECEIPT.status`
  with SHA-256
  `cc4c52d5a8cd6353ee30ba77330a90814eb09ffa8b754a8ae6a8a7bec3ea0df2`.
- Dedicated capture binary SHA-256:
  `34a1fbe5daa6c7868696abc3bf772236439d72181fe1a1a90b93d3b169390c59`.

The runner verifies the build receipt against the frozen capture source,
Makefile, core serving/identity headers, compiler dependency manifest, object,
and final executable before the attempt. This prevents a stale same-path
binary from consuming the one attempt.

The historical Retry3 `time_only` checkpoint is forbidden because it was
trained without frequency tokens and would confound training with serving.

The source closure must itself verify:

- isolated registry
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/config/ujcamei.source.registry.development_prefix.dsl`
  at SHA-256
  `54d87853a1d41facd54c24dc4031c2983e9cce40064a8ac7e793fe5fee77cf5c`;
- isolated base config
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_isolated_development_source_v1/config/synthetic_benchmark.isolated_development.config`
  at SHA-256
  `9d5bb23194c5a227ec91cf5882225a26a4f2b1f3f631c167810bd7f71314d7ab`;
- source manifest SHA-256
  `7cf41d721647579924620c9daf7e38931898ba28a02c71c38cc7cd6e3f6431fa`;
- `strict_cache_freshness=pass`, `source_tree_read_only=true`,
  `config_read_only=true`, `canonical_data_raw_access=false`, and
  `final_holdout_available=false`;
- 3,261 accepted and candidate anchors, maximum anchor index 3,260, no
  missing-edge/fetch skip, and no duplicate anchor.

The unchanged source/target dependencies recorded by that closure are fixed:

| Dependency | Required SHA-256 |
|---|---|
| V2 retrieval-channel DSL | `36bcb2d4430f9e18673829bc4945ce04715d0f7749608177b8cd6a519fd58feb` |
| V2 split DSL | `74bf90e6ef55dac297e8ee36184de2a81ea7827cc7af011c3c2d18815a4938a6` |
| active `cwu_02v` protocol DSL | `d8b3fd860028c0f074f7e5a326db56284c0c40e6126e00ded0e2ac9a15eb8f1c` |
| V2 graph topology DSL | `6469e3408ae12cd070b4e84a72c9f6fe98170d49313d6e758a194b5852d4f440` |
| NodeLift SRL DSL | `41e47be6f6c07f62953396875d8a5c607c03391a5c3778a0e729d634a16bbecc` |
| canonical MTF net | `df4398835b7eff3496ac8c20e7713b2d3d3a245754916c81b77271c696a08cda` |

### Declared MTF-DSL dependency drift

The immutable source closure recorded the live MTF component DSL at SHA-256
`48185ce863433e8ae83283db09f43e1382e8e5c0ca8942709b76c76717d6f5a5`.
The Phase 1 implementation adds the explicit serving-pool setting and has
pre-scoring SHA-256
`68015c25d689227141ef62c94b8d0aa01549787f7454b0396ee4fee5c8aa61ba`.
Its grammar has pre-scoring SHA-256
`ff6be58c9e70cafd906ffeac1a068f84fbd77bb9cbb7ac1a26e14e7e4d9e657a`.

This declared drift is not evidence that the all-token baseline stayed the
same. The runner must bind those two execution hashes before capture and must
require the new `all_tokens` probes to be byte-identical to the immutable
historical probes below. Any other config-dependency drift is forbidden.

## Frozen exposure boundaries

- Train range: `[0,2496)`, 2,496 anchors, 22,464 probe rows.
- Validation range: `[2560,2816)`, 256 anchors, 2,304 probe rows.
- Maximum permitted development anchor read: `2815`.
- Purged gaps `[2496,2560)` and `[2816,2880)` are not captured or scored.
- Certified development `[2880,3261)` is not captured, parsed, hashed, or
  scored.
- Final `[3328,4096)` is not captured, parsed, hashed, or scored.
- Canonical `data/raw`, `data/final`, every certified artifact root, and every
  quarantined pre-isolation/non-retry runtime are forbidden paths.

Every input path must be absolute and canonical (`realpath -e` equals the
declared path). Every path component must be non-symlinked. Input files must be
regular, non-writable, owner `0`, and single-link. The nine source receipts must
be unique exact tuples under the isolated source root and must name exactly the
expected three instruments by three intervals. Output paths must remain under
the fixed Phase 1 runtime root; no output may alias or overwrite an existing V2
artifact.

## Fixed serving arms

Each source batch is encoded once. All arms are derived from the identical
encoded token tensor, token mask, and metadata. Every arm preserves the existing
`[B,C,32]` representation and 96-wide `[base, quote, base-minus-quote]` edge
probe.

1. `all_tokens`
   - Existing behavior.
   - Mean of every valid token belonging to a channel.
   - Domains are implicitly weighted by valid token count.
2. `pool_time_tokens`
   - Mean of valid `domain_id=0` tokens within each channel.
3. `pool_frequency_tokens`
   - Mean of valid `domain_id=1` tokens within each channel.
4. `pool_domain_balanced`
   - Compute time-domain and frequency-domain means independently within each
     channel, then compute exactly `(time_mean + frequency_mean) / 2`.
   - Both domain masks must be valid for every sample/channel. Missing either
     domain is terminal-invalid; there is no single-domain fallback.

`pool_time_tokens` is deliberately distinct from Retry3 arm `time_only`: the
former changes serving for the canonical checkpoint; the latter retrained a
frequency-free representation.

## Exact capture and integrity contract

The dedicated capture process is invoked exactly twice: once for the exact
train range and once for the exact validation range. It loads the fixed
checkpoint once per invocation, hence twice total. Within each invocation each
source batch is encoded once and all four arms are materialized from that same
in-memory encoding. It may not construct an MDN module or read an MDN
checkpoint.

For each split, the runner must derive actual cursor coverage from emitted
batches, not from command-line arguments. It requires:

- first, last, and next-expected anchor indices to prove contiguous exact
  coverage with no gap, overlap, or duplicate;
- train minimum/maximum anchor indices `0/2495`;
- validation minimum/maximum anchor indices `2560/2815`;
- exactly nine ordered rows per anchor: three fixed edges by three channels;
- strictly increasing anchor keys across anchors;
- exact row counts stated above; and
- no row or cursor outside its declared split.

The fixed target is feature coordinate `3` (`close`). For every row:

```text
target_edge_close_return =
  future_close(base_node, channel) - future_close(SYN2USD, channel)
```

The coordinate/target projection is the ordered raw CSV fields 2 through 10,
with no header and one newline per row:

```text
anchor_key,anchor_index,anchor_local_index,edge_index,edge_id,
base_node_id,quote_node_id,channel_index,target_edge_close_return
```

The runner computes and records the SHA-256 of this exact projection for every
arm and split. All four projection hashes must be identical within a split
before any affine evaluator opens a probe.

The complete new `all_tokens` probe files, including header, features, order,
and serialized values, must be byte-identical to:

- historical train probe
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/train/representation_edge_features.probe`
  with SHA-256
  `d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75`;
- historical validation probe
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/validation/representation_edge_features.probe`
  with SHA-256
  `8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd`.

Those probes are bound by immutable manifests:

- train manifest SHA-256
  `d7dc64ab1d424160a30756bdadb449cb2ad27ce9788fbb184d13fdaf66526b6e`;
- validation manifest SHA-256
  `0b6d85705e478321ad285f784d09391ca1255664f24624c962d57523d75ed02c`;
- historical component-spawn fingerprint `5ba58d2de0fb7dcb`;
- historical protocol-contract fingerprint `d8a39dbf11f94332`; and
- historical graph-order fingerprint `4133db527907a8e4`.

Any authority, hash, path, mode, containment, source-receipt, split, cursor,
shape, row, projection, or historical-baseline mismatch makes the attempt
`terminal_invalid_integrity`. No metric from that attempt may be interpreted,
reported as a scientific negative, or used to choose later work.

## Fixed diagnostic evaluator

The exact evaluator authority is:

- frozen helper source
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/frozen_selection_sources/frozen_representation_affine_probe.cpp`
  at SHA-256
  `45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939`;
- frozen runner
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/frozen_selection_sources/run_frozen_representation_affine_probe_isolated_v2.sh`
  at SHA-256
  `ebdb5b52bd291c40d8d4742b65c6781351223d9e1dcfd51a8036638bf0bc0173`;
- frozen binary
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_affine_development_isolated_v2/bin/frozen_representation_affine_probe`
  at SHA-256
  `733841623165e1be1dbf76e82264022292b5c16825211696800fd5876cddad3f`.

It runs with `--development-only`, `probe_kind=representation`, and feature
width 96. Main and replay reports for each arm must be byte-identical.

- Standardization and fitting use train only.
- Ridge grid: `1e-12,1e-10,1e-8,1e-6,1e-4,1e-2`.
- A ridge candidate is invalid if float64 factorization fails, any coefficient
  is nonfinite, or maximum normalized solve residual exceeds `1e-7`.
- Within an arm, select on validation by direction, pairwise rank,
  correlation, then raw `selected.validation.rmse`, all with comparison
  tolerance `1e-12`, then smaller ridge.
- There is no post-selection refit.
- Across arms, compare the selected validation results by direction, pairwise
  rank, correlation, then lower raw RMSE with tolerance `1e-12`. An exact tie
  prefers `all_tokens`, `pool_time_tokens`, `pool_frequency_tokens`, then
  `pool_domain_balanced`.

## Fixed gates and development-only interpretation

The validation strong gate is the conjunction:

- directional accuracy `>= 0.95`;
- pairwise rank accuracy `>= 0.95`;
- correlation `>= 0.95`;
- RMSE / target RMS `<= 0.25`.

The partial gate is direction `>= 0.80` and rank `>= 0.78`. Partial is not
success.

Phase 1 may publish
`development_serving_pool_sufficiency_candidate` only when all integrity checks
above pass, the byte-identical historical `all_tokens` baseline fails the full
validation strong gate, and at least one alternate pool passes the full
validation strong gate. This is a development-only causal-isolation result; it
is not certified representation success.

If integrity passes but that conjunction does not, publish
`serving_pool_sufficiency_not_established`. Relative or partial improvement is
reported descriptively using the fixed comparator and may inform the next
development diagnostic, but it is not called a fix.

No Phase 1 classification—including a positive one—authorizes certified or
final access, production deployment, a default serving-policy change, MDN
training, policy work, or the next experiment. Each requires a separately
reviewed bounded milestone; certified access additionally requires explicit
user authorization.

## MDN and policy boundary

Static MDN and policy DSL files may be parsed only as inert dependencies of the
existing graph-first config-bundle loader. This limited parsing is not model
execution and must be disclosed in the receipt.

Forbidden actions are: constructing an MDN or policy module, reading an MDN or
policy checkpoint/state, creating an MDN or policy optimizer, invoking a model
forward/action/allocation path, reading or writing MDN/policy training or
inference artifacts, or computing an MDN/policy metric. The final receipt must
record at least:

```text
mdn_model_constructed=false
mdn_checkpoint_access=false
mdn_execution=false
policy_config_parsed_as_inert_dependency=true
policy_model_constructed=false
policy_checkpoint_access=false
policy_execution=false
policy_metric_access=false
```

## Exactly-once execution and stop contract

The public runner is fixed at
`src/scripts/benchmarks/synthetic_continuous_graph_v2/run_mtf_serving_pool_replay_v1.sh`
and exposes only `--plan`, `--run-development`, and `--verify-development`.
`--plan` and `--verify-development` are read-only.

The fixed attempt receipt is
`${runtime_root}/attempt.status`, with schema
`synthetic_v2_mtf_serving_pool_replay_development_v1.attempt.v1`,
`attempt_ordinal=1`, and `status=consumed`. After source/binary freezing and
preflight, it is atomically published mode `0444` before the first capture
child starts. The runner then executes in the foreground under
`${runtime_root}/.execution.lock`.

The 5,400-second ceiling covers the whole scientific run from attempt
publication through final receipt. Timeout sends TERM, waits at most 30
seconds, then sends KILL to the bounded child process group. No background or
detached job is started and no automatic retry occurs.

If the attempt receipt or any scientific artifact exists without a complete,
valid final development receipt, the attempt is permanently consumed and
terminal-invalid. It cannot resume, overwrite, or retry. Renaming the schema to
repeat the same scientific question is forbidden; a genuinely new attempt
requires a new preregistration fixed before any partial scientific value from
this attempt is inspected.

The runner seals regular artifacts mode `0444` and directories mode `0555`,
rejects symlinks/special entries/write bits, and publishes a hash-bound final
`${runtime_root}/development.status`. That receipt binds the attempt,
preregistration, runner/capture/evaluator sources and binaries, all authority
receipts, config dependencies, checkpoint, both split captures, all eight probe
files, coordinate/target projection hashes, historical baseline hashes, all
eight main/replay evaluator reports, selection, gates, classification, actual
cursor extrema, and all protected-access flags.

The production `cuwacunu_exec` binary and every sealed Retry1/Retry2/Retry3
artifact remain read-only and are not rebuilt, overwritten, or modified.
Completion stops after the final development receipt. `PROJECT_STATE.md` is
updated separately from that immutable receipt; the runner does not start or
authorize another experiment.
