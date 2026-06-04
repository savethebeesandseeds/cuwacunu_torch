# Marshal Testing Roadmap

Marshal is now intentionally small:

```text
hero.marshal.status
hero.marshal.prepare
hero.marshal.rollout
hero.marshal.inspect
```

This roadmap is rebased around testing the joint system rather than carrying
the full implementation history. Future work should be opened as finite goals.
Avoid broad "finish Marshal" goals.

Current testing work should be recorded here only when it is still active.
Completed test receipts belong in runtime/Lattice evidence, not in a separate
Marshal status file.

## Marshal V2 Contract Refactor Roadmap

Current state:

```text
V1 public tools are stable and tested:
  hero.marshal.status
  hero.marshal.prepare
  hero.marshal.rollout
  hero.marshal.inspect

Recent cleanup clarified the operative model:
  Lattice proves.
  Runtime executes.
  Marshal coordinates, validates, delegates, records, and explains.
  Cajtucu paper-executes replay mechanics under Runtime replay.
```

Known remaining V1 debt:

```text
schemas are still permissive where prose is strict
some public fields are test hooks or compatibility aliases
prepare still exposes reserved intent values
rollout execution profile is still permissive
rollout idempotency is bound into the request but does not yet have a durable
  duplicate-handoff ledger
inspect still accepts alias fields such as family/fact_family and digest/fact_digest
authority fields are mostly flat instead of namespaced
responses do not yet share one operator envelope
```

The V2 refactor should happen as finite milestones, stopping after the first
failed gate. Do not attempt another broad "all Marshal cleanup" pass.

### V2.1a Static MCP Surface And Schema Lock

Goal: make the public Marshal MCP catalog and schemas match the current V1
operator contract without changing Runtime behavior, Lattice behavior,
target-driver behavior, rollout execution behavior, receipt semantics, or the
response-envelope shape.

Changes:

```text
preserve exactly four public tools: status, prepare, rollout, inspect
add property-level enums for current V1 mode fields:
  prepare.requested_mode = dry_run|execute
  prepare.drive_mode = one_step|budgeted
  rollout.requested_mode = dry_run|execute before V2.2a
  inspect.subject = run|target|protocol|spawn|component|facts
  inspect.identity_mode = report|strict
add required fields only where the current handler has a single canonical
  required field and the constraint is expressible without top-level schema
  unions
leave compatibility alias pairs such as target_id|lattice_target to
  handler-level validation
keep MCP-compatible root schemas; no top-level oneOf/anyOf/allOf
make tool descriptions match the operator model
add tests proving old hidden tool names are still unknown
add tests proving unknown fields and unsupported inspect subjects fail closed
```

Out of scope:

```text
removing lattice_target
removing or narrowing prepare.intent
changing prepare requested_mode semantics
introducing requested_mode=plan
changing rollout requested_mode=dry_run to plan
removing prepare_only
adding rollout_attempt_id or idempotency_key behavior
adding replay_batch_index_path requirements
typing rollout execution_profile
inspect alias removal
nested request payloads
response envelope rewrite
next_action removal
policy-training support
PPO or environment scheduling
Runtime/Lattice behavior changes
```

Acceptance:

```text
build-marshal-hero passes
run-test_hero_mcp_schema_compat passes from src/tests/bench/kikijyeba/runtime
run-test_kikijyeba_marshal_dispatch passes
hero_marshal.mcp --list-tools-json exposes only the four public tools
```

### V2.1b Help And Documentation Synchronization

Goal: after V2.1a is locked, keep `--help`, README, ROADMAP, and
`hero_marshal.def` synchronized without changing behavior.

Changes:

```text
refresh help examples when public schema text changes
keep the operator model consistent across README and main Hero docs
keep the accepted command list aligned with actual Makefile locations
```

### V2.2a Rollout Public Vocabulary Cleanup

Goal: make rollout use the RL/environment public vocabulary before deeper
identity and execution-profile validation.

Changes:

```text
requested_mode becomes plan|execute
prepare_only is removed from the public schema and handler
add rollout_attempt_id and idempotency_key
add replay_batch_index_path as an explicit public field
request/plan digests bind rollout_attempt_id, idempotency_key, and
  replay_batch_index_path
```

Acceptance:

