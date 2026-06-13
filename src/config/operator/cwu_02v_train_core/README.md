# cwu_02v Train-Core Operator Profiles

These files are durable operator profiles for proof-clean train-core recovery
after a runtime dev reset. They are intentionally separate from the canonical
250-step defaults so longer training does not silently become the project-wide
baseline.

Run order:

1. `representation.config`
   - active wave id: `train_core_mtf_jepa_mae_vicreg`
   - training policy: `mtf_jepa_mae_vicreg.train_core_3000.jkimyei`
   - training id: `real_mtf_jepa_mae_vicreg_train_core_v1_operator_3000_clean_parent`
   - full contract also references `mdn.train_core_3500.jkimyei`, so the
     representation and MDN runs share one active contract fingerprint
   - execution must go through the current Marshal/Runtime target-dispatch path
   - target: `cwu_02v_representation_train_core_ready`

2. `mdn.config`
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
recoverable from these repo-local profile files and the Runtime/Lattice evidence
that Marshal records.

Representation and MDN must be trained under the same full config contract.
Changing only the MDN training profile after representation training changes the
active protocol contract fingerprint and makes the representation dependency
stale for MDN. If these files change, rerun representation first.

Both profiles use the shared `src/config/ujcamei.source.cursor.dsl` cursor catalog.
The durable wave profiles select `SOURCE_CURSOR_ID = train_core.all`;
Marshal/Runtime overlays provide the concrete train-core range at launch.

The profile configs use only the canonical
`wikimyei_policy_portfolio_spot_distributional_utility_*` keys. Portfolio
policy evidence remains protocol/config identity; it does not make portfolio
policy output a train-core target or Lattice authority.

The previous direct 5000-step run is exploratory only because it used an
all-source wave and overlaps validation/test holdouts. These profiles let
Marshal apply the Lattice-advised train-core range as a Runtime wave overlay.
