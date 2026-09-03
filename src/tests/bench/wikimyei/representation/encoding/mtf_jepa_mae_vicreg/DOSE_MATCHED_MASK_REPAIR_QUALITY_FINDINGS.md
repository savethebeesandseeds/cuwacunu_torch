# IMA-3 — Dose-Matched Mask Repair Quality Findings

## Human conclusion

IMA-3 completed successfully as an experiment, but the proposed mask repair did
not improve the learned representation.

The leakage found by IMA-2 is real: the leaky control exposed raw support from
the target interval in every audited sample, while the repaired mask exposed
none. However, removing that leakage did not restore representation quality.
The repaired arm was slightly worse than its exactly dose-matched leaky control
on average and was worse than the frozen FSPA-4 anchor on all three seeds.

Therefore:

- do not activate `support_separated_pair_v1` as a representation-quality
  repair;
- retain `legacy_soft_overlap` as the explicit rollback/default;
- do not spend another long run tuning mask overlap or target count;
- inspect the JEPA target/predictor/EMA learning mechanism next.

This advances the diagnosis: masking was a plausible cause and has now been
causally tested. It is not the repair-sized cause of the representation damage.

## Isolated question and arms

The encoder, FSPA-4 checkpoint, data rows, seeds, weak views, optimizer, update
budget, objective coefficients, structured readout, probes, and bootstrap table
were frozen. No downstream model was constructed and no outer augmentation was
called.

Only two new arms were trained for 512 updates on seeds 17, 31, and 47:

| Arm | Target | Context |
|---|---|---|
| M1 `paired_target_legacy_context_v1` | One exact finest-scale time/frequency pair | Overlapping raw support remains eligible |
| M2 `support_separated_pair_v1` | The identical pair selected with the identical RNG schedule | All same-channel raw-support overlap is excluded |

The already-cached six-target legacy JEPA arm (M0) was reused and not retrained.
Total new authorized work was `2 × 3 × 512 = 3072` optimizer updates.

## Zero-update causal gate

The gate ran before any optimizer was constructed:

- scheduled comparisons: 1,536;
- audited samples: 147,456;
- identical M1/M2 target masks: 1,536/1,536 updates;
- identical post-mask RNG state: 1,536/1,536 updates;
- exact two-target/context-count contract: 1,536/1,536 updates;
- intentionally different context masks: 1,536/1,536 updates;
- M1 samples containing target-support overlap: 147,456/147,456;
- M2 samples containing target-support overlap: 0/147,456;
- anchor initialization, cached M0 custody, and config isolation: exact;
- no-training forward/finiteness/weak-view smoke: passed.

This proves that the quality comparison changed the intended causal variable,
not target dose, rows, initialization, weak views, or random schedule.

## Representation-quality result

The endpoint is the existing sparse structured sequence representation and its
sealed frozen-probe AULC. Higher is better.

| Representation after 512 JEPA updates | Seed 17 | Seed 31 | Seed 47 | Mean | Mean change from anchor |
|---|---:|---:|---:|---:|---:|
| Frozen FSPA-4 anchor | 0.628304 | 0.647031 | 0.649311 | **0.641549** | — |
| M0 cached legacy six-target | 0.580148 | 0.646983 | 0.670133 | **0.632422** | −0.009127 |
| M1 paired target, leaky context | 0.610660 | 0.644667 | 0.645829 | **0.633719** | −0.007830 |
| M2 paired target, separated context | 0.610899 | 0.632816 | 0.645536 | **0.629750** | **−0.011798** |

The decisive contrasts were:

| Contrast | Mean AULC change | Paired bootstrap 95% interval | Positive seeds | Decision |
|---|---:|---:|---:|---|
| M2 repaired − M1 dose-matched leaky | **−0.003968** | [−0.012967, +0.005143] | 1/3 | No support-separation benefit |
| M1 paired-target − M0 six-target | +0.001297 | [−0.006466, +0.008851] | 1/3 | No target-dose/topology benefit |
| M2 total repair − M0 legacy | −0.002671 | [−0.012215, +0.006590] | 1/3 | No total repair benefit |
| M2 repaired − frozen anchor | **−0.011798** | **[−0.020955, −0.002616]** | 0/3 | Reliably worse than the starting representation |

