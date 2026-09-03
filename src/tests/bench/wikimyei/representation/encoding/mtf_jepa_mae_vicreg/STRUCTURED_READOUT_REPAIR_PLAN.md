# SRR-1 — Structured Readout Repair

**Human name:** The Readout Replacement Test  
**Status:** approved module-only follow-up; execution remains gated by a sealed
protocol and preflight  
**Parent finding:** PSM-1 classified
`coarse_position_separation_sufficient`

## Plain-language objective

Replace the destructive all-token channel mean in a test-only shadow path with
the deterministic structure that PSM-1 proved sufficient: channel, domain,
scale, and coarse within-scale position (`CDSB`). Then establish whether that
implementation preserves the same representation quality when it runs through
a realistic CUDA/dtype path.

This experiment is intentionally narrow. It does not search for a new encoder,
train anything, alter augmentations, or change the production serving API. It
asks whether the already demonstrated `CDSB` mechanism can be implemented as a
reliable readout.

## What is already known

PSM-1 isolated the current serving mean as a representation bottleneck. On the
frozen three-seed benchmark:

| Surface | Continuous AULC | Reversal AULC | Reading |
|---|---:|---:|---|
| current channel mean `C` | `0.5193` | `0.5745` | useful encoder structure is lost |
| `CDSB` summary | `0.5931` | `0.9295` | first tested sufficient summary |
| full encoder `E` under `Q_psm` | `0.5833` | `0.9569` | valid encoder reference |

`CDSB-C` was a material continuous gain and `CDSB-E` was noninferior under the
frozen PSM gates. The large reversal improvement showed that coarse position,
not merely domain or scale, carries the missing order information.

SRR-1 does not reopen that mechanism search. Its job is to reproduce the proven
mechanism in an implementation-shaped shadow readout.

## The bounded scientific question

For the same accepted module, data, targets, seeds, probes, controls, and
projection as PSM-1:

> Can a metadata-driven, production-shaped `CDSB` shadow readout produce the
> same canonical CPU-float64 features, survive CUDA float32 translation, and
> retain the material predictive gain and reversal decodability established by
> PSM-1?

There are two separate burdens of proof:

1. **Implementation identity:** the shadow policy must derive the same cells
   and exact canonical CPU-float64 features as the sealed PSM `CDSB` arm.
2. **Representation quality:** its CUDA/dtype output must pass the same
   continuous and reversal gates against the current served `C` surface and
   the full encoder `E` reference.

A quality result cannot excuse an identity failure, and an identity result
cannot excuse a quality failure.

## What stays frozen

SRR-1 freezes all upstream representation production:

- encoder architecture, initialization, weights, and model seeds `17,31,47`;
- tokenizer, token metadata, token masks, and encoder public `encode()` path;
- optimizer state, training loop, losses, and target-encoder behavior;
- every augmentation function and launcher path;
- production serving policies, enum values, configuration schema, and public
  output types;
- the PSM `Q_psm` construction and hash;
- normalizer, fit, validation, test, and reversed group identities;
- twelve continuous targets, four families, sample ladder, ridge grid,
  validation selection, bootstrap rows, shuffles, and decision thresholds;
- output width: three channels by 32 values, flattened to 96 only for probes.

No production header or implementation is changed during SRR-1. The readout is
a test-only free function with no parameter, module, optimizer, or RNG state.

## The shadow readout

The shadow consumes the public encoder output, its masks and token metadata,
plus the already frozen `Q_psm` projection. It returns a serving-shaped value
tensor `[B,3,32]` and channel-valid mask `[B,3]` on the same device and in the
same floating dtype as the encoder embeddings.

For each channel, it performs the following deterministic operations:

1. Validate and sort tokens by metadata, not by the incidental incoming token
   order.
2. Within every `(domain, scale)` group, rank tokens by `(start, width)`.
3. Assign rank `r` of `n` to
   `min(2, floor(3 * (r + 0.5) / n))`.
4. Average each nonempty coarse-position cell.
5. Lift the 16 cell means back to the canonical 24 token positions.
6. Flatten the `24 x 32` channel tensor and multiply by the frozen
   `768 x 32` `Q_psm` matrix.

The accepted metadata layout has two domains and per-domain scale counts
`7,3,1,1`, producing exactly 16 nonempty cells per channel. The readout does
not contain learned attention, a fitted projection, an arm-specific rotation,
or a result-dependent choice.

## Why retain the lift and projection

