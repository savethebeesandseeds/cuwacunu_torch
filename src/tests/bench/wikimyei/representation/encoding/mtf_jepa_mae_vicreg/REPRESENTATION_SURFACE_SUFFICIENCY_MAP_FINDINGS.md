# RSSM-1 — Representation Surface Sufficiency Map findings

Date completed: 2026-08-26

Terminal classification: `serving_pooling_loss`

## Validity

RSSM-1 is valid and complete. It ran the exact active representation module in
isolation on CUDA for the three frozen model seeds, with no optimizer, no
backward pass, no launcher augmentation, no checkpoint training, and no
end-to-end component. The attempt was consumed once, only after all data,
capture, tokenizer, projection, permutation, bootstrap, parameter, and RNG
checks had passed. The process then ended with
`execution_status=rssm_measurements_complete`.

The external auditor reproduced all 72 accepted JEPA/MAE step-zero keys and all
six legacy raw keys byte-for-byte. It found no missing, extra, or duplicate key
and no schema, UTF-8, or reference-identity error. A strict audit selected all
5,269 machine key/value records: every key was unique and every numeric value
was finite. Exactly one additional non-machine debug line contained an equals
sign and was explicitly excluded. The broader scientific audit also proved
that all twelve preflight-sealed projection/permutation/bootstrap hashes
matched and all nine emitted permutations had the correct range, cardinality,
uniqueness, and zero fixed points.

Authoritative public capture was exact on both passes; the CUDA preflight
additionally proved both-pass direct/public encoder parity. Parameter and
CPU/CUDA generator hashes were unchanged for seeds 17, 31, and 47. Dataset,
group, mask, target, counterfactual-pair, metadata, token mask, source-dtype
surface, CPU-float64 surface, and fixed-width surface identities all passed.
The tokenizer independently reproduced exactly 72 tokens, 12 clipped
full-history reversal collisions, 60 shorter-window tokens, and all 60 shorter
tokens changing.

The frozen protocol SHA-256 is
`b1554abf3ebfab5de2f263649e34688b07c8d84f5050722f60099ee65f4abc0e`.
The principal evidence is:

| artifact | bytes | SHA-256 |
| --- | ---: | --- |
| pre-run manifest | 5,269 | `1deef8e903f127cc8e57d11809cee130aebc03c32eb2a72154ff36ebf19aa250` |
| authoritative log | 422,058 | `16be35d27836fb72bbd777e1795f10433850c5108c34670fcc27f4d287059932` |
| exact reference audit | 2,709 | `a99f9bdf3dd99f310915ceeb8a75b00b9f195666e5ae7cd06ab87c79bc0de4a9` |
| full scientific post-run audit | 15,379 | `4d3e30ce67b903d6ff5bc1cb3f08585d5479e398383031b4f2fad82987b5b0b4` |
| strict machine-key audit | 743 | `f47fb801b5bd3f85ed2d1bfa50483e9cf2e81ea8d699d928d08ec234ad1ccdd1` |

## Known-gap reproduction

The old result was reproduced exactly before interpreting the new surfaces:

- equal-width legacy raw AULC: `0.6022866`;
- accepted untrained served AULC: `0.5192625`;
- served-minus-legacy-raw: `-0.0830241`, paired 95% interval
  `[-0.1115056, -0.0589042]`.

The causal RSSM input is normalized, unlike that historical raw control. Its
fixed-96 raw AULC was `0.5865929`; served-minus-normalized-raw remained a
material loss of `-0.0673304`, interval `[-0.0947728, -0.0419295]`. The known
gap therefore survives the fair normalized, equal-width comparison and is not
an artifact of the old raw preprocessing convention.

## Four surface AULCs

These are fixed-three-seed means over the labeled-sample ladder
`{32,64,128,256}`. Native preserves each surface's available width; fixed-96
uses the preregistered deterministic orthonormal projection so all four
surfaces have the same width.

| surface | meaning | native AULC | fixed-96 AULC |
| --- | --- | ---: | ---: |
| `R` | normalized raw history | 0.6770879 | 0.5865929 |
| `T` | tokenizer output before the transformer | 0.6902308 | 0.5878948 |
| `E` | encoder tokens before serving aggregation | 0.6737323 | 0.5788650 |
| `S` | current per-channel `all_tokens` mean | 0.5192625 | 0.5192625 |

Raw was computed once and reused exactly across the three paired model-seed
aggregates. Tokenizer and encoder surfaces remain close to raw in both tracks.
The large fall appears only after the encoded tokens are reduced to the served
mean.

## Transition localization

Each contrast is downstream minus upstream. The labels below use the frozen
point, interval, seed, and family rules; a numerically negative point is not
called a material loss unless every required clause passes.

| adjacent boundary | native point (95% interval) | native result | fixed-96 point (95% interval) | fixed-96 result |
| --- | ---: | --- | ---: | --- |
| tokenizer minus raw (`T-R`) | +0.0131429 `[-0.0039771, +0.0306128]` | unresolved | +0.0013019 `[-0.0166264, +0.0179075]` | unresolved |
| encoder minus tokenizer (`E-T`) | -0.0164985 `[-0.0204404, -0.0128414]` | unresolved | -0.0090298 `[-0.0197578, +0.0019807]` | noninferior |
| served minus encoder (`S-E`) | -0.1544698 `[-0.1765745, -0.1324949]` | material loss | -0.0596025 `[-0.0824881, -0.0372968]` | material loss |

