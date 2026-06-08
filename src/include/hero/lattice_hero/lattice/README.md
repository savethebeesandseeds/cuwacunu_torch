# Hero Lattice

The Hero Lattice language records latent lineage state
    : typed values,
      domains,
      runtime report facts,
      and the cursor context that says which wave and source range produced
              those facts
                  .

          Namespace : `cuwacunu::hero::lattice`.

    Core parser aliases,
      left - hand - side helpers,
      and the lattice decoder live directly in this namespace.The file
              extension or
          grammar flavor is not a C++ namespace
                  .

              Runtime component `
                  .lls` reports live under
`cuwacunu::hero::lattice::runtime_report`.

              Normal runtime reports and
              debug `.lls` reports are different evidence grades.The current
                  target evaluator can work from normal job manifests,
      job state,
      component reports,
      and checkpoints; it does not require every wave to run with
`debug`. Debug `.lls` facts should become the richer lineage layer for cursor
coverage, exposure accounting, and leakage checks.

The exposure ledger lives under `cuwacunu::hero::lattice::exposure`. It is
the normalized bridge between job-shaped runtime artifacts and cursor-indexed
lattice reasoning. Runtime now emits `lattice.exposure.fact` sidecars after
successful terminal jobs and records that sidecar path in `job.state`. The
scanner prefers those sidecars and falls back to deriving an exposure fact from
`job.manifest`, `job.state`, `representation.report`, and `inference.report`
when a sidecar is absent. Facts are a subset of the broader runtime evidence
bundle; checkpoints, reports, raw source data, and config files remain evidence
even when they are not `.lls` facts. Exposure facts record
contract/component/config identity, Ujcamei graph-anchor cursor range,
observed/target/evaluation/selection use flags, optimizer effort,
valid-target evidence, checkpoint inputs, and checkpoint outputs. Each fact
also has an explicit `fact_type`; v0 primarily emits `exposure`, but the
lattice should eventually hold other fact types such as metrics and source
split declarations. Current runtime manifests emit
`graph_anchor_row_index_v1` footprints derived from the resolved graph-anchor
range and the Hx/Hf source windows:
observed input uses `[anchor_begin - (Hx - 1), anchor_end)`, while future
supervision uses `[anchor_begin + 1, anchor_end + Hf)`. Older artifacts without
these fields still fall back to `anchor_range_v0`. Exposure facts also preserve
`source_input_length` and `source_future_length` when runtime provides them.
Split protection uses those values, or the observed/target footprints as a
fallback, to expand protected holdout intervals by the relevant context/future
windows before forbidden-overlap checks. Runtime manifests also emit
`graph_anchor_key_window_v1` source-key windows when the graph cursor has a
regular reference key step. Those key windows are audit metadata that map row
footprints back to source keys/timestamps; v0 overlap math still uses row-index
intervals so split checks remain deterministic. Lattice scans warn when a
source-key window is incomplete, non-numeric, internally non-monotone, when its
numeric key endpoints invert the row-endpoint ordering, or when a regular
anchor-key step can be inferred but the observed/target windows do not match the
affine row-to-key map. It also counts missing endpoint pairs, irregular anchor
steps, and row/source-key mismatches as explicit gap warnings. That treats the
source-key map as an audit-only order-preserving coordinate system; row-index
leakage math remains authoritative.
`hero.lattice.inspect subject=exposure` is a read-only, non-dispatchable evidence scan; it
is not a target proof or Runtime executor, and its fact families are not
`TARGET_KIND` values. It also exposes a structured
`source_key_window_audit` object with the parsed endpoints,
completeness/numeric/monotonicity/order-preservation/affine-consistency booleans,
the inferred reference key step when available, gap/mismatch counters, and issue
ids so agents do not have to parse warning text to inspect the auxiliary
coordinate map. `inspect subject=exposure` also returns
`source_key_map_audit_summary`, which binds available audits to graph-order
identity, source-cursor identity, and source-receipt parent facts while keeping
the summary audit-only.
Evaluation and planning JSON also emit
`source_key_coordinate_policy_vocabulary`, which makes the coordinate policy
explicit: row-index intervals are the coverage/leakage authority, while
source-key windows are audit-only order-preserving map checks.
`source_key_coordinate_policy_summary` self-checks that boundary with one
row-index coverage/leakage authority row, four audit-only source-key rows,
declared order-preserving/affine/gap fields, and no source-key audit row with
coverage or leakage authority.
The manifest also carries active source-file receipts
(`edge|instrument|interval|record_type|source`) so a lattice fact can be traced
back to the concrete Ujcamei files that supplied the wave. Evaluation and
planning JSON emit `source_receipt_policy_vocabulary`, which keeps those compact
receipts as audit metadata: they do not satisfy coverage, prove leakage
cleanliness, alter contract identity, or replace row-index footprints. Structured
source receipt facts are a V2 scan surface only: they normalize the compact
receipts for audit/provenance and remain outside V1 readiness authority.
`source_receipt_policy_summary` self-checks that boundary with one row-index
coverage/leakage authority row, three audit-only receipt rows, no contract
identity authority, one available structured source-receipt fact surface, and
declared compact plus structured receipt fields.

## Fact Emission Contract And Inventory

This README is the source of truth for the current fact inventory and
fact-emission contract. Earlier standalone inventory/contract files were review
artifacts; their durable content is consolidated here.

Use `fact` only for a durable normalized evidence row with schema, identity,
source range, and authority semantics. Do not call every report field a fact.

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

Runtime/components write what happened. The lattice scans, normalizes,
evaluates, certifies, and explains those artifacts; it does not execute waves or
invent missing evidence. Runtime model-state paths, loaded checkpoint paths,
output checkpoint paths, and admission flags such as
`ALLOW_UNTRAINED_REPRESENTATION` are evidence, not protocol contract identity.
They become proof-relevant only when reports, facts, checkpoint closure, and
target rules bind them to the active identity.

Promoted fact rows bind, where applicable, to:

```text
contract_fingerprint
graph_order_fingerprint
source_cursor_token
split_policy_fingerprint
component_assembly_fingerprint
target_component_family_id
job_id
wave_id
job_status
wave_action
anchor/completed row-index intervals
checkpoint_id and checkpoint_file_digest for checkpoint facts
parent_exposure_fact_digest for derived fact families
```

Current fact families:

```text
kikijyeba.lattice.exposure.v1
  Runtime sidecar after terminal jobs; scanner fallback may derive it from
  manifest/state/reports. Completed rows can provide coverage, load, lineage,
  and leakage evidence. Dry-run/running/failed rows are audit visibility only.

kikijyeba.lattice.checkpoint.v1
  Runtime sidecar when a checkpoint exists. checkpoint_id plus
  checkpoint_file_digest is closure authority. The sidecar carries the producing
  protocol id and contract fingerprint; missing or stale checkpoint identity
  fails closed.

kikijyeba.lattice.source_receipt.v1
  Scanner-derived audit/provenance rows from exposure.source_file_receipts.
  These rows inherit protocol identity from their parent exposure fact, do not
  replace row-index coverage/leakage math, and do not alter contract identity.

kikijyeba.lattice.source_analytics.v1
  Scanner-derived source health and complexity rows from
  lattice.source_analytics.fact sidecars or job-local source data analytics
  `.lls` reports when no dedicated source-analytics sidecar exists. These rows
  bind entropy, compression, validity, missingness, duplicate-sample, and
  source-health payloads to a parent exposure digest and active graph/source
  identity. Summaries also expose train-validation source-regime metric,
  volatility, feature-variance, and anchor-support deltas for comparable source
  identities as warning-only visibility. They do not satisfy coverage, prove
  leakage cleanliness, alter contract identity, gate readiness, or add a
  TARGET_KIND.

kikijyeba.lattice.target_transform.v1
  Scanner-derived target transform contracts from lattice.target_transform.fact
  sidecars. These rows bind target feature ids, horizon, target mode,
  normalization/inverse-transform contracts, units, mask policy, and support
  surface identity to a parent exposure digest. They are artifact-contract
  prerequisites for future forecast-evaluation facts, not forecast-quality,
  performance, readiness, allocation, or deployment decisions.

kikijyeba.lattice.forecast_baseline.v1
  Scanner-derived deterministic forecast baseline evidence from
  lattice.forecast_baseline*.fact sidecars. Runtime emits previous-value,
  zero-return, moving-average, and last-valid-channel baseline rows for
  non-mutating channel-MDN evaluation runs. These rows bind baseline kind,
  parameters, target transform digest, horizon, support counts, metric status,
  and any available baseline error metrics to the same protocol/split/source
  identity as their parent exposure fact. Runtime marks unscored deterministic
  references as `metric_status=deferred_v1` instead of implying computed
  scores. They are evidence prerequisites and warning support, not
  forecast-quality acceptance, checkpoint selection, or deployment authority.

kikijyeba.lattice.forecast_eval.v1
  Scanner-derived forecast evaluation evidence from lattice.forecast_eval.fact
  sidecars. These rows bind forecast metrics, stratified NLL surfaces, support
  surfaces including horizon-level support, target-transform and baseline
  digests, evaluated
  representation/MDN checkpoint digests, and forecast artifact lineage to a
  parent exposure digest. Forecast summaries derive skill-versus-baseline
  diagnostics only after the baseline digest resolves under the same identity,
  and expose calibration-coverage, PIT-summary, and sigma-scale availability as
  warning-only visibility. They are non-mutating artifact evidence and warning
  support, not quality acceptance, best-checkpoint selection, performance
  readiness, allocation, or deployment authority.

kikijyeba.lattice.observer_belief.v1
  Scanner-derived observer belief evidence from lattice.observer_belief.fact
  sidecars. These rows bind deterministic post-inference diagnostics,
  confidence/data-quality/liquidity summaries, scenario/covariance identity,
  feature semantics, dock binding, and forecast artifact lineage to a parent
  exposure digest. The payload must distinguish raw_nodelift_potential from
  allocation_belief. Raw NodeLift potential is not tradable return evidence,
  and observer belief facts are warning/visibility evidence only: they are not
  allocation, quality, performance, market-readiness, checkpoint-selection,
  coverage, leakage, contract-identity, or readiness authority.

kikijyeba.lattice.allocation_engine.v1
  Scanner-derived allocation-engine audit evidence from
  lattice.allocation_engine.fact sidecars. These rows bind target-node weights,
  accounting numeraire graph node/source/BasePolicy match and numeraire weight,
  turnover, objective terms, CVaR, transaction costs, constraints/caps,
  scenario floor status,
  fallback/de-risk reasons, observer belief digest, forecast artifact digest,
  and BasePolicy digest to a parent exposure digest. They prove output shape
  and lineage for later narrow artifact-readiness claims only. Lattice does not
  allocate, select an allocation, route execution, judge market readiness, or
  override the portfolio engine.

kikijyeba.lattice.replay_environment.v1
  Scanner-derived replay environment audit evidence from job-local
  runtime_replay_batches.index, runtime_replay_experiments.index, and replay
  experiment reports. These rows bind replay bundle counts, policy counts,
  attempted/completed episode counts, reward/log-growth/equity summaries, and
  replay report identity to a parent exposure digest. The report digest
  declared by the Runtime experiment index is recomputed from the report body
  before readiness can pass. Artifact readiness over this family also requires
  the explicit replay contract surface: Runtime historical-replay world mode,
  V1 runtime batch-index/experiment-index schemas plus the Cajtucu-ready replay
  experiment artifact schema, cursor range resolution, no-future-leakage and
  projection-validation guards, graph-node BasePolicy reserve semantics,
  completed attempted episodes, episode cursor/range evidence, sequential
  source-order evidence, explicit requested/resolved parallelism evidence,
  observation/action/execution
  time-law step counters matching the expected completed-step count, action
  decision timestamps after knowledge and before realization availability, zero
  future-observation/realization-key violations, and finite
  projection-validation summaries with matching step evidence.
  They are audit-only experiment evidence: they do not execute replay,
  allocate, route Runtime, prove quality/performance/market readiness, select
  checkpoints, or become coverage, leakage, readiness, deployment, or
  contract-identity authority.

kikijyeba.lattice.selection_signal.v1
  Scanner-derived causal leakage visibility from exposure rows with
  use_selection_signal=true. These rows inherit protocol identity from their
  parent exposure fact and record selector split, selector metric, tie policy,
  selected checkpoint, candidate-checkpoint count/digest, and optional parent
  evaluation fact digests. The candidate digest is an identity audit for the
  declared checkpoint surface, not a quality ranking. The event digest is the
  stable cross-fact binding key; parent evaluation digests are included in a
  separate provenance digest so forecast-eval and selection-signal evidence do
  not form a circular digest dependency. It is not a scheduler, selector, model
  chooser, or artifact-readiness target; the catalog reports its promotion
  blocked reason as `leakage_visibility_only`.
  The summary resolves parent evaluation digests against forecast-eval facts
  and requires the parent evaluation to reference the same stable selection
  event digest. Missing parent evaluations, identity mismatches, and missing
  back-references are provenance integrity issues only; they do not promote
  selection signals into proof or policy authority.

kikijyeba.lattice.node_exposure.v1
  Scanner-derived MDN-head support visibility from MDN per-node report fields.
  These rows inherit protocol identity from their parent exposure fact and are
  not VICReg or NodeLift support evidence.

kikijyeba.lattice.representation_support.v1
  Scanner-derived shared-representation support visibility from representation
  reports and NodeLift .lls sidecars when honest support payloads exist. These
  rows inherit protocol identity from their parent exposure fact, are
  warning/visibility evidence only, and are never backfilled from downstream
  MDN node rows.
  They are not artifact-readiness targets; the catalog reports their promotion
  blocked reason as `support_visibility_only`.
```