```text
rollout plan accepts valid completed Runtime job evidence with explicit
  replay_batch_index_path
rollout execute delegates to hero.runtime.run operation=replay
requested_mode=dry_run is rejected
prepare_only is rejected as an unknown field
missing rollout_attempt_id, idempotency_key, or replay_batch_index_path fails
  closed
run-test_kikijyeba_marshal_rollout passes
```

### V2.2b Rollout Identity And Execution Profile Validation

Goal: make rollout identity and Cajtucu profile validation strict enough for
larger environment dispatch.

Status: implemented for the current static/handler contract.

Changes:

```text
add graph_order_fingerprint and asset_universe_digest
type the Cajtucu execution profile enough to expose backend id, synthetic-edge
  policy, partial-fill policy, and transaction-cost model identity
```

Rules:

```text
rollout_id is the stable logical rollout identity
rollout_attempt_id is per invocation
idempotency_key is bound into rollout identity digests for audit and future
  duplicate-handoff prevention
durable duplicate-handoff prevention requires a future ledger and is not
  claimed by this checkpoint
Runtime remains the replay executor
Marshal produces no metrics and no Lattice proof
```

Acceptance:

```text
rollout plan binds graph and asset-universe identity
typed execution profile rejects unsupported Cajtucu paper profile fields
synthetic execution edges require an explicit research reason
run-test_kikijyeba_marshal_rollout passes
```

Implemented notes:

```text
graph_order_fingerprint is a required rollout request field and is checked
  against the completed Runtime job manifest
asset_universe_digest is required and must match the ordered
  base_reserve_node_id + risky_node_ids action universe
policy_set is required and must be non-empty
max_steps is required and must be positive
execution_profile is typed in the public Marshal schema
execution_profile_digest and policy_set_digest are emitted in rollout plans
cost_model_id=linear_transaction_cost_rate.v1 is the only accepted V1 cost
  model
allow_partial_fills=true is refused until Runtime replay forwards that option
```

### V2.3 Prepare Contract Cleanup

Goal: make prepare strictly target-dispatch oriented.

Status: implemented for the current target-dispatch handler and public schema.

Changes:

```text
remove lattice_target public alias; target_id is canonical
remove public intent or narrow accepted public values to train|evaluate
requested_mode becomes plan|dry_run|execute
plan means no Runtime call
dry_run means Runtime dry-run is called for dispatchable targets
execute means dry-run gate then Runtime execution if policy allows
artifact-readiness targets route to inspect
replay routes to rollout
policy-training is reserved until a finite Runtime job contract exists
```

Acceptance:

```text
prepare plan does not call Runtime
prepare dry_run calls Runtime dry-run
prepare execute requires accepted dry-run evidence and policy permission
artifact-readiness refusal has next_safe_actions=[inspect]
replay refusal has next_safe_actions=[rollout]
policy-training refusal is explicit and stable
target-driver tests pass
```

Implemented notes:

```text
target_id is now the only public target identity; lattice_target is rejected
requested_mode accepts plan|dry_run|execute
plan is the safe default and does not call Runtime
dry_run and execute request Runtime dry-run for dispatchable targets
replay/rollout intent is refused with a pointer to hero.marshal.rollout
artifact_validation intent is refused with a pointer to hero.marshal.inspect
policy_training intent remains reserved until a finite Runtime contract exists
```

### V2.4 Inspect Alias Cleanup

Goal: keep inspect read-only while making its inputs canonical.

Status: implemented for the current public schema, handler validation, help,
and focused dispatch tests.

Changes:

```text
subject remains run|target|protocol|spawn|component|facts
mode validation becomes subject-specific in handler code
family/fact_family becomes fact_family_id
digest/fact_digest becomes fact_digest
digest_prefix becomes fact_digest_prefix
index/fact_index becomes fact_index
component becomes component_family_id where that is the intended domain
```

Acceptance:

```text
all six subjects have positive and negative tests
old aliases are rejected
facts summary, lineage, preview, and ambiguous digest-prefix behavior are tested
compare mode still never chooses a winner
```

Implemented notes:

```text
fact_family_id is the only public fact-family selector
fact_digest, fact_digest_prefix, and fact_index are the only public fact
  preview selectors
component_family_id is the only component subject selector
retired aliases family, fact_family, digest, digest_prefix, index, and
  component are rejected as unknown fields
facts mode is validated as summary|lineage|preview
Marshal still translates canonical fact selectors into the collapsed Lattice
  inspect arguments:
  hero.lattice.inspect subject=facts mode=summary
  hero.lattice.inspect subject=facts mode=lineage
  hero.lattice.inspect subject=facts mode=preview
```

