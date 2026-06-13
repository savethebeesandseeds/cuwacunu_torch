# Component / Policy Surface Reconciliation Audit V1

Milestone:

```text
component_policy_surface_reconciliation_audit.v1
```

## Scope

This audit checks whether graph-node allocation policy training now resembles
the older Wikimyei component lanes enough for operators, Runtime, Marshal, and
Lattice to reason about the system as one architecture rather than two parallel
systems.

The audit is intentionally not an implementation milestone. It does not change
Runtime behavior, Lattice targets, Marshal dispatch, Cajtucu execution, PPO
training, replay behavior, or component schemas. It records the current boundary
and defines the next finite cleanup goals.

## Current Verdict

The policy lane is now close enough to the component model at the operator and
identity level, but it should not be forced into the old component-wave shape.

Recommended direction:

```text
Use partial unification.

Keep:
  component_wave
  policy_training_profile
  environment_replay_profile

Add/keep:
  the shared operator-surface vocabulary and the implemented digest layer for
  older representation/MDN components.
```

Policy training should keep its own profile because it binds replay lineage,
causal schedule, reward, execution, action distribution, PPO update evidence,
and checkpoint lineage. Those are not ordinary representation/MDN wave
settings. The older component lanes should move closer to policy by exposing a
clearer operator-surface digest, not by pretending policy training is just
another wave.

Update: `component_operator_surface_digest_contract.v1` implements that first
digest layer for representation and MDN component-wave manifests, Runtime
terminal facts, Lattice exposure facts, and Marshal inspect diagnostics.

## Observed Current Surfaces

Runtime documentation already defines three operator profiles:

```text
component_wave
  Runtime wave selected from hero.runtime.wave.dsl.
  Used for representation/MDN train or run jobs over a source range.

policy_training_profile
  Runtime policy-training contract.
  Used for graph-node allocation training, including replay lineage, causal
  schedule, policy DSL/net/features/jkimyei, action distribution, reward,
  execution profile, target-node universe, PPO evidence, and checkpoint lineage.

environment_replay_profile
  Kikijyeba historical replay environment profile.
  Used for reset/step/reward worlds that consume completed Runtime artifacts
  and call Cajtucu paper execution.
```

The active Runtime wave inspected through Hero Runtime is still a component
wave:

```text
protocol: cwu_02v
target component: wikimyei.inference.expected_value.mdn
active representation: wikimyei.representation.encoding.mtf_jepa_mae_vicreg
observer: wikimyei.observer.belief
allocation policy family: wikimyei.policy.portfolio.spot_distributional_utility
policy training wave: false
```

That is correct. The active wave still describes an MDN/component job chain, not
a policy-training run.

## Comparison Table

| Surface | Main job | Operator profile | Identity today | Should change now? |
| --- | --- | --- | --- | --- |
| `wikimyei.representation.encoding.vicreg` | Train/run representation | `component_wave` | `vicreg_assembly_fingerprint` | Not in this audit |
| `wikimyei.representation.encoding.mtf_jepa_mae_vicreg` | Train/run representation | `component_wave` | `mtf_jepa_mae_vicreg_assembly_fingerprint` | Not in this audit |
| `wikimyei.inference.expected_value.mdn` | Train/run MDN inference head | `component_wave` | `mdn_assembly_fingerprint` | Not in this audit |
| `wikimyei.observer.belief` | Build AllocationBelief from upstream artifacts | Component/replay consumer | Parent digest lineage in replay/policy facts | Needs no collapse into policy |
| `kikijyeba.environment.replay.v1` | Historical reset/step/reward world | `environment_replay_profile` | Replay report/fact digests | Should remain a profile, not a wave |
| `wikimyei.policy.portfolio.spot_distributional_utility` | Deterministic policy baseline | Replay/policy set member | Policy set/report identity | Keep as deterministic baseline |
| `wikimyei.policy.portfolio.graph_node_allocation` | Trainable graph-node allocation | `policy_training_profile` | `policy_operator_surface_digest` plus policy DSL/net/features/jkimyei and checkpoint digests | Already component-like enough for policy |

## What Is Already Reconciled

Policy now has component-like discipline where it matters:

- Runtime policy jobs use `policy_operator_surface_digest` as the
  component-spawn identity for layout and fact wiring.
- Lattice policy-training facts require policy DSL, policy net, feature
  manifest, policy jkimyei, target-node universe, action distribution, reward,
  execution, causal schedule, checkpoint, optimizer, CUDA, and resume lineage.
- Runtime manifests can expose policy surface fields side by side with older
  component assembly fingerprints.
- Policy artifacts are not allowed to claim paper-online readiness, market
  readiness, deployment readiness, policy quality, selection, or live authority.
- The old deterministic spot-distributional-utility policy can remain as a
  baseline without being the trainable PPO surface.

This means policy already looks like a first-class trainable surface to Runtime
and Lattice. The remaining split is mostly vocabulary and older-component
identity shape.

## What Is Still Split

Older component lanes still expose family-specific assembly fingerprints:

```text
vicreg_assembly_fingerprint
mtf_jepa_mae_vicreg_assembly_fingerprint
mdn_assembly_fingerprint
dock_binding_fingerprint
```

Policy exposes a broader surface digest:

```text
policy_operator_surface_digest
```

That asymmetry is understandable because policy training has more contract
inputs, but it makes the architecture read unevenly. Operators see one policy
surface and several older family-specific component fingerprints.

