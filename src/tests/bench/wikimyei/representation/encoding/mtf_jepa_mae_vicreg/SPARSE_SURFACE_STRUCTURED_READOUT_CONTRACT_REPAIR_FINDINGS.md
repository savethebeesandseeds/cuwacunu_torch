# SRR-4 — Sparse-Surface Structured Readout Contract Repair Findings

Date: 2026-08-28
Terminal decision: `sparse_structured_repair_qualified`

## Human conclusion

The sparse structured readout repair works on the actual graph-first
H4/H10/H30 input surface, preserves full three-channel coverage, and improves
the frozen representation-quality endpoints over `all_tokens` under the
precommitted equal-compute comparison.

This resolves the prerequisite that stopped SRR-3. The previous SRR-3 failure
was a coverage-contract failure in `structured_cdsb_v1`, not evidence that the
structured representation lacked value. The separately versioned
`structured_cdsb_sparse_v1` policy repairs that coverage boundary without
changing the public `[B,3,32]` shape, without adding learned weights, and
without changing the accepted complete-block `structured_cdsb_v1` result.

SRR-4 does **not** activate the new readout and does **not** establish frozen
MDN-head compatibility. It authorizes one fresh SRR-3 Stage A using the
qualified sparse policy. `all_tokens` remains active and is the explicit
rollback.

## What was repaired

The accepted `structured_cdsb_v1` contract requires all 24 source tokens in a
channel block. Real graph-first rows are front padded: H4 and H10 expose only
10 and 14 tokenizer-valid source tokens, so v1 correctly invalidates those
two channels. Only the complete H30 channel survives.

The appended policy `structured_cdsb_sparse_v1` retains the same frozen
channel/domain/scale/cell layout and the same fixed `Q_psm` projection. It:

1. computes each compact cell from its tokenizer-valid source tokens;
2. records source-token, compact-cell, repeated-position, and domain-scale
   support explicitly;
3. fills an unsupported compact position only with the neutral mean of
   supported positions in its own domain-scale group;
4. marks a channel computable only when all eight domain-scale groups have
   support and the upstream sample/channel masks are valid; and
5. selects the literal v1 output for every complete channel row, preserving
   accepted v1 bytes rather than relying on algebraic equivalence.

No inverse-support, square-root, or other energy rescaling was introduced.
The existing `structured_cdsb_v1` implementation remains unchanged and
fail-closed on partial blocks.

## Mechanical qualification

The focused independent oracle passed on CPU float64, CPU float32, and CUDA
float32. It covered the real suffix-derived H4/H10/H30 masks, partial and empty
cells, missing domain-scale groups, upstream-invalid rows, metadata
permutations, constant canaries, perturbation visibility, determinism, and
state/RNG purity.

A raw bit-pattern comparator was required before endpoint capture. Numeric
`torch::equal` is insufficient for a byte-exact claim because it treats
`+0.0` and `-0.0` as equal. The final mechanics test includes a signed-zero
canary, and the capture guards the raw logical bytes of the encoded object and
the converted tensors actually passed to the encoder throughout both
selectors.

Final focused result:

- 21 logical groups, zero failures;
- CPU float64, CPU float32, and CUDA float32 passed;
- signed-zero byte canary passed;
- complete-row v1 bytes were exact;
- actual source-token counts were `10/14/24`;
- supported compact-cell counts were `8/12/16`;
- repeated supported positions were `10/18/24`.

The legacy production structured-readout goldens, graph-spec policy
fingerprint, downstream adapter seam, and checkpoint-policy identity tests
also passed. The sparse policy is a distinct exact identity; a sparse-policy
checkpoint is not accepted as `structured_cdsb_v1`.

## Frozen capture custody

The runner invoked the frozen encoder once per source batch and fed the same
retained encoded object through `all_tokens` and
`structured_cdsb_sparse_v1`.

| Range | Anchors | Encoder batches/calls | Public rows | Valid cells, baseline | Valid cells, sparse |
|---|---:|---:|---:|---:|---:|
| Development `[0,730)` | 730 | 12 / 12 | 2,920 | 8,760 / 8,760 | 8,760 / 8,760 |
| Confirmation `[760,1088)` | 328 | 6 / 6 | 1,312 | 3,936 / 3,936 | 3,936 / 3,936 |

For both ranges, encoded bytes, converted encoder inputs, masks, model
parameters, buffers, eval mode, CPU RNG, CUDA RNG, config bytes, checkpoint
bytes, graph order, row/key/target identity, and adapter targets were stable.
The candidate was finite and had the same public shape and mask coverage as
`all_tokens`.

Fresh legacy probes reproduced the frozen authorities exactly:

- development SHA-256
  `d3465e44ed15647e158b9cabf00f4b1670797fdddc5539c0dd4a067db7b193ed`;
- confirmation SHA-256
  `8f6f72b78c0708b5f23512ada4ca8536ea8818f8e2d3d9bc501401d8ab0ce3c7`.

