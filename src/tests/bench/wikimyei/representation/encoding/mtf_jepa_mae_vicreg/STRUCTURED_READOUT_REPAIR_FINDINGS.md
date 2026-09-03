# SRR-1 — Structured Readout Repair Findings

## Plain-language answer

**The structured readout repair works.** The encoder already contains useful
sequence structure that the current all-token channel average hides. Without
changing the encoder, training, or augmentations, the test-only structured
readout raised predictive AULC from `0.5193` to `0.5931` and raised reversal
decoding from `0.5745` to `0.9295`. It passed every precommitted quality gate,
and the independent auditor returned the terminal classification
`structured_readout_reproduced` with zero errors.

This result localizes a real bottleneck: averaging all tokens inside each
channel destroys information that remains present in the encoder-token surface.
The repair preserves channel, time/frequency domain, scale, and coarse
within-scale position before projecting to the same serving-shaped `[B,3,32]`
output. It does not establish that the training augmentations are correct, and
it does not authorize an end-to-end or production change. It establishes that
the readout must be repaired before another augmentation attribution can be
interpreted cleanly.

## Four-arm result

`C` is the current channel-average serving readout. `D` is the sealed canonical
CPU-float64 channel/domain/scale/bin (`CDSB`) reference from PSM-1. `R` is the
new metadata-driven shadow readout running on the original CUDA float32 encoder
output. `E` is the full ordered encoder-token reference. No arm reran or trained
the encoder.

Predictive values are fixed-seed macro AULC over twelve sequence targets and the
`32,64,128,256` probe-sample ladder. Reversal values are held-out
original-versus-exactly-reversed accuracy AULC. Intervals are deterministic 95%
paired row-bootstrap intervals. In the shuffle column, `cont` is shuffled
predictive AULC and `order` is shuffled reversal AULC; all eight controls passed.

| Arm | Predictive AULC, 95% CI | Frozen continuous contrasts | Reversal AULC, 95% CI | Shuffle controls, 95% CI | CPU identity / CUDA maximum | Frozen-gate reading |
|---|---:|---|---:|---|---|---|
| `C` current mean | 0.5193 [0.4873, 0.5426] | Baseline | 0.5745 [0.5627, 0.5865] | cont -0.1488 [-0.1819, -0.1226]; order 0.5125 [0.4933, 0.5314] | 18/18 sealed parent feature identities; CUDA translation not applicable | Order unresolved; serving-loss baseline |
| `D` canonical `CDSB` | 0.5931 [0.5716, 0.6080] | `D-C` +0.0738 [0.0556, 0.0929], material gain; `D-E` +0.0098 [0.0011, 0.0195], noninferior | 0.9295 [0.9161, 0.9418] | cont -0.1882 [-0.2260, -0.1566]; order 0.5024 [0.4867, 0.5173] | 18/18 sealed parent feature identities; CPU reference | Parent materiality, noninferiority, and order boundary reproduced |
| `R` CUDA shadow | 0.5931 [0.5716, 0.6080] | `R-C` +0.0738 [0.0556, 0.0929], material gain; `R-E` +0.0098 [0.0011, 0.0195], noninferior | 0.9295 [0.9161, 0.9418] | cont -0.1882 [-0.2260, -0.1566]; order 0.5024 [0.4867, 0.5173] | 18/18 CPU-float64 byte identities versus `D`; CUDA max `5.663e-7` <= `2e-5` | **Structured readout reproduced** |
| `E` full encoder | 0.5833 [0.5628, 0.5979] | `E-C` +0.0640 [0.0431, 0.0852], material gain; full-token reference | 0.9569 [0.9467, 0.9667] | cont -0.1883 [-0.2253, -0.1557]; order 0.5031 [0.4869, 0.5178] | 18/18 sealed parent feature identities; CUDA translation not applicable | Encoder boundary reproduced; order decodable |

The exact candidate contrasts were:

- `R-C = +0.073799267391755394`, 95% interval
  `[0.055623165307082938, 0.092895816258745281]`. All three seed deltas were
  positive: `+0.0928053`, `+0.0712054`, and `+0.0573871`.
- `R-E = +0.0097632202069557472`, 95% interval
  `[0.0010917648738582365, 0.019507133427596648]`. All three seed deltas were
  positive, and the four family deltas were `+0.0200113` multiscale state,
  `+0.0018428` order/regime, `+0.0241045` cross-channel, and `-0.0069057`
  future. Every family remained inside the frozen `-0.05` noninferiority
  margin.

The shadow and canonical `D` results differ only at the expected CUDA-float32
translation level: their predictive AULCs differ by about `7.55e-8`, while the
maximum componentwise feature error over every seed and accepted dataset was
`5.662762241342989e-7`, roughly 35 times smaller than the frozen `2e-5` limit.
The independent CPU-float64 path was byte-identical in all 18 comparisons.

## What this establishes

The current readout is not merely a little noisy. It removes most of the
sequence-direction signal exposed by this test: `C` remains below the frozen
`0.60` order threshold, while `R` reaches `0.9295` and approaches the full
encoder's `0.9569`. The predictive gain is also material and stable across all
three seeds.

