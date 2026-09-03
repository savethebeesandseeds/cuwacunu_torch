# IMA-4A — JEPA Target Predictability Ceiling Protocol

## Decision question

At the frozen FSPA-4 representation, does the causally permitted JEPA context
contain enough information to predict the EMA teacher target, and, if it does,
which boundary prevents the current JEPA learner from using that information?

IMA-4A is diagnostic only. It must not train the tokenizer, encoder, predictor,
or EMA teacher. It must not modify the completed IMA-3 source, findings, or
archives.

## Frozen custody

The audit uses:

- FSPA-4 archives for seeds `17`, `31`, and `47`;
- the normalized, generated group rows already used by RMC/OCA/IMA-3;
- group-disjoint probe-train, probe-validation, and development splits;
- M1 `paired_target_legacy_context_v1` and M2
  `support_separated_pair_v1` masks;
- the existing 32-dimensional EMA target, predictor, and structured sparse
  representation;
- the existing ridge grid and validation-selection rule.

Every model parameter and buffer must be bit-exact before and after the audit.
The default CPU and CUDA generator states must be restored exactly. The report
must state `optimizer_steps=0` and `ema_updates=0`.

## Stage 0 — Mask-composition receipt

Replay the complete frozen `3 seeds × 512 updates × 96 rows` IMA-3 mask
schedule without a forward or update. M1 and M2 must retain identical target
masks, target counts, context counts, and post-mask RNG state.

For each arm report:

- context-token counts by channel × domain × scale;
- removed M1-only and replacement M2-only token counts by the same cells;
- per-channel histograms and means of unique raw-time positions covered by
  context after excluding the selected target-support union.

This receipt determines whether M2 changed only target-support visibility or
also materially changed the composition of the 54 retained context tokens.

## Stage 1 — Teacher target decomposition

For each selected target token, retain the same token layout and validity mask
and compute:

```
z_full     = EMA teacher latent from the complete token field
z_target0  = EMA teacher latent after zeroing that target token value
z_support0 = EMA teacher latent after zeroing every same-channel token whose
             raw-time support overlaps the target support

h_self   = z_full - z_target0
h_alias  = z_target0 - z_support0
h_hidden = z_full - z_support0
```

The decomposition must satisfy `h_hidden == h_self + h_alias` within the
declared floating-point tolerance. Report separately for time and frequency
targets:

- hidden-support energy divided by target-identity-centered target variance;
- prediction-residual energy projected onto `h_hidden`;
- fraction of latent dimensions where hidden-component variance exceeds the
  support-zeroed component variance;
- effective rank and participation rank of `h_hidden`.

## Stage 2 — Frozen oracle ladder

Each target sample receives the same normalized 6-D target metadata query. The
query-only control and four nested context surfaces are:

| Oracle | Frozen input |
|---|---|
| Q | Target metadata query only; detects target-identity bias that is not context prediction |
| G | Global masked mean of context latents |
| S | Context latents pooled into all channel × domain × scale cells, with normalized cell occupancies |
| F | All context latents in canonical token slots, non-context slots zeroed, with context-mask bits |
| R | Learned tokenizer outputs before `SharedTokenEncoder` in the same canonical slots and with the same mask bits |

Closed-form ridge probes are fitted independently in each seed coordinate
system on probe-train groups, select one alpha per seed/surface by aggregate
validation NMSE, and are reported only on disjoint development groups. The
denominator is target-identity-centered held-out target variance:

```
NMSE = sum ||prediction - z_full||^2
       / sum ||z_full - held_out_mean[target_token_id]||^2
R2   = 1 - NMSE
```

The unmodified current predictor is evaluated directly in the same teacher
coordinates. M1 and M2 are audited separately and use paired target masks. A
legacy-context replay guards the interpretation of the predictor score against
mask-policy distribution shift. Validation selection must fail closed if the
best alpha is at a grid edge and the validation curve is still improving.

Uncertainty uses the existing 4,096 deterministic group-bootstrap rows. Both
target tokens belonging to a sampled group move together, fits and selected
alphas remain fixed, and seed effects are averaged only after scoring within
their independent latent coordinates.

## Stage 3 — Gradient localization without an update

On one frozen 96-row diagnostic batch per seed, compute served-parameter
gradients for:

- the complete target MSE;
- the support-zeroed target MSE;
- time-target MSE and frequency-target MSE separately;
- frozen protected order/regime and cross-channel probe losses.

Define `g_hidden = g_full - g_support0`. Report its norm fraction, time/frequency
gradient cosine, M1/M2 gradient cosine, and the cosine plus normalized
first-order loss change against both protected probes. The M2-minus-M1 gradient
receives the same protected-direction report.

The diagnostic calls through the model's registered tokenizer, target
tokenizer, online/target encoders, and predictor must reproduce the public
tokenizer, target encoder, actual masks, and JEPA loss before their gradients
are accepted. Gradients may be materialized and cleared, but no parameter value
may be changed.

## Routing gates

Use M2 as the causal decision arm. A difference is material when the three-seed
mean absolute R2 gap is at least `0.05` and at least two seeds have the stated
sign.

1. `F - G >= 0.05`: canonical context structure carries useful information;
   a mean-like/context-blind path is a bottleneck.
2. `R - F >= 0.05`: the tokenizer retains useful information that the context
   encoder damages; inspect a JEPA-only context contextualizer.