No MDN checkpoint was opened or constructed, there were zero MDN forwards,
zero optimizer steps, zero backward calls, no augmentation, no prediction
artifact, no policy change, and no access to final holdout `[1088,1170)`.

## Representation-quality result

The evaluator used the existing 96-wide edge surface—base `[32]`, quote
`[32]`, and base-minus-quote `[32]`—with deterministic per-edge CPU-float64
ridge heads. Alpha selection used fit `[0,554)`, purge `[554,584)`, and
validation `[584,730)`; each arm then refit on `[0,730)` and was compared on
the untouched historical confirmation range `[760,1088)`. Compute was equal.
Confidence intervals use 4,096 paired anchor-cluster bootstrap resamples with
the frozen PCG64 seed.

| Confirmation endpoint | `all_tokens` | Sparse structured | Paired change |
|---|---:|---:|---:|
| Directional accuracy | 0.7449 | 0.7961 | **+0.0512**, 95% CI `[+0.0305,+0.0715]` |
| Pairwise rank accuracy | 0.7236 | 0.7612 | **+0.0376**, 95% CI `[+0.0169,+0.0589]` |
| RMSE | 0.02170 | 0.02021 | ratio **0.9313**, 95% CI `[0.9083,0.9554]` |
| Correlation | 0.6273 | 0.6876 | **+0.0603**, 95% CI `[+0.0401,+0.0803]` |
| Best-asset agreement | 0.5996 | 0.6565 | **+0.0569**, 95% CI `[+0.0193,+0.0965]` |

The selected ridge alpha was `1e-7` for `all_tokens` and `1e-4` for the sparse
structured arm. A common-alpha diagnostic using the baseline alpha gave the
same conclusion: direction `+0.0522`, rank `+0.0342`, and RMSE ratio `0.9324`,
with all corresponding confidence intervals favorable. The result is
therefore not explained by giving the candidate a different alpha search
budget.

The two feature surfaces remain globally close (confirmation cosine
`0.99720`) but are not equal. This is useful evidence that the repair changes
small, task-relevant directions rather than replacing the representation with
an unrelated surface.

## Frozen gate decision

All noninferiority gates passed:

| Gate | Required | Observed bound | Result |
|---|---:|---:|---|
| Direction delta lower | `>= -0.01` | `+0.03049` | pass |
| Rank delta lower | `>= -0.01` | `+0.01694` | pass |
| RMSE ratio upper | `<= 1.05` | `0.95544` | pass |

All three materiality flags passed although only two were required:

- direction point change was at least `+0.02` and its lower bound exceeded
  zero;
- rank point change was at least `+0.02` and its lower bound exceeded zero;
- RMSE ratio was at most `0.95` and its upper bound was below one.

The terminal classification is therefore
`sparse_structured_repair_qualified`.

## What this advances—and what remains unknown

We now know that the structured readout can be made computable on the actual
sparse graph-first surface without erasing its representation advantage. This
closes the sparse-mask prerequisite that made the original SRR-3 comparison
invalid.

We still do not know:

- whether the frozen production MDN head can consume the changed feature
  semantics without adaptation;
- whether its precommitted downstream predictions improve with the readout
  switch alone; or
- if incompatible, whether a bounded equal-compute head-only adaptation
  recovers the value.

The next experiment should be a **fresh SRR-3 Stage A**, not augmentation
attribution and not a long end-to-end run. Freeze the encoder, augmentation
state, graph rows, seed, checkpoints, and endpoints; capture once; compare
`all_tokens` and `structured_cdsb_sparse_v1` through the existing frozen MDN
head. The candidate must carry a versioned readout/checkpoint identity. Only
if that old head is incompatible should the already-defined bounded
equal-compute head-only A/B be opened.

## Authority and replay

- Sealed protocol SHA-256:
  `a634a1ae386a5a0bebb10440cf0d45730e2dd652375aa3fb80a4d9277708ed30`.
- Runtime root:
  `.runtime/benchmarks/synthetic_continuous_graph_v1/wikimyei.mtf_jepa_mae_vicreg.srr4_sparse_surface_contract_repair.v1/attempt_000001`.
- Evaluation report and replay SHA-256:
  `47252fc1fc51ca8ab55db570e914a3c2f11d62bc3e6d5dc01359c4512d61fd9f`;
  the two reports are byte identical.
- Final output manifest contains 16 entries and independently replays with
  zero hash failures.
- The independent post-run audit replayed all seven custody manifests: 757
  unique referenced files, with zero malformed entries, duplicates, missing
  files, or SHA-256 mismatches. It also recomputed the split arithmetic,
  selected-alpha minima, equal solve counts, endpoint deltas, confidence
  gates, and terminal decision.

An initial orchestration preflight under Windows PowerShell 5.1 stopped on its
long-path limitation before creating a runtime root or invoking any encoder.
The runner now requires PowerShell 7 explicitly. The authoritative
`attempt_000001` above is the only SRR-4 capture/evaluation attempt.
