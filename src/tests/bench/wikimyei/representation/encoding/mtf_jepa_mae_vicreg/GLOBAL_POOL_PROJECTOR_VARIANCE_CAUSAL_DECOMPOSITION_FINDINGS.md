# GPV-1 — Global Pool–Projector–Variance Causal Decomposition Findings

## Human conclusion

GPV-1 did not find a safe repair to activate.

It did find one important causal clue. Removing the two GELU activations from
the existing VICReg projector, while retaining the same three trained Linear
layers, global pool, and encoder-coupled variance path, raised mean
representation quality from `0.611449` to `0.651624`. The improvement over the
current VICReg objective was `+0.040175`, its paired 95% interval was
`[+0.029556, +0.051021]`, and all three seed effects were positive. It also
exceeded the FSPA-4 anchor by `+0.010075`, with paired interval
`[+0.001130, +0.019458]` and three positive seeds.

That raw gain was not broad or safe. The protected `order_regime` family fell
by `-0.030194`, beyond the frozen `-0.02` family floor. The candidate therefore
failed its direct gate. It also did not satisfy the full frozen geometry gate.
No confirmation rows were opened and no production default changed.

An independent audit gives a split validity verdict:

```text
scientific decision: PASS
strict sealed-protocol reporting conformance: FAIL
```

The conformance failure is confined to the descriptive mask-0 self-contrast.
The log prints `Q0-Q0` as `-Q0` instead of zero. This defect does not feed any
mask 1–7 gate, factor contrast, selection, classification, or confirmation
decision. It is documented precisely below and must not be hidden or mistaken
for a treatment-result failure.

The result is therefore:

```text
no_safe_candidate
```

This closes the present local search over the tested pool, projector
activation, and variance-gradient switches. It does not prove that every
possible projector or VICReg redesign must fail. It proves that none of these
seven exact interventions is a representation repair that preserves all
protected properties.

The next justified goal is:

```text
IMA-5 — Support-Permitted Teacher Target Alignment
```

Begin with its no-encoder-training admission gate. Do not activate mask 2 and
do not begin augmentation attribution yet.

## The question GPV-1 answered

The current isolated VICReg objective was already known to damage the
representation relative to the FSPA-4 anchor. GPV-1 asked whether that damage
could be causally assigned to any combination of three implementation choices:

- `P`: one masked global token mean versus 72 canonical-slot-local VICReg
  populations;
- `J`: the production projector's two GELUs versus identity at the same two
  locations, retaining the exact same three live Linear layers;
- `V`: allowing the variance term to push the encoder directly versus
  preserving its value and projector-head gradient while detaching only its
  encoder gradient.

The experiment was representation-module-only. It constructed no downstream
model, did not measure throughput, did not run an end-to-end task, and did not
change the augmentations.

## Exact factorial result

Mask 0 is the cached current harmful VICReg condition. Masks 1–7 were trained
for 512 updates under seeds 17, 31, and 47.

| Mask | Interventions | Mean AULC | Versus current | Decision |
|---:|---|---:|---:|---|
| 0 | current global pool + GELU + coupled variance | 0.611449 | — | cached control |
| 1 | P | 0.597515 | -0.013933 | failed |
| 2 | J | 0.651624 | +0.040175 | raw gain, direct gate failed |
| 3 | P + J | 0.564447 | -0.047001 | failed |
| 4 | V | 0.606087 | -0.005361 | failed |
| 5 | P + V | 0.592884 | -0.018564 | failed |
| 6 | J + V | 0.588285 | -0.023164 | failed |
| 7 | P + J + V | 0.601181 | -0.010267 | failed |

The FSPA-4 anchor mean was `0.641549`.

Mask 2 was the only treatment with a positive mean difference from current.
Its seed-wise improvements were:

| Seed | Mask 2 − current |
|---:|---:|
| 17 | +0.025032 |
| 31 | +0.066372 |
| 47 | +0.029122 |

Its family changes relative to current were:

| Protected family | Difference |
|---|---:|
| multiscale state | +0.086902 |
| order/regime | -0.030194 |
| cross-channel | +0.077500 |
| future | +0.026492 |

