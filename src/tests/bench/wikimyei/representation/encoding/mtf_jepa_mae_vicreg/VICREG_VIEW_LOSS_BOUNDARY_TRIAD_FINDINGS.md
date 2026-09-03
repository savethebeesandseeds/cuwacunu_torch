# VVA-1B — VICReg View–Loss Boundary Triad Findings

Date executed: 2026-09-01

## Human conclusion

VVA-1B found that independently sampled weak view pairs are not a
rescue-sized cause of the current VICReg representation deficit.

Replacing the independent weak pair with the same weak view on both branches
changed mean clean representation AULC by only `+0.000551`. Its paired 95%
interval was `[-0.001997,+0.003249]`, wholly inside the frozen `±0.005`
equivalence band. The effect was positive in only two of three seeds. The
pairing intervention is therefore practically equivalent to the current
route, and a rescue-sized pairing effect is ruled out.

Replacing the tied weak view with clean identical inputs produced a small
mean increase of `+0.003420`, with interval `[+0.000186,+0.006414]`. This is
not a supported mechanism: the point estimate is below the frozen `+0.005`
floor, and only seed 31 improved while seeds 17 and 47 became worse. Because
the interval still extends above `+0.005`, the corruption effect is imprecise
rather than proven equivalent or definitively ruled out at rescue size.

Most importantly, the clean-identical arm remained materially harmful versus
the accepted FSPA-4 representation. Its mean deficit was `-0.026129`, with
paired interval `[-0.038443,-0.013363]`, and it was worse in all three seeds.
Thus weak augmentation is not necessary for the damage. The unresolved defect
remains inside the unchanged global-pool, projector, variance/covariance loss
surface that still trains under clean identical views.

No arm was eligible, safe, or a representation rescue. Confirmation remained
sealed, no treatment was promoted, and no production default changed.

The frozen terminal decision is:

```text
mechanism_route=mixed_or_imprecise_view_effect
classification=no_candidate
promotion=none
```

## Question and isolated design

VVA-1B separated three possible explanations for the harmful VICReg result:

1. disagreement between independently sampled weak views;
2. training on weakly corrupted inputs; and
3. the unchanged global-pool, nonlinear-projector, variance/covariance
   surface that remains after the views become clean and identical.

The three arms were:

| Arm | Branch A | Branch B | Boundary isolated |
| --- | --- | --- | --- |
| `V0_current` | weak draw A | independent weak draw B | current behavior |
| `V1_tied_weak` | weak draw A | the same weak draw A | independent-pair disagreement |
| `V2_clean_identical` | canonical clean input | an identical clean clone | weak corruption removed; unchanged loss surface retained |

Both ordinary weak draws were still generated in every arm before the arm's
post-draw substitution. CPU and CUDA RNG schedules, JEPA masks, row schedules,
initialization, optimizer, EMA, evaluation rows, and all non-view parameters
were held fixed. Each branch used a separate encoder and projector call.

The experiment remained representation-module-only:

- `module_only=true`;
- `downstream_models_constructed=0`;
- `outer_augmentation_calls=0`;
- no downstream head, policy, observer, execution system, or end-to-end path
  was constructed.

## Exact representation-quality endpoints

The endpoint is the frozen clean macro probe AULC. Values below are full-sample
endpoints for each fixed seed.

| Arm | Seed 17 | Seed 31 | Seed 47 | Fixed-seed mean |
| --- | ---: | ---: | ---: | ---: |
| FSPA-4 anchor | 0.628304 | 0.647031 | 0.649311 | 0.641549 |
| `V0_current` | 0.604797 | 0.588885 | 0.640664 | 0.611449 |
| `V1_tied_weak` | 0.600365 | 0.590258 | 0.645377 | 0.612000 |
| `V2_clean_identical` | 0.596107 | 0.610303 | 0.639848 | 0.615420 |

Every trained arm was below its seed-matched FSPA-4 anchor in all three
seeds:

| Arm versus FSPA-4 | Point | Paired 95% interval | Positive seeds | Materially harmful |
| --- | ---: | ---: | ---: | --- |
| `V0_current` | -0.030100 | [-0.042395,-0.017015] | 0/3 | yes |
| `V1_tied_weak` | -0.029549 | [-0.041948,-0.016651] | 0/3 | yes |
| `V2_clean_identical` | -0.026129 | [-0.038443,-0.013363] | 0/3 | yes |

