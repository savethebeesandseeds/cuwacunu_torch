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
src/include/kikijyeba/marshal/evaluation_marshal.h
src/include/kikijyeba/marshal/rollout_marshal.h
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

## M2 Evaluation Marshal Contract

M2 defines the first reusable environment-evaluation marshal contract. It does
not execute replay, choose a policy, or prove performance. It prepares a bounded
Runtime replay evaluation request and records a non-authoritative receipt after
Runtime has produced replay reports.

Implemented surface:

```text
marshal_evaluation_anchor_range_t
marshal_evaluation_request_t
marshal_evaluation_plan_t
marshal_evaluation_runtime_handoff_t
marshal_evaluation_policy_summary_t
marshal_evaluation_receipt_t

canonical_evaluation_request_text(...)
evaluation_request_digest(...)
validate_evaluation_request(...)
prepare_evaluation_plan(...)
canonical_evaluation_plan_text(...)
evaluation_plan_digest(...)
prepare_evaluation_runtime_handoff(...)
canonical_evaluation_runtime_handoff_text(...)
evaluation_runtime_handoff_digest(...)
canonical_evaluation_receipt_text(...)
evaluation_receipt_digest(...)
finalize_evaluation_receipt(...)
```

The receipt preserves policy-neutral replay health counters, metric-scope labels,
policy-wise summaries, projection-validation diagnostics, and Cajtucu paper
execution summaries. Projection diagnostics include interval width,
zero-return baseline error, and model skill versus that baseline. Cajtucu
summaries include backend identity, execution-step counts, rejected/partial fill
counts, order/fill counts, and paper fee/spread/slippage/transaction-cost
totals. It is evidence for later review; it does not claim target satisfaction.

The request is accepted only when it is finite and explicitly out-of-sample:

```text
trained_range and validation_range are valid half-open anchor-index ranges
trained_range and validation_range do not overlap
trained_range_evidence_digest is present and strong
validation_split_policy_digest is present and strong
Runtime executable, evaluation config, MDN checkpoint, and representation
  checkpoint paths exist when path checks are enabled
base_reserve_node_id is present
risky_node_ids is non-empty
base_reserve_node_id is not duplicated inside risky_node_ids
risky_node_ids contains no duplicates
```

The plan emits command templates for:

```text
1. Runtime MDN run on the validation anchor range.
2. Runtime replay-from-job evaluation using the completed job directory.
```

The evaluation runtime handoff prepares a bounded dry-run argument package for:

```text
hero.runtime.execute
```

It binds the validation anchor range, expected Runtime wave identity, replay
command template, request digest, and plan digest. It is dry-run only:

```text
dry_run=true
confirm_execute=false
target_satisfaction_claimed=false
runtime_executor=false
lattice_proof_authority=false
```

Marshal does not call Runtime from this contract; the handoff is a deterministic
package that can be inspected, digested, and later handed to the existing Runtime
Hero execution surface by an authorized operator/tooling path.

The receipt records replay metrics such as bundle count, attempted/completed
count, projection MAE/RMSE/correlation/directional accuracy, interval coverage,
time-law counters, transaction-cost summary, Cajtucu paper execution summary,
and final equity summary. It also labels the top-level aggregate scope so
experiment means are not confused with one policy's score:

```text
top_level_metric_scope=mean_over_completed_episode_reports_across_policies
policy_metric_scope=mean_over_completed_episode_reports_for_policy
```

Policy-wise summaries may be embedded in the receipt for compact review:

```text
policy_id
policy_kind
attempted/completed/failed counts
final equity
max drawdown
turnover
transaction cost
Cajtucu backend / fill / cost counters
projection metrics
```

The receipt always carries:

```text
target_satisfaction_claimed=false
runtime_executor=false
lattice_proof_authority=false
```

Lattice must still re-read Runtime evidence to prove any target. Marshal
evaluation receipts are coordination and audit metadata only.

## M3 Rollout Marshal Contract

M3 defines the environment rollout preparation contract. A rollout is a bounded
historical environment run:

```text
completed Runtime job artifacts
  + policy set
  + Kikijyeba replay environment
  + Cajtucu paper execution profile
    -> trajectory evidence after Runtime executes the command
```

Implemented surface:

```text
marshal_rollout_execution_profile_t
marshal_rollout_request_t
marshal_rollout_plan_t

canonical_rollout_request_text(...)
rollout_request_digest(...)
validate_rollout_request(...)
prepare_rollout_plan(...)
canonical_rollout_plan_text(...)
rollout_plan_digest(...)
rollout_plan_json(...)
```

`hero.marshal.prepare intent=rollout` returns a plan only. It does not execute,
train, inspect reports, choose a policy, produce a receipt, or claim Lattice
target satisfaction.

The accepted V1 rollout mode is:

```text
environment_mode=historical_replay
environment_assembly_id=kikijyeba.environment.replay.v1
execution_backend_id=cajtucu.execution.paper.v1
```

The request must bind:

```text
rollout_id
completed Runtime job directory with replay_artifacts_written=true
replay_batch_index_path
base_reserve_node_id
risky_node_ids
finite max_steps / max_parallel_jobs
policy_set
Cajtucu paper execution profile
```

