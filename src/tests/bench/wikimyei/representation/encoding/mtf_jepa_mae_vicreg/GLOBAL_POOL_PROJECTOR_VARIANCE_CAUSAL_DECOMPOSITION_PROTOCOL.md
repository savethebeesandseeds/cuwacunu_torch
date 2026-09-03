# GPV-1 — Global Pool–Projector–Variance Causal Decomposition protocol

Date frozen: 2026-08-30, before GPV-1 implementation or training

## Decision question and boundary

VVA-1 established that the two active module-owned VICReg view corruptions do
not explain the harmful certified VICReg-only trajectory. GPV-1 now asks which
part of the unchanged objective surface transmits that harm to the served
representation:

1. reducing all 72 canonical encoder tokens to one global mean before VICReg;
2. the two GELU nonlinearities in the shared `32 -> 64 -> 64 -> 64`
   projector; or
3. direct variance-floor pressure from the projector through the pool into the
   tokenizer and encoder.

The complete `2 x 2 x 2` experiment also measures every interaction. It is
strictly representation-module-only:

```text
frozen synthetic sequence rows
  -> exact MTF tokenizer and shared encoder
  -> fixed current paired views
  -> controlled pool/projector/variance-gradient surface
  -> structured_cdsb_sparse_v1 clean evaluation
  -> fixed lightweight representation probes and geometry
```

No downstream head, graph, NodeLift, MDN/readout, observer, policy, launcher
augmentation, JEPA redesign, or end-to-end path may be constructed. Labels may
not influence representation training.

## Frozen evidence and custody

Before any optimizer update require:

- VVA-1 protocol SHA-256
  `8e5f3e0aecf9990cb5adb54e090d781cc91b22c5880b8ae622c1fea10fa33616`;
- VVA-1 findings SHA-256
  `8e651276f444bbafe4f534132245b48b718b16d98de0f31b62a21bec0b6851f0`;
- VVA-1 authoritative log SHA-256
  `d73635a87d96f6d251a8a008b442657066893d3074194bf7f9de055ff61d9d33`;
- VVA-1 harness source SHA-256
  `5b807c5dfd9bb371a40e1ee062c72f0754b817b91158d8cbdfe16ee3b9642d31`;
- OCA-1 authoritative log SHA-256
  `3ade025b37525c45d376eabaca2c31771ec7d23fbd2f32f26277cec0d9be606d`;
- audited representation header SHA-256
  `93640972e497dc49f37e7690e59c2f2e55f12ece25687fe4e6f6c96b28c3c9ea`;
- FSPA-4 seed-17/31/47 archive SHA-256 values
  `5d96b2961daa2bbd08a07a157ddab5debd9d4928234d8341b3961678327e9434`,
  `a85c00d5694d1e3f0063e8bce6fc3c2e3132a393e5bcee195909742754e76775`,
  and `b9c2f82f26a5516069f8460095d3a2b85c482b7cd0e20db120ce2e0bbd68e392`;
- OCA-1 `anchor_challenge` completed-cache SHA-256 values
  `5290559894fa6e3f5d2fd57f32a90e97bb0eec924ec22cc9354ae26d0e629c92`,
  `bb5391840ede1ad4aaf8ff760d39b796a7a300918e1aab881fbb5255051fcc39`,
  and `aaa7dd4b7638f240d1e940db7ede3bc40eb8ad01000baa90dc043801a29256a6`.

Parse, rather than infer, VVA-1's completed status, no selected view recipe,
unchanged production defaults, and authorization of this boundary. Parse
OCA-1's completed status, zero outer-augmentation calls, and harmful VICReg
verdict. Load objective mask `8` from each completed OCA cache, require mask
order `{4,1,2,8,15}`, select position 3 / `arm_8`, and require its metadata,
512-step receipt, initialization, and current model to pass. Load every FSPA-4
archive into a fresh model and require exact metadata and state replay.

Before interpreting a GPV result, reproduce current-mask clean AULCs exactly
at logged precision: seed 17 `0.60479711475045383`, seed 31
`0.58888535837559386`, seed 47 `0.64066353570869317`, and mean
`0.61144866961158029`. Reproduce the FSPA-4 mean
`0.64154862079148123`.

## Frozen factors and complete inventory

A set bit denotes the intervention relative to current behavior:

- bit `0` (`P`): use canonical-slot-local VICReg instead of the global token
  mean;
- bit `1` (`J`): replace both projector GELUs with identity while retaining
  the exact same three live Linear modules;
- bit `2` (`V`): block only the variance component's gradient at the selected
  pool before it reaches the served trunk, while retaining its value and head
  gradients.