### V2.5a Shared Read-Only Operator Envelope

Goal: add the shared operator-envelope fields to read-only packets first,
without changing dispatch/delegation semantics or removing legacy flat fields.

Status: implemented for `hero.marshal.status` and `hero.marshal.inspect`
top-level packets.

Implemented fields:

```text
schema_version
dispatch_state
refusal_reasons
warnings
next_safe_actions
authority
observed
evidence_refs
digests
```

Implemented authority namespace:

```text
authority.marshal_proves_target=false
authority.marshal_executes_runtime=false
authority.marshal_selects_checkpoint=false
authority.marshal_selects_policy=false
authority.marshal_edits_config=false
authority.marshal_makes_allocation_decision=false
authority.marshal_makes_market_readiness_decision=false
authority.marshal_makes_deployment_decision=false
```

Notes:

```text
existing flat authority flags remain for compatibility in V2.5a
status and inspect now expose next_safe_actions alongside existing
  next_safe_action fields where present
prepare and rollout response migration is intentionally left for V2.5b
```

### V2.5b Shared Dispatch/Delegation Operator Envelope

Goal: make all public tools easier for humans and automation to consume.

Status: implemented as an additive envelope supplement for
`hero.marshal.prepare` and `hero.marshal.rollout`.

Response envelope:

```text
ok
tool
schema_version
requested_mode
dispatch_state
operator_summary
stop_reason
refusal_reasons
warnings
next_safe_actions
authority
observed
evidence_refs
digests
machine_payload
```

Authority namespace:

```text
authority.marshal_proves_target=false
authority.marshal_executes_runtime=false
authority.marshal_selects_checkpoint=false
authority.marshal_selects_policy=false
authority.marshal_makes_allocation_decision=false
authority.marshal_makes_market_readiness_decision=false
authority.marshal_makes_deployment_decision=false
```

Observed evidence stays separate:

```text
observed.lattice_target_status
observed.lattice_proof_quoted
observed.runtime_handoff_delegated
observed.runtime_tool
observed.runtime_result_observed
```

Acceptance:

```text
status, prepare, rollout, and inspect all return the envelope
next_safe_actions is used everywhere
legacy top-level next_action is removed from public operator packets
Marshal may quote Lattice satisfaction but never sets marshal_proves_target=true
```

Implemented notes:

```text
prepare and rollout keep existing dispatch_state fields
prepare and rollout now add schema_version, refusal_reasons, warnings,
  next_safe_actions, authority, observed, evidence_refs, and digests
top-level next_action has been removed from prepare/rollout operator packets;
  use next_safe_actions
existing flat authority fields remain for compatibility
target-driver iteration records may still carry next_action as historical
  ledger evidence, not as the top-level operator command field
```

### V2.6 Documentation And Review Packet

Goal: keep the operator surface understandable while the contract changes.

Status: implemented for the current public Marshal V2 surface.

Changes:

```text
refresh hero_marshal.mcp --help after every public schema/mode change
keep README as the conceptual/operator manual
keep ROADMAP as the milestone and test contract
regenerate dump.temp only when sending a review packet
```

Acceptance:

```text
--help, README, ROADMAP, and hero_marshal.def agree on public modes and fields
dump.temp contains only review-relevant files when requested
```

Implemented notes:

```text
live --help documents status, prepare, rollout, and inspect
rollout help uses requested_mode=plan|execute and labels plan examples as plan
README and ROADMAP use next_safe_actions for public operator guidance
dump.temp was not regenerated during this pass because no review packet was
  requested
```


## Boundary

```text
Runtime executes and writes durable evidence.
Lattice proves target satisfaction from Runtime evidence.
Marshal prepares bounded handoffs and explains state.
```

Marshal must not become:

```text
proof authority
model selector
checkpoint selector
config editor
unbounded scheduler
performance judge
```

The only acceptable target-reached claim is:

```text
Lattice re-read Runtime evidence and reported the target satisfied.
```

## Current Contract

`hero.marshal.status` answers:

```text
What Marshal surface and receipts are visible?
```

`hero.marshal.prepare` answers:

```text
Given one target_id and finite driver policy, what is the next safe movement
toward that target, and can Marshal drive it through Runtime under policy?
```

`hero.marshal.rollout` answers:

```text
Given a completed Runtime job, policy set, finite replay limits, and Cajtucu
paper execution profile, what historical replay rollout can Runtime run next?
```

`hero.marshal.inspect` answers:

```text
What happened, what does Runtime evidence show, what does Lattice say when
quoted, and what should the operator inspect next?
```

Inspect subjects:

```text
subject=run
subject=target
subject=protocol
subject=spawn
subject=component
subject=facts
```

All inspect subjects are read-only. `subject=target` labels target status as
Lattice-sourced. `subject=protocol` has `identity_mode=report|strict`; strict
mode requires explicit expected identity fields. `subject=spawn` and
`subject=component` are Runtime evidence grouping only. `subject=facts` is an
audit-only fact-family summary/lineage/preview surface and never promotes facts
into target satisfaction.

## Testing Goal

Prove that the whole operator loop works:

```text
1. Marshal asks Lattice what target is missing.
2. Marshal prepares a concrete Runtime handoff.
3. Runtime dry-runs or executes under policy.
4. Runtime writes job evidence, terminal facts, checkpoint I/O, and exposure.
5. Lattice re-reads Runtime evidence and proves or blocks the target.
6. Marshal inspects the result in a clear operator packet.
```

The test campaign is complete when this loop succeeds for the real channel
VICReg -> channel MDN -> validation-eval chain, and failure drills prove the
same loop fails closed.

## Active Environment-Era Marshal Slice

Status: active as contract-only `rollout_marshal.v1`.

The current environment-era Marshal slice standardizes bounded historical
rollout preparation without becoming an executor, proof authority, policy
selector, trainer, or performance judge. Keep three responsibilities distinct:

```text
environment evidence inspection
  Read-only panel over Runtime replay artifacts and Lattice
  replay_environment facts. No dispatch, proof authority, policy selection,
  allocation, or market/deployment decision.

environment policy target coordination
  Bounded Runtime handoff for a dispatchable policy-training target, if and
  only if Lattice exposes such a target with a concrete Runtime job contract.

policy acceptance visibility
  Read-only display of disabled policy-gate reservations, never a Marshal
  decision or target-satisfaction claim.
```

Render disabled policy gates as disabled policy reservations in operator-facing
packets. They are context for missing future decision inputs, not active gates
and not Marshal authority.

Current implemented rollout slice:

```text
rollout_marshal.v1
  Input:
    completed Runtime job directory
    Runtime Hero config path
    replay artifact state and runtime_replay_batches.index
    Runtime executable path
    rollout_attempt_id and idempotency_key
    requested_mode=plan|execute
    base reserve graph node
    risky node universe
    policy set
    finite rollout bounds
    Cajtucu paper execution profile

  Gate:
    reject missing/non-directory Runtime job dir
    reject missing rollout_attempt_id or idempotency_key
    reject missing Runtime job state when replay evidence is required
    reject replay_artifacts_written=false
    reject missing replay_batch_index_path
    reject missing reserve/risky nodes
    reject missing or empty policy set
    reject missing or nonpositive max_steps
    reject reserve duplication inside risky nodes
    reject duplicate risky nodes
    reject unsupported policy tokens
    reject unsupported environment/backend ids
    reject live execution authority
    reject synthetic direct edges unless explicitly justified for research

  Output:
    non-authoritative replay command template
    resolved policy ids
    Cajtucu paper execution profile
    expected report path
    Runtime replay handoff result when requested_mode=execute
    all authority flags false
```

This gives the environment work a reusable Marshal vocabulary for:

```text
policy + environment + execution profile + Runtime artifacts
  -> trajectory evidence
```

The older `evaluation_marshal.v1` remains useful for compatibility and receipt
shape references, but new environment handoff language should use `rollout`.
Receipts and metrics belong after Runtime has produced reports, normally through
future `hero.marshal.inspect` support.

Expected evolution:

```text
1. Inspect environment evidence
   Marshal reads Lattice panels for replay facts and the reserved future
   replay_environment artifact-readiness proof. No Runtime dispatch.

2. Prepare deterministic rollout readiness
   Marshal prepares bounded historical replay rollout plans from completed
   Runtime jobs. With `requested_mode=execute`, Marshal invokes Runtime Hero
   replay and records handoff digests; Runtime still runs the replay. Lattice
   re-reads Runtime evidence to prove any target.

3. Reach policy-training artifact readiness
   Marshal may dispatch bounded Runtime environment jobs only when Lattice has
   a concrete dispatchable target and Runtime has a matching job contract.

4. Inspect policy acceptance reservations
   Marshal shows disabled policy-gate state and missing acceptance prerequisites.
   It does not choose policies or claim acceptance.
```

Potential future tool names are deliberately undecided. Preserve two mental
buckets:

```text
inspect_environment_panel
  read-only, no dispatch

reach_policy_training_artifact_target
  dispatchable only for concrete finite Runtime policy-training job contracts
```

`hero.marshal.rollout requested_mode=plan` is plan-only.
`hero.marshal.rollout requested_mode=execute` delegates the already bounded
rollout to Runtime Hero replay. Neither mode produces metrics or receipts.
Whether dispatchable policy training uses the existing `prepare` path or a
separate environment-specific handoff remains an open design question.

No policy-training handoff tool should be implemented until the contract
specifies:

```text
target id
environment/replay contract version
episode bundle ids or source ranges
max environment jobs
max parallel workers
max episodes
max attempts
resume ledger identity
Runtime job kind
policy artifact/checkpoint output contract
parent evidence digests
reward definition digest
selector split and anti-leakage policy
post-run Lattice target to recheck
terminal stop condition
```

Marshal may prepare and explain those bounded handoffs. It must not become an
unbounded environment scheduler, policy optimizer, reward judge, checkpoint
selector, allocation engine, or deployment authority. Target satisfaction still
comes only from Lattice re-reading Runtime evidence after Runtime has written
durable environment/training records.

For the current deterministic Wikimyei policy, Marshal should not dispatch
policy training. The useful surface is inspection of environment facts and
artifact-readiness proof. Dispatch becomes relevant only for trainable policies
with a finite Runtime job contract.

Open questions for the next design session:

```text
Is policy training reached through the existing prepare path or a
separate environment-specific handoff tool?
What fields make an environment handoff finite enough for Marshal to drive?
How does the resume ledger prevent repeated episode/job expansion?
Which post-run Lattice target proves the policy-training job wrote usable
artifacts without judging reward quality?
What does Marshal show when policy-gate reservations exist but remain disabled?
```

## Testing Milestones

### T0: Static Surface And Schema

Purpose: prove the public surface is small and harness-safe.

Checks:

```text
- Marshal tool catalog exposes exactly:
  hero.marshal.status
  hero.marshal.prepare
  hero.marshal.rollout
  hero.marshal.inspect

- old hidden or compatibility tools are not callable:
  hero.marshal.inspect_run
  hero.marshal.prepare_target_dispatch
  hero.marshal.dry_run_dispatch
  hero.marshal.execution_gate
  hero.marshal.replay_receipt
  hero.marshal.batch_preview

- all Marshal tool schemas are MCP-compatible
- subject enum is present for hero.marshal.inspect
- unknown fields fail closed
- unsupported subjects fail closed
```

Commands:

```sh
make -C src/tests/bench/kikijyeba/marshal run-test_kikijyeba_marshal_dispatch -j12
make -C src/main/hero build-marshal-hero -j12
/cuwacunu/.build/hero/hero_marshal.mcp --global-config /cuwacunu/src/config/.config --list-tools
```

Acceptance:

```text
four public tools
no hidden public tools
schema check passes
direct CLI and MCP expose the same primitives
```

### T1: Deterministic Evaluate Subjects

Purpose: prove `hero.marshal.inspect` reports without becoming proof authority.

Checks:

```text
subject=run:
  latest_chain, training_state, single_job, and compare modes work
  compare describes deltas and does not choose a winner

subject=target:
  calls hero.lattice.evaluate operation=deficit
  labels target_status_source=hero.lattice.evaluate operation=deficit
  labels proof_authority=lattice
  does not imply certificate inspection unless certificate material exists
  malformed Lattice payload fails closed

subject=protocol:
  report mode treats observed-only identity as observation
  strict mode requires expected identity fields
  strict mode returns identity_verified=true only on complete match
  mismatch, missing, and conflict states are visible

subject=spawn:
  accepts spawn_id, component_spawn_id, or component_spawn_fingerprint
  no-match returns an observation packet, not proof

subject=component:
  groups jobs by component family
  reports spawn ids, fingerprints, wave actions, and job summaries

subject=facts:
  fact summary works
  lineage is audit-only
  preview is opt-in
  digest_prefix ambiguity fails closed
  facts are never promoted into target satisfaction
```

