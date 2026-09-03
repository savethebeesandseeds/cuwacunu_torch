# PSM-1 — Pooling Structure Mechanism Map

**Human name:** The Missing Structure Test  
**Status:** proposed bounded follow-up; no PSM scientific run has been authorized
or executed  
**Parent finding:** RSSM-1 classified the first supported loss as
`serving_pooling_loss`

## Direct objective

Determine exactly how much structure the current serving-time mean removes.
Starting from the same frozen encoder tokens, restore one organization axis at
a time—domain, scale, then coarse within-scale position—and find the first
summary that recovers both sequence-factor accessibility and reversal
decodability.

This is a representation-quality experiment. It does **not** benchmark runtime
speed, train a network, compare optimizers, or run the end-to-end system.

## Why this is the next experiment

RSSM-1 established the following bounded facts on the frozen synthetic
benchmark:

- the encoder surface retained substantially more accessible information than
  the served surface;
- the fixed-96 encoder AULC was `0.5788650`, while the served AULC was
  `0.5192625`;
- the fixed-96 `S - E` difference was approximately `-0.05960`;
- encoder reversal was strongly decodable, while served reversal remained
  unresolved;
- the first supported loss boundary was therefore the operation that averages
  encoder tokens into the served per-channel vector.

That result locates the loss but does not identify **which averaging axis** is
responsible. PSM-1 answers only that question. It avoids another architecture
or training search until the missing structure is known.

## The bounded scientific question

For the accepted `C=3`, `H=30`, `F=9`, `d_model=32`, `latent_dim=32` module and
seeds `17`, `31`, and `47`:

> What is the coarsest deterministic summary of the encoder tokens that is
> statistically noninferior to the encoder reference on sequence-factor AULC
> and also makes sequence reversal decodable?

The conclusion applies only to this frozen module, data generator, target
battery, and seed set. It will not by itself establish forecasting quality or
authorize a production pooling implementation.

## What stays fixed

PSM-1 inherits the accepted RSSM-1 contract rather than creating another broad
benchmark:

- the exact model construction and the three initialization seeds;
- normalizer-only, fit, validation, and final-test group identities;
- the normalized rows, masks, targets, and original/reversed pairings;
- all twelve continuous targets and their four families;
- sample ladder `32, 64, 128, 256`;
- ridge grid, validation-only alpha selection, and R2 calculation;
- the 512 shared held-out-group bootstrap rows;
- continuous-target and balanced order-label shuffles;
- fixed output width of 96 values: 32 values per channel;
- no optimizer, no backward call, no learned pooling, and no production edit.

The existing RSSM authoritative log and receipt remain the reference evidence.
PSM must reproduce the relevant served and encoder identities before its result
can be interpreted.

## The mechanism ladder

Each channel has 24 encoder tokens: two domains, four scales per domain, and
the accepted per-domain scale counts `7, 3, 1, 1`. All candidates are computed
from one captured encoder tensor. No arm reruns the encoder.

| Code | Structure retained | Cells per channel | Human question |
|---|---|---:|---|
| `C` | channel only; mean all 24 tokens | 1 | What the current serving path exposes |
| `CD` | channel × domain | 2 | Is mixing time and frequency domains the loss? |
| `CDS` | channel × domain × scale | 8 | Is mixing scales within each domain the loss? |
| `CDSB` | channel × domain × scale × coarse position bin | 16 | Is early/middle/late position within a scale needed? |
| `E` | every ordered encoder token | 24 | Unpooled encoder reference, not a proposed summary |

The arms are nested. Moving from left to right only restores structure; it does
not change the encoder, examples, targets, or probe procedure.

### Frozen coarse-position rule

Within each `(channel, domain, scale)` cell, order tokens by the already audited
`(start, width, token_index)` order. For zero-based rank `r` among `n` tokens,
assign the bin

```text
bin = min(2, floor(3 * (r + 0.5) / n))
```

This gives symmetric early/middle/late bins for the 7-token scale, one token in
each bin for the 3-token scale, and the middle bin for a 1-token scale. The rule
is metadata-only, contains no fitted parameter, and is frozen before results.

## Equal-width construction

A comparison of 96, 192, 768, 1,536, and 2,304 raw coordinates would confuse
structure with feature width and sample efficiency. PSM therefore makes every
scientific arm exactly 96 values wide.

For each arm:

1. Average encoder tokens inside that arm's cells.
2. Lift the cell means back onto their original 24 token positions, so every
   arm again has shape `24 × 32` per channel.
3. Apply one frozen `768 × 32` projection to each channel.
4. Flatten the three projected channels to 96 values.

The projection is shared byte-for-byte by every arm and every seed. It is
constructed before any PSM endpoint is read.

