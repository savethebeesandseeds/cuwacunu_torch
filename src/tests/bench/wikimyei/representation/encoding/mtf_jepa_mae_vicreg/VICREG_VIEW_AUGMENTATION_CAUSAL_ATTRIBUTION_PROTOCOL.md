# VVA-1 — VICReg View-Augmentation Causal Attribution protocol

Date frozen: 2026-08-30, before implementing or executing VVA-1 training

## Decision question and boundary

VVA-1 asks whether either module-owned corruption active in the harmful
VICReg-only OCA-1 run caused, mitigated, or interacted in that representation
quality deficit:

1. time-cell dropout, whose effective probability is
   `0.10 * 0.10 = 0.01`; and
2. Gaussian jitter, standard deviation `0.005`.

This is a representation-module-only experiment:

```text
frozen synthetic sequence rows
  -> exact MTF tokenizer and shared encoder
  -> controlled VICReg paired views
  -> unchanged global projector and VICReg loss
  -> structured_cdsb_sparse_v1
  -> fixed lightweight representation probes and geometry
```

No downstream head, graph, NodeLift, MDN/readout, observer, policy, execution
system, launcher-owned outer augmentation, or end-to-end path may be
constructed. Evaluation labels may not affect representation training.

The certified OCA-1 configuration has `mask_ratio_channel=0`. Therefore the
weak-view feature-element dropout branch has probability zero and cannot have
caused the observed OCA-1 VICReg deficit. VVA-1 records this exclusion and
does not introduce a new nonzero feature-dropout treatment. Doing so would ask
a different question and unnecessarily double the experiment.

## Frozen evidence and custody

Before any optimizer update require:

- OCA-1 authoritative log SHA-256
  `3ade025b37525c45d376eabaca2c31771ec7d23fbd2f32f26277cec0d9be606d`;
- OAA-1 findings SHA-256
  `42abd19f65f9a41ce50bed1d481ecf983750499a34a6f9d9799e232d7503a9c7`;
- IMA-1 findings SHA-256
  `ee53b9a97bf1b80153f7fd22ecf5c6dd9857cb0b3dccdb183729e5cfa05854d6`;
- the audited representation header SHA-256
  `93640972e497dc49f37e7690e59c2f2e55f12ece25687fe4e6f6c96b28c3c9ea`;
- FSPA-4 seed-17/31/47 archive SHA-256 values
  `5d96b2961daa2bbd08a07a157ddab5debd9d4928234d8341b3961678327e9434`,
  `a85c00d5694d1e3f0063e8bce6fc3c2e3132a393e5bcee195909742754e76775`,
  and `b9c2f82f26a5516069f8460095d3a2b85c482b7cd0e20db120ce2e0bbd68e392`;
- OCA-1 `anchor_challenge` completed-cache SHA-256 values
  `5290559894fa6e3f5d2fd57f32a90e97bb0eec924ec22cc9354ae26d0e629c92`,
  `bb5391840ede1ad4aaf8ff760d39b796a7a300918e1aab881fbb5255051fcc39`,
  and `aaa7dd4b7638f240d1e940db7ede3bc40eb8ad01000baa90dc043801a29256a6`.

Parse rather than infer OCA-1's `outer_augmentation_calls=0`, VICReg harmful
verdict, and completed status. Parse OAA-1's authorization of intrinsic
paired-view attribution and its conclusion that outer augmentation did not
rescue VICReg. Parse IMA-1's zero-update current/tied/clean view mechanics and
the fact that its VICReg training triad was withheld rather than executed.

Load exactly objective mask `8` from each OCA completed cache and require its
metadata, 512-step receipt, initialization, and current-view model to pass.
Load every FSPA-4 archive into a fresh model and require exact metadata and
state replay.

## Stage 0 — exact treatment and RNG proof

Use this complete two-factor inventory. A set bit means the corruption is
active:

| mask | name | time dropout scale | Gaussian jitter |
| ---: | --- | ---: | ---: |
| `0` | `clean_identical` | `0` | `0` |
| `1` | `time_only` | `0.10` | `0` |
| `2` | `jitter_only` | `0` | `0.005` |
| `3` | `current_time_jitter` | `0.10` | `0.005` |

Only the two listed configuration scalars may differ. The model architecture,
initial state, global projector, global VICReg definition and weights, all
other objective coefficients, masks, datasets, and serving policy remain
fixed. The mask-3 manifest must equal the unchanged OCA-1 VICReg manifest.

The implementation already consumes the time-dropout random tensor whenever
`mask_ratio_time>0`, even when its effect scale is zero, and always consumes
the Gaussian random tensor even when jitter standard deviation is zero.
Consequently all four profiles must have identical CPU/CUDA generator
poststates after a paired forward. Require additionally:

- exact initial parameters and exact JEPA masks across all profiles;
- mask 0 produces two bit-exact clean views;
- masks 1 and 3 have bit-exact weak-view masks;
- masks 0 and 2 retain the clean feature mask;
- masks 2 and 3 use bit-exact jitter values on the support retained by mask 3;
- zero values outside every weak-view mask and finite values everywhere;
- a separately instantiated mask-3 model has bit-exact outputs, loss,
  gradients, and RNG poststate versus the unmodified default configuration;
