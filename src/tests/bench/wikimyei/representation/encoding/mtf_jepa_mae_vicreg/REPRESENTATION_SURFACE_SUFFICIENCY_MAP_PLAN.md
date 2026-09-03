# RSSM-1 — Representation Surface Sufficiency Map

**Formal name:** RSSM-1 — Representation Surface Sufficiency Map  
**Human name:** First Information-Loss Boundary Map  
**Pre-run status on 2026-08-26:** implementation, focused build, mechanical
tests, and the non-scientific CUDA preflight passed. This plan records the state
immediately before the exact pre-run seal: no RSSM scientific measurement had
been executed, so there was no localization result yet.

## The plan in one sentence

Measure the same clean sequences at raw input (`R`), tokenizer output (`T`),
encoder output (`E`), and served representation (`S`), then find the first
adjacent boundary where sequence factors stop being decodable—without training
the model or running the end-to-end system.

## Where we stood at the pre-run seal

The earlier work answered several useful questions, but not the location of the
information loss:

| Established evidence | Human meaning |
|---|---|
| equal-width raw AULC `0.6023`; accepted untrained served AULC `0.5193` | the served representation starts about `0.0830` behind raw history |
| matched outer-augmentation arms did not materially separate in clean representation quality | launcher augmentation is not presently the best explanation for this gap, even though the full profile has semantic defects |
| JEPA-only and MAE-only were each worse than the unchanged encoder; their combination was protective relative to either alone | removing either objective is not a justified repair |
| RSSM protocol and sidecar agree at SHA-256 `b1554abf...`; the transition gate and harness are implemented | the experiment definition is frozen |
| the external auditor self-tests pass for the exact 72 accepted keys and six legacy-raw keys, including deliberate corruption cases | a result can be checked independently rather than trusted because the harness emitted it |
| the focused build and consolidated mechanical suite pass | gate, tokenizer, isolated contracts, protocol pin, and auditor mechanics are executable evidence |
| the CUDA preflight passes with exact repeated capture, unchanged parameters/RNG, and zero scientific fits | the isolated encoder path is ready for the sealed measurement |
| no RSSM authoritative run exists | we have not yet learned whether the tokenizer, encoder, or serving pool is responsible |

This distinction matters: we have advanced from a broad suspicion about
training and augmentation to one precise unanswered question, but we have not
answered that final question yet.

## Ordered execution plan

The phases below are dependencies, not a menu. A failed gate stops the plan
before later and more expensive work.

| Phase | Work | Required evidence before advancing | Relative cost |
|---|---|---|---:|
| 1. Reconcile and freeze | Review the drafted harness against this plan and the protocol; correct token order, projection, ridge, seed, and audit definitions; regenerate the protocol checksum | plan, protocol, gate, and code describe the same experiment; checksum matches | very low |
| 2. Finish the isolated test surface | Complete the exact-reference auditor and any missing test-only mechanics; do not change production behavior | focused source review passes; no optimizer or end-to-end dependency exists | low |
| 3. Compile and run mechanical tests | Build only RSSM, gate, tokenizer-information, and relevant representation targets; run gate boundaries, primal/dual ridge equivalence, permutation, projection, and auditor self-tests | all tests pass; known 72-token/12-collision/60-change tokenizer contract is reproduced | low |
| 4. Run a non-scientific CUDA preflight | Capture small non-authoritative rows twice and verify public/direct parity, exact repeatability, shapes, parameter state, RNG state, projections, and counters | byte-identical capture; unchanged parameters/RNG; `optimizer_steps=0`; `backward_calls=0`; no scientific probe fit | low–medium |
| 5. Seal the run manifest | Hash the frozen protocol, relevant source, binary, accepted JMCD log, projections, permutations, bootstrap table, and preflight log | one pre-run manifest makes the upcoming command and inputs immutable | very low |
| 6. Execute RSSM-1 once | In one CUDA invocation, reconstruct seeds 17/31/47, capture all four surfaces, run native and fixed-96 probes, shuffled controls, reversal probe, and diagnostics | one complete machine-readable authoritative log; no retry merely to improve a result | medium; no training |
| 7. Audit before interpretation | Independently compare all 72 accepted step-zero keys and six legacy raw keys, recompute identities and transition contrasts, and verify leakage controls | exact reference audit and every validity gate pass | low |
| 8. Decide and report | Apply the frozen causal tree and write a findings report in plain language | one validity verdict, one surface table, one terminal classification, and at most one justified follow-up | very low |