Acceptance:

```text
all subjects return read_only=true
all subjects return target_satisfaction_claimed=false
protocol/spawn/component declare evidence_scope=runtime_jobs_only
protocol/spawn/component declare proof_authority=none
```

### T2: Target Driver Dry-Run

Purpose: prove Marshal can move toward a Lattice target without mutating state.

Scenario:

```text
target_id = channel_mdn_validation_eval_ready
drive_mode = one_step
requested_mode = dry_run
include_runtime_dry_run = true
```

Checks:

```text
- Marshal calls Lattice evaluate operation=deficit
- symbolic latest_satisfying inputs are materialized or blocked explicitly
- Runtime active wave or handoff matches target/mode/range/checkpoints
- Runtime policy is respected
- dry-run does not write model evidence
- target-driver ledger records evaluate operation=deficit and handoff/request digests
```

Acceptance:

```text
dry-run accepted or blocked with a stable reason
no Runtime mutation
no target satisfaction claim by Marshal
next safe actions are clear
```

### T3: Target Driver Execute On A Small Controlled Wave

Purpose: prove bounded execution works without broad automation.

Preconditions:

```text
Runtime policy explicitly allows execute for this test
driver_policy.max_waves is finite
driver_policy.max_wall_clock_seconds is finite
driver_policy.allow_execute=true
driver_policy.require_runtime_job_completion=true
driver_policy.require_post_wave_lattice_satisfied_check=true
```

Checks:

```text
- Runtime execution is reached only after dry-run evidence and execution gate
- train waves additionally require allow_train_execute on both Marshal policy
  and Runtime policy
- Runtime writes job.manifest, job.state, runtime.result.fact, and when needed
  runtime.checkpoint_io.fact
- Marshal records durable Runtime job/fact/handoff digests in the ledger
- Lattice rechecks after Runtime writes evidence
```

Acceptance:

```text
Marshal terminal state is reached, blocked, or budget_exhausted
reached means Lattice satisfaction, not Runtime success alone
```

### T4: End-To-End Training Chain

Purpose: prove the whole current training/eval path works.

Target chain:

```text
vicreg_train_core_ready
channel_mdn_train_core_ready
channel_mdn_train_core_no_validation_leakage
channel_mdn_train_core_no_test_leakage
channel_mdn_validation_eval_ready
```

Expected sequence:

```text
1. train channel VICReg on train_core
2. prove vicreg_train_core_ready
3. train channel MDN on train_core with resolved VICReg checkpoint
4. prove channel_mdn_train_core_ready
5. prove no validation/test leakage
6. run channel MDN validation eval with exact MDN and representation checkpoints
7. prove channel_mdn_validation_eval_ready
8. evaluate the chain with hero.marshal.inspect subject=run
```

Acceptance:

```text
validation eval has optimizer_steps=0
validation eval has checkpoint_written=false
validation eval loaded exact MDN checkpoint
validation eval loaded exact representation checkpoint
checkpoint closure is complete
all required Lattice targets are satisfied
Marshal inspect explains the chain in one readable packet
```

### T5: Failure Drills

Purpose: prove the system fails closed.

Required drills:

```text
Runtime policy refusal:
  allow_execute=false blocks execute
  allow_train_execute=false blocks train execute

Lattice blocker:
  evaluate operation=deficit has no dispatchable plan
  unresolved latest_satisfying input blocks Runtime handoff

Protocol identity:
  protocol strict mode with missing expected identity fails
  protocol mismatch reports identity_mismatch
  conflicting observed identities report identity_conflict

Runtime handoff:
  active wave target mismatch blocks
  active wave range mismatch blocks
  checkpoint input mismatch blocks
  symbolic model-state input blocks

Runtime terminal evidence:
  require_runtime_job_completion=true blocks when runtime.result.fact is absent
  missing job.manifest blocks durable completion
  missing checkpoint_io fact blocks when checkpoint I/O is required

Ledger/replay:
  stale target id blocks resume
  stale driver policy digest blocks resume
  stale Runtime policy digest blocks resume
  tampered iteration digest blocks replay

Warnings:
  blocking warning stops the target driver
  unknown severity fails closed when warning stop policy is enabled
  missing severity fails closed when warning stop policy is enabled

No progress:
  repeated evaluate operation=deficit/suggested_wave signature stops with no-progress reason
```

