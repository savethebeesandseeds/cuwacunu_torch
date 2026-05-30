# Kikijyeba Marshal

Marshal is the coordination layer between Lattice advice and Runtime execution.
It does not prove target satisfaction, and it does not relax Runtime Hero
execution policy.

```text
Lattice Hero:
  proves from runtime evidence and emits advice.

Marshal:
  validates advice, builds deterministic dispatch requests, and records audit
  receipts.

Runtime Hero:
  dry-runs or executes allowed waves and writes the evidence that Lattice later
  proves.
```

## M1 Dispatch Advice Schema

M1 defines the deterministic schema and validation rules for consuming lattice
plan advice. It intentionally does not execute or dry-run Runtime Hero.

Implemented surface:

```text
src/include/hero/marshal_hero/hero_marshal.def
src/include/hero/marshal_hero/hero_marshal.h
src/include/hero/marshal_hero/hero_marshal_tools.h
src/include/kikijyeba/marshal/dispatch_advice.h
src/include/kikijyeba/marshal/dispatch_adapter.h
src/include/kikijyeba/marshal/runtime_hero_handoff.h
src/include/kikijyeba/marshal/dispatch_operation.h
src/include/kikijyeba/marshal/execution_gate.h
src/include/kikijyeba/marshal/dispatch_receipt.h
src/include/kikijyeba/marshal/status.h
src/include/kikijyeba/marshal/operational_report.h
src/include/kikijyeba/marshal/batch_preview.h
src/include/kikijyeba/marshal/codex_assist.h
src/include/kikijyeba/marshal/tool_schema.h
src/include/kikijyeba/marshal/tool_handler.h
src/include/kikijyeba/marshal/performance_budget.h
src/main/hero/hero_marshal_mcp.cpp
```

`src/include/hero/marshal_hero` is the thin Hero symmetry layer: it owns the
public Marshal Hero tool registry and the MCP-facing facade. The implementation
stays in `src/include/kikijyeba/marshal`, where the deterministic dispatch,
evaluation, receipt, and target-driver methods live.

`src/config/hero.marshal.dsl` is intentionally minimal and exists for Hero
policy-path symmetry only. Runtime Hero policy still owns execution authority,
and Lattice Hero still owns proof authority.

The M1 API provides:

```text
marshal_dispatch_advice_t
marshal_dispatch_request_t
marshal_dispatch_validation_context_t
marshal_dispatch_validation_result_t

canonical_*_text(...)
*_digest(...)
validate_dispatch_advice(...)
```

## Dispatchability

M1 advice is dispatchable only when it is concrete, fresh, and allowed by the
requested-mode context.
The validator fails closed for:

```text
missing_plan_basis
missing_suggested_wave
target_already_satisfied
target_status_not_dispatchable
stale_active_identity
stale_target_spec
stale_split_policy
max_waves_exhausted
malformed_wave_range
missing_model_state_input
forbidden_dispatch_mode
unknown_required_field
unsupported_wave_target
unsupported_source_range
advice_digest_mismatch
advice_runtime_root_mismatch
target_id_mismatch
runtime_policy_refused
runtime_wave_mismatch
checkpoint_input_mismatch
runtime_checkpoint_input_missing
runtime_handoff_unavailable
unproven_lattice_advice
stale_lattice_advice
dry_run_receipt_missing
dry_run_receipt_mismatch
```

`PLAN_INPUT_*` values must be resolved runtime model-state paths by the time
Marshal validates advice. Symbolic hints such as
`latest_satisfying:<target_id>` remain Lattice plan annotations and are not
dispatchable model-state inputs. Required model-state inputs must be absolute
paths, must stay inside configured `allowed_model_state_roots` when that
allowlist is provided, and must also appear in the Runtime Hero active-wave
snapshot before a handoff is accepted.

Fresh advice means:

```text
source_lattice_tool is one of the allowed Lattice plan/evaluate tools
source_lattice_timestamp parses as RFC3339 UTC
the timestamp is not in the future beyond the configured clock skew
the timestamp is not older than max_advice_age when a freshness clock is set
active identity matches the operator context
```

When a trust boundary requires signed Lattice advice, the validation context can
set `require_signed_lattice_advice=true` and provide a trusted advice key id plus
verification key. Advice may then carry `advice_receipt_digest`,
`advice_signature_key_id`, and `advice_signature`; Marshal recomputes the
canonical advice payload and fails closed on missing, malformed, untrusted, or
mismatched signature material. The current verifier is deterministic SHA-256
keyed material for local receipt integrity; a future asymmetric signer can
replace the signing primitive without changing the advice gate.