For planning purposes, the remaining work is approximately 3–5 bounded
context-cost units: seal the manifest, execute exactly one full no-training CUDA
measurement, audit it, and report it. A context-cost unit means one focused
implementation, verification, or decision batch; it is a relative budget, not
a wall-clock or token guarantee.

### Resolved pre-run blockers from Phases 1–4

These defects were found by the independent harness audit and are recorded here
so the authoritative run is not interpreted against the earlier draft:

| Earlier blocker | Resolution now in the sealed implementation |
|---|---|
| protocol checksum was stale | protocol and sidecar now agree; the mechanical suite pins the exact checksum |
| reversal-label shuffle used one 512-row permutation and shorter prefixes | each `2n` order sample count now has its own sealed, balanced Sattolo permutation |
| identity evidence covered only part of the promised data | receipts now cover group IDs, raw/normalized data, masks, targets, normalizer, pairs, source surfaces, CPU-float64 surfaces, and fixed-width surfaces |
| scientific validity lacked the full tokenizer receipt | the live preflight and scientific gate require exactly 72 tokens, 12 clipped collisions, 60 shorter-window tokens, and all 60 changed |
| raw probes were recomputed inside each seed path | raw surfaces and probes are computed once and reused unchanged across paired seed aggregation |
| primal/dual alpha parity was checked in only one dimensional regime | both `D < n` and `D > n` fixtures require finite equivalent predictions and identical selected alphas |
| permutation output recorded only hashes | every permutation now emits row count, values, uniqueness, fixed points, and hash |
| preflight lacked the final command and authorization flags | the preflight is self-describing and denies all scientific/follow-on authorization while printing the one final command |
| exact 72-key plus six-key external auditor was missing | the read-only C++ auditor and its mutation self-tests pass |
| binaries predated the RSSM edits | a fresh focused build and consolidated mechanics run pass, followed by the CUDA preflight |

### Mandatory stop conditions

Stop without a scientific claim if any of these occurs:

- accepted step-zero or legacy raw values cannot be reproduced exactly;
- repeated capture, direct/public capture, parameters, or RNG state differ;
- the tokenizer contract, projections, primal/dual probes, or data hashes fail;
- shuffled targets appear decodable;
- the normalized raw control is not informative;
- any optimizer, backward pass, training loop, or end-to-end component enters
  the path.

Stopping here is a valid outcome: it means the measurement is unreliable, not
that the representation has passed or failed.

## What each valid result would authorize us to investigate

| First supported loss | Meaning | Single next mechanism to investigate |
|---|---|---|
| `R -> T` | the tokenizer discards or aliases useful sequence structure before the transformer sees it | `RSSM-2T — Token Construction Repair`: windows, clipping, scale coverage, positional/domain metadata |
| `T -> E` | useful information reaches the encoder but its untrained transformation makes it less accessible | `RSSM-2E — Encoder Preservation Repair`: positional treatment, attention mixing, normalization |
| `E -> S` | pre-pool tokens retain information that the exported channel mean removes | `RSSM-2S — Serving Aggregation Repair`: learned/query pooling or structured domain/scale pooling |
| a `*_family_specific_loss` result | the corresponding boundary harms exactly one information family, not the representation uniformly | a family-targeted version of that boundary's repair plan |
| native loses but fixed-96 does not | the apparent loss is driven by width/projection/sample efficiency, not clean stage-localized destruction | `RSSM-1B — Capacity-Matched Disambiguation` before redesigning the module |
| legacy raw is strong but normalized raw is not | the old raw-to-served comparison was confounded by normalization | normalization audit, followed by `AUG-1 — Augmentation Information-Preservation Map` if the fair module gap is absent |
| fixed-96 total loses but no adjacent step resolves | small losses are distributed across stages | `RSSM-1B — Cumulative Pathway Disambiguation`; do not guess one culprit |
| no valid material `S - R` gap | this run does not reproduce a causal internal deficit | stop module repair; `AUG-1 — Augmentation Information-Preservation Map` becomes the leading next test |
| valid but mixed evidence supports no terminal label | the benchmark ran correctly, but it does not isolate one mechanism | a narrowly named `RSSM-1B` disambiguation study; do not edit architecture yet |