The FSPA-4 archive replayed exactly at mean `0.64154862079148123`; these are
evaluated archive endpoints, not copied constants.

## Causal attribution

The frozen causal support rule required a point estimate of at least `+0.005`,
a strictly positive paired lower bound, and positive effects in all three
seeds. Practical equivalence required the full paired interval to lie inside
`[-0.005,+0.005]`.

| Contrast | Point | Paired 95% interval | Seed 17 | Seed 31 | Seed 47 | Frozen conclusion |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| pairing: `V1-V0` | +0.000551 | [-0.001997,+0.003249] | -0.004433 | +0.001372 | +0.004713 | equivalent; rescue-sized effect ruled out |
| corruption: `V2-V1` | +0.003420 | [+0.000186,+0.006414] | -0.004257 | +0.020046 | -0.005528 | mixed/imprecise; not supported |
| complete package: `V2-V0` | +0.003971 | [+0.000309,+0.007556] | -0.008690 | +0.021418 | -0.000815 | mixed/imprecise; not supported |

The positive group-bootstrap lower bounds for `V2-V1` and `V2-V0` do not
override the fixed-seed sign gate. Their means are driven by seed 31, while
the other two seeds move in the wrong direction. Calling this a causal repair
would therefore be a post-hoc weakening of the protocol.

The attribution is deliberately asymmetric:

- independent weak-view disagreement is closed as a rescue-sized cause;
- weak corruption may have a small context-dependent effect, but it is not a
  reproducible mechanism or repair at this boundary;
- harmful clean-identical training proves that view corruption is not
  necessary for the deficit and retains the intrinsic loss-surface boundary.

This does not claim that every possible augmentation is harmless. It answers
only the exact current weak-view recipe and the exact frozen representation
quality boundary.

## Mitigation, safety, and rescue

Causal attribution was kept separate from activation safety.

Neither V1 nor V2 was candidate-eligible because neither corresponding
`Vi-V0` contrast met the causal support rule. V2's higher mean is therefore a
descriptive mitigation, not an admissible candidate.

All three arms also failed the inherited complete safeguard bundle. Relative
to FSPA-4 they failed the protected family floor, order-retention gate, and
geometry gate. Their four family deltas were:

| Arm | Multiscale state | Order/regime | Cross-channel | Future |
| --- | ---: | ---: | ---: | ---: |
| `V0_current` | -0.053575 | +0.045903 | -0.061211 | -0.051517 |
| `V1_tied_weak` | -0.063065 | +0.050348 | -0.054686 | -0.050794 |
| `V2_clean_identical` | -0.067933 | +0.054903 | -0.047041 | -0.044445 |

The decision predicates were therefore:

```text
V1 candidate_eligible=false
V1 objective_made_safe=false
V1 representation_rescue=false
V2 candidate_eligible=false
V2 objective_made_safe=false
V2 representation_rescue=false
```

There is no mitigation to activate, no safe objective, and no representation
rescue. Because the only permitted confirmation routes were
`objective_made_safe` and `representation_rescue`, confirmation correctly
remained sealed with zero rows and zero training updates.

## Mechanical validity and recovery accounting

All nine seed-by-arm cells completed exactly 512 Adam and 512 EMA updates.
Every cell passed its row schedule, mask schedule, consumed-draw schedule, RNG
schedule, treatment semantics, validity mask, finite-value, parameter
partition, and completion checks. No update was gradient-clipped.

The accepted replacement used:

```text
3 seeds × 3 arms × 512 = 4,608 accepted Adam updates
3 seeds × 3 arms × 512 = 4,608 accepted EMA updates
```

The first execution attempt had previously completed 1,536 Adam and 1,536 EMA
updates before a reporting/cache serializer type error. It produced no cache,
endpoint, contrast, candidate, confirmation, or accepted evidence. A3 froze
that attempt as discarded and allowed only a serializer/reporting repair. The
clean replacement then started from the frozen initialization and completed
independently.

Final accounting is exact:

| Ledger | Adam | EMA |
| --- | ---: | ---: |
| discarded first attempt | 1,536 | 1,536 |
| accepted clean replacement | 4,608 | 4,608 |
| lifetime physical work | 6,144 | 6,144 |
| uncommitted work in successful replacement | 0 | 0 |

