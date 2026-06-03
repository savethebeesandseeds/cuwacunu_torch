# Config Hero

The fresh Hero migration currently has Config Hero, Runtime Hero, Lattice Hero,
and Marshal Hero.

Each Hero binary has two entry modes:

- MCP stdio server mode for Codex/tool clients.
- Direct one-shot mode with `--tool <name> --args-json <json>` for local smoke
  tests and scripts.

Build:

```bash
make -C /cuwacunu/src/main/hero build-config-hero
make -C /cuwacunu/src/main/hero build-runtime-hero
make -C /cuwacunu/src/main/hero build-lattice-hero
make -C /cuwacunu/src/main/hero build-marshal-hero
```

Direct smoke:

```bash
/cuwacunu/.build/hero/hero_config.mcp \
  --global-config /cuwacunu/src/config/.config \
  --tool hero.config.status \
  --args-json '{}'

/cuwacunu/.build/hero/hero_runtime.mcp \
  --global-config /cuwacunu/src/config/.config \
  --tool hero.runtime.status \
  --args-json '{}'

/cuwacunu/.build/hero/hero_lattice.mcp \
  --global-config /cuwacunu/src/config/.config \
  --tool hero.lattice.status \
  --args-json '{}'

/cuwacunu/.build/hero/hero_marshal.mcp \
  --global-config /cuwacunu/src/config/.config \
  --list-tools-json
```

MCP schema hygiene is a harness-safety gate. Tool catalogs emitted by the
Config, Runtime, Lattice, and Marshal Heroes validate each `inputSchema` before
`--list-tools-json` or JSON-RPC `tools/list` returns it: the root schema must
have `type=object`, and top-level `oneOf`, `anyOf`, `allOf`, `enum`, and `not`
are rejected with a message naming the tool, schema path, and offending
construct. The focused smoke
`make -C src/tests/bench/kikijyeba/runtime run-test_hero_mcp_schema_compat -j12`
checks the generated Config, Runtime, Lattice, and Marshal catalogs. This
protects Codex/MCP harnesses;
it is not a lattice proof condition and does not affect runtime execution
policy.

The default policy paths are `[HERO].config_hero_dsl_path`,
`[HERO].runtime_hero_dsl_path`, `[HERO].lattice_hero_dsl_path`, and
`[HERO].marshal_hero_dsl_path` in
`src/config/.config`, falling back to
`/cuwacunu/src/config/hero.config.dsl` and
`/cuwacunu/src/config/hero.runtime.dsl` and
`/cuwacunu/src/config/hero.lattice.dsl` and
`/cuwacunu/src/config/hero.marshal.dsl`.

Marshal Hero has a minimal policy DSL for symmetry only. Its preferred
operator-facing surface is small:

```text
hero.marshal.status
hero.marshal.prepare
hero.marshal.inspect
```

Marshal is a deterministic coordination surface over explicit Lattice target
state/advice and Runtime Hero policy/wave snapshots. Marshal exposes no
low-level compatibility tools: advice lookup, target-dispatch preparation,
validation, dry-run dispatch, execution gating, receipt replay, and batch
preview are internal C++ methods used by the high-level tools where needed.
Marshal does not prove target satisfaction and does not execute waves by
itself; execution handoff still goes through Runtime Hero.

For operator visibility, use `hero.marshal.inspect`. Its `subject=run` modes
(`latest_chain`, `training_state`, `single_job`, and `compare`) read Runtime
jobs and reports, quote Lattice target statuses when requested, and return
compact operator packets with a summary, stop reason, runtime/lattice/audit
panels, current state, job chain, explicit chain summary, target blockers,
warnings, suspicious items, comparison deltas, and next safe actions. Compare
mode exposes config-identity, wave/range, metric, checkpoint, warning, and
proof-context deltas without choosing a winner. `subject=target` quotes
`hero.lattice.target_deficit` status and only points at certificate inspection
when Lattice returned certificate material. `subject=protocol` has report and
strict identity modes; report mode is observed-only and strict mode requires
explicit expected identity. `subject=spawn` and `subject=component` group
Runtime evidence only. Detailed job rows, checkpoint rows, target-status maps,
performance panels, checkpoint-lineage details, and metrics are returned only
with `include_machine_payload=true`. It is read-only and does not prepare or
execute a wave.

For operator-facing target pursuit, use
`hero.marshal.prepare`. It accepts one `target_id` and an explicit
`drive_mode`:

```text
one_step
budgeted
```

`one_step` is the compact single-move packet. `budgeted` is a finite target
driver controlled by `driver_policy.max_waves`, optional wall-clock budget,
Runtime policy, and the existing Marshal execution gate. Both modes expose
`operator_summary`, `stop_reason`, `wave_panel`, `runtime_panel`,
`lattice_panel`, and `audit_panel` before detailed machine fields. Marshal
queries the
current Lattice target-advice surface, materializes `latest_satisfying` hints
through its read-only Lattice callback, validates the materialized request,
checks Runtime policy/wave alignment, and emits a target-driver ledger. Runtime
still owns dry-run/execution, and Lattice remains the only target-satisfaction
authority.

For Lattice artifact-readiness targets and fact-family evidence, use
`hero.marshal.inspect`. It is read-only and calls Lattice
`evaluate_target`, `fact_summary`, `scan_facts`, `fact_lineage`, or
`fact_preview`; Marshal does not dispatch, dry-run Runtime, select checkpoints,
or become proof authority from this panel.
Failed artifact proofs are summarized with failed-proof count,
proof-template bound/unbound count, proof kind, proof-template claim,
identity-mismatch count, lineage-unbound count, authority-drift count,
integrity flags, authority flags, explicit boundary denial flags for target
dependencies, Runtime waves, Marshal reachability, checkpoint sources, and plan
checkpoint inputs, issue codes, artifact deficit keys, related fact-integrity
issue codes, and primary deficit key so operators inspect evidence rather than
chase a generic certificate failure.
Fact panels relay Lattice's native `fact_integrity_summary` with declared,
bound, unresolved, identity-mismatch, and digest-mismatch relation counts plus
affected families and issue codes. This makes unresolved fact lineage visible in
the evidence panel before it becomes a target-proof blocker. The same field is
native to `hero.lattice.scan_facts`, `hero.lattice.fact_summary`, and
`hero.lattice.fact_lineage`; `hero.lattice.fact_preview` adds concrete
family-row inspection by digest, digest prefix, or fact index while preserving
the same audit-only contract. Clients do not need Marshal to see relation
integrity. Lattice fact-catalog tools expose top-level read-only,
non-target-proof, non-dispatchable, non-runtime-executor, non-writing, and
non-decision flags so fact-family names do not imply target kinds, Marshal
reachability, model selection, allocation, market-readiness, or deployment
authority. Their catalog summaries also expose zero-count decision-authority
fields for quality/performance authority, checkpoint selection, allocation,
execution, market/deployment authority, policy gates, target dependencies,
Runtime waves, Marshal reachability, checkpoint sources, and plan checkpoint
inputs, plus `decision_authority_clean=true`. Registry and catalog-summary rows
also expose `fact_identity_contract`, the shared runtime evidence identity
envelope: schema/fact-type/digest, parent exposure digest when applicable,
protocol/contract/graph/source-cursor/split/component/job-wave identity,
row-index anchor/completed intervals, lineage digest fields, and support
fields. It keeps row-index intervals authoritative, source-key windows
audit-only, and target-kind, Runtime-wave, Marshal reachability, and policy-gate
authority false. `hero.lattice.fact_lineage` adds
selected runtime-index relation/key/digest rows as audit-only lineage witnesses;
those rows do not satisfy targets or create Marshal reachability.
`hero.lattice.fact_preview` returns concrete rows plus matching lineage rows
with `facts_used_for_target_satisfaction=false` and `checkpoint_selected=false`.
Those preview rows now include `identity_envelope`, a normalized typed Lattice
catalog projection of the common fact identity contract. Hero serializes that
projection with active identity, row-index ranges, parent digests, support
counters when present, and no proof/dispatch/selection authority.
Marshal fact panels now include that compact lineage view by default, with
`include_lineage=false` available only to suppress the audit panel; callers can
request `include_preview=true` with a fact family and optional digest/index for
the concrete-row view.
Artifact target panels also relay Lattice `fact_preview_hint` entries as
`artifact_fact_preview_*` fields, so a failed proof can point to the exact
catalog row to inspect without making Marshal a proof authority or selector.
Fact panels also relay Lattice catalog boundary fields for artifact-readiness:
proofable families, proof kinds, proof-template claims, promotion-blocked
families, blocked reasons, and warning-summary-only families. Marshal keeps
those as read-only catalog facts and does not turn blocked or proofable fact
families into targets by itself.
Disabled Lattice policy-gate reservations are also relayed in Marshal target
and prepare panels as read-only context; run-level target blockers preserve the
same context, including Lattice's policy-fingerprint verification and mismatch
counters plus policy-input contract completeness and missing-input counts.
Marshal keeps policy-gate dispatch, decision-policy, target-status, and proof
authority false.
Marshal's public status, inspect, prepare, and run-report
surfaces now carry explicit boundary fields such as `target_proof=false`,
`fact_families_are_not_target_kinds=true`, `allocation_decision=false`,
`market_readiness_decision=false`, and `deployment_decision=false`; those fields
prevent evidence inspection from becoming proof, allocation, market-readiness,
or deployment authority by implication.
The same artifact summary is preserved in run-level `target_blockers` when
`hero.marshal.inspect` asks Lattice for bulk target status.
If `prepare` sees `target_class=artifact_readiness`, it stops with
`next_action=inspect` before Runtime dispatch validation or
dry-run preview construction.
Fact-family-only panels never call target proof evaluation and remain
non-dispatchable audit views. Parked `replay_environment` rows, when explicitly
inspected, are audit data only and are not Marshal reachability or active
Lattice proof evidence.