The generic fact catalog is the expansion path for roadmap evidence. Hero
exposes `hero.lattice.inspect subject=fact_families`, `hero.lattice.inspect subject=facts mode=scan`,
`hero.lattice.inspect subject=facts mode=summary`, `hero.lattice.inspect subject=facts mode=lineage`, and
`hero.lattice.inspect subject=facts mode=preview` so agents can inspect fact families without
promoting them to `TARGET_KIND`. Catalog families are read-only,
non-dispatchable, not target-proof authority, not Runtime executors, do not
write evidence, and are not Marshal reachability surfaces.
These tools report those boundaries at the top level, including
`model_selector=false`, `performance_selector=false`, `policy_gate=false`,
`allocation_decision=false`, `market_readiness_decision=false`, and
`deployment_decision=false`, so clients do not infer authority from fact-family
names. A future target may prove a narrow artifact claim over catalog facts, but
fact-family existence alone is never a readiness, quality, allocation, or
deployment decision.
Fact-family registry and summary rows expose `artifact_readiness_proof_kind`
and `artifact_readiness_proof_claim` for proofable catalog families, and null
values for warning/summary-only families. Non-proofable rows also expose
`artifact_readiness_target_promotion_blocked = true` with a concrete blocked
reason such as `warning_summary_only`, `leakage_visibility_only`, or
`support_visibility_only`; clients should inspect those facts, not synthesize
artifact targets from them.
The catalog summary also carries explicit zero-count decision-authority fields:
`quality_authority_family_count`, `performance_authority_family_count`,
`checkpoint_selector_family_count`, `allocation_authority_family_count`,
`execution_authority_family_count`, `policy_gate_family_count`,
`target_dependency_authority_family_count`,
`runtime_wave_authority_family_count`, `marshal_reachability_family_count`,
`checkpoint_source_authority_family_count`, and
`plan_checkpoint_input_authority_family_count`. `decision_authority_clean=true`
is the catalog-level guard that these fact families remain evidence rows, not
selectors, gates, Runtime wave sources, checkpoint sources, or Marshal reach
surfaces.

### Producer Contracts For Active Expansion Families

The active expansion families use durable input files plus scanner-derived
summaries. The durable inputs are job-local sidecars or source analytics `.lls`
payloads; summaries are read models and must not become a second evidence
contract.

Common binding:

- Every produced fact inherits parent exposure identity: parent exposure digest,
  protocol id, contract fingerprint, graph order, source cursor, split policy,
  component/job/wave identity, and row-index intervals.
- Empty producer payloads are ignored. Malformed non-empty payloads remain
  visible and emit warnings according to their authority model.
- All families below are read-only catalog rows. Only the explicit
  artifact-readiness templates may prove narrow artifact claims over them.

`source_analytics`

- Producers: `lattice.source_analytics.fact`, `source_analytics.fact`,
  `runtime.source_analytics.fact`, `source.analytics.fact`, or scanner-derived
  `data_analytics*.lls` / `source_data_analytics*.lls` numeric and symbolic
  payloads.
- Required payload: at least one source-health metric or health level, plus the
  inherited parent/source cursor identity.
- Visibility fields: entropy, entropy rate, information density, compression
  ratio, power-spectrum entropy, volatility, feature variance, validity,
  missingness, duplicate samples, source receipts, and train-validation regime
  deltas.
- Authority: warning and summary only; no coverage, leakage, readiness,
  contract identity, quality, or performance authority.

`target_transform`

- Producers: `lattice.target_transform.fact`, `target_transform.fact`,
  `runtime.target_transform.fact`, or `target.transform.fact`.
- Required payload: target feature ids, horizon, target mode, normalization
  contract, inverse-transform contract, units, and target mask policy digest.
- Visibility fields: support surface identity and support surface digest.
- Authority: artifact-contract prerequisite only; not a quality, readiness,
  coverage, leakage, allocation, or deployment claim.

`forecast_baseline`

- Producers: `lattice.forecast_baseline.fact`, `forecast_baseline.fact`,
  `runtime.forecast_baseline.fact`, `forecast.baseline.fact`, or deterministic
  sidecars named for `previous_value`, `zero_return`, `moving_average`, and
  `last_valid_channel`.
- Required payload: target feature ids, horizon, baseline kind, target-transform
  fact digest, support or valid count, and metric status.
- Visibility fields: baseline parameters and baseline NLL/EV error, signed
  error, and directional-accuracy metrics when computed.
- Authority: evidence prerequisite and comparison support only; not model
  quality acceptance or checkpoint selection.

`forecast_eval`

- Producers: `lattice.forecast_eval.fact`, `forecast_eval.fact`,
  `runtime.forecast_eval.fact`, or `forecast.eval.fact`.
- Required payload: target feature ids, horizon, support or valid count,
  forecast artifact digest, evaluated representation checkpoint digest,
  evaluated MDN checkpoint digest, target-transform fact digest, baseline fact
  digests, and horizon support surface.
- Visibility fields: aggregate and stratified NLL, EV error, signed error,
  directional accuracy, calibration coverage, PIT summary, sigma-scale sanity,
  support surfaces, and selection-signal digests.
- Authority: non-mutating artifact evidence only; not forecast-quality
  acceptance, best-checkpoint selection, allocation, performance, or deployment
  authority.

`observer_belief`

- Producers: `lattice.observer_belief.fact`, `observer_belief.fact`,
  `runtime.observer_belief.fact`, or `observer.belief.fact`.
- Required payload: belief kind, forecast artifact digest, scenario-bank
  digest, feature-semantics fingerprint, and dock-binding fingerprint.
- Visibility fields: channel consensus, potential-surface diagnostics, NodeLift
  return projection, covariance coupling, residual quality, projection
  validation, confidence, data quality, liquidity, and forecast lineage.
- Authority: observer visibility only. `raw_nodelift_potential` is never
  tradable return evidence, and `allocation_belief` is not allocation authority.

`allocation_engine`

- Producers: `lattice.allocation_engine.fact`, `allocation_engine.fact`,
  `runtime.allocation_engine.fact`, or `allocation.engine.fact`.
- Required payload: accounting numeraire graph node, accounting numeraire node
  source, BasePolicy accounting numeraire node, graph-bound numeraire flag,
  observer-belief fact digest, forecast artifact digest, BasePolicy digest,
  objective terms, CVaR loss, transaction-cost estimate, constraint
  diagnostics, fallback reason contract, and de-risk reason contract.
- Visibility fields: target-node weights, numeraire weight, turnover, cap
  diagnostics, scenario growth-floor status, fallback reasons, and de-risk
  reasons.
- Authority: deterministic audit evidence only. Lattice does not allocate,
  select allocations, route execution, decide market readiness, deploy, or
  override the portfolio engine.

Registry and catalog-summary rows also include `fact_identity_contract`, the
shared runtime evidence identity envelope for fact families. It names the common
schema/fact-type/digest fields, parent exposure digest when applicable,
protocol/contract/graph/source-cursor/split/component/job-wave identity,
row-index anchor/completed intervals, family-specific lineage digest fields, and
support fields. The contract keeps row-index intervals authoritative,
source-key windows audit-only, and target-kind, Runtime-wave, Marshal
reachability, and policy-gate authority false.
`inspect subject=facts mode=scan`, `inspect subject=facts mode=summary`, `inspect subject=facts mode=lineage`, and `inspect subject=facts mode=preview` also return
`fact_integrity_summary`, a native Lattice rollup that separates declared
relation digests from resolved, identity-compatible bindings. It reports bound,
unresolved, identity-mismatch, and digest-mismatch relation counts, affected
families, integrity flags, and issue codes for transform, baseline,
selection-signal, forecast, observer, allocation, and policy-training lineage.
Policy-training parent lineage can cross component families: a
`wikimyei.policy.trainable` subject fact may bind MDN, observer/allocation, and
replay parent facts when the shared runtime/protocol/graph/cursor/split identity
and declared digests match. `inspect subject=facts mode=lineage` adds selected
relation/key/digest rows from the rebuildable runtime index as audit-only
witnesses; those cache rows are not target-proof authority and are never used
for Marshal dispatch or target satisfaction. `inspect subject=facts
mode=preview` returns concrete fact rows for one family by `fact_digest`,
`fact_digest_prefix`, or `fact_index`, plus matching lineage rows. Prefix
lookup fails closed unless the prefix resolves to exactly one full fact digest
in the selected family; the generic `digest`, `digest_prefix`, and `index` names
remain reserved for `subject=index mode=query`. Preview rows are audit-only and
explicitly report `facts_used_for_target_satisfaction=false`. Each preview row
also carries display-only `fact_ref` and `fact_digest_prefix` helpers next to
the full `fact_digest`; those refs are operator handles, not proof identity or
accepted authority inputs. Each row also carries a normalized `identity_envelope`
next to the concrete family payload. Lattice
projects that envelope through the typed catalog API
`lattice_fact_identity_envelope_t` /
`make_lattice_fact_identity_envelope`; Hero only serializes the projection. The
envelope applies the shared `fact_identity_contract` to the row: family, digest,
protocol/contract/graph/source-cursor/split/component/job-wave identity,
row-index ranges, parent exposure/fact/checkpoint/artifact digests, support
counters when present, and the same no-proof/no-dispatch/no-selection authority
flags.
Artifact-readiness proof JSON also emits `fact_preview_hint` for concrete
artifact facts, and blocked artifact plan-basis JSON aggregates those hints in
`fact_preview_hints`. These hints name `hero.lattice.inspect subject=facts mode=preview` and
`hero.marshal.inspect` arguments for inspection only; they do
not alter proof certificates, target status, checkpoint selection, or dispatch.
Target evaluation and checkpoint closure display JSON may also include
`certificate_ref`, `checkpoint_ref`, and `root_checkpoint_ref` next to the
corresponding full digest fields; these refs are operator handles only.

Narrow catalog proofs use `TARGET_CLASS = artifact_readiness`, `PROOF_KIND`,
and `SUBJECT_FACT_FAMILY` instead of adding roadmap evidence to `TARGET_KIND`.
The active proof templates are `target_transform_contract_bound`,
`forecast_baseline_artifact_bound`, `forecast_eval_artifact_bound`,
`observer_belief_artifact_bound`, and `allocation_artifact_bound` over their
matching fact families. These targets require matching protocol/contract and
graph/source identity, a parent exposure digest, required lineage fields, and
clean authority flags. `replay_environment` is an active non-dispatchable
artifact-readiness family through `replay_environment_artifact_ready`. Its proof
template binds durable Runtime replay reports, Cajtucu paper-execution counters,
nonzero cost evidence, policy-set and execution-profile digests, clean time-law
counters, and authority denials; it does not prove policy quality,
profitability, market readiness, or deployment readiness.
Certificate artifact rows bind the source fact back to the catalog contract with
`fact_schema`, `fact_type`, `fact_identity_contract_id`,
`fact_identity_contract_bound = true`, `fact_identity_envelope_complete = true`,
`row_index_interval_authority = true`, and
`source_key_window_audit_only = true`. They also bind `proof_kind` back to the
artifact registry with `proof_template_bound = true` and a
`proof_template_claim`; verification rejects rows whose fact family, fact
schema, identity-contract binding, proof kind, bound flag, or claim no longer
match the registered catalog/proof templates. This keeps artifact readiness as
an explicit template-bound claim rather than arbitrary fact existence.
Fact families without an explicit artifact proof template, including
`source_analytics`, remain warning/summary evidence and cannot be promoted to
artifact-readiness targets by spelling a new `PROOF_KIND`.
Forecast baseline and evaluation proofs also require their referenced
target-transform/baseline/selection-signal digests to exist under the same
target identity, not merely somewhere in the scanned ledger. Observer-belief
and allocation proofs likewise close over identity-matched forecast and observer
digests so artifact lineage cannot be borrowed from another split, cursor, or
protocol. Forecast evaluation artifact proofs require checkpoint-lineage
digests, support counts, baseline/transform bindings, selection-signal audit,
and no model-state mutation. Replay environment artifact proofs require bound
replay batch, experiment, and report lineage, exact replay artifact schemas,
exact historical-replay contract identity, resolved-cursor/no-future-leakage/
projection-validation guards, graph-node
BasePolicy reserve semantics, complete attempted episodes, per-episode
cursor/range evidence, sequential source-order evidence, explicit
requested/resolved parallelism evidence, observation/action/execution time-law
step evidence matching the expected completed-step count, action decision
timestamps after knowledge and before realization availability, zero
future-observation/realization-key
violations, and projection-validation metric and step summaries matching the
same count. They force
`CHECKPOINT_SOURCE = none`, default `PLAN_MAX_ATTEMPTS = 0`, emit no suggested
wave, and write their proof into `proof_certificate.artifacts[]`. They are
artifact-integrity proofs only: they do not judge forecast quality, choose
checkpoints, allocate capital, route execution, declare deployment/market
readiness, or make replay environment evidence reachable work. Artifact proof
rows expose those boundaries as explicit false authority flags, including
quality/performance authority, checkpoint selection, allocation/execution
authority, market/deployment authority, policy gates, target dependencies,
Runtime wave authority, Marshal reachability, checkpoint-source authority, plan
checkpoint inputs, model-state mutation, raw-potential tradable-return claims,
and replay execution. When an identity-matched artifact fact exists but
fails those rules, the failed evaluation keeps the first matching
`proof_certificate.artifacts[]` row with `passed = false`, its explicit drift
flags, and its issue list instead of hiding the failure in prose-only reasons.
The deficit vector is derived from that failed row with keys such as
`artifact:forecast_eval_authority` and
`artifact:forecast_eval_issue_forecast_eval_must_remain_artifact_evidence_only`,
so the primary operator action remains evidence-panel inspection rather than a
generic certificate failure.

