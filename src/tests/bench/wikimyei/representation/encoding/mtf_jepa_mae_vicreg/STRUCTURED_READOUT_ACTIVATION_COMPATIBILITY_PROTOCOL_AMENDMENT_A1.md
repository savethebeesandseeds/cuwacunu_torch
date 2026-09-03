# SRR-3 Protocol Amendment A1 — Evaluation-Wave Resolution

Sealed before any SRR-3 source batch, encoder call, downstream forward, or
endpoint evaluation.

## Trigger

The first orchestration attempt passed source, checkpoint, protocol, active
policy, evaluator-self-test, and build checks, then stopped during graph-first
pipeline construction. The frozen historical base config selects
`policy_training_ppo_v0`, which the builder correctly rejects as a policy
training target. The failure occurred before source construction and before
the representation or MDN checkpoint was loaded by the capture executable.
The empty `stage_a` directory and pre-metric manifests are retained under
`.runtime/benchmarks/synthetic_continuous_graph_v1/wikimyei.mtf_jepa_mae_vicreg.srr3_activation_compatibility.v1`.

## Frozen correction

SRR-3 will use
`src/config/benchmarks/synthetic_continuous_graph_v1/srr3_activation_compatibility.config`,
SHA-256
`23d94f9222527fcc14cbc3948861b42e06b0f55c992f8e3e01b8ebc1bd8149e0`.
It is byte-for-byte line-equivalent to the frozen base config except for:

```text
runtime_wave_id = cwu_02v_certified_replay_eval_mdn
```

This is the same deterministic wave substitution used by the historical
frozen-feature capture that produced the pinned `all_tokens` baseline. It
selects MDN evaluation rather than policy training; it does not change source
registry files, anchor bounds, source order, model configuration, readout
policy, checkpoints, targets, metrics, thresholds, seeds, or compute budget.

The original base config remains pinned at SHA-256
`7c84cee94ecf839336c0383878298981b9ab362e80a570cefef20e9fed272fd6`.
The second attempt must verify both hashes and must still reproduce the
historical `all_tokens` feature probe byte-for-byte before endpoint evaluation.

## Attempt accounting

The corrected authoritative root is suffixed `attempt_000002`. Attempt 000001
performed zero encoder calls and exposed zero task endpoints, so the original
Stage-A ceiling remains six encoder calls total for the scientific attempt.
No Stage-B authority changes.

