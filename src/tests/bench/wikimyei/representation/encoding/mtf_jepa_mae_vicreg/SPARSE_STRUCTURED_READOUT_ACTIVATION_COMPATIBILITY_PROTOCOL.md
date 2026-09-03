# SRR-3R — Sparse Structured Activation Compatibility Gate Protocol

Status before execution: **sealed design; no SRR-3R downstream result
observed**.

## 1. Question and settled premises

This experiment resolves one boundary only: whether the historical frozen MDN
head can consume `structured_cdsb_sparse_v1`, whose public shape remains
`[B,3,32]` but whose feature semantics differ from `all_tokens`.

The following are settled and are not re-estimated:

- averaging every token within a channel destroys useful sequence information;
- `structured_cdsb_v1` was production-equivalent to the accepted SRR-1 shadow;
- SRR-4 repaired its sparse-surface coverage contract as the append-only
  `structured_cdsb_sparse_v1` policy;
- SRR-4 qualified the sparse policy's representation value and explicitly
  authorized a fresh frozen-head Stage A.

This protocol does not activate a policy, rewrite a checkpoint, train the
encoder or MDN, change augmentation, or open the final holdout. `all_tokens`
must remain the checked-in active policy and explicit rollback.

## 2. Frozen authorities

- Base config, 4,287 bytes:
  `src/config/benchmarks/synthetic_continuous_graph_v1/synthetic_benchmark.config`
  - SHA-256: `7c84cee94ecf839336c0383878298981b9ab362e80a570cefef20e9fed272fd6`
- Evaluation config, 4,298 bytes:
  `src/config/benchmarks/synthetic_continuous_graph_v1/srr3_activation_compatibility.config`
  - SHA-256: `23d94f9222527fcc14cbc3948861b42e06b0f55c992f8e3e01b8ebc1bd8149e0`
  - it differs from the base only by the certified replay runtime wave;
- Active DSL, 639 bytes:
  `src/config/wikimyei.representation.mtf_jepa_mae_vicreg.dsl`
  - SHA-256: `68015c25d689227141ef62c94b8d0aa01549787f7454b0396ee4fee5c8aa61ba`
  - its serving policy must be exactly `all_tokens`;
- Qualified production readout header:
  `src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h`
  - SHA-256: `0664d062914971af58424037f92569fc0828ffc3bc2b240bb589670d69164b88`;
- Qualified policy parser:
  `src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg_spec.h`
  - SHA-256: `7d5e05da2fb64e2101074c9775b0209d69c831bb2d0f534a4c3ca15313e65d49`;
- Representation checkpoint, 853,867 bytes:
  `.runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001/channel_representation.report.mtf_jepa_mae_vicreg.pt`
  - SHA-256: `8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de`
- Historical MDN checkpoint, 3,227,665 bytes:
  `.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/channel_inference.report.channel_mdn.pt`
  - SHA-256: `eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e`
  - recorded input policy: `all_tokens`;
- SRR-4 protocol, 13,186 bytes:
  `SPARSE_SURFACE_STRUCTURED_READOUT_CONTRACT_REPAIR_PROTOCOL.md`
  - SHA-256: `a634a1ae386a5a0bebb10440cf0d45730e2dd652375aa3fb80a4d9277708ed30`;
- SRR-4 completion receipt, 406 bytes:
  `.runtime/benchmarks/synthetic_continuous_graph_v1/wikimyei.mtf_jepa_mae_vicreg.srr4_sparse_surface_contract_repair.v1/attempt_000001/completion.receipt`
  - SHA-256: `9058e77cf384ec33fbeced1d3aac2ac9288783be70fece8025f24cc5b9d6c8ac`
  - required decision: `sparse_structured_repair_qualified`;
  - required authorization: `fresh_srr3_stage_a=true`.
- SRR-4 representation-value report, 21,790 bytes:
  `.runtime/benchmarks/synthetic_continuous_graph_v1/wikimyei.mtf_jepa_mae_vicreg.srr4_sparse_surface_contract_repair.v1/attempt_000001/representation_value.report`
  - SHA-256: `47252fc1fc51ca8ab55db570e914a3c2f11d62bc3e6d5dc01359c4512d61fd9f`;
  - it is existing bounded head-recovery evidence, not a new SRR-3R result.
- Audited SRR-3 numerical primitive module:
  `evaluate_structured_readout_activation_compatibility.py`
  - SHA-256: `94d343284d3ce2d2272d31c6e5d24c8c34820356d5238434a85caecd8c423663`;
  - SRR-3R may import its loaders, metrics, bootstrap, and classifier only with
    explicit constant-drift guards; it must emit a new schema and policy.

