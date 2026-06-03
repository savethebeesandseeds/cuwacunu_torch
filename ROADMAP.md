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

Status: in progress.

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

Current implementation emits `lattice.source_analytics.fact` sidecars from
Runtime terminal jobs with source-range health, anchor acceptance, missingness,
duplicate-anchor, source cursor, graph order, split/window, and parent exposure
identity. When job-local `data_analytics.v2.latest.lls` and symbolic analytics
reports already exist, Runtime folds their entropy, density, compression, and
spectrum metrics into the same visibility-only fact. Lattice scans these rows
as warning/summary-only catalog evidence and keeps row-index coverage/leakage
authority unchanged. Source-analytics summaries also expose compatible
train-validation metric, volatility, feature-variance, and anchor-support deltas
as warning-only source-regime visibility.

### Forecast Quality Evidence Into Lattice

Status: in progress.

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
  Current Lattice summaries expose baseline-kind coverage as warning-only
  evidence so missing reference families are visible without becoming quality
  gates.
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

Current implementation writes target-transform, forecast-baseline, and
forecast-eval facts for non-mutating channel-MDN run jobs. Runtime now emits
the deterministic baseline family set as separate catalog facts for
previous-value, zero-return, moving-average, and last-valid-channel references.
Forecast-eval facts carry aggregate NLL plus the MDN report's stratified NLL
and valid-target support surfaces by channel, target feature,
channel-target-feature, and horizon. Forecast summaries also expose
calibration-coverage, PIT-summary, and sigma-scale availability diagnostics as
warning-only visibility.
Forecast summaries also derive skill-versus-baseline diagnostics when the
referenced baseline fact resolves under the same identity. These fields are
catalog evidence for inspection and warnings only; they do not make Lattice a
quality gate, checkpoint selector, or deployment authority.

### Deterministic Observer And Allocation Engine Evidence Into Lattice

Status: in progress.

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
- Current implementation tracks these as `observer_belief` and
  `allocation_engine` fact families. The first active slice keeps confidence,
  data-quality, liquidity, observer diagnostic completeness, cap diagnostics,
  scenario-growth floor, fallback, and de-risk signals warning-only.
- Update the Lattice roadmap, DSL/man docs, target vocabulary, contract tests,
  and Runtime evidence writers when this becomes active implementation work.

### Kikijyeba Replay Environment

Status: in progress.

Build `kikijyeba.environment.replay.v1` as the validation-first operating world
for portfolio belief/action loops. The environment should use the same
`reset(EpisodeSpec) -> Observation` and `step(Action) -> Transition` contract
for historical replay, paper mode, and later live mode. Its action surface is
target risky node weights plus a graph-node base reserve weight, so the current
deterministic Wikimyei allocator and future reinforcement-learning policies can
share execution simulation, ledger accounting, rewards, and reports.