The precommitted effect floor was +0.005 AULC with a positive paired lower
bound and at least two positive seeds. Neither support separation, paired-target
dose, nor the combined repair met it.

## What the broader gates show

All training mechanics passed: losses and gradients were finite, the served
encoder and JEPA predictor changed, the target EMA moved, inactive MAE/VICReg
heads stayed unchanged, and all completed seed caches were committed.

The failure is therefore about what JEPA learned, not whether optimization ran.
In particular:

- M1 changed cross-channel representation quality by −0.07347 relative to the
  anchor and failed the geometry gate;
- M2 changed cross-channel quality by −0.10648, failed the geometry gate, and
  also failed the temporal-order retention gate;
- M2's order AULC change versus the dose-matched M1 arm was −0.01888 with a
  fully negative 95% interval [−0.02979, −0.00798].

Removing visible target support made the JEPA prediction task harder, as
intended, but the learned updates did not become more sequence-semantic. They
damaged cross-channel and order information instead.

## What is now ruled out

1. **Mechanical mask leakage as the main cause.** It exists, but removing it
   does not recover quality.
2. **Six targets versus two paired targets as the main cause.** The
   dose/topology change was only +0.00130 and statistically unresolved.
3. **A failed optimizer or dead JEPA branch.** Predictor, served encoder, and
   EMA updates were all nonzero with finite gradients.
4. **Outer augmentation or downstream interference in this result.** Neither
   entered the executable.

## Remaining unknown and next recommendation

The remaining high-value question is why the JEPA update itself moves a good
FSPA-4 representation away from cross-channel and temporal-order information.
The next bounded module-only milestone should be:

**IMA-4 — JEPA Target-Dynamics and Predictor Causal Decomposition**

Hold the now-resolved mask boundary fixed and localize, before another long
run, whether harmful gradients originate from:

1. the predictor mapping/context aggregation;
2. the target-latent definition and stop-gradient boundary;
3. target-network EMA dynamics; or
4. the latent regression loss emphasizing local reconstructability rather than
   sequence-level information.

Begin with zero-update gradient-direction and target/prediction geometry
measurements at the frozen anchors. Open a bounded training A/B only for the
single mechanism that those measurements identify. Do not return to downstream
or end-to-end training for this diagnosis.

## Artifacts and custody

- Harness:
  `quality_wikimyei_mtf_jepa_mae_vicreg_dose_matched_mask_repair.cpp`
  (`97c096b5331dcf83cea4c23067dc2806ec09d03d8d9f19614c86595028196c16`)
- Representation header:
  `mtf_jepa_mae_vicreg.h`
  (`93640972e497dc49f37e7690e59c2f2e55f12ece25687fe4e6f6c96b28c3c9ea`)
- Built executable:
  `50398f4746638d234110e72bb6b6b77c773ec1cc1a04a8f30c6124cb4ea76f7e`
- Seed 17 cache:
  `16bacf85d0dbffaccb5c78c0f144a504c72ed9476677f6c30cb337661a775ce2`
- Seed 31 cache:
  `80cf4b15acc046f798e7b8cd3bf0e83de761fa0919eee8d583c7257553814d6c`
- Seed 47 cache:
  `4f92f545043770b2671154c43d4dafbb78c4ae18fbfd8e098fd14ff86a117626`

An initial completed seed-17 run exposed a cache-writer integer-type mismatch
after training and was not retained. The mismatch was corrected, the changed
source was rebuilt, the full zero-update gate was rerun, and all three reported
seed runs then completed with durable checksum-marked caches.

Final machine decision:
`ima3.decision=mask_repair_has_no_material_representation_effect`.