Seed/RNG sentinel is `31`. Source order is sequential contiguous anchor index.
The runner must seal the protocol, production readout/parser surface, capture,
evaluator, runner, configs, checkpoints, SRR-4 authorization, and emitted
artifacts before endpoint inspection, then recheck them before and after a
byte-exact evaluator replay.

## 3. Stage A — frozen historical head, no training

### 3.1 Data and compute

- Historical confirmation anchors: `[760,1088)` only;
- 328 anchors, 1,312 node rows, 3,936 representation mask cells;
- 2,952 edge-by-channel feature and prediction rows per arm;
- source batch size at most 64 and at most six source batches;
- exactly one representation encode per source batch;
- no development capture and no access to `[1088,1170)`.

For each source batch, retain one encoder output object and pass that same
object sequentially through `all_tokens` and
`structured_cdsb_sparse_v1`. The public selector result for each arm must be
`[M,3,32]` plus boolean `[M,3]`. Both policies must report all 3,936 context
mask cells valid, with masks exactly equal. This readout-coverage requirement
is distinct from downstream prediction validity, which also includes the
frozen future mask. Persist exactly 2,952 prediction rows per arm and require
their valid bits to be paired and identical; report the observed valid count
rather than assuming every future target is valid.

The freshly captured feature probes must be byte-identical to the already
authenticated SRR-4 confirmation artifacts:

- `all_tokens`, 6,133,066 bytes, SHA-256
  `8f6f72b78c0708b5f23512ada4ca8536ea8818f8e2d3d9bc501401d8ab0ce3c7`;
- `structured_cdsb_sparse_v1`, 6,112,783 bytes, SHA-256
  `dfac215b73b08525dcba90d8891c8dede328ed99ec0117e2e2efaea6a5afbd73`.

### 3.2 Honest checkpoint identity and counterfactual feed

Construct the MDN once and load the historical checkpoint exactly once under
its saved `all_tokens` identity. Freeze parameters and buffers, keep evaluation
mode, and use the same loaded model sequentially for both arms. The receipt
must distinguish one successful weight load from identity-authentication
attempts and record two MDN forwards per source batch.

As a separate diagnostic, attempting to authenticate that same checkpoint
with expected policy `structured_cdsb_sparse_v1` must reject specifically on
serving-policy identity before weights are loaded and without changing model
bytes. Identity rejection is expected metadata safety and is not evidence
against the representation. The candidate computation is explicitly
`legacy_all_tokens_head_on_sparse_semantics`; it may not relabel, copy,
rewrite, or bypass the old checkpoint identity.

### 3.3 Pre-metric mechanics gate

Before predictions are interpreted, require all of the following:

- frozen config, checkpoint, policy, range, graph/node order, row/key/target
  order, and seed identities match;
- no more than one encoder call per batch and no more than six total;
- input tensors, converted data/masks, and the full retained encoded object are
  unchanged before, between, and after selectors using exact contract plus raw
  bytes, not value-only equality or a lossy digest;
- selector value/mask shape, dtype, device, layout, finiteness, exact invalid
  zeroing, and full/equal context coverage pass;
- the two production graph/MDN adapters preserve paired keys, indices, future
  values/masks, graph identities, targets, shapes, and 96-feature width;
- MDN outputs and sigma tensors are finite on their valid surface; prediction
  validity and coverage are identical between arms;
- representation and MDN parameter/buffer bytes, evaluation modes, CPU RNG,
  CUDA RNG, config bytes, and checkpoint bytes remain unchanged;
- the old identity loads once as `all_tokens`, sparse expected identity rejects
  safely, and the failed attempt changes no model byte;
- augmentation, optimizer steps, backward calls, checkpoint writes, policy
  activation changes, and final-holdout access are all zero/false;
- the two new feature hashes equal the frozen SRR-4 confirmation hashes.

Any failure yields `invalid_mechanics`, forbids endpoint inspection, and ends
SRR-3R.

### 3.4 Frozen endpoint evaluation

Primary endpoints are direct edge-return directional accuracy,
within-anchor/channel three-pair asset-rank accuracy, and RMSE. Pearson
correlation is an additional endpoint; best-asset agreement and valid coverage
are diagnostics.

Use 4,096 paired anchor-cluster bootstrap resamples with NumPy `PCG64`, seed
`8387496322364763509`, linear 2.5/97.5 percentiles. Report candidate-minus-
baseline deltas, except report candidate/baseline RMSE ratio.

Compatibility requires all three primary gates and identical finite coverage:

- direction-delta lower 95% bound `>= -0.02`;
- rank-delta lower 95% bound `>= -0.02`;
- RMSE-ratio upper 95% bound `<= 1.10`.