The current environment code includes the contract scaffold, a preloaded
`replay_world_t` for deterministic fixture/historical frames, and a
`kikijyeba.environment.replay.dsl` contract that fixes reset/step, cursor,
time-law, action, reward, artifact, and audit-only authority semantics before
the implementation grows wider. It also includes a `replay_source` helper that
converts accepted Ujcamei graph-anchor cursors plus
future edge batches into spawnable replay episode bundles. A bundle is the
current unit of parallelization: one accepted episode range can spawn one
independent replay-world instance, with sequential steps inside that instance.
Baseline policy adapters, a deterministic Wikimyei allocation policy adapter,
and a bounded-parallel replay experiment runner now allow the same bundles to be
compared across multiple policy factories, with per-policy summaries in the
experiment report. A replay bundle source interface is now the streaming
boundary between bundle construction and experiment execution, and the
graph-anchor implementation can materialize bundles from resolved Ujcamei cursor
evidence and edge batches. A replay observation artifact source/enricher now
attaches time-`t` MDN, NodeLift potential, `AllocationBelief`, market-state,
projected-return, and residual-quality artifacts to accepted frames while
rejecting anchor, graph, node-order, base-policy, and timestamp mismatches. It
can read a replay observation artifact index, load persisted Wikimyei forecast
artifacts, reconstruct replay-ready `AllocationBelief`, and expose projected
log-return scenarios for validation. A
Runtime graph-anchor replay bundle source now consumes Runtime `job_manifest_t`
and `wave_plan_t` evidence plus resolved Ujcamei cursor/batch/artifact records,
then validates job id, protocol id, graph fingerprint, graph nodes/edges,
requested and resolved anchor ranges, accepted anchor count, first/last keys,
and cursor identity before exposing bundles. It can now read `job.manifest` from a Runtime
job directory and reconstruct the replay wave evidence, save/load persisted
graph-anchor edge-batch artifacts, write/read a job-local replay artifact path
index, persist accepted replay batches plus forecast artifacts into the
job-local tree, and build a runtime replay record from a Runtime job directory.
Representation stream batches now carry future realization keys, and the replay
Runtime adapter can derive the persisted replay edge-batch artifact from that
actual stream output. It can also convert observer
`allocation_belief_build_result_t` outputs into persisted replay forecast
records, and it can build those forecast records directly from an MDN `MdnOut`
plus `channel_mdn_input_batch_t` using explicit observer builder options. The
graph-first MDN inference launcher now exposes an optional run-mode batch
observer carrying `MdnOut`, `channel_mdn_input_batch_t`, and the representation
batch with future realization keys, so Runtime can install the replay artifact
writer without changing the inference loop. Runtime/job execution now installs
that observer for non-dry-run MDN `MODE=run` jobs, derives observer options from
the graph and belief DSL, supports explicit base-reserve and risky-node
overrides, and writes one pulse-local replay artifact packet plus a job-local
batch index for each streamed MDN pulse. The environment now has a direct
Runtime job-directory replay source that reads those indexed pulse packets,
materializes replay bundles, and runs complete reserve-baseline and
deterministic Wikimyei allocation episodes in focused Runtime tests. A
code-level Runtime job replay experiment driver now loads the protocol graph
from the config recorded in `job.manifest`, consumes the job-local replay
artifact index, runs enabled replay policies, and writes a Cajtucu-ready
experiment report plus a job-local replay experiment index under the replay
artifact tree. The report now carries common runtime/environment ids, explicit
audit-only authority flags, policy summaries, episode summaries, requested and
accepted cursor/range evidence, and compact per-step evidence for timing,
target weights, realized growth, reward components, projection metrics, and
warnings/failures. Source-side replay now rejects frames whose direct
asset/base projections resolve to mixed future realization keys, and experiment
reports now persist explicit task, bundle, and policy indices for every
bundle/policy episode product. `cuwacunu_exec` now
exposes that driver through
`--replay-from-job-dir`, keeping replay as a post-job adapter rather than
replacing normal Runtime job launch. Runtime Hero now exposes the same bounded
post-job path as `hero.runtime.replay_from_job`: it accepts `job_id` or
`job_dir`, verifies the job is completed and has replay batch evidence, and then
delegates to the replay CLI surface. Runtime Hero read paths can also inspect
`replay_batch_index`, `replay_experiment_index`, and
`replay_experiment_report` by name, and job summaries include those replay
artifact summaries beside the standard Runtime manifest/state/report. Marshal's
read-only `subject=run` operational report now also surfaces replay evidence
from those same job-local indexes in `chain_summary`, `runtime_panel`, and
`current_state` without executing or claiming proof authority. Lattice has a
parked `replay_environment` fact-family design for those job-local replay
indexes/reports, including per-episode requested-range, accepted-cursor,
accepted-anchor-interval, accepted-anchor-key evidence counts, explicit
historical world-mode identity, V1 runtime batch-index/experiment-index/report
schema identity, runtime experiment-index report digests matched against the
referenced report body, observation/action/execution time-law step evidence,
expected completed-step counts, sequential source-order evidence, action
decision timestamps bound to the observation-to-realization window,
future-observation violation counts, realization-key violation counts, bounded
requested/resolved parallelism, and projection-validation step evidence.
Runtime replay experiment reports now emit those counters from their compact
per-step evidence. `replay_environment` remains parked evidence: it is
catalog-listed, not derived by default, explicit-audit-scan only, and not an
active target instance. A future non-dispatchable
`replay_environment_artifact_ready` proof shape is reserved for proving schema
identity, report digest identity, source order, declared/observed parallelism,
time-law cleanliness, lineage binding, and required projection-counter presence
once Runtime replay artifacts and fact contracts are stable. That future proof
must remain audit evidence, not execution, allocation, policy selection, reward
acceptance, or market-readiness authority. The next accepted replay milestone
should continue hardening this end-to-end path against real Runtime job
directories while preserving the time law: observations contain only time-`t`
information while time-`t+1` realizations are revealed only after action and
execution simulation.
#### Environment, Lattice, And Marshal Design Checkpoint

