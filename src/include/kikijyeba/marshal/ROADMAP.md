# Marshal Roadmap

Marshal is the coordination layer between Lattice proof/advice and Runtime
execution. Its job is deliberately narrow:

```text
Lattice proves and advises.
Marshal validates and prepares one Runtime handoff.
Runtime executes and writes evidence.
Lattice re-reads the evidence and proves again.
```

Marshal is not a proof authority, not a scheduler, and not a model selector.
A Marshal receipt explains what was requested; it never satisfies a lattice
target by itself.

## Current State

The current Marshal packet is complete for the v1 dispatch surface.

```text
M0  complete: roadmap boundary and finite dispatch list
M1  complete: deterministic dispatch advice schema
M2  complete: Runtime Hero dry-run handoff adapter
M3  complete: execution gate with verified prior dry-run binding
M4  complete: dispatch receipts and replay audit
M5  complete: Codex-assisted wrappers over deterministic primitives
M6  complete: operator status surface
M7  complete: independent batch preview
M8  complete: scheduler research packet
M9  complete: MCP/CLI schema compatibility hygiene
M10 complete: Marshal performance budget smoke
M11 complete: receipt retention and compaction policy

PBB complete:
  execution safety, Runtime handoff binding, SHA-256 digests, shared advice
  freshness/provenance checks, concrete model-state input checks, execution
  receipt digests, receipt replay integrity, canonical prior dry-run evidence
  integrity, and final Runtime decoded-wave model-state binding.

Target lookup complete:
  hero.marshal.lookup_target_advice asks Lattice Hero plan_target for a target
  id and materializes explicit Marshal advice/request objects.

Signed advice complete:
  optional local signed Lattice advice/proof receipt verification exists.

Remaining blockers for this Marshal packet:
  none known.
```

Future work should be opened as a new finite goal, not appended as a broad
"finish Marshal" objective.

## Operating Model

The operational path for pursuing a lattice target is:

```text
1. Pick a lattice target id.
2. Ask Lattice Hero to plan the target.
3. Marshal materializes the Lattice plan into explicit advice and request
   objects.
4. Symbolic PLAN_INPUT_* hints are resolved to concrete runtime checkpoint
   paths.
5. Runtime Hero's active wave is checked against the advised wave.
6. Marshal performs a dry-run handoff.
7. Execution, if requested, must pass the execution gate using the same accepted
   dry-run evidence and a matching confirmation token.
8. Runtime Hero executes and writes manifests, reports, facts, and checkpoints.
9. Lattice Hero re-evaluates the target from runtime evidence.
```

The important boundary is that step 9 is the proof. Steps 2-8 are planning,
validation, dry-run, handoff, and audit.

## Wave Shape Rule

Marshal does not invent the wave. The wave shape comes from the target's
`LATTICE_PLAN` clause in `kikijyeba.lattice.targets.dsl`.

```text
WAVE_TARGET -> suggested_wave.target
WAVE_MODE   -> suggested_wave.mode
WAVE_RANGE  -> suggested_wave.source_range + anchor bounds
PLAN_INPUT_* -> suggested_wave.plan_inputs
PLAN_MAX_ATTEMPTS -> plan recommendation budget
```

When `WAVE_RANGE = split:<name>`, Lattice resolves `<name>` through
`kikijyeba.lattice.splits.dsl`. For example:

```text
split:train_core         -> anchor_index [0,1600)
split:validation_holdout -> anchor_index [1800,2050)
split:test_holdout       -> anchor_index [2100,2247)
```

Marshal then checks that Runtime Hero's decoded active wave has the same:

```text
target_component_family_id
mode
source range
anchor begin/end
source_key begin/end, when SOURCE_RANGE=source_key
required PLAN_INPUT_* model-state paths
```

If any of those differ, Marshal refuses the handoff.

## Runtime Wave Ownership

Runtime Hero owns the active wave because it is the component that reads
`kikijyeba.settings.wave.dsl` immediately before dry-run or execution. Lattice
may advise a desired wave and Marshal may compare that advice to Runtime's
decoded wave, but neither Lattice nor Marshal silently edits or reinterprets the
active wave.

Canonical active-wave fields are:

```text
wave_id
target_component_family_id
mode
source_range
anchor_index_begin/end
source_key_begin/end
source_order
job_kind
train_target
model_state_inputs
```

`marshal_expected_wave` uses the same canonical names when Marshal calls Runtime
Hero. `wave_target` and `wave_mode` are compatibility aliases only; new Marshal
handoffs should send `target_component_family_id` and `mode`.

## Model-State Inputs

`PLAN_INPUT_*` fields are runtime model-state inputs, not contract identity.

Examples:

```text
PLAN_INPUT_REPRESENTATION_CHECKPOINT
PLAN_INPUT_MDN_CHECKPOINT
```

Lattice plans may carry symbolic hints such as:

```text
latest_satisfying:vicreg_train_core_ready
latest_satisfying:channel_mdn_train_core_no_test_leakage
```