Even after a valid localization, RSSM-1 authorizes only a proposal for one
separately frozen repair. It does not authorize implementing or training that
repair.

## Reporting contract

To keep the work understandable, every phase report must state four things in
this order:

1. **What changed:** files or evidence produced in that phase.
2. **What passed or failed:** the concrete gate, not merely “tests passed.”
3. **What we learned:** scientific conclusion, or explicitly “mechanics only.”
4. **What happens next:** the next phase and its stop condition.

The final findings report must begin with the direct conclusion in ordinary
language, followed by the four-surface AULC table and uncertainty. Detailed
hashes and mechanics belong after that conclusion, not in place of it.

## Plain-language objective

The last experiment established a large gap between the raw sequence and the
representation served by the untrained module, but it did not show where that
gap enters. RSSM-1 will inspect the same examples at each internal boundary and
ask one causal question:

> At what first stage does useful sequence information stop being linearly
> decodable: token construction, encoder processing, or serving-time pooling?

RSSM-1 is deliberately a **no-training, module-only** experiment. It constructs
no optimizer, performs no backward pass, changes no production API, and does
not run the end-to-end system.

## Why this is the next plan

JMCD-1 showed that the accepted step-zero served representation has mean AULC
`0.51926249887869513`, while the historical 96-wide raw control has AULC
`0.60228658165276872`. The gap is about `0.0830`. JEPA and MAE ablation did not
localize it: each singleton was worse than the unchanged null, and the combined
interaction was protective rather than harmful.

Before changing another loss or augmentation, we therefore need to identify
the first internal surface at which the information disappears.

One correction is essential. The historical raw control was projected before
the SSL-fitted input normalization, while the model consumed normalized input.
RSSM-1 will reproduce that value only as an audit reference. Its causal upstream
baseline will be the **normalized** raw input seen by the module.

## Question and bounded claim

The primary endpoint remains the existing four-family, twelve-target sequence
factor battery and its sample-efficiency AULC. RSSM-1 can conclusively localize
the published AULC gap if its validity controls pass.

Those twelve targets are continuous sequence factors; they are not themselves
an explicit order-classification task. A small group-paired
original-versus-reversed probe will therefore be included as a secondary test
of order information. It will not replace or override the primary endpoint.

The result will support a claim about the frozen synthetic benchmark and the
three accepted initializations only. It will not establish universal sequence
sufficiency or downstream forecasting quality.

## Surfaces to capture

For the exact accepted `C=3`, `H=30`, `F=9`, `d_model=32`, `latent=32`
configuration and seeds `17`, `31`, and `47`:

| Code | Surface | Meaning | Native flattened width |
|---|---|---|---:|
| `R` | normalized raw history | exact input consumed by the module | 810 |
| `T` | tokenizer tokens | output of token construction, before transformer processing | 2,304 |
| `E` | encoder tokens | transformer output before serving aggregation | 2,304 |
| `S` | served `all_tokens` | current channelwise mean exposed downstream | 96 |

Production creates `T` and `E` in `(domain, channel, scale, start, width)`
order. The harness audits that physical order, then explicitly regroups tokens
by channel while preserving `(domain, scale, start, width)` within each
channel. Domain-by-scale means may be reported as a diagnostic view, but they
cannot replace the full ordered surfaces because they discard within-cell
order.

Every surface is evaluated on two tracks:

1. **Native track:** use the complete flattened surface. This asks whether the
   information exists anywhere at that stage.
2. **Fixed-96 track:** reduce each channel to 32 values and flatten to 96. Raw
   uses a frozen `270 x 32` orthonormal matrix; tokenizer and encoder share one
   frozen `768 x 32` matrix; served output is already width 96. This asks whether
   an apparent advantage is only a consequence of having more coordinates.

The shared tokenizer/encoder projection is important: it makes `E - T` a fair
processing contrast rather than a comparison between unrelated projections.

## Frozen data and evaluation

RSSM-1 reuses the exact JMCD identities:

- normalizer-only groups: `0..255`;
- probe-fit groups: start `1,000,000`, `n=256`;
- validation groups: start `2,000,000`, `n=128`;
- final test groups: start `3,000,000`, `n=256`;
- sample ladder: `32, 64, 128, 256`;
- ridge grid: `1e-5` through `1`;
- ridge strength selected per target on validation only;
- four target families and twelve targets unchanged;
- the same 512 held-out-group bootstrap resamples shared across all surfaces,
  tracks, seeds, families, and controls.