The inventory classifies current report and fact fields as follows:

```text
runtime identity:
  training_id, config_path, component_assembly_id, graph_order_fingerprint,
  wave_id, wave_mode, target_action, source cursor policy, stream_plan

normalized proof identity:
  contract_fingerprint, graph_order_fingerprint,
  component_assembly_fingerprint, target_component_family_id, job_id, wave_id,
  job_status, wave_action, cursor_domain, source_cursor_token

model-state input/output:
  representation_checkpoint_path, mdn_checkpoint_path, checkpoint_written,
  checkpoint_path, input_checkpoints, output_checkpoint, mutated_component,
  optimizer_steps

exposure support:
  source anchor counts, accepted/skipped anchor counts, row-index footprints,
  completed ranges, source_input_length, source_future_length, and use flags

checkpoint lineage:
  checkpoint_id, checkpoint_file_digest, component, direct exposure digest,
  input_checkpoints, closure digest, producer job/wave, and fact digest

source receipts:
  compact source_file_receipts normalized into structured audit-only receipt
  facts, with malformed non-empty receipts warning and missing receipts treated
  as audit absence

source analytics:
  optional source-health sidecars normalized into structured visibility-only
  facts and summaries, with row-index intervals kept as the only
  coverage/leakage authority

target transform:
  optional target contract sidecars normalized into artifact-contract facts and
  summaries; they make forecast errors interpretable but do not make any
  quality, performance, readiness, allocation, or deployment claim

forecast baseline:
  optional deterministic baseline sidecars normalized into evidence-prerequisite
  facts and summaries; they make later model forecast evidence comparable but do
  not select checkpoints or accept/reject model quality

forecast evaluation:
  optional forecast-evaluation sidecars normalized into artifact-evidence facts
  and summaries; they expose NLL, EV error, directional accuracy, calibration,
  support, evaluated checkpoint lineage, target-transform binding, and baseline
  binding without declaring forecast quality ready

node and representation support:
  MDN per-node support rows for MDN heads; representation/NodeLift aggregate and
  node-indexed support rows for shared representation visibility

diagnostic warning metrics:
  VICReg health/geometry metrics and MDN loss/support/distribution diagnostics;
  these remain warnings/visibility unless a target explicitly promotes them
```

Guardrails:

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

Exposure facts also carry Ujcamei graph-anchor domain health. This reports the
candidate anchor count, accepted anchor count/fraction, skipped anchors by
reason, duplicate anchors, common/reference key bounds, and a warning level. The
graph-first source path already clips training to the accepted common anchor
domain; these fields make that clipping visible to Runtime/Lattice Hero. The v0
policy is warning-oriented: zero accepted anchors is an error, low accepted
fraction, failed fetch probes, and high missing-edge coverage are scan warnings.
Target satisfaction is unchanged by these warnings unless a target later adds an
explicit requirement.

VICReg representation exposure facts also preserve representation-health
metrics when the runtime report emits them: invariance, variance, and covariance
loss components, gradient norm, valid projection rows, adapter valid-channel
fraction, the finite-parameter check, and optional geometry metrics such as
embedding dimension, effective rank, effective-rank fraction, min/max dimension
variance, condition number, and isotropy score. These fields are
visibility/audit evidence only in V1. They do not turn VICReg readiness into a
performance or geometry gate unless a future target explicitly requires one.
Hero JSON emits `representation_geometry_vocabulary` with the metric families,
units, high-bad/low-bad threshold direction, and V1 non-blocking warning scope.
Hero JSON also emits `representation_geometry_summary`, which self-checks that
the 18 VICReg health/geometry metrics remain V1 non-blocking warnings, not
active performance or geometry gates.
V3-H adds `representation_geometry_gate_review_summary`: it reports observed
VICReg geometry distributions, confirms that no default threshold was promoted,
and documents the only hard-gate path as explicit
`LATTICE_REQUIRES KIND=representation_geometry`. Missing geometry facts fail
that opt-in gate closed; they do not silently satisfy it.

MTF-JEPA-MAE-VICReg representation facts follow the same evidence boundary.
Runtime should emit compact, identity-bound facts for
`wikimyei.representation.encoding.mtf_jepa_mae_vicreg`: config/checkpoint/source
lineage, finite loss/parameters/gradients, sample/channel support, valid latent
rows, split JEPA/MAE-time/MAE-frequency/TF/VICReg loss means, TF-pair support,
VICReg row counts, context-starvation counters, target EMA distance, and
geometry summaries. Lattice treats geometry, EMA, TF-pair, and context-quality
signals as warning visibility unless a target explicitly promotes a gate. It
must not ingest raw embeddings, token tensors, per-token masks, unbounded
histograms, or downstream forecast/MDN claims from this representation family.

For node-centered MDN jobs, the scanner also derives
`kikijyeba.lattice.node_exposure.v1` facts from the existing
`inference.report` per-node fields. These facts link back to the parent
exposure fact digest and carry node id/index, routed row count, active row
count, trained row count, evaluated row count, valid target count,
valid-target opportunity count, valid target fraction over the completed anchor
range, mean NLL, and the same
contract/graph/source/component identity as the parent MDN fact. This is
intentionally MDN-only as MDN-head evidence: VICReg is a shared encoder, so
shared-representation node support must come from NodeLift or representation
reports that emit honest node-support summaries. Hero JSON emits
`node_support_scope_policy_vocabulary` so agents can inspect that boundary
programmatically: MDN node rows are support visibility for MDN heads, not
per-node VICReg readiness, and future representation-node support must come from
component-scoped NodeLift/representation evidence rather than synthetic backfill
from downstream MDN rows.
Hero JSON also emits `node_support_scope_policy_summary`, which self-checks that
the policy remains read-only MDN-head support visibility, forbids downstream MDN
backfill into VICReg per-node readiness, and requires node-indexed
representation-node support to come from honest per-node
NodeLift/representation facts.
For the shared VICReg path, V2 scans also derive
`kikijyeba.lattice.representation_support.v1` facts from representation reports
and NodeLift `.lls` sidecars when support payloads are present. Aggregate facts
bind to the parent representation exposure digest and active
contract/graph/source identity, expose valid projection rows, adapter
kept/dropped/support counts, and NodeLift mask/support diagnostics. Node-indexed
facts are emitted when reports provide honest node ids, denominators, valid
lifted-row/feature/cell counts, valid projection rows, and lifted-feature
fractions. Hero exposes weakest-node and projection-row balance metrics for the
shared representation path. All of these rows are explicitly
`visibility_only`: they are not hard readiness gates and they do not reuse MDN
node-support rows as representation evidence.
The lattice also derives `node_support_summary` views over these MDN facts:
support-row count, unique-node count, routed/active/trained/evaluated row totals,
support-row incidence counts by exposure use and mutation, valid-target totals,
aggregate valid-target opportunity counts, aggregate Wilson 95% support bounds,
finite valid-target fraction range, weakest node, and simple support-balance
metrics such as coefficient of variation, Gini, and normalized entropy over
valid target counts. The weakest-node view also carries its own Wilson 95%
interval for valid-target support, so small per-node samples remain visibly
uncertain instead of looking like exact support fractions. Summaries include an
ordered `support_rows` matrix with the parent exposure fact digest, node
id/index, use flags, and per-node row/valid-target counts. Valid-target
denominators are target-opportunity counts inferred from the reported
valid-target fraction when available, not plain row counts; routed rows remain
visibility and do not become Wilson denominators by themselves. The shared
`lattice_valid_target_uncertainty_vocabulary` names the Wilson 95% method,
success/opportunity fields, interval fields, and no-trials non-claiming policy;
Hero JSON emits the same `valid_target_uncertainty_vocabulary`.
Hero JSON also emits `valid_target_uncertainty_summary`, the compact self-check
for the three support scopes, Wilson-95/binomial method, field bindings,
no-trials non-claiming policy, and support-visibility-only boundary. Certificate
checks require each row to name a parent exposure fact in closure, then
recompute the use/mutation incidences, aggregate totals, Wilson intervals, and
balance metrics from those rows instead of trusting scalar summaries alone.
`LATTICE_WARN KIND=node_support_balance` can flag high Gini/CV imbalance or low
normalized entropy, and `KIND=node_support_floor` can flag a weak lowest-support
node or low aggregate Wilson lower-bound support. These warnings filter the
support-row matrix by `USE` and `EFFECT` before measuring metrics; the default is
`USE=target_supervision` and `EFFECT=mutated_component`. If the warning declares
`SPLIT` or explicit `ANCHOR_INDEX_BEGIN`/`ANCHOR_INDEX_END`, the node-support
matrix is re-derived for that warning interval before the USE/EFFECT filter is
applied. These warnings are non-blocking and do not change readiness status.
Each warning result carries a structured `measurement_available` bit;
unavailable warning counts and evidence order dimensions do not require agents
to parse message text.
`LATTICE_WARN KIND=forecast_eval` is the catalog-fact warning surface for
scanner-derived forecast evaluation facts. It can flag `mean_nll`, `ev_mae`,
`ev_rmse`, `signed_error`, `directional_accuracy`, `calibration_coverage`,
`pit_summary`, `sigma_scale_sanity`, `support_count`, `valid_count`,
`missing_count`, `weakest_support_rows`, `weakest_horizon_support_rows`,
`mean_nll_per_horizon_max`,
`skill_vs_baseline`, `skill_vs_naive`, `skill_vs_baseline_mean_nll`,
`skill_vs_baseline_ev_mae`, `skill_vs_baseline_ev_rmse`, and
`directional_accuracy_delta_vs_baseline`.
These warnings can attach to artifact-readiness targets but remain visibility
only: they do not accept forecast quality, rank checkpoints, select allocations,
or alter dispatch.
Warning envelopes classify these metrics by family: calibration metrics emit
`forecast_calibration`, support-surface metrics emit `forecast_support`,
baseline skill metrics emit `forecast_baseline_comparison`, and other forecast
diagnostics emit `forecast_quality_visibility`.
`LATTICE_WARN KIND=forecast_baseline` is the catalog-fact warning surface for
scanner-derived deterministic baseline facts. It can flag `baseline_mean_nll`,
`baseline_ev_mae`, `baseline_ev_rmse`, `baseline_signed_error`,
`baseline_directional_accuracy`, `support_count`, `valid_count`,
`missing_count`, `baseline_kind_count`, `computed_metric_fact_count`,
`partial_metric_fact_count`, `deferred_metric_fact_count`,
`computed_metric_value_count`, and `metric_status_mismatch_count`. These
warnings show whether baseline support, baseline-kind coverage, metric
coverage, and reference error are legible; they do not make the model good,
select checkpoints, or promote a baseline into a policy gate.
Baseline support metrics emit `forecast_support`; baseline reference and
coverage diagnostics emit `forecast_baseline_comparison`.
`LATTICE_WARN KIND=observer_belief` is the catalog-fact warning surface for
scanner-derived observer facts. It can flag `confidence`, `data_quality`,
`liquidity`, `raw_potential_tradable_return`, `allocation_authority`,
`forecast_lineage_bound`, `channel_consensus_bound`,
`potential_surface_diagnostics_bound`, `nodelift_return_projection_bound`,
`covariance_coupling_bound`, `scenario_bank_bound`,
`nodelift_residual_quality_bound`, `projection_validation_scores_bound`,
`feature_semantics_bound`, and `dock_binding_bound`. These warnings are
visibility diagnostics only: raw NodeLift potential remains distinct from
allocation belief, and Lattice still does not treat observer output as tradable
return, allocation quality, market readiness, or deployment authority.
Observer contract and lineage metrics emit `lineage_integrity`; confidence,
data-quality, liquidity, and authority-boundary diagnostics emit
`observer_belief_consistency`.
`LATTICE_WARN KIND=allocation_engine` is the catalog-fact warning surface for
scanner-derived allocation-engine audit facts. It can flag `numeraire_weight`,
`turnover`, `cvar_loss`, `transaction_cost_estimate`,
`deterministic_artifact`, `visibility_only`, `allocation_authority`,
`execution_authority`, `market_readiness_authority`,
`deployment_authority`, `accounting_numeraire_node_bound`, `observer_belief_bound`,
`forecast_artifact_bound`, `base_policy_bound`,
`accounting_numeraire_node_from_base_policy`, `accounting_numeraire_node_base_policy_match`,
`accounting_numeraire_node_graph_bound`, `cap_diagnostics_bound`,
`scenario_growth_floor_status_bound`, `scenario_growth_floor_met`,
`scenario_growth_floor_attention`, `fallback_declared`, `fallback_active`,
`derisk_declared`, and `derisk_active`. These warnings are audit visibility
only: Lattice still does not select allocations, route execution, judge market
readiness, or override the portfolio engine.
Allocation binding/completeness metrics emit `lineage_integrity`; allocation
cost, risk, turnover, and authority-boundary diagnostics emit
`allocation_engine_diagnostics`.
`LATTICE_WARN KIND=mdn_distribution_calibration` is the non-blocking
distributional MDN warning surface. V3-D can evaluate aggregate `mean_nll`,
channel and horizon max-NLL summaries when runtime reports emit them, and
per-node max NLL from node exposure facts. PIT KS, predictive interval coverage
error, tail coverage error, and calibration slope remain future non-gating
metrics until runtime emits samples and uncertainty. Hero JSON emits
`mdn_distribution_calibration_vocabulary` and
`mdn_distribution_calibration_summary` for the current available-vs-future
metric policy, plus
`mdn_distribution_calibration_diagnostic_vocabulary` and
`mdn_distribution_calibration_diagnostic_summary` for the V3-D diagnostic
binding policy: exact evaluated MDN checkpoint, representation checkpoint,
split policy, active identity, validation split, sample count, uncertainty
method, and warning-only/non-performance status.
Hero JSON emits `representation_geometry_vocabulary` for the analogous VICReg
health/geometry warning surface: loss components, gradient norms, projection
support, finite-parameter checks, and embedding-spectrum metrics remain
visibility unless a future target makes them explicit gates.
Hero JSON also emits `representation_geometry_summary` for the compact
self-check of that non-gating representation-health boundary.
Hero JSON also emits `join_law_vocabulary`, which states the algebraic merge
laws behind the product evidence state: identity-scoped idempotent joins for
coverage, closure, leakage witnesses, warnings, and deficits; additive joins
for repeated exposure load and distinct node-support rows; and fail-closed
rules for unsafe joins.
Hero JSON also emits `performance_uncertainty_policy_vocabulary`: V1 exposes
Wilson support intervals as visibility, while future performance gates must use
declared uncertainty methods and compare conservative confidence bounds rather
than raw point estimates.
Hero JSON also emits `performance_uncertainty_policy_summary`, which self-checks
that one row is V1 support visibility, five rows are future performance-gate
policy, V1 has no performance-gate authority, point-estimate gates are
disallowed, and confidence bounds plus selection-leakage policy are required.
Hero JSON also emits `validation_performance_evidence_policy_vocabulary`, the
V3-C evidence contract for validation performance facts. It names the finite
available metrics, split field, checkpoint identity fields, exact evaluated
checkpoint binding, sample-count field, point-estimate field, uncertainty
method, confidence-bound fields, target syntax, selection-leakage policy, and
fail-closed cases.
Hero JSON also emits `validation_performance_evidence_policy_summary`, which
self-checks that V3-C has no active performance-gate authority, available point
estimates are either bounded or warning-only, mean NLL remains warning-only
until uncertainty is emitted, and missing uncertainty, wrong checkpoint binding,
stale identity, and selector leakage are named fail-closed cases.
Hero JSON also emits `mathematical_readiness_v1_vocabulary`, a crosswalk from
the mathematical strengthening points to the concrete Hero fields that carry
them, including which claims are V1 read-side authority and which remain future
work.
It also emits `mathematical_readiness_v1_summary`, a compact self-check that the
crosswalk has 16 included read-only items, zero runtime-executor/performance/DB
authority items, and no empty Hero-surface rows.
Hero JSON also emits `validation_performance_scope_policy_vocabulary`, which
keeps `channel_mdn_validation_eval_ready` as clean evaluation-readiness evidence:
evaluation coverage, zero optimizer mutation, exact MDN checkpoint binding,
exact evaluated checkpoint input edges, and matching representation lineage. It
does not claim model quality or performance approval; future performance gates
need explicit metrics, uncertainty, and selection-leakage policy.
Hero JSON also emits `validation_performance_scope_policy_summary`, which
self-checks that V1 validation evaluation is not performance acceptance and that
future performance authority remains deferred behind explicit uncertainty and
selection-leakage policy.

