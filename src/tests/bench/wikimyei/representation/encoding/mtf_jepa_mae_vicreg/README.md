# MTF-JEPA-MAE-VICReg representation: scientific handoff

**Continuation update — 2026-09-05:**
[LSA-0](CLEAN_IDENTITY_COVARIANCE_ADMISSION_FINDINGS.md) completed with
mechanics/custody **PASS**, decision **`not_admitted`**; LSA-1 stayed closed.
[LWM-0](CAUSAL_ONLINE_TARGET_COMPATIBILITY_FINDINGS.md) is now implemented and
completed: causal support, online-target gradients and held-out predictability
passed, but both combined objectives violated protected semantic-family floors.
Its decision is **`not_admitted_regularizer`**; LWM-1 stays closed.
[LWM-0A](TEMPORAL_FAMILY_DIRECTION_ATTRIBUTION_FINDINGS.md) completed the component
attribution: all four family failures associate with the regularizer direction;
prediction alone passes the full quality guard in all three seeds. All retained
baseline/combined evaluations replayed exactly. Zero encoder optimizer/EMA
updates were made.
[LWM-0B](TEMPORAL_SPREAD_FLOOR_FINDINGS.md) implemented a reference-relative
spread floor: collapse fixtures and healthy inactivity pass, but active
corrections still fail semantic-family guards in two seeds. Decision:
**`not_admitted`**. The next repair should preserve feature relationships during
active anti-collapse corrections. Prediction-only training is not admitted.
The original handoff below is retained; its “none registered” and proposed-LSA-0/
LWM-0 status statements predate these completed continuations.

**Handoff date:** 2026-09-05

**Repository state audited:** `main` at `29878dca563db97a83e0814229ab9bc2855c4e94`

**Scope:** the representation encoder, its self-supervised training objectives, and
the readout boundary immediately attached to it

**Next-work roadmap:** **ROR-1 — Representation Objective Resolution** (a proposal,
not a frozen protocol)

This document is the context handoff for the next Codex session. It replaces the
old operational README because that file stopped at RSSM-1 and no longer described
what the experiments had established. The detailed protocols and findings remain
the authorities; this README explains how they fit together.

The user asked for the representation module to be isolated because end-to-end
work was consuming enormous context and compute while hiding the actual failure.
That isolation was productive. Do not expand back into the entire application
unless a representation result has first earned that expansion.

## Read this first

The short, truthful conclusion is:

1. **The encoder architecture can represent the sequence well.** We demonstrated
   this on the sealed isolated synthetic-sequence benchmark after repairing the
   readout and finding a geometry-preserving encoder state (FSPA-4). This is
   strong, bounded evidence; it is not a universal proof about all data or tasks.
2. **The original `all_tokens` readout was destructive.** It averaged token roles
   that the sealed probes required to remain distinguishable. That was the first
   major problem, and the structured sparse readout repairs it.
3. **The currently implemented self-supervised objectives do not improve the
   certified encoder state.** Under matched isolated tests, the current JEPA,
   VICReg, time-frequency alignment, and even MAE definitions failed their full
   safety/quality gates. Some arms improved the scalar quality score from an
   untrained initialization, but none safely improved FSPA-4.
4. **Augmentation is not the main remaining explanation.** Outer augmentation,
   weak VICReg view corruption, independent view pairing, and clean-identical
   views were tested. Removing them did not rescue VICReg or JEPA to the certified
   boundary.
5. **This does not mean JEPA or VICReg are bad methods.** It means these particular
   objective contracts are harmful or mismatched here. The old JEPA predicts
   masked token latents from the *same sample* against a detached EMA target; it
   does not predict a semantic future observation. That distinction matters.
6. **The next high-value question is objective design, still inside the isolated
   representation boundary.** The cheapest remaining local question concerns the
   VICReg covariance interaction. If that closes, the more meaningful alternative
   is a genuinely temporal, history-conditioned, same-online-encoder prediction
   objective with explicit anti-collapse protection.

## Evidence labels used here

Please keep these categories separate:

- **Established:** directly supported by a completed, retained experiment.
- **Bounded conclusion:** established only on the sealed rows, seeds, budget,
  metric, and module surface used by that experiment.
- **Hypothesis:** a plausible explanation that has not been causally established.
- **Proposal:** a possible next experiment that has not yet been registered or
  implemented.

In particular, LSA and LWM below are **proposals**. There is no `LSA` or `LWM`
protocol, target, source file, or result in the repository at this handoff.

Small glossary:

- **AULC:** area under the probe learning curve across labeled fractions; higher
  is better on this benchmark;
- **CI:** the paired 95% confidence interval reported by the relevant protocol;
- **3/3:** all three fixed seeds, 17, 31, and 47, moved in the stated direction;
- **EMA:** exponential-moving-average target encoder;
- **raw:** the fixed equal-width raw-sequence control, not an encoder output;
- **untrained:** the initialized encoder/readout before objective updates;
- **FSPA-4 anchor:** the certified trained encoder reference, not a deployed
  production checkpoint.

## What is active, what is certified, and what is not

These three states must not be conflated:

| Surface | Current state | Meaning |
|---|---|---|
| Checked-in serving default | `all_tokens` | Operational compatibility default and rollback; scientifically known to discard useful sequence structure |
| Opt-in serving readout | `structured_cdsb_sparse_v1` | Qualified sparse structured readout, shape-compatible at `[B,3,32]` |
| Best isolated scientific encoder | FSPA-4 minimal spectral repair | Certified retained representation state on the sealed benchmark |
| Checked-in SSL training configuration | JEPA + MAE + VICReg + TF alignment enabled | Existing behavior, not scientifically promoted by this work |
| Downstream activation | Not promoted | Old head accepts the new shape but did not extract changed decisions from it |
| Next experiment | None registered | ROR-1 below is a flexible recommendation only |

