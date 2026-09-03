# IMA-4A — JEPA Target Predictability Ceiling Findings

## Human conclusion

IMA-4A passed and localized a real implementation boundary.

The frozen, support-separated JEPA context contains a small amount of usable
information about the EMA teacher target, but that information is not
accessible through a shared affine context mapping or through the existing
predictor.  It becomes positive only when the readout is allowed to use a
different context-field mapping for each categorical target-token identity.

The accepted routing decision is:

`categorical_target_slot_interaction_bottleneck`

The next experiment is therefore a narrow target-relative predictor gate, not
augmentation attribution and not another mask-count experiment.

This is useful localization, not a claim that the JEPA objective is already
healthy.  The best frozen categorical oracle explains only `4.16%` of the
target-identity-centered held-out variance on average.  The existing predictor
is dramatically worse (`R2=-1.413`).  A target-relative predictor may recover
the small available signal, but the teacher target may still need redesign if
that ceiling remains too weak to help representation learning.

## What was held fixed

- FSPA-4 checkpoints for seeds `17`, `31`, and `47`.
- Encoder, tokenizer, EMA teacher, augmentations, data rows, masks, seeds,
  metrics, and group splits.
- `256` fit groups, `128` validation groups, and `256` development groups.
- M1 `paired_target_legacy_context_v1` and M2
  `support_separated_pair_v1`, with paired target selections.
- Zero optimizer steps and zero EMA updates.

No downstream model was constructed.  No representation parameter or buffer
changed.  IMA-3 caches were verified read-only and never loaded through a
train-or-resume path.

## Mechanical result

The final CUDA audit exited successfully:

- all three seed receipts passed;
- all ridge grids closed away from an unresolved edge or at the checked
  intercept limit;
- the diagnostic child modules reproduced the public tokenizer, encoder,
  target encoder, masks, predictor, and JEPA loss path;
- gradients were cleared after measurement;
- model state was bit-exact before and after;
- `optimizer_steps=0` and `ema_updates=0`.

The CPU self-test also passed.  It demonstrated that the wide dual ridge
implementation recovers a synthetic bilinear target that an affine readout
cannot, and that the categorical kernel recovers a slot-dependent target that
its affine control cannot.

## Frozen oracle ladder

R2 is measured against target-identity-centered target variance.  `R2=0`
means matching the target-identity mean baseline; negative values are worse.

| M2 surface | Mean development R2 | Meaning |
|---|---:|---|
| Existing predictor | `-1.4129` | Current JEPA predictor is far below every useful ceiling |
| Q: target metadata only | `-0.4516` | Metadata bias alone is insufficient |
| G: global context mean | `-0.3489` | A shared pooled-context mapping is insufficient |
| S: channel/domain/scale pools | `-0.4075` | Fixed structured pooling does not rescue prediction |
| F: canonical encoded field | `-0.4804` | A shared affine full-slot mapping fails |
| R: canonical tokenizer field | `-0.5318` | Raw learned tokens are not more linearly accessible |
| B: continuous query × encoded field | `-0.0872` | Target-metadata interactions nearly reach baseline but remain unusable |
| C: categorical target slot × encoded field | `+0.0416` | A small usable target-relative signal exists |

C was positive in every seed:

| Seed | B R2 | C R2 |
|---:|---:|---:|
| 17 | `-0.0483` | `+0.0740` |
| 31 | `-0.1221` | `+0.0422` |
| 47 | `-0.0913` | `+0.0086` |

The categorical control materially beat the continuous bilinear control in
all three seeds:

- `C - B = +0.1288` mean R2;
- paired group-bootstrap interval `[+0.1048, +0.1767]`;
- `C - F = +0.5220`, positive in all three seeds.

This is why the result is specifically target-slot conditioning rather than a
generic request for more predictor capacity.  C is a fixed kernel feature map,
not a trained nonlinear feature extractor, and the categorical route was
allowed only because C materially exceeded both F and B.

## What the mask receipt added

The complete `3 seeds × 512 updates × 96 rows` IMA-3 schedule was replayed
without a model update:

- `1,536` updates and `147,456` samples per arm;
- target masks, target counts, context counts, and post-mask RNG state paired
  exactly on every update;
- M2 target-support overlap was exactly zero;
- M1 exposed an average `2.667` target-overlap raw-time positions per channel;
- M2 retained about `26.668` non-target raw-time positions per channel versus
  `27.333` in M1.

M2 did not merely remove the leaking target-support tokens.  To keep the
context count at 54, it removed and replaced `1,401,282` token selections over
the full schedule: about `9.50` of 54 context tokens per sample.  The
channel/domain/scale composition therefore changed materially.  This confirms
that the IMA-3 M2 intervention was causally clean with respect to target
support, but not composition-identical to M1.

