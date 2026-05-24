# Marshal Scheduler Research Packet

This packet records the scheduler requirements without implementing a
scheduler. It exists so scheduler work stays separate from deterministic
dispatch.

## Boundary

```text
Lattice decides proof.
Marshal validates dispatch advice and policy.
Runtime executes allowed waves and writes evidence.
Scheduler, if added later, may only choose when to ask Marshal for a bounded
dispatch decision.
```

The scheduler must not:

```text
- satisfy lattice targets
- rewrite lattice targets
- bypass Runtime Hero policy
- manufacture confirmation tokens
- ignore warnings as approval
- loop forever
```

## Wake Events

Possible wake events for a future scheduler:

```text
- explicit operator request
- new runtime job completed
- new lattice target advice available
- scheduled maintenance window
```

A future first scheduler milestone must support only explicit operator requests.
Other wake events require separate stop conditions and policy review.

## Stop Conditions

Every unattended loop must have all of:

```text
- max dispatches per run
- max wall-clock duration
- max consecutive refusals
- max consecutive Runtime Hero failures
- stop on any leakage or lineage failure
- stop on unknown target status
- stop when a target becomes satisfied
```

## Confirmation

Unattended execution is not approved by this packet.

If it is ever considered, confirmation must be represented as an explicit policy
artifact that binds the same fields as the M3 confirmation token:

```text
target_id
runtime_root
active identity
suggested_wave
plan inputs
requested mode
```

## Eligible Targets

Automatic selection across unrelated targets is deferred. A future scheduler
may only operate on an operator-provided allowlist of target ids, and each target
must still pass the same Marshal M1/M2/M3 gates.

## Warnings

Warnings are not approval. A scheduler must surface warnings in the stop/receipt
summary and must not convert warning-only health into target satisfaction.

## Required Stop Receipt

Any scheduler run must produce a stop receipt recording:

```text
why it woke
which targets were allowed
which dispatches were attempted
which Runtime Hero responses were observed
why it stopped
whether any target must be rechecked by Lattice Hero
```

This receipt remains non-authoritative for lattice target satisfaction.
