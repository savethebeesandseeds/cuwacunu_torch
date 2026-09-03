# Isolated representation-quality findings

Date: 2026-08-25

These results concern the MTF-JEPA-MAE-VICReg module itself. They exclude the
graph, downstream readout/MDN, source pipeline, checkpoints, reports, and
launcher-owned input augmentation. The tested encoder architecture and input
shape match the active `H=30,F=9,D=32` configuration, and training uses the
active core objective, Adam learning rate, and 96 flattened model rows per
step. Unless a result is explicitly labeled weak-view-off, the module's
internal VICReg weak views remain active: 1% effective time-cell dropout and
Gaussian noise with standard deviation `0.005`.

## Tokenizer information boundary

The focused reversal test compares a deterministic non-palindromic sequence to
its time reversal.

```text
raw maximum absolute difference                         23.3361667248
full-history time-descriptor maximum difference          1.33e-15
full-history frequency-descriptor maximum difference     1.51e-15
full-history projected-token maximum difference          1.78e-15
```

With the active H30 scale plan, all 12 clipped scale-32/64 full-history tokens
collide under reversal. All 60 shorter-window tokens change for this fixture.
This proves that the full-history token branches cannot retain order/phase; it
does not claim that the complete multiscale token set is order-blind.

## Corrected three-seed quality screen

The screen uses disjoint self-supervised, probe-fit, ridge-selection, and final
test groups. Frozen ridge probes are selected only on validation data. The
primary surface is the 96-wide active `all_tokens` output. Controls are the
same-seed encoder initialization and a deterministic 96-wide orthonormal raw
history projection. The synthetic future targets are causal; an earlier
channel-lead fixture was rejected and is not included in these findings.

Command:

```bash
CUBLAS_WORKSPACE_CONFIG=:4096:8 \
  ./.build/tests/quality_wikimyei_mtf_jepa_mae_vicreg_representation \
  --tier fast --device cuda --seeds 3 --steps 128
```

The equal-width raw control reached AULC `0.6022866` and final macro R2
`0.7134757`. Per-seed trained-versus-initialization AULC deltas were:

```text
seed 17   -0.0131350
seed 31   -0.0440529
seed 47   -0.0398904
```

The paired 512-replicate group bootstrap, averaging the fixed three model
seeds, produced:

```text
trained - initialization AULC 95% interval   [-0.0422170, -0.0222587]
trained - raw-96 control AULC 95% interval   [-0.1435861, -0.0893420]
```

All three training losses improved substantially, so this is not an optimizer
mechanics failure. Nevertheless, all trained representations failed the true
centered-covariance geometry guard. Across seeds/channels, trained effective
rank fractions were approximately `0.038-0.049`, and largest-eigenvalue shares
were approximately `0.908-0.967`. Initialization was also low-rank, but less
concentrated: effective-rank fractions `0.096-0.155` and largest-eigenvalue
shares `0.414-0.687`.

## Training-length characterization

For seed 17 on the bounded split, extending training from 128 to 512 steps
reduced the final loss window from `0.31049` to `0.25994`, but AULC moved from
`0.49716` to `0.49228`. At 512 steps the trained-minus-initialization AULC delta
was `-0.01802`, and the largest eigenvalue explained `95.5-98.1%` of each
channel's activation variance. Longer proxy optimization therefore did not
repair the representation in this screen; rank concentration progressed.

## Internal weak-view attribution

The corrected screen above is the `outer-off / weak-view-on` arm. Therefore it
already proves that launcher-owned augmentation is not necessary for the
observed collapse, but it does not distinguish the internal VICReg weak views
from the remaining encoder/objective path.

A matched seed-17 localization reran the active data tier for 128 steps with
only the internal weak-view strengths switched off. Both arms consumed the
same weak-view random draws, preserving the subsequent RNG schedule. The raw
control AULC (`0.6510321`), untrained AULC (`0.5789723`), initialization, input
groups, batches, JEPA masks, and optimizer settings were identical.

| Arm | trained AULC | trained - initialization | top-eigenvalue share, C0/C1/C2 |
| --- | ---: | ---: | --- |
| outer off, weak off | 0.5686120 | -0.0103603 | 0.917431 / 0.910796 / 0.962171 |
| outer off, weak on | 0.5687374 | -0.0102349 | 0.917557 / 0.910799 / 0.962123 |

The weak-view-on minus weak-view-off AULC effect was only `+0.0001254` and the
collapse geometry was effectively unchanged. This one-seed paired
localization does not qualify the weak-view recipe generally, but it rules out
the active internal jitter/dropout as the primary cause of this failure. The
core encoder/objective path still fails with both explicit input/view
augmentation layers absent. JEPA/MAE context-target masking remains active in
both arms and must be localized separately; it is part of the training signal,
not evidence against an augmentation-related cause in the broader sense.

That next localization has now been completed and is recorded in
`REPRESENTATION_OBJECTIVE_MASK_ATTRIBUTION_FINDINGS.md`. Across three paired
seeds, JEPA/MAE-only rescued the full objective by `+0.016244` AULC with paired
95% interval `[+0.010152,+0.022615]`. A triggered split found independent AULC
costs from TF alignment (`-0.008834`) and VICReg (`-0.006030`) when each was
added to JEPA/MAE. VICReg was the dominant incremental rank-collapse pressure;
TF alignment improved rank geometry while reducing probe sample efficiency.
JEPA/MAE itself remained geometrically inadequate. Allowing all non-target
context did not rescue the full objective.

The launcher-owned stack was qualified separately in
`jkimyei/training/channel_graph_first_launchers/MTF_AUGMENTATION_SEMANTIC_FINDINGS.md`.
Its full active profile was not semantically qualified. Frequency masking
damaged order/coupling, while dilation and warp damaged support and the causal
terminal anchor. A candidate containing only Gaussian jitter, amplitude scale,
and frequency gain jitter passed the bounded transform-only gates, but has not
yet been shown to improve a trained representation.

## Supported conclusion and remaining scope

The current active served representation has useful decodable information, but
the tested self-supervised training does not improve it as a general sequence
representation. Under this isolated workload, training reduces overall labeled
sample efficiency relative to both initialization and an equal-width raw
control, while concentrating activation variance into very few directions.

This is strong failure evidence for the bounded module-only screen, not a full
production qualification. The release claim remains disabled until the wider
protocol's PCA, shuffled-target, family-level interval, five-seed, and canonical
training-budget controls are completed. The next engineering investigation
should redesign the VICReg-to-served-representation pressure and normalize or
delay the initially dominant TF-alignment gradient in module-only experiments
before changing downstream components or starting another end-to-end run.

That mechanism-repair screen has now been executed and is recorded in
`REPRESENTATION_OBJECTIVE_REPAIR_FINDINGS.md`. Gradient-matched TF warmup fixed
the intended initial scale mismatch but failed AULC/noninferiority and geometry
gates. Projected per-channel-stratified VICReg removed the global-pooling
loophole but also failed AULC/noninferiority and worsened served geometry. No
combined or longer arm was authorized; the production recipe remains
unchanged.
