# PSM-1 — Pooling Structure Mechanism Map Findings

## Plain-language answer

The representation is not absent from the encoder. The current serving mean is
discarding structure that the downstream probes need. Keeping only channel and
time/frequency domain does not repair the loss. Keeping domain and scale
recovers much of the predictive signal and makes sequence direction barely
decodable, but it misses the frozen all-family noninferiority gate. The first
tested summary that fully restores the boundary is **channel + domain + scale +
coarse within-scale position (`CDSB`)**.

This is good news because the result identifies a concrete readout bottleneck:
the full network does not need to be redesigned before testing a better serving
pool. It does **not** clear the training augmentations; augmentation quality was
deliberately outside this experiment. It shows that the present serving pool
can erase useful representation even when the encoder tokens already contain
it, so pooling must be repaired or controlled before another augmentation
attribution can be interpreted cleanly.

## Five-arm result

All predictive values are fixed-seed macro AULC across twelve sequence targets
and the `32,64,128,256` probe-sample ladder. Intervals are deterministic 95%
row-bootstrap intervals. Reversal AULC is held-out original-versus-reversed
sequence accuracy; the frozen order-decodable threshold is `0.60` with lower
bound above `0.50` and at least two positive seeds.

| Arm | Structure retained (cells/channel) | Predictive AULC, 95% CI | Contrast versus `C` | Contrast versus full `E` | Reversal AULC, 95% CI | Frozen-gate reading |
|---|---|---:|---|---|---:|---|
| `C` | Channel only (1) | 0.5193 [0.4873, 0.5426] | Baseline | — | 0.5745 [0.5627, 0.5865] | Order unresolved; serving loss baseline |
| `CD` | Channel + domain (2) | 0.5139 [0.4793, 0.5375] | -0.0053 [-0.0121, 0.0017], noninferior | -0.0694 [-0.0936, -0.0491], unresolved | 0.5809 [0.5692, 0.5939] | Domain separation is insufficient |
| `CDS` | Channel + domain + scale (8) | 0.5871 [0.5635, 0.6042] | +0.0679 [0.0553, 0.0820], material gain | +0.0038 [-0.0096, 0.0186], unresolved | 0.6165 [0.6041, 0.6311] | Predictive and order gains, but not full-family restoration |
| `CDSB` | Channel + domain + scale + coarse position (16) | 0.5931 [0.5716, 0.6080] | +0.0738 [0.0556, 0.0929], material gain | +0.0098 [0.0011, 0.0195], noninferior | 0.9295 [0.9161, 0.9418] | **Earliest sufficient tested structure** |
| `E` | All encoder token positions (24) | 0.5833 [0.5628, 0.5979] | +0.0640 [0.0431, 0.0852], material gain | Reference | 0.9569 [0.9467, 0.9667] | Encoder boundary reproduced |

The encoder-minus-channel gain was positive for all three seeds
(`+0.0850`, `+0.0678`, `+0.0393`). Its family deltas were `+0.1701`
multiscale state, `+0.0273` order/regime, `-0.1032` cross-channel, and
`+0.1619` future. The conclusion is therefore a macro representation boundary,
not a claim that every target family improves under every structural view.

`CDS` is an informative near miss. It beat `C` materially and crossed the order
gate, but its future-family delta versus `E` was `-0.05384`, just beyond the
frozen `-0.05` family margin. `CDSB` passed the same comparison: all seed deltas
versus `E` were positive (`+0.0078`, `+0.0034`, `+0.0181`), and its four family
deltas were `+0.0200`, `+0.0018`, `+0.0241`, and `-0.0069`.

## What the reversal test adds

The predictive probe alone would say that scale-aware summaries recover most
of the signal. The reversal probe locates the missing sequence structure more
precisely:

- `C` and `CD` remain below the order-decodable threshold.
- `CDS` reaches only `0.6165`: scale identity exposes some direction, but not
  most of it.
- Adding three deterministic coarse position bins raises reversal AULC to
  `0.9295`, close to the full encoder's `0.9569`.