Acceptance:

```text
each failure has a stable stop_reason
no failure path executes after a blocked gate
no failure path claims target satisfaction
```

### T6: Evidence And Replay Audit

Purpose: prove Marshal can be audited without trusting Marshal as evidence.

Checks:

```text
- target-driver ledger binds:
  target id
  driver policy digest
  Lattice evaluate operation=deficit digest
  suggested wave digest
  Runtime handoff digest
  Runtime job id
  job.manifest digest
  runtime.result.fact digest
  runtime.checkpoint_io.fact digest when required
  post-run Lattice status digest when available

- replay rejects stale identities:
  protocol contract
  graph order
  target spec
  split policy
  Runtime policy
  driver policy

- compact ledgers remain non-authoritative
- compact ledgers keep enough durable evidence identity to resume safely
```

Acceptance:

```text
replay explains what can be trusted
replay never proves target satisfaction
Runtime and Lattice evidence remain authoritative
```

### T7: Performance Budget

Purpose: catch expensive regressions without confusing speed with proof.

Commands:

```sh
make -C src/tests/bench/kikijyeba/marshal run-test_kikijyeba_marshal_performance_budget -j12
```

Checks:

```text
advice validation
dry-run preview
Runtime policy read
tool schema catalog
CLI tool catalog
Runtime handoff
receipt write
```

Acceptance:

```text
all budget stages pass
performance report says speed is not target satisfaction evidence
```

### T8: Operator Experience Test

Purpose: prove the tools are usable by an operator.

For each main command, output should answer:

```text
hero.marshal.status:
  what surface exists?
  what receipts/policy are visible?

hero.marshal.prepare:
  is the target already reached?
  what wave would move it?
  did Runtime run or dry-run?
  why did the loop stop?
  what are the next safe actions?

hero.marshal.rollout:
  what completed Runtime job is being replayed?
  what policy set and Cajtucu execution profile are bound?
  is this a plan-mode rollout plan or a Runtime replay handoff?
  what finite step/parallel limits apply?
  what Runtime replay evidence should be inspected next?

hero.marshal.inspect:
  what happened?
  what Runtime jobs/facts exist?
  what Lattice status was quoted?
  what identity/spawn/component evidence was observed?
  what warnings or suspicious items matter?
```

Acceptance:

```text
default output fits in a compact operator packet
machine payload is opt-in
no raw digest wall unless include_machine_payload=true
next_safe_actions is actionable
```

## Complete Test Run Order

Use this order for a full Marshal/Lattice/Runtime confidence pass:

```text
1. T0 static surface and schema
2. T1 deterministic inspect subjects
3. T7 performance budget
4. T2 target driver dry-run
5. T5 failure drills
6. T3 small controlled execute
7. T4 end-to-end training chain
8. T6 evidence and replay audit
9. T8 operator experience review
```

Stop immediately on the first failed gate that could permit mutation after a
blocked condition.

## Blessing Criteria

Marshal testing is considered complete for the current cycle when:

```text
- public tool surface is exactly
  status/prepare/rollout/inspect
- every inspect subject has passing positive and negative tests
- target driver dry-run is deterministic and non-mutating
- target driver execute is bounded by explicit finite policy
- Runtime handoffs are concrete and identity-bound
- Runtime writes durable terminal evidence
- Lattice re-proves after Runtime evidence exists
- Marshal reports reached only when Lattice reports satisfied
- replay and resume reject stale or tampered ledgers
- failure drills fail closed
- operator packets are understandable without reading raw job files
```

## Not Now

Do not add these as part of this testing roadmap:

```text
multi-target scheduling
dependency traversal
automatic config editing
checkpoint ranking by validation metric
best-model selection
Marshal-owned performance gates
unbounded objective agents
Codex-in-the-loop public evaluation
```

Potential future tool after the deterministic surface proves itself:

```text
hero.marshal.review
```

If added later, it must consume deterministic `hero.marshal.inspect` output and
remain advisory. It must not change target status, issue counts, proof gates,
checkpoint choice, or Runtime execution policy.
