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
src/include/kikijyeba/marshal/dispatch_advice.h
src/include/kikijyeba/marshal/dispatch_adapter.h
src/include/kikijyeba/marshal/runtime_hero_handoff.h
src/include/kikijyeba/marshal/dispatch_operation.h
src/include/kikijyeba/marshal/execution_gate.h
src/include/kikijyeba/marshal/dispatch_receipt.h
src/include/kikijyeba/marshal/status.h
src/include/kikijyeba/marshal/batch_preview.h
src/include/kikijyeba/marshal/codex_assist.h
src/include/kikijyeba/marshal/tool_schema.h
src/include/kikijyeba/marshal/tool_handler.h
src/include/kikijyeba/marshal/performance_budget.h
src/main/hero/hero_marshal_mcp.cpp
```

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

`hero.marshal.lookup_target_advice` is the target-id convenience wrapper. It
asks Lattice Hero `hero.lattice.plan_target` for a specific `target_id`, maps
the returned plan into the same explicit `marshal_dispatch_advice_t` and
`marshal_dispatch_request_t` objects used by M1/M2, and stops before any Runtime
dispatch. It rejects free-form target text through the tool schema; dispatch
still requires the explicit advice path, concrete `PLAN_INPUT_*` model-state
paths, and the normal M1/M2 validation.

`hero.marshal.prepare_target_dispatch` is the operator-facing target-first
wrapper for “pursue lattice target X.” It calls Lattice `plan_target`, asks
Lattice read-only to resolve symbolic `latest_satisfying:<target_id>` model
state hints, validates the materialized advice, inspects Runtime policy and
active wave shape, and returns one compact operator packet. By default it does
not execute and does not even dry-run; `include_runtime_dry_run=true` is an
explicit opt-in and still stops before the execution gate. Marshal reports
desired wave differences but does not edit wave config, select checkpoints, or
claim target satisfaction.

Runtime Hero owns the active wave. Marshal compares advice to Runtime's decoded
wave using the canonical fields `target_component_family_id`, `mode`,
`source_range`, range bounds, `job_kind`, `train_target`, and
`model_state_inputs`. `wave_target` and `wave_mode` remain accepted only as
handoff compatibility aliases.

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
execute schema also accepts a `marshal_expected_wave` object with the canonical
Runtime active-wave field names and rejects mismatched expected wave fields,
including checkpoint/model-state input differences.

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

Batch preview is a dry-run-only wrapper over independent M1/M2 decisions. It
preserves per-item ids plus advice/request, validation-context, Runtime-policy,
and Runtime-wave digests for traceability. It does not infer dependency order,
retries, execution order, or target selection.

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