Marshal treats those as unresolved. Before dispatch, they must become concrete
absolute checkpoint paths under an allowed model-state root. Runtime Hero must
also decode the same concrete inputs from the active wave. This keeps checkpoint
loading as runtime state, while contract identity remains stable.

## Safety Rules

Marshal is dispatchable only when all of these are true:

```text
- target status is unsatisfied but dispatchable
- plan_basis is available
- suggested_wave is present and finite
- suggested_wave range matches plan_basis range
- active contract, target spec, split policy, and runtime_root match context
- Lattice advice is fresh and from an allowed Lattice tool
- required PLAN_INPUT_* values are concrete absolute paths
- Runtime Hero policy allows the requested dry-run or execution mode
- Runtime Hero active wave matches the advised wave
- execution has verified canonical prior accepted dry-run evidence for the same
  request
- execution confirmation token binds the exact request
```

Marshal refuses when evidence is stale, symbolic, widened, shifted,
contradictory, missing, or policy-forbidden.

## Tool Surface

Marshal Hero exposes the current v1 primitives:

```text
hero.marshal.status
hero.marshal.lookup_target_advice
hero.marshal.prepare_target_dispatch
hero.marshal.validate_advice
hero.marshal.dry_run_dispatch
hero.marshal.execution_gate
hero.marshal.replay_receipt
hero.marshal.batch_preview
```

`lookup_target_advice` is a convenience wrapper. It does not dispatch. It asks
Lattice Hero for a target plan and returns explicit advice/request objects for
the deterministic path.

`prepare_target_dispatch` is the target-first operator wrapper. It asks Lattice
for the plan, asks Lattice to resolve symbolic `latest_satisfying:*`
model-state hints, validates the materialized Marshal advice, inspects Runtime
policy and active-wave shape, and returns one operator packet. It does not
dry-run unless `include_runtime_dry_run=true`, and it never executes.

## Direct Invocation Sketch

Plan a target through Lattice:

```bash
/cuwacunu/.build/hero/hero_lattice.mcp \
  --global-config /cuwacunu/src/config/.config \
  --tool hero.lattice.plan_target \
  --args-json '{"target_id":"channel_mdn_validation_eval_ready"}'
```

Prepare the target-first operator packet:

```bash
/cuwacunu/.build/hero/hero_marshal.mcp \
  --global-config /cuwacunu/src/config/.config \
  --tool hero.marshal.prepare_target_dispatch \
  --args-json '{
    "target_id":"channel_mdn_validation_eval_ready",
    "requested_mode":"dry_run",
    "materialize_plan_inputs":true,
    "include_runtime_dry_run":false,
    "source_lattice_timestamp":"2026-05-24T00:00:00Z",
    "context":{
      "freshness_check_timestamp_utc":"2026-05-24T00:00:00Z"
    }
  }'
```

If the returned packet is blocked, follow `next_action`. Once the packet is
ready, validate/dry-run through:

```text
hero.marshal.validate_advice
hero.marshal.dry_run_dispatch
```

Execution is a separate step through:

```text
hero.marshal.execution_gate
```

Runtime execution still goes through Runtime Hero policy.

## Target-First Operator Packet

`hero.marshal.prepare_target_dispatch` implements the bounded simplification
goal. The operator-facing phrase is:

```text
pursue lattice target <target_id>
```

The tool name remains "prepare" because it performs one read-only preparation
step. It does not imply scheduling, retries, dependency traversal, config
editing, checkpoint selection, or execution ownership.

The default packet is small and target-first. A representative blocked response
for `channel_mdn_validation_eval_ready` looks like:

```json
{
  "ok": true,
  "tool": "hero.marshal.prepare_target_dispatch",
  "target_id": "channel_mdn_validation_eval_ready",
  "target_status": "metric_failed",
  "dispatch_state": "blocked",
  "blocker_bucket": "unresolved_model_state",
  "explanation": "Lattice advises the MDN validation-eval wave for anchors 1800..2050. Runtime is aligned on target, mode, and range, but two checkpoint inputs are still symbolic and must be resolved before dry-run.",
  "suggested_wave": {
    "target": "wikimyei.inference.expected_value.mdn",
    "mode": "run|debug",
    "source_range": "anchor_index",
    "anchor_index_begin": 1800,
    "anchor_index_end": 2050
  },
  "model_state_inputs": [
    {
      "key": "PLAN_INPUT_MDN_CHECKPOINT",
      "symbolic_hint": "latest_satisfying:channel_mdn_train_core_no_test_leakage",
      "concrete_path": null,
      "status": "unresolved",
      "resolution_owner": "lattice"
    },
    {
      "key": "PLAN_INPUT_REPRESENTATION_CHECKPOINT",
      "symbolic_hint": "latest_satisfying:vicreg_train_core_ready",
      "concrete_path": null,
      "status": "unresolved",
      "resolution_owner": "lattice"
    }
  ],
  "runtime_wave_match": {
    "shape_match": true,
    "checkpoint_inputs_match": "pending",
    "policy_match": true,
    "differing_fields": []
  },
  "next_action": "resolve_model_state",
  "audit": {
    "full_payload_available": true,
    "replay_tool": "hero.marshal.replay_receipt"
  }
}
```