Both tracks first cross a material-loss boundary at `E → S`. The gate
therefore returns `serving_pooling_loss`. It does not return tokenizer loss,
encoder-processing loss, distributed loss, or a width-only explanation.

The fixed-96 family deltas at `E → S` were `-0.1617` for multiscale state,
`-0.0304` for order/regime, `+0.1192` for cross-channel structure, and
`-0.1655` for future state. Native deltas were `-0.2590`, `-0.1845`, `+0.0373`,
and `-0.2117`. Pooling is therefore not uniformly destructive: the per-channel
mean retains or improves the tested cross-channel family while discarding much
of the multiscale, temporal, and future-state accessibility.

## Native/fixed-width agreement

The native path sees a larger `E → S` loss because it allows the pre-pool token
surface to use its full coordinate width. That alone could have produced a
misleading width advantage. The equal-width path removes that explanation:
after projecting raw, tokenizer, and encoder surfaces to the same 96
coordinates as serving, `E → S` still loses `0.0596` AULC with an interval
strictly below zero and all three seed deltas negative.

Thus the localization is stable to dimensional control. The experiment does
not merely show that 2,304 coordinates decode better than 96; it shows that
the current deterministic reduction to 96 is worse than a frozen 96-wide view
of the encoder tokens.

## Reversal behavior

The secondary reversal probe independently shows what the serving mean
forgets. Accuracy AULC is balanced original-versus-time-reversed decoding.

| surface | native order AULC (95% interval) | fixed-96 order AULC (95% interval) | status |
| --- | ---: | ---: | --- |
| raw | 0.9941 `[0.9893, 0.9980]` | 0.9556 `[0.9438, 0.9668]` | order decodable |
| tokenizer | 0.9985 `[0.9972, 0.9997]` | 0.9481 `[0.9370, 0.9592]` | order decodable |
| encoder | 0.9959 `[0.9933, 0.9982]` | 0.9561 `[0.9459, 0.9660]` | order decodable |
| served | 0.5745 `[0.5627, 0.5865]` | 0.5745 `[0.5627, 0.5865]` | order unresolved |

The served surface is not proven to be pure chance, but it falls below the
preregistered `0.60` decodability threshold while the three upstream surfaces
remain near-perfect. Balanced shuffled-label points stayed around `0.50–0.52`
and every shuffled interval passed its leakage gate.

## Controls and diagnostics

- Normalized fixed-96 raw history is informative: real-minus-shuffled AULC is
  `+0.7935754`, interval `[+0.7479351, +0.8414712]`.
- Every continuous shuffled-target AULC and every reversal-label shuffle
  passed. No target leakage classification was triggered.
- All 24 surface/track/seed semantic-versus-nuisance robustness checks were
  supported. The localization is not caused by the nuisance fixture becoming
  larger than the semantic perturbation.
- In the equal-width geometry diagnostic, mean effective-rank ratio falls from
  `0.3033` at encoder tokens to `0.1209` after serving, while mean top
  eigenvalue share rises from `0.3170` to `0.5618`. This sharp concentration is
  consistent with destructive averaging, although geometry remains diagnostic
  and does not decide the terminal label.
- All active-dimension fractions remain one. This is concentration and loss of
  accessible sequence structure, not literal zeroed dimensions.

## Bounded conclusion

The representation encoder is not the first place where this screen loses the
tested sequence information. At unchanged initialization, tokenizer and
encoder-token surfaces retain raw-like linear accessibility, including almost
perfect reversal information. The current public serving path then averages
the 24 time/frequency/scale tokens within each channel and creates the first
material loss in both fair tracks.

This conclusion is deliberately narrower than “the encoder is solved.” It is
a three-fixed-seed, synthetic-family, linear-accessibility result at accepted
step zero. It does not prove information-theoretic sufficiency, trained-market
performance, or that every alternative pool will work. Earlier same-checkpoint
replays already showed that selecting only time tokens, only frequency tokens,
or equal domain means did not recover the downstream forecast strong gate.
RSSM-1 therefore does not justify switching the production default to one of
those simple policies.

What it does change is the next engineering question: another objective,
augmentation, optimizer, or end-to-end run is premature while the module's
served interface discards information that is visibly accessible one line
upstream. RSSM-1 authorizes no training, long run, production change,
end-to-end test, or automatic follow-on repair:

```text
training_authorized=false
long_run_authorized=false
production_or_end_to_end_authorized=false
follow_on_repair_authorized=false
```

## One recommended follow-up: PSM-1

**PSM-1 — Pooling Structure Mechanism Map** should be one separately frozen,
no-training, module-only comparison derived from the same encoder tokens. Hold
the encoder, rows, targets, split identities, probes, shuffles, bootstrap
table, and fixed-96 projection constant. Compare the current per-channel
all-token mean with deterministic summaries that preserve, in order, domain,
scale, and coarse token position. Do not repeat a broad architecture or
optimizer search.

The decisive endpoint should be restoration of fixed-96 encoder-level AULC
and reversal decodability without violating shuffled controls. The first
structured summary that restores both identifies which averaging axis loses
the information; if none does, the result should stop at “fixed summaries not
sufficient” rather than authorize learned attention or training. PSM-1 must
have its own protocol and explicit authorization before execution.