When a job writes a checkpoint, runtime also emits a transitional
`lattice.checkpoint.fact` sidecar. This records component/contract/graph/source
identity, the producer job/wave, the direct exposure digest, input checkpoint
paths, a preview `checkpoint_id`, and a `checkpoint_file_digest` when the
checkpoint file is available. Current closure authority is checkpoint id/file
digest identity; path reachability alone is not a satisfying checkpoint
identity and missing or stale identity fails closed. Historical V1 receipts may
describe older closure semantics, but that wording is receipt history, not
active target behavior.
The runtime-root scanner now parses checkpoint facts into the in-memory ledger
when they are present.

The ledger can compute checkpoint exposure closures by following input
checkpoint links. Closure resolution is fail-closed: if an output checkpoint
names an input checkpoint and the ledger has no producer fact for that input,
target evaluation treats the closure as incomplete instead of silently ignoring
the missing upstream lineage. This is the basis for leakage checks such as:
`output_checkpoint_exposure_closure` must not overlap validation or test cursor
footprints under forbidden uses like observed input, target supervision, or
selection signal.

Read-only lattice targets live under
`cuwacunu::hero::lattice::target`. A target is a desired state over
contract-scoped evidence, not a wave and not a scheduler. The v0 evaluator reads
runtime job manifests, job state, component reports, and checkpoints, then
returns a status plus an optional suggested wave. It does not execute.

Named split policy lives under `cuwacunu::hero::lattice::split`.
`hero.lattice.splits.dsl` defines graph-anchor cursor ranges and roles
outside the target language. Targets may use `TRAIN_SPLIT` / `OVER_SPLIT` to
resolve their anchor-index training range. Validation/test splits may also
declare `PROTECT_FROM_USES`, and targets can use `PROTECT_SPLIT` to apply that
holdout protection without repeating low-level forbidden-use lists. This keeps
train/validation/test definitions centralized instead of duplicating raw ranges
across target blocks. The split policy has a deterministic fingerprint. Target
evaluation reports that fingerprint, and exposure facts can carry it; once a
fact records a split-policy fingerprint, evaluation will not accept it under a
different split policy. Concrete named-split facts without that fingerprint are
also rejected while evaluating under an active split policy, while historical
unknown-split facts remain admissible.

When a target requires contract or component matching, the active contract and
component fingerprints must be supplied by the caller. Graph-anchor ranged
targets and exposure requirements also require the active graph order
fingerprint and source cursor token, because anchor indices only have meaning
inside that cursor/order identity. Missing active identity blocks the target
instead of silently accepting stale evidence. Target evaluation scans matching
jobs newest-first and accepts the newest satisfying candidate; failed, stale, or
checkpoint-missing attempts do not hide an older compatible ready artifact.
`MAX_WAVES` is a planning guard over active non-dry-run train attempts with the
same contract/component/graph/cursor identity, not a readiness failure: existing
satisfying evidence can still pass, but exhausted targets stop recommending
another wave.
The target decoder rejects duplicate `TARGET_ID`s. Reusing the same component in
multiple targets is valid when ranges/splits differ; v0 exposes a validation
report that warns when duplicate target selectors appear so accidental duplicate
component/range targets are visible without blocking legitimate future
multi-range targets.
The target language also supports lightweight `LATTICE_PROFILE` and
`LATTICE_GUARD` blocks. Profiles name reusable readiness defaults; guards remain
available for low-level reusable forbidden exposure policies, while ordinary
holdout protection should prefer split-level `PROTECT_FROM_USES` plus target
`PROTECT_SPLIT`. The decoder lowers these forms into the existing v0 scalar
target fields, so they improve authoring and inspection without changing
evaluator semantics. Preferred aliases are `SUBJECT_COMPONENT`, `OVER_SPLIT`,
`WAVE_MODE`, and `PLAN_MAX_ATTEMPTS`; older names still load for compatibility
and mixed old/new spellings produce validation warnings.
The first clause blocks are also supported and lower into the same v0 evaluator
model:

```text
LATTICE_DEPENDS
  declares the upstream target relationship, currently used by channel MDN to bind
  to the loaded representation checkpoint proof.

LATTICE_REQUIRES
  declares artifact, metric, optimizer-effort, node-head-count,
  valid-target-fraction, and exposure-coverage requirements.

LATTICE_FORBIDS
  declares forbidden exposure overlap. V0 supports source-row-footprint checks
  over the checkpoint closure.

LATTICE_PLAN
  declares the suggested wave mode/range and planning attempt limit.

LATTICE_WARN
  declares non-blocking warning checks over exposure load, effort density, or
  anchor-domain health, MDN node support, or VICReg representation health.
  Warning clauses may use SPLIT or ANCHOR_INDEX_BEGIN/END to scope their
  measurement interval. They report evidence shape; they do not change readiness
  status or execute anything.
```

These clauses are intentionally not execution machinery. The compiler keeps a
compiled proof object beside the lowered v0 target: applied profiles, applied
guards, dependency clauses, requirement clauses, forbidden-exposure clauses,
plan clauses, warning clauses, language warnings, and a deterministic
`target_spec_fingerprint` over the proof obligation. Advisory plan values are
preserved in the compiled object and lowered target, but excluded from that
fingerprint. Non-blocking warning clauses are also preserved for explanation
and evaluation while remaining outside proof-obligation identity. The existing
evaluator still consumes the lowered v0 target, while `hero.lattice.inspect subject=target mode=explain`
exposes the preserved proof object without scanning runtime evidence.

Targets can optionally bind readiness to the exposure ledger with cursor-indexed
checks. `MIN_OBSERVED_INPUT_COVERAGE` and
`MIN_TARGET_SUPERVISION_COVERAGE` require a checkpoint exposure closure to cover
a fraction of an explicit `SOURCE_RANGE=anchor_index` interval. In
`LATTICE_REQUIRES KIND=exposure_coverage`, `CURSOR_EPOCHS` is the calmer
authoring alias for `MIN_COVERAGE`: it means the selected exposure use covers
that fraction of the named Ujcamei graph-anchor cursor range. Coverage uses the
target component's graph-anchor coverage interval, so an MDN readiness target
cannot satisfy its own training coverage from an upstream VICReg checkpoint.
Evaluation targets use the same cursor-epoch language with
`USE=evaluation_metric` / `MIN_EVALUATION_METRIC_COVERAGE`; those checks require
non-mutating, zero-optimizer-step run-mode validation evidence with no output
checkpoint after the training checkpoint has passed its no-leakage guard.
The evaluator also reports derived exposure-load summaries for these coverage
requirements. Unique coverage answers how much of the cursor range has been
covered at least once; `cursor_exposure_load` counts repeated completed
exposure over the same range. For example, two completed passes over
`train_core` still have unique coverage `1.0`, but exposure load `2.0` cursor
epochs.

`LATTICE_WARN` adds non-blocking warning semantics over these summaries. Warning
clauses can report high `exposure_load`, high `effort_density`, suspicious
`anchor_domain_health`, source-health shifts from `source_analytics`,
forecast-baseline visibility from `forecast_baseline`, forecast-evaluation
visibility from `forecast_eval`, observer diagnostics from `observer_belief`,
allocation-engine audit visibility from `allocation_engine`,
weak/imbalanced/statistically thin MDN node support, or suspicious
`representation_health` metrics without changing target status, `plan_ready`,
suggested waves, `PLAN_MAX_ATTEMPTS` accounting, or `target_spec_fingerprint`.
Every warning kind may use `SPLIT` or explicit `ANCHOR_INDEX_BEGIN` /
`ANCHOR_INDEX_END`; measurements are scoped to that warning interval and fall
back to the target range when no warning-local range is provided. If trusted
completed coverage is unavailable, warning overlap can use anchor-range
visibility; that does not make untrusted coverage satisfy readiness. Hero JSON
returns the resolved interval as `warning_results[].warning_anchor_range`, while
`hero.lattice.inspect subject=target mode=explain` exposes the same pre-evaluation scope as
`warning_scope_previews[].resolved_warning_anchor_range`.
`warning_anchor_scope_policy_vocabulary` documents the visibility-only fallback
and node-support warning matrix scope for both surfaces.
Warning results are returned beside the normal evaluation as `warning_results`;
the short `warnings` list now projects typed non-blocking warning envelopes for
Marshal and other agents, while `warning_messages` preserves the ordered
projection of `warning_results[].message` where
`warning_results[].threshold_triggered` is true. If a warning input measurement
is absent, the result stays non-blocking and reports the measurement as
unavailable instead of formatting a non-finite value as a numeric comparison;
threshold-backed warnings still name the configured threshold. Each warning
result carries `warning_family`, `source=lattice`,
`component=lattice_target`, `blocking=false`,
`readiness_effect=non_blocking_warning_only`, `evidence_digest`,
`machine_reason_code`, `human_explanation`, `suggested_inspection_panel`,
`target_ids_observed_against`, and its `evidence_basis`, such as
`exposure_load_summary`, `source_analytics_facts`,
`forecast_baseline_facts`, `forecast_eval_facts`, `observer_belief_facts`,
`allocation_engine_facts`, `filtered_node_support_summary`,
`representation_health_facts`, or `anchor_domain_facts`, so agents can tell
which summary or fact family produced the measurement. The companion
`exposure_summary_available` and `node_support_summary_available` booleans mark
which summary envelope is a real warning basis and which is merely an empty
non-claiming placeholder. This keeps repeated exposure and
representation-health checks neutral: the lattice reports evidence shape, but
does not call the model overtrained, promote representation-geometry visibility
into a performance gate, split proof identity, or change the proof certificate
digest for the same proof evidence.
When a target explicitly opts into `KIND=representation_geometry`, the gate
compares the worst finite representation-health metric in the target evidence
against the declared value and returns a metric deficit on failure. This opt-in
gate is readiness/health evidence only, not validation performance.
Warning results expose `threshold_direction` (`above` or `below`) next to the
numeric threshold, `threshold_triggered` as a structured trigger bit, and
`threshold_relation` values such as `above_threshold`,
`not_above_threshold`, `below_threshold`, `not_below_threshold`, or
`unavailable`, so agents do not have to infer the comparison operator or trigger
state from the warning kind, status string, or diagnostic text.
Hero JSON includes `warning_threshold_relation_vocabulary` with the relation
names and their trigger/measurement-availability semantics, plus
`warning_anchor_scope_policy_vocabulary` for warning-interval visibility. It
also includes a `warning_summary` object that aggregates warning-result count,
triggered-warning count, unavailable-warning count, clear measured warning
count, blocking and non-blocking warning counts, `all_warnings_non_blocking`,
the compatibility `warning_count` alias, and relation counts for each threshold
relation value.
The target compiler also applies small dimensional checks to authored numeric
thresholds. Coverage fractions, valid-target fractions, accepted-anchor
fractions, Gini/normalized-entropy thresholds, adapter valid-channel fractions,
and boolean-style health thresholds must be finite values in `[0,1]`.
Cursor-epoch load and effort density thresholds must be finite and non-negative.
Count thresholds such as
node support floors, valid projection rows, and representation embedding
dimensions must be non-negative integral counts, and representation condition
number thresholds must be finite values at least `1`. Representation-health
warnings also enforce the mathematically bad direction for one-sided metrics:
loss, gradient-norm, maximum-variance, and condition-number warnings use
`ABOVE`; projection-row, valid-channel, finite-parameter, rank,
minimum-variance, and isotropy warnings use `BELOW`. Anchor-domain warning
clauses carry exactly one metric threshold so one warning result cannot hide
another measurement. These checks make the DSL safer for agents without changing
how runtime facts are evaluated.
Opt-in representation-geometry gates use the same unit checks with `VALUE`:
low-bad metrics require `OP=ge`, high-bad metrics require `OP=le`, count values
must be integral, fractions stay in `[0,1]`, and condition-number values are at
least `1`.
Targets may also set `CHECKPOINT_SOURCE = latest_satisfying:<target_id>`.
This is for read-only guard targets that inspect a checkpoint produced by
another target instead of matching their own runtime job. For example,
`channel_mdn_train_core_no_validation_leakage` inspects the latest checkpoint from
`channel_mdn_train_core_ready`, then applies validation split protection to that
checkpoint closure. `channel_mdn_train_core_no_test_leakage` chains from that
validation-clean target and applies `test_holdout` protection to the same
checkpoint. If the source target is not satisfied, the guard target blocks and
forwards the source target's suggested wave; it still does not execute anything.
`latest_satisfying` is a deterministic readiness selector over the referenced
target's newest satisfied checkpoint candidate under the active identity. It is
only valid for checkpoint-producing `readiness` and `leakage_guard` source
targets. It must not point at `artifact_readiness`, `evaluation_readiness`,
policy, performance, market, or deployment-readiness surfaces. It is not a
best-model selector, Pareto optimizer, performance ranking, deployment
recommendation, or scalar score. Hero JSON emits
`checkpoint_selection_policy_vocabulary` to make that selection scope explicit.
Hero JSON also emits `checkpoint_selection_policy_summary`, which self-checks
that V1 selector rows remain readiness-based, exact runtime binding is required
where model-state inputs are involved, and no selector row is performance,
Pareto, contract identity, or runtime execution authority.