The cleanup should not remove the family-specific fingerprints. They are useful
and already part of Lattice/Marshal compatibility checks. The cleanup should add
a shared umbrella term and, later, a generic field that older components can
also expose.

## Recommendation

Adopt this vocabulary:

```text
operator_surface
  The complete operator-visible identity surface for a trainable or runnable
  component family.

operator_surface_digest
  A digest of the canonical operator surface for that family.

component_operator_surface_digest
  The generic field older component lanes can add later.

policy_operator_surface_digest
  The existing policy-specific instance, kept because the policy surface is
  larger and already deployed in Runtime/Lattice evidence.
```

This gives us one mental model:

```text
Every trainable thing has an operator surface.
Different families have different surface fields.
Lattice can prove the family-specific surface without pretending all families
are identical.
```

## Bring Policy Closer To Components

Do:

- Keep using Runtime layout, manifests, facts, and Lattice readiness targets.
- Keep policy jobs bound by family ID, source/profile identity, checkpoint
  lineage, and component-spawn identity.
- Keep policy training dispatchable through Runtime contracts and later Marshal
  materialized handoffs.

Do not:

- Rename `policy_training_profile` to `wave`.
- Hide replay, reward, action distribution, PPO update, or causal schedule
  fields inside ordinary wave settings.
- Let policy training run through ad hoc direct execution.

## Bring Older Components Closer To Policy

Do:

- Keep the explicit `component_operator_surface_digest` for representation and
  MDN jobs, and extend it only through bounded follow-up contracts if additional
  component families need the same surface.
- Define a canonical text for each older component family that includes the
  durable DSL/net/jkimyei/config/source/protocol/dock fields that already affect
  the assembly fingerprint.
- Keep the existing `vicreg_assembly_fingerprint`,
  `mtf_jepa_mae_vicreg_assembly_fingerprint`, `mdn_assembly_fingerprint`, and
  `dock_binding_fingerprint` fields for compatibility and readability.
- Let Lattice expose the generic operator surface beside the family-specific
  field.

Do not:

- Collapse all older component identities into `policy_operator_surface_digest`.
- Remove family-specific fingerprints in the first cleanup.
- Change source-range or wave semantics just to match policy.

## Environment Replay

Environment replay should not become a component wave.

It is a world/profile:

```text
completed Runtime artifacts
  -> Kikijyeba replay reset/step/reward
  -> Cajtucu paper execution
  -> replay report/fact evidence
```

It consumes component/policy artifacts but is not itself a trainable component.
The correct standardization is:

```text
environment_replay_profile
  not
environment_wave
```

## Policy Baselines

`wikimyei.policy.portfolio.spot_distributional_utility` should be treated as
deprecated for new trainable-policy design, but not removed. It remains useful
as a deterministic replay baseline and sanity check.

The trainable policy lane remains:

```text
wikimyei.policy.portfolio.graph_node_allocation
```

## Follow-Up Goals

### 1. Component Operator Surface Digest Contract V1

Status:

```text
completed
```

Milestone:

```text
component_operator_surface_digest_contract.v1
```

Goal:

```text
Define and add a generic component operator-surface digest for older
representation/MDN component lanes while preserving the existing
family-specific assembly fingerprints.
```

Acceptance:

- representation/MDN manifests can carry `component_operator_surface_digest`
- Lattice facts can expose it without replacing family-specific fields
- Marshal identity reports can show it as display/diagnostic identity
- canonical text includes only durable operator-relevant inputs
- no readiness target weakens its current family-specific checks

### 2. Marshal Policy Training Profile Handoff V1

Milestone:

```text
marshal_policy_training_profile_handoff.v1
```

Goal:

```text
Make policy-training usability look like other readiness-target dispatch paths
by materializing a Marshal-approved policy-training handoff before non-dry-run
Runtime policy execution.
```

Acceptance:

- Marshal can prepare policy-training profile handoffs
- Runtime refuses non-dry-run handoffs that are missing required Marshal fields
  where policy requires Marshal mediation
- handoff records policy operator surface, replay profile, causal schedule,
  reward, execution, and checkpoint lineage
- direct `cuwacunu_exec` remains an explicit emergency bypass, not the normal
  operator path

### 3. Environment Replay Profile Catalog V1

Milestone:

```text
environment_replay_profile_catalog.v1
```

Goal:

```text
Clarify replay profiles as first-class environment profiles without calling
them waves.
```

Acceptance:

- replay profiles are discoverable from docs/Config/Runtime surfaces
- profile identity binds environment contract, reward, execution profile,
  accounting numeraire, target-node universe, pair policy, and report schema
- docs clearly explain how replay consumes component waves and policy artifacts
- no replay profile is treated as a trainable component spawn

### 4. Component Profile Vocabulary Docs Cleanup V1

Milestone:

```text
component_profile_vocabulary_docs_cleanup.v1
```

Goal:

```text
Align root, Config, Runtime, Marshal, and Lattice docs around the terms
component_wave, policy_training_profile, environment_replay_profile, and
operator_surface.
```

Acceptance:

- no doc implies policy training is an ordinary component wave
- no doc implies environment replay is a trainable component
- policy and older component identity terms are cross-referenced
- spot_distributional_utility is documented as deterministic baseline, not the
  current trainable policy lane

## Stop Condition For This Audit

This audit is complete when:

- this root report exists
- it recommends either full unification, partial unification, or no unification
- it identifies the current split points
- it defines bounded follow-up goals
- the root roadmap points to the next recommended cleanup

The recommendation is partial unification.