The native tokenizer and encoder widths require a dual ridge solution based on
the sample Gram matrix. A unit fixture must demonstrate agreement between the
existing primal and new dual formulations in both `D < n` and `D > n` cases
before the scientific run.

## Required validity and negative controls

The authoritative result is invalid unless all of the following pass:

1. No optimizer is constructed; optimizer steps and backward calls are zero.
2. Model parameters and CPU/CUDA RNG states are identical before and after
   capture.
3. Repeated capture is byte-identical, including tokens, masks, metadata, and
   final representations.
4. Every surface uses exactly the same examples, inputs, masks, targets, and
   group pairings, verified by hashes.
5. Projection matrices are finite, deterministic, orthonormal within numerical
   tolerance, and their hashes are recorded.
6. Reconstructed seed-`17/31/47` models reproduce the accepted JMCD step-zero
   served probe and geometry keys exactly. No JMCD checkpoint exists, so failed
   reconstruction invalidates RSSM-1.
7. The legacy pre-normalization raw control reproduces its accepted AULC only as
   a continuity audit.
8. The active tokenizer plan reproduces 72 tokens: the known 12 clipped
   full-history reversal collisions and all 60 shorter-window tokens changing.
9. The normalized raw fixed-96 baseline is informative: AULC at least `0.50`
   and real-minus-shuffled bootstrap lower bound above `0.20`.
10. Independently deranged targets follow the same selection/evaluation path.
    For every surface and track, shuffled AULC must be at most `0.02`, with its
    95% bootstrap upper bound at most `0.05`.

If a validity rule fails, the output is a mechanics failure report—not a claim
about representation quality.

## Quantitative localization rule

For each adjacent transition, compute the oriented downstream-minus-upstream
contrast in both tracks:

- tokenizer construction: `T - R`;
- encoder processing: `E - T`;
- serving aggregation: `S - E`;
- total module effect: `S - R`.

Report macro AULC, the four family AULCs, a paired 95% interval, and all three
seed directions. A transition is classified as:

- **material loss:** point estimate `<= -0.02`, interval upper bound `< 0`, at
  least two of three seed deltas negative, and at least two of four family
  points negative;
- **family-specific loss:** the same macro and seed requirements, with exactly
  one family point negative;
- **material gain:** point estimate `>= +0.02`, interval lower bound `> 0`, and
  at least two of three seed deltas positive;
- **noninferior:** interval lower bound `> -0.02`, at least two of three seed
  deltas above `-0.02`, and every family point above `-0.05`;
- **unresolved:** anything else.

Material gain takes precedence if it also satisfies noninferiority.

These thresholds are frozen before results are read. Centered geometry and
semantic-versus-nuisance separation are diagnostics; they cannot override the
AULC localization. Native effective rank is normalized by `min(D, n-1)` so
surface width does not mechanically determine the comparison.

## Terminal interpretation tree

The final report will first require the causal normalized fixed-96 `S - R`
total itself to be a loss state. If that fair total gap is not present, no
tokenizer, encoder, or serving stage may be blamed, even if an adjacent
contrast looks unfavorable. Once the total gap is established, the report
assigns the earliest supported location while still showing every transition:

| Evidence | Conclusion |
|---|---|
| both tracks materially lose at `T - R` | `token_construction_loss` |
| both tracks materially lose at `E - T` | `encoder_processing_loss` |
| both tracks materially lose at `S - E` | `serving_pooling_loss` |
| both tracks have a family-specific loss at the same first stage | corresponding `*_family_specific_loss` |
| native loses at `S - E`, fixed-96 is noninferior | `prepool_width_advantage_only` |
| native and fixed-96 localize different stages | `projection_sensitive_localization` |
| fixed-96 loses at `S - R`, no adjacent transition resolves | `distributed_internal_loss` |
| normalized fixed-96 `S - R` does not lose although legacy raw does | `legacy_raw_gap_normalization_confounded` |
| no material total gap is reproduced | `no_material_surface_gap_reproduced` |
| total gap is valid but the remaining evidence pattern is unsupported | `no_terminal_interpretation_supported` |

If more than one adjacent stage loses, the report will say so explicitly; the
terminal label is a navigation aid, not a substitute for the surface table.

## Execution phases and stop points