Marshal computes 256-bit SHA-256 digests for advice, requests, confirmation
tokens, handoff arguments, and receipt ids. These digests are local integrity
and replay aids; they do not make Marshal a proof authority.

## Non-Authority Rule

Marshal advice, decisions, and receipts are audit metadata only. They must not
satisfy:

```text
train-core readiness
validation evaluation readiness
leakage cleanliness
checkpoint closure
performance gates
```

After Runtime Hero writes artifacts, Lattice Hero must re-read runtime evidence
and prove the target.

## Public Operator Surface

The current high-level Marshal surface is intentionally small:

```text
hero.marshal.status
hero.marshal.reach_lattice_target
hero.marshal.evaluate
```

`reach_lattice_target` is the deterministic Marshal for moving one lattice
target toward readiness. It has two explicit drive modes:

```text
drive_mode = one_step
drive_mode = budgeted
```

`one_step` computes one Lattice-backed move, optionally dry-runs it, emits a
target-driver ledger, and stops. `budgeted` performs a finite driver loop: it
asks Lattice for the current target deficit, materializes model-state inputs,
checks Runtime wave/policy alignment, dry-runs Runtime, and either stops before
execution or executes only when `requested_mode=execute`, `driver_policy`
allows execution, Runtime policy allows execution, and the existing execution
gate accepts the dry-run evidence. The target is reported reached only after
Lattice reports satisfaction.

Warning stops use typed Runtime/Lattice warning envelopes rather than loose JSON
sniffing. A warning used for driver policy must carry `warning_id`, `severity`,
`source`, `component`, `scope`, `blocking`, and `evidence_digest`. Unknown or
missing severity fails closed when the matching stop policy is enabled; a
blocking warning stops even below the configured severity threshold.

The operator packet presents a compact `operator_summary`, `stop_reason`,
`wave_panel`, `runtime_panel`, `lattice_panel`, and `audit_panel`, followed by
the compatibility fields for target, blocker, advised wave, concrete
model-state inputs, Runtime wave match, dry-run result, target-driver terminal
state, target-driver ledger, and next safe action.

`evaluate` is the high-level deterministic evaluator. It supports:

```text
subject = run
subject = target
subject = protocol
subject = spawn
subject = component
```

`subject=run` wraps the operational report and comparison paths through
`latest_chain`, `training_state`, `single_job`, and `compare` modes.
`subject=target` asks Lattice for the target deficit and explains the plan or
blocker. It labels target status as sourced from `hero.lattice.target_deficit`
and only suggests certificate inspection when Lattice returned certificate
material. `subject=protocol` checks observed Runtime identity fields against
the requested protocol/graph/source/assembly identity. Its default
`identity_mode=report` treats observed-only identity as an operator observation,
not as strict verification; `identity_mode=strict` requires explicit expected
identity fields and reports `identity_verified=true` only when they all match.
`subject=spawn` and `subject=component` group Runtime evidence for one
component spawn or component family with `evidence_scope=runtime_jobs_only`.
All subjects explain evidence without execution, proof claims, model selection,
or checkpoint selection.

The lower-level behavior is implementation detail, not a public tool surface:

```text
target advice materialization
dispatch validation
Runtime dry-run handoff
execution-gate validation
receipt replay
batch preview
```

These remain ordinary C++ methods where needed. They are not registered as
Marshal Hero tools and cannot be invoked through MCP/CLI tool names.

Standalone scheduler research notes were removed from this folder. Scheduler
ideas remain intentionally out of scope for Marshal's current public surface;
the active roadmap keeps them in the `Not Now` section instead of carrying a
separate research artifact.

## Marshal Operational Report v1

`hero.marshal.evaluate` with `subject=run` is the read-only operator report
surface. Its `latest_chain`, `training_state`, and `single_job` modes answer:

```text
Where are we, what just happened, and what is safe to do next?
```

It reads Runtime job directories, `job.state`, Runtime terminal facts, component
reports, checkpoint load/write fields, and Lattice target statuses. Runtime
terminal facts are preferred when present; component reports remain the
fallback for older jobs. The current packet includes:

```text
evidence_scope
operator_summary
stop_reason
runtime_panel
lattice_panel
audit_panel
current_state
job_chain
chain_summary
target_blockers
warnings
suspicious_items
next_safe_actions
```

`chain_summary` groups each Runtime job by component family, component spawn
identity, wave action/mode, source range, checkpoint I/O, handoff identity, and
terminal-fact availability.

When `include_machine_payload=true`, the report also includes full job rows,
checkpoint rows, the target-status map, and detailed metrics under
`machine_payload`.

