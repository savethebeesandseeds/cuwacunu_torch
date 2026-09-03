# SRR-3 — Frozen Downstream Compatibility Protocol

Status before execution: **sealed design; no endpoint result observed**.

## Fixed premises and exclusions

- Do not rerun SRR-1 representation probes, SRR-2 production parity, their
  bootstraps, or any long end-to-end training.
- Encoder weights, readout definitions, augmentations, source rows, ordering,
  checkpoints, metrics, gates, and the final holdout are immutable.
- Augmentations are disabled. There are no optimizer steps, backward calls, or
  checkpoint writes in Stage A or feature capture.
- The final anchor range `[1088,1170)` stays sealed.

## Frozen authority

- Base config:
  `src/config/benchmarks/synthetic_continuous_graph_v1/synthetic_benchmark.config`
  - SHA-256: `7c84cee94ecf839336c0383878298981b9ab362e80a570cefef20e9fed272fd6`
- Representation checkpoint:
  `.runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001/channel_representation.report.mtf_jepa_mae_vicreg.pt`
  - SHA-256: `8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de`
- MDN checkpoint:
  `.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/channel_inference.report.channel_mdn.pt`
  - SHA-256: `eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e`
- Baseline policy: `all_tokens`.
- Candidate policy: `structured_cdsb_v1`.
- Active production DSL must remain exactly `all_tokens` during the experiment.
- Seed/RNG sentinel: `31`.
- Source order: contiguous sequential anchor index.

The runner must seal the implemented capture/evaluator/auditor sources and all
loaded configuration dependencies before endpoint evaluation, and must record
SHA-256 manifests for inputs and outputs.

## Stage A — no training

Population: historical anchors `[760,1088)`, 328 anchors, three directed edges,
three channels, 2,952 paired edge/channel rows. A batch size of 64 permits at
most six encoder calls.

For every source batch:

1. Run the frozen encoder exactly once under evaluation/no-grad and retain its
   complete encoded result.
2. Call the public production selector on that same object for `all_tokens` and
   `structured_cdsb_v1`.
3. Pass both outputs through the same production graph and MDN adapters.
4. Load the MDN checkpoint once as `all_tokens`, keep every downstream weight
   frozen, and forward the two contexts sequentially through the same model.
5. Persist paired predictions and both 96-wide edge feature surfaces for replay;
   replay may not invoke the encoder.

The actual checkpoint identity diagnostic is separate from manual scientific
feeding: an `all_tokens` expected identity must load, while a
`structured_cdsb_v1` expected identity must reject with the production mismatch
guard. The rejection is not evidence against the representation.

### Hard pre-metric stop gates

Do not inspect task endpoints if any item fails:

- authority hashes or active DSL differ;
- an anchor is outside `[760,1088)` or row/key/target order differs by arm;
- encoder calls exceed one per retained source batch;
- policy outputs differ in shape, dtype, device, or mask contract;
- a valid value or downstream output is non-finite;
- invalid values are not zeroed under the same masks;
- graph order, target masks, valid-row coverage, parameters, buffers,
  evaluation mode, RNG state, or checkpoint bytes change;
- persisted feature/prediction replay hashes change;
- the production structured selector fails its minimal SRR-2 smoke contract;
- the real MDN checkpoint fails `all_tokens` identity authentication or is
  accepted under structured identity.

Record per arm: feature mean, standard deviation, L2 norm, per-channel
variance, valid coverage, paired cosine similarity, prediction mean/standard
deviation/range, and finite MDN sigma. These are diagnostics, not quality
claims.

### Precommitted task endpoints

Primary endpoints on the direct edge-return output:

- directional accuracy;
- within-anchor/channel pairwise asset-rank accuracy;
- return RMSE;
- return Pearson correlation.

Secondary endpoints: best-asset agreement and valid prediction coverage.

Uncertainty is computed only for these new downstream comparisons using 4,096
paired anchor-cluster resamples with seed `8387496322364763509`. Candidate-minus-
baseline deltas are used except for the candidate/baseline RMSE ratio. Rows are
never bootstrapped independently.

Compatibility requires all of:

- direction-delta lower 95% bound `>= -0.02`;
- rank-delta lower 95% bound `>= -0.02`;
- RMSE-ratio upper 95% bound `<= 1.10`;
- identical valid coverage and all downstream outputs finite.

A material-value flag is raised by any one of:

- direction point gain `>= 0.02` with lower bound `> 0`;
- rank point gain `>= 0.02` with lower bound `> 0`;
- RMSE point ratio `<= 0.95` with upper bound `< 1`;
- correlation point gain `>= 0.05` with lower bound `> 0`.

Stage-A classification:

- compatible plus at least one material flag: stop; frozen weights are
  scientifically compatible, but legacy identity still requires a versioned
  checkpoint migration;
- compatible without material value: stop and retain `all_tokens`;
- mechanics pass but compatibility fails: `frozen_head_incompatible`; authorize
  Stage B;
- mechanics failure: invalid experiment; stop without Stage B.

## Stage B — conditional equal-compute head-only A/B

Stage B runs only after `frozen_head_incompatible`. It tests whether a small
fresh downstream head can recover the structured representation's value; it is
not a production MDN retraining claim.

Frozen splits:

| Purpose | Anchors | Rows |
|---|---:|---:|
| selection fit | `[0,554)` | 4,986 |
| purge | `[554,584)` | 270 |
| validation | `[584,730)` | 1,314 |
| refit | `[0,730)` | 6,570 |
| confirmation | `[760,1088)` | 2,952 |

For each arm, fit deterministic CPU-float64 per-edge ridge heads to the same
96 features: base `[32]`, quote `[32]`, and base-minus-quote `[32]`. Use fit-only
standardization, an unpenalized intercept, and the fixed alpha grid
`[1e-10,1e-9,1e-8,1e-7,1e-6,1e-5,1e-4,1e-3,1e-2,1e-1,1,10]`. Select the
lowest validation RMSE; ties choose the lower alpha. Refit each arm on
`[0,730)`. Report a common-alpha sensitivity comparison using the baseline's
selected alpha. No arm receives extra features, trials, or compute.

Stage B passes only if all of:

- direction-delta lower 95% bound `>= -0.01`;
- rank-delta lower 95% bound `>= -0.01`;
- RMSE-ratio upper 95% bound `<= 1.05`;
- at least two Stage-A material-value flags among direction, rank, and RMSE.

A pass means `activation_requires_versioned_head_checkpoint_migration` with a
fresh/adapted head. Failure means `downstream_bottleneck_unresolved`; it does
not reverse the settled SRR-1/SRR-2 representation findings.

## Rollback and next boundary

No SRR-3 run changes the active `all_tokens` policy. Any later activation must
be opt-in, carry a structured policy/head checkpoint identity, and preserve an
explicit `all_tokens` rollback. Augmentation attribution begins only after this
readout/downstream decision is held fixed.