This v2 surface intentionally removes old Config Hero responsibilities that
belonged to retired migration layers:

- no retired identity receipts
- no TSODAO optim file surface
- no Config-owned runtime reset tool
- no default/temp/objective split

Config Hero is now a small policy-controlled config file surface. It can list,
read, write, and delete files under configured roots, while domain-specific DSL
validation remains owned by Ujcamei, Kikijyeba, Wikimyei, and Jkimyei. It can
also describe an exact global config bundle with `hero.config.capture_bundle`:
the receipt records every `_path`/`_filename` entry, canonical file paths,
content digests, a stable `config_bundle_id`, and a per-capture
`config_receipt_id`.

Agent workflow:

1. `hero.config.status` checks policy health.
2. `hero.config.map` finds global config path references and missing files.
3. `hero.config.capture_bundle` records the current config provenance receipt
   when runtime spawn evidence or target proof context needs an exact config
   link.
4. `hero.config.resolve` preflights a target path before read or mutation.
5. `hero.config.read` returns file content plus `sha256`.
6. `hero.config.write` uses `dry_run=true`, then writes with
   `expected_sha256` when replacing an existing file.

`hero.config.delete` also supports `dry_run=true` and requires
`expected_sha256` by default. Direct one-shot CLI calls are useful for smoke
tests; MCP sessions are better for multi-step agent work because in-memory
policy edits from `hero.config.set` stay alive for the session.

Runtime Hero is the agent surface around `/cuwacunu/.build/exec/cuwacunu_exec`.
It does not own model logic; it decodes the active wave, invokes the runtime
executable with policy guards, and reads `job.manifest`, `job.state`, and report
artifacts.

Runtime agent workflow:

1. `hero.runtime.status` checks policy, executable availability, and active
   wave intent.
2. `hero.runtime.wave` decodes `kikijyeba.settings.wave.dsl`. Canonical
   VICReg/MDN targets report channel job kinds; old node MDN targets are not
   active.
   The experimental
   `wikimyei.representation.encoding.mtf_jepa_mae_vicreg` target reports the
   separate `channel_representation_mtf_jepa_mae_vicreg` job kind. It is
   representation-only and does not reuse the production VICReg job identity.
3. `hero.runtime.dry_run` builds and validates the protocol contract through
   `cuwacunu_exec --dry-run` and returns job artifacts.
4. `hero.runtime.list_jobs` and `hero.runtime.get_job` inspect prior job
   directories.
5. `hero.runtime.replay_from_job` runs the Kikijyeba replay adapter against an
   already completed Runtime job that has
   `artifacts/kikijyeba.environment.replay.v1/runtime_replay_batches.index`.
   It delegates to `cuwacunu_exec --replay-from-job-dir`, writes replay reports
   and `runtime_replay_experiments.index`, and does not launch a new wave or
   mutate model checkpoints.
6. `hero.runtime.read_artifact` can read replay evidence by name:
   `replay_batch_index`, `replay_experiment_index`, and
   `replay_experiment_report`. `hero.runtime.get_job` includes the same replay
   artifact summaries next to the normal manifest/state/report summaries.
   Runtime Hero does not follow parent-directory components in report paths
   declared by `runtime_replay_experiments.index`, so malformed replay indexes
   cannot redirect `replay_experiment_report` reads outside the replay artifact
   tree. Runtime Hero also exposes read-only replay report integrity fields:
   index-declared `report_digest`, computed report-body digest, digest
   bound/match booleans, and whether an indexed report path was rejected. These
   fields are operator visibility; Lattice replay targets remain the proof
   authority.
7. `hero.runtime.dev_nuke` previews and, when explicitly enabled, clears the
   runtime artifact root for developer reset workflows.

`hero.runtime.execute` defaults to dry-run. Non-dry-run execution is denied
unless `allow_execute=true`; MODE=train has the additional
`allow_train_execute=true` guard.
`hero.runtime.replay_from_job` also defaults to the policy `default_dry_run`,
but it is a post-job report adapter rather than a wave executor: non-dry-run
replay is permitted under the default locked policy once the job is completed,
inside an allowed job root, and has the replay batch index.

Marshal handoffs should use the explicit `runtime_handoff` object accepted by
`hero.runtime.execute`. Runtime validates the object against the effective wave
and policy, rejects unresolved `latest_satisfying:*` model-state selectors, and
only then launches `cuwacunu_exec`. Concrete source ranges should be supplied as
`wave_overlay` launch fields rather than baked into reusable wave profiles.
Accepted handoff id/digest values are passed into the Runtime job and echoed in
`job.manifest`, `runtime.result.fact`, and the derived lattice exposure fact;
Lattice proof certificates then carry the same handoff identity on closure causal
exposures when that evidence participates in checkpoint lineage. The older
`marshal_expected_wave` field remains as compatibility scaffolding.

The checked-in default policy is intentionally locked for Codex/MCP safety:
execute/train remain disabled, while developer reset is available only through
the guarded `hero.runtime.dev_nuke` path with explicit confirmation. Use
`src/config/hero.runtime.train.dsl` or an operator-local equivalent for
intentional training runs; it enables execute/train with confirmation while
keeping developer reset disabled.

`hero.runtime.dev_nuke` also defaults to dry-run. Non-dry-run reset is denied
unless `allow_dev_nuke=true` and, by default, `confirm_dev_nuke=true` is passed.
The checked-in policies allow the disposable `/cuwacunu/.runtime` tree to be
selected for reset, but keep `dev_nuke_backup_enabled=false` so reset does not
create new backup clutter under `.runtime`. Operators can explicitly enable
backups to the configured `/tmp` backup root. The tool refuses roots outside
`allowed_dev_nuke_roots` and refuses execution while nonterminal or
unknown-status `job.state` files are present.

