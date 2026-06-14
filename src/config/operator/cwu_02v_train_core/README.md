# cwu_02v Train-Core Operator Policies

These files are durable long-train `.jkimyei` policies for proof-clean
train-core recovery after a runtime dev reset. The active project config is
only `src/config/.config`; do not add local `.config` copies here.

Run order:

1. Representation train-core wave
   - active wave id: `train_core_mtf_jepa_mae_vicreg`
   - training policy: `mtf_jepa_mae_vicreg.train_core_3000.jkimyei`
   - training id: `real_mtf_jepa_mae_vicreg_train_core_v1_operator_3000_clean_parent`
   - full contract also references `mdn.train_core_3500.jkimyei`, so the
     representation and MDN runs share one active contract fingerprint
   - execution must go through the current Marshal/Runtime target-dispatch path
   - target: `cwu_02v_representation_train_core_ready`

2. MDN train-core wave
   - active wave id: `train_core_channel_mdn`
   - training policy: `mdn.train_core_3500.jkimyei`
   - full contract also references `mtf_jepa_mae_vicreg.train_core_3000.jkimyei`
   - keep `INPUT_REPRESENTATION_CHECKPOINT` empty in the static profile; after
     representation is proven, Marshal resolves
     `latest_satisfying:cwu_02v_representation_train_core_ready` into a
     `runtime_handoff`, and Runtime passes the concrete checkpoint path as a
     launch-time override before execution
   - target: `cwu_02v_mdn_train_core_no_test_leakage`

Do not use temporary config copies for these runs. The spawn identity should be
recoverable from `src/config/.config`, these repo-local policy files, and the
Runtime/Lattice evidence that Marshal records.

Representation and MDN must be trained under the same full config contract.
Changing only the MDN training profile after representation training changes the
active protocol contract fingerprint and makes the representation dependency
stale for MDN. If these files change, rerun representation first.

Both waves use the shared `src/config/ujcamei.source.cursor.dsl` cursor catalog.
The durable wave profiles select `SOURCE_CURSOR_ID = train_core.all`;
Marshal/Runtime overlays provide the concrete train-core range at launch.

Current preflight entrypoints:

```sh
.build/hero/hero_marshal.mcp \
  --tool hero.marshal.prepare.train \
  --args-json '{"mode":"plan","profile":"single_wave_operator","target_id":"cwu_02v_representation_train_core_ready"}'

.build/hero/hero_marshal.mcp \
  --tool hero.marshal.prepare.train \
  --args-json '{"mode":"dry_run","profile":"single_wave_operator","target_id":"cwu_02v_representation_train_core_ready"}'
```

Before representation training, `src/config/.config` should select
`runtime_wave_id = train_core_mtf_jepa_mae_vicreg`. After representation
evidence is completed and proven, the MDN phase selects
`runtime_wave_id = train_core_channel_mdn` in the same file.

The dry-run preflight is expected to reach `ready_for_execution_gate` without
starting non-dry-run training. The MDN target should be checked after
representation evidence is completed and proven; before that, Marshal will
correctly point the MDN target back to the representation checkpoint-source
dependency.

The canonical config uses only the canonical
`wikimyei_policy_portfolio_spot_distributional_utility_*` keys. Portfolio
policy evidence remains protocol/config identity; it does not make portfolio
policy output a train-core target or Lattice authority.

The previous direct 5000-step run is exploratory only because it used an
all-source wave and overlaps validation/test holdouts. These profiles let
Marshal apply the Lattice-advised train-core range as a Runtime wave overlay.

## Config Audit

The duplicate operator `.config` files were removed. Keep this directory for
durable train-core policy files only; the active wave and global path map belong
to `src/config/.config`. If a future handoff surface can bind target, wave,
source range, checkpoint source, and training policy without editing the
canonical config, document that surface here instead of reintroducing config
copies.