Keep the full audit surface available under:

```text
machine_payload:
  advice
  request
  validation
  runtime_policy_snapshot
  runtime_wave_snapshot
  resolver_receipts
  decision_digests
```

Normal operator output does not lead with digest material.

## Plan-Input Resolution

`latest_satisfying:<target_id>` is a Lattice concept. Marshal may orchestrate a
read-only call, but Lattice must own the semantics.

Implemented resolver:

```text
hero.lattice.resolve_latest_satisfying
```

The resolver returns enough proof-side material for Marshal to validate and
audit the result:

```json
{
  "ok": true,
  "symbolic_hint": "latest_satisfying:vicreg_train_core_ready",
  "source_target_id": "vicreg_train_core_ready",
  "target_status": "satisfied",
  "concrete_path": "/absolute/checkpoint/path",
  "checkpoint_closure_complete": true,
  "selection_basis": "latest_satisfying",
  "resolver_receipt_digest": "..."
}
```

Marshal then validates only dispatch-admission properties:

```text
- concrete path is present
- path is absolute
- path is within allowed_model_state_roots when configured
- resolver receipt is fresh/trusted when required
- Runtime active wave uses the same concrete path before handoff
```

Marshal must not:

```text
- choose newest checkpoint by filesystem time
- rank checkpoints by metric
- choose a fallback if latest_satisfying cannot resolve
- accept operator checkpoint overrides for symbolic inputs without Lattice
  verification
```

## Dry-Run Policy

`prepare_target_dispatch` defaults to no Runtime dry-run:

```json
{
  "include_runtime_dry_run": false
}
```

When false, it returns `ready_for_dry_run` plus the exact next command. When
true, it may call the existing dry-run path only after:

```text
- Lattice plan exists
- all required model-state inputs are concrete
- concrete paths are absolute and allowed
- advice/request validation passes
- Runtime policy allows dry-run handoff
- Runtime active wave matches target, mode, range, and model-state inputs
- advice and resolver receipts are fresh enough
```

If dry-run succeeds, the packet may return:

```text
dispatch_state: ready_for_execution_gate
next_action: execution_gate
```

Execution remains separate and must still pass the existing dry-run evidence and
confirmation-token gate.

## Wave Alignment

When Runtime active wave does not match the advised wave, Marshal returns a
desired-wave diff, not a patch.

Good:

```json
{
  "runtime_wave_match": {
    "shape_match": false,
    "differing_fields": [
      {
        "field": "anchor_index_begin",
        "active": 1600,
        "advised": 1800
      }
    ]
  },
  "desired_wave": {
    "target": "wikimyei.inference.expected_value.mdn",
    "mode": "run|debug",
    "source_range": "anchor_index",
    "anchor_index_begin": 1800,
    "anchor_index_end": 2050
  },
  "next_action": "align_runtime_wave"
}
```

Risky:

```text
Marshal emits a literal kikijyeba.settings.wave.dsl patch.
```

Forbidden:

```text
Marshal edits kikijyeba.settings.wave.dsl.
```

Config Hero or Runtime Hero can later own patch generation/application if that
becomes useful.

## Guardrails

Simplification must not move ownership boundaries.

Scheduler risk:

```text
- accepts multiple targets and chooses order
- traverses dependencies
- loops over max attempts
- retries waves automatically
- chooses the next target after one completes
```

Model selector risk:

```text
- chooses newest checkpoint by filesystem time
- ranks checkpoints by metric
- chooses a fallback if latest_satisfying cannot resolve
- accepts arbitrary operator checkpoint overrides without Lattice verification
```

Proof authority risk:

```text
- marks target satisfied after dry-run
- marks target satisfied after Runtime execution
- treats receipt success as target proof
- suppresses Lattice proof-certificate failure
```

Runtime policy bypass risk:

```text
- executes directly from prepare_target_dispatch
- skips Runtime active-wave re-decode
- skips dry-run receipt matching
- skips execution confirmation token
- sends Runtime a wave not identical to the accepted Marshal request
```

Config-owner risk:

```text
- writes or applies kikijyeba.settings.wave.dsl
- treats desired_wave as an applied runtime state
```

## Discussion Notes

The simplest mental model remains:

```text
target -> plan -> concrete inputs -> matching active wave -> dry-run -> execute
-> lattice proof
```

The implementation enforces that model. The lower-level tool surface still
exposes advice, request, policy snapshot, wave snapshot, prior dry-run evidence,
receipts, and digests for audit/debugging, but operators usually see the smaller
target-first packet:

```text
Can I dispatch this target now?
If not, what exact thing is missing?
If yes, what exact wave will Runtime Hero dry-run or execute?
After it runs, what Lattice target should I re-check?
```

That is the simplification rule: keep the strict internal machinery, but wrap it
in a target-first operator packet.

Implementation invariant:

```text
Marshal simplifies the operation, not the proof.
```