The report does not execute, dry-run, edit config, select checkpoints, or claim
target satisfaction. Lattice remains the only proof authority; Marshal only
quotes Lattice statuses, proof-check issues, deficits, and warnings, then turns
Runtime artifacts into an operator-readable summary.

## M2 Preview And Handoff Core

The M2 adapter core can turn validated M1 advice plus Runtime Hero policy/wave
snapshots into a deterministic dry-run request preview.

It checks:

```text
Runtime Hero availability
runtime executable availability
runtime_root agreement
active wave target/mode/range agreement
checkpoint/model-state input agreement
dry_run=true and confirm_execute=false
field derivations for each generated Runtime request field
```

The handoff helper calls Runtime Hero through `hero.runtime.execute` with
`dry_run=true` and `confirm_execute=false`, returning the Runtime Hero tool
result JSON plus the Marshal non-authority statement. Runtime handoff parsing
reads the MCP result's top-level `isError` and `structuredContent.ok` fields so
nested text or nested objects cannot accidentally masquerade as Runtime
acceptance.

The public M2 operation accepts explicit fresh Lattice Hero advice, refuses
unproven advice, calls the handoff helper, and includes the live Runtime Hero
dry-run result in the Marshal response.

`hero.marshal.reach_lattice_target` is the operator-facing wrapper for
“pursue lattice target X.” The target wrapper calls the current Lattice target
advice query, asks Marshal's read-only Lattice callback to materialize symbolic
`latest_satisfying:<target_id>` model-state hints, validates the materialized
advice, inspects Runtime policy and active wave shape, and returns one compact
operator packet.

In `one_step` mode, `include_runtime_dry_run=true` is still an explicit opt-in.
In `budgeted` mode, Runtime dry-run is part of the driver loop and is bounded by
`driver_policy.max_waves`. `budgeted` execution also requires
`driver_policy.max_wall_clock_seconds`, `driver_policy.allow_execute=true`, and
Runtime `allow_execute=true`. Train waves additionally require both
`driver_policy.allow_train_execute=true` and Runtime `allow_train_execute=true`.
Marshal reports desired wave differences but does not edit wave config, select
checkpoints, or claim target satisfaction.

Every target-driver run emits a ledger with the driver policy digest,
per-iteration Lattice target-deficit digest, suggested-wave digest, Runtime
handoff/request digests, terminal Runtime evidence digests, terminal state, and
non-authority statement. When `require_runtime_job_completion=true`, Marshal
requires durable `runtime.result.fact` plus `job.manifest`; a completed
`job.state` alone is not enough. If Runtime reports checkpoint writes or loaded
model-state checkpoints, `runtime.checkpoint_io.fact` must also be present and
is bound into the ledger. A compact `resume_ledger` may be supplied to continue
a bounded run without forgetting spent handoff/execution budget; stale target,
mode, policy identity, missing terminal evidence identity, missing manifest
identity, or missing handoff identity blocks resume.

Target-driver replay audit is also available internally for retention drills.
Full ledger replay recomputes iteration and ledger digests. Compact ledger
replay accepts removed iteration bodies only when `compacted_fields` says so and
the durable Runtime evidence identities remain. Replay rejects stale target,
protocol, graph order, target spec, split policy, driver policy, and Runtime
policy identities. It remains audit-only and never proves target satisfaction.

Runtime Hero owns the active wave. Marshal compares advice to Runtime's decoded
wave using the canonical fields `target_component_family_id`, `mode`,
`source_range`, range bounds, `job_kind`, `train_target`, and
`model_state_inputs`.

## M3 Execution Gate

M3 allows an execution handoff only after the dry-run path has produced
structured accepted dry-run evidence for the same dispatch. A non-empty string
or placeholder digest is not enough.

Prior dry-run evidence binds:

```text
accepted dry-run status
advice_digest
dispatch_request_identity_digest
runtime_preview_request_digest
suggested_wave_digest
plan_input_digest
Runtime Hero policy snapshot digest
Runtime Hero active-wave snapshot digest
Runtime response digest
dry-run response digest
canonical prior evidence digest
```

The confirmation token binds:

```text
target_id
runtime_root
protocol_contract_fingerprint
target_spec_fingerprint
split_policy_fingerprint
suggested_wave_digest
plan_input_digest
requested_mode
```

The execution gate refuses missing confirmation, confirmation mismatch, missing
or mismatched prior dry-run evidence, unproven/stale Lattice advice, Runtime
Hero `allow_execute=false`, and train execution when
`allow_train_execute=false`.