- feature-dropout probability resolves to exactly zero.

Any failure stops before training. No scalar or arm may be changed after this
stage.

## Stage 1 — complete 2×2 representation-quality attribution

Reuse the completed OCA-1 mask-3 VICReg-only models; do not retrain them.
Train masks 0, 1, and 2 only. For every new arm freeze:

- exact seed-matched FSPA-4 initialization;
- seeds `17,31,47`;
- exactly 512 Adam updates, learning rate `1e-3`;
- gradient clipping at norm `5.0`;
- batch size 96 and the exact OCA row schedule;
- one Adam step followed by one target-EMA update;
- objective mask `8`: `lambda_vicreg=0.05`, all other outer objective
  coefficients zero;
- unchanged global VICReg route, projector, component weights, masks,
  tokenizer, encoder, rows, normalization, structured readout, probes,
  bootstrap rows, metrics, and thresholds;
- no launcher-owned outer augmentation.

This is exactly `3 profiles × 3 seeds × 512 = 4,608` new optimizer updates.
No longer run or coefficient search is authorized.

For zero-based update `u`, all profiles use the exact OCA forward seed

```text
splitmix64(0x6f626a5f6d61736b XOR seed XOR (u << 32))
```

and the exact same retained row indices. Reset CPU and CUDA RNG immediately
before each arm's forward. Require equal pre/post RNG states, row hashes, JEPA
masks, one finite positive VICReg-only loss, finite gradients, paired clipping,
one optimizer and one EMA update, and the expected parameter partitions on
every trajectory. A mechanically invalid arm invalidates the experiment.

## Endpoints and causal contrasts

Evaluate only clean inputs with `structured_cdsb_sparse_v1`. The primary
endpoint is the existing macro probe AULC. Retain the four family scores,
reversal/order and shuffled controls, raw equal-width control, and per-channel
effective rank, participation rank, largest-eigenvalue share, and active
dimension fraction.

Use the unchanged deterministic 256-replicate paired generated-group
bootstrap table. It measures held-out generated-group uncertainty; the three
fixed seeds are paired and averaged.

Precommit these contrasts, where `Q_m` is quality for profile mask `m`:

```text
remove time from current:     Q_2 - Q_3
remove jitter from current:   Q_1 - Q_3
remove both from current:     Q_0 - Q_3

time-off main effect:   0.5 * [(Q_0 - Q_1) + (Q_2 - Q_3)]
jitter-off main effect: 0.5 * [(Q_0 - Q_2) + (Q_1 - Q_3)]
interaction:            Q_0 - Q_1 - Q_2 + Q_3
```

Report point estimate, paired 95% interval, all three per-seed values, and
four family effects for every contrast. Also report every profile against the
FSPA-4 anchor and against current mask 3.

A removal is a supported causal improvement at the current boundary only if
its AULC point is at least `+0.0025`, paired lower bound is greater than zero,
all three seed deltas are positive, no family effect is below `-0.02`, and no
new frozen safeguard failure appears. A factor has a general harmful main
effect only if the same clauses hold for its precommitted main effect. An
individual conditional removal may be supported even when an interaction
prevents the averaged main-effect gate from passing; the report must keep
those claims distinct.

## Candidate, confirmation, and stop gates

Among masks 0, 1, and 2, a candidate is eligible only if it passes the direct
current-removal gate above. Select the eligible candidate with the greatest
point improvement over current; break an exact tie by fewer active
corruptions, then lower mask id.

Classify the selected development result as:

- `representation_rescue` only if candidate minus FSPA-4 is at least
  `+0.005`, has lower bound greater than zero, is positive in all three seeds,
  has at least three positive family deltas, no family below `-0.02`, and all
  frozen safeguards pass;
- `objective_made_safe` if it materially beats current, candidate minus
  FSPA-4 has lower bound greater than `-0.005`, and all family and frozen
  safeguards pass, but the rescue clauses do not all pass;
- `view_augmentation_mitigates_harm_only` if it materially beats current but
  remains unsafe or materially below FSPA-4;
- `view_augmentation_not_causal_at_quality_boundary` if no candidate passes
  the direct current-removal gate;
- `invalid_numeric_or_mechanics` on any custody, finite, pairing, RNG,
  inventory, or update failure.

Only `representation_rescue` or `objective_made_safe` opens untouched
confirmation rows: 256 synthetic groups beginning at `9,000,000`. Confirmation
performs no training and must pass the same classification clauses. A failed
confirmation leaves no promoted treatment.

No VVA-1 outcome directly edits the production default. Preserve explicit
rollback to the FSPA-4 checkpoint and `structured_cdsb_sparse_v1`; preserve
the operational `all_tokens` rollback already established by SRR.

If no candidate confirms a safe or rescued representation, close the active
paired-view-corruption explanation and proceed next to the already implicated
global pooling/projector/variance coupling. Do not reopen JEPA, outer
augmentation, downstream compatibility, or predictor-capacity search.