The three immutable replacement caches and SHA-256 digests are:

| Seed | Cache SHA-256 |
| ---: | --- |
| 17 | `dc20c65c8d6216d14294dd523f5421d5db7ae0a6b4f3497defd67c7dc332eb38` |
| 31 | `e3ac59c306fd92b00e5333052217468b3cb858d3fb68a389955efcea79f5a61c` |
| 47 | `aa5c75e8a7fa94601b1bb8e8771991c439a1df85226094f5809a266054f72c7a` |

Every cache sidecar matches. The final immutable authoritative log has SHA-256
`c687a2d2639fabf4a737be2eb49f6a70158e21ce436c2859e45692ce8d4d9db4`.
It binds:

- source `c5a2f10be8ea2e647500c57e127eea68520e4f13de6d98dce788a812b85cb74b`;
- executable `6c68adf15914696f714c6e04449fea96906d092a7aab2f0d3e6bc17281259b49`;
- build manifest `753d6c05255a06534375500d7e98505bcfcf7c451de5ccb34418b3f9fa421c74`;
- CPU self-test `b3257ad175b3a902a9040ae5c934d2783976ec454a07cc2fdb5c1af7ed1118fb`;
- CUDA preflight `8e0d461cc8cebd31892c481848a69f6029e8551b56bc72887bbaa75e40f99ef0`;
- A3 recovery amendment `cb8a00caee454437aab19016e85c5e3caccf5259f1c5b154e2573ad40fd2c161`.

The final receipt reports `vva1b_measurements_complete`, valid Adam and EMA
accounting, three validated caches, passing custody and mechanics, and no
numeric or post-training failure.

## Independent post-run audits

Two independent read-only audits returned `GO` for accepting the completed
measurement and its `no_candidate` decision. Neither audit authorized
promotion.

The statistics audit independently reconstructed all nine endpoints, every
seed delta, all three paired bootstrap intervals, the fixed causal predicates,
candidate eligibility, selection, and confirmation decision. It found no
leakage, missing or nonfinite endpoint, post-hoc rule change, or partial-cache
contamination.

The mechanics audit independently opened and validated all three binary cache
archives, recomputed every cache and report checksum, closed the Adam and EMA
ledgers, verified the custody chain, and found no failed or temporary artifact.
It found no blocking defect.

One non-blocking hygiene note remains. The authoritative log intentionally
repeats per-seed progress keys at completed steps `{128,256,384,512}` and
prints `production_defaults_changed=false` twice; all duplicate terminal
values are consistent. The Makefile publication gate checks exit status,
terminal status, and the physical/accepted totals but does not independently
grep every cache, confirmation, and rollback receipt. Those fields are present
and valid in this sealed run. Expanding the wrapper grep set is a future
hardening opportunity, not a reason to alter or rerun this result.

## What remains unknown

VVA-1B narrows the boundary without claiming more than its frozen design can
show:

- A rescue-sized clean-view effect is not supported, but it is also not
  statistically ruled out: both V2 contrast intervals extend above `+0.005`.
- Three deliberately fixed seeds establish the result for this controlled
  experiment, not a general seed-population theorem.
- Confirmation behavior is unknown because no arm qualified to open the
  untouched confirmation rows.
- Downstream performance and other architectures are unknown by design; they
  are not evidence needed to answer this representation-only question.
- Harm under clean identical views retains the combined global-pool,
  projector, variance/covariance surface as the unresolved boundary. VVA-1B
  does not separate those internal components from one another.

## Decision and retained boundary

The production decision is intentionally conservative:

```text
selected=none
classification=no_candidate
confirmation.opened=false
promotion=none
production_defaults_changed=false
```

Rollback remains FSPA-4 with `structured_cdsb_sparse_v1`. Operational
`all_tokens` rollback also remains available.

VVA-1B closes independent view pairing as a rescue-sized explanation and
shows that removing the current weak corruption does not make VICReg safe or
useful. The next representation-only work must retain this result and address
the intrinsic global-pool/projector/variance-covariance boundary. It must not
return to downstream testing, outer-augmentation attribution, JEPA, or
predictor-capacity search to explain this particular deficit.