| mask | pool/statistics | projector | variance path |
| ---: | --- | --- | --- |
| `0` | global mean | production GELU | trunk-coupled |
| `1` | 72 canonical slots | production GELU | trunk-coupled |
| `2` | global mean | same Linears, no GELU | trunk-coupled |
| `3` | 72 canonical slots | same Linears, no GELU | trunk-coupled |
| `4` | global mean | production GELU | head-only |
| `5` | 72 canonical slots | production GELU | head-only |
| `6` | global mean | same Linears, no GELU | head-only |
| `7` | 72 canonical slots | same Linears, no GELU | head-only |

Mask `0` is the exact harmful OCA/VVA current model and is loaded, never
retrained. Masks `1..7` are the complete new factorial.

### P — global versus canonical-slot-local constraints

The current route applies the shared projector to each view's masked global
mean `[B,32]`, then computes one VICReg population across `B`.

The slot-local route applies the same shared projector independently to the
unchanged encoder tensor `[B,72,32]`. For canonical slot `s`, compute VICReg
across batch rows with the joint view mask for only `s`; average the 72 scalar
slot losses with the fixed divisor 72. Never flatten `B x 72`, never sum slot
losses, and never allow a varying active-slot denominator. Require all 72
metadata positions and their order to be exact across rows and views, at least
two jointly valid rows in every slot, and manual-loop equality.

This factor estimates `one global sample constraint` versus `equally weighted
canonical-slot-local constraints`. The change in gradient allocation is an
intrinsic part of removing the global pool; it must not be described as an
arithmetic-mean-only effect. No CDSB projection, imputation, serving readout,
or additional learned module enters training. Encoder layers retain their
existing broadcast all-token mean; P tests only the final pre-VICReg global
mean, not removal of all global mixing.

### J — nonlinear versus affine use of the same projector

The production route is:

```text
Linear(32,64) -> GELU -> Linear(64,64) -> GELU -> Linear(64,64)
```

The affine route calls those exact same registered weights and biases in the
same order but omits only the two GELUs. It retains 64 output dimensions,
identical parameter names/counts, the same initialization, optimizer
allocation, and all three trainable Linear modules. Direct 32-dimensional
projector bypass is excluded because it would change dimensionality,
covariance normalization, and parameter participation.

### V — trunk-coupled versus variance-head-only

Both levels retain `vicreg_var_weight=25`, the identical scalar loss, projected
values, and projector-head optimization. Similarity and covariance always use
the normally attached selected pool. In the head-only level only the variance
component recomputes the same deterministic projector on `pool.detach()`.
Thus its raw variance value and head gradient remain equal at an identical
state, while its tokenizer/encoder gradient is exactly zero.

This factor isolates direct variance-floor pressure on the served trunk
without making the branch inert. It does not claim to remove every later
indirect effect mediated by the evolving shared projector.

## Stage 0 — exact treatment, dose, and RNG proof

Runtime is frozen to `cuda:0`, float32,
`CUBLAS_WORKSPACE_CONFIG=:4096:8`, deterministic PyTorch algorithms, and the
repository's deterministic cuDNN settings. Build only with:

```text
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg gpv1-audit
```

The executable must emit and bind the protocol, final harness source,
executable, representation header, transitive scientific-source/build
manifest, dataset/split, and bootstrap-table hashes before any update.

Metadata, masks, views, hashes, RNG states, parameter names/counts, config
manifests, and cache bytes declared exact use bit equality. Float32
production-parity fields use `atol=1e-7, rtol=1e-6`. Stable single-graph
gradient/component reconstruction uses maximum absolute residual `5e-5` and
relative L2 residual `1e-4`; direct subtraction of separately rounded large
gradient vectors is descriptive only and never gates.

Before training, use all seeds `17,31,47` and require:

- mask `0` custom-route outputs, component losses, total VICReg loss, all
  trainable gradients, weak views, masks, and CPU/CUDA RNG poststate equal the
  unchanged production forward under the explicit exact/float tolerances above;
- exactly 72 stable canonical slots with exact metadata/order, a fixed divisor
  of 72, at least two joint rows per slot, and equality to an independent
  explicit 72-loop calculation;
- in preflight, every mask consumes one retained pair of weak-view tensors
  generated once from the exact OCA forward seed; the raw objects, masks,
  hashes, and row indices are shared by all masks;
- J pairs have identical model parameters and 64-dimensional output; their
  manifests differ only by the test-only activation policy;
- V pairs have equal projected values, raw sim/variance/covariance values,
  scalar loss, and projector-head gradient at identical state;
- the V-pair served-trunk gradient difference reconstructs the effective
  variance trunk gradient, and the head-only variance trunk gradient is zero;
- all component and total gradients are finite; active gradients and updates
  are nonzero; inactive parameter partitions remain exact;
- current view time-dropout probability remains `0.01`, Gaussian jitter
  remains `0.005`, and feature dropout remains exactly inactive.

