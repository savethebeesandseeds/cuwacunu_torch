# Lattice Validation Eval Wrong MDN Fixture

This fixture preserves the V1 failure case where validation evaluation loads a
different MDN checkpoint from the checkpoint selected by the no-leakage guard.
It is intentionally small, read-only, and uses explicit legacy node-MDN target
names so it cannot be confused with the active channel-MDN target DSL.

Inspect it through Lattice Hero with:

```sh
.build/hero/hero_lattice.mcp \
  --global-config /cuwacunu/src/config/.config \
  --tool hero.lattice.evaluate_target \
  --args-json '{"config_path":"/cuwacunu/src/tests/fixtures/kikijyeba/lattice_validation_eval_wrong_mdn/.config","runtime_root":"/cuwacunu/src/tests/fixtures/kikijyeba/lattice_validation_eval_wrong_mdn/runtime_root","target_id":"fixture_legacy_node_mdn_validation_eval_ready"}'
```

Expected result: `status=exposure_failed` with a
`dependency:mdn_checkpoint` mismatch deficit. `proof_certificate_check` should
also fail with a checkpoint-source mismatch issue because the emitted dependency
proof is intentionally inconsistent: the runtime-loaded MDN checkpoint is not
the selected trained checkpoint.
