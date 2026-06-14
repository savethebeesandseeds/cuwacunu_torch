# Marshal Roadmap

Marshal is intentionally small:

```text
hero.marshal.status
hero.marshal.prepare.*
hero.marshal.rollout
hero.marshal.paper_online.session_handoff
hero.marshal.inspect
```

This roadmap tracks future finite coordination work. It does not retain the
completed V2 surface-collapse, cost-aware rollout, or pre-PPO rehearsal history.

## Boundary

```text
Lattice proves.
Runtime executes.
Environment admits environment evidence and certified sidecars.
Marshal coordinates, validates, delegates, records, and explains.
Cajtucu paper-executes replay mechanics under Runtime replay.
```

Marshal must not become:

```text
proof authority
model selector
checkpoint selector
policy trainer
reward judge
config editor
unbounded scheduler
allocation engine
market-readiness authority
deployment authority
```

## Current Stable Surface

```text
status
  read-only Marshal visibility and boundary state

prepare
  one dispatchable Lattice target -> bounded Runtime handoff plan/dry-run/execute

rollout
  completed Runtime job evidence -> bounded replay rollout through Runtime

paper_online.session_handoff
  paper-online readiness/admission evidence -> bounded Environment session
  handoff plan/dry-run/run receipt

inspect
  read-only Runtime/Lattice/Marshal evidence explanation
```

Marshal can coordinate bounded cost-aware replay through Runtime and inspect the
resulting Runtime/Lattice evidence. Direct `cuwacunu_exec` remains an explicit
debug path with warning, not the recommended proof-clean operator path.

## Completed Marshal Work

### Paper-Online Session Handoff

Milestone:

```text
paper_online_session_marshal_handoff.v1
paper_online_session_marshal_run_handoff.v1
```

Current state:

- `hero.marshal.paper_online.session_handoff mode=plan|dry_run|run` prepares
  or delegates a compact handoff from a `handoff_request` object or
  `handoff_request_path`.
- Plan mode builds Environment-compatible admission/session payload digests
  without calling Environment or writing a receipt.
- Dry-run mode calls only safe validators:
  `hero.environment.certify.paper_online_session_admission mode=check` and,
  once admission evidence exists,
  `hero.environment.paper_online.session mode=validate`.
- Run mode repeats the same gates and then delegates
  `hero.environment.paper_online.session mode=run`; successful execution
  reports `dispatch_state=executed`, while a failed delegated run reports
  `dispatch_state=run_blocked`.
- Dry-run and run write a durable
  `kikijyeba.marshal.paper_online_session_handoff_receipt.v1` receipt with
  request digests, Environment validation/run digests, readiness/admission and
  session sidecar visibility, authority denials, and next safe actions.
- Marshal never issues admission, executes Cajtucu paper mechanics, routes
  broker orders, selects policy/checkpoint, proves Lattice targets, or
  authorizes live capital.

### Dispatch Readiness Repair

Milestone:

```text
marshal_dispatch_readiness_repair.v1
```

Current state:

- Post-dev-nuke train-core recovery starts through Marshal again without a
  direct `cuwacunu_exec` bypass.
- Lattice preserves first-wave advice when no completed Runtime candidate
  exists, while still failing closed if existing candidate evidence cannot be
  compared to active proof identity.
- Marshal accepts first-wave plan advice without requiring a runtime-derived
  contract fingerprint before the first Runtime evidence exists.
- `hero.marshal.prepare.train` returns representation train-core plan and
  dry-run-ready packets through the canonical `src/config/.config`.
- `cwu_02v_mdn_train_core_no_test_leakage` correctly points back to the
  representation checkpoint-source dependency until representation evidence is
  completed and proven.
- `policy_training_artifact_ready` remains proof-only and non-dispatchable by
  design.

Remaining boundary:

- The checked-in Marshal profiles now authorize non-live execute-mode training.
  Live execution remains disabled by Marshal/Runtime/Environment contracts.

## Next Marshal Work

## Future Milestone: Tsodao Handoff

Tsodao should protect approved settings after evidence exists. Marshal may
eventually prepare or inspect Tsodao-bound handoffs, but only after Tsodao has a
finite contract for:

- settings identity
- evidence digest binding
- promotion criteria
- allowed mutation policy
- rollback/revocation evidence

Marshal must not use Tsodao as a hidden optimizer.

## Validation

Use the smallest relevant checks for the touched surface.

Marshal dispatch:

```text
make -C src/tests/bench/kikijyeba/marshal -j12 run-test_kikijyeba_marshal_dispatch
```

Marshal rollout:

```text
make -C src/tests/bench/kikijyeba/marshal -j12 run-test_kikijyeba_marshal_rollout
```

Hero schema compatibility:

```text
make -C src/tests/bench/kikijyeba/runtime -j12 run-test_hero_mcp_schema_compat
```

Build:

```text
make -C src/main/hero -j12 build-marshal-hero
```