The mechanism is therefore stronger than a generic “more dimensions help”
story. `R` returns the same `[B,3,32]` shape as serving; the improvement comes
from retaining the proven `CDSB` organization before compression. Shuffled
targets remained below their frozen limits, so the successful readout did not
turn the controls into apparent signal.

The gain is a macro representation boundary, not a claim that every target
family improves relative to every baseline. Against `C`, `R` improved
multiscale-state and future families strongly, improved order/regime modestly,
and reduced the cross-channel family by `0.0791`. Against the full encoder `E`,
however, all four families satisfied the frozen noninferiority rule. The proper
conclusion is that the structured readout preserves the tested representation
substantially better overall, not that it dominates every task.

## Validity and audit

The result crossed every frozen validity boundary before it was interpreted:

- the sealed authoritative log contains exactly one consumed-attempt marker;
- optimizer construction, optimizer steps, backward calls, training-loop calls,
  augmentation-launcher calls, and end-to-end calls were all zero;
- the protocol, 29-entry pre-run manifest, mechanics log, preflight log, eight
  parent artifacts, executing auditor binary, and runtime manifest binding all
  matched their sealed hashes;
- token layout, metadata cells, masks, projection, deterministic permutation and
  bootstrap tables, repeated captures, model parameters, RNG state, and device
  contracts all passed;
- canonical CPU reconstruction had maximum absolute error `0` in preflight and
  all 18 authoritative CPU identities were byte-exact;
- all 18 CUDA translation checks passed; the authoritative maximum was
  `5.662762241342989e-7`;
- all four arms passed both continuous and reversal shuffle controls.

The independent auditor closed the full `2,839`-key SRR schema with zero
unaccessed, duplicate, malformed, or non-finite machine values. It parsed
`351,232` logged prediction and target values, independently reconstructed the nine
permutation tables and bootstrap table, recomputed 96 continuous and 96 reversal
ladder points, and recomputed 21 confidence intervals from 512 bootstrap rows
each (`10,752` replicate evaluations). It also checked 1,596 endpoint fields,
1,857 sealed parent-reference fields, 54 parent feature hashes, 18 CPU byte
identities, and 18 device thresholds. Its result was:

```text
audit.error_count=0
audit.pass=true
audit.final_classification=structured_readout_reproduced
audit.failure_reason=none
```

The authoritative log deliberately labels its own pre-audit decision
`invalid_mechanics`: a scientific process is not allowed to certify its own
manifest, parent evidence, or auditor. The sealed post-run auditor supplied
those independent checks and produced the only terminal classification used in
these findings.

## Bounded interpretation and limitations

SRR-1 demonstrates the readout mechanism inside the isolated representation
module on the frozen deterministic sequence task family. It does not show that
`CDSB` is globally optimal, that sixteen cells are minimal, or that the same
effect size will transfer unchanged to real data. The scientific estimate uses
three frozen encoder seeds.

The auditor recomputed SRR metrics and bootstrap intervals from the logged raw
predictions, but it did not rerun the ridge fits or independently reproduce the
smallest-alpha tie choice. The sealed parent PSM log does not contain its raw
predictions, so parent bootstrap intervals were authenticated and compared in
full but could not be regenerated from the parent log alone.

The single-attempt proof is bounded to the sealed authoritative log: the auditor
verified exactly one consumed-attempt marker in that file, but a text artifact
cannot independently prove that no separate invocation was discarded or
overwritten outside the sealed custody chain.

No production code, optimizer, training loop, or augmentation was changed.
Consequently, this result neither clears nor condemns the augmentations. It
removes a major confound: future augmentation tests can become interpretable
only after a production implementation independently reproduces this shadow
readout.

## One next recommendation

Proceed with **SRR-2 — Production Structured Readout Parity**: in a separately
frozen module-only experiment, implement the exact metadata-driven `CDSB`
readout behind the production serving interface and require parity with this
sealed shadow on values, masks, dtype/device behavior, repeated captures, and
the same representation gates. Keep the encoder, optimizer, training loop,
augmentations, and end-to-end policy frozen throughout SRR-2.

## Evidence and authorization boundary

- Protocol: 21,848 bytes, SHA-256
  `ad7c9381d58a23e8f3cec27b59b44e6532aa561227ad22d57578cc6ba0a04946`.
- Pre-run manifest: 7,166 bytes, SHA-256
  `515c9c8a851b3aceb03c160e5c9c19fff5265774d51eb396c6d56123cf0d3acb`.
- Authoritative log: 7,324,951 bytes, SHA-256
  `f38c99ef1294dab5f40f57fff79a958cd214c593eedd10284531976cda20ae6a`.
- Audit log: 2,964 bytes, SHA-256
  `fe943fb2aa8ad26f53953364181f7c2b452692fde17643c5a8d94ca45c9bb841`.
- Executing auditor: 2,209,936 bytes, SHA-256
  `12d961e53512a8a1577a809f276d824effb154e7b38c7888b1123c0afe21c50a`.
- Parent authoritative log: 255,304 bytes, SHA-256
  `8243798d5af03d66257cbd1fd9da49a16ff7d6ba3f9e6bc54b5568dae41aa8b9`.
- Terminal classification: `structured_readout_reproduced`.

```text
training_authorized=false
augmentation_change_authorized=false
long_run_authorized=false
production_or_end_to_end_authorized=false
follow_on_production_repair_authorized=false
```