Status: discussion required before implementation.

The next environment-era challenge is not only adding replay facts. The hard
part is deciding how future trainable policies use the environment without
collapsing the recovered Lattice/Marshal boundary.

Keep four surfaces separate:

```text
environment evidence
  Replay reports, episode records, time-law checks, and
  replay_environment facts. These are audit/proof inputs, not work orders.

environment execution
  Runtime-owned workers, episode bundles, parallel replay, simulation,
  policy-gradient or other training loops, checkpoint writing, and reports.

policy-training readiness
  A future dispatchable target only if backed by an explicit Runtime job kind,
  component family, report schema, artifact/checkpoint contract, finite
  episode budget, finite parallelism budget, and lineage contract.

policy acceptance
  Reward thresholds, baseline comparisons, uncertainty, selector policy,
  deployment, and market readiness. This remains a reserved policy-gate
  discussion, not an active target.
```

`replay_environment` facts and the reserved future
`replay_environment_artifact_ready` proof shape should prove replay/artifact
lineage, schema identity, report digest identity, source order, parallelism
bounds, time-law cleanliness, and projection-counter presence. They must not
train a policy, select a policy, judge reward quality, or imply
execution/allocation/market readiness.

Lifecycle state:

```text
replay_environment fact family
  known / parked / catalog-listed

replay_environment fact derivation
  disabled by default
  explicit audit scan only

replay_environment_artifact_ready proof template
  designed / reserved / not active yet

replay_environment_artifact_ready target instance
  absent until Runtime replay artifacts and fact contracts are stable

policy_training_artifact_ready
  future dispatchable target only with a concrete Runtime job contract

policy_acceptance
  disabled policy reservation only
```

Marshal may eventually need an environment-era surface, but the first candidate
is read-only inspection: a panel over Runtime replay artifacts and Lattice
`replay_environment` facts. A second, separate future surface may coordinate a
bounded Runtime handoff for a dispatchable policy-training target, but only
after the target contract is explicit. Marshal must never become an open-ended
environment scheduler, policy optimizer, reward judge, checkpoint selector,
allocation engine, or deployment authority.

The design is not ready for code until these are specified:

```text
Runtime job kind and producer component family
environment/replay contract version
episode bundle identity and source ranges
max environment jobs, workers, episodes, and attempts
resume ledger identity and terminal stop condition
policy artifact/checkpoint output contract
parent forecast, observer, allocation, and environment evidence digests
reward definition digest and accounting assumptions
baseline policy and comparison semantics
selector split, anti-leakage policy, and replay/live separation
support minimums, uncertainty policy, and negative tests
checkpoint-source policy for policy artifacts
post-run Lattice target to recheck
```

Prompt for a future refinement session:

```text
Refine the environment-era Lattice/Marshal design without implementing it.
Separate replay_environment artifact proof, policy-training readiness, and
policy acceptance. Decide which future targets are dispatchable, which are
inspection-only, which remain disabled policy gates, and what bounded Marshal
handoff contract is required before Runtime may parallelize environment jobs.
```

Initial scope:

- Run episodes over graph-anchor source ranges, parallel across episodes and
  sequential within each episode.
- Keep direct asset/reference edge orientation explicit when deriving realized
  asset/base log returns from historical future close coordinates; routed
  projections remain out of V1.
- Carry `runtime_run_id`, `environment_run_id`, requested range, resolved cursor
  identity, batch cursor token, accepted anchor interval, and accepted anchor
  keys for every episode.
- Validate `AllocationBelief` projection semantics by comparing
  `phi_asset - phi_reference` against realized executable asset/base log
  returns.
- Emit per-step observation/action/execution/ledger/reward evidence and
  per-episode summaries with projection MAE, bias, directional accuracy,
  interval coverage, log growth, drawdown, turnover, costs, and warnings.
- Compare deterministic policies against simple baselines under the same
  accounting and execution assumptions.
- Emit Cajtucu-ready step, episode, experiment, and policy-comparison artifact
  schemas without creating the `cajtucu` pillar before its evaluation boundary
  is defined.

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
