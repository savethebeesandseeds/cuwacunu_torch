# Representation Surface Sufficiency Map (RSSM-1)

Date frozen: 2026-08-26

## Question and boundary

RSSM-1 asks where the clean sequence-information gap first appears between the
input and the representation served by the exact accepted MTF module:

1. raw normalized history to tokenizer tokens;
2. tokenizer tokens to encoder tokens;
3. encoder tokens to the served `all_tokens` channel mean.

This is one isolated, no-training map. It constructs no optimizer, performs no
backward pass, updates no parameter or target network, uses no launcher outer
augmentation, changes no production source or API, and constructs no NodeLift,
readout, graph, Runtime, observer, policy, or end-to-end path.

The exact model is the accepted `jepa_mae_only` step-zero initialization with
seeds `17,31,47`, `C=3`, `H=30`, `F=9`, `d_model=32`, `latent_dim=32`, four
time scales `{8,16,32,64}`, strides `{4,8,16,32}`, time and frequency tokens,
and serving policy `all_tokens`. Objective coefficients do not affect encode at
step zero; the accepted combined configuration is used because its step-zero
namespace was independently audited by JMCD-1.

The primary claim is bounded to decodability of the twelve frozen synthetic
sequence factors. A separately specified reversal probe is secondary evidence
about explicit temporal order. No loss value or training trajectory is an
endpoint.

## Authoritative continuity

JMCD-1 saved no checkpoint. RSSM-1 must reconstruct each initialization with
`set_paired_rng(seed, cuda:0)` and the exact accepted combined configuration.
Failure to reproduce the accepted scalar reference prevents interpretation.

The authoritative log is
`.build/tests/representation_jmcd_v1_authoritative.log`, exactly `1,050,475`
bytes with SHA-256
`269665e337730d5d3085848904d2aa6217fdbc14aa65e78393723567d818f1bd`.

The accepted step-zero selector is the union of:

```text
^seed_(17|31|47)\.arm\.jepa_mae_only\.step_0\.(probe|geometry)\.
^summary\.arm\.jepa_mae_only\.step_0\.(probe_area_fixed_seed_mean|family_(multiscale_state|order_regime|cross_channel|future)_r2_fixed_seed_mean|geometry\.(channel_mean_effective_rank_ratio_fixed_seed_mean|channel_mean_participation_rank_ratio_fixed_seed_mean|channel_max_top_eigenvalue_share_fixed_seed_mean|channel_min_active_dimension_fraction_fixed_seed_mean))$
```

It selects exactly 72 unique keys. Canonical UTF-8 records use bytewise key
order and one LF after every `key` or `key=value` record:

- sorted `key\n` SHA-256:
  `3d32071fc014a2df24745f9b1328e8073dd798a23811679c20e4ee4419347915`;
- sorted `key=value\n` SHA-256:
  `97e87857111ec58e94c2b799808576ea86823a797edf9c52b176031ade791ab1`.

The accepted served AULCs are seed 17 `0.51029806802386968`, seed 31
`0.5121433689059538`, seed 47 `0.53534605970626181`, and fixed-seed mean
`0.51926249887869513`.

The legacy pre-normalization raw selector `^control\.raw_equal_width\.` selects
exactly six unique keys. Its sorted key hash is
`5c09768b4bd6c71e38051c1163cf9bce18911cd474c30974e81923d71d005048`
and sorted key/value hash is
`061252d17a9768702bb1c43dbd7ba62dcb4984be6811c3e2ca88299a7c586446`.
Its AULC is `0.60228658165276872`. It is reproduced before normalization only
as a continuity control; it is not the causal raw surface.

The read-only post-run auditor must compare all 72 accepted model keys and all
six legacy-raw keys exactly. RSSM's scientific classification is provisional
until that external audit passes.

## Frozen data and targets

Generate the exact deterministic disjoint datasets in current row order:

| purpose | group start | groups |
|---|---:|---:|
| normalizer only | 0 | 256 |
| probe fit | 1,000,000 | 256 |
| ridge selection | 2,000,000 | 128 |
| final test | 3,000,000 | 256 |
| nuisance pairs | 3,000,000 | 256, view 1 |
| semantic pairs | 3,000,000 | 256, semantic counterfactual |

Fit the existing per-channel/per-feature mean and population variance only on
the normalizer rows. Apply it to every causal input, set masked values to zero,
and preserve contiguous group order. Compute the legacy raw control before this
mutation and every RSSM surface after it. Emit stable hashes for group ids,
unnormalized and normalized data, masks, targets, normalization tensors, and
counterfactual pair identities.

The twelve targets and families are fixed to the implemented generator:

- multiscale state: trend, slow amplitude, mid amplitude, sine of phase, cosine
  of phase;
- order/regime: change index divided by 29, jump magnitude/sign;
- cross-channel: lag divided by 4, coupling;
- future: noiseless channel-0 means over horizons 1, 2, and 4.

## Surfaces and capture

For every batch, capture these four causal surfaces:

| code | tensor before probe flattening | flat native width |
|---|---|---:|
| `R` | normalized raw `[S,3,270]` | 810 |
| `T` | tokenizer output `[S,3,24,32]` | 2,304 |
| `E` | encoder pre-pool output `[S,3,24,32]` | 2,304 |
| `S` | served channel output `[S,3,32]` | 96 |

Production token order is `(domain, channel, scale, start, width)`. The harness
must audit that metadata order, then explicitly regroup by channel without
changing within-channel `(domain, scale, start, width)` order. Every channel
must have exactly 24 valid tokens: 12 time and 12 frequency. Masks and metadata
must be identical for all rows and seeds.

Use batch size 96 and original contiguous row order, including a final short
batch. Set the model to eval mode and use `torch::NoGradGuard`.

The scientific capture uses a public-API sandwich:

1. call public `tokenize` to obtain `T`, mask, and metadata;
2. call public `encode` to obtain `E`, masks, metadata, and pools;
3. call public `select_mtf_serving_pool(..., all_tokens, ...)` to obtain `S`;
4. call public `tokenize` again.

Require byte-exact equality between the two tokenizer batches for tokens, all
reconstruction targets and masks, token mask, and all metadata tensors. Require
their mask/metadata to equal the encode output. In the mechanical fixture only,
obtain the registered online `encoder` child through LibTorch
`named_children()`/`Module::as<SharedTokenEncoder>()`, feed the first `T`
directly, and require its `E` and `detail::pooled_by_channel` result to be
byte-exact with the public encode/serving outputs. This proves the hidden public
encode tokenization is the same while keeping scientific capture on public
APIs. No production accessor is added.

Use exactly two capture passes per batch and seed: retain the first and use the
second only to prove exact identity. Also require unchanged parameter
snapshots, unchanged CPU/CUDA generator states, and restored model mode. All
three seeds and every dataset must pass capture mechanics before the first
accepted-row probe is fit.

Move captured values to contiguous CPU float64 before probing. Byte hashes are
computed before and after the move so dtype/device conversion cannot conceal a
capture mismatch.

## Native and fixed-96 tracks

Every transition is evaluated twice:

1. `native`: flatten all coordinates shown above;
2. `fixed96`: reduce each channel to 32 values and flatten three channels.

For `R`, use the existing deterministic `270 x 32` raw orthonormal projection,
unchanged, for both the legacy and normalized raw controls. `S` is already 96
wide and uses identity.

For both `T` and `E`, use one shared `768 x 32` projection. Construct a CPU
float64 dense matrix with the existing `splitmix64`/`signed_uniform` algorithm
and domain tag `0x7273736d5f74655fULL`; for coordinate `(row,column)` use

```text
signed_uniform(splitmix64(tag ^ splitmix64(row)
                          ^ splitmix64(uint64(column) << 32)))
```

Apply reduced QR. Canonicalize each Q column by multiplying it by the sign of
the corresponding R diagonal, treating zero as positive. Use the same matrix
for every channel, surface, split, seed, and control. Require finite values,
shape `[768,32]`, and maximum `|Q'Q-I| <= 1e-10`.