Evaluation targets can bind to a previously proven checkpoint with
`EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:<target_id>`. For example,
`channel_mdn_validation_eval_ready` proves run-mode validation evidence only if the
MDN report loaded the exact checkpoint selected by
`channel_mdn_train_core_no_test_leakage`, and the representation checkpoint loaded
for the run matches that checkpoint lineage. Suggested waves may carry
`PLAN_INPUT_MDN_CHECKPOINT` and `PLAN_INPUT_REPRESENTATION_CHECKPOINT` model
state hints, but those are symbolic `latest_satisfying` source-target
references, not contract identity or proof evidence, and the concrete checkpoint
loaded by runtime still must be proven by the resulting report/facts. Changing
those plan-input hint values does not change `target_spec_fingerprint`; the
fingerprint is reserved for the target proof obligation, not the checkpoint
source hint used to run the next wave. Likewise, advisory planning knobs such as
`WAVE_MODE`, `PLAN_MODE`, `PLAN_MAX_ATTEMPTS`, and `MAX_WAVES` lower into
suggested-wave behavior but do not change `target_spec_fingerprint`.
Hero JSON emits `contract_identity_boundary_vocabulary` to make the split
machine-readable: protocol/graph/component/target proof fields are
separate from runtime-loaded checkpoint paths, plan advice, warning visibility,
and audit-only source receipt/key-window metadata.
It also surfaces config provenance and component spawn fields:
`config_bundle_id`, `config_receipt_id`, `component_spawn_registry_id`,
`component_family_id`, `component_spawn_fingerprint`, scoped
`component_spawn_id`, and `component_spawn_label`. The full fingerprint is the
audit authority; the spawn id is only a readability and retrieval hint.
Component spawn identity is stable across train/eval waves and source slices;
`source_cursor_token`, requested ranges, and completed ranges remain
job/evidence/checkpoint lineage used by Lattice source and leakage checks.
It also emits `contract_identity_boundary_summary`, a compact self-check that
model-state, planning advice, warning visibility, and audit metadata have zero
overlap with protocol contract or target proof context while active identity
surfaces are present.
The target remains a readiness proof, not a performance gate. Validation metric
coverage proves that the checkpoint was evaluated over enough trusted anchors;
it does not assert that NLL, calibration, economic utility, or deployability is
acceptable.

Artifact-readiness targets are the separate non-trainable proof path. They may
omit `TARGET_KIND` entirely and instead declare, for example:

```text
LATTICE_TARGET {
  TARGET_ID = observer_belief_artifact_ready;
  TARGET_CLASS = artifact_readiness;
  SUBJECT_FACT_FAMILY = observer_belief;
  SUBJECT_COMPONENT = wikimyei.inference.expected_value.mdn;
  PROTOCOL_ID = cwu_02v;
  SOURCE_RANGE = anchor_index;
  OVER_SPLIT = validation_holdout;
}
```

This proves the selected fact exists and is bound to the declared identity and
lineage. For forecast facts, the referenced transform and baseline digests must
be present in the catalog. It does not make Marshal "reach" the fact family and
it does not reuse training-job checks such as optimizer steps, finite loss, or
checkpoint output. It also cannot use `UPSTREAM_TARGET_ID`, `LATTICE_DEPENDS`,
`EVALUATED_CHECKPOINT_SOURCE`, or `PLAN_INPUT_*` checkpoint hints; artifact
lineage is proved from fact parent digests and the registered proof template,
not from target-dependency scheduling. `inspect subject=targets`, `inspect subject=target mode=explain`,
`evaluate operation=target`, and `evaluate operation=deficit` expose this as `target_surface_kind =
evidence_catalog_artifact`, `dispatchable_target = false`,
`runtime_wave_dispatchable = false`, and
`recommended_operator_action = inspect`. They also emit
`kind = not_applicable`, `target_kind_applicable = false`, and
`target_kind_effective = none`, so the internal default enum cannot masquerade
as a declared `TARGET_KIND`.

Decision policies remain separate. `TARGET_CLASS = policy_gate`,
`TARGET_CLASS = performance_acceptance`, `TARGET_CLASS =
validation_performance`, `TARGET_CLASS = market_readiness`, and
`TARGET_CLASS = deployment_readiness` all fail closed.
`LATTICE_POLICY_GATE` is reserved syntax only: the parser accepts
`ENABLED=false` declarations as reviewable future-policy reservations for the
explicit `forecast_quality_acceptance` and `allocation_acceptance` kinds,
computes their policy fingerprints, verifies any supplied `POLICY_FINGERPRINT`
against the canonical reservation digest, and keeps them out of target status
and `target_spec_fingerprint`. Each reservation must bind to the matching
`artifact_readiness` proof target (`forecast_eval` for forecast quality,
`allocation_engine` for allocation acceptance). `ENABLED=true` still fails
closed before it can change target status. Future gates must carry an explicit
policy input contract: policy identity, metric definition, baseline definition,
threshold, uncertainty policy and model, support minimum, selector split, tie
policy, anti-leakage policy, negative tests, calibration requirements, holdout
declaration, and threshold-selection audit trail.
Hero JSON exposes these reservations through `policy_gate_reservations` and
`policy_gate_reservation_summary` on target-list, explain, and evaluation
surfaces. Those rows report canonical policy-fingerprint verification, required
policy inputs, missing input lists, policy-input contract completeness, and zero
decision-policy, target-status, target-fingerprint, proof-certificate,
runtime-execution, allocation, market-readiness, and deployment authority.

Forbidden exposure ranges are a different question: they inspect the full
checkpoint closure, including upstream components, and reject a checkpoint when
its closure overlaps a protected source-row footprint under uses such as
`observed_input`, `target_supervision`, `evaluation_metric`, or
`selection_signal`. This split is intentional: target supervision coverage
answers "which anchors trained this component?", while target supervision
leakage answers "which future source rows did this checkpoint depend on?" These
checks return `exposure_failed`. The evaluator can build the ledger from normal
runtime job directories automatically; when that is disabled, missing ledger
wiring returns `blocked` because the target cannot be evaluated honestly.
Hero JSON emits `selection_signal_policy_vocabulary` to make this boundary
explicit: V1 treats `selection_signal` as a forbidden causal use over the
causal exposure anchor footprint. V2 additionally derives read-only
`kikijyeba.lattice.selection_signal.v1` event facts for those uses, binding the
selector id, selector split, selector metric, tie policy, selected checkpoint
path, candidate-checkpoint surface, parent exposure digest, and selector
footprint without adding coverage, contract-identity, ranking, or checkpoint
selection authority. When a forecast-evaluation fact references a selection
event, the ledger enriches the selection event with the forecast-eval digest as
parent evaluation provenance; that enrichment changes the provenance digest, not
the event digest used for relation resolution. Selection summaries verify that
the parent forecast-eval digest resolves under the same identity and points back
to the selection event. Arbitrary cross-event selector graph proofs remain
future provenance work.
`selection_signal_policy_summary` self-checks the four-row boundary: known
selection-signal is forbidden and causal-anchor based, first-class selector
events are read-only V2 evidence, known selector paths fail closed when
forbidden, all rows require closure completeness, and Lattice has no executor
authority.

Node-centered MDN readiness also verifies the exact representation checkpoint
recorded by the MDN report when exposure requirements are active. It is not
enough for some VICReg target to be ready elsewhere; the loaded checkpoint must
have an active compatible VICReg producer fact under the same contract, graph
order, source cursor, and VICReg assembly identity.
Split-backed MDN training targets use trained-node-head evidence, while
evaluation-style targets may still require evaluated-node-head evidence.

## Proof Certificates

Every `evaluate operation=target` result now carries a
`kikijyeba.lattice.target_certificate.v1` proof certificate. The certificate is
not a second evaluator; it is the structured record of the same checks that
produced the target status. It currently records:

Artifact-readiness evaluations also carry a `fact_integrity_summary` scoped to
the target's `SUBJECT_FACT_FAMILY`. Failed artifact proofs may add
`fact_integrity:*` deficit rows when the subject catalog has unresolved,
identity-mismatched, or digest-mismatched relation evidence. These rows explain
the catalog condition behind proof issues; they are not Runtime dispatch,
quality acceptance, allocation advice, or a separate policy gate.
Artifact proof deficits also expose `related_fact_integrity_issue_codes` when a
proof issue corresponds to catalog issue rows, so
`baseline_fact_digest_not_found` can point directly at
`artifact_job:baseline_fact_digest_not_found` or the equivalent component/job
issue code.

```text
proof obligation:
  certificate digest, target id, target_spec_fingerprint, and
  split_policy_fingerprint used for this evaluation

identity:
  active/evidence contract, graph order, source cursor, and component assembly
  fingerprints, plus the match booleans used by the evaluator

dependencies:
  upstream target status, CHECKPOINT_SOURCE bindings, and
  EVALUATED_CHECKPOINT_SOURCE exact MDN/representation checkpoint bindings

coverage:
  use, target cursor range, required fraction, actual fraction, covered anchors,
  sorted contributing intervals, contributing exposure fact digests, local
  contributing fact witnesses, merged idempotent-union intervals, missing
  complement intervals, and the associated load summary

closure:
  checkpoint path, closure completeness, fact count, unresolved inputs, and
  exposure fact digests. When checkpoint sidecars are available and their
  direct exposure digest binds to the current closure facts, closure also
  carries checkpoint identity fields: checkpoint path, checkpoint
  id, file digest, component, producer job/wave, and direct exposure digest.
  Stale or incomplete identity fails closed instead of becoming path-only
  proof authority.

causal_exposures:
  one labeled summary per exposure fact inherited through checkpoint closure:
  producer job/wave, optional Runtime handoff id/digest, optional Marshal
  target-driver run id, component, uses, mutation bit, anchor/footprint
  intervals, input/output checkpoints, source-key window fields, and a
  structured source-key order-preserving/affine-consistency audit. The label
  semantics are exposed by `lattice_causal_edge_vocabulary`: source-row use
  labels point into runtime jobs, `loaded_checkpoint`/`created_checkpoint` form
  checkpoint reachability edges, and `mutated_component` marks state mutation.
  When Runtime evidence carries a handoff digest, the proof certificate carries
  that same identity through the closure causal exposure and checks the
  `runtime_handoff_<digest>` id binding.

leakage:
  protected split/source-row footprint, the undilated split range, left/right
  dilation widths from Hx/Hf purge policy, forbidden uses, mutation requirement,
  whether any forbidden overlap was found, and witness facts/intersection
  intervals when leakage is detected

node_support:
  MDN node-support summaries derived from inherited `node_exposure` facts,
  including closure-backed ordered support rows, aggregate and weakest-node
  support, Wilson 95% support bounds, and support-balance metrics such as CV,
  Gini, and normalized entropy when available
```

