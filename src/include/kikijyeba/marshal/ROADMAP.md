# Marshal Testing Roadmap

Marshal is now intentionally small:

```text
hero.marshal.status
hero.marshal.reach_lattice_target
hero.marshal.evaluate
```

This roadmap is rebased around testing the joint system rather than carrying
the full implementation history. Future work should be opened as finite goals.
Avoid broad "finish Marshal" goals.

Current testing work should be recorded here only when it is still active.
Completed test receipts belong in runtime/Lattice evidence, not in a separate
Marshal status file.


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

`hero.marshal.reach_lattice_target` answers:

```text
Given one target_id and finite driver policy, what is the next safe movement
toward that target, and can Marshal drive it through Runtime under policy?
```

`hero.marshal.evaluate` answers:

```text
What happened, what does Runtime evidence show, what does Lattice say when
quoted, and what should the operator inspect next?
```

Evaluate subjects:

```text
subject=run
subject=target
subject=protocol
subject=spawn
subject=component
```

All evaluate subjects are read-only. `subject=target` labels target status as
Lattice-sourced. `subject=protocol` has `identity_mode=report|strict`; strict
mode requires explicit expected identity fields. `subject=spawn` and
`subject=component` are Runtime evidence grouping only.

## Testing Goal

Prove that the whole operator loop works:

```text
1. Marshal asks Lattice what target is missing.
2. Marshal prepares a concrete Runtime handoff.
3. Runtime dry-runs or executes under policy.
4. Runtime writes job evidence, terminal facts, checkpoint I/O, and exposure.
5. Lattice re-reads Runtime evidence and proves or blocks the target.
6. Marshal evaluates the result in a clear operator packet.
```

The test campaign is complete when this loop succeeds for the real channel
VICReg -> channel MDN -> validation-eval chain, and failure drills prove the
same loop fails closed.

## Testing Milestones

### T0: Static Surface And Schema

Purpose: prove the public surface is small and harness-safe.

Checks:

```text
- Marshal tool catalog exposes exactly:
  hero.marshal.status
  hero.marshal.reach_lattice_target
  hero.marshal.evaluate

- old hidden or compatibility tools are not callable:
  hero.marshal.evaluate_run
  hero.marshal.prepare_target_dispatch
  hero.marshal.dry_run_dispatch
  hero.marshal.execution_gate
  hero.marshal.replay_receipt
  hero.marshal.batch_preview

- all Marshal tool schemas are MCP-compatible
- subject enum is present for hero.marshal.evaluate
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
three public tools
no hidden public tools
schema check passes
direct CLI and MCP expose the same primitives
```

### T1: Deterministic Evaluate Subjects

Purpose: prove `hero.marshal.evaluate` reports without becoming proof authority.

Checks:

```text
subject=run:
  latest_chain, training_state, single_job, and compare modes work
  compare describes deltas and does not choose a winner

subject=target:
  calls hero.lattice.target_deficit
  labels target_status_source=hero.lattice.target_deficit
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
- Marshal calls Lattice target_deficit
- symbolic latest_satisfying inputs are materialized or blocked explicitly
- Runtime active wave or handoff matches target/mode/range/checkpoints
- Runtime policy is respected
- dry-run does not write model evidence
- target-driver ledger records target_deficit and handoff/request digests
```

Acceptance:

```text
dry-run accepted or blocked with a stable reason
no Runtime mutation
no target satisfaction claim by Marshal
next safe action is clear
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
8. evaluate the chain with hero.marshal.evaluate subject=run
```

Acceptance:

```text
validation eval has optimizer_steps=0
validation eval has checkpoint_written=false
validation eval loaded exact MDN checkpoint
validation eval loaded exact representation checkpoint
checkpoint closure is complete
all required Lattice targets are satisfied
Marshal evaluate explains the chain in one readable packet
```

### T5: Failure Drills

Purpose: prove the system fails closed.

Required drills:

```text
Runtime policy refusal:
  allow_execute=false blocks execute
  allow_train_execute=false blocks train execute

Lattice blocker:
  target_deficit has no dispatchable plan
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
  repeated target_deficit/suggested_wave signature stops with no-progress reason
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
  Lattice target_deficit digest
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

hero.marshal.reach_lattice_target:
  is the target already reached?
  what wave would move it?
  did Runtime run or dry-run?
  why did the loop stop?
  what is the next safe action?

hero.marshal.evaluate:
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
next_safe_action is actionable
```

## Complete Test Run Order

Use this order for a full Marshal/Lattice/Runtime confidence pass:

```text
1. T0 static surface and schema
2. T1 deterministic evaluate subjects
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
- public tool surface is exactly status/reach_lattice_target/evaluate
- every evaluate subject has passing positive and negative tests
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

If added later, it must consume deterministic `hero.marshal.evaluate` output and
remain advisory. It must not change target status, issue counts, proof gates,
checkpoint choice, or Runtime execution policy.
