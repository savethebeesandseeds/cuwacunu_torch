# Lattice Operational Readiness V3-E Derived Query Audit

Generated on `2026-05-23` against `/cuwacunu/.runtime/cuwacunu_exec`.

V3-E adds `hero.lattice.derived_query` as a read-only derived-rule pilot. The
tool answers a finite rule set with concrete witnesses and never executes waves
or uses cache rows for target satisfaction.

## Verified Commands

```text
hero.lattice.derived_query
  relation = target_satisfied
  target_id = channel_mdn_validation_eval_ready

Observed:
  canonical_relation = target_satisfied
  value = true
  witness_type = target_evaluation
  cache_rows_used_for_target_satisfaction = false
```

```text
hero.lattice.derived_query
  relation = forbidden_overlap
  target_id = channel_mdn_train_core_no_validation_leakage

Observed:
  canonical_relation = forbidden_overlap
  known = true
  value = false
  protected_split = validation_holdout
  witness_count = 0
```

```text
hero.lattice.derived_query
  relation = checkpoint_ancestor
  checkpoint_path =
    /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_channel_train_core_mdn_0000_1600.train.channel_inference_mdn/channel_inference.report.channel_mdn.pt

Observed:
  canonical_relation = checkpoint_ancestor
  closure_complete = true
  value = true
  witness_type = checkpoint_ancestor
```

```text
hero.lattice.derived_query
  relation = checkpoint_ancestor
  checkpoint_path =
    /cuwacunu/.runtime/cuwacunu_exec/cwu_01v_channel_train_core_mdn_0000_1600.train.channel_inference_mdn/channel_inference.report.channel_mdn.pt
  ancestor_checkpoint_path = missing_v3e_ancestor.pt

Observed:
  canonical_relation = checkpoint_ancestor
  closure_complete = true
  value = false
  missing_witness_rejected = true
  fail_closed = true
```

```text
hero.lattice.derived_query
  relation = stale_cache
  index_path = .lattice_index/missing_v3e_cache.lls
  validation_strength = header_only

Observed:
  canonical_relation = stale_cache
  value = true
  fail_closed = true
  issues = [missing_cache, schema_mismatch]
  cache_rows_used_for_target_satisfaction = false
```

```text
hero.lattice.evaluate_target
  target_id = channel_mdn_validation_eval_ready

Observed in derived_query_results:
  result_count = 18
  rule_vocabulary_relation_count = 18
  result_covers_rule_vocabulary = true
  summary_self_check_passed = true
```

```text
hero.lattice --list-tools-json

Observed:
  hero.lattice.derived_query inputSchema has top-level type object
  required = [relation]
  no top-level oneOf/anyOf/allOf/not/enum was detected in Lattice tool schemas
```

## Boundary

The derived-query pilot is not a target evaluator replacement. Target
satisfaction remains computed from runtime evidence and proof certificates.
Cache/index rows remain rebuildable read models and cannot upgrade readiness.
