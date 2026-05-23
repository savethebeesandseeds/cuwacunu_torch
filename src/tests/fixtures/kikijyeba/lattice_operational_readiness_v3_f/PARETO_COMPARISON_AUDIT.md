# Lattice Operational Readiness V3-F Pareto Comparison Audit

Generated: 2026-05-23

## Scope

V3-F adds a read-only Pareto evidence comparison surface. It compares two
evaluated target evidence vectors without collapsing the lattice into a scalar
score, deployment decision, or `latest_satisfying` replacement.

## Implemented Surface

```text
hero.lattice.compare_evidence
```

The tool evaluates both targets from one runtime scan and reports:

```text
- left and right target status
- checkpoint path, checkpoint_id, and checkpoint_file_digest
- proof certificate digest and certificate self-check
- clean-checkpoint participation/exclusion reasons
- evidence_order_vector for both sides
- relation_vocabulary and dimension_vocabulary
- per-dimension comparison rows
- final relation and dominance_reason
```

The relation vocabulary is:

```text
unavailable
selector_leaked
equivalent
left_dominates
right_dominates
incomparable
```

## Boundary Checks

```text
read_only = true
runtime_executor = false
writes_evidence = false
db_source_of_truth = false
cache_rows_used_for_target_satisfaction = false
deployment_decision = false
scalar_score_emitted = false
latest_satisfying_policy_separate = true
```

Selection-signal leakage is checked before a checkpoint can participate. A
selector-leaked side is excluded and the relation is reported as
`selector_leaked`.

## Direct Runtime Smoke

Runtime root:

```text
/cuwacunu/.runtime/cuwacunu_exec
```

Active identity observed by direct Lattice Hero:

```text
protocol_contract_fingerprint = 8aaff747e54c7c4f
graph_order_fingerprint       = 507df53170ffab6a
vicreg_assembly_fingerprint   = 759931dbbd341fcf
mdn_assembly_fingerprint      = a4d5971c5c4cbb01
```

Same-target clean guard comparison:

```text
left_target_id  = channel_mdn_train_core_no_validation_leakage
right_target_id = channel_mdn_train_core_no_validation_leakage
relation        = equivalent
left.participates = true
right.participates = true
scalar_score_emitted = false
deployment_decision = false
```

Train-core readiness versus no-validation-leakage guard:

```text
left_target_id  = channel_mdn_train_core_ready
right_target_id = channel_mdn_train_core_no_validation_leakage
relation        = incomparable
left_better_dimensions  = min_unique_coverage_fraction, max_cursor_exposure_load
right_better_dimensions = triggered_warning_count, unavailable_warning_count
```

This proves the comparison preserves Pareto tradeoffs instead of ranking by a
hidden scalar.

## Regression Coverage

Focused target regression:

```text
make -C src/tests/bench/kikijyeba/lattice_target run-test_kikijyeba_lattice_target -j12
```

Covered cases:

```text
- relation vocabulary includes selector_leaked
- dimension vocabulary includes selector leakage, support imbalance, and
  unresolved lineage dimensions
- equivalent vectors
- unavailable vectors
- left dominates
- right dominates
- incomparable tradeoff
- selector-leaked exclusion
- lower node-support imbalance is stronger
- unresolved lineage is weaker
```

Hero/schema regression:

```text
make -C src/tests/bench/kikijyeba/runtime run-test_hero_mcp_schema_compat -j12
```

Build smoke:

```text
make -C src/main/hero build-lattice-hero -j12
```

## Status

```text
V3-F complete.
```