The score increase was therefore real but uneven. The experiment was designed
to reject exactly this kind of aggregate win when it sacrifices a protected
sequence property. Mask 2 passed raw noninferiority and introduced no new named
safeguard failure relative to current, but it failed the family floor because
`order_regime < -0.02`. Its full geometry safeguard also remained false. It is
evidence about mechanism, not an activation candidate.

## What the causal effects mean

### Pool/statistics switch P

The descriptive P main effect was `-0.025354`, with paired interval
`[-0.032128, -0.018874]`. No positive P simple effect met the frozen support
rule. Canonical-slot-local VICReg is therefore not a repair by itself in this
implementation.

Supported interactions involving P mean its effect is context-dependent, so
the negative average must not be generalized to every possible P setting.
What is settled is narrower: none of the four concrete masks containing P was
safe or better than current on mean.

### Projector activation switch J

The averaged J main effect was nearly zero: `-0.000600`, interval
`[-0.006661, +0.005329]`. This average hides a supported conditional effect.
With the current global pool and encoder-coupled variance path, removing GELU
improved AULC by `+0.040175`, interval `[+0.029556, +0.051021]`, in all three
seeds. In the other factor strata, that benefit became negative or unstable.

Thus projector nonlinearity is a genuine conditional contributor, but simply
removing it is not a representation-safe solution.

### Variance-gradient switch V

The descriptive V main effect was `-0.009149`, interval
`[-0.019967, +0.001824]`. No positive V simple effect passed all frozen support
conditions. Blocking the variance term's direct encoder gradient is therefore
not supported as a standalone repair.

### Interactions

Two non-additive interactions were supported:

- `P × V = +0.050402`, interval `[+0.039520, +0.062040]`, same positive sign
  in all three seeds;
- `P × J × V = +0.099343`, interval `[+0.077897, +0.121108]`, same positive
  sign in all three seeds.

These are difference-of-differences, not treatment gains. They show that the
three implementation choices change one another's effects. They do not imply
that the combined treatment worked: actual mask 7 was `-0.010267` versus
current and failed the safety gates.

The `P × J` and `J × V` interactions did not meet their frozen same-sign
support rules.

## Mechanical validity and reporting erratum

Training, cache custody, and every decision-bearing calculation completed:

- module-only execution: `true`;
- downstream models constructed: `0`;
- Stage-0 mechanics and current-route parity: passed;
- disposable Stage-0 clones: `24`, with zero authoritative Stage-0 updates;
- new training: `7 masks × 3 seeds × 512 = 10,752` Adam updates;
- resumed cells and resumed updates: `0`;
- every trained cell completed 512 Adam updates and its matching EMA schedule;
- v2 archives: `21`;
- v2 checksum markers: `21`, all independently revalidated;
- unexpected v2 artifacts: `0`;
- temporary v2 cache or authoritative-log artifacts: `0`;
- failed-run logs: `0`;
- authoritative log checksum: valid;
- final mechanics: `true`;
- execution status: `gpv1_measurements_complete`.

The sealed report is not perfectly protocol-conformant. The protocol requires
every `Q_m-Q_0`, including `Q_0-Q_0`. The helper constructing pair weights
assigns `+1` to the positive index and then `-1` to the negative index. When
both indices are zero, the second assignment overwrites the first instead of
cancelling it. Consequently the log incorrectly emits:

```text
gpv1.development.mask_0.minus_current.point=-0.61144866961158029
```

The correct mask-0 self-contrast is:

```text
point=0
bootstrap_95_low=0
bootstrap_95_high=0
seed_17=0
seed_31=0
seed_47=0
all four family effects=0
no_new_safeguard_failure=true
```

This is a strict reporting defect, so the sealed log must be read together with
this erratum. It does not contaminate the scientific verdict because:

- candidate classification explicitly iterates masks 1–7;
- every mask 1–7 contrast uses distinct positive and negative indices and was
  independently reconstructed exactly;
- current safeguards are built directly from the mask-0 evaluation, not from
  the broken self-contrast;
- main effects, all twelve simple effects, and interactions use separate
  weight constructors and independently matched the log;
- selection and confirmation recompute exactly to `none` and closed.

Retraining 10,752 updates would not add scientific information merely to
correct this unused self-comparison. Any future GPV harness version must fix
the weight helper and add a zero-self-contrast regression assertion before it
is used. The evidence-bound source and executable are intentionally left
unchanged so their hashes continue to match the sealed run.