The scientific reference is **FSPA-4 plus `structured_cdsb_sparse_v1`**; retained
findings also call it the “canonical rollback” while its authenticated local
artifacts exist. The operational compatibility rollback remains **`all_tokens`**.
Keep the distinction explicit.

## The isolated boundary

The quality work deliberately used the active representation dimensions
`C=3, H=30, F=9, D=32` and excluded the graph, MDN, data-source stream, runtime
pipeline, and end-to-end report machinery unless a named boundary experiment
specifically required them.

The fast mechanical target constructs the model directly from in-memory rank-4
tensors and LibTorch. It covers configuration, tokenization, masking, forward and
backward paths, losses, missing-data behavior, serving pools, and target EMA. It
does not establish representation quality.

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-isolated
```

`all`, `run`, and `run-isolated` are deliberately non-training aliases. Historical
experiment targets still exist, but completed phases are consumed evidence. Do
not rerun them merely to rebuild confidence or context.

## The central result in plain language

The encoder emits many tokens with different roles: channel, domain, scale, and
position. The original serving path averaged all valid tokens inside each
channel. This retained the output shape `[B,3,32]`, but erased distinctions that
the sealed probes required to remain distinguishable.

The repair does not add a large downstream system. It organizes token information
before reducing it. The successful CDSB construction preserves coarse
channel/domain/scale/position structure, and the sparse version preserves that
contract when real production masks do not contain complete blocks.

Once this readout defect was repaired, a separate frozen-alignment study showed
that the encoder itself could carry strong and safe sequence information. That is
why the correct statement is not “the encoder never worked.” The correct statement
is:

> The architecture and structured readout can produce a useful representation,
> but the legacy training objectives do not reliably move the encoder toward that
> state and often move it away from it.

## Completed evidence, in explanatory order

The order below follows the scientific argument, not strict execution time.
VVA-1B was executed after IMA-5 and GPV-1 and its custody depends on those retained
states, even though augmentation results are grouped together for readability.

### 1. Initial isolated quality and objective plumbing

[REPRESENTATION_QUALITY_FINDINGS.md](REPRESENTATION_QUALITY_FINDINGS.md) first
showed that the isolated optimizer and loss paths were active, while full legacy
training reduced representation quality and concentrated variance.

Key figures on that early surface:

- fixed raw-96 control AULC: `0.6022866`;
- untrained encoder with `all_tokens`: approximately `0.5192625`;
- training objectives executed and optimized, but the representation worsened.

The following short, early studies helped locate loss behavior:

- [REPRESENTATION_OBJECTIVE_MASK_ATTRIBUTION_FINDINGS.md](REPRESENTATION_OBJECTIVE_MASK_ATTRIBUTION_FINDINGS.md)
- [REPRESENTATION_OBJECTIVE_REPAIR_FINDINGS.md](REPRESENTATION_OBJECTIVE_REPAIR_FINDINGS.md)
- [REPRESENTATION_VICREG_VARIANCE_NECESSITY_FINDINGS.md](REPRESENTATION_VICREG_VARIANCE_NECESSITY_FINDINGS.md)
- [REPRESENTATION_JEPA_MAE_CORE_DECOMPOSITION_FINDINGS.md](REPRESENTATION_JEPA_MAE_CORE_DECOMPOSITION_FINDINGS.md)

These used the damaged `all_tokens` surface and a short 32-update budget. They are
valid precursors, but they are not the final objective attribution and their scores
must not be pooled with later FSPA/OCA studies. The early projected,
channel-stratified no-variance comparison was roughly equal to JEPA+MAE; its point
rescue was only `+0.004833` and the confidence interval crossed zero. Its terminal
classification was `variance_necessity_not_supported`.

The separate early [REPRESENTATION_OUTER_AUGMENTATION_TRAINING_FINDINGS.md](REPRESENTATION_OUTER_AUGMENTATION_TRAINING_FINDINGS.md)
screen also belongs to this 32-update, `all_tokens`, JEPA+MAE surface. It found that
the full `light_phase_safe_v2` outer profile was semantically harmful—it removed
valid support and often the terminal causal anchor—but replacing it with a
qualified jitter/amplitude/frequency-gain subset did not materially improve the
clean representation endpoint. Its classification was
`qualified_candidate_not_supported`. Thus semantic harm was real, but was not a
material cause of this short-run representation deficit. The later OAA/VVA work
tested augmentation at the repaired quality boundary.

### 2. RSSM-1 localized the first destructive boundary

[REPRESENTATION_SURFACE_SUFFICIENCY_MAP_FINDINGS.md](REPRESENTATION_SURFACE_SUFFICIENCY_MAP_FINDINGS.md)
compared the raw, tokenizer, encoder-prepool, and serving surfaces. Sequence access
was retained through the encoder and was first decisively lost at the
encoder-to-serving reduction.

- terminal localization: `serving_pooling_loss`;
- fixed-96 encoder-to-serving AULC loss: `0.0596`;
- all three seeds were negative and the interval stayed below zero.

This is the experiment that justified fixing the representation readout before
blaming the entire architecture.

### 3. PSM-1 found the earliest tested sufficient retained structure

[POOLING_STRUCTURE_MECHANISM_MAP_FINDINGS.md](POOLING_STRUCTURE_MECHANISM_MAP_FINDINGS.md)
tested which distinctions had to survive reduction. The earliest sufficient
structure was CDSB: channel, domain, scale, and coarse position, with 16 cells per
channel.

- CDSB AULC: `0.5931`;
- reversal score: `0.9295`;
- terminal classification: `coarse_position_separation_sufficient`.

This did not say every token must remain. It showed that indiscriminate averaging
was the damage and that a compact structured summary was sufficient.

### 4. SRR-1 and SRR-2 transported the structured repair

[STRUCTURED_READOUT_REPAIR_FINDINGS.md](STRUCTURED_READOUT_REPAIR_FINDINGS.md)
reproduced the repair and matched the translated implementation:

- `all_tokens`: about `0.5193`;
- structured CDSB: about `0.5931`;
- terminal decision: `structured_readout_reproduced`.

[PRODUCTION_STRUCTURED_READOUT_PARITY_FINDINGS.md](PRODUCTION_STRUCTURED_READOUT_PARITY_FINDINGS.md)
then proved that opt-in production `structured_cdsb_v1` was byte-exact with the
accepted structured shadow over 18 CUDA captures. The default remained unchanged
and the new path remained dormant.

This was transport/parity evidence, not a new estimate of representation quality.

### 5. SRR-3 exposed a real sparse-mask contract mismatch

[STRUCTURED_READOUT_ACTIVATION_COMPATIBILITY_FINDINGS.md](STRUCTURED_READOUT_ACTIVATION_COMPATIBILITY_FINDINGS.md)
showed that complete-block `structured_cdsb_v1` was not valid on the actual sparse
production surface. It invalidated channels 0 and 1:

- structured-valid rows: `1312 / 3936`;
- `all_tokens`-valid rows: `3936 / 3936`.

No downstream-head conclusion was justified on that invalid surface. This was a
contract failure, not evidence against structured representation.

### 6. SRR-4 repaired the sparse contract

[SPARSE_SURFACE_STRUCTURED_READOUT_CONTRACT_REPAIR_FINDINGS.md](SPARSE_SURFACE_STRUCTURED_READOUT_CONTRACT_REPAIR_FINDINGS.md)
qualified `structured_cdsb_sparse_v1` on the real sparse surface. It preserved
byte identity with the complete-block implementation when complete blocks were
available, restored coverage, and improved fresh equal-compute graph-surface ridge
endpoints: direction `+0.05115`, rank `+0.03760`, and RMSE ratio `0.93134`.

Terminal result: `sparse_structured_repair_qualified`.

Those gains establish value accessible to a freshly fitted ridge. They do not by
themselves establish value through the historical MDN or justify activation.

### 7. SRR-3R tested the old downstream boundary without retraining the encoder

[SPARSE_STRUCTURED_READOUT_ACTIVATION_COMPATIBILITY_FINDINGS.md](SPARSE_STRUCTURED_READOUT_ACTIVATION_COMPATIBILITY_FINDINGS.md)
showed that the old MDN head can mechanically consume the same `[B,3,32]` shape,
but it did not turn the changed feature semantics into changed decisions:

- full row coverage;
- direction, rank, and best-asset deltas: `0`;
- RMSE ratio: `0.9999923`;
- classification: `compatible_no_downstream_gain`;
- final decision: `downstream_bottleneck_remains_unresolved`.

The old checkpoint identity correctly rejects the sparse readout. An old head
trained on damaged feature semantics is not a fair judge of the repaired
representation. The result also does not locate which downstream layer attenuates
the change or prove that a newly trained head will succeed. This downstream thread
is parked; do not make it the next encoder experiment. The findings mention
`FHSL-1` and `SRR-5` as possible future labels, but neither has a protocol, source,
target, runtime, or result at handoff.

### 8. RMC and FSPA established that the architecture can work

[REPRESENTATION_MODULE_CERTIFICATION_FINDINGS.md](REPRESENTATION_MODULE_CERTIFICATION_FINDINGS.md)
showed that repaired readout alone did not make the current JEPA+MAE training
beneficial. FSPA then asked a narrower question: can the architecture be aligned
to a fixed sequence-projection target without changing its architecture or runtime
interface? The target projection, data, and interface stayed frozen; the encoder
weights were optimized, and FSPA-4 ultimately distilled the repair into those
weights.

- [FROZEN_SEQUENCE_PROJECTION_ALIGNMENT_FINDINGS.md](FROZEN_SEQUENCE_PROJECTION_ALIGNMENT_FINDINGS.md):
  128 updates were undertrained.
- [FROZEN_SEQUENCE_PROJECTION_ALIGNMENT_CONVERGENCE_FINDINGS.md](FROZEN_SEQUENCE_PROJECTION_ALIGNMENT_CONVERGENCE_FINDINGS.md):
  at 1024 updates the mean improved `+0.046011` over initialization and `+0.036786`
  over raw, but channel participation failed for C0 and C2.
- [GEOMETRY_PRESERVING_WHITENING_DISTILLATION_FINDINGS.md](GEOMETRY_PRESERVING_WHITENING_DISTILLATION_FINDINGS.md):
  full ZCA repaired geometry but hurt low-label access, so the student phase was
  correctly stopped.
- [MINIMAL_PARTICIPATION_SPECTRAL_REPAIR_FINDINGS.md](MINIMAL_PARTICIPATION_SPECTRAL_REPAIR_FINDINGS.md):
  a minimal spectral cap repaired participation and was distilled into encoder
  weights, with no runtime adapter.

FSPA-4 results:

| Split | Mean AULC | Relevant comparison | Result |
|---|---:|---|---|
| Development | `0.641549` | vs initialization `+0.048487`, CI `[0.034669, 0.062732]` | 3/3 positive |
| Confirmation | `0.614808` | vs initialization `+0.052797`, CI `[0.037586, 0.069006]` | 3/3 positive |
| Confirmation | `0.614808` | vs raw `+0.018443`, CI `[0.000826, 0.035473]` | passed |

Family, order, shuffle, geometry, and participation safeguards passed. The
certificate is [REPRESENTATION_MODULE_CERTIFICATE.md](REPRESENTATION_MODULE_CERTIFICATE.md):

- identifier: `representation_certified_fspa4_minimal_spectral_repair_v1`;
- served readout: `structured_cdsb_sparse_v1`;
- certificate SHA-256:
  `e8c9a0abb1faad3856afb875d0339e396ef7b542b070e924fd414eade2342cf3`.

The certificate is bounded to its sealed benchmark. It certifies that the
architecture/readout can carry useful sequence information; it does **not**
certify the legacy SSL objective, market performance, a downstream checkpoint,
or universal generalization.

### 9. OCA-1 attributed the four current objectives at the right boundary

[FOUR_OBJECTIVE_CAUSAL_ATTRIBUTION_FINDINGS.md](FOUR_OBJECTIVE_CAUSAL_ATTRIBUTION_FINDINGS.md)
started all arms from identical initialization and also judged each candidate
against the FSPA-4 scientific anchor.

| Objective arm | Mean AULC | Delta from untrained initialization | Interpretation |
|---|---:|---:|---|
| Untrained | `0.593062` | — | common baseline |
| MAE only | `0.660352` | `+0.067290` | scalar gain, but geometry failed |
| JEPA + MAE | `0.651506` | `+0.058444` | unsafe |
| JEPA only | `0.557783` | `-0.035279` | harmful |
| All four | `0.539214` | `-0.053847` | harmful |
| VICReg only | `0.419735` | `-0.173327` | strongly harmful |
| TF alignment only | `0.406950` | `-0.186112` | strongly harmful |

Here **TF means time-frequency alignment, not TensorFlow**.

At the FSPA-4 anchor, no current objective qualified. MAE had a mean point gain of
`+0.012047`, but only one of three seeds improved and the future-family floor and
geometry gates failed. The terminal classification for the current definitions
was `harmful_at_certified_boundary`.

This is stronger and more relevant than the early 32-update decompositions because
it evaluates objective effects at the repaired, certified representation boundary.

### 10. OAA-1, VVA-1, and VVA-1B ruled out augmentation as the rescue

[OUTER_AUGMENTATION_CAUSAL_ATTRIBUTION_FINDINGS.md](OUTER_AUGMENTATION_CAUSAL_ATTRIBUTION_FINDINGS.md)
found no augmentation that rescued JEPA or VICReg. The VICReg amplitude arm gained
only `+0.002853` versus identity, remained `-0.027247` below FSPA-4, and was unsafe.

[VICREG_VIEW_AUGMENTATION_CAUSAL_ATTRIBUTION_FINDINGS.md](VICREG_VIEW_AUGMENTATION_CAUSAL_ATTRIBUTION_FINDINGS.md)
showed that weak paired-view corruption was not the causal explanation.

[VICREG_VIEW_LOSS_BOUNDARY_TRIAD_FINDINGS.md](VICREG_VIEW_LOSS_BOUNDARY_TRIAD_FINDINGS.md)
then controlled the view/loss boundary more tightly:

- tied weak views minus independent weak views: `+0.000551`,
  CI `[-0.001997, +0.003249]`; inside the precommitted `±0.005` equivalence band,
  with 2/3 seeds positive;
- clean-identical views minus tied weak views: `+0.003420`,
  CI `[0.000186, 0.006414]`; only 1/3 seeds positive, so not a supported rescue;
- clean-identical views minus FSPA-4: `-0.026129`,
  CI `[-0.038443, -0.013363]`; 0/3 seeds improved.

Terminal classification: `no_candidate`; promotion: `none`. The frozen three-seed
protocol is closed. Additional post-hoc seeds would be a new experiment and cannot
retroactively change its recorded decision.

Only the VVA-1B v3 authority should be cited:

- log: `.build/tests/representation_vva1b_v3_authoritative.log`;
- log SHA-256:
  `c687a2d2639fabf4a737be2eb49f6a70158e21ce436c2859e45692ce8d4d9db4`;
- findings SHA-256:
  `46fb58436c3b27fac36e5ecfcdfa093b625521a47dadee4364a1b313ff1c68b2`;
- accepted work: `4608` Adam updates plus `4608` EMA updates;
- the invalid serializer attempt consumed `1536` additional Adam updates plus
  `1536` EMA updates, so the physical process lifetime was `6144` of each; the
  discarded work is not part of the accepted estimate.

### 11. IMA-1 through IMA-5 closed the nearby legacy-JEPA repair line

The original mask contract requested six target tokens plus 54 context tokens with
strict raw-support separation. [INTRINSIC_MASK_VIEW_CAUSAL_ATTRIBUTION_FINDINGS.md](INTRINSIC_MASK_VIEW_CAUSAL_ATTRIBUTION_FINDINGS.md)
proved this was mathematically infeasible: the maximum legal context was 52. All
1536 observed legacy samples relaxed the contract and leaked support. No training
was opened in IMA-1.

[FEASIBLE_SUPPORT_SEPARATED_MASK_CONTRACT_FINDINGS.md](FEASIBLE_SUPPORT_SEPARATED_MASK_CONTRACT_FINDINGS.md)
introduced an opt-in feasible contract: an exact two-token time/frequency target
pair with 54 legal context tokens, preserving the legacy rollback.

[DOSE_MATCHED_MASK_REPAIR_QUALITY_FINDINGS.md](DOSE_MATCHED_MASK_REPAIR_QUALITY_FINDINGS.md)
then compared that repair with a dose-matched leaky control:

- repaired minus leaky: `-0.003968`, CI `[-0.012967, 0.005143]`;
- repaired minus FSPA-4: `-0.011798`, CI `[-0.020955, -0.002616]`, 0/3 positive;
- terminal result: `mask_repair_has_no_material_representation_effect`.

Thus the leakage was real, but it was not the rescue-sized cause of JEPA's quality
failure.

[JEPA_TARGET_PREDICTABILITY_CEILING_FINDINGS.md](JEPA_TARGET_PREDICTABILITY_CEILING_FINDINGS.md)
found only small legal context signal. A categorical target-specific oracle
explained about `4.16%` of centered variance, while the existing predictor had
`R² = -1.413`.

The next bounded predictor studies also failed:

- [JEPA_TARGET_RELATIVE_PREDICTOR_SUFFICIENCY_FINDINGS.md](JEPA_TARGET_RELATIVE_PREDICTOR_SUFFICIENCY_FINDINGS.md):
  the narrow target-relative predictor was insufficient;
- [JEPA_CANONICAL_SLOT_INTERACTION_SUFFICIENCY_FINDINGS.md](JEPA_CANONICAL_SLOT_INTERACTION_SUFFICIENCY_FINDINGS.md):
  additional canonical slot interaction capacity did not rescue the target;
- [SUPPORT_PERMITTED_TEACHER_TARGET_ALIGNMENT_FINDINGS.md](SUPPORT_PERMITTED_TEACHER_TARGET_ALIGNMENT_FINDINGS.md):
  the legal target was bit-invariant to hidden target supports, but remained weak
  and unstructured and predictor R² remained negative.

The terminal decision was `jepa_branch_closed_time_target_noncollapse`. This closes
the **nearby repair family around the legacy same-sample masked-latent teacher**.
It does not show that JEPA as a general method is invalid.

Most importantly, the old implementation did **not** predict a semantic future.
It predicted masked token latents from the same sample against an EMA target that
was detached from gradients. Future work must not describe that as temporal future
prediction.

### 12. GPV-1 isolated a VICReg interaction but found no safe candidate

[GLOBAL_POOL_PROJECTOR_VARIANCE_CAUSAL_DECOMPOSITION_FINDINGS.md](GLOBAL_POOL_PROJECTOR_VARIANCE_CAUSAL_DECOMPOSITION_FINDINGS.md)
tested three factors in the current global VICReg path:

- `P`: local versus global pooling;
- `J`: remove the two GELUs while retaining the same three linear layers;
- `V`: detach variance's encoder gradient.

Only GPV mask 2—the `J`-only intervention that kept global pooling and coupled
variance but removed the two projector GELUs—produced a large aggregate
improvement:

- `0.611449 → 0.651624`;
- delta `+0.040175`, CI `[0.029556, 0.051021]`, 3/3 positive;
- `+0.010075` over FSPA-4.

It still failed safety: `order_regime = -0.030194`, below the `-0.02` floor, and
geometry remained unsafe. Therefore the result was `no_safe_candidate`. Global
pooling alone and variance encoder pressure alone were not sufficient explanations;
the boundary contains interactions.

There is one known reporting defect: a descriptive Q0-vs-Q0 contrast was printed
as `-Q0`. It does not affect the scientific decision. The science is valid, while
strict sealed-report conformance failed for that descriptive field.

A useful caution for the next audit comes from the retained GPV evidence. At the
FSPA/Stage-0 state on the affine global-pool arm, covariance trunk-gradient norms
were tiny (`~6.4e-6` to `1.11e-5`) compared with variance (`~0.0240` to `0.0297`).
Across 512 updates, however, covariance loss sums grew to roughly `2160–2200`
while variance sums were roughly `156–163`. A single initial virtual step could
therefore miss a delayed covariance mechanism.

VVA-1B already performed the corresponding component-gradient and one-step audit
for the **current nonlinear projector** under clean-identical views. Invariance was
exactly zero; covariance encoder-gradient norms were only `3.08e-6` to `5.60e-6`
versus variance `0.01754` to `0.03161`, and covariance-only virtual order/channel
effects were tiny and mixed. Do not repeat that audit. LSA-0 is worth considering
only because it changes to the affine GPV surface and inspects the delayed endpoint
as well as the start.

## What is proven, and what remains unknown

### Proven within the sealed isolated benchmark

- The first major information loss was `all_tokens` serving pooling.
- Coarse channel/domain/scale/position structure is sufficient to retain most of
  the useful sequence signal.
- The production structured implementation matched its accepted shadow exactly.
- `structured_cdsb_sparse_v1` repairs the actual sparse-mask contract.
- The architecture can realize a safe, useful representation when aligned as in
  FSPA-4.
- Every current objective definition failed promotion at the certified boundary.
- Tested outer/view augmentation choices do not explain or rescue that failure.
- The original JEPA masking contract was infeasible, but repairing leakage did not
  recover quality.
- Nearby predictor-capacity changes did not fix the old JEPA teacher/target
  contract.
- A linearized VICReg projector improves aggregate score, but the tested arm is
  not safe and cannot be promoted.

### Not proven

- That FSPA-4 is optimal, universally general, or ready for a production market
  claim.
- That the old downstream head can exploit the repaired features without a
  versioned adaptation.
- That covariance is *the cause* of VICReg harm. GPV makes it a live interaction;
  only matched training can establish causality.
- That JEPA, VICReg, SIGReg, MAE, or predictive learning are intrinsically wrong
  for this project.
- That a LeWorldModel-style temporal objective will work on this action-free
  structured sequence domain.
- That raw SIGReg or temporally centered SIGReg is safe here.
- That the machine-local `.build` and `.runtime` evidence will exist in another
  checkout.

## Current scientific interpretation

The leading interpretation is an implementation/objective mismatch, not an
incapable encoder and not primarily bad augmentation.

For VICReg, VVA-1B already covered the nonlinear projector's clean-view starting
surface. The remaining narrow hypothesis is delayed covariance behavior on the
clean-identical, global-pool, **linearized GPV arm**. GPV supports investigating
that boundary, but has not established covariance as causal.

For JEPA, the nearby old branch is exhausted. Its same-sample, masked-token,
detached-EMA target is weak and poorly structured under the legal context. A more
meaningful next family would predict a genuinely later observation from a temporal
history. That idea is inspired by recent online-target world-model work, but it is
a new hypothesis here, not an explanation already established by our data.

Relevant external design references, to be treated as inspiration rather than
project evidence:

- [LeWorldModel paper](https://arxiv.org/abs/2603.19312) and
  [official implementation](https://github.com/lucas-maes/le-wm/blob/main/train.py):
  same online encoder supplies the later target and target gradients are retained;
- [TC-LeWM](https://arxiv.org/abs/2607.26924): temporal centering can matter when
  regularizing prediction residuals;
- [LpWM](https://arxiv.org/abs/2608.22764): sparse representation regularization
  can alter predictor burden.

These are preprints and their image/action settings differ from this action-free
structured sequence setting. Do not import their conclusions as facts.

## Proposed roadmap: ROR-1 — Representation Objective Resolution

ROR-1 is a decision framework for the next session, not a command to follow
blindly. The next model should inspect the retained evidence and may improve the
protocol before registration. The purpose is to spend at most one new three-seed
encoder-training budget on the most informative remaining question.

### ROR-1A / proposed LSA-0 — Clean-Identity Covariance Admission Audit

**Status:** proposed and unregistered.

**Cost:** zero encoder optimizer or EMA transitions.

Purpose: decide whether the covariance interaction is strong and distinctive
enough to justify one matched training comparison.

Suggested frozen surface:

- FSPA-4 encoder and `structured_cdsb_sparse_v1` scientific anchor;
- sealed rows, seeds, masks, data order, thresholds, and metrics;
- clean-identical views;
- global pool;
- GPV mask 2 (`J` only): keep global pooling and coupled variance, retain the same
  three linear layers, and remove only the two GELUs;
- existing VICReg weights for the audit; no tuning.

Suggested measurements:

- separately attribute encoder gradients from invariance, variance, covariance,
  and the full loss;
- require the components to reconstruct the total gradient and prove that the
  audit leaves model state, optimizer state, EMA state, and RNG unchanged;
- compare norms, cosine directions, channel participation, and protected virtual
  directions after equal-norm virtual steps;
- audit both the **FSPA starting state** and the authenticated **GPV mask-2 endpoint
  caches** under clean-identical replay.

This is an affine-projector/delayed-state audit, not a rerun of VVA-1B's existing
clean-view audit on the nonlinear projector. Do not close covariance from the
initial gradient alone. The GPV evidence shows a
very small initial covariance gradient but large accumulated covariance loss later
in training. LSA-0 is only an admission/directionality audit; it cannot prove that
covariance causes the trajectory failure. Adam can also rescale a small raw
gradient, so gradient magnitude alone is neither admission nor rejection.

Possible stop gate: if covariance has no distinctive adverse direction at either
the start or endpoint and remains negligible relative to the full gradient, do not
spend the matched training budget. Exact numeric thresholds should be written and
hash-sealed before seeing the new audit output.

### ROR-1B / proposed LSA-1 — Clean-Identity Covariance Necessity at the Linear-Projector Boundary

**Open only if LSA-0 admits it.** This is the causal test.

Use one equal-compute A/B comparison:

- `C0`: clean-identical views, global pool, linearized three-layer projector,
  variance weight 25, covariance weight 1;
- `C1`: identical in every respect except covariance weight 0.

Suggested maximum budget: `2 arms × 3 seeds × 512 updates = 3072` accepted encoder
updates. Freeze the dataset, seeds, masks, ordering, augmentation, representation
policy, optimizer, evaluation surface, and compute.

Only matched training can justify the statement that covariance caused harm in
this clean-identical, affine-projector chassis. It would not by itself explain the
original VICReg failure generally. A
candidate must improve the scalar endpoint **and** preserve order, shuffle,
geometry, channel participation, and family safeguards. If covariance removal does
not rescue the endpoint, or if order/geometry remains unsafe, close the current
global VICReg family. Do not follow it with weight ladders, variance-floor sweeps,
or projector searches. A relative win between C0 and C1 is insufficient if both
remain below the unchanged FSPA-4 anchor.

### ROR-1C / proposed LWM-0 — Causal Online-Target Compatibility Audit

Open this route if LSA is not admitted or LSA-1 closes VICReg.

This is **not** permission to toggle two flags on the legacy JEPA branch and call
the result fixed. A valid new contract must include:

- a genuinely later observation as the target;
- a precommitted history window and prediction horizon;
- strictly authenticated, non-overlapping raw support between context and future
  target windows, with fail-closed rejection of any boundary leakage;
- the same online encoder on context and target sides;
- a temporal, history-conditioned predictor;
- no target-side detach in the full-gradient arm;
- no EMA teacher in the new full-gradient arm;
- an explicit anti-collapse regularizer;
- action-free conditioning appropriate to this dataset.

The existing implementation does contain low-level primitives that need auditing:
with `use_target_ema=false`, `compute_target_latents` returns online
`full_latents`, and `stop_gradient_target` selects detach versus target-side
gradient. Existing tests cover EMA freeze/update/no-op behavior, but they do not
certify a matched detached-versus-full target gradient decomposition or quality.
With EMA still enabled, the target path runs under `NoGradGuard`, so changing only
`stop_gradient_target` cannot create a full target-side gradient. Treat these
primitives as under-tested, not as an already implemented world model.

LWM-0 should use zero encoder updates. Predictor-only warm-up may be used if it is
precommitted and identical across arms. On the same predictor state and batch,
compare a detached target with full target gradient, inspect branch-wise gradients,
and use protected equal-norm virtual directions. Before admitting training, require:

- exact forward-value parity between detached and full-gradient arms;
- exact context-side and target-side encoder-gradient decomposition;
- causal/support legality under hidden-support interventions;
- held-out predictor compatibility;
- noncollapse, order, channel, family, and permutation safeguards;
- proof that EMA, augmentations, and every other objective stayed out of the test.

For anti-collapse admission, a small first audit could compare:

- `A0`: raw SIGReg on the prediction residual;
- `A1`: temporally centered, channel-structure-preserving SIGReg on `[B,3,32]`.

Neither is presumed safe. Preserve order, channel contrast, and low-dimensional
causal structure. Do not add a learned projector, a regularizer-weight search, or
the LpWM sparse arm to the first audit.

### ROR-1D / proposed LWM-1 — Online-Target Co-adaptation Causal Test

Open only if LWM-0 shows a coherent target gradient and an anti-collapse choice
that preserves protected geometry.

Suggested matched arms:

- `L0`: the same online encoder supplies future target values, but the
  prediction-target branch is detached;
- `L1`: same-online future target with full target gradient.

Everything else must match. Exclude the legacy EMA/masked-token JEPA, current
VICReg, reconstruction, TF alignment, downstream head, and outer system. Suggested
maximum: `2 arms × 3 seeds × 512 updates = 3072` accepted encoder updates.

A reasonable proposed mechanistic gate is `L1 − L0 ≥ +0.005`, lower confidence
bound above zero, 3/3 seed agreement, and all protected safeguards. Before opening
confirmation, L1 must also meet a precommitted absolute development gate against
the unchanged FSPA-4 anchor; a relative win is insufficient if both arms remain
materially below it. Promotion should require the same absolute and relative
standard on an untouched confirmation split. These numbers are suggestions until
a protocol is written and hash-sealed.

Do not run LSA-1 and LWM-1 as parallel candidate searches against one holdout.
Execute one precommitted line at a time. If the first line reaches confirmation,
reserve a genuinely untouched domain for the other.

### If both objective families close

Do not return automatically to the full stack. Design a new objective directly on
the structured `[B,3,32]` representation that treats common mode and channel
differences separately. That objective is not yet designed and should begin with a
zero-update semantic/gradient audit.

## Explicitly forbidden shortcuts

- Do not simply set `stop_gradient_target=false` on legacy JEPA and interpret the
  result as temporal online-target learning.
- Do not simply set `use_target_ema=false` on legacy JEPA and call it LeWorldModel.
- Do not retest whether `all_tokens` destroys information, whether production
  structured parity holds, or whether clean-identical views rescue VICReg.
- Do not add more VVA-1B seeds; the relevant decision is already closed.
- Do not judge a repaired representation by an old head trained on damaged feature
  semantics.
- Do not combine scores from incompatible readouts, budgets, or early 32-update
  surfaces.
- Do not start a long end-to-end experiment before the objective earns promotion
  in isolation.
- Do not treat a gradient audit as causal training evidence.
- Do not perform broad weight ladders, variance-floor sweeps, projector searches,
  or multiple new three-seed training families.
- Do not mutate production defaults or checkpoints as a side effect of a science
  experiment.

## Source-code map at the handoff commit

Line numbers below refer to `29878dca563d` and may drift after edits.

Primary implementation:
`src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h`

| Concern | Approximate line |
|---|---:|
| Policy enums | 35–54 |
| Defaults and objective weights | 102–155 |
| `all_tokens` default | 122–123 |
| Target EMA and gradient defaults | 143–145 |
| Default enabled losses | 149–154 |
| Weak-view corruption | 762 onward |
| Sparse structured lift | 1526 onward |
| Serving readout selector | 1777–1823 |
| JEPA mask-policy dispatch | 2244 onward |
| Projector, including its two GELUs | 3043–3070; GELUs 3067–3068 |
| Encoding and pooling | 3139–3158 |
| Masked target selection/detach | 3161–3201 |
| EMA/no-grad target path | 3598–3617 |
| VVA-1B view-pairing seam | 3686 onward |
| Global/channel VICReg boundary | 3756–3827 |

Other relevant files:

- `src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg_spec.h:46-67,185-186`
  parses policy and falls back to `all_tokens`;
- `src/config/wikimyei.representation.mtf_jepa_mae_vicreg.dsl:16`
  is the checked-in active serving policy;
- `src/config/wikimyei.representation.mtf_jepa_mae_vicreg.jkimyei:26,35-36,38-48`
  contains active objective weights, view settings, and global/channel choices;
- `src/include/jkimyei/training/representation/mtf_jepa_mae_vicreg_graph_first_launcher.h:803,1482`
  contains outer-augmentation call sites;
- `quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_triad.cpp:170`
  defines the exact VVA-1B arms;
- `Makefile:200` keeps the default target isolated and non-training;
- `Makefile:350` explicitly excludes VVA-1B from default execution.

The current nonlinear projector is `32 → 64 → 64 → 64` with two GELUs. GPV's
linearized arm retained all three linear layers and removed only those GELUs.

## Evidence custody and hygiene

At handoff, the experiment directory contained 204 tracked files. A read-only audit
confirmed:

- all 57 tracked single-entry SHA manifests match their referents;
- all 58 same-name `.build` sidecars match;
- embedded authority hashes for SRR-1/2, IMA-4A/B/C/5, VVA-1, GPV-1, and VVA-1B
  match the present local logs.

However, `.build` and `.runtime` are ignored and contain no tracked files. Their
logs, caches, and runtime reports are machine-local evidence and will not survive a
normal clone. The tracked findings and their checked-in sidecars are the portable
record.

Use care around superseded local artifacts:

- SRR-2's short pre-A3 authority is not final; A3 is final;
- IMA-4A contains refused edge-grid attempts;
- IMA-4B contains a compute-censored attempt and superseded v1a1 logs;
- VVA-1 contains a stopped diagnostic;
- VVA-1B v1/v2 custody generations and its failed serializer run are not the v3
  scientific authority;
- GPV has a superseded seed-17 mask-1 v1 cache;
- the generated tracked `__pycache__/evaluate_structured_readout_activation_compatibility.cpython-312.pyc`
  is not scientific authority.

IMA-1, IMA-2, and IMA-3 do not have standalone protocol documents. IMA-4A's
protocol digest and VVA-1's log digest are embedded in their tracked findings
rather than adjacent sidecars. Record those limitations honestly in any later
custody report.

## Low-cost mechanical commands

Run these Bash commands from `/cuwacunu` inside the canonical
`unnamed_taoist` container. They do not authorize creating, replacing, or
reconfiguring that container.

For ordinary implementation changes, use the smallest applicable gate:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-isolated
```