Thus coarse within-scale position carries the large remaining order signal.
The experiment does not claim that three bins are globally optimal; it proves
that some structure finer than domain and scale is needed, and that the tested
coarse partition is already sufficient under the frozen gate.

## Controls and validity

The measurement is valid under the frozen PSM-1 protocol:

- one authoritative attempt was consumed; it was not rerun;
- optimizer construction, optimizer steps, and backward calls were all zero;
- launcher augmentation, production execution, and end-to-end execution were
  disabled;
- direct and public encoder captures, repeated captures, token order,
  cardinality, and the exact `7,3,1,1` scale layout all matched;
- model parameters, CPU RNG, CUDA RNG, and model mode were unchanged;
- the five partitions were finite, nested, idempotent, and shape/dtype exact;
- the common mean-preserving projection was orthonormal and reproduced `C` to
  `1.22e-15` in preflight;
- the parent channel and full-encoder endpoints reproduced to the frozen
  `1e-12` tolerance, including reversal means `0.57454427083333337` and
  `0.9560546875`;
- every continuous shuffle control passed, with shuffled predictive AULC below
  zero; every shuffled reversal control stayed near chance (`0.5024–0.5125`);
- all 21 sealed pre-run artifacts matched before and after the measurement.

The independent auditor parsed `3,369` unique machine keys and `2,975` numeric
values, found zero duplicates and zero non-finite values, recomputed the five
arm summaries, seed/family contrasts, reversal inputs, validity flags, boundary,
and terminal decision, and returned the same classification:
`coarse_position_separation_sufficient`.

One audit limitation is explicit: the authoritative text log contains the
bootstrap interval endpoints but not the raw prediction tensors, so the
standalone auditor could verify that every interval input was unique, finite,
and ordered but could not regenerate the interval percentiles from raw
predictions. The deterministic bootstrap table, its hash, all point/seed/family
quantities, and every gate use of the logged endpoints were independently
checked.

## Bounded interpretation

PSM-1 establishes a mechanism within the isolated representation module and
the deterministic synthetic sequence task family used here. It supports these
claims:

1. The encoder-token surface contains materially more sequence representation
   than the current channel-mean serving surface.
2. Merely separating time and frequency domains does not recover it.
3. Domain and scale recover broad predictive information and a small amount of
   order, but fail the frozen all-family equivalence rule.
4. Adding coarse within-scale position is the earliest tested partition that
   is materially better than serving, noninferior to the full encoder, and
   decisively order-decodable.

It does not establish that the current augmentations are correct, that this is
the optimal readout for real data, that sixteen cells are minimal, or that an
end-to-end policy will improve. Those questions remain downstream of the now
demonstrated pooling bottleneck.

## One next recommendation

Proceed with **SRR-1 — Structured Readout Repair**: implement a module-only
shadow serving policy that pools encoder tokens by the proven `CDSB`
metadata cells and compresses them through the same deterministic,
mean-preserving 96-dimensional path, while freezing the encoder, optimizer,
training loop, and augmentations. Require the shadow public output to repeat
the PSM-1 gates—material gain over `C`, noninferiority to `E` in at least two
seeds and all four families, order AULC at least `0.60`, exact capture identity,
and unchanged parameters/RNG—before changing production code or returning to
augmentation experiments.

## Evidence

- Protocol SHA-256:
  `2f574310d79d581dbc39d4040d16431f8e067ae5d3583d6d4a5597b5a8ad72d3`
- Pre-run manifest SHA-256:
  `22ea52b1c31916e0da57c436917076805d5482e49e38ae1bdea62cbce31418f2`
- Authoritative log SHA-256:
  `8243798d5af03d66257cbd1fd9da49a16ff7d6ba3f9e6bc54b5568dae41aa8b9`
- Terminal classification: `coarse_position_separation_sufficient`
- Training authorized: `false`
- Production/end-to-end authorized: `false`
- Follow-on repair authorized by PSM-1 itself: `false`; SRR-1 remains a new,
  separately frozen module-only experiment.