Lattice Hero is the read-only control surface above runtime evidence. It scans
normal job artifacts and lattice fact sidecars into an in-memory exposure
ledger, explains the compiled proof object for
`kikijyeba.lattice.targets.dsl`, evaluates targets, recommends the next wave
when a target is not satisfied, and inspects checkpoint exposure closures. It
does not execute waves. Named train/validation/test ranges live in
`kikijyeba.lattice.splits.dsl`; target evaluation resolves `TRAIN_SPLIT` /
`OVER_SPLIT` from that file before planning and can apply split-level holdout
defaults through `PROTECT_SPLIT`. The future lattice DB should index immutable
runtime/fact files for faster queries; runtime remains the producer of durable
evidence files. Runtime manifests and exposure facts carry
`config_bundle_id`, `config_receipt_id`, `component_spawn_registry_id`,
`component_family_id`, `component_spawn_fingerprint`, the scoped base26
`component_spawn_id`, and `component_spawn_label` as `<family>#<spawn_id>`.
The full fingerprint is the audit authority; the spawn id is only an operator
display and retrieval handle inside its registry/family scope.

Lattice Hero agent runbook:

1. Call `hero.lattice.status` first. Report the active target/split DSL paths,
   split-policy fingerprint, target count, runtime root, fact counts, warnings,
   and any active identity inferred from runtime manifests. Treat it as a
   read-only, non-dispatchable status packet, not target proof or Runtime
   execution; it writes no evidence and has no model, performance, policy-gate,
   allocation, market-readiness, or deployment authority. Fact counts are catalog
   evidence, not `TARGET_KIND` declarations.
2. Call `hero.lattice.list_targets` to discover target ids. V0 has one active
   target file per global config, but that file may contain many
   `LATTICE_TARGET` blocks. The response is a read-only catalog:
   `target_proof=false`, `dispatchable=false`, `runtime_executor=false`, and
   `fact_families_are_not_target_kinds=true`.
3. Call `hero.lattice.explain_target` before editing or evaluating a target. It
   does not scan runtime evidence; it explains the compiled proof obligation,
   profile, dependency, cursor-epoch exposure requirements, split protection,
   checkpoint source, resolved split range, derived query vocabulary, proof
   obligation vocabulary, proof digest policy, checkpoint selection policy,
   plan-advice policy, contract identity boundary, mathematical readiness
   crosswalk, operational V1 scope/gate crosswalks, static mathematical policy
	   vocabularies, deficit priority and evidence-order vocabularies, numeric
	   dimension vocabulary, warning policy vocabularies, and per-warning resolved
	   scope previews. An unbounded warning preview means no anchor-range filter is
	   applied for that warning kind. Artifact-readiness definitions expose
	   `target_surface_kind=evidence_catalog_artifact`,
	   `dispatchable_target=false`, `runtime_wave_dispatchable=false`, and
	   `recommended_operator_action=inspect` before any runtime
	   scan. They also emit `kind=not_applicable`,
	   `target_kind_applicable=false`, and `target_kind_effective=none` so the
	   internal default enum cannot masquerade as a declared `TARGET_KIND`.
	   Artifact-readiness targets cannot declare `UPSTREAM_TARGET_ID`,
	   `LATTICE_DEPENDS`, `EVALUATED_CHECKPOINT_SOURCE`, or `PLAN_INPUT_*`
	   checkpoint hints; artifact lineage comes from fact parent digests and proof
	   templates rather than target-dependency scheduling. The explanation
	   response is not live proof: `target_proof=false`,
	   `dispatchable=false`, and `runtime_executor=false`.
4. Call `hero.lattice.scan_exposure` before evaluating unfamiliar runtime
   roots. Runtime roots contain job directories with `job.manifest`,
   `job.state`, component reports, Runtime terminal facts
   (`runtime.result.fact`, `runtime.checkpoint_io.fact`,
   `runtime.health_measurement.fact`), checkpoints, `lattice.exposure.fact`,
   Runtime-emitted
   `lattice.source_analytics.fact` source-health and optional
   source-data-analytics visibility, forecast evidence sidecars, and
   `lattice.checkpoint.fact`. The scan is read-only, non-dispatchable, not a
   target proof, not a Runtime executor, and its fact families are not
   `TARGET_KIND` values. Report scan warnings verbatim. Fact previews
   include `anchor_domain_health`:
   candidate/accepted graph anchors, skip
   reasons, common/reference key bounds, and source-domain warning level. Fact
   previews also include `source_key_window` and structured
   `source_key_window_audit` metadata; incomplete, non-numeric, internally
   non-monotone, row-order-inverting, or affine-inconsistent source-key windows
   produce scan warnings while row-index footprints remain the leakage authority.
   The audit object exposes parsed endpoints, order-preservation booleans, and
   inferred key-step/affine-consistency fields, gap-warning counts, and
   row/source-key mismatch counts so agents can inspect the auxiliary source-key
   coordinate map without parsing warning text. `scan_exposure` also emits
   `source_key_map_audit_summary`, binding audits to graph-order identity,
   source-cursor identity, and source-receipt parent facts while preserving
   row-index authority.
   MDN jobs also produce derived
   `node_exposure` previews from per-node report fields, including
   routed/active/trained/evaluated row counts, valid targets,
   valid-target opportunity counts, and mean NLL.
   `node_support_summaries` add support-row and unique-node counts, support-row
   incidence counts by exposure use and mutation, aggregate support,
   weakest-node, Wilson support-bound, and support-balance metrics including CV,
   Gini, and normalized entropy over those MDN facts. They also expose ordered
   `support_rows` with parent exposure fact digests plus the use flags and
   per-node row/valid-target counts used to derive those metrics. Treat
   valid-target denominators as
   target-opportunity counts, not plain row counts, and do not treat routed rows
   alone as Wilson denominators. This lets agents inspect node-level target
   support without assuming per-node VICReg support.
   Hero JSON also exposes `node_support_scope_policy_vocabulary`, recording
   that V1 node-support rows are MDN-head support visibility only, future
   shared-representation node support must be emitted by NodeLift or
   representation facts, and synthetic backfill from MDN rows is not allowed.
   Hero JSON also exposes `node_support_scope_policy_summary`, self-checking
   that boundary with four policy rows, no VICReg per-node readiness authority,
   no synthetic backfill, and no runtime executor authority.
   `scan_exposure` also exposes `representation_support_facts` and
   `representation_support_summary` when representation reports or NodeLift
   `.lls` sidecars emit aggregate shared-encoder support payloads. These rows
   bind to the parent representation exposure digest, inherit protocol identity,
   and remain visibility-only: they do not become per-node VICReg readiness and
   they do not reuse downstream MDN node rows.
   VICReg
   exposure previews may include `representation_health` with invariance/
   variance/covariance loss components, gradient norm, valid projection rows,
   adapter valid-channel fraction, finite-parameter check, and optional
   geometry metrics such as embedding dimension, effective rank/fraction,
   min/max dimension variance, condition number, and isotropy; treat these as
   visibility fields unless a target explicitly gates on them with
   `LATTICE_REQUIRES KIND=representation_geometry`.
   MTF-JEPA-MAE-VICReg jobs should emit the same compact identity/lineage
   fields plus representation-health facts such as finite loss/parameters,
   finite gradients, sample/channel support, valid latent rows, split
   JEPA/MAE-time/MAE-frequency/TF/VICReg loss means, TF-pair support, VICReg
   row counts, context-starvation counters, target EMA distance, and geometry
   summaries. Lattice must not ingest raw embeddings, token tensors, per-token
   masks, unbounded histograms, or downstream forecast claims from this
   representation family.
