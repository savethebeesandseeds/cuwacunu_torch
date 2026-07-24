# MDN engine-interruption recovery amendment

Status: fixed on 2026-07-21 after the first clean isolated-source
representation run completed and after the first clean isolated-source MDN
attempt stopped, but before that MDN attempt completed, before a clean MDN
result receipt existed, and before any clean frozen-feature capture, affine
route, ablation selection, or certified-development result existed. This is
an operational recovery amendment. It changes no scientific choice.

## Interrupted attempt

The original clean MDN runtime is
`synthetic_v2_mdn_train_isolated_v2`. Its immutable input receipt binds the
isolated 3,261-anchor development source, train range `[0,2496)`, the clean
representation checkpoint, and a requested 3,500 optimizer steps. The
runtime's last report records the resolved seed 31 and objective fields, 620
optimizer steps, finite parameters, and zero non-finite outputs. The report
and job manifest jointly record zero source skips or duplicates. The
interruption sealer separately pins the objective file that supplied the
resolved objective fields. Its latest periodic checkpoint records optimizer
step 600. It has neither
`job/runtime.result.fact` nor `result.status`.

The command supervising this attempt returned Docker engine exit code 255.
That is an external orchestration observation, not a Runtime-authenticated
process exit record and not evidence about the model. The retained report,
metrics, probes, and partial checkpoint are incomplete operational artifacts.
They must not be used as scientific evidence, as a route input, as a model
comparison, or as a source of a readiness claim. This amendment makes no
claim about the interrupted process's exit status or liveness beyond the
absence of a completion fact and the sealer's ability to acquire the original
runner lock.

## Immutable interruption closure

Before a retry, the runner
`seal_and_verify_interrupted_mdn_attempt_v2.sh` must publish
`interruption.status` in the original runtime with schema
`synthetic_v2_mdn_train_isolated_v2.interruption.v1`. The closure must:

1. accept exactly the seven-directory runtime hierarchy (counting its root)
   and the fifteen pre-receipt payload files fixed by the sealer;
2. reject every unknown, symbolic-link, hard-linked, or special entry;
3. bind the exact bytes of all fifteen payload files, the original runner and
   executable, the MDN objective file, the isolated-source closure and cursor
   erratum, and the clean representation result and checkpoint;
4. verify the 620-step report, step-600 periodic checkpoint, finite checks,
   clean source range and zero skip counters, and absence of completion
   receipts;
5. atomically publish an exact-key receipt before making all sixteen files,
   including the receipt, mode `0444` and all seven directories mode `0555`;
   and
6. record the Docker exit-code observation only as external operational
   context with scientific-evidence authorization set to false.

The fifteen-file allowlist describes the interrupted payload before the
receipt is added. A sealed tree therefore contains those fifteen files plus
`interruption.status`. An existing receipt is valid only if its exact content,
tree, hashes, and modes verify; it is never overwritten.

The original runtime, partial MDN checkpoint, report, probes, log, job
manifest, and component metadata remain retained history. None may be
resumed, copied into, linked into, or otherwise reused by a retry.

## One clean retry

Exactly one recovery runtime is authorized under the distinct schema
`synthetic_v2_mdn_train_isolated_v2_retry1` at
`.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_mdn_train_isolated_v2_retry1`.
Its runner is `run_mdn_train_isolated_v2_retry1.sh`; its result schema is
`synthetic_v2_mdn_train_isolated_v2_retry1.result.v1`.

Retry 1 must verify and bind this amendment, the interruption sealer, and the
immutable interruption receipt before it creates its runtime. It must use the
same clean source closure, clean representation result and checkpoint,
executable, derived scientific configuration, architecture, objectives,
optimizer, seed 31, random-per-epoch order policy, train range `[0,2496)`, and
3,500 requested optimizer steps. Its configuration leaf, job directory, log,
input receipt, and result receipt are respectively:

```text
synthetic_benchmark.train_core_channel_mdn.isolated.retry1.config
job/
mdn_train.retry1.log
inputs.status
result.status
```

The retry must start from optimizer step zero with an empty MDN input
checkpoint. It may read only the already sealed clean representation
checkpoint as a model input. Discovery of any pre-existing retry job,
checkpoint, report, probe, log, result, or incomplete receipt fails closed; it
does not authorize another retry or in-place recovery.

This recovery changes no data, split, metric, threshold, route, or
interpretation ladder. Canonical `data/raw`, V2 final evidence, and policy
execution remain forbidden. V2 remains development-only and cannot provide
an independent final-holdout claim.