The coverage proof names the algebra explicitly:
`idempotent_interval_union`. Repeated exposure remains in the additive
`exposure_load` summary. The summary also carries machine-readable algebra and
unit labels: unique coverage is an idempotent interval union reported as a
coverage fraction, while repeated load is an additive interval measure reported
in cursor epochs and anchor events. Readiness uses the idempotent coverage proof;
warning clauses may inspect repeated load or optimizer effort density without
changing target status. The allowed measure semantics are exposed by
`lattice_exposure_measure_algebra_vocabulary` and Hero JSON emits the same
`exposure_measure_algebra_vocabulary`, so clients can distinguish idempotent
coverage from additive load without parsing field names.
Hero JSON also emits `exposure_measure_algebra_summary`, which makes that
distinction self-checking: exactly two measures, one idempotent coverage
measure, one additive load measure, no dual-role measure, and distinct units.
Hero JSON also emits `source_key_coordinate_policy_vocabulary`, which states
that row-index intervals are authoritative for coverage and leakage, while
source-key windows and row-to-source-key monotonicity/affine checks are
auxiliary audit coordinates.
Hero JSON also emits `source_key_coordinate_policy_summary`, the compact
self-check for the row-index authority row, audit-only source-key coordinate
rows, order-preserving/affine/gap audit fields, and no coverage/leakage
authority on source-key audit rows. `hero.lattice.inspect subject=exposure` emits
`source_key_map_audit_summary`, a runtime summary of monotonicity,
order-preservation, affine consistency, gap/irregular-key warnings,
row/source-key mismatches, and source-receipt binding counts.
The certificate self-check recomputes the merged idempotent union from the
contributing intervals, requires covered intervals to be canonical
non-overlapping intervals inside the target range, and recomputes the missing
complement. That keeps a coverage proof from passing with matching totals but
overlapping, out-of-range, or non-complement interval evidence. Coverage proofs
also carry contributing exposure fact digests and the contributing exposure fact
witnesses used for the interval proof. The self-check recomputes each witness
digest and coverage interval locally; for mutating checkpoint-backed coverage,
it also binds those digests to closure causal exposures.
The same contributing interval list is also the additive load witness: the
self-check requires `loaded_anchor_events` to equal the sum of contributing
interval lengths and `fact_count` to equal the number of contributing exposure
intervals/fact digests/fact witnesses. Repeated exposure therefore remains
visible as repeated intervals before the idempotent union is formed.
Target satisfaction is monotone only with respect to clean evidence growth:
adding matching, non-forbidden evidence may increase load while preserving or
improving unique coverage, but adding evidence that intersects a protected or
forbidden footprint can still turn the target into `exposure_failed`.

Leakage proof uses the protected split as a temporal dilation of the undilated
holdout interval. In row-index terms, the certificate records:

```text
protected_range =
  [base.begin - dilation_left, base.end + dilation_right)
```

where the dilation widths come from the Hx/Hf purge policy or equivalent
runtime source footprints. Forbidden leakage is then a non-empty intersection
between an inherited source-row footprint and that protected range. This keeps
the math simple: coverage is idempotent union over cursor anchors, load is
additive measure over repeated completed anchors, and leakage is protected-set
intersection after holdout dilation. The rule is exposed as
`lattice_leakage_rule_vocabulary` and Hero JSON emits `leakage_rule_vocabulary`;
the current rule is `protected_split_dilation_intersection`, using temporal
morphological dilation and requiring complete checkpoint closure before
witnesses can be accepted.
Hero JSON also emits `leakage_rule_summary`, which makes that invariant
self-checking: one V1 rule, complete closure required, protected range contains
the base split, and witnesses come from labeled causal exposure reachability.

The `certificate_digest` is a deterministic digest over the proof-relevant
certificate fields. It is an audit handle for comparing or referencing a proof
object; it is not a DB key and does not replace the underlying runtime evidence.
The canonical digest text sorts dependency proofs and other set-like summaries
so equivalent proof objects do not get different digests only because an
unordered proof vector was emitted in a different order.
Hero JSON also emits `derived_query_rule_vocabulary`, a read-only
Datalog-style vocabulary over the evaluator's derived relations:
`identity_match`, `dependency_satisfied`, `all_dependencies_satisfied`,
`coverage_satisfied`, `all_coverage_satisfied`, `closure_complete`,
`checkpoint_ancestor`, `unresolved_lineage`, `closure_clean_if_checked`,
`forbidden_overlap`, `no_forbidden_overlap`, `warning_triggered`,
`stale_cache`, `plan_deficit`, `proof_certificate_check_passed`,
`no_blocking_deficit`, `status_satisfied`, and `target_satisfied`. These rules
name the proof fields and fail-closed policies that evaluate/plan projections
and `hero.lattice.inspect subject=derived` use, but they do not execute waves and do not
replace runtime files as evidence authority. Rule rows declare
`projection_scope`, so clients can distinguish target-wide predicates from
per-row and compact aggregate relations before evaluation, and
`projection_quantifier`, so clients can distinguish `single`, `per_row`, `all`,
`any`, and `none` semantics without parsing relation names.
They also declare `empty_projection_policy`, so zero-row `all`, `any`, and
`none` projections can be audited without relying on implicit vacuous truth.
Hero JSON also emits `derived_query_rule_vocabulary_digest_schema` and
`derived_query_rule_vocabulary_digest`, the same stable digest reported by live
`derived_query_results`, so clients can compare the declared query vocabulary
from `inspect subject=target mode=explain` with evaluate/plan projections.
Hero JSON also emits `derived_query_projection_semantics_vocabulary`, which
defines the allowed quantifier, empty-policy, and compatibility-alias values
used by rule vocabulary and live result rows.
`evaluate operation=target` and `evaluate operation=deficit` additionally emit
`derived_query_results`, a compact read-only projection of those relation truth
values from the existing proof object, warning summary, deficits, plan basis,
and status. It is a query view only: it is excluded from certificate digests,
cannot satisfy readiness, and is not a scheduler or DB source of truth.
The `derived_query_results` envelope declares
`schema=kikijyeba.lattice.derived_query_results.v1`,
`rule_vocabulary_digest_schema`, and `result_projection_digest_schema`, and
includes `projection_target_id`, `projection_target_spec_fingerprint`,
`projection_certificate_digest`, `projection_status`,
`projection_basis_complete`, `projection_basis_field_count`,
`rule_vocabulary_digest`, `result_projection_digest`, `result_count`,
`known_count`, `unknown_count`, `known_true_count`, `known_false_count`,
`aggregate_projection_count`, and `single_projection_count`, so clients can
sanity-check the projection set before walking individual rows. The basis check
requires target id, target spec fingerprint, certificate digest, and status
before the projection can self-check cleanly. It names
`summary_source=emitted_result_rows` and reports `summary_self_check_passed`,
`summary_issue_count`, and `summary_issues` for count/quantifier, empty-policy
compatibility, vocabulary-coverage, basis, and projection-semantics consistency.
It
also reports
`rule_vocabulary_relation_count`, `result_covers_rule_vocabulary`,
`unique_result_relation_count`, `missing_rule_relation_count`,
`extra_result_relation_count`, and `duplicate_result_relation_count`, plus
`missing_rule_relations`, `extra_result_relations`, and
`duplicate_result_relations`, so clients can detect and diagnose stale, extra,
missing, or non-unique Hero projections relative to the declared rule
vocabulary.
Rows whose rules are parameterized, such as `dependency_satisfied(T,D)`,
`coverage_satisfied(T,U)`, `forbidden_overlap(T,W)`, `warning_triggered(T,W)`,
and `plan_deficit(T,D)`, may be emitted as compact live booleans. Live rows
therefore include `rule_projection_scope` for the declared rule shape and
`rule_projection_quantifier` for the declared rule quantifier; they also include
`result_projection_scope` and `result_projection_quantifier` for the emitted
compact projection. `projection_scope` and `projection_quantifier` are kept as
compatibility aliases for the result fields. Live rows also carry
`rule_empty_projection_policy`, `result_empty_projection_policy`, and the
compatibility alias `empty_projection_policy`, plus `projection_row_count`,
`projection_true_count`, `projection_false_count`, and
`projection_row_source` so clients can audit which dependency, coverage,
warning, leakage-witness, plan-deficit, or fixed-premise rows were summarized by
the compact boolean and how zero summarized rows are interpreted.
Hero JSON also emits `db_cache_policy_vocabulary`, which records the DB/cache
boundary directly: runtime files and fact sidecars remain the source of truth,
cache rows are rebuildable read models over derived relations, cached plans are
advice rather than evidence, checkpoint ids/digests are cache keys only when
they validate against runtime files, and Runtime Hero remains the only
executor.
Hero JSON also emits `evidence_abstraction_vocabulary`, which names the
abstract-interpretation step from concrete runtime files and fact sidecars into
proof summaries: identity, coverage/load, checkpoint closure, leakage, node
support, source-receipt audit, warnings, planning, and evidence order. Each
entry lists concrete inputs, abstract outputs, the soundness condition,
conservative policy, and join semantics that a future DB/cache must preserve.
Hero JSON also emits `product_evidence_state_vocabulary`, which names the
factorized proof space: identity, coverage lattice, exposure-load monoid,
closure graph, leakage predicate, metric/warning vector, node-support matrix,
source-receipt audit set, and deficit vector. Each factor records its partial
order, identity-scoped join semantics, proof fields, clean-growth rule, and
target-status effect.
Hero JSON also emits `target_numeric_dimension_vocabulary`, the unit/bounds
vocabulary behind the target compiler's numeric checks. It names DSL contexts,
fields, units, numeric kinds, lower/upper bounds, integrality, and threshold
direction for readiness thresholds, warning thresholds, node-support metrics,
representation-health metrics, and opt-in representation-geometry gates.
Hero JSON also emits `target_numeric_dimension_summary`, the compact read-side
self-check for that vocabulary. It proves declared units/kinds, unique
context/field pairs, well-formed bounds, known threshold directions, required
V1 rows, and separate coverage/load units.
Hero JSON also emits `evidence_retention_policy_vocabulary`,
`evidence_retention_audit_scenario_vocabulary`, and
`evidence_retention_policy_summary`. These classify reports, sidecars,
checkpoints, source receipts, selection signals, proof certificates, cache rows,
human receipts, and archive manifests by retention role before any pruning or
compaction is allowed. Compact receipts, PASS files, proof certificates, and
cache rows remain audit/read-model metadata; runtime reports, sidecars,
checkpoint material, and selection-signal evidence remain the replay authority.
Pruning must refuse or warn when required checkpoint lineage would become
unresolved, and archive manifests must bind active identity, split policy,
source cursor, graph order, and checkpoint identity.
Hero JSON also emits `benchmark_regression_budget_vocabulary` and
`benchmark_regression_budget_summary`. These define the V3-L performance
budget as finite timing rows over three separate layers: library functions,
long-lived MCP calls with session reuse, and direct CLI calls with process
startup included. Each row names its proof mode (`header_only`,
`watched_file_manifest`, `full_runtime_metadata_digest`, `live_scan`, or
`live_parity`), required measurement, regression guard, and whether a full live
scan is allowed. Header-only fast audit rows explicitly forbid live scans and
metadata digests, and no benchmark row grants target-satisfaction authority to
cache rows.
Hero JSON also emits `monotonicity_invariant_vocabulary`, which makes the
clean-growth boundary explicit. Target satisfaction is an upper set only under
clean evidence growth; forbidden overlaps, stale identity, checkpoint mismatch,
new blocking deficits, and warning/order tradeoffs are separate weakening or
incomparable dimensions rather than scalar penalties.
Hero JSON also emits `proof_obligation_vocabulary`, mapping certificate fields
to derived relations, required scopes, status effects, deficit kinds,
non-claiming conditions, certificate-digest participation, and planner
relevance. This lets clients inspect which parts of a certificate are required
for target satisfaction and which are visibility/advisory surfaces.
Hero JSON also emits `proof_certificate_digest_policy_vocabulary`, which is the
machine-readable boundary for the `certificate_digest`: target/split identity,
identity proofs, dependency bindings, coverage/load proofs, closure causal
graph fields, leakage witnesses, checkpoint-preview audit fields, and MDN
node-support summaries are canonicalized into the digest. The digest field
itself, warnings, deficits, plan advice, evidence-order projections, and policy
vocabulary surfaces are emitted beside the proof and do not contribute to the
digest.
Hero JSON also emits `proof_certificate_digest_policy_summary`, a compact
self-check proving the digest-boundary row counts, advisory/report exclusion,
and visibility-only non-status policy.
Hero JSON also emits `checkpoint_identity_policy_vocabulary`, which states the
V1 checkpoint identity boundary explicitly: closure authority is still
path/exposure-fact based; `checkpoint_id` and `checkpoint_file_digest` are
audit/index previews; exact validation bindings are runtime-loaded checkpoint
paths; and digest/id promotion is future DB/cache work, not protocol contract
identity.
Each evaluation also includes a `proof_certificate_check` result. The check
requires the certificate digest, recomputes it, and verifies the proof envelope
and internal proof arithmetic, including split-policy identity for concrete
named-split facts and causal exposures. Unsatisfied results may still pass this
local check when the certificate is a well-formed non-claiming envelope; target
failure belongs in `status`, `reasons`, and `deficits`, not in malformed proof
content. If a would-be satisfied target has a malformed certificate, evaluation
fails closed
with a `certificate:proof_certificate_check` deficit instead of a generic status
deficit. The check
validates certificate schema, target fingerprint presence, duplicate dependency rejection, dependency
kind/status vocabulary, upstream dependency non-binding semantics, satisfied
checkpoint-source path completeness, required identity field presence, identity
match booleans, exact non-vacuous checkpoint dependency bindings, coverage use/fraction
dimensions, positive coverage requirement thresholds, non-vacuous coverage target ranges, coverage/missing interval complements, additive exposure-load
arithmetic, finite load/effort/support measures, coverage/load mutation-filter agreement, load component-scope
vocabulary, target-component-local coverage load scope, duplicate coverage obligation rejection,
target-component assembly identity in mutated coverage causal support,
mutated coverage causal-closure support and contributing-interval multiplicity,
runtime evaluation coverage as non-mutating evidence outside checkpoint closure
whose input checkpoint edges exactly match the satisfied evaluated-checkpoint
dependency and whose output checkpoint edge is empty,
checked-closure root checkpoint producer assembly identity, non-vacuous
complete closure facts, closure fact counts, causal output-checkpoint presence,
causal checkpoint edge-label presence/uniqueness, root-reachable causal output
closure, causal input-edge producer/unresolved resolution and exclusivity,
causal checkpoint self-loop/cycle/root-cycle rejection,
exposure digests, causal exposure identity envelope/component-assembly presence, causal exposure
job/wave identity, causal exposure use labels/source-footprint/completed-range presence,
causal source-key window audit recomputation, including inferred affine key-step
consistency,
unchecked-closure and unchecked-leakage non-claiming semantics, closure
incompleteness witness presence/uniqueness/reachability,
non-vacuous leakage base range, protected split dilation, named-split leakage proof split-policy identity,
leakage forbidden-use membership, leakage witness mutation requirements,
leakage proof complete-closure backing, leakage witness closure membership,
leakage witness job/wave identity, leakage witness causal-field consistency,
leakage witness set recomputation from causal closure overlaps, non-empty leakage witness intersections,
duplicate leakage overlap witness rejection,
valid-target uncertainty support totals, no-trials uncertainty non-claiming, and
valid-target fraction/Wilson interval bounds. For MDN certificates it also verifies
node-support summary schema, MDN-only target-component scope, inherited causal
MDN exposure use/mutation/split/assembly backing for mutating node support,
non-mutating validation/evaluation node support as row-parent exposure evidence,
row-count ordering when row totals are available,
duplicate node-support summary rejection, non-empty node-support summaries,
finite-fraction claim/non-claim semantics, aggregate and weakest-node
fraction/Wilson arithmetic/non-claiming, weakest-node identity, aggregate mean
bounds, support-row identity/order/fraction/use-incidence checks and aggregate
recomputation, mutating support-row parent-exposure closure backing, and balance-metric
presence/bounds/recomputation. A
failed self-check is a certificate construction bug, not a target failure mode.