### Mean-preserving projection constraint

The primary PSM projection must satisfy two properties:

- orthonormal columns within numerical tolerance;
- summing its 24 token blocks produces the `32 × 32` identity.

The second property makes the projected `C` arm mathematically identical to
the current served channel mean. Without it, a random projection could rotate
or distort the baseline and the experiment would partly measure projection
rather than pooling.

The contrast directions will be deterministically derived from the already
frozen RSSM projection, projected away from the mean subspace, orthonormalized,
and combined with the identity-preserving mean component. Its algorithm and
hash are sealed in the PSM protocol. It is not tuned against probe results.

The original RSSM projection remains an audit-only reference: applying it to
the full `E` tensor must reproduce the accepted RSSM encoder endpoint. It is not
used to choose a favorable PSM conclusion.

## What “representation performance” means

PSM has two required scientific endpoints.

### 1. Sequence-factor sample efficiency

Use the same twelve held-out sequence factors and report:

- R2 at `32, 64, 128, 256` fit groups;
- AULC, the arithmetic mean over those four sample counts;
- macro AULC, four family AULCs, all three seed directions, and a paired 95%
  bootstrap interval.

This tests whether a frozen representation makes known sequence properties
accessible to the same simple linear readout, especially with limited data.
It is the primary endpoint.

### 2. Reversal decodability

Use the same original-versus-exactly-reversed paired sequences and balanced
linear order probe. Report accuracy AULC, its paired 95% interval, and seed
directions. This verifies that a summary has not recovered only order-insensitive
statistics.

No throughput, latency, or GPU wake-up measurement enters either conclusion.

## Frozen restoration rule

First, the `E - C` contrast under the PSM projection must reproduce the
qualitative RSSM boundary:

- `E` materially improves continuous AULC over `C`; and
- `E` is order-decodable while `C` is not already equivalent to `E`.

If that boundary is absent, PSM stops with
`encoder_boundary_not_reproduced`; no pooling axis is blamed.

For `CD`, then `CDS`, then `CDSB`, in that order, a candidate is called
**restored** only when all of the following hold:

1. candidate-minus-`E` is noninferior on continuous AULC;
2. candidate-minus-`C` is a material gain on continuous AULC;
3. the candidate passes the frozen order-decodable rule;
4. both continuous-target and order-label shuffle controls pass;
5. every mechanics and identity gate remains valid.

The inherited quantitative classifications are:

- **material gain:** point estimate at least `+0.02`, interval lower bound
  above `0`, and at least two of three seed deltas positive;
- **noninferior:** interval lower bound above `-0.02`, at least two of three
  seed deltas above `-0.02`, and every family delta above `-0.05`;
- **order-decodable:** accuracy AULC at least `0.60`, interval lower bound above
  `0.50`, and at least two seeds above `0.50`;
- **continuous shuffle pass:** shuffled AULC at most `0.02` and interval upper
  bound at most `0.05`;
- **order shuffle pass:** shuffled accuracy point at most `0.55` and interval
  upper bound at most `0.60`.

These rules are evaluated mechanically. The report cannot promote a visually
appealing curve that fails them.

## Terminal decision tree

The first restored arm is the answer:

| First restored arm | Terminal result | Plain-language meaning |
|---|---|---|
| `CD` | `domain_separation_sufficient` | Keep time and frequency summaries separate; later distinctions are unnecessary on this test |
| `CDS` | `domain_scale_separation_sufficient` | Domain alone is insufficient; separate scale summaries retain the missing information |
| `CDSB` | `coarse_position_separation_sufficient` | Averaging within a scale erases useful early/middle/late sequence structure |
| none, while `E` is valid | `fixed_summaries_not_sufficient` | The tested fixed summaries cannot replace ordered encoder tokens; fine position or token interactions remain necessary |

Additional non-terminal mismatches are reported explicitly:

- AULC restored but reversal unresolved:
  `factors_restored_order_not_restored`;
- reversal restored but AULC remains inferior:
  `order_restored_factors_not_restored`;
- the old RSSM encoder endpoint cannot be reproduced:
  `reference_reproduction_failure`;
- a shuffle, identity, capture, or numerical check fails:
  `invalid_mechanics`.

Mixed results do not authorize selecting an arm by preference.

## Ordered execution plan

Each phase is a gate. A failure stops later, more expensive work.