Tensor hashes use the existing 64-bit FNV-1a stable-byte routine including
scalar type, rank, dimensions, and contiguous bytes. The preflight-emitted raw
and shared-token projection hashes are sealed in the pre-run manifest; the
authoritative run must match them exactly. The manifest is frozen before any
accepted-row probe is fit.

The normalized raw surfaces, probes, shuffles, reversal curves, geometry, and
robustness diagnostics are computed once. That exact stored result is reused
for seeds 17, 31, and 47 only when forming paired fixed-seed aggregates; raw is
never refit inside a model-seed loop.

## Ridge probe and primary endpoint

Use sample counts `{32,64,128,256}` and ridge values
`{1e-5,1e-4,1e-3,1e-2,1e-1,1}`. At each sample count:

- use the first `n` fit groups;
- compute feature mean and population variance from those rows only;
- use inverse standard deviation when variance is greater than `1e-12`, else
  use one;
- center targets by their fit-row mean, which is the intercept;
- add `n * alpha` to the ridge diagonal;
- select alpha independently per target by minimum validation MSE;
- break exact ties in favor of the first, smallest alpha;
- evaluate the selected prediction once on the untouched final test rows.

The 96-wide surfaces retain the existing primal Cholesky solution so the
accepted reference remains exact. Native `R`, `T`, and `E` use the algebraically
equivalent dual solution
`W = X' (X X' + n alpha I)^-1 Y`. Before scientific execution, deterministic
fixtures with `D<n` and `D>n` must show primal/dual prediction maximum absolute
difference at most `1e-9`, finite solutions, and identical selected alphas.

For each target, `R2 = 1-SSE/max(SST,1e-12)`. Average targets within each of the
four families, then average the four families for macro R2. Family AULC and
macro AULC are the arithmetic means over the four sample counts. This macro
AULC is the primary endpoint.

## Shuffled-target and leakage controls

For fit, validation, and test independently, generate a fixed no-fixed-point
row permutation using the Sattolo form of Fisher-Yates driven by `splitmix64`
and respective domain tags:

- fit: `0x7273736d5f74726eULL`;
- validation: `0x7273736d5f76616cULL`;
- test: `0x7273736d5f746573ULL`.

Apply the same split permutation to all twelve target columns, and share it
across every surface, track, and seed. Emit the permutation, fixed-point count,
uniqueness count, and stable hash. Run exactly the same ridge-selection path.
For `n` rows, initialize
`state=splitmix64(tag ^ splitmix64(uint64(n)))`; for `i=n-1..1`, replace state
with `splitmix64(state)`, choose `j=state % i` in `[0,i-1]`, and swap rows `i`
and `j`.

For every unique surface/track, the fixed-seed-mean shuffled AULC must be at
most `0.02` and its paired 95% bootstrap upper bound at most `0.05`. Raw
surfaces are computed once and repeated across seeds only during paired seed
aggregation. Any failure classifies as `shuffled_target_leakage`.

The normalized raw fixed-96 control must have real AULC at least `0.50`, and
the real-minus-shuffled paired interval lower bound must be greater than
`0.20`. Failure classifies as `normalized_raw_control_not_informative` and
prevents causal interpretation.

## Paired uncertainty

Use exactly 512 held-out-group bootstrap replicates and seed
`8387496322364763509`. For replicate `r`, initialize the existing SplitMix
state from that seed and `r`, sample 256 final-test group indices with
replacement, and reuse the same row-index table across every surface, track,
seed, family, and negative control. Seal the table hash in the pre-run manifest.

Recompute R2 and AULC from stored predictions and resampled targets; never rerun
the model. Within each replicate, compute downstream-minus-upstream per seed,
then average the three fixed seeds. Report the point estimate, linearly
interpolated 2.5/97.5 percentiles, all per-seed points, and all four family-AULC
points.

## Transition and terminal gates

Evaluate `T-R`, `E-T`, `S-E`, and `S-R` in both tracks. For each oriented
downstream-minus-upstream contrast:

- `material_loss`: point `<= -0.02`, interval high `< 0`, at least two seed
  deltas `< 0`, and at least two family-AULC deltas `< 0`;
- `family_specific_loss`: the same macro and seed clauses, with exactly one
  family-AULC delta `< 0`;
- `material_gain`: point `>= +0.02`, interval low `> 0`, and at least two seed
  deltas `> 0`;
- `noninferior`: interval low `> -0.02`, at least two seed deltas `> -0.02`,
  and every family-AULC delta `> -0.05`;
- `unresolved`: no preceding rule passes.

`material_gain` takes precedence over overlapping `noninferior`. Inclusive and
strict comparisons above are literal.

After numeric/mechanical/reference/leakage validity, first require the
canonical normalized fixed-96 `S-R` transition itself to be a loss state. If it
is not, a legacy total loss is
`legacy_raw_gap_normalization_confounded`; a native-only total loss is
`projection_sensitive_localization`; otherwise the result is
`no_material_surface_gap_reproduced`. No adjacent stage is blamed after the
canonical total gap has recovered.

When the canonical total gap is a loss, apply this exhaustive tree:

1. If both tracks first lose at the same adjacent stage, name that earliest
   stage. Emit `token_construction_loss`, `encoder_processing_loss`, or
   `serving_pooling_loss` only when both are material losses; otherwise emit the
   corresponding `*_family_specific_loss`.
2. Native `S-E` loss with fixed-96 `S-E` noninferiority is
   `prepool_width_advantage_only`, not proven pooling harm.
3. Different or otherwise asymmetric native/fixed-96 earliest loss stages are
   `projection_sensitive_localization`.
4. A fixed-96 total `S-R` loss with no resolved adjacent loss is
   `distributed_internal_loss`.
5. Any other valid evidence pattern is
   `no_terminal_interpretation_supported`.

Always emit every transition result even when the terminal label names the
earliest stage. The pure gate implementation and boundary fixtures are frozen
before results.

## Explicit reversal probe

For each fit, validation, and test group, pair its normalized sequence with an
exact time-axis reversal (`flip` of data and mask along history). Concatenate
original and reversed rows with labels `+1` and `-1`. Splits remain disjoint by
group. A sample count `n` means the first `n` groups and therefore `2n` rows.

Use the same feature standardization, ridge grid, primal/dual surface rule, and
validation-MSE alpha selection. Score balanced sign accuracy at threshold zero
and average it over `{32,64,128,256}`. Bootstrap whole paired groups, never
individual rows. Report fixed-seed mean and interval per surface/track.

A surface is `order_decodable` only when accuracy AULC is at least `0.60`, its
95% lower bound is greater than `0.50`, and at least two seeds exceed `0.50`.
It is `order_chance_consistent` only when its interval upper bound is at most
`0.55`; otherwise it is `order_unresolved`. A deterministic balanced label
shuffle must have point at most `0.55` and interval upper bound at most `0.60`
for every surface/track. This probe is diagnostic and cannot override the
twelve-target primary localization.

The balanced order-label shuffle uses a separate Sattolo permutation over the
complete `2n` fit-label vector for each `n` in `{32,64,128,256}`. Because `n`
is part of Sattolo initialization, these are four distinct sealed
permutations—not prefixes of a 512-row permutation. Validation uses one full
256-row permutation and test uses one full 512-row permutation. Tags are
`0x7273736d5f6f7472ULL` (fit), `0x7273736d5f6f7661ULL` (validation), and
`0x7273736d5f6f7465ULL` (test), shared across surfaces and seeds. Emit the
complete values, row count, uniqueness count, fixed-point count, and hash for
all six permutations.

The tokenizer-information contract must separately reproduce 72 tokens, 12
clipped scale-32/64 full-history reversal collisions, 60 shorter-window tokens,
and all 60 shorter tokens changing for its deterministic fixture.

## Geometry and semantic/nuisance diagnostics