Synthetic direct execution edges are rejected by default. Research replay may
opt in only by setting `allow_synthetic_direct_edges=true` and providing
`synthetic_edge_research_reason`.

`prepare` emits a replay command template for:

```text
cuwacunu_exec --replay-from-job-dir ...
```

The receipt lifecycle is intentionally separate:

```text
prepare rollout request -> marshal_rollout_plan_t
Runtime executes replay command -> replay report / trajectory evidence
inspect reads report later -> receipt/summary, future work
```

The rollout plan always carries:

```text
target_satisfaction_claimed=false
runtime_executor=false
lattice_proof_authority=false
policy_training_authority=false
live_execution_authority=false
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
hero.marshal.prepare
hero.marshal.inspect
```

`prepare` is the deterministic Marshal for moving one lattice
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

Artifact-readiness targets are evidence proofs, not Runtime wave requests.
When `hero.lattice.target_deficit` reports `target_class=artifact_readiness`,
`prepare` returns a non-dispatchable packet with
`next_action=inspect`; it does not resolve checkpoints, inspect
Runtime wave shape, or dry-run Runtime. Use `hero.marshal.inspect`
to view the target proof and any related fact-family summary.

`inspect` is read-only. It calls Lattice
`evaluate_target` for a target proof, `fact_summary` or `scan_facts` for
fact-family evidence, `fact_lineage` for audit-only relation/key/digest lineage
rows, and `fact_preview` when the caller explicitly asks for a concrete fact
row by digest, digest prefix, or fact index. Marshal does not become proof
authority, does not claim target satisfaction, and does not select checkpoints
from this panel.
The public inspect, status, and operational-report JSON surfaces
declare this boundary explicitly with fields such as `target_proof=false`,
`dispatchable=false`, `runtime_executor=false`,
`fact_families_are_not_target_kinds=true`, `allocation_decision=false`,
`market_readiness_decision=false`, and `deployment_decision=false`.
For failed artifact proofs, the panel preserves the Lattice summary:
failed-proof count, proof-template bound/unbound count, proof kind,
proof-template claim, identity-mismatch count, lineage-unbound count,
authority-drift count, integrity flags, authority flags, explicit boundary
denial flags for target dependencies, Runtime waves, Marshal reachability,
checkpoint sources, and plan checkpoint inputs, issue codes, artifact deficit
keys, related fact-integrity issue codes, and primary deficit key.
Fact panels also relay Lattice's native `fact_integrity_summary`, which reports
declared relation digests, resolved relation digests, unresolved relations,
identity-mismatch counts, digest-mismatch counts, warning counts, affected
families, and issue codes. This keeps unresolved transform, baseline,
selection-signal, observer, and forecast artifact lineage visible before an
artifact proof failure is interpreted as a generic target problem.
Fact panels include a compact `lineage_panel` by default. It relays
`hero.lattice.fact_lineage` row counts, selected relations, lineage rows, and
explicit non-authority flags such as
`cache_rows_used_for_target_satisfaction=false`; operators may pass
`include_lineage=false` only to suppress that audit view.
Fact panels include `preview_panel` only when requested with `include_preview`,
`digest`, `fact_digest`, `digest_prefix`, `fact_index`, or `index`. The preview
relays `hero.lattice.fact_preview` rows and keeps
`preview_rows_are_audit_only=true`,
`facts_used_for_target_satisfaction=false`, `checkpoint_selected=false`, and
`model_selector=false`. The relayed rows include Lattice's normalized
`identity_envelope`, so Marshal clients can inspect a fact's family, digest,
active identity, parent digests, support counters, and audit-only authority
flags without parsing each family payload.
Target panels also summarize Lattice artifact `fact_preview_hint` entries as
`artifact_fact_preview_*` fields. Marshal relays the family, digest, Lattice
tool, and Marshal tool as navigation metadata only; it still does not prove,
select, or dispatch from those hints.
They also relay the fact catalog's artifact-readiness boundary metadata:
proofable family count, proof kind, proof-template claim, promotion-blocked
family count, promotion-blocked reason, and warning-summary-only families.
Marshal displays these as read-only catalog facts; it does not promote a fact
family into a target or override Lattice's proof-template registry.
When Lattice reports disabled policy-gate reservations, Marshal relays the
reservation summary and matching reservations in target and prepare panels, but
keeps policy-gate dispatch, target-status, and proof authority false. Marshal
also preserves Lattice's policy-fingerprint verification counters and
policy-input contract counters as read-only audit context, including mismatch
counts, missing-input counts, and the all-inputs-complete bit; it does not
recompute or promote them into decision-policy authority.
Fact-family-only evidence panels, including `replay_environment`, do not call
target proof evaluation and cannot be routed through `prepare`.

Warning stops use typed Runtime/Lattice warning envelopes rather than loose JSON
sniffing. A warning used for driver policy must carry `warning_id`, `severity`,
`source`, `component`, `scope`, `blocking`, and `evidence_digest`. Unknown or
missing severity fails closed when the matching stop policy is enabled; a
blocking warning stops even below the configured severity threshold. Lattice
target evaluations project their `warning_results` into this typed `warnings`
surface with `blocking=false`; human-readable warning text stays in
`warning_messages`.