5. Call `hero.lattice.evaluate_target` to ask whether a target is satisfied by
   evidence. Call `hero.lattice.target_deficit` when the user asks what proof
   deficit remains or which target-authored wave would address it; it returns
   the same proof certificate/check plus any suggested wave, and the wave is a
   recommendation only. Both responses are read-only proof-engine outputs:
   `target_proof=true`, `target_proof_engine=lattice_target_evaluator`,
   `dispatchable=false`, `runtime_executor=false`, and `writes_evidence=false`.
   `target_deficit` additionally declares `plan_advice_only=true`.
   Exposure-backed target results
   include `exposure_summaries`: unique cursor coverage, repeated cursor-epoch
   load, and optimizer-step density. These are visibility metrics, not
   automatic failures. Validation evaluation targets use `evaluation_metric`
   coverage from run-mode jobs, so they can be satisfied without optimizer
   mutation or a new checkpoint. Targets may also return
   `warning_results` from `LATTICE_WARN`; the target-level `warnings` list now
   projects typed non-blocking warning envelopes, while `warning_messages`
   preserves the ordered human-readable projection of triggered
   `warning_results` messages. Missing warning measurements are reported as
   unavailable with their configured threshold while remaining non-blocking.
   Each warning result carries `warning_family`, `source=lattice`,
   `component=lattice_target`, `blocking=false`,
   `readiness_effect=non_blocking_warning_only`, `evidence_digest`,
   `machine_reason_code`, `human_explanation`, `suggested_inspection_panel`,
   `target_ids_observed_against`, and its `evidence_basis`, such as
   `exposure_load_summary`,
   `filtered_node_support_summary`, `representation_health_facts`, or
   `anchor_domain_facts`, and marks which summary envelope is available with
   `exposure_summary_available` and `node_support_summary_available`. Each
   catalog warning is bucketed by its diagnostic role: forecast calibration,
   forecast support, forecast-baseline comparison, target-transform contract,
   observer/allocation consistency, lineage integrity, selection-signal audit,
   replay-environment diagnostics, and source health remain typed warning
   families, not implicit readiness gates.
   Each warning kind may use `SPLIT` or explicit `ANCHOR_INDEX_BEGIN` /
   `ANCHOR_INDEX_END`; measurements are scoped to that warning interval and
   default to the target range. If trusted completed coverage is unavailable,
   warning overlap can use anchor-range visibility without making that evidence
   satisfy readiness coverage. Hero JSON exposes the resolved interval as
   `warning_results[].warning_anchor_range`; `hero.lattice.explain_target`
   exposes the same pre-evaluation scope as
   `warning_scope_previews[].resolved_warning_anchor_range`; and
   `warning_anchor_scope_policy_vocabulary` describes the visibility-only
   fallback for both surfaces.
   evaluation also returns `warning_summary`, which aggregates warning-result
   count, triggered-warning count, unavailable-warning count, clear measured
   warnings, blocking and non-blocking warning counts,
   `all_warnings_non_blocking`, the compatibility `warning_count` alias, and
   threshold-relation counts without requiring clients to recount
   `warning_results`.
   Each evaluation also returns a `proof_certificate` with target/split
   fingerprints plus the identity,
   dependency, artifact, coverage, closure, leakage, and warning-adjacent proof
   material used to derive the status. Artifact-readiness targets write
   `proof_certificate.artifacts[]` rows for fact-family proofs such as
   `target_transform_contract_bound`, `forecast_baseline_artifact_bound`,
   `forecast_eval_artifact_bound`, `observer_belief_artifact_bound`, and
   `allocation_artifact_bound`; these rows are lineage/completeness proofs, not
   forecast-quality gates, checkpoint selectors, allocation recommendations, or
   runtime dispatch authority. The rows expose explicit false authority flags
   for quality/performance, checkpoint selection, allocation/execution,
   market/deployment, policy gates, target dependencies, Runtime waves, Marshal
   reachability, checkpoint sources, plan checkpoint inputs, model-state
   mutation, raw-potential tradable-return claims, and replay execution, plus
   `fact_schema`, `fact_type`, `fact_identity_contract_bound`,
   `fact_identity_envelope_complete`, `row_index_interval_authority`, and
   `source_key_window_audit_only` fields that show the row's source fact is
   bound to the catalog identity contract. `proof_template_bound` and
   `proof_template_claim` show that the row's `proof_kind` is bound to the
   registered artifact proof template for its fact family. If an
   identity-matched artifact fact exists but fails, the failed evaluation keeps
   the first matching `proof_certificate.artifacts[]` row with `passed=false`,
   the drift flags, and the issue list. Deficits are derived from that failed
   row with artifact-specific keys such as `artifact:forecast_eval_authority`, so
   the primary operator action remains `inspect subject=facts` rather than a
   generic certificate failure. Artifact lineage digests are resolved only
   against fact rows that match the target identity, so a transform, baseline,
   selection-signal, forecast, or observer digest from another split/cursor
   cannot satisfy the proof. Fact families without an explicit artifact proof
   template, including `source_analytics`, remain warning/summary evidence and
   cannot become artifact-readiness targets by spelling a new `PROOF_KIND`.
   Disabled `LATTICE_POLICY_GATE` reservations are exposed separately from proof
   status. Each reservation carries a policy input contract with metric and
   baseline definitions, threshold, uncertainty policy/model, support minimum,
   selector split, anti-leakage policy, tie policy, negative tests, calibration
   requirements, holdout declaration, and threshold-selection audit. Hero JSON
   reports required inputs, missing inputs, input-contract completeness, and
   `decision_policy_authority=false`; policy gates remain review metadata until
   enabled policy authority is implemented separately.
   The certificate includes a deterministic
   `certificate_digest` over its proof content for
   audit/reference. The digest
   canonicalizes dependency proofs and other set-like summaries before hashing.
   Leakage proof records the protected split, the undilated split interval, the
   Hx/Hf-dilated protected interval, and the resulting left/right dilation
   widths. When leakage is detected, it also includes overlap witnesses with
   fact digest, forbidden use, source footprint, and intersection interval.
   Treat the leakage certificate as protected-set intersection after temporal
   dilation: `protected = [base.begin - left, base.end + right)`. The local
   certificate check recomputes that interval before accepting the witnesses.
   Hero JSON exposes `leakage_rule_vocabulary`; the current rule is
   `protected_split_dilation_intersection`, a temporal morphological dilation
   over a complete checkpoint closure with labeled causal-exposure witnesses.
   Hero JSON also exposes `leakage_rule_summary`, the compact self-check for the
   single V1 dilation rule, complete-closure requirement, and causal witness
   basis.
   Hero JSON also exposes `derived_query_rule_vocabulary`, a read-only
   Datalog-style vocabulary for the evaluator's identity, dependency,
   aggregate dependency, coverage, aggregate coverage, closure, guarded
   closure-cleanliness, leakage, guarded no-overlap, warning, planning,
   certificate-check, blocking-deficit, and satisfaction relations. It also
   names the raw `status_satisfied` enum predicate used by final satisfaction.
   Treat it as query/cache metadata only; Runtime Hero remains the only
   executor. Rule rows include `projection_scope`, `projection_quantifier`, and
   `empty_projection_policy`, making target-wide, per-row, compact aggregate,
   all-row, any-witness, none-exists, and zero-row relation shapes discoverable
   before evaluation. They also declare the emitted result shape with
   `result_projection_scope`, `result_projection_quantifier`, and
   `result_empty_projection_policy`, so the live self-check compares result rows
   against the vocabulary instead of a private hardcoded shape table.
   `derived_query_rule_vocabulary_digest_schema` and
   `derived_query_rule_vocabulary_digest` bind the declared rule vocabulary and
   are repeated in live `derived_query_results` so clients can compare
   explain/evaluate/plan surfaces without recomputing the canonical text.
   `derived_query_projection_semantics_vocabulary` defines the allowed
   projection-scope, quantifier, empty-policy, and compatibility-alias values
   used by rule and result rows.
   `evaluate_target` and `target_deficit` also include
   `derived_query_results`, a read-only projection of those relation truth
   values from the existing proof certificate, warning summary, deficits, plan
   basis, and status. It is not separate evidence and does not affect target
   status or certificate digests.
   Artifact-readiness evaluations and target deficits also include
   `fact_integrity_summary` scoped to the target subject family. Unresolved or
   mismatched catalog relations may appear as `fact_integrity:*` deficit rows
   beside artifact proof issues, but they remain inspection context rather than
   Runtime dispatch, policy, quality, or allocation authority.
   Artifact proof deficits include `related_fact_integrity_issue_codes` when
   their proof issue matches concrete catalog issue rows, preserving the link
   from a proof issue such as `baseline_fact_digest_not_found` to the exact
   job/component-prefixed fact-integrity issue code.
   `hero.lattice.derived_query` is the V3-E standalone pilot for asking one
   named relation for concrete witnesses. It supports `target_satisfied`,
   `checkpoint_ancestor`, `forbidden_overlap`, `stale_cache`, and
   `unresolved_lineage`; each response reports the rule row, result source,
   witnesses, and fail-closed flags while keeping cache rows non-authoritative
   for target satisfaction. Its top-level JSON declares `target_proof=false`,
   `dispatchable=false`, `checkpoint_selector=false`,
   `automatic_checkpoint_selection=false`, `db_source_of_truth=false`, and
   `fact_families_are_not_target_kinds=true`.
   The envelope declares
   `schema=kikijyeba.lattice.derived_query_results.v1`,
   `rule_vocabulary_digest_schema`, and `result_projection_digest_schema`, and
   includes `projection_target_id`, `projection_target_spec_fingerprint`,
   `projection_certificate_digest`, `projection_status`,
   `projection_basis_complete`, `projection_basis_field_count`,
   `result_projection_digest_rule_vocabulary_bound`,
   `result_projection_digest_basis_field_count`, `rule_vocabulary_digest`,
   `result_projection_digest`, `result_count`, `known_count`, `unknown_count`,
   `known_true_count`, `known_false_count`, `aggregate_projection_count`, and
   `single_projection_count` for whole-set sanity checks. The result projection
   digest binds the rule-vocabulary schema and digest plus target id, target
   spec fingerprint, certificate digest, status, and emitted rows; the digest
   basis field count is `6`. The basis check requires target id, target spec
   fingerprint, certificate digest, and status before the projection can
   self-check cleanly.
   It names
   `summary_source=emitted_result_rows` and reports
   `summary_self_check_passed`, `summary_issue_count`, and `summary_issues`
   for count/quantifier, projection scope, declared rule/result
   projection-shape compatibility, empty-policy compatibility,
   vocabulary-coverage, basis, and projection-semantics consistency.
   It also reports `rule_vocabulary_relation_count`,
   `result_covers_rule_vocabulary`, `unique_result_relation_count`,
   `missing_rule_relation_count`, `extra_result_relation_count`,
   `duplicate_result_relation_count`, `missing_rule_relations`,
   `extra_result_relations`, and `duplicate_result_relations`, so agents can
   detect stale, extra, missing, or non-unique projections relative to the
   declared rule vocabulary.
   Live result rows include `rule_projection_scope` and
   `rule_projection_quantifier` for the declared relation shape, plus
   `result_projection_scope` and `result_projection_quantifier` for the emitted
   compact projection. `projection_scope` and `projection_quantifier` remain
   compatibility aliases for the result fields. Compact rows also expose
   `rule_empty_projection_policy`, `result_empty_projection_policy`,
   `empty_projection_policy`, `projection_row_source`,
   `projection_row_count`, `projection_true_count`, and
   `projection_false_count`, making empty, all-pass, partial, and any-witness
   summaries auditable without reinterpreting status text.
   Hero JSON also exposes `db_cache_policy_vocabulary`, recording that runtime
   files/facts are the source of truth, DB/cache rows are rebuildable read
   models, cached plans are not evidence, checkpoint ids/digests are preview
   cache keys until v2, and Runtime Hero remains the only executor.
   V3-A adds `hero.lattice.index_query`, a read-only audit query surface over
   rebuildable index rows. It filters by relation/key/digest, reports stored
   cache parity against live scan, falls back to live scan on missing/stale/
   mismatched cache rows, and never upgrades target satisfaction. Interactive
   callers can opt into `validation_strength=header_only` plus
   `allow_unproven_cache=true` and `compare_live_scan=false` for a fast
   read-only audit query; the response marks that as
   `cache_used_unproven_for_audit_query=true`, not proof. `index_status` and
   `index_query` both declare `target_proof=false`, `dispatchable=false`,
   `checkpoint_selector=false`, `automatic_checkpoint_selection=false`,
   `db_source_of_truth=false`, and `fact_families_are_not_target_kinds=true`.
	   V3-B adds `hero.lattice.evaluate_targets` so agents can evaluate several
	   readiness targets through one runtime scan. The batch response has the same
	   read-only, non-dispatching proof-engine envelope as `evaluate_target`.
	   Runtime index caches also carry
   `watched_file_metadata_digest`, `row_set_digest`, and `relation_counts`;
   `validation_strength=watched_file_manifest` is a bounded freshness check,
   while `header_only` remains the explicit fastest unproven inspection mode.
   Runtime metadata digests are metadata freshness checks over path, size, and
   mtime records. They are not content digests and do not make cache rows target
   proof authority.
   Hero JSON also exposes `evidence_abstraction_vocabulary`, the
   abstract-interpretation vocabulary that maps concrete runtime/fact inputs
   into abstract proof outputs with soundness, conservative, and join semantics.
   Hero JSON also exposes `product_evidence_state_vocabulary`, the product
   proof-space vocabulary for identity, coverage, load, closure, leakage,
   warnings, node support, source-receipt audit, and deficits, including
   partial orders and identity-scoped join rules.
   Hero JSON also exposes `join_law_vocabulary`, the algebraic law table for
   those joins: idempotent coverage/closure/leakage/warning/deficit joins,
   additive repeated-load and distinct node-support joins, duplicate policies,
   cache requirements, and fail-closed unsafe joins.
   Hero JSON also exposes `node_support_scope_policy_vocabulary`, making the
   MDN-only node-support scope and future shared-representation support boundary
   explicit.
   Hero JSON also exposes `node_support_scope_policy_summary`, the compact
   self-check for that MDN-only/no-backfill boundary.
   Hero JSON also exposes `representation_geometry_vocabulary`, naming the
   VICReg representation-health and embedding-geometry warning metrics, their
   units, threshold directions, and V1 non-blocking visibility scope.
   Hero JSON also exposes `representation_geometry_summary`, checking that the
   18 VICReg health/geometry metrics remain non-blocking warning visibility and
   do not grant active performance-gate authority.
   Hero JSON also exposes `representation_geometry_gate_review_summary`, which
   reviews observed VICReg geometry distributions, records that no default
   threshold was promoted, and confirms that hard geometry checks are opt-in and
   fail closed when geometry facts are missing.
   Hero JSON also exposes `evidence_retention_policy_vocabulary`,
   `evidence_retention_audit_scenario_vocabulary`, and
   `evidence_retention_policy_summary`. These are the retention/compaction
   guardrails: runtime reports, sidecars, checkpoint material, and
   selection-signal evidence remain replay authority; proof certificates, PASS
   files, compact receipts, and cache rows remain non-authoritative audit or
   read-model metadata.
   Hero JSON also exposes `benchmark_regression_budget_vocabulary` and
   `benchmark_regression_budget_summary`. These are the V3-L performance
   guardrails: benchmark rows are finite, split library-function, long-lived
   MCP, and direct CLI timing layers, and label proof modes as `header_only`,
   `watched_file_manifest`, `full_runtime_metadata_digest`, `live_scan`, or
   `live_parity`. Header-only fast audit rows must not trigger live scans or
   metadata digests; proof/parity rows may be slower, but their cost is named
   separately from the fast path.
   Hero JSON also exposes `performance_uncertainty_policy_vocabulary`,
   recording that future performance gates must use declared uncertainty
   methods and conservative confidence bounds, not raw point estimates, with
   selection-leakage policy in scope.
   Hero JSON also exposes `performance_uncertainty_policy_summary`, checking
   that the six policy rows keep V1 support intervals as visibility, defer five
   future performance-gate rows, disallow point-estimate gates, and require
   confidence bounds plus selection-leakage policy.
   Hero JSON also exposes `validation_performance_evidence_policy_vocabulary`,
   naming the V3-C validation performance evidence contract: metric family,
   split field, checkpoint identity, evaluated checkpoint binding, sample
   count, point estimate, uncertainty method, confidence-bound fields, target
   syntax, selection-leakage policy, and fail-closed cases.
   Hero JSON also exposes `validation_performance_evidence_policy_summary`,
   checking that V3-C has no active performance gate, available point estimates
   are bounded or warning-only, mean NLL remains warning-only until uncertainty
   exists, and missing uncertainty, wrong checkpoint binding, stale identity,
   and selector leakage fail closed.
   Hero JSON also exposes `mathematical_readiness_v1_vocabulary`, a compact
   crosswalk from the mathematical strengthening points to their Hero surfaces,
   proof basis, V1 scope boundary, and future work.
   `mathematical_readiness_v1_summary` is the checked companion: it counts the
   16 V1 mathematical items and reports that all are included, read-only,
   non-executing, non-performance, non-DB-source-of-truth, and backed by at
   least one Hero surface.
   Hero JSON also exposes `mdn_distribution_calibration_vocabulary`, naming the
   warning-only aggregate, channel, target-feature, channel/target-feature,
   per-node, and future distribution-calibration metrics.
   Hero JSON also exposes `mdn_distribution_calibration_summary`, keeping the
   warning-only metric policy visible and confirming that no MDN calibration
   row is a performance gate.
   Hero JSON also exposes
   `mdn_distribution_calibration_diagnostic_vocabulary` and
   `mdn_distribution_calibration_diagnostic_summary`, checking that V3-D
   diagnostics bind exact evaluated MDN checkpoint, representation checkpoint,
   split policy, active identity, validation split, sample count, uncertainty
   method, and warning-only/non-performance effect.
   Hero JSON also exposes `validation_performance_scope_policy_vocabulary`,
   recording that validation eval readiness is clean evaluation evidence, not a
   model-quality or deployability gate.
   Hero JSON also exposes `validation_performance_scope_policy_summary`,
   self-checking that V1 validation eval has no performance-gate authority and
   future performance targets require uncertainty plus selection-leakage policy.
   Hero JSON also exposes `target_numeric_dimension_vocabulary`, the
   unit/bounds/integrality table for numeric target and warning fields.
   Hero JSON also exposes `target_numeric_dimension_summary`, proving that the
   numeric table has declared units/kinds, well-formed bounds, known threshold
   directions, key V1 rows, and separate coverage/load units.
   Hero JSON also exposes `monotonicity_invariant_vocabulary`, which names the
   clean-growth-only monotonicity rule and the weakening/incomparable
   dimensions for leakage, deficits, warnings, Pareto order, and visibility.
   Hero JSON also exposes `proof_obligation_vocabulary`, mapping certificate
   fields to derived relations, status effects, non-claiming conditions, digest
   participation, and planner relevance.
   Hero JSON also exposes `proof_certificate_digest_policy_vocabulary`, making
   the digest boundary explicit: canonical proof content is hashed, while the
   digest field itself, warnings, deficits, advisory plans, evidence-order
   projections, and policy vocabulary surfaces are not.
   Hero JSON also exposes `proof_certificate_digest_policy_summary`, which
   self-checks the digest boundary counts and proves advisory/report surfaces
   have zero digest authority.
   Hero JSON also exposes `checkpoint_identity_policy_vocabulary`, recording
   that V1 checkpoint closure remains path/exposure-fact based, checkpoint
   ids/file digests are preview-only audit metadata, and exact loaded-checkpoint
   bindings use runtime paths.
   Target satisfaction is monotone only under clean evidence growth; forbidden
   or protected-footprint evidence can still turn an otherwise ready target into
   `exposure_failed`.
   Evaluations expose `exposure_measure_algebra_vocabulary`, the shared
   coverage/load algebra vocabulary: unique coverage is an idempotent interval
   union in coverage-fraction units, while repeated exposure load is an
   additive interval measure in cursor-epoch units.
   Evaluations also expose `exposure_measure_algebra_summary`, the compact
   self-check proving the coverage/load algebra partition and unit boundary.
   Evaluations also expose `source_key_coordinate_policy_vocabulary`, recording
   that row-index intervals are authoritative for coverage/leakage and
   source-key windows are audit-only order-preserving/affine/gap map checks.
   Evaluations also expose `source_key_coordinate_policy_summary`, which
   self-checks the coordinate boundary: one row-index coverage/leakage authority
   row, four audit-only source-key rows, declared order-preserving/affine/gap
   fields, and no source-key audit row with coverage or leakage authority.
   Evaluations also expose `source_receipt_policy_vocabulary`, recording that
   compact `source_file_receipts` are audit metadata only: they trace source
   files but do not satisfy coverage, prove leakage cleanliness, alter contract
   identity, or replace row-index footprints. V2 scans derive structured
   audit-only source receipt facts from those compact strings; they remain
   outside V1 readiness authority.
   Evaluations also expose `source_receipt_policy_summary`, the compact
   self-check for one row-index coverage/leakage authority row, three
   audit-only receipt rows, no contract identity authority, no structured
   receipt facts in V1, and declared compact/future structured receipt fields.
   Evaluations also expose `evidence_order_vector`, a Pareto-order basis with
   separate identity, coverage, load, closure, leakage, node-support confidence,
   node-support imbalance, source-key audit issue counts, unresolved-lineage
   count, deficit-count, warning-result count, triggered-warning count, and
   unavailable-warning count dimensions.
   `warning_count` remains the compatibility alias for triggered warnings. Do
   not treat the vector as a scalar score; the warning dimensions are projected
   from `warning_summary`, and two clean evaluations can be
   incomparable across these dimensions. Selection-signal leakage excludes a
   checkpoint before dominance is considered.
   Warning results expose a structured `measurement_available` bit, and the
   unavailable-warning dimension is derived from that bit instead of diagnostic
   message wording.
   Internal callers can use `compare_lattice_evidence_order_vectors` when they
   need an explicit dominance/equivalence/incomparability relation; Hero JSON
   includes the relation vocabulary, dimension-order polarity, and
   `dimension_vocabulary` beside the vector for agent reporting. The vocabulary
   comes from the same C++ source as the comparator, and `warning_count`
   remains a compatibility alias while comparison uses
   `triggered_warning_count`. Repeated cursor-load only strengthens the vector
   as clean load; if it comes with more
   warnings, the relation remains
   incomparable.
   `hero.lattice.compare_evidence` is the read-only V3-F surface for comparing
	   two clean satisfying checkpoint targets. It reports both vectors, the
	   per-dimension dominance rows, participation/exclusion reasons, and the final
	   relation; it is read-only, not target proof, non-dispatchable, not a Runtime
	   executor, and emits explicit false authority flags for model selection,
	   checkpoint selection, quality acceptance, policy gates, allocation, market
	   readiness, scalar scores, and deployment decisions.
	   Hero JSON also exposes `checkpoint_selection_policy_vocabulary`, recording
	   that `latest_satisfying` is deterministic readiness selection over a
	   referenced satisfied target, not a best-model, Pareto, performance,
	   deployment, or scalar-score selector.
	   `hero.lattice.latest_satisfying_checkpoint` now reports the
	   `source_target_class` and `checkpoint_selectable_source_target` gate; calls
	   against `artifact_readiness` or `evaluation_readiness` sources fail closed as
	   `resolution_status=non_checkpoint_target_class`. Its top-level JSON also
	   declares `read_only`, `target_proof=false`, `dispatchable=false`, and
	   `fact_families_are_not_target_kinds=true`.
	   Hero JSON also exposes `checkpoint_selection_policy_summary`, the compact
   self-check for selector counts, exact runtime binding rows, and zero
   performance/Pareto/identity/executor authority.
   Node-MDN certificates include derived `node_support_summaries` when inherited
   MDN node facts are available. `proof_certificate_check` reports local
   self-consistency of the required certificate digest, schema, target
   fingerprint presence, split-policy identity for concrete named-split facts,
   and proof arithmetic. Evaluation filtering rejects concrete named-split
   exposure facts that omit split-policy identity while a split policy is
   active; historical unknown-split facts remain admissible. Unsatisfied targets
   can still have
   `proof_certificate_check.passed=true` when the certificate is a well-formed
   non-claiming envelope; read `status`, `reasons`, and `deficits` for the
   target failure. The check covers duplicate dependency rejection, dependency kind/status vocabulary, upstream dependency non-binding
   semantics, satisfied checkpoint-source path completeness, required identity
   field presence, identity match booleans, exact non-vacuous checkpoint dependency
   bindings,
   coverage use/fraction dimensions, positive coverage requirement thresholds,
   non-vacuous coverage target ranges, interval arithmetic, checked-closure
   checkpoint identity, non-vacuous complete closure facts, closure counts,
   causal output-checkpoint presence, causal checkpoint edge-label
   presence/uniqueness, root-reachable causal output closure, causal input-edge
   producer/unresolved resolution and exclusivity, causal checkpoint self-loop/cycle/root-cycle rejection,
   coverage/load mutation-filter agreement, finite load/effort/support measures,
   load component-scope vocabulary, target-component-local coverage load scope,
   duplicate coverage obligation rejection,
   target-component assembly identity in mutated coverage causal support,
   mutated coverage causal-closure support and contributing-interval
   multiplicity, runtime evaluation coverage as non-mutating evidence outside
   checkpoint closure,
   unchecked-closure and unchecked-leakage non-claiming semantics, causal
   identity envelope/component-assembly/job-wave identity/completed-range presence,
   checked-closure root checkpoint producer assembly identity, closure
   incompleteness witness presence/uniqueness/reachability,
   non-vacuous leakage base range, protected split dilation,
   named-split leakage proof split-policy identity, leakage forbidden-use
   membership, leakage proof complete-closure backing, leakage witness closure
   membership, leakage witness job/wave identity, leakage witness
   causal-field consistency, leakage witness set recomputation from causal
   closure overlaps, leakage witness mutation requirements, non-empty leakage
   intersections, duplicate leakage overlap witness rejection, valid-target uncertainty
   support totals, no-trials uncertainty non-claiming, valid-target Wilson
   bounds, node-support summary schema, MDN-only node-support scope, inherited
   causal MDN exposure use/mutation/split/assembly backing for mutating node
   support, non-mutating validation/evaluation node support as row-parent
   exposure evidence, duplicate node-support summary rejection, non-empty node-support summaries, finite-fraction
   claim/non-claim semantics, aggregate and weakest-node fraction/Wilson
   arithmetic/non-claiming, weakest-node identity, aggregate mean
   bounds, support-row identity/order/fraction/use-incidence checks and
   aggregate recomputation, mutating support-row parent-exposure closure backing,
   balance-metric presence/bounds/recomputation, and node-support row-count
   bounds.
   Unsatisfied evaluations also include `deficits`, a compact list of missing or
   contradicted proof dimensions for agents to report before suggesting a wave.
   Each deficit carries a stable `key = kind:dimension` for planning and
   indexing, plus a deterministic `priority` and `priority_class`; lower
   priority values name more fundamental proof obligations.
   `plan_basis` links any suggested wave back to those proof deficits and
   carries coverage missing intervals when the next wave is addressing an
   uncovered cursor range. Its deficit keys/messages are the unique projection
   of plan-relevant `deficits[]`; `deficit_priority_classes` is the matching
   projection of addressed deficit classes. These are not a second proof source.
   It also exposes the
   primary plan-relevant deficit key/message/priority/class derived from the
   deficit priorities, plus the `deficit_priority_vocabulary` used to interpret
   those numeric priorities.
   `plan_basis.available` is true only when
   `plan_ready` is true and a concrete suggested wave exists; exhausted planning
   budgets can still report deficit keys/messages without advertising an
   available plan. It is explanatory only and does not execute or alter planning
   guards. Leakage source-row witness ranges remain in `deficits` and the proof
   certificate; they are not copied into `plan_basis.target_range`.
   Hero JSON also exposes `plan_advice_scope_policy_vocabulary`, recording that
   `plan_basis`, `suggested_wave`, and `PLAN_INPUT_*` are advisory deficit
   projections only; `PLAN_MAX_ATTEMPTS`/`MAX_WAVES` guard recommendation
   availability; `PLAN_INPUT_*` stays a symbolic source-target hint until a
   runtime report proves the loaded checkpoint path; Runtime Hero remains the
   executor.
   Hero JSON also exposes `deficit_vector_planning_summary`, the compact
   self-check that the priority order and plan advice policies remain
   deterministic, non-evidence, non-identity, and non-executing for Lattice Hero.
   Hero JSON also exposes `operational_readiness_v1_scope_vocabulary`, the
   consolidated V1 scope table for included read-only evidence-authority claims,
   deferred DB/performance/checkpoint-identity/source-receipt/selection-event/
   VICReg-node-support work, and explicit non-authority boundaries.
   V2 `scan_exposure` now emits read-only `selection_signal_facts` and a
   `selection_signal_summary` when runtime evidence contains
   `use_selection_signal=true`; these rows explain selector lineage and selected
   checkpoint paths without becoming coverage, contract identity, execution, or
   performance authority.
   `operational_readiness_v1_scope_summary` self-checks that those 12 scope rows
   partition into 6 included read-only claims and 6 deferred items, with zero
   runtime-executor/performance/DB authority and no empty claim-boundary fields.
   Hero JSON also exposes `operational_readiness_v1_gate_vocabulary`, the
   concrete V1 acceptance checklist covering identity, train-core VICReg/MDN
   readiness, validation/test leakage guards, validation exact-checkpoint
   evaluation, warnings/node-support visibility, and Hero read/plan/closure
   inspection.
   `operational_readiness_v1_gate_summary` self-checks that the 10 gates are
   required, read-only, non-executing, non-performance, non-DB-source-of-truth,
   have pass/fail conditions and Hero surfaces, and reference all five required
   V1 targets.
   Hero JSON also exposes `contract_identity_boundary_vocabulary`, the
   machine-readable split between protocol/graph/component active identity,
   target proof identity, runtime-loaded checkpoint model state, advisory plan
   inputs, warning visibility, and audit-only source receipt/key-window
   metadata.
   `contract_identity_boundary_summary` self-checks that model-state, planning
   advice, warning visibility, and audit metadata have zero overlap with protocol
   contract or target proof identity while the active identity surfaces are
   present.
   Closure proofs include `causal_exposures`, labeled summaries of the inherited
   exposure facts: producer job/wave, component, uses, mutation bit,
   anchor/source-row intervals, source-key window audit, and checkpoint
   inputs/outputs. The certificate check rejects unknown exposure-use labels,
   non-completed producer facts, missing source footprints for observed-input or
   target-supervision labels, and source-key audit summaries that do not
   recompute from the causal source-key window fields, including affine key-step
   consistency. Hero JSON exposes `causal_edge_vocabulary` so clients can treat
   closure as a labeled graph with source-row, loaded-checkpoint,
   created-checkpoint, and mutation edges instead of reverse-engineering field
   names.
   Hero JSON also exposes `causal_edge_summary`, the compact self-check for the
   seven V1 edge labels, checkpoint reachability edges, source-row leakage
   edges, and mutation edge.
   Hero JSON also exposes `selection_signal_policy_vocabulary`, recording that
   V1 treats `selection_signal` as a forbidden causal use over the causal
   exposure anchor footprint while first-class selector event provenance remains
   future work.
   Hero JSON also exposes `selection_signal_policy_summary`, which self-checks
   the four-row boundary: known selection-signal is forbidden and
   causal-anchor based, first-class selector event streams and arbitrary
   selector-path proofs remain future/non-claiming, all rows require closure
   completeness, and Lattice has no executor authority.
   Node-MDN evaluations can include `node_support_summaries` for the target
   component's inherited MDN node facts. `LATTICE_WARN` may flag high MDN node
   support imbalance, low normalized support entropy, weak lowest-node support,
   or low aggregate Wilson lower-bound support. Node-support warnings first
   derive the warning interval's support matrix and then filter support rows by
   `USE` and `EFFECT`, defaulting to `target_supervision` and
   `mutated_component`, and remain non-blocking.
   `LATTICE_WARN KIND=mdn_distribution_calibration` can flag aggregate,
   channel, target-feature, channel/target-feature, and per-node MDN NLL
   diagnostics as warning-only evidence.
   Future distribution-calibration metrics such as PIT KS statistic, predictive
   interval coverage error, tail coverage error, and calibration slope error
   are named but report unavailable until runtime emits samples and
   uncertainty.
   Validation evaluation targets remain readiness proofs: `evaluation_metric`
   coverage means the checkpoint was evaluated over trusted validation anchors,
   not that the observed distributional metrics pass a quality bar.
   `LATTICE_WARN KIND=representation_health` may also flag
   suspicious VICReg health metrics such as high variance/covariance loss, low
   adapter valid-channel fraction, low effective-rank fraction, or high
   condition number; those warnings remain visibility only.
   `LATTICE_REQUIRES KIND=representation_geometry` is the explicit opt-in gate
   form. It is readiness/health evidence, not validation performance, and it
   produces a metric deficit if the worst matching geometry measurement is
   missing or outside the declared bound.
   Coverage proofs use idempotent interval union and expose contributing,
   merged covered, and missing complement intervals; repeated exposure load
   remains an additive visibility summary. Exposure summaries expose these
   algebra and unit labels directly so agents do not have to infer whether a
   number is a coverage fraction, cursor epoch, or anchor-event count. The
   certificate check recomputes the merged union and missing complement, so a
   proof with overlapping covered intervals or non-complement missing intervals
   is invalid even if its scalar totals look plausible. It also checks that
   additive load counts match the contributing interval list before union.
   Warning results expose structured `threshold_direction`,
   `threshold_triggered`, and `threshold_relation` fields next to the numeric
   threshold, so agents can report warning comparisons and trigger state without
   parsing `status` or message text. Hero JSON also includes
   `warning_threshold_relation_vocabulary` for the allowed relation values and
   `warning_anchor_scope_policy_vocabulary` for warning-interval visibility.
   `target_numeric_dimension_summary` is the compact read-side check for that
   table: it counts units, numeric kinds, bounds, integrality, threshold
   directions, duplicate context/field pairs, and required V1 rows.
   Target compilation also rejects dimensionally invalid thresholds: fractions
   must be finite values in `[0,1]`, cursor-epoch and effort-density thresholds
   must be finite and non-negative, and count thresholds must be non-negative
   integral counts. Representation condition-number thresholds must be finite
   values at least `1`. Representation-health warnings additionally reject
   inverted one-sided directions: high-bad metrics such as loss, gradient norm,
   maximum dimension variance, and condition number use `ABOVE`; low-bad metrics
   such as valid projection rows, finite parameter checks, rank, minimum
   dimension variance, and isotropy use `BELOW`. Anchor-domain warning clauses
   carry exactly one metric threshold. Treat these as authoring errors, not
   runtime evidence failures. Opt-in representation-geometry gates use `VALUE`
   with `OP=ge` for low-bad metrics and `OP=le` for high-bad metrics.
   Exposure summaries may include valid-target support uncertainty when runtime
   evidence provides both `valid_target_count` and a finite
   `valid_target_fraction`: inferred success/opportunity counts, an aggregate
   fraction estimate, and a Wilson 95% interval. Report it as support
   visibility only, not as a performance gate. Hero JSON exposes
   `valid_target_uncertainty_vocabulary` with the Wilson method,
   success/opportunity fields, interval fields, and no-trials non-claiming
   policy.
   Hero JSON also exposes `valid_target_uncertainty_summary`, which self-checks
   the three support scopes, Wilson-95/binomial method, field bindings,
   no-trials non-claiming policy, and support-visibility-only boundary.
   Hero JSON exposes `validation_performance_scope_policy_vocabulary` with the
   same boundary for evaluate/plan output; future performance targets need
   explicit metrics, uncertainty, and selection-leakage policy.
   `EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:<target_id>` means the
   evaluation report must prove it loaded that exact MDN checkpoint, with the
   matching representation checkpoint lineage. Suggested waves may include
   symbolic model-state source hints such as `INPUT_MDN_CHECKPOINT`, but
   Runtime Hero remains responsible for resolving/executing them and the
   runtime report remains the proof.