Returning all 16 cell vectors directly would widen the representation and
change the consumer contract. The PSM lift-plus-projection path keeps exactly
32 values per channel and was constructed to preserve the channel mean while
retaining deterministic contrast directions. Reusing it means SRR-1 tests the
readout implementation rather than changing both structure and output width.

The projection remains common to the canonical `CDSB` reference, the shadow
readout, the channel baseline, and the full encoder reference. Its frozen
stable hash is `ac8a43fd65b2c8a8`.

## Four minimal arms

All arms come from the same captured public encoder output. No arm causes
another model forward pass.

| Code | Arm | Purpose |
|---|---|---|
| `C` | current public all-token channel serving pool | damaging baseline |
| `D` | canonical offline CPU-float64 PSM `CDSB` construction | exact mechanism reference |
| `R` | metadata-driven shadow readout on CUDA in encoder dtype | implementation under test |
| `E` | full ordered encoder tokens under `Q_psm` | upper reference used by the PSM gate |

`D` must reproduce the sealed PSM evidence before `R` is interpreted. `R` is
not compared with a newly tuned reference.

## Success rule

SRR-1 succeeds only if all of the following are true:

1. parent PSM evidence and all sealed hashes are intact;
2. mechanics, capture identity, metadata layout, masks, projection, parameter
   state, RNG state, and zero-training counters are valid;
3. the canonical CPU-float64 shadow path is byte-identical to offline `D` for
   every accepted row, dataset, and seed;
4. CUDA/dtype `R`, converted only for comparison, is within the frozen
   componentwise translation tolerance of canonical `D`;
5. `R-C` is a material continuous gain;
6. `R-E` is noninferior or a material gain on continuous AULC;
7. `R` is order-decodable;
8. all continuous-target and order-label shuffle controls pass; and
9. an independent auditor recomputes the same terminal result.

The inherited PSM thresholds remain literal:

- **material gain:** point `>= +0.02`, lower interval `> 0`, and at least two
  of three seed deltas `> 0`;
- **noninferior:** lower interval `> -0.02`, at least two seed deltas
  `> -0.02`, and every family delta `> -0.05`;
- **order-decodable:** reversal accuracy AULC `>= 0.60`, lower interval
  `> 0.50`, and at least two seed AULCs `> 0.50`;
- **continuous shuffle:** point `<= 0.02` and upper interval `<= 0.05`;
- **order shuffle:** point `<= 0.55` and upper interval `<= 0.60`.

## Ordered module-first execution

Each milestone is a stop gate. Later work does not begin when earlier evidence
is invalid.

| Milestone | Work | Evidence required to advance | Cost |
|---|---|---|---:|
| 1. Freeze SRR contract | Pin parent evidence, shadow API, metadata rule, mask semantics, projection, tolerances, arms, controls, gate, command, and attempt boundary | protocol and SHA-256 sidecar match; no accepted-row fit exists | very low |
| 2. Implement the test-only shadow | Add a metadata-driven free function returning `[B,3,32]` and `[B,3]` without changing production | source lives only under the isolated benchmark; no parameter/module/RNG | low |
| 3. Prove pure mechanics | Test cell assignment, permutation behavior, masks, lift, projection identity, dtype/device, determinism, and rejected layouts | all CPU mechanics and threshold-boundary tests pass | low |
| 4. Extend isolated harness | Capture public encoder output and derive `C,D,R,E` in memory | public/direct parity, two identical captures, zero training/end-to-end calls | low–medium |
| 5. CUDA preflight | Use only non-scientific groups to test production-like translation | exact CPU64 `D` parity, CUDA error within tolerance, zero scientific fits | low–medium |
| 6. Seal pre-run manifest | Hash protocol, parent evidence, new sources/tests, binary, preflight, deterministic tables, and exact command | one immutable manifest; authorization flags remain false | very low |
| 7. One authoritative run | One CUDA invocation across the same three seeds and six dataset views | complete machine-readable log; one consumed attempt; no retry for outcome | medium, no training |
| 8. Independent audit | Recompute identities, endpoints, controls, contrasts, and terminal gate | external result agrees with emitted result | low |
| 9. Human findings | Explain the answer, evidence, limitation, and one next action | reader can tell whether the repair works and what remains unauthorized | very low |

## Required mechanics coverage

Before any accepted-row probe fit, tests must establish:

- exact accepted cell IDs
  `0,0,1,1,1,2,2,3,4,5,6,7,8,8,9,9,9,10,10,11,12,13,14,15`;
- exactly 24 tokens and 16 nonempty cells per channel, with scale counts
  `7,3,1,1` in each of two domains;
- global token-permutation invariance when embeddings, masks, and every
  metadata field are permuted together;
