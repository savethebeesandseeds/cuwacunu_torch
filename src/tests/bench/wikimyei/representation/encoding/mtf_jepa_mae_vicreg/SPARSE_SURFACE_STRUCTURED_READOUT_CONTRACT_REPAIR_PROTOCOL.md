# SRR-4 — Sparse-Surface Structured Readout Contract Repair Protocol

Status before execution: **sealed design; no SRR-4 quality result observed**.

## 1. Scope and settled premises

This protocol repairs the complete-block validity failure found by SRR-3. It
does not reopen the accepted SRR-1 representation result or SRR-2 production
parity result. It does not test the MDN or authorize activation.

The baseline policy is `all_tokens`. The candidate is the new, append-only
policy `structured_cdsb_sparse_v1` at ordinal 5. The existing
`structured_cdsb_v1` implementation,
projection, name, ordinal, validity behavior, and accepted complete-block
outputs are immutable. The checked-in active DSL must remain `all_tokens`.

No optimizer, training loop, backward call, augmentation, checkpoint write,
MDN construction/forward, end-to-end run, or final-holdout access is allowed.

## 2. Frozen authority

- Graph-first evaluation config:
  `src/config/benchmarks/synthetic_continuous_graph_v1/srr3_activation_compatibility.config`
  - size: 4,298 bytes
  - SHA-256: `23d94f9222527fcc14cbc3948861b42e06b0f55c992f8e3e01b8ebc1bd8149e0`
- Representation checkpoint:
  `.runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001/channel_representation.report.mtf_jepa_mae_vicreg.pt`
  - size: 853,867 bytes
  - SHA-256: `8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de`
- Frozen legacy development feature probe `[0,730)`:
  `.runtime/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_frozen_feature_capture.v1/anchor_0_730/representation_edge_features.probe`
  - size: 13,648,442 bytes
  - SHA-256: `d3465e44ed15647e158b9cabf00f4b1670797fdddc5539c0dd4a067db7b193ed`
- Frozen legacy confirmation feature probe `[760,1088)`:
  `.runtime/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_frozen_feature_capture.historical_760_1088.v1/anchor_760_1088/representation_edge_features.probe`
  - size: 6,133,066 bytes
  - SHA-256: `8f6f72b78c0708b5f23512ada4ca8536ea8818f8e2d3d9bc501401d8ab0ce3c7`
- Seed/RNG sentinel: `31`.
- Bootstrap seed: `8387496322364763509` using NumPy `PCG64`.
- Source order: sequential, contiguous anchor index.
- Active policy and rollback: `all_tokens`.

Before endpoint evaluation, the runner must hash-seal the protocol, production
header, parser, capture source, evaluator, config dependencies, representation
checkpoint, and emitted probes. It must replay output hashes after evaluation.

## 3. Frozen `structured_cdsb_sparse_v1` contract

### 3.1 Unchanged layout and projection

V2 uses the exact v1 metadata plan and fixed `Q_psm` projection:

- input token layout `[B,72,32]`;
- three channels, 24 ordered tokens per channel;
- two domains, four scales, sixteen compact cells;
- frozen cell IDs
  `[0,0,1,1,1,2,2,3,4,5,6,7,8,8,9,9,9,10,10,11,12,13,14,15]`;
- cell cardinalities `[2,3,2,1,1,1,1,1,2,3,2,1,1,1,1,1]`;
- fixed projection shape `[768,32]`, hash and invariants inherited unchanged
  from v1, including the 24-block sum identity `sum_t Q_t = I_32`.

### 3.2 Partial-cell estimator

For batch row `b`, channel `c`, compact cell `j`, and latent feature `d`, let
`V_j` be the positions in the frozen cell, `m_t` the source token-valid bit,
and `e_t` the encoder token.

```text
s_j = OR_{t in V_j} m_t
z_j = sum_{t in V_j} m_t e_t / max(1, sum_{t in V_j} m_t)
```

If every token in a cell is valid, use the direct unmasked cell mean exactly as
v1 does. If `s_j=false`, set `z_j` exactly to zero. Repeat `z_j` into each
frozen position belonging to cell `j`; repeat `s_j` into its support bit. This
preserves the accepted cell geometry while making missing support explicit.

Upstream sample/channel validity remains authoritative. A zero vector without
real support is never marked valid.

### 3.3 Domain-scale-neutral completion and projection