6. Call `hero.lattice.checkpoint_closure` for checkpoint lineage. A complete
   MDN closure should include the MDN exposure fact and the exact upstream
   VICReg checkpoint producer fact. The tool accepts either checkpoint_path or
   checkpoint_id plus checkpoint_file_digest. Missing input lineage or
   id/digest mismatch must fail closed and should be treated as a blocker.
   Report resolution_authority and root checkpoint identity so readers can
   verify id/digest authority. The JSON is explicitly read-only and non-proof:
   `target_proof=false`, `dispatchable=false`, `checkpoint_selector=false`,
   `automatic_checkpoint_selection=false`, and
   `fact_families_are_not_target_kinds=true`.

Read-only guard targets may use
`CHECKPOINT_SOURCE = latest_satisfying:<target_id>`. Treat those as checkpoint
inspection over another target's proven artifact, not as separate training
requests, performance ranking, or Pareto optimization. If the source target is
unsatisfied, the guard target blocks and forwards the source target's suggested
wave.

When the active runtime root is empty, graph-anchor targets cannot infer active
identity. Pass `protocol_contract_fingerprint`, `graph_order_fingerprint`,
`source_cursor_token`, and component assembly fingerprints explicitly to
`evaluate_target` or `target_deficit`, or run a Runtime Hero dry-run first to
produce identity-bearing manifests. Lattice Hero may suggest a wave, but Runtime
Hero remains the only executor.

Agent invariants:

- Lattice Hero is read-only: never use it as an executor or scheduler.
- Runtime Hero is the execution surface for any suggested wave.
- `explain_target` is the source of truth for target language meaning.
- `scan_exposure` is the source of truth for runtime evidence warnings,
  including Ujcamei graph-anchor domain health warnings and derived MDN
  node-exposure previews. It also exposes audit-only source receipt facts and
  summaries; malformed non-empty receipts warn, while missing receipts are
  counted as audit absence.
- `evaluate_target` is the source of truth for exposure-load summaries,
  non-blocking target warnings, and proof deficits. Warning measurements are
  anchor-interval scoped. Repeated exposure is suspicious only when an explicit
  `LATTICE_WARN` policy says it is suspicious.
- Empty runtime roots require explicit active identity for graph-anchor targets.