The public execution handoff takes an accepted `marshal_execution_gate_result_t`.
It cannot be called from a plain dry-run decision. Immediately before calling
`hero.runtime.execute`, Marshal asks Runtime Hero to decode the active wave and
compares target, mode, source range, and anchor bounds against the derived
request, including concrete `PLAN_INPUT_*` model-state inputs. The Runtime Hero
execute schema accepts a canonical `runtime_handoff` object. The object carries
handoff schema/id/digest, creator/timestamp, target id, base config path/hash,
concrete wave fields, concrete checkpoint inputs, Runtime policy path/hash,
dry-run or execute intent, and an `unresolved_symbols` list. Runtime rejects
non-empty `unresolved_symbols`, symbolic `latest_satisfying:*` inputs,
mismatched intent, and mismatched active-wave fields before launching
`cuwacunu_exec`. Accepted handoffs are echoed back through the Runtime job
manifest, terminal result fact, and derived lattice exposure fact using
`runtime_handoff_id` and `runtime_handoff_digest`.

`marshal_expected_wave` remains as compatibility scaffolding with the same
canonical Runtime active-wave field names. Runtime also rejects symbolic
model-state inputs on that legacy path.

## M4 Dispatch Receipts

M4 receipts explain what Marshal requested and what Runtime Hero returned. They
record:

```text
target_id
lattice_proof_context
plan_basis_digest
suggested_wave_digest
Runtime request digest
Runtime preview request digest
Runtime execution request digest
Runtime handoff argument digest
Runtime response digest
policy decision
confirmation status
refusal reasons
post-run verification instruction
non-authority statement
```

`lattice_proof_context` is the active proof context copied from Lattice advice;
it is not a component spawn id, component spawn fingerprint, or target
satisfaction proof. Replay audit checks receipt schema, active identity, receipt
id/content consistency, required audit digests, retention class, and the
non-authority statement. A fresh receipt can explain a handoff; it still cannot
satisfy a lattice target.

Receipts may be compacted. Compact receipts remove bulky Runtime Hero output but
retain request/advice/decision/runtime-response digests, refusal reasons,
identity, compacted-field notes, and the non-authority statement. Compact and
tombstone receipts remain audit metadata only.

## M6 Status And M7 Batch Preview

Marshal status is a compact read-only summary over recent receipts and Runtime
policy. It reports last accepted dry-run, last execution handoff, latest refusal
reason, and Lattice advice surface availability. It does not evaluate lattice
targets.

## Marshal Run Comparison

`hero.marshal.evaluate` with `subject=run` and `mode=compare` compares two
Runtime jobs descriptively. It reads the same Runtime job state, terminal facts,
checkpoint I/O facts, health facts, and raw report fallback fields used by the
operational report, then returns:

```text
operator_summary
stop_reason
runtime_panel
lattice_panel
audit_panel
run_pair
comparability
config_identity_deltas
wave_range_deltas
metric_deltas
checkpoint_deltas
warning_deltas
proof_context
```

`config_identity_deltas` and `wave_range_deltas` make config identity, protocol
identity, graph/source identity, wave action/mode, source-range policy, anchor
bounds, and accepted anchor count visible before metric differences. When
`include_machine_payload=true`, comparison also returns full baseline and
candidate job rows, full performance visibility panels, checkpoint lineage
details, and per-run metric maps.

The comparison surface is read-only. It does not execute, edit config, select a
checkpoint, choose a winner, or claim target satisfaction. Lattice cleanliness
is reported as a boundary: comparison can say that Lattice replay is required,
but it cannot prove cleanliness by itself.

Invariant:

```text
Compare does not choose. Compare explains.
```

Use `mode=compare` for run-to-run comparison, `mode=latest_chain` for the
latest training/eval chain, and `mode=single_job` for a bounded inspection of
one job.

## M5, M9, And M10

Codex-assisted Marshal wrappers call deterministic primitives and keep free-form
text as operator explanation or prompt text. They cannot bypass validation,
policy, confirmation, range, identity, or checkpoint checks.

Marshal tool schemas are cataloged for MCP/CLI compatibility, and
`hero_marshal.mcp` exposes the deterministic primitives through direct CLI and
JSON-RPC stdio modes. The handler layer rejects missing required fields and
unknown fields before materializing Marshal structs. Schema compatibility checks
remain harness safety only; they do not change proof semantics.

The performance budget separates library validation, preview derivation,
Runtime Hero policy read, long-lived MCP schema surface, direct CLI schema
surface, Runtime Hero handoff, and receipt IO. Every stage uses repeated samples
and reports sample count plus min/max/p50/p95 timing. Performance reports
explicitly state that speed is not target satisfaction evidence.