### Phase 1 — Freeze the protocol

- Convert this plan into an immutable preregistration.
- Freeze the protocol, accepted-log identity, seed, split, projection,
  permutation, and bootstrap algorithms. Final source, binary, and generated
  projection/permutation/bootstrap hashes are recorded later, after preflight,
  in the pre-run manifest.
- Implement the pure classification gate first and exercise every boundary.
- Stop if a surface cannot be captured without a production change; RSSM-1
  authorizes test-side instrumentation only.

**Deliverables:** frozen protocol, SHA-256 sidecar, pure gate header/test.

### Phase 2 — Build the isolated harness

- Add an RSSM mode to the existing representation-quality harness so the data,
  normalizer, probes, targets, and accepted initialization path remain shared.
- Capture tokenizer output, encoder pre-pool output, and served output from the
  same batches. Call `tokenize`, `encode`, and `tokenize` again, then require
  exact token/mask/metadata equality; this avoids a production API change.
- Implement deterministic fixed-96 projections, native dual ridge, stored
  prediction reuse, shuffled-target controls, order probe, and normalized
  geometry.
- Emit machine-readable counters and hashes for every contract.

**Deliverables:** test-only harness path and focused mechanical fixtures.

### Phase 3 — Prove mechanics before spending the scientific run

- Build only the relevant representation targets.
- Run tokenizer-information, primal/dual equivalence, projection,
  shuffled-permutation, gate-boundary, and reference-auditor tests.
- Run one small CUDA preflight on non-scientific rows.
- Require `optimizer_steps=0`, `backward_calls=0`, and
  `scientific_probe_fits=0` in preflight, plus correct shapes, exact repeated
  hashes, unchanged parameters/RNG, and a fully specified upcoming scientific
  command. Mechanical solver fixtures may fit synthetic test probes and are
  not scientific fits.
- Stop and fix mechanics if any check fails. A preflight is never interpreted
  scientifically.

**Deliverables:** build receipt and preflight log.

### Phase 4 — Run one authoritative map

- Execute exactly one CUDA invocation. It must contain all three seeds, four
  surfaces, both tracks, controls, and diagnostics.
- Reuse captured tensors and predictions for contrasts and bootstrap; do not
  rerun the model for each comparison.
- Do not tune thresholds or add arms after results become visible.
- The attempt is consumed once an accepted-row probe is fit or any scientific
  endpoint is emitted. A completed interpretable run is never silently rerun.

**Deliverable:** one authoritative log with a SHA-256 receipt.

### Phase 5 — Audit and decide

- Audit the accepted step-zero reference keys, data identities, mechanics,
  projection hashes, shuffled controls, and all recomputed contrasts.
- Apply the frozen transition gate and interpretation tree.
- Mark the result invalid if any required audit fails.

**Deliverables:** exact audit output and one terminal classification.

### Phase 6 — Report in human terms, then stop

The findings report will begin with five direct answers:

1. Did the run validly reproduce the known raw-to-served gap?
2. At which first stage was information lost?
3. Was that conclusion stable across native and equal-width tracks?
4. Which sequence-information families were affected, including explicit
   reversal/order behavior?
5. What single mechanism-specific repair is justified next?

It will then show a compact surface table, uncertainty, seed consistency,
negative controls, limitations, and exact artifact hashes. RSSM-1 stops there.
Any optimizer use, objective search, architecture edit, longer horizon,
production change, or end-to-end run needs a separately named and frozen plan.

## Cost controls

- One implementation path, one preflight, and one authoritative invocation.
- No optimizer, backward pass, epoch, checkpoint, or augmentation sweep.
- Reuse the existing generator, normalization, target battery, ridge ladder,
  counterfactual examples, and accepted initialization code.
- Use exactly two validation capture passes per batch and seed—the first is
  retained and the second proves deterministic identity—then reuse the cached
  surfaces and predictions thereafter. The public-API sandwich is part of each
  pass, not an additional scientific rerun.
- Fail early on mechanics, reference, or leakage checks before expensive probe
  work.
- Do not rerun an interpretable result merely to improve confidence intervals.

## Authorization state

This document names and specifies the next plan. It does not itself authorize
training, production changes, an end-to-end experiment, or any follow-on
repair.

`training_authorized=false`

`long_run_authorized=false`

`production_or_end_to_end_authorized=false`

`follow_on_repair_authorized=false`