The earlier v1 attempt produced no representation endpoint. It failed during
mandatory cache reload because one destination Tensor was reused across mixed
dtypes. Its archive and marker remain quarantined and unchanged. The v2
recovery used a fresh undefined Tensor for every field load, ran positive and
negative codec tests, rejected v1 reuse, and retrained all 21 cells. The final
log records `legacy_v1_reused=false` and every v2 cell records
`resumed=false`.

Bound evidence hashes:

- protocol: `01c6b1d9fcc95a0c831426a481c866cb196f413030d2f3c195b5219d84d57a2a`;
- codec recovery addendum: `d4989abcc4dc3f71b40bf0743c8e5ba43210165493595ca84347956d8768c2d4`;
- harness source: `7b3b4c0a368a3d64812ee2efceaa6e38c573951fe39b4de7f5ff0a38f0a8c5ad`;
- executable: `e074c06899d60a1ad660bf617d7db6e73a820fad702ee0c33cec877ea4297cea`;
- scientific manifest: `cf129ea6a6db232ad59eb7b0e67fc2801c5dcfd7655e27f6279a507f333c8eb9`;
- authoritative log: `eb0b8a5821a9aa613ae60508574d10abc18dd37155c2bc68b0ead1d8a68eef27`.

## What is now settled

- Global pooling is not the sole explanation. Replacing it with the exact
  canonical-slot-local treatment did not repair the representation.
- Direct encoder pressure from the variance term is not the sole explanation.
  Detaching that path did not reliably help.
- The projector's GELU nonlinearities materially affect representation
  learning under the current pool and variance path.
- Removing GELU raises aggregate quality but damages a protected family enough
  to fail the precommitted gate.
- Interactions are real, so these mechanisms cannot be optimized independently.
- None of the seven concrete GPV treatments is authorized for confirmation,
  checkpoint migration, or production activation.

## What remains unknown

GPV-1 does not establish that:

- every possible pool or projector design must fail;
- a purpose-built order-preserving projector could not retain mask 2's gains;
- the representation encoder is universally correct;
- the JEPA teacher target is valid;
- the current augmentations are harmless;
- a support-permitted teacher abstraction will improve representation utility.

The result also does not justify a post-result projector search. Any new
projector family would need a separately named and precommitted hypothesis,
especially because mask 2's aggregate improvement already showed how easily a
change can hide damage to sequence structure.

## Required next route

Proceed with `IMA-5 — Support-Permitted Teacher Target Alignment`, beginning
with IMA-5A only:

1. Keep the encoder, augmentations, rows, seeds, readout, and teacher checkpoint
   frozen. Perform no encoder or EMA updates.
2. Retain the exact full EMA target as a control.
3. Measure a support-zeroed target only as a deletion sanity control, not as an
   assumed training candidate.
4. Construct the primary cross-fitted legal-context conditional-residual
   teacher from calibration groups disjoint from every gate and utility row.
5. Prove by intervention that changing hidden target support cannot change the
   candidate teacher.
6. Require noncollapsed, sample-varying order/channel structure and strong
   predictor-only held-out predictability before any representation training.
7. Only if the candidate passes those gates, run the bounded paired IMA-5B
   representation A/B against a fresh frozen-teacher exact-target control.

If IMA-5A fails, close this JEPA target branch at the present representation
boundary. If it passes, test its representation value before reopening moving
EMA or augmentation attribution.

## Rollback and evidence files

No production default changed. Preserve:

- canonical rollback: `fspa4_structured_cdsb_sparse_v1`;
- operational rollback: `all_tokens`.

Evidence:

- protocol: `GLOBAL_POOL_PROJECTOR_VARIANCE_CAUSAL_DECOMPOSITION_PROTOCOL.md`;
- codec recovery addendum:
  `GLOBAL_POOL_PROJECTOR_VARIANCE_CAUSAL_DECOMPOSITION_CODEC_RECOVERY_V2.md`;
- harness:
  `quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition.cpp`;
- authoritative log:
  `.build/tests/representation_gpv1_v2_authoritative.log`;
- authoritative log checksum:
  `.build/tests/representation_gpv1_v2_authoritative.log.sha256`.