- invariance to reorderings within a cell and sensitivity to moving a token
  across a bin boundary;
- canonical lift shape and row-major token/latent flattening;
- byte-exact CPU-float64 equality with the PSM `CDSB` implementation;
- constant-token mean preservation through `Q_psm`;
- output shape, dtype, device, contiguity, finiteness, and mask contract;
- fail-closed partial-mask behavior and safe invalid-channel zeroing;
- rejection of missing tensors, wrong dimensions, wrong projection, invalid
  metadata IDs, duplicate ordering keys, wrong domain/scale counts, and empty
  required cells;
- repeated output identity, unchanged parameters and RNG, and zero optimizer,
  backward, augmentation, training, or end-to-end calls.

## Cost controls

- Build only the new mechanics test, SRR harness, and auditor.
- Run CPU mechanics before any CUDA capture.
- Use one small preflight with no scientific targets or probe fits.
- Capture each scientific dataset twice only for identity; retain the first.
- Derive all four arms from each retained capture.
- Store predictions and reuse them for family summaries and all 512 bootstrap
  rows; bootstrapping never reruns the model.
- Allow at most one consumed authoritative invocation.
- Do not add learned pooling, attention, another projection, another seed,
  another dataset, an augmentation arm, training duration, or end-to-end work.
- Do not rerun a valid unfavorable result.

## Mandatory stop conditions

Stop without claiming a readout repair if:

- a parent PSM file, hash, classification, feature identity, or required
  endpoint cannot be reproduced;
- the accepted metadata does not describe exactly the frozen 72-token layout;
- any CPU-float64 `D` feature differs from the sealed PSM `CDSB` feature;
- the CUDA shadow exceeds the frozen translation tolerance;
- capture repetition, public/direct parity, projection identity, masks,
  parameters, RNG, deterministic tables, or finite checks fail;
- an optimizer, backward call, training loop, launcher augmentation, production
  serving edit, or end-to-end component enters the path;
- a sealed source, binary, permutation, bootstrap table, or command differs
  from the manifest;
- a shuffle control fails; or
- the shadow fails one of the inherited representation gates.

The first eight cases make the measurement invalid. The last case is a valid
negative result: the shadow implementation did not reproduce the demonstrated
quality.

## Terminal decision tree

Invalidity takes precedence over scientific interpretation.

| First applicable condition | Terminal result | Plain-language meaning |
|---|---|---|
| local mechanics, capture, deterministic, control, or zero-training contract fails | `invalid_mechanics` | the measurement cannot be trusted |
| sealed PSM evidence is missing or changed | `parent_evidence_failure` | the claimed baseline is not the approved parent result |
| canonical offline `D`, `C`, or `E` does not reproduce PSM | `offline_reference_failure` | SRR did not reconstruct the experiment it claims to implement |
| CPU identity passes but CUDA/dtype translation exceeds tolerance | `device_translation_failure` | the implementation changes too much in its realistic execution path |
| all validity gates pass but `R` fails a quality gate | `readout_gate_failure` | the tested shadow readout is valid but does not preserve the proven representation result |
| every validity and quality gate passes | `structured_readout_reproduced` | the `CDSB` repair is implementation-ready for a separately authorized production proposal |

No mixed or visually favorable outcome may bypass this ordering.

## What SRR-1 may authorize

`structured_readout_reproduced` may authorize only a separate proposal to add
the same deterministic structured readout to production. It does not itself
authorize that production edit, downstream integration, training, a long run,
or augmentation evaluation.

After a separately reviewed production parity change, augmentation attribution
can resume against both the old and repaired readout. Until then, augmentation
conclusions remain confounded by the demonstrated pooling bottleneck.

## Human reporting contract

Every progress report and the final findings must state, in this order:

1. **What changed** — exact artifact or measurement produced.
2. **What passed or failed** — named gate and concrete value.
3. **What we learned** — implementation identity, representation result, or
   explicitly “mechanics only.”
4. **What happens next** — next bounded milestone and its stop condition.

The final report begins with one ordinary-language sentence, for example:

> The structured shadow readout reproduced the proven encoder information on
> CUDA, so the current mean can now be replaced in a separately reviewed
> production change.

or:

> The canonical readout was correct, but its CUDA implementation did not retain
> the same output closely enough, so no production change is justified yet.

Hashes and receipts follow the answer; they never replace it.

## Definition of done

SRR-1 is complete when exactly one terminal classification has been independently
audited and reported, with one bounded recommendation. At that point this
experiment stops. No production modification, training run, augmentation
experiment, or end-to-end benchmark is implicitly authorized.