Let `z_t` be the repeated cell value, `s_t` its repeated support bit, and
`g(t)` its frozen `(time/frequency domain, scale)` group. Each of the eight
domain-scale groups must contain at least one supported compact cell. Candidate
channel validity is therefore:

```text
v_bc = sample_valid_b AND upstream_channel_valid_bc
       AND ALL_{g in 8 domain-scale groups} (OR_{j in g} s_bcj)
```

For each group `g`, define its support-position-weighted neutral mean and its
completed slots:

```text
n_g = sum_{t: g(t)=g} s_t
mu_g = sum_{t: g(t)=g} s_t z_t / n_g
z'_t = s_t z_t + (1-s_t) mu_{g(t)}
r_partial = sum_t z'_t Q_t
```

An absent cell therefore receives only its own domain-scale group's neutral
baseline and no fabricated within-scale position contrast. Cell means are
weighted by their frozen position cardinality, exactly matching the accepted
24-position lift. The
implementation must expose internal source-token counts, expected counts,
cell-support bits, and repeated support counts for tests, so a partly observed
multi-token cell is never misreported as fully observed. These diagnostics do
not alter the public serving shape.

The public `valid_mask` means only that the readout is computable from at least
one tokenizer-valid compact cell in each required domain-scale group. It never
means that all source slots, windows, or observations are present. Token validity itself is
the existing tokenizer's binary any-observation window contract; SRR-4 does not
silently promote it to a coverage or confidence estimate.

No scalar inverse-coverage or residual-energy multiplier, pseudoinverse,
learned parameter, random draw, cross-domain/scale neutral mean, or
zero-as-observed convention is permitted. The unscaled neutral completion is
deliberately conservative: because the frozen projection has orthonormal
columns, it cannot amplify the flattened residual norm. No support-dependent
residual-energy rescaling is allowed.

This form has three mandatory algebraic properties:

- **constant preservation:** if every observed cell equals latent vector `x`,
  then the output is exactly `x` (within the dtype's deterministic arithmetic),
  regardless of the supported nonempty pattern in each group;
- **domain-scale-constant preservation:** if all observed cells in each group
  equal `x_group`, the sparse result is the same as projecting the corresponding
  complete domain-scale-constant field;
- **complete-block identity:** when all 24 source tokens are valid, select the
  direct complete projection path and require exact value/mask equality with
  `structured_cdsb_v1`. The complete path may not be reconstructed by an
  approximately equivalent rearrangement.

The public output remains `[B,3,32]` plus boolean mask `[B,3]`. The policy adds
no weights and consumes no RNG.

## 4. Mechanical qualification before metrics

Tests must cover all of the following on CPU float64 and float32, and CUDA
float32 when CUDA is available:

- append-only ordinal/name/parser surface; default and active policy remain
  `all_tokens`;
- rejected unknown policy and rejected unsupported layout/config;
- v1 goldens and all legacy policy goldens unchanged;
- exact sparse-policy/v1 equality for complete blocks, including metadata
  permutations;
- independent oracle parity for single missing token, partially supported
  multi-token cell, empty cells, and the actual H4/H10/H30
  masks, upstream-invalid sample/channel, and all-invalid input;
- exact constant and domain-scale-constant preservation for every supported
  pattern used by the tests; any missing domain-scale group must make the
  channel invalid and exactly zero;
- a perturbation in a supported cell is visible and a perturbation confined to
  an unsupported token/cell is invisible;
- value/mask shape, dtype, device, contiguity, finiteness, invalid-zeroing,
  repeated-call determinism, input purity, parameter/buffer purity, and RNG
  purity;
- production selector parity with the independent sparse-policy oracle;
- protocol fingerprint, dry-run report, and checkpoint identity automatically
  distinguish `structured_cdsb_sparse_v1`; no parallel identity field is
  allowed.

Complete-block equality with v1 is byte-exact. Where a sparse oracle follows an
identical CPU-float64 operation order, require byte identity; otherwise require
maximum absolute error `<= 1e-12`. Float32/CUDA results translated to the
CPU-float64 oracle require maximum absolute error `<= 2e-5`. Sparse constant
and domain-scale-constant canaries use these tolerances rather than an invalid
general promise of bit equality after 10- or 18-position reductions.

The real sparse-surface capture is a second mechanics gate. For each source
batch, invoke the frozen encoder exactly once, retain its encoded result, and
feed that same object sequentially through both policies. Require:

- byte-stable encoded object before/between/after selectors;
- exact paired row/key/target/order and graph fingerprint;
- identical `[M,3,32]` values contract and `[M,3]` mask contract;
- candidate mask exactly equal to `all_tokens` over every cell;
- all three channels valid at all 1,312 anchor/node positions in confirmation;
- actual H4/H10/H30 source-token support counts equal `10/14/24` per channel,
  while repeated compact-cell support equals `10/18/24`; each per-cell
  observed/expected count remains auditable;
- finite values and exact invalid zeroing;
- unchanged model parameters, buffers, eval mode, CPU RNG, CUDA RNG, input
  data, input mask, config bytes, and checkpoint bytes;
- no more than 12 encoder batches for `[0,730)` and six for `[760,1088)`;
- freshly captured legacy probes byte-identical to both frozen legacy hashes.

Any failure produces `invalid_mechanics`, forbids endpoint inspection, and
ends SRR-4.

## 5. Smallest representation-value experiment

Only after all mechanics pass, evaluate the two captured feature arms. The
probe surface is the existing 96-wide edge/channel input: base representation
`[32]`, quote representation `[32]`, and base-minus-quote `[32]`. This is a
bounded representation diagnostic, not production-head training.

Frozen splits:

| Purpose | Anchors | Rows |
|---|---:|---:|
| alpha-selection fit | `[0,554)` | 4,986 |
| purge | `[554,584)` | 270 |
| validation | `[584,730)` | 1,314 |
| refit | `[0,730)` | 6,570 |
| historical confirmation | `[760,1088)` | 2,952 |
| forbidden final holdout | `[1088,1170)` | not opened |

For each arm independently, fit deterministic CPU-float64 per-edge ridge
heads. Standardize from fit rows only using population standard deviation; use
an unpenalized intercept. Select the lowest validation RMSE from the fixed grid
`[1e-10,1e-9,1e-8,1e-7,1e-6,1e-5,1e-4,1e-3,1e-2,1e-1,1,10]`; exact ties
choose the lower alpha. Refit each arm on `[0,730)`. Give both arms identical
feature width/schema and equal compute, with paired targets, rows, splits,
candidate count, solves, and bootstrap rows. The feature values are expected to
differ by policy. Also report candidate predictions using the baseline-selected
common alpha.

Primary confirmation endpoints are directional accuracy, within-anchor/channel
pairwise asset-rank accuracy, and return RMSE. Report Pearson correlation,
best-asset agreement, coverage, selected alphas, validation curves, model
hashes, zero-standard-deviation counts, feature moments, norms, and paired
cosines as diagnostics.

Use 4,096 paired anchor-cluster bootstrap resamples. Candidate-minus-baseline
deltas are used except for the candidate/baseline RMSE ratio. Rows are never
resampled independently.

### Frozen acceptance gate

Noninferiority requires all three:

- direction-delta lower 95% bound `>= -0.01`;
- rank-delta lower 95% bound `>= -0.01`;
- RMSE-ratio upper 95% bound `<= 1.05`.

Material flags are:

- direction point gain `>= 0.02` and lower bound `> 0`;
- rank point gain `>= 0.02` and lower bound `> 0`;
- RMSE point ratio `<= 0.95` and upper bound `< 1`.

Qualification requires noninferiority plus at least two of those three material
flags. Correlation and best-asset agreement cannot rescue a failed primary
gate. The common-alpha comparison must be reported but is diagnostic.

## 6. Decisions and authorization

- `sparse_structured_repair_qualified`: all mechanical, coverage,
  noninferiority, and materiality gates pass. Keep `all_tokens` active, but
  authorize a fresh SRR-3 Stage A that compares the frozen MDN head with
  `all_tokens` versus `structured_cdsb_sparse_v1` on the same retained objects.
- `sparse_surface_value_gate_not_passed`: mechanics and coverage pass but the
  quality gate does not qualify the candidate. This is not proof that the
  sparse representation has no value. Keep `all_tokens`; do not interpret this
  as a reversal of SRR-1, and do not spend compute on SRR-3 yet.
- `invalid_mechanics`: custody, authority, contract, purity, parity, capture, or
  compute gate fails. Inspect no endpoint result.

SRR-4 never activates the sparse policy, migrates a checkpoint, tests the old
MDN head, or changes augmentation. `all_tokens` remains the explicit rollback. Augmentation
attribution remains deferred until SRR-3 resolves the downstream boundary with
the qualified sparse policy held fixed.