Frozen Stage-A material flags are:

- direction point gain `>= 0.02` and lower bound `> 0`;
- rank point gain `>= 0.02` and lower bound `> 0`;
- RMSE point ratio `<= 0.95` and upper bound `< 1`;
- correlation point gain `>= 0.05` and lower bound `> 0`.

Correlation cannot rescue a failed compatibility gate. It remains a Stage-A
material flag only because that rule was frozen before this candidate result;
Stage B uses only the three primary endpoints.

Stage-A classification and stop gate:

- compatible plus any material flag:
  `frozen_head_compatible_and_useful`; stop, do not run Stage B;
- compatible with no material flag:
  `compatible_no_downstream_gain`; stop, do not run Stage B;
- any primary compatibility failure with mechanics otherwise passing:
  `frozen_head_incompatible`; authorize Stage B;
- mechanics failure: `invalid`; stop.

## 4. Conditional Stage B evidence — bounded head-only recovery

Stage-B evidence is forbidden from affecting the decision unless Stage A
returns exactly `frozen_head_incompatible`. SRR-4 already ran the same bounded,
equal-compute ridge A/B using this candidate, these features, splits, alpha
grid, confirmation rows, bootstrap, and gates. SRR-3R must hash-authenticate
and conditionally admit that sealed report; it must not present a deterministic
replay as a fresh experiment, capture or encode any row, or fit the same heads
again. This is pre-existing bounded head-recovery evidence, not independent
confirmation and not production-MDN training.

Frozen development artifacts `[0,730)`:

- `all_tokens`, 13,648,442 bytes, SHA-256
  `d3465e44ed15647e158b9cabf00f4b1670797fdddc5539c0dd4a067db7b193ed`;
- `structured_cdsb_sparse_v1`, 13,601,883 bytes, SHA-256
  `2d65315536246e56f35fc8981c5f0f1157770d25964050e5265951589fd508d1`.

Use the same confirmation feature hashes in section 3.1. The splits are
selection fit `[0,554)`, purge `[554,584)`, validation `[584,730)`, refit
`[0,730)`, confirmation `[760,1088)`, with `[1088,1170)` forbidden.

The imported experiment independently fit deterministic per-edge CPU-float64
ridge heads on the same 96 features. It standardized using fit-only population
standard deviation, kept an unpenalized intercept, selected the lowest
validation RMSE from
`[1e-10,1e-9,1e-8,1e-7,1e-6,1e-5,1e-4,1e-3,1e-2,1e-1,1,10]` with lower-alpha
tie breaking, and refit each selected arm on `[0,730)`. Its custody proves
paired rows/targets/splits and equal feature width, candidate count, solves,
refits, and bootstrap work. The common baseline-alpha comparison is diagnostic.

Use the Stage-A bootstrap contract. Stage B passes only if all three
noninferiority gates pass and at least two primary material flags pass:

- direction-delta lower bound `>= -0.01`;
- rank-delta lower bound `>= -0.01`;
- RMSE-ratio upper bound `<= 1.05`;
- material direction: point `>= 0.02` and lower `> 0`;
- material rank: point `>= 0.02` and lower `> 0`;
- material RMSE: point ratio `<= 0.95` and upper `< 1`.

## 5. Final decisions

- `safe_direct_activation`: only possible if an authenticated checkpoint
  already accepts the sparse policy identity. The frozen historical checkpoint
  is expected not to satisfy this, so no identity bypass may manufacture this
  decision.
- `activation_requires_versioned_head_checkpoint_migration`:
  - Stage A is `frozen_head_compatible_and_useful`; create a versioned
    checkpoint-identity migration while preserving the frozen head tensors, or
  - Stage A is incompatible and the authenticated bounded head-recovery
    evidence passes; train and validate a fresh/adapted versioned production
    head. A ridge pass does not itself constitute or authorize an MDN artifact.
- `downstream_bottleneck_remains_unresolved`:
  - Stage A is compatible but has no downstream gain, or
  - Stage A is incompatible and conditional Stage B fails.

The historical spelling `downstream_bottleneck_unresolved` is an alias only;
new SRR-3R artifacts must emit the canonical `..._remains_...` token.

If migration is recommended, SRR-3R does not write it. The next activation
step must create a new versioned artifact/SHA, preserve accepted tensor bytes
when reusing weights, authenticate under `structured_cdsb_sparse_v1`, reject
`all_tokens`, and reproduce the manually fed Stage-A predictions byte-exactly
before activation. `all_tokens` remains active and rollback throughout.

Augmentation attribution may begin only after this downstream decision is
recorded and the selected representation/head boundary is held fixed.
