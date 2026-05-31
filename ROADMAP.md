# Active Roadmap

This is the short operator-facing index for current work. Keep it finite. Do
not use this file as a history log for completed Lattice, Runtime, Marshal,
source-range, dataloader, legacy-cleanup, or MDN-shape work.

Detailed subsystem records live in:

```text
src/include/kikijyeba/lattice/ROADMAP.md
src/include/kikijyeba/marshal/ROADMAP.md
src/include/wikimyei/inference/expected_value/mdn/README.md
.deprecated/src_legacy/MIGRATION_INVENTORY.md
```

## North Star

```text
Runtime executes and writes durable evidence.
Lattice proves target satisfaction from Runtime evidence.
Marshal prepares bounded handoffs and explains state.
```

The system should keep these boundaries:

```text
Runtime is not proof authority.
Lattice is not an executor or scheduler.
Marshal is not proof authority, a config editor, a checkpoint selector, or an
unbounded scheduler.
```

## Active Item

### Source Facts And Analytics Into Lattice

Status: queued.

Add source-side evidence to Lattice without changing the current authority
boundaries. Existing source receipts and source-key windows remain audit
metadata; row-index intervals stay authoritative for coverage and leakage.

Initial scope:

- Carry source analytics such as source entropic load, entropy rate,
  information density, compression ratio, power-spectrum entropy, sample
  validity, and related source-health measurements into Lattice evidence.
- Bind those facts to source receipts, source cursor identity, graph order,
  split/window, and parent exposure fact digests.
- Start as read-only summaries and `LATTICE_WARN` diagnostics, not hard
  readiness gates, performance gates, contract identity, or executor authority.
- Update the Lattice roadmap, DSL/man docs, and tests when implementation
  begins.

### Forecast Quality Evidence Into Lattice

Status: queued.

Add a compact forecast-evaluation evidence contract so Lattice can answer
whether an end-to-end protocol is forecasting well enough to inspect, without
turning Lattice into a model runner, checkpoint selector, or performance gate.

Initial scope:

- Emit protocol-bound validation facts for MDN forecasting runs, including the
  evaluated protocol id, representation checkpoint, MDN checkpoint, split,
  source cursor identity, graph order, and parent exposure/checkpoint digests.
- Add protocol-specific validation targets such as
  `cwu_02v_mdn_validation_eval_ready` that depend on the matching
  protocol-specific representation and MDN train-core readiness targets.
- Carry forecast-quality visibility fields such as mean NLL, expected-value
  MAE/RMSE, signed error, directional accuracy, naive-baseline error,
  skill-versus-naive, valid target support, and calibration coverage summaries.
- Bind every forecast metric to target-transform facts: target feature ids,
  horizon, raw/return/log-return mode, normalization and inverse transform,
  units, and target mask policy. A forecast error without this contract is not
  interpretable evidence.
- Emit baseline facts next to model facts, starting with previous-value,
  zero-return, moving-average, and last-valid-channel baselines. Lattice should
  see skill versus baseline, not only raw error numbers.
- Add selection-signal facts before any checkpoint is chosen by validation
  performance: candidate checkpoints, chosen checkpoint, selector split,
  selector metric, tie policy, and parent evaluation facts. These remain
  read-only evidence and are required to audit validation leakage.
- Track evaluation support by node, channel, target feature, and horizon,
  including weakest-support rows, so aggregate quality cannot hide unsupported
  forecast surfaces.
- Add MDN calibration visibility: interval coverage, PIT summaries,
  sigma/scale sanity over valid targets, and overconfidence or underconfidence
  warnings.
- Carry source/regime-shift visibility between train and validation windows:
  missingness drift, volatility drift, feature variance drift, support drift,
  and source entropy/complexity drift.
- Keep these as read-only facts and `LATTICE_WARN` diagnostics first. Do not
  promote them to hard readiness or deployment gates until uncertainty,
  baselines, split leakage policy, and threshold policy are explicit.
- Preserve the existing Lattice boundary: Runtime produces forecast evidence;
  Lattice scans, explains, evaluates readiness, and reports warnings. Lattice
  does not execute runs, train models, choose best checkpoints, or claim market
  readiness from a single point estimate.

### Deterministic Observer And Allocation Engine Evidence Into Lattice

Status: queued.

Extend Lattice so deterministic post-inference Wikimyei components can be
audited as first-class evidence surfaces, without making Lattice responsible
for portfolio execution or capital allocation.

Initial scope:

- Add read-only Lattice visibility for deterministic Wikimyei assemblies after
  MDN inference, starting with `wikimyei.observer.belief` and
  `wikimyei.engine.portfolio.spot_distributional_utility`.
- Bind observer/engine evidence to the same protocol id, graph order,
  source cursor identity, MDN assembly fingerprint, MDN forecast artifact,
  feature-semantics fingerprint, and dock-binding fingerprint used by the
  graph-first contract.
- Require Lattice evidence to distinguish raw NodeLift potential belief from
  post-projection `AllocationBelief`; raw NodeLift potentials must not be
  treated as tradable return evidence.
- Carry explicit base/reserve semantics: the reserve asset must be a graph
  node supplied by `BasePolicy`, not an external cash bucket or unbound scalar.
- Track observer outputs such as channel consensus, potential-surface
  diagnostics, NodeLift return projection, covariance coupling, scenario bank
  identity, NodeLift residual quality, projection-validation scores,
  confidence, data quality, liquidity, and forecast-artifact lineage.
- Track allocation-engine outputs such as target risky node weights, reserve
  node id, reserve weight, turnover, scenario-growth floor status, objective
  terms, CVaR loss, transaction-cost estimate, constraint/cap diagnostics, and
  fallback/de-risk reasons.
- Start with `LATTICE_WARN` diagnostics and artifact-readiness targets only.
  Do not make Lattice judge market readiness, select allocations, route
  execution, or override the portfolio engine.
- Update the Lattice roadmap, DSL/man docs, target vocabulary, contract tests,
  and Runtime evidence writers when this becomes active implementation work.

## Not Active Here

Do not redispatch these from this root roadmap:

```text
multi-target scheduling
dependency traversal
automatic config editing
checkpoint ranking by validation metric
best-model selection
Marshal-owned performance gates
Codex-in-the-loop public evaluation
unbounded objective agents
completed cleanup/testing receipts
```

Open a new finite goal and update the relevant subsystem roadmap when one of
those items becomes current work.