The complete eight-arm manifest must be identical except for test-only P/J/V
policy fields. The affine helper must resolve the three live registered Linear
modules by exact name and order. All head gradients must be finite and all
three weight tensors must participate. Bias gradients and updates are emitted
per tensor; exact zeros are permitted because VICReg is invariant to a common
output translation and the affine stack can make its composed biases
structurally inactive. Any Stage-0 update check uses a disposable shadow clone,
proves one Adam then EMA update, and restores/discards the clone; Stage 0
performs zero authoritative updates.

Any failure stops before training. No factor, scalar, threshold, row, or arm
may change after this stage.

## Stage 1 — complete factorial representation-quality attribution

Train masks `1..7` only. Freeze:

- exact seed-matched FSPA-4 initialization;
- seeds `17,31,47`;
- exactly 512 Adam updates per mask, learning rate `1e-3`;
- gradient clipping at norm `5.0`;
- batch size 96 and the exact OCA row schedule;
- one Adam step followed by one target-EMA update;
- objective mask `8`: `lambda_vicreg=0.05`, all other outer objective
  coefficients zero;
- `lambda_global_vicreg=0.25` as the common inner multiplier;
- component weights `{similarity, variance, covariance}={25,25,1}`;
- the exact current time-dropout and Gaussian-jitter weak views;
- tokenizer, encoder, model initialization, rows, normalization, masks,
  probes, bootstrap rows, metrics, thresholds, and clean structured readout.

This is exactly `7 masks x 3 seeds x 512 = 10,752` new Adam updates and the
same number of EMA updates. No duration, floor, coefficient, normalization,
pool, projector, or optimizer search is authorized.

For zero-based update `u`, generate the weak-view pair from the exact OCA
forward seed

```text
int64(splitmix64(0x6f626a5f6d61736bULL
                 ^ uint64_t(seed)
                 ^ (uint64_t(u) << 32U))
      & 0x7fffffffffffffffULL)
```

After seeding, run the exact production RNG prelude: unchanged tokenization and
`create_masks()` through the unchanged full forward before extracting its
debug weak-view tensors. Direct weak augmentation from the initial seed is
forbidden because it skips prelude draws. Require bit-exact regenerated view
values, masks, and poststates across all masks. Require identical row hashes,
view hashes, one finite positive loss, finite gradients, independent per-arm
norm clipping at `5.0`, one optimizer step, and one EMA update with
`tau=0.990`. Tokenizer, encoder, and all six VICReg-head weight/bias tensors
must have their per-tensor deltas reported, with all three head weights and at
least one head tensor changing. Structurally translation-invariant bias tensors
may remain exact. Predictor and MAE-decoder tensors must remain exact; target
tokenizer and target encoder may change only through the one EMA update.

Train and commit each `(seed,mask)` cell independently so one active autograd
graph is retained at a time. After 512 updates, atomically commit one complete
cell archive named `.build/tests/gpv1/seed_<seed>_mask_<mask>_v1.complete.pt`
under schema `gpv1.cell_cache.v1`. It contains hashes for the protocol, final
harness source, executable, representation header, transitive build/scientific
sources, VVA/OCA evidence, dataset/splits, and bootstrap table; seed; mask;
full factor manifest; final model state; 512 ordered row hashes; four ordered
view hashes per update; min/max/mean loss, total/trunk/head gradient and served
update norms; clipping count; all parameter-partition deltas; Adam/EMA counts;
finite/mechanics flags; and `complete=true`.

Write and close a unique same-directory temporary archive, hash it, rename it
to the immutable final archive, write a temporary marker containing one
lowercase 64-hex SHA-256 plus newline, and rename the marker last. Marker
rename is the commit operation. A final archive without its marker is
uncommitted, has no authority, and is removed only at that exact GPV cache path
before restarting the cell. A hash or metadata mismatch in a committed
archive/marker pair is fatal. A valid complete pair is reused; any other
interrupted temporary file is ignored and removed. This bounds interruption
loss to one cell without partially serialized Adam-state ambiguity.

## Endpoints and factorial contrasts

Evaluate only clean inputs with `structured_cdsb_sparse_v1`. The primary
endpoint is the existing macro probe AULC. Retain all four family scores,
reversal/order and shuffled controls, raw equal-width control, and per-channel
effective rank, participation rank, largest-eigenvalue share, and active
dimension fraction.

Reuse exactly `rmc_bootstrap_rows(256)`, require its existing validity check,
emit and cache-bind its stable tensor hash, and use the existing
`percentile_interval` implementation. Every `Q_m` is the arithmetic mean of
the three fixed-seed macro probe AULCs. The interval measures held-out
generated-group uncertainty; the fixed seeds are paired but do not estimate
training-seed uncertainty. No multiplicity correction is implied.