The unchanged predictor score under M1 versus M2 is still informative:
`M1 - M2 = -0.00222` R2, with interval `[-0.00272, -0.00214]`.  The existing
predictor is essentially equally poor under both masks; simply restoring
legacy overlap is not a solution.

## Teacher-target and gradient diagnosis

The teacher target is dominated by information removed with the target's own
raw support:

- mean hidden-support energy fraction: `5.240`;
- mean own-token component: `5.301`;
- mean cross-scale/domain alias component: only `0.0718`;
- hidden support dominated nearly every latent dimension;
- about `51.5%` of current prediction-residual energy projected onto the
  hidden-support component.

These quantities are normalized energies, not additive percentages; values
above one are possible.  The important comparison is that the own-token
component is large while the alias-only component is small.  Frequency targets
were especially severe and variable across seeds.

The mean hidden-support gradient norm was `3.256` times the full served
gradient norm.  This can exceed one because visible and hidden components
partly cancel.  Time- and frequency-target gradients had mean cosine about
`-0.316`, so they often ask the shared representation parameters to move in
opposing directions.  Only one of three seeds showed a harmful full-gradient
alignment with a protected probe; damage to protected tasks was not consistent
enough to declare the loss direction itself the primary boundary.

Together, these results say:

1. The current predictor lacks the target-relative mapping needed to use the
   small permissible context signal.
2. The EMA target also depends heavily on information the causal context is
   forbidden to see, which keeps the achievable ceiling weak.

## What this does and does not establish

Established:

- The context signal is not exactly zero.
- The existing JEPA predictor does not use it.
- A shared affine or continuous-metadata interaction is insufficient.
- Categorical target-slot conditioning is the smallest tested readout that
  crosses the held-out baseline.
- Mask leakage/count tuning is not the next repair.

Not established:

- That a practical trainable predictor will reach the C oracle.
- That `4.16%` target R2 is enough to improve representation learning.
- That the frozen encoder is universally correct; IMA-4A tests the JEPA
  context-to-target boundary, while earlier representation probes carry the
  broader representation-quality evidence.
- That augmentations are harmless.  Augmentation attribution remains deferred
  until this objective boundary is fixed.
- That attempt 3 is newly blinded confirmation.  Attempts 1 and 2 exposed the
  development scores; the final run is a recorded fail-closed diagnostic
  continuation.

## Next experiment

Name the next goal:

**IMA-4B — Target-Relative Predictor Sufficiency Gate**

Use the same frozen encoder, tokenizer, EMA teacher, augmentations, rows, M2
masks, seeds, and metrics.  Compare the current predictor with one minimal
target-slot-conditioned predictor under equal, bounded predictor-only compute.
Do not update the representation encoder.

The decision should be:

- if the narrow predictor approaches the C ceiling, the predictor boundary is
  confirmed; then determine whether the small ceiling is useful enough;
- if it cannot approach C, inspect its attention/optimization implementation;
- if it reaches C but JEPA remains too weak to help protected representation
  probes, redesign the teacher target to depend on support-permitted
  abstractions;
- begin augmentation attribution only after this boundary is resolved and
  held fixed.

## Evidence custody

- Final authoritative log:
  `.build/tests/representation_ima4a_v1_authoritative.log`
- Final log SHA-256:
  `eebd1e59167aad75bdfce69f5176ee4f4e040400181133e5f8f66dca3afe7606`
- Protocol SHA-256:
  `23fc3d516bfca285dda9ac901efe89acc12b29032ddcabf50020bfb0ebf77af1`
- Source SHA-256:
  `d41011207cde4f5a780c6ff96a77a59aef6c2086932874087db4d8388565867c`
- Binary SHA-256:
  `7792b9f522530b666641bb057e4d69eee060e50188b646a2907b1bfa1fc5d9f5`
- Refused attempt 1 log SHA-256:
  `8c3c3dc6d1d7f23fb3f87c40b5fcf285dc181e9e571ecbf1609193fe4621bd38`
- Refused attempt 2 log SHA-256:
  `358879f5da2e55b871a56847642bccf5db41cc8be501ec50fd071ca07fcf84cd`

Sealed inputs remained byte-identical:

- IMA-3 source:
  `97c096b5331dcf83cea4c23067dc2806ec09d03d8d9f19614c86595028196c16`
- production header:
  `93640972e497dc49f37e7690e59c2f2e55f12ece25687fe4e6f6c96b28c3c9ea`
- IMA-3 seed caches:
  - seed 17: `16bacf85d0dbffaccb5c78c0f144a504c72ed9476677f6c30cb337661a775ce2`
  - seed 31: `80cf4b15acc046f798e7b8cd3bf0e83de761fa0919eee8d583c7257553814d6c`
  - seed 47: `4f92f545043770b2671154c43d4dafbb78c4ae18fbfd8e098fd14ff86a117626`