For each surface/track/seed, report centered final-test effective rank,
participation rank, top-eigenvalue share, and active-dimension fraction.
Normalize effective and participation rank by `min(D,n-1)`, not raw width.
All values must be finite, but geometry does not override probe localization.

Reuse the exact group-paired nuisance and semantic counterfactual datasets.
After centering on the base rows and row-normalizing, compute cosine distances
from base to nuisance and base to semantic. Bootstrap the paired indicator
`semantic distance > nuisance distance`. Label robustness supported only when
its 95% lower bound is at least `0.75`. This remains diagnostic.

## Validity precedence

The gate fails closed in this order:

1. non-finite/unordered inputs, optimizer/backward activity, changed parameters
   or RNG, non-identical repeated/direct-public capture, identity/hash mismatch,
   or invalid projections: `invalid_numeric_or_mechanics`;
2. failed 72-key accepted audit: `accepted_step_zero_reference_not_reproduced`;
3. failed six-key legacy raw audit: `legacy_raw_reference_not_reproduced`;
4. failed 72/12/60 tokenizer contract: `tokenizer_plan_not_reproduced`;
5. failed normalized-raw informativeness gate:
   `normalized_raw_control_not_informative`;
6. failed continuous or reversal shuffle control: `shuffled_target_leakage`.

Only after all six stages pass may the scientific terminal tree be read.

## Workflow, attempts, and artifacts

Before the scientific invocation:

1. pin this protocol and its SHA-256 sidecar;
2. implement only the test-side pure gate, harness mode, dual ridge, auditor,
   and mechanical fixtures;
3. build the focused targets and run gate, tokenizer-information,
   primal/dual, projection/permutation, protocol-pin, and auditor self-tests;
4. run one CUDA mechanics preflight on non-scientific groups, requiring
   `optimizer_constructed=false`, `optimizer_steps=0`, `backward_calls=0`,
   `scientific_probe_fits=0`, one full batch plus a short final batch and masked
   rows, exact direct/public capture parity on both passes, unchanged
   parameters/RNG, the 72/12/60/60 tokenizer receipt, exact
   projection/bootstrap/permutation hashes, all four authorization flags, and
   the fully specified authoritative command;
5. seal protocol, parent/final source, model-header, gate, binary, reference,
   projection, permutation, and bootstrap-table hashes in a pre-run manifest.

Run on CUDA:0 with `CUBLAS_WORKSPACE_CONFIG=:4096:8`, one CPU Torch thread,
deterministic cuDNN, deterministic algorithms, eval mode, and no-grad. No Make
target may start the authoritative measurement automatically.

Exactly one authoritative invocation must contain all three accepted seeds,
surfaces, tracks, and controls. Before the attempt boundary it must generate
and hash all data, validate projections/permutations/bootstrap/tokenizer, and
complete both capture passes for all datasets and seeds. It must emit
`rssm.attempt.consumed=false` and stop if those pre-fit mechanics fail. The
attempt becomes consumed immediately before the first accepted-row probe fit
or scientific endpoint and emits `rssm.attempt.consumed=true`. A failure after
that marker is a consumed failed attempt; an interpretable completed run is
not rerun to improve uncertainty.

Canonical artifacts are:

- `.build/tests/representation_rssm_v1_preflight.log`;
- `.build/tests/representation_rssm_v1_prerun.sha256`;
- `.build/tests/representation_rssm_v1_authoritative.log`;
- `.build/tests/representation_rssm_v1_reference_audit.log`;
- `.build/tests/representation_rssm_v1_receipt.sha256`;
- `REPRESENTATION_SURFACE_SUFFICIENCY_MAP_FINDINGS.md`.

After exact audit, write the findings in this order: validity, known-gap
reproduction, four surface AULCs, transition localization, native/fixed-width
agreement, reversal behavior, controls/diagnostics, bounded conclusion, and one
recommended mechanism-specific follow-up. Then stop.

Always emit:

`training_authorized=false`

`long_run_authorized=false`

`production_or_end_to_end_authorized=false`

`follow_on_repair_authorized=false`