The operator packet presents a compact `operator_summary`, `stop_reason`,
`wave_panel`, `runtime_panel`, `lattice_panel`, and `audit_panel`, followed by
the compatibility fields for target, blocker, advised wave, concrete
model-state inputs, Runtime wave match, dry-run result, target-driver terminal
state, target-driver ledger, and next safe action.

`inspect` is also the high-level deterministic read-only evaluator. It
supports:

```text
subject = run
subject = target
subject = protocol
subject = spawn
subject = component
```

`subject=run` wraps the operational report and comparison paths through
`latest_chain`, `training_state`, `single_job`, and `compare` modes.
When `subject=run` asks Lattice for bulk target status, artifact-readiness
blockers preserve the failed artifact proof summary, including explicit
boundary denials for non-dispatchable/non-checkpoint authority fields, and route
to `inspect` rather than presenting the failure as a dispatchable
training deficit. Disabled policy-gate reservations remain attached as
read-only blocker context and do not change next-safe-action selection.
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

`hero.marshal.inspect` with `subject=run` is the read-only operator report
surface. Its `latest_chain`, `training_state`, and `single_job` modes answer:

```text
Where are we, what just happened, and what is safe to do next?
```

It reads Runtime job directories, `job.state`, Runtime terminal facts, component
reports, checkpoint load/write fields, Kikijyeba replay evidence indexes, and
Lattice target statuses. Runtime terminal facts are preferred when present;
component reports remain the fallback for older jobs. The current packet
includes:

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
identity, wave action/mode, source range, checkpoint I/O, handoff identity,
terminal-fact availability, and read-only replay evidence. Replay evidence is
reported from `artifacts/kikijyeba.environment.replay.v1/` when present:
`runtime_replay_batches.index`, `runtime_replay_experiments.index`, and the
latest replay experiment report. Marshal only summarizes these files; Runtime
Hero remains the replay executor/reader.

When `include_machine_payload=true`, the report also includes full job rows,
checkpoint rows, the target-status map, and detailed metrics under
`machine_payload`.

The report does not execute, dry-run, edit config, select checkpoints, or claim
target satisfaction. Lattice remains the only proof authority; Marshal only
quotes Lattice statuses, proof-check issues, deficits, and warnings, then turns
Runtime artifacts into an operator-readable summary.
For artifact-readiness targets, target blockers also quote related
fact-integrity issue codes from Lattice proof deficits so the compact run view
can connect an artifact issue to the exact catalog issue row without
re-evaluating proof.

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

`hero.marshal.prepare` is the operator-facing wrapper for
“pursue lattice target X.” The target wrapper calls the current Lattice target
advice query, asks Marshal's read-only Lattice callback to materialize symbolic
`latest_satisfying:<target_id>` model-state hints, validates the materialized
advice, inspects Runtime policy and active wave shape, and returns one compact
operator packet.

If that query identifies an artifact-readiness target, Marshal stops before the
handoff path and routes the operator to `hero.marshal.inspect`.
This keeps non-trainable Lattice evidence surfaces out of Runtime dispatch
semantics.

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

Target-driver identity is split deliberately:

```text
target_driver_run_key
  Deterministic grouping key for the target, known active identity, drive mode,
  requested mode, and driver policy digest. It groups comparable driver ledgers
  but is not a terminal evidence identity.

target_driver_run_id
  Per-invocation ledger identity. It is derived from the run key plus
  ledger_created_at_utc and ledger_nonce, exists before the first Runtime
  handoff, is echoed into Runtime handoff/job evidence, and must remain stable
  across resume.

runtime_handoff_id / runtime_handoff_digest
  Per-handoff execution identity. Runtime terminal evidence must match these
  exactly; they are the hard reconciliation identity.
```

`ledger_created_at_utc` and `ledger_nonce` may be supplied by tests; otherwise
Marshal generates them for a fresh prepare call. Runtime handoff id/digest remain
the strict terminal-evidence identity even when several ledgers share the same
target-driver run key.

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

Current `budgeted` execution uses policy-confirmed automation. When
`driver_policy.allow_execute=true` and Runtime policy also allows execution,
Marshal creates the execution confirmation token internally from the accepted
dry-run evidence and exact request identity before it calls the execution gate.
This is not a second human confirmation step. A future interactive operator
surface may split this into a dry-run challenge followed by a caller-supplied
confirmation token.

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
targets, prove targets, promote fact families, or make allocation, market, or
deployment decisions.

## Marshal Run Comparison

`hero.marshal.inspect` with `subject=run` and `mode=compare` compares two
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

The comparison and run-report surfaces are read-only. They do not execute, edit
config, select a checkpoint, choose a winner, claim target satisfaction, promote
fact families into target kinds, or make allocation, market, or deployment
decisions. Lattice cleanliness is reported as a boundary: comparison can say
that Lattice replay is required, but it cannot prove cleanliness by itself.

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