The four families are exactly `multiscale_state`, `order_regime`,
`cross_channel`, and `future`. The named safeguards are exactly
`numeric_valid`, `mechanics_pass`, `family_floor_pass`,
`raw_noninferiority_pass`, `order_point_pass`, `order_lower_pass`,
`order_retention_pass`, `continuous_shuffle_pass`, `order_shuffle_pass`, and
`geometry_pass`.

Define `Q_pjv = Q_(p + 2*j + 4*v)`. Report every `Q_m-Q_0`, every
`Q_m-FSPA4`, all three seed values, four family effects, all twelve
stratum-specific simple effects, and exactly:

```text
P   = [(Q1-Q0)+(Q3-Q2)+(Q5-Q4)+(Q7-Q6)]/4
J   = [(Q2-Q0)+(Q3-Q1)+(Q6-Q4)+(Q7-Q5)]/4
V   = [(Q4-Q0)+(Q5-Q1)+(Q6-Q2)+(Q7-Q3)]/4

PJ  = [(Q3-Q2-Q1+Q0)+(Q7-Q6-Q5+Q4)]/2
PV  = [(Q5-Q4-Q1+Q0)+(Q7-Q6-Q3+Q2)]/2
JV  = [(Q6-Q4-Q2+Q0)+(Q7-Q5-Q3+Q1)]/2
PJV = Q7-Q6-Q5-Q3+Q4+Q2+Q1-Q0
```

A positive main or simple effect is supported when its AULC point is at least
`+0.0025`, paired lower 95% bound is greater than zero, and all three seed
effects are positive. Family effects are emitted but safeguard state is not
assigned to weighted virtual contrasts. An interaction is supported
two-sided when `abs(point)>=0.0025`, its interval excludes zero, and all three
seed effects have the same nonzero sign. If an interaction involving a factor
is supported, that factor's main effect is descriptive and causal language
uses only its supported conditional simple effects.

A concrete cell's `Q_m-Q_0` improvement requires point at least `+0.0025`,
paired lower 95% bound greater than zero, all three seed effects positive, no
family effect below `-0.02`, and no newly introduced safeguard failure. This
exact predicate defines `materially beats current` everywhere below. A new
failure means one of the ten named safeguards passes for current mask 0 but
fails for that candidate on the same paired evaluation. Do not reuse VVA-1's
audited-overstrict aggregate helper. Full candidate safety requires all ten
named safeguards to pass, regardless of current mask 0.

## Candidate, confirmation, and stop gates

Classify every mask `1..7` independently. Candidate selection is tiered so an
unsafe large effect cannot suppress a smaller usable repair:

1. select among all `representation_rescue` cells if any;
2. otherwise select among all `objective_made_safe` cells if any;
3. otherwise select among cells that materially beat current as
   `mechanism_mitigates_harm_only`;
4. otherwise return `no_safe_candidate`.

Within a tier select the greatest direct AULC improvement over current; points
within `1e-12` are tied, then fewer set bits and lower mask id break the tie.

Emit mechanism attribution independently of candidate safety. For P, J, and V
emit whether the positive main effect or any positive conditional simple
effect is supported, with interaction-qualified wording. Emit separate PJ,
PV, JV, and PJV support flags. `mechanism_effect_supported=false` only when no
main, simple, or interaction test is supported. A supported effect from an
unsafe cell remains causal evidence but is not a usable objective.

Classify the selected development result as:

- `representation_rescue` only if candidate minus FSPA-4 is at least `+0.005`,
  has lower bound greater than zero, is positive in all three seeds, has at
  least three positive family deltas, no family below `-0.02`, and all frozen
  safeguards pass;
- `objective_made_safe` if it materially beats current, candidate minus
  FSPA-4 has lower bound greater than `-0.005`, no family delta is below
  `-0.02`, and all ten named safeguards pass, but rescue is not established;
- `mechanism_mitigates_harm_only` if it materially beats current but remains
  unsafe or materially below FSPA-4;
- `no_safe_candidate` if no concrete cell passes the direct candidate gate;
- `invalid_numeric_or_mechanics` on any custody, inventory, dose, finite,
  pairing, cache, RNG, or update failure.

Only `representation_rescue` or `objective_made_safe` opens untouched
confirmation rows: 256 synthetic groups beginning at `9,000,000`.
Confirmation performs no training and must pass the same classification
clauses. A failed confirmation leaves no promoted treatment.

No GPV-1 outcome directly edits production defaults. A confirmed candidate
may authorize a separately versioned objective-implementation and checkpoint
study. Preserve rollback to FSPA-4 with `structured_cdsb_sparse_v1` and preserve
the operational `all_tokens` rollback. If no candidate is safe, close the
current VICReg objective at this representation boundary rather than starting
a coefficient or architecture search.
