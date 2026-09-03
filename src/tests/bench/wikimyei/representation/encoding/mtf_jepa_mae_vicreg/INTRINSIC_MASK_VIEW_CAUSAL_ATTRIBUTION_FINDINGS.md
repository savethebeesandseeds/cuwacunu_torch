# IMA-1 — Intrinsic Mask/View Causal Attribution

Date: 2026-08-29

## Decision

**The current tokenizer-mask contract is infeasible.**

The required JEPA treatment cannot simultaneously retain exactly six targets,
exactly 54 contexts, and raw-support separation on the active 72-token layout.
This is a mathematical capacity failure, not a training-speed problem and not
evidence that JEPA itself is ineffective.

The precommitted Stage-0 stop gate therefore closed. No 512-update JEPA or
VICReg treatment was authorized or run.

## What was tested

The isolated Stage-0 executable used the production tokenizer and public
representation APIs. It constructed no downstream head, graph component, outer
augmentation pipeline, optimizer, or end-to-end system.

Frozen custody:

- seeds: `17`, `31`, `47`;
- FSPA-4 anchor SHA-256 values:
  - seed 17: `5d96b2961daa2bbd08a07a157ddab5debd9d4928234d8341b3961678327e9434`;
  - seed 31: `a85c00d5694d1e3f0063e8bce6fc3c2e3132a393e5bcee195909742754e76775`;
  - seed 47: `b9c2f82f26a5516069f8460095d3a2b85c482b7cd0e20db120ce2e0bbd68e392`;
- all archive schema, seed, protocol, configuration, certificate, and
  `structured_cdsb_sparse_v1` metadata checks passed;
- optimizer steps: `0`;
- EMA updates: `0`;
- maximum parameter change: exactly `0` for every anchor.

The deterministic fully-valid `[96,3,30,9]` fixture was used only for the
zero-update VICReg identity and gradient audit. The JEPA feasibility conclusion
depends only on the tokenizer geometry and is independent of input values.

## JEPA result

The active tokenizer produces 72 tokens: 36 time and 36 frequency tokens. The
current mask requests four time targets, two frequency targets, and 54 context
tokens.

The audit exhaustively evaluated every target set the current sampler can
produce:

- current target sets examined: `20,790`;
- largest available same-channel raw-support-disjoint context: `52`;
- target sets capable of supplying the required 54 contexts: `0`;
- fully conservative capacity, also excluding overlaps across channels: at most
  `12`.

Thus even the more permissive, channel-aware interpretation cannot provide 54
nonleaking contexts.

The proposed coherent target repair cannot preserve the count either:

- coherent four-time-target candidates: `14,244`;
- best time-only context upper bound: `30`;
- adding the two frequency targets cannot increase that bound.

The frozen three-seed, 512-step mask schedule confirmed what the exhaustive
proof predicts:

- samples audited: `1,536`;
- count/subset/disjoint-token mechanics failures: `0`;
- samples requiring soft-exclusion relaxation: `1,536 / 1,536`;
- samples retaining same-channel raw-support overlap: `1,536 / 1,536`;
- samples retaining an exact time/frequency raw-window alias: `1,536 / 1,536`;
- exact cross-domain alias pairs in the tokenizer layout: `42`.

The metadata describes real receptive fields: both tokenizers slice the same
raw patch `[start,start+width)` before producing their respective time and
frequency descriptors. The 42 aliases include six extra cross-scale aliases
created because the nominal 32- and 64-step windows both clip to the full
30-step history.

The target topology defect is also present in the observed schedule. Only 532
of 1,536 four-time-target blocks remained within one channel and scale; 732
crossed scales and 272 crossed channels.

## VICReg result

The zero-update triad passed at all three frozen anchors:

- `V0`, current independent weak views: branch inputs differed as expected;
- `V1`, tied weak view: branch inputs were bit-exact;
- `V2`, clean identical view: branch inputs were bit-exact;
- all sample and channel validity masks were identical and fully valid;
- `V1` and `V2` invariance loss was exactly zero;
- their tokenizer, encoder, and projector invariance-gradient norms were
  exactly zero;
- the paired-view RNG post-state was identical across treatments.

This rules out a basic paired-view identity or validity failure on the active
fully-valid branch. It does **not** prove that VICReg improves representation,
because the training triad was correctly withheld after the combined Stage-0
gate failed.

One useful diagnostic remains: across seeds and all three view treatments, the
variance loss stayed near `0.98` and its encoder/projector gradient was orders
of magnitude larger than the invariance and covariance gradients. This keeps
the global pooling/projector/variance surface as the main unresolved VICReg
boundary; it is not yet a causal quality conclusion.

## Consequence

Do not spend the planned 7,680 updates on the original IMA-1 factorial. `J01`
and `J11` cannot satisfy their precommitted count-preserving support-aware
contract, so their comparison would require silently changing the experiment.

The next isolated goal should repair the tokenizer-mask budget first. It must
choose a feasible combination of target count, context count, and receptive
field geometry, then rerun this same zero-update proof. Only a passing geometry
may open the bounded JEPA factorial and VICReg quality triad.

Rollback is explicit: production representation code and all frozen checkpoints
were left unchanged.

## Reproduction

Build and run only the isolated audit:

```text
make -C src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg -j12 ima1-stage0
.build/tests/test_wikimyei_mtf_jepa_mae_vicreg_ima1_stage0
```

Evidence hashes:

- production module: `621966eb6e21bae28a757d02d5eb9dcbf252543ec3c3477369036fc871b5e97b`;
- Stage-0 source: `cc10a1f62af91ac52e6542e2888991c0ea4957823e81bbe113d6dba4e1afb7f0`;
- Stage-0 executable: `fffd02fe77f42ac42f41fc699c238aab4ca450e3b2f9284a462ae70e7ea3ef43`.