3. `F - current >= 0.05`: the predictor does not reach the frozen latent
   ceiling; inspect explicit context metadata and actual attention use next.
4. `F` and `R` both have `R2 <= 0`, while hidden-gradient fraction is at least
   `0.25`: do not enlarge the predictor; redesign the teacher target
   abstraction.
5. The current predictor is within `0.05` of `F`, but its JEPA gradient has
   negative protected cosine in at least two seeds: capacity is not the main
   problem; audit target/loss/EMA dynamics next.
6. If none is decisive, report the boundary as unresolved and identify the
   single smallest additional zero-update measurement needed. In particular,
   query-only-level performance from every affine context oracle cannot prove
   intrinsic unpredictability because an appended-query ridge cannot select a
   target-relative slot; run one fixed bilinear query-by-field control before a
   target-redesign verdict. Do not authorize training from an unresolved
   signature.

`F` is a nonlinear deterministic transformation of `R` and the mask. Therefore
`R > F` supports loss of *linear accessibility*, while `F > R` supports useful
linearization; neither direction is an information-theoretic proof. High-D
ridge surfaces also need not be empirically monotone because standardization
changes their regularization geometry.

No IMA-4B training arm is selected until all mechanical, custody, decomposition,
oracle, and gradient receipts pass.

## Fail-closed continuation recorded after attempt 1

The first complete measurement run was retained as
`.build/tests/representation_ima4a_v1_attempt1_edge_grid.log`, SHA-256
`8c3c3dc6d1d7f23fb3f87c40b5fcf285dc181e9e571ecbf1609193fe4621bd38`.
It exited nonzero because every S/F/R validation curve in both mask arms was
still improving at the inherited upper ridge edge `alpha=1`.  This is a
mechanical refusal, not a scientific result.

Before a second run, only the following two zero-update continuations are
added.  All data, groups, seeds, representations, masks, targets, metrics, and
decision thresholds remain fixed:

1. Extend the ridge tail deterministically with `10, 100, 1e3, 1e4, 1e6,
   1e8, 1e10`.  The original six values remain unchanged.  The same edge rule
   still fails closed if the selected endpoint is measurably improving.
2. Add the precommitted fixed bilinear control B.  If `q` is the same 6-D
   target query and `f` is the same canonical F field (masked latents plus mask
   bits), B is the frozen feature map `[q, f, vec(q outer-product f)]`.  B has
   no learned feature extractor; only the same validation-selected closed-form
   ridge readout is fitted.  It is evaluated for both M1 and M2 with the same
   group custody.

Pre-execution review identified that B permits field weights affine in the six
continuous metadata coordinates but not an arbitrary categorical target-slot
selector.  Therefore add C as the final fixed identifiability control:
`[q, f, one_hot(target_token_id) outer-product f]`.  C is evaluated exactly by
its dual kernel, with train-only standardized `[q,f]` and inverse-frequency
normalization of the categorical blocks; the huge explicit feature matrix is
never materialized.  Validation/test target identities unseen in the fit split
fail closed.

The alpha tail is also checked against the exact intercept-only validation
NMSE.  A finite upper-tail value may close the edge only when it agrees with
that intercept endpoint within a scale-aware `1e-9` tolerance.  Deterministic
CPU tests must exercise the wide dual path, bilinear recovery, categorical
slot recovery, and both edge directions before the CUDA run.

Scientific routing is emitted only when every mechanical receipt passes.
Relative improvements between negative R2 values are not called usable
ceilings.  If M2 B or C obtains positive development R2 and materially exceeds
F, route to a target-conditioned interaction bottleneck.  Because target
metadata is deterministic by categorical slot and C can therefore express B's
field slopes, call the effect categorical only when C also materially exceeds
B; otherwise prefer the simpler continuous-query route.  If
the best of G/S/F/R/B/C remains at or below zero while the hidden-support
gradient fraction remains at least `0.25`, report a *bounded* context-control
failure and route to a support-permitted teacher-target abstraction rather
than enlarging the current predictor.  This does not claim an information-
theoretic impossibility.

Attempt 1 already exposed the development scores.  The continuation is an
explicitly recorded development-set diagnostic continuation, not a newly
blinded held-out confirmation.  This continuation still authorizes no
representation training.

The reviewed second run is retained as
`.build/tests/representation_ima4a_v1_attempt2_lower_edge.log`, SHA-256
`358879f5da2e55b871a56847642bccf5db41cc8be501ec50fd071ca07fcf84cd`.
It closed every original oracle and all M2 grids, but exited nonzero because C
on legacy M1 alone selected the lower inherited edge: its improvement from
`1e-4` to `1e-5` was approximately `1.4e-7` to `1.8e-7` across the three
seeds.  Before the next execution, extend only the lower grid tail with
`1e-10, 1e-8, 1e-6`.  The same scale-aware edge rule remains in force.  No
result or routing threshold changes, and no scientific route from the refused
second log is accepted.

## Completion evidence

IMA-4A is complete only when:

- a separate executable passes deterministic CPU self-tests;
- the full three-seed CUDA audit exits successfully;
- an immutable audit log and checksum are retained;
- the findings state the measured ceiling, the localized boundary, the exact
  routing decision, and what remains unknown;
- IMA-3 artifacts remain byte-identical to their recorded hashes.