Exposure-load summaries also carry a small uncertainty view for target support
when runtime evidence provides both `valid_target_count` and a finite
`valid_target_fraction`. The lattice derives the implied opportunity count,
reports the aggregate valid-target fraction estimate, and attaches a Wilson 95%
interval. This is visibility only: readiness still uses completed cursor
coverage, and the uncertainty interval does not become a performance gate.
Hero JSON exposes `valid_target_uncertainty_summary` beside the vocabulary so
agents can report that boundary without reverse-engineering field names.

Evaluations expose an `evidence_order_vector` as a Pareto-order basis over the
separate proof dimensions. It reports target satisfaction, certificate-check
health, identity matches, minimum unique coverage, maximum repeated cursor-load,
closure and leakage booleans, aggregate and weakest-node support confidence when
available, node-support imbalance maxima, source-key audit count, source-key
audit issue count, affine source-key mismatch count, blocking deficit count,
unresolved lineage count, total warning-result count, triggered warning count,
unavailable warning-measurement count, and the compatibility `warning_count`
alias for triggered warnings. These warning dimensions are projected from
`warning_summary`, so clients can inspect aggregate warning state without
recounting every warning row. The lattice intentionally does not collapse these
dimensions into one score: two evaluations can be incomparable when one has more
coverage and another has fewer warnings or better lineage. Selection-signal
leakage is a participation blocker before dominance is considered. The C++
helper `compare_lattice_evidence_order_vectors` returns `left_dominates`,
`right_dominates`, `equivalent`, `incomparable`, `selector_leaked`, or
`unavailable` using this Pareto relation. Hero JSON includes the same
`comparison_relation`, `comparison_helper`, and `relation_vocabulary` metadata
beside the vector so agents can report the partial-order semantics without
guessing from field names. The relation vocabulary is also available in C++ as
`lattice_evidence_order_relation_vocabulary`. Hero JSON also includes
`dimension_order` metadata and a structured `dimension_vocabulary` generated
from `lattice_evidence_order_dimension_vocabulary`:
booleans are true-is-stronger, coverage/support confidence are
higher-is-stronger, source-key audit issues, source-key affine mismatches,
triggered warnings, and unavailable warning counts are lower-is-stronger, total
source-key audit and warning-result counts are visibility-only, and repeated
cursor-load is higher-is-stronger only as clean load. If higher load arrives with
more warnings, the comparator reports incomparability rather than hiding the
warning behind a scalar score. The compatibility `warning_count` field remains
an alias for `triggered_warning_count`; comparison uses the structured
`triggered_warning_count` dimension.
`hero.lattice.compare` evaluates two targets with one scan and returns
the two vectors, per-dimension comparison rows, clean-checkpoint participation
reasons, and the dominance relation. It is read-only, not target proof,
non-dispatchable, not a Runtime executor, and emits explicit false authority
flags for model selection, checkpoint selection, quality acceptance, policy
gates, allocation, market readiness, scalar scores, and deployment decisions.
This Pareto basis is explanatory unless a future selector explicitly opts into
it. V1 `latest_satisfying` does not use Pareto dominance to pick checkpoints.
`checkpoint_selection_policy_summary` exposes that as a countable invariant.

Unsatisfied evaluations also return a `deficits` vector. A deficit is the
planning-side dual of the certificate: it names the missing or contradicted proof
dimension, such as `identity`, `certificate`, `dependency`, `model_state`,
`artifact`, `coverage`, `closure`, `leakage`, or `metric`, carries a stable
`key = kind:dimension`, and includes the required/actual value when the failure
has a numeric shape. Missing active contract/component/graph/source identity
inputs and stale runtime evidence with mismatched contract/component/graph/source
identity are reported as `identity:*` deficits. Scalar readiness failures such
as optimizer effort, valid-target fraction, finite loss, and node-head counts are
reported as `metric:*` deficits instead of generic status failures. Missing runtime
manifests/reports/checkpoints, exposure ledgers, closure facts, active
exposure facts, target-component-local exposure facts, producer exposure facts,
split policies, and non-completed job status are reported as `artifact:*`
deficits. Cyclic target dependency graphs are reported as
`dependency:target_cycle`, while missing loaded model-state inputs use
`model_state:*`.
Malformed proof envelopes are reported as
`certificate:proof_certificate_check`. Validation
evaluation evidence with missing, wrong, or extra checkpoint input edges is
reported as `model_state:input_checkpoint`; evidence that mutates component
state or runs optimizer steps is reported as `model_state:mutated_component`;
evidence that writes checkpoint/model state is reported as
`model_state:output_checkpoint`, so read-only violations do not collapse into a
generic status deficit. Each deficit
also carries a deterministic `priority` and
`priority_class`; lower priority values mean the proof obligation is more
fundamental for planning explanation.
Coverage deficits also expose the missing cursor intervals inside the target
range, so an agent can report the uncovered proof obligation without guessing.
Deficits are explanatory only. They do not dispatch waves, change `MAX_WAVES`
accounting, or replace the evaluator status.

Evaluations also include `plan_basis`. This is the compact planning explanation
derived from the deficits and the suggested wave: it records whether planning
advice is available, the proof dimensions the wave is meant to address, any
missing cursor intervals for coverage failures, and the suggested action string.
`plan_basis.deficit_keys` and `plan_basis.deficit_messages` are the unique,
order-preserving projection of plan-relevant `deficits[].key` and
`deficits[].message`; `plan_basis.deficit_priority_classes` is the matching
unique projection of plan-relevant deficit classes. They are not a second proof
source.
`plan_basis.primary_deficit_key` / `primary_deficit_message` /
`primary_deficit_priority` / `primary_deficit_priority_class` identify the
lowest-priority plan-relevant deficit, making the deficit vector a deterministic
planning explanation without turning Lattice Hero into a scheduler.
Hero JSON includes `deficit_priority_vocabulary` beside `plan_basis` using the
same lower-is-primary order, so clients can inspect the priority classes instead
of relying on external docs.
`plan_basis.available` is true only when `plan_ready` is true and a concrete
suggested wave exists; exhausted planning budgets can still expose deficits, but
must not advertise an available plan. Even when unavailable, `plan_basis` keeps
the relevant deficit keys/messages so an agent can explain why the target is
unsatisfied without implying that another wave is currently recommended. It is
explanatory only. Runtime Hero remains the executor, and runtime reports remain
the proof that a suggested model-state input was actually loaded.
Hero JSON emits `plan_advice_scope_policy_vocabulary`, which makes the planning
boundary machine-readable: `plan_basis`, `suggested_wave`, and `PLAN_INPUT_*`
are advisory projections from deficits; `PLAN_MAX_ATTEMPTS`/`MAX_WAVES` are
recommendation guards; none of these surfaces is evidence authority, contract
identity, scheduler authority, or a Lattice execution path. `PLAN_INPUT_*`
remains a symbolic source-target hint until Runtime Hero executes a wave and the
runtime report proves the exact checkpoint path that was loaded.
Hero JSON also emits `deficit_vector_planning_summary`, a compact self-check for
the planning algebra: ordered deficit classes, advisory plan surfaces, no
evidence or contract-identity authority, no Lattice executor, and the explicit
Runtime Hero executor boundary.
Only coverage deficits populate `plan_basis.target_range` and
`plan_basis.missing_intervals`; leakage source-row witnesses stay in `deficits`
and the proof certificate so they are not mistaken for cursor ranges to train.

The closure proof is intentionally a labeled causal summary, not only a list of
paths. Each `causal_exposure` says which runtime job produced a checkpoint, what
component and use labels were involved, which input checkpoints were loaded, and
which graph-anchor/source-row intervals the fact carries. This keeps the current
checkpoint identity authority tied to an inspectable reachability argument.
Hero JSON emits the same `causal_edge_vocabulary` so
clients can render closure as a labeled causal graph without inferring edge
types from field names.
Hero JSON also emits `causal_edge_summary`, which makes the edge-family boundary
self-checking: source-row uses, checkpoint load/create reachability, component
mutation, and no direct leakage role for checkpoint reachability edges.
Hero JSON also emits `selection_signal_policy_vocabulary`, which records that
known `selection_signal` uses are leakage-relevant when forbidden and that V2
exposes structured model-selection event provenance as read-only evidence.
Hero JSON also emits `selection_signal_policy_summary`, the compact self-check
for the known forbidden-use row, causal-anchor source footprint, read-only
selector-event facts, known selector-path fail-closed behavior, closure
completeness, and no executor authority.

Exposure facts carry graph order identity and job status. A job only counts as
mutating a component when it completed a `train` wave, recorded optimizer effort,
and its manifest explicitly lists the target component in `mutated_components`.
Coverage is not credited from the requested anchor range alone. V1 exposure
facts carry `coverage_precision`; coverage checks use `completed_anchor_range`
only when the fact is marked `contiguous_completed_range_v1`. Requested ranges
without enough completion evidence are marked `requested_range_untrusted_v0` and
do not satisfy readiness coverage.

Lattice Hero is the read-only operating surface for this layer. It currently
loads lattice targets from the active global config, builds an in-memory
exposure ledger from runtime artifact files, evaluates/plans targets, previews
facts, and inspects checkpoint closures. It intentionally does not execute
waves.

## Operative Notes

For day-to-day operation, treat the lattice as an inspector over runtime
evidence:

```text
Runtime writes what happened.
Exposure facts say what data was seen and how it was used.
Checkpoint closure says what lineage a checkpoint inherits.
Targets say what must be true.
Lattice Hero reads, explains, evaluates, and plans.
Runtime Hero executes.
```

The active global config points to one target DSL and one split DSL:

```text
hero.lattice.targets.dsl
hero.lattice.splits.dsl
```

There is one active file of each kind per config in v0, but each file may carry
multiple blocks. `hero.lattice.targets.dsl` contains many
`LATTICE_TARGET` declarations plus reusable profiles, dependencies,
requirements, forbids, and plan clauses. `hero.lattice.splits.dsl`
contains one split policy and many named graph-anchor cursor ranges such as
`train_core`, `validation_holdout`, and `test_holdout`.

Agent runbook:

1. `hero.lattice.status`
   Always start here. Report target/split paths, split-policy fingerprint,
   fact counts, warnings, and inferred active identity before making claims
   about target state. This is a read-only, non-dispatchable status packet, not
   target proof or Runtime execution, writes no evidence, and its fact counts
   are catalog evidence rather than `TARGET_KIND` declarations. It also declares
   no model-selection, performance-selection, policy-gate, allocation,
   market-readiness, or deployment authority.
2. `hero.lattice.inspect subject=targets`
   Use this to discover the target ids compiled from the active target DSL. It
   is read-only, not target proof, non-dispatchable, not a Runtime executor, and
   not a fact-family target-kind expansion path.
3. `hero.lattice.inspect subject=target mode=explain`
   Use this before editing or evaluating a target. It is the source of truth for
   what the compiled proof obligation means without scanning runtime evidence,
   including derived query vocabulary, proof obligation vocabulary, numeric
   dimension vocabulary and summary, proof digest policy, checkpoint selection
   policy,
   plan-advice policy, contract identity boundary vocabulary, mathematical
   readiness crosswalk, operational V1 scope/gate crosswalks, static
   mathematical policy vocabularies, deficit priority and evidence-order
   vocabularies, non-blocking warning trigger policy, interval-scope policy
   vocabulary, and per-warning resolved scope previews. An unbounded warning
   preview means no anchor-range filter is applied for that warning kind. It is
   read-only explanation, not a live target proof or dispatch authority.
4. `hero.lattice.inspect subject=exposure`
   Use this before evaluating unfamiliar runtime roots. Report warnings
   verbatim. The preview ledger is derived from `job.manifest`, `job.state`,
   reports, checkpoints, and lattice fact sidecars. It includes
   `anchor_domain_health` so agents can see whether the graph-anchor cursor was
   heavily clipped by source availability before interpreting readiness. For
   MDN jobs it also previews derived `node_exposure` facts so agents can inspect
   per-node target support without making shared-encoder claims for VICReg.
