# Lattice Fact-Emission Contract

Snapshot date: 2026-05-23

This contract is the bounded V2 review artifact for Wikimyei fact emission. It
keeps the lattice read-only: runtime/components write artifacts that describe
what happened, and the lattice scans, normalizes, evaluates, certifies, and
explains those artifacts without executing waves.

## Boundary

Use "fact" for a durable, normalized evidence row with schema, identity, source
range, and authority semantics. Do not call every report field a fact.

```text
component report field:
  raw component-owned output, such as optimizer_steps, mean_loss,
  checkpoint_path, valid target counts, geometry diagnostics, or support counts

runtime artifact:
  runtime-owned job.manifest, job.state, component report path, checkpoint path,
  and emitted lattice sidecar paths

lattice fact:
  normalized evidence row consumed by the exposure ledger or target evaluator

derived summary:
  scanner/Hero aggregate over facts; useful for inspection, not source of truth

proof certificate:
  target-evaluation proof object; proves why a target status was returned, but
  is not itself runtime evidence
```

Model-state paths, loaded checkpoint paths, output checkpoint paths, and
model-state admission flags such as `ALLOW_UNTRAINED_REPRESENTATION` are runtime
evidence. They must not alter protocol contract identity. They become
proof-relevant only when reports, facts, checkpoint closure, and target rules
bind them to the active identity.

## Owners

```text
Wikimyei components:
  write component reports and debug .lls diagnostics for their own execution

Runtime:
  writes job.manifest and job.state for runtime attempts. It may also write
  lattice.exposure.fact sidecars for dry-run attempts as audit/wiring evidence.
  It writes lattice.checkpoint.fact sidecars only when a checkpoint artifact
  exists.

Lattice scanner:
  derives audit/visibility fact families from runtime artifacts and component
  reports; it does not execute jobs or invent missing evidence

Lattice targets:
  decide which fact families are authority, warning, visibility, or deferred
  performance evidence for a target

Hero:
  exposes scans, target statuses, proof certificates, warnings, and plans;
  Lattice Hero remains read-only and Runtime Hero remains the executor
```

## Fact Families

```text
kikijyeba.lattice.exposure.v1
  producer: Runtime sidecar via job_runner after terminal jobs; scanner fallback
            may derive it from job.manifest, job.state, and component reports.
  authority: coverage, load, lineage, and leakage input when job_status is
             completed and identity matches.
  non-authority: dry_run/running/failed rows may be scanned for audit and
                 wiring visibility, but they do not contribute readiness
                 coverage, repeated exposure load, or completed causal exposure
                 witnesses.

kikijyeba.lattice.checkpoint.v1
  producer: Runtime sidecar when a checkpoint exists.
  authority: checkpoint closure. V2 prefers checkpoint_id plus
             checkpoint_file_digest; legacy path fallback is explicit and must
             be reported.
  failure: wrong digest, mismatched id, or unresolved upstream input fails
           closed.

kikijyeba.lattice.source_receipt.v1
  producer: Lattice scanner from exposure.source_file_receipts.
  authority: audit/provenance only.
  non-authority: source receipts do not replace row-index coverage/leakage math
                 and do not alter contract identity.
  failure: malformed non-empty receipts warn and are counted; missing receipts
           are audit absence.

kikijyeba.lattice.selection_signal.v1
  producer: Lattice scanner from exposure rows with use_selection_signal=true.
  authority: causal leakage visibility when a protected split forbids
             selection_signal.
  non-authority: it is not a scheduler, selector, or model chooser.

kikijyeba.lattice.node_exposure.v1
  producer: Lattice scanner from MDN report per-node fields.
  authority: MDN head support visibility and node-support warnings; target
             rules may require MDN head counts explicitly.
  non-authority: these rows are not VICReg or NodeLift support evidence.

kikijyeba.lattice.representation_support.v1
  producer: Lattice scanner from representation reports and NodeLift .lls
            sidecars when support payload fields exist.
  authority: shared-representation support visibility and warnings only.
  non-authority: not a hard VICReg readiness gate, not one VICReg per node, and
                 never backfilled from downstream MDN node rows.
```

## Required Bindings

Every promoted fact row must bind, where applicable, to:

```text
contract_fingerprint
graph_order_fingerprint
source_cursor_token
split_policy_fingerprint
component_assembly_fingerprint
target_component
job_id
wave_id
job_status
wave_action
anchor/completed row-index intervals
checkpoint_id and checkpoint_file_digest for checkpoint facts
parent_exposure_fact_digest for derived fact families
```

If a fact family cannot provide the required binding, it must remain diagnostic
or visibility evidence until the emitting component/report can provide honest
denominators and identity fields.

## Component Responsibilities

Representation/VICReg reports should keep emitting:

```text
checkpoint_written
checkpoint_path
optimizer_steps
mean_loss
finite_parameter_check
VICReg loss components
valid projection rows
adapter kept/dropped/support counts
optional geometry diagnostics
```

NodeLift and representation debug sidecars may emit aggregate support
diagnostics. Channel representation reports may also emit honest per-node
shared-representation denominators and support counts such as valid lifted rows,
valid lifted feature counts, valid lifted cell counts, and valid projection
rows. The lattice may promote those fields into node-indexed
`representation_support` rows, but downstream MDN rows must not be used to
synthesize them.

MDN reports should keep emitting:

```text
loaded representation checkpoint
loaded MDN checkpoint for validation eval
checkpoint_written/checkpoint_path when training mutates model state
optimizer_steps
valid target counts/fractions
active/trained/evaluated node-head counts
per-node support rows
distribution diagnostics when available
```

Validation evaluation must report exact loaded model-state inputs. A validation
eval target is not satisfied by a fresh MDN, a wrong MDN checkpoint, a wrong
representation checkpoint, or a run that mutates/writes model state.

## Guardrails

```text
- completed runtime evidence can satisfy targets; dry-run evidence can only
  validate wiring
- row-index intervals remain coverage and leakage authority
- source-key windows and source receipts remain audit coordinates
- model-state inputs remain outside contract identity
- diagnostic metrics remain warnings/visibility unless a target rule promotes
  them
- DB/index rows must be rebuildable cache rows, not truth
- Lattice Hero never executes waves
```