| Phase | Work | Evidence required to advance | Cost |
|---|---|---|---:|
| 1. Freeze protocol | Convert this plan into an immutable protocol; pin parent RSSM log/receipt, data identities, binning, projection construction, thresholds, seeds, shuffles, bootstrap table, and exact command | protocol and SHA-256 sidecar agree; no scientific output exists | very low |
| 2. Implement pure mechanics | Add a pure decision gate and unit tests for every boundary; implement partition/lift/projection utilities in the isolated benchmark only | exhaustive gate cases; exact cell counts; nesting, idempotence, mean identity, projection orthogonality, and full-`E` identity pass | low |
| 3. Extend isolated harness | Add PSM preflight and authoritative modes to the existing representation harness; capture `E` and `S` once and derive all arms in memory | no production file changed; counters prove zero optimizer/backward/training/end-to-end calls | low–medium |
| 4. Focused build and preflight | Build only the PSM gate, representation harness, and reference auditor; run mechanical tests and one small non-scientific CUDA capture | exact repeated capture, public/direct parity, unchanged parameters/RNG, accepted token metadata, zero scientific fits | low–medium |
| 5. Seal manifest | Hash protocol, sidecar, sources, binary, parent evidence, projection, permutations, bootstrap rows, preflight log, and final command | one immutable pre-run manifest; all four authorization flags remain false except the explicit one-run PSM authorization | very low |
| 6. One authoritative run | One CUDA invocation for all three seeds; capture each required dataset once per seed; derive all arms; fit real/shuffled continuous and reversal probes | one complete machine-readable log and receipt; no result-driven retry | medium, no training |
| 7. Independent audit | Recompute accepted reference keys, identities, AULCs, intervals, gates, and terminal decision from the sealed log | external audit passes; emitted and independently calculated result agree | low |
| 8. Human findings report | Lead with the answer, then show the arm table, uncertainty, controls, limitations, and one bounded recommendation | reader can see what changed, what passed, what was learned, and what is or is not authorized | very low |

## Cost-control rules

- One encoder capture supplies every pooling arm; arms never cause another
  forward pass.
- Capture only fit, validation, test, and their reversal views. Do not repeat
  unrelated robustness or end-to-end suites.
- Run mechanical CPU tests before starting CUDA.
- Use one small preflight and at most one authoritative scientific invocation.
- Store predictions once and reuse them for family summaries and all 512
  bootstrap contrasts; never rerun the model during bootstrap.
- Do not add an attention pool, learned query, augmentation arm, optimizer, or
  training duration to PSM-1.
- Do not rerun a valid result because it is surprising or unfavorable.

## Mandatory stop conditions

Stop without a scientific claim if any of the following occurs:

- accepted RSSM served or encoder reference evidence is not reproduced;
- token metadata does not give exactly 24 tokens per channel, two domains, and
  per-domain scale counts `7, 3, 1, 1`;
- partition nesting, cell counts, lift behavior, mean preservation, or full-`E`
  identity fails;
- repeated capture differs, or model parameters/RNG state change;
- any optimizer step, backward call, training loop, augmentation launcher, or
  end-to-end component enters the path;
- the projection, permutation, or bootstrap hashes differ from the manifest;
- a continuous or reversal shuffle control fails;
- the PSM `E - C` boundary does not reproduce the encoder advantage.

A stopped experiment means the measurement is not trustworthy. It does not
mean pooling passed.

## What PSM-1 may authorize

PSM-1 may authorize only a proposal for one separately frozen serving-summary
implementation:

- `CD`: concatenate or deterministically combine per-domain summaries;
- `CDS`: preserve per-domain, per-scale summaries;
- `CDSB`: preserve coarse within-scale position as well;
- none: investigate a separately specified order-aware pool, without assuming
  that learned attention is automatically the answer.

Any proposed production change must first demonstrate the same representation
result in an implementation-level parity test, then receive separate authority
for downstream or training evaluation. PSM-1 itself changes no production API.

## Reporting contract

Every phase report and the final findings report must say, in this order:

1. **What changed** — exact artifact or evidence produced.
2. **What passed or failed** — named gate and concrete value.
3. **What we learned** — scientific conclusion, or explicitly “mechanics only.”
4. **What happens next** — next phase and its stop condition.

The final report begins with one ordinary-language sentence such as:

> Preserving domain and scale was enough to recover the encoder's sequence
> information; averaging those groups together was the damaging operation.

or:

> None of the fixed summaries recovered both sequence factors and order, so we
> have not yet justified a replacement pooling design.

Hashes and implementation receipts follow that conclusion; they do not replace
it.

## Definition of done

PSM-1 is complete when exactly one of these has been audited and reported:

1. the earliest restored structure level is identified;
2. no tested fixed summary is sufficient while the encoder reference remains
   valid; or
3. a named validity failure prevents a scientific conclusion.

At that point the work stops. No architecture edit, learned pooling experiment,
training run, augmentation change, or end-to-end benchmark is implicitly
authorized.