Configuration/contract gate:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-contracts
```

Short optimizer-path smoke test:

```bash
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg \
  -j12 run-training-smoke
```

The training smoke is not a quality experiment. GPU timing should include an
explicit warm-up because process/context wake-up can dominate tiny tests on this
machine. Timing throughput was never the scientific meaning of “performance” in
this work; performance means the encoder's ability to retain and expose useful
sequence information under the sealed quality probes.

## Environment and Docker safety

The last read-only health validation found the canonical environment healthy:
Docker `29.7.2`, the recovered `unnamed_taoist` Debian 12 container running, CUDA
12.4 visible through an RTX A2000 8 GB, driver 596.52, and the expected LibTorch
tools available. This is only a last-known-health note, not a request to touch it.

The authoritative container definition is in the repository-root `README.md`,
approximately lines 47–63. At that audited state it specified:

- container name `unnamed_taoist`;
- image `debian:12`;
- command `/bin/bash`;
- project bind-mounted at `/cuwacunu`;
- 1 GiB shared memory;
- `127.0.0.1:4872` port mapping;
- locale environment and `--gpus all`.

A prior rogue session deleted Docker containers and recovery consumed a full day.
Treat containers, images, volumes, writable layers, and caches as user data. Never
prune, delete, reset, recreate, unregister WSL storage, or invent a replacement
container. Inspect and reuse the canonical environment. If it is unhealthy, stop
and report the exact read-only diagnosis.

## Suggested first actions for the next session

1. Read this README and the FSPA-4, OCA-1, GPV-1, and VVA-1B findings. Do not load
   the entire application stack.
2. Confirm the working tree and authority hashes with read-only commands.
3. Decide whether the two-point LSA-0 audit is truly the highest-information next
   step; improve its proposed thresholds if necessary.
4. Write and hash-seal a concise protocol before observing new measurements.
5. Run zero-update evidence first. Open at most one new matched three-seed encoder
   training budget if its admission gate passes.
6. Report in plain language: what question was asked, what changed, what stayed
   frozen, what was learned, how confidence changed, and exactly what remains
   unknown.

## Final note to the next Codex

Please be careful with the user's time, tokens, and trust. The most important
progress in this session came from making the problem smaller, not larger. The
representation architecture is no longer an undifferentiated suspect: its readout
failure was found and repaired, its capacity was demonstrated, and the remaining
problem was narrowed to how it is trained.

Be equally precise about good news and limitations. Preserve successful evidence,
close failed branches when their decision is complete, and do not turn an
interesting hypothesis into a fact. You have room to improve the roadmap; you do
not need to repeat the journey that produced it.