5. `hero.lattice.evaluate operation=target`
   Use this when the question is "is this target satisfied?" A satisfied
   train-core node-MDN target proves the exact loaded representation checkpoint
   has compatible upstream VICReg producer lineage. A validation evaluation
   target proves run-mode `evaluation_metric` coverage over the validation
   split with zero optimizer steps, no component mutation, exact evaluated
   checkpoint input edges, and no new training checkpoint. For exposure-backed
   targets, the result also includes
   `exposure_summaries` with unique coverage, repeated
   cursor-epoch load, and optimizer-step density. Its envelope marks
   `target_proof=true` and `target_proof_engine=lattice_target_evaluator`, while
   still declaring `dispatchable=false`, `runtime_executor=false`, and
   `writes_evidence=false`.
6. `hero.lattice.evaluate operation=deficit`
   Use this when the question is "what proof deficit remains, and what
   target-authored wave would address it?" Treat the returned wave as advice
   only. It does not execute anything. The result carries the same proof
   certificate/check and deficits as evaluation, plus `plan_basis`, so the
   suggested wave is auditable against the proof deficits it is intended to
   resolve. Its envelope marks `target_proof=true`, `plan_advice_only=true`,
   `dispatchable=false`, and `runtime_executor=false`.
7. `hero.lattice.evaluate operation=latest_satisfying_checkpoint`
   Use this when a target-deficit result carries a symbolic
   `latest_satisfying:<target_id>` model-state hint. Lattice resolves the hint
   only when the source target is cleanly satisfied and its checkpoint closure
   is complete. It reports `source_target_class` and
   `checkpoint_selectable_source_target`; artifact/evaluation sources fail
   closed with `resolution_status=non_checkpoint_target_class`. The tool is
   read-only, declares `target_proof=false`, `dispatchable=false`, and
   `fact_families_are_not_target_kinds=true`, does not rank alternatives, and
   does not execute or select a fallback checkpoint.
8. `hero.lattice.compare`
   Use this to compare two clean satisfying checkpoint targets by Pareto
   evidence vector. The tool reports each vector, per-dimension dominance
   reasons, explicit incomparable/unavailable/selector-leaked states, and the
   boundary that `latest_satisfying` remains separate. It never emits a scalar
   lattice score or deployment decision.
9. `hero.lattice.inspect subject=checkpoint mode=closure`
   Use this for checkpoint lineage. The tool accepts either a checkpoint path
   or `checkpoint_id` plus `checkpoint_file_digest`. Missing upstream producer
   evidence or id/digest mismatch must fail closed and should be reported as a
   blocker, not ignored. The JSON reports closure authority and root checkpoint
   identity. It is read-only and declares `target_proof=false`,
   `dispatchable=false`, `checkpoint_selector=false`,
   `automatic_checkpoint_selection=false`, and
   `fact_families_are_not_target_kinds=true`.
10. `hero.lattice.inspect subject=derived`
   Use this when the question is "show the proof witnesses for one named
   derived rule." The V3-E pilot supports `target_satisfied`,
   `checkpoint_ancestor`, `forbidden_overlap`, `stale_cache`, and
   `unresolved_lineage`. Each result includes the versioned rule row, result
   source, concrete witnesses, and fail-closed fields. It is read-only and
   declares `target_proof=false`, `dispatchable=false`,
   `checkpoint_selector=false`, `automatic_checkpoint_selection=false`, and
   `fact_families_are_not_target_kinds=true`: cache/index rows are never used
   for target satisfaction, and Runtime Hero remains the executor.

For an empty runtime root, graph-anchor targets cannot infer active identity.
Pass `protocol_contract_fingerprint`, `graph_order_fingerprint`,
`source_cursor_token`, and the component assembly fingerprints explicitly when
evaluating/planning, or first run a Runtime Hero dry-run to create
identity-bearing runtime artifacts.

Agent invariants:

```text
MUST NOT execute waves from Lattice Hero.
MUST report Lattice Hero warnings and blocked/exposure_failed reasons verbatim.
MUST provide active identity explicitly when evaluating an empty runtime root.
MUST use Runtime Hero, not Lattice Hero, for execution.
SHOULD prefer inspect subject=target mode=explain before changing target DSL.
SHOULD prefer OVER_SPLIT, PROTECT_SPLIT, and CURSOR_EPOCHS when authoring.
```

## Operational Readiness V1 Gate

Lattice Operational Readiness V1 means the lattice is ready to serve as the
read-only evidence authority for a compiled graph-first contract. It does not
mean the lattice is a scheduler, a performance judge, or a DB-backed audit
system. Hero JSON emits `operational_readiness_v1_scope_vocabulary`, which is
the machine-readable summary of the V1 included claims, deferred work, and
non-authority boundaries. It also emits `operational_readiness_v1_scope_summary`,
a compact self-check that those 12 rows partition into 6 included read-only
claims and 6 deferred items, with zero runtime-executor/performance/DB authority
and no empty claim-boundary fields. It also emits
`operational_readiness_v1_gate_vocabulary`, the concrete V1 acceptance
checklist: read-only authority, active identity binding, VICReg train-core
readiness, MDN train-core readiness, validation/test leakage guards, validation
evaluation exact checkpoint binding, warnings visibility, MDN node-support
visibility, and Hero proof/plan/closure inspection.
It also emits `operational_readiness_v1_gate_summary`, a compact self-check that
the 10 V1 gates are required, read-only, non-executing, non-performance,
non-DB-source-of-truth, have pass/fail conditions and Hero surfaces, and
reference all five required V1 targets.

The V1 gate is satisfied only when these checks are true for the active
contract/runtime root:

```text
1. Identity
   Graph-anchor readiness targets require protocol contract fingerprint, graph
   order fingerprint, source cursor token, and the relevant component assembly
   fingerprint. Stale contract, graph, cursor, or assembly evidence cannot
   satisfy a target.

2. Readiness
   vicreg_train_core_ready and channel_mdn_train_core_ready are
   satisfied from completed normal runtime evidence, not debug-only reports.
   MDN readiness proves the exact representation checkpoint loaded by the MDN
   report. The migrated channel target `vicreg_train_core_ready` is a V2/channel
   readiness target and should remain unsatisfied until channel VICReg runtime
   evidence exists.

3. Holdout cleanliness
   channel_mdn_train_core_no_validation_leakage and
   channel_mdn_train_core_no_test_leakage inspect the checkpoint closure from the
   train-core MDN target and fail closed on protected split overlap. Split
   protection expands holdout footprints by Hx/Hf purge policy before checking
   source-row exposure overlap.

4. Validation evaluation
   channel_mdn_validation_eval_ready is satisfied only by run/evaluation evidence
   over validation_holdout. It must prove evaluation_metric coverage, zero
   training mutation, the exact MDN checkpoint selected by the no-test leakage
   guard, and the matching representation checkpoint lineage; the local
   evaluation exposure witness must also carry those checkpoint input edges and
   no output checkpoint edge. It is not a model performance or deployability
   claim; Hero emits
   `validation_performance_scope_policy_vocabulary` to make that boundary
   machine-readable, plus `validation_performance_scope_policy_summary` to
   self-check the no-V1-performance-gate boundary.

5. Warnings
   Source-domain health, repeated cursor exposure load, and optimizer effort
   density are visible as summaries and non-blocking warning results. Warnings
   do not change status, planning, or max-attempt accounting.

6. Node support
   MDN node_exposure previews show routed, active, trained, and evaluated row
   support plus valid targets and mean NLL by node. Node-support summaries add
   aggregate support, weakest-node, Wilson support-bound, and balance metrics
   over those facts.
   They are MDN support facts, not VICReg per-node readiness claims; Hero emits
   `node_support_scope_policy_vocabulary` to make that V1 scope boundary
   machine-readable, plus `node_support_scope_policy_summary` to self-check the
   no-VICReg-readiness/no-synthetic-backfill boundary.

7. Hero surface
   `hero.lattice.status`, `hero.lattice.inspect subject=targets`,
   `hero.lattice.inspect subject=target mode=explain`,
   `hero.lattice.inspect subject=exposure`,
   `hero.lattice.evaluate operation=target`,
   `hero.lattice.evaluate operation=deficit`,
   `hero.lattice.evaluate operation=latest_satisfying_checkpoint`, and
   `hero.lattice.inspect subject=checkpoint mode=closure` are enough for an
   agent to understand evidence, failure
   reasons, warnings, closure completeness, and the checkpoint identity
   authority used by closure, plus the next suggested wave without executing
   anything. Config/catalog surfaces declare `target_proof=false`; evaluation and
   deficit surfaces declare `target_proof=true` but remain read-only,
   non-dispatching, non-executing proof evaluations.
```

DB/index work, performance gates, stable checkpoint ids, structured source
receipt facts, first-class selection-signal events, and VICReg node-indexed
support facts are intentionally outside this V1 gate. Compact `source_file_receipts`
remain V1 audit metadata only. V2 may derive audit-only
`kikijyeba.lattice.source_receipt.v1` rows from them, but row-index intervals
remain the V1 coverage and leakage authority. V2 may also derive read-only
`kikijyeba.lattice.selection_signal.v1` rows from known selection-signal
exposures; those rows explain selector lineage but do not make V1 a performance
or deployment-selection gate. V2 may derive aggregate and node-indexed
read-only `kikijyeba.lattice.representation_support.v1` rows for shared VICReg
support visibility when components emit honest per-node support denominators.
The same boundary is present in `operational_readiness_v1_scope_vocabulary` so
agents do not need to infer V1 scope from prose. The gate-level pass/fail
conditions are present in `operational_readiness_v1_gate_vocabulary` so agents
can inspect the V1 acceptance checklist without parsing this section.

The V1 acceptance receipt is recorded in
`src/tests/fixtures/kikijyeba/lattice_operational_readiness_v1/PASS.md`. That
file is an audit receipt for the completed train-core/validation evidence chain;
it is not a portable runtime fixture and it does not make `.runtime` artifacts
part of the source tree.
When the target language feels too low-level, prefer the calmer authoring layer:

```text
OVER_SPLIT = train_core;
PROTECT_SPLIT = validation_holdout;
CURSOR_EPOCHS = 0.95;
```

The compiler lowers those fields into the same explicit proof clauses used by
the evaluator. Do not bypass the proof layer by treating targets as schedules or
campaigns; a target may recommend a wave, but it must not dispatch it.

The lattice DB is a rebuildable index/cache over durable evidence, not the
source of truth. Runtime writes immutable files first: `job.manifest`,
`job.state`, component reports, checkpoints, `lattice.exposure.fact`, and
`lattice.checkpoint.fact`. The V2 cache materializes a line-oriented
`kikijyeba.lattice.runtime_index_cache.v1` read model over scanned exposure,
node, checkpoint, source-receipt, selection-signal, and representation-support
rows. Cache validity is tied to a runtime-file metadata digest, and
`hero.lattice.inspect subject=index mode=status` reports whether it is using valid cache rows or a
live scan fallback. Target evaluation still uses live runtime evidence; a stale
or missing cache never upgrades readiness. Its top-level JSON declares
`read_only=true`, `target_proof=false`, `dispatchable=false`,
`runtime_executor=false`, `db_source_of_truth=false`,
`checkpoint_selector=false`, and `fact_families_are_not_target_kinds=true`.
V3-A adds `hero.lattice.inspect subject=index mode=query`, a read-only audit query surface over the
same rows. It filters by relation, exact key, key substring, exact digest, or
digest prefix, and reports cache/live-scan parity. By default, a stored cache
is used for the query only when metadata validation passes and its row answers
match a fresh live scan. It declares the same non-proof, non-dispatchable,
non-selector boundary as `inspect subject=index mode=status`. V3-B adds `hero.lattice.evaluate operation=targets` for one-scan
multi-target evaluation, long-lived watched-file-guarded scan reuse, internal
cache `row_set_digest`/`relation_counts` integrity checks, and streaming
checkpoint file digesting. `validation_strength=watched_file_manifest` checks
only runtime artifacts that can affect index rows; `validation_strength=header_only`
skips the runtime metadata walk for explicit structural inspection.
`allow_unproven_cache=true` with `compare_live_scan=false` enables an explicit
unproven audit-cache fast path. That path reports
`cache_used_unproven_for_audit_query=true`, keeps
`cache_valid_for_audit_query=false`, and still never affects target
satisfaction. Missing, stale, or mismatched proof-mode cache rows fall back to
live scan.
The `runtime_metadata_digest` and `watched_file_metadata_digest` fields are
metadata freshness digests: they hash the relevant path, size, and mtime
records, not file contents. They are cache invalidation hints for read-model
freshness, not content-proof authority. Runtime reports, sidecars, checkpoint
facts, and target evaluation remain the proof source of truth.
`db_cache_policy_vocabulary` is the Hero-facing version of that rule. A stale,
missing, identity-mismatched, or digest-mismatched cache must be rebuilt from
runtime files before cache rows can be reported as current.
V3-L adds `benchmark_regression_budget_vocabulary` and
`benchmark_regression_budget_summary` so performance receipts stay bounded.
Future benchmark reports must separate library function timings, long-lived MCP
session timings, and direct CLI timings; every row must name the proof mode it
is measuring. The explicit header-only audit-cache fast path is allowed to be
fast because it is marked unproven and non-authoritative, while full metadata,
live-scan, and parity rows are allowed to be slower because their proof cost is
named.

DEV_WARNING: the current graph-first protocol contract fingerprint still
includes some Jkimyei training selectors. Runtime model-state fields such as
loaded checkpoint paths are excluded from contract identity and must be proven
through job reports, exposure facts, and checkpoint lineage.
