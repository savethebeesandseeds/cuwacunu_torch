# Hero Marshal

Marshal is the coordination layer between Lattice advice and Runtime execution.
It does not prove target satisfaction, and it does not relax Runtime Hero
execution policy.

```text
Lattice Hero:
  proves from runtime evidence and emits advice.

Marshal:
  validates advice, builds deterministic dispatch requests, and records audit
  receipts.

Runtime Hero:
  dry-runs or executes allowed waves and writes the evidence that Lattice later
  proves.
```

## Operator Quick Start

Marshal has concrete public Hero tools:

```text
hero.marshal.status
hero.marshal.prepare.train.one_step
hero.marshal.prepare.train.budgeted
hero.marshal.prepare.evaluate.one_step
hero.marshal.prepare.evaluate.budgeted
hero.marshal.rollout
hero.marshal.inspect.run.latest_chain
hero.marshal.inspect.run.training_state
hero.marshal.inspect.run.single_job
hero.marshal.inspect.run.compare
hero.marshal.inspect.target
hero.marshal.inspect.protocol.report
hero.marshal.inspect.protocol.strict
hero.marshal.inspect.spawn.by_id
hero.marshal.inspect.spawn.by_label
hero.marshal.inspect.spawn.by_registry
hero.marshal.inspect.spawn.by_fingerprint
hero.marshal.inspect.spawn.by_job
hero.marshal.inspect.component
hero.marshal.inspect.facts.summary
hero.marshal.inspect.facts.lineage
hero.marshal.inspect.facts.preview
hero.marshal.inspect.facts.preview.by_digest
hero.marshal.inspect.facts.preview.by_digest_prefix
hero.marshal.inspect.facts.preview.by_index
```

The shortest rule is:

```text
status   = what Marshal can see
prepare  = how one Lattice target can move next
rollout  = how one completed Runtime job can be replayed through environment
inspect  = what existing evidence says
```

Use the direct CLI when checking behavior from a shell:

```bash
/cuwacunu/.build/hero/hero_marshal.mcp --help
/cuwacunu/.build/hero/hero_marshal.mcp \
  --global-config /cuwacunu/src/config/.config \
  --list-tools
/cuwacunu/.build/hero/hero_marshal.mcp \
  --global-config /cuwacunu/src/config/.config \
  --list-tools-json
```

Use `--help` for the human runbook. Use `--list-tools-json` when an MCP client
needs exact tool schemas.

### Which Tool To Use

`hero.marshal.status` is the first smoke check. It is read-only and tells an
operator whether Marshal is healthy, what recent receipts are visible, and
whether Marshal is still denying execution/proof/deployment authority.

```bash
/cuwacunu/.build/hero/hero_marshal.mcp \
  --global-config /cuwacunu/src/config/.config \
  --tool hero.marshal.status \
  --args-json '{}'
```

`hero.marshal.prepare.*` is for target pursuit. It starts from one dispatchable
Lattice training/evaluation target, then prepares the next finite Runtime
handoff under policy. The tool name encodes the target intent and driver shape:
`train.one_step`, `train.budgeted`, `evaluate.one_step`, or
`evaluate.budgeted`.

Current dispatchable target movement is the training/evaluation path. Replay is
not a `prepare` responsibility; use `hero.marshal.rollout`. Artifact-readiness
targets are evidence proofs and route to `inspect`. Policy-training remains
reserved until Marshal has an explicit policy-training profile/handoff and must
not become the trainer, optimizer, or Lattice proof authority.

```bash
REQUEST=/tmp/hero_marshal_prepare.kv
printf 'materialize_plan_inputs = true\n' > "$REQUEST"
ARGS='{"target_id":"channel_mdn_train_core_ready","mode":"plan","args_path":"/tmp/hero_marshal_prepare.kv"}'
/cuwacunu/.build/hero/hero_marshal.mcp \
  --global-config /cuwacunu/src/config/.config \
  --tool hero.marshal.prepare.train.one_step \
  --args-json "$ARGS"
```

`hero.marshal.rollout` is for environment replay. It starts from an already
completed Runtime job directory, binds a policy set and Cajtucu paper execution
profile, then prepares or delegates a bounded historical replay through Runtime
Hero.

```bash
REQUEST=/tmp/hero_rollout.kv
cat > "$REQUEST" <<'EOF'
rollout_id = holdout_rollout_v1
rollout_attempt_id = holdout_rollout_v1_attempt_001
idempotency_key = holdout_rollout_v1_attempt_001
runtime_job_dir = /tmp/runtime_job
replay_batch_index_path = /tmp/runtime_job/artifacts/kikijyeba.environment.replay.v1/runtime_replay_batches.index
graph_order_fingerprint = <graph_fingerprint>
asset_universe_digest = <asset_universe_digest>
target_node_ids = USDT,BTC,ETH
policy_set = numeraire,equal_weight,sdu
max_steps = 250
max_parallel_jobs = 4
EOF
ARGS='{"mode":"plan","args_path":"/tmp/hero_rollout.kv"}'
/cuwacunu/.build/hero/hero_marshal.mcp \
  --global-config /cuwacunu/src/config/.config \
  --tool hero.marshal.rollout \
  --args-json "$ARGS"
```

`hero.marshal.inspect.*` is for reading evidence. Use the concrete inspect
tool that matches the evidence panel after Runtime has written jobs, replay
reports, or fact sidecars.

```bash
ARGS='{"runtime_root":"/tmp/cuwacunu_runtime"}'
/cuwacunu/.build/hero/hero_marshal.mcp \
  --global-config /cuwacunu/src/config/.config \
  --tool hero.marshal.inspect.run.latest_chain \
  --args-json "$ARGS"
```

### Safe Workflow

A normal training-oriented control loop is:

```text
hero.marshal.status
  -> hero.marshal.prepare.train.one_step mode=plan
  -> inspect the planned handoff, refusal fields, and next_safe_actions
  -> hero.marshal.prepare.train.one_step mode=dry_run when Runtime dry-run is wanted
  -> hero.marshal.prepare.train.one_step mode=execute only when policy allows it
  -> Runtime writes evidence
  -> hero.marshal.inspect.run.latest_chain
  -> Lattice re-reads evidence and proves or refuses the target
```

A normal environment rollout loop is:

```text
completed Runtime job with replay batches
  -> hero.marshal.rollout mode=plan
  -> inspect rollout plan, policy set, finite limits, and execution profile
  -> hero.marshal.rollout mode=execute when the plan is accepted
  -> Runtime Hero calls the Runtime replay adapter
  -> Runtime writes replay report / trajectory evidence
  -> hero.marshal.inspect.run.latest_chain or hero.marshal.inspect.facts.summary reads the evidence later
```

`prepare` and `rollout` are not synonyms. `prepare` is target-driven and starts
from Lattice. `rollout` is environment-driven and starts from a completed
Runtime job.

### Authority Boundaries

Marshal must keep these statements true:

```text
target_satisfaction_claimed=false
marshal_runtime_executor=false
runtime_handoff_delegated=true only when Marshal invoked Runtime Hero
lattice_proof_authority=false
marshal_policy_training_authority=false
live_execution_authority=false
checkpoint_selected=false
allocation_decision=false
market_readiness_decision=false
deployment_decision=false
```

Runtime executes. Lattice proves. Cajtucu paper-executes replay mechanics under
the Runtime replay contract. Marshal coordinates, delegates, records, and
explains; it does not obtain live execution authority from Cajtucu.

### Evidence Terms

Use these terms consistently:

```text
marshal_handoff_receipt
  Marshal audit record for what Marshal requested or delegated.
  Never target satisfaction.

runtime_report
  Runtime-owned output after execution or replay.
  May contain metrics, trajectories, terminal facts, or replay reports.

lattice_proof
  Lattice-owned proof or refusal from Runtime evidence.

inspection_summary
  Marshal read-only explanation of existing Runtime/Lattice/Marshal evidence.
```

Marshal may quote `lattice_target_status=satisfied` when the value came from
Lattice, but it must still keep `marshal_proves_target=false`.

## D1 Dispatch Advice Schema

D1 defines the deterministic schema and validation rules for consuming lattice
plan advice. It intentionally does not execute or dry-run Runtime Hero.

Implemented surface:

```text
src/include/hero/marshal_hero/hero_marshal.def
src/include/hero/marshal_hero/hero_marshal.h
src/include/hero/marshal_hero/hero_marshal_tools.h
src/include/hero/marshal_hero/marshal/dispatch_advice.h
src/include/hero/marshal_hero/marshal/dispatch_adapter.h
src/include/hero/marshal_hero/marshal/runtime_hero_handoff.h
src/include/hero/marshal_hero/marshal/dispatch_operation.h
src/include/hero/marshal_hero/marshal/execution_gate.h
src/include/hero/marshal_hero/marshal/dispatch_receipt.h
src/include/hero/marshal_hero/marshal/evaluation_marshal.h
src/include/hero/marshal_hero/marshal/rollout_marshal.h
src/include/hero/marshal_hero/marshal/status.h
src/include/hero/marshal_hero/marshal/operational_report.h
src/include/hero/marshal_hero/marshal/batch_preview.h
src/include/hero/marshal_hero/marshal/codex_assist.h
src/include/hero/marshal_hero/marshal/tool_schema.h
src/include/hero/marshal_hero/marshal/tool_handler.h
src/include/hero/marshal_hero/marshal/performance_budget.h
src/main/hero/hero_marshal_mcp.cpp
```

`src/include/hero/marshal_hero` owns the public Marshal Hero tool registry and
MCP-facing facade. `src/include/hero/marshal_hero/marshal` owns the
deterministic dispatch, evaluation, receipt, rollout, and target-driver
contracts.

`src/config/hero.marshal.dsl` is intentionally minimal and exists for Hero
policy-path symmetry only. Runtime Hero policy still owns execution authority,
and Lattice Hero still owns proof authority.

The M1 API provides:

```text
marshal_dispatch_advice_t
marshal_dispatch_request_t
marshal_dispatch_validation_context_t
marshal_dispatch_validation_result_t

canonical_*_text(...)
*_digest(...)
validate_dispatch_advice(...)
```

## Legacy E1 Evaluation Marshal Contract

E1 defines the first reusable environment-evaluation marshal contract. It does
not execute replay, choose a policy, or prove performance. It prepares a bounded
Runtime replay evaluation request and records a non-authoritative receipt after
Runtime has produced replay reports.

This contract is retained as compatibility and receipt-shape reference material.
New environment handoff language should use `rollout`.

Implemented surface:

```text
marshal_evaluation_anchor_range_t
marshal_evaluation_request_t
marshal_evaluation_plan_t
marshal_evaluation_runtime_handoff_t
marshal_evaluation_policy_summary_t
marshal_evaluation_receipt_t

canonical_evaluation_request_text(...)
evaluation_request_digest(...)
validate_evaluation_request(...)
prepare_evaluation_plan(...)
canonical_evaluation_plan_text(...)
evaluation_plan_digest(...)
prepare_evaluation_runtime_handoff(...)
canonical_evaluation_runtime_handoff_text(...)
evaluation_runtime_handoff_digest(...)
canonical_evaluation_receipt_text(...)
evaluation_receipt_digest(...)
finalize_evaluation_receipt(...)
```

The receipt preserves policy-neutral replay health counters, metric-scope labels,
policy-wise summaries, projection-validation diagnostics, and Cajtucu paper
execution summaries. Projection diagnostics include interval width,
zero-return baseline error, and model skill versus that baseline. Cajtucu
summaries include backend identity, execution-step counts, rejected/partial fill
counts, order/fill counts, and paper fee/spread/slippage/transaction-cost
totals. It is evidence for later review; it does not claim target satisfaction.

The request is accepted only when it is finite and explicitly out-of-sample:

```text
trained_range and validation_range are valid half-open anchor-index ranges
trained_range and validation_range do not overlap
trained_range_evidence_digest is present and strong
validation_split_policy_digest is present and strong
Runtime executable, evaluation config, MDN checkpoint, and representation
  checkpoint paths exist when path checks are enabled
accounting_numeraire_node_id is resolved from
  [ACCOUNTING].accounting_numeraire_node_id, unless explicitly overridden
target_node_ids is non-empty
accounting_numeraire_node_id appears inside target_node_ids
target_node_ids contains no duplicates
```

The plan emits command templates for:

```text
1. Runtime MDN run on the validation anchor range.
2. Runtime replay-from-job evaluation using the completed job directory.
```

The evaluation runtime handoff prepares a bounded dry-run argument package for:

```text
hero.runtime.run mode=dry_run
```

It binds the validation anchor range, expected Runtime wave identity, replay
command template, request digest, and plan digest. It is dry-run only:

```text
dry_run=true
target_satisfaction_claimed=false
runtime_executor=false
lattice_proof_authority=false
```

Marshal does not call Runtime from this contract; the handoff is a deterministic
package that can be inspected, digested, and later handed to the existing Runtime
Hero execution surface by an authorized operator/tooling path.

The receipt records replay metrics such as bundle count, attempted/completed
count, projection MAE/RMSE/correlation/directional accuracy, interval coverage,
time-law counters, transaction-cost summary, Cajtucu paper execution summary,
and final equity summary. It also labels the top-level aggregate scope so
experiment means are not confused with one policy's score:

```text
top_level_metric_scope=mean_over_completed_episode_reports_across_policies
policy_metric_scope=mean_over_completed_episode_reports_for_policy
```

Policy-wise summaries may be embedded in the receipt for compact review:

```text
policy_id
policy_kind
attempted/completed/failed counts
final equity
max drawdown
turnover
transaction cost
Cajtucu backend / fill / cost counters
projection metrics
```

The receipt always carries:

```text
target_satisfaction_claimed=false
runtime_executor=false
lattice_proof_authority=false
```

Lattice must still re-read Runtime evidence to prove any target. Marshal
evaluation receipts are coordination and audit metadata only.

## R1 Rollout Marshal Contract

R1 defines the environment rollout preparation contract. A rollout is a bounded
historical environment run:

```text
completed Runtime job artifacts
  + policy set
  + Kikijyeba replay environment
  + Cajtucu paper execution profile
    -> trajectory evidence after Runtime executes the command
```

Implemented surface:

```text
marshal_rollout_execution_profile_t
marshal_rollout_request_t
marshal_rollout_plan_t

canonical_rollout_request_text(...)
args_digest(...)
validate_rollout_request(...)
prepare_rollout_plan(...)
canonical_rollout_plan_text(...)
rollout_plan_digest(...)
rollout_plan_json(...)
```

`hero.marshal.rollout` has two modes:

```text
mode=plan
  validate and return a rollout plan / replay command template

mode=execute
  validate the same rollout plan, then call Environment/Runtime replay
```

Public rollout arguments are `mode`, `args_path`, `args_digest`, and optional
`include_machine_payload`. The digest is optional for `plan` and required for
`execute`. The execute path still does not train, inspect reports, choose a
policy, produce a receipt, or claim Lattice target satisfaction. Environment
owns rollout admission, Runtime remains the executor, Kikijyeba remains the
environment, and Cajtucu remains the paper execution backend.

The accepted V1 rollout mode is:

```text
environment_mode=historical_replay
environment_assembly_id=kikijyeba.environment.replay.v1
execution_backend_id=cajtucu.execution.paper.v1
```

The request must bind:

```text
rollout_id
rollout_attempt_id
idempotency_key
config_path
completed Runtime job directory with replay_artifacts_written=true
replay_batch_index_path
graph_order_fingerprint
asset_universe_digest
accounting_numeraire_node_id, resolved from global .config unless explicitly
  overridden
target_node_ids
policy_set as a non-empty explicit policy list
max_steps as a positive finite step cap
max_parallel_jobs as a positive finite worker cap
Cajtucu paper execution profile
```

Those fields live in the rollout request file, not as public top-level MCP
arguments.

`idempotency_key` is identity binding in this checkpoint. It is included in
request and plan digests for audit and future retry handling, but Marshal does
not yet maintain a durable duplicate-handoff ledger and does not claim durable
duplicate execution prevention.

Path-bearing rollout fields are trust boundaries. `runtime_job_dir` must be an
existing completed Runtime job when the corresponding requirements are enabled.
`replay_batch_index_path` must match the path recorded by that job's state.
`runtime_exec_path` is used for plan templates and Runtime replay delegation
only under Runtime Hero policy. `report_path`, when supplied, names the replay
report target that Runtime will write; Marshal records it but does not treat it
as proof.

`asset_universe_digest` is the Marshal digest of the ordered target-node action
universe:

```text
schema_version=kikijyeba.marshal.rollout_asset_universe.v1
accounting_numeraire_node_id=<accounting numeraire graph node>
target_node_ids=<comma-separated graph nodes in action order>
```

The graph fingerprint must match the completed Runtime job manifest. The asset
universe digest must match the resolved `accounting_numeraire_node_id` and
`target_node_ids`; the numeraire default comes from
`[ACCOUNTING].accounting_numeraire_node_id` in `.config`, not from the selected
Runtime wave or graph inference.

Synthetic direct execution edges are forbidden for Marshal validation rollout.
Research replay with synthetic execution surfaces belongs outside this
validation contract and must not be used to satisfy cost-aware rollout or
readiness claims.

The Cajtucu paper execution profile is part of rollout identity. Marshal emits
an `execution_profile_digest` and a `policy_set_digest` in the rollout plan so
costs, synthetic-edge policy, and resolved policy identity are visible in
replay handoff evidence. The V1 linear transaction cost rate is interpreted as
a fraction of traded notional. Equity mismatch tolerances are carried as
Cajtucu paper-backend profile values and remain forbidden for live execution.

The V1 Cajtucu profile is typed at the Marshal boundary:

```text
execution_backend_id=cajtucu.execution.paper.v1
cost_model_id=linear_transaction_cost_rate.v1
allow_synthetic_direct_edges=false by default
allow_partial_fills=false
live_execution_allowed=false
```

Marshal execute mode marks the delegated Runtime replay payload with
`validation_rollout=true`. Runtime then requires nonzero transaction cost,
finite replay bounds, execution-profile and policy-set digests, and rejects
synthetic execution edges at the request boundary. Runtime also checks the
produced replay report before accepting validation replay: synthetic Cajtucu
market steps or invalid Cajtucu traces block the handoff, even if the underlying
environment episodes completed. Marshal rejects partial fills for rollout until
Runtime replay has a concrete forwarding contract for that option.

`rollout` emits a replay command template for:

```text
cuwacunu_exec --config ... --replay-from-job-dir ...
```

In `mode=execute`, Marshal/Environment builds a compact replay handoff whose
payload is carried by `execution_request_path`, then hands it to the non-catalog
Runtime replay executor. Direct operators should prefer
`hero.environment.rollout`, which validates rollout admission before delegating
low-level replay to Runtime.
The response records Runtime tool digests and whether replay execution
completed, but it remains a handoff result, not a rollout receipt.

The report/summary lifecycle is intentionally separate:

```text
plan rollout -> marshal_rollout_plan_t
execute rollout -> Runtime replay handoff result
Runtime writes replay report / trajectory evidence
inspect reads report later -> inspection_summary, future work
```

`hero.marshal.rollout mode=execute` records handoff digests in the
response. Durable rollout handoff receipts are future work. Runtime reports,
metrics, and trajectory evidence are Runtime-owned artifacts, not Marshal
receipts.

The rollout plan always carries:

```text
target_satisfaction_claimed=false
marshal_runtime_executor=false
lattice_proof_authority=false
marshal_policy_training_authority=false
live_execution_authority=false
```

## Dispatchability

M1 advice is dispatchable only when it is concrete, fresh, and allowed by the
requested-mode context.
The validator fails closed for:

```text
missing_plan_basis
missing_suggested_wave
target_already_satisfied
target_status_not_dispatchable
stale_active_identity
stale_target_spec
stale_split_policy
max_waves_exhausted
malformed_wave_range
missing_model_state_input
forbidden_dispatch_mode
unknown_required_field
unsupported_wave_target
unsupported_source_range
advice_digest_mismatch
advice_runtime_root_mismatch
target_id_mismatch
runtime_policy_refused
runtime_wave_mismatch
checkpoint_input_mismatch
runtime_checkpoint_input_missing
runtime_handoff_unavailable
unproven_lattice_advice
stale_lattice_advice
dry_run_receipt_missing
dry_run_receipt_mismatch
```

`PLAN_INPUT_*` values must be resolved runtime model-state paths by the time
Marshal validates advice. Symbolic hints such as
`latest_satisfying:<target_id>` remain Lattice plan annotations and are not
dispatchable model-state inputs. Required model-state inputs must be absolute
paths, must stay inside configured `allowed_model_state_roots` when that
allowlist is provided, and must also appear in the Runtime Hero active-wave
snapshot before a handoff is accepted.

Fresh advice means:

```text
source_lattice_tool is one of the allowed Lattice plan/evaluate tools
source_lattice_timestamp parses as RFC3339 UTC
the timestamp is not in the future beyond the configured clock skew
the timestamp is not older than max_advice_age when a freshness clock is set
active identity matches the operator context
```

When a trust boundary requires signed Lattice advice, the validation context can
set `require_signed_lattice_advice=true` and provide a trusted advice key id plus
verification key. Advice may then carry `advice_receipt_digest`,
`advice_signature_key_id`, and `advice_signature`; Marshal recomputes the
canonical advice payload and fails closed on missing, malformed, untrusted, or
mismatched signature material. The current verifier is deterministic SHA-256
keyed material for local receipt integrity; a future asymmetric signer can
replace the signing primitive without changing the advice gate.

Marshal computes 256-bit SHA-256 digests for advice, requests, confirmation
tokens, handoff arguments, and receipt ids. These digests are local integrity
and replay aids; they do not make Marshal a proof authority.

## Non-Authority Rule

Marshal advice, decisions, and receipts are audit metadata only. They must not
satisfy:

```text
train-core readiness
validation evaluation readiness
leakage cleanliness
checkpoint closure
performance gates
```

After Runtime Hero writes artifacts, Lattice Hero must re-read runtime evidence
and prove the target.

## Public Operator Surface

The current high-level Marshal surface is intentionally small:

```text
hero.marshal.status
hero.marshal.prepare.train.one_step
hero.marshal.prepare.train.budgeted
hero.marshal.prepare.evaluate.one_step
hero.marshal.prepare.evaluate.budgeted
hero.marshal.rollout
hero.marshal.inspect.run.latest_chain
hero.marshal.inspect.run.training_state
hero.marshal.inspect.run.single_job
hero.marshal.inspect.run.compare
hero.marshal.inspect.target
hero.marshal.inspect.protocol.report
hero.marshal.inspect.protocol.strict
hero.marshal.inspect.spawn.by_id
hero.marshal.inspect.spawn.by_label
hero.marshal.inspect.spawn.by_registry
hero.marshal.inspect.spawn.by_fingerprint
hero.marshal.inspect.spawn.by_job
hero.marshal.inspect.component
hero.marshal.inspect.facts.summary
hero.marshal.inspect.facts.lineage
hero.marshal.inspect.facts.preview
hero.marshal.inspect.facts.preview.by_digest
hero.marshal.inspect.facts.preview.by_digest_prefix
hero.marshal.inspect.facts.preview.by_index
```

`prepare.*` is the deterministic Marshal family for moving one lattice target
toward readiness. Its public arguments are `target_id`, `mode`, and optional
`args_path`/`args_digest`. The target intent and driver shape live in the tool
name: `train.one_step`, `train.budgeted`, `evaluate.one_step`, or
`evaluate.budgeted`. Target-specific packet fields such as `driver_policy`,
`context`, Runtime snapshots, materialization flags, and resume evidence live
in the prepare request file. The digest is optional for `plan` and required for
`dry_run`/`execute`.

`one_step` computes one Lattice-backed move, optionally dry-runs it, emits a
target-driver ledger, and stops. `budgeted` performs a finite driver loop: it
asks Lattice for the current target deficit, materializes model-state inputs,
checks Runtime wave/policy alignment, dry-runs Runtime, and either stops before
execution or executes only when `mode=execute`, `driver_policy`
from the prepare request file allows execution, Runtime policy allows
execution, and the existing execution gate accepts the dry-run evidence. The
target is reported reached only after Lattice reports satisfaction.

For V1, `prepare` should be read as dispatchable Runtime target movement, not
as a general environment or artifact tool. Replay rollout uses
`hero.marshal.rollout`; artifact-readiness targets stop with
`next_safe_actions=["inspect"]`; policy-training uses Runtime policy-training
contracts and should be routed through a finite Marshal-materialized handoff
surface when it is promoted to the operator path.

`rollout` is the deterministic Marshal for replaying already produced Runtime
job evidence through the Kikijyeba environment and Cajtucu paper execution
backend. It starts from a completed Runtime job directory, a policy set, an
accounting numeraire graph node, ordered target graph nodes, finite replay
limits, and an execution profile. In `mode=plan` it returns a rollout
plan. In `mode=execute` it delegates through Runtime replay execution
and records the handoff state and digests. Only `mode`,
`args_path`, `args_digest`, and `include_machine_payload` are public; the
rollout packet lives in the request file. Its current execute path delegates through
`hero.environment.rollout mode=replay`, which then calls Runtime.
Direct replay operators should use `hero.environment.rollout`. It does not
train, tune, inspect reports, produce a rollout receipt, or claim Lattice target
satisfaction.

`inspect.*` exposes concrete read-only tools for run, target, protocol, spawn,
component, and fact panels. Selectors such as `runtime_root`, `job_id`,
`target_id`, `fact_family_id`, preview selectors, identity fingerprints, and
`include_machine_payload` are direct arguments on the relevant concrete tool.

Artifact-readiness targets are evidence proofs, not Runtime wave requests.
When `hero.lattice.evaluate.deficit` reports `target_class=artifact_readiness`,
`prepare` returns a non-dispatchable packet with
`next_safe_actions=["inspect"]`; it does not resolve checkpoints, inspect
Runtime wave shape, or dry-run Runtime. Use `hero.marshal.inspect.target` or
`hero.marshal.inspect.facts.summary` to view the target proof and any related
fact-family summary.

`inspect.*` is read-only. It calls Lattice `hero.lattice.evaluate.target` for a
target proof, `hero.lattice.inspect.facts.summary` for fact-family
evidence, `hero.lattice.inspect.facts.lineage` for audit-only
relation/key/digest lineage rows, and `hero.lattice.inspect.facts.preview`
when the inspect call asks for a concrete fact row by digest, digest prefix, or
fact index. Marshal does not
become proof authority, does not claim target satisfaction, and does not select
checkpoints from this panel.
The public inspect, status, and operational-report JSON surfaces
declare this boundary explicitly with fields such as `target_proof=false`,
`dispatchable=false`, `runtime_executor=false`,
`fact_families_are_not_target_kinds=true`, `allocation_decision=false`,
`market_readiness_decision=false`, and `deployment_decision=false`.
For failed artifact proofs, the panel preserves the Lattice summary:
failed-proof count, proof-template bound/unbound count, proof kind,
proof-template claim, identity-mismatch count, lineage-unbound count,
authority-drift count, integrity flags, authority flags, explicit boundary
denial flags for target dependencies, Runtime waves, Marshal reachability,
checkpoint sources, and plan checkpoint inputs, issue codes, artifact deficit
keys, related fact-integrity issue codes, and primary deficit key.
Fact panels also relay Lattice's native `fact_integrity_summary`, which reports
declared relation digests, resolved relation digests, unresolved relations,
identity-mismatch counts, digest-mismatch counts, warning counts, affected
families, and issue codes. This keeps unresolved transform, baseline,
selection-signal, observer, and forecast artifact lineage visible before an
artifact proof failure is interpreted as a generic target problem.
Fact panels include a compact `lineage_panel` by default. It relays
`hero.lattice.inspect.facts.lineage` row counts, selected
relations, and explicit non-authority flags such as
`cache_rows_used_for_target_satisfaction=false`; operators may put
`include_lineage=false` on the inspect call only to suppress that audit
view. Raw relation/key/digest rows stay in `machine_payload` when
`include_machine_payload=true` is present on the inspect call.
Fact panels include `preview_panel` on `hero.marshal.inspect.facts.preview` and
its selector splits: `hero.marshal.inspect.facts.preview.by_digest`,
`hero.marshal.inspect.facts.preview.by_digest_prefix`, and
`hero.marshal.inspect.facts.preview.by_index`. The preview relays matching
`hero.lattice.inspect.facts.preview*` rows; `fact_digest_prefix` must resolve
uniquely in the selected family before Lattice returns a preview. The panel keeps
`preview_rows_are_audit_only=true`,
`facts_used_for_target_satisfaction=false`, `checkpoint_selected=false`, and
`model_selector=false`. The operator panel summarizes display-only handles as
`preview_fact_refs`; full fact rows, `fact_digest` fields, parent digest arrays,
lineage rows, and Lattice `identity_envelope` payloads stay in
`machine_payload` when `include_machine_payload=true` is present in the request
file.
Target panels also summarize Lattice artifact `fact_preview_hint` entries as
`artifact_fact_preview_*` fields, including `artifact_fact_preview_refs`. Marshal
relays refs, family, digest count, Lattice tool, and Marshal tool as navigation
metadata only; full preview digests stay in machine payload. Marshal still does
not prove, select, or dispatch from those hints.
They also relay the fact catalog's artifact-readiness boundary metadata:
proofable family count, proof kind, proof-template claim, promotion-blocked
family count, promotion-blocked reason, and warning-summary-only families.
Marshal displays these as read-only catalog facts; it does not promote a fact
family into a target or override Lattice's proof-template registry.
When Lattice reports disabled policy-gate reservations, Marshal relays the
reservation summary and matching reservations in target and prepare panels, but
keeps policy-gate dispatch, target-status, and proof authority false. Marshal
also preserves Lattice's policy-fingerprint verification counters and
policy-input contract counters as read-only audit context, including mismatch
counts, missing-input counts, and the all-inputs-complete bit; it does not
recompute or promote them into decision-policy authority.
Fact-family-only evidence panels, including `replay_environment`, do not call
target proof evaluation and cannot be routed through `prepare`.

Warning stops use typed Runtime/Lattice warning envelopes rather than loose JSON
sniffing. A warning used for driver policy must carry `warning_id`, `severity`,
`source`, `component`, `scope`, `blocking`, and `evidence_digest`. Unknown or
missing severity fails closed when the matching stop policy is enabled; a
blocking warning stops even below the configured severity threshold. Lattice
target evaluations project their `warning_results` into this typed `warnings`
surface with `blocking=false`; human-readable warning text stays in
`warning_messages`.

The operator packet presents a compact `operator_summary`, `stop_reason`,
`wave_panel`, `runtime_panel`, `lattice_panel`, and `audit_panel`, followed by
the compatibility fields for target, blocker, advised wave, concrete
model-state inputs, Runtime wave match, dry-run result, target-driver terminal
state, target-driver ledger, and next safe actions.

`inspect` is also the high-level deterministic read-only evidence inspector. It
supports:

```text
hero.marshal.inspect.run.latest_chain
hero.marshal.inspect.run.training_state
hero.marshal.inspect.run.single_job
hero.marshal.inspect.run.compare
hero.marshal.inspect.target
hero.marshal.inspect.protocol.report
hero.marshal.inspect.protocol.strict
hero.marshal.inspect.spawn.by_id
hero.marshal.inspect.spawn.by_label
hero.marshal.inspect.spawn.by_registry
hero.marshal.inspect.spawn.by_fingerprint
hero.marshal.inspect.spawn.by_job
hero.marshal.inspect.component
hero.marshal.inspect.facts.summary
hero.marshal.inspect.facts.lineage
hero.marshal.inspect.facts.preview
hero.marshal.inspect.facts.preview.by_digest
hero.marshal.inspect.facts.preview.by_digest_prefix
hero.marshal.inspect.facts.preview.by_index
```

The `hero.marshal.inspect.run.*` tools wrap the operational report and
comparison paths through concrete `latest_chain`, `training_state`,
`single_job`, and `compare` tool names.
When `hero.marshal.inspect.run.*` asks Lattice for bulk target status,
artifact-readiness
blockers preserve the failed artifact proof summary, including explicit
boundary denials for non-dispatchable/non-checkpoint authority fields, and route
to concrete inspect tools rather than presenting the failure as a dispatchable
training deficit. Disabled policy-gate reservations remain attached as
read-only blocker context and do not change next-safe-action selection.
`hero.marshal.inspect.target` asks Lattice for the target deficit and explains
the plan or blocker. It labels target status as sourced from
`hero.lattice.evaluate.deficit` and only suggests certificate inspection when
Lattice returned certificate material. `hero.marshal.inspect.protocol.report`
reports observed Runtime identity fields without expected-identity arguments.
`hero.marshal.inspect.protocol.strict` requires explicit expected identity
fields and reports `identity_verified=true` only when they all match.
`hero.marshal.inspect.spawn.*` and `hero.marshal.inspect.component` group
Runtime evidence for one component spawn or component family with
`evidence_scope=runtime_jobs_only`. All inspect tools explain evidence without
execution, proof claims, model selection,
or checkpoint selection.

The lower-level behavior is implementation detail, not a public tool surface:

```text
target advice materialization
dispatch validation
Runtime dry-run handoff
execution-gate validation
receipt replay
batch preview
```

These remain ordinary C++ methods where needed. They are not registered as
Marshal Hero tools and cannot be invoked through MCP/CLI tool names.

Standalone scheduler research notes were removed from this folder. Scheduler
ideas remain intentionally out of scope for Marshal's current public surface;
the active roadmap keeps them in the `Not Now` section instead of carrying a
separate research artifact.

## Marshal Operational Report v1

`hero.marshal.inspect.run.latest_chain`,
`hero.marshal.inspect.run.training_state`, and
`hero.marshal.inspect.run.single_job` are the read-only operator report
surfaces. They answer:

```text
Where are we, what just happened, and what is safe to do next?
```

It reads Runtime job directories, `job.state`, Runtime terminal facts, component
reports, checkpoint load/write fields, Kikijyeba replay evidence indexes, and
Lattice target statuses. Runtime terminal facts are preferred when present;
component reports remain the fallback for older jobs. The current packet
includes:

```text
evidence_scope
operator_summary
stop_reason
runtime_panel
lattice_panel
audit_panel
current_state
job_chain
chain_summary
target_blockers
warnings
suspicious_items
next_safe_actions
```

`chain_summary` groups each Runtime job by component family, component spawn
identity, wave action/mode, source range, checkpoint I/O, handoff identity,
terminal-fact availability, and read-only replay evidence. Replay evidence is
reported from `artifacts/kikijyeba.environment.replay.v1/` when present:
`runtime_replay_batches.index`, `runtime_replay_experiments.index`, and the
latest replay experiment report. Marshal only summarizes these files; Runtime
Hero remains the replay executor/reader.

When `include_machine_payload=true` is present in the inspect request file, the
report also includes full job rows, checkpoint rows, the target-status map, and
detailed metrics under `machine_payload`.

The report does not execute, dry-run, edit config, select checkpoints, or claim
target satisfaction. Lattice remains the only proof authority; Marshal only
quotes Lattice statuses, proof-check issues, deficits, and warnings, then turns
Runtime artifacts into an operator-readable summary.
For artifact-readiness targets, target blockers also quote related
fact-integrity issue codes from Lattice proof deficits so the compact run view
can connect an artifact issue to the exact catalog issue row without
re-evaluating proof.

## D2 Preview And Handoff Core

The D2 adapter core can turn validated D1 advice plus Runtime Hero policy/wave
snapshots into a deterministic dry-run request preview.

It checks:

```text
Runtime Hero availability
runtime executable availability
runtime_root agreement
active wave target/mode/range agreement
checkpoint/model-state input agreement
mode=dry_run
field derivations for each generated Runtime request field
```

The handoff helper calls Runtime Hero through
`hero.runtime.run mode=dry_run` without retired
Runtime-side confirmation or timeout arguments, returning the Runtime Hero tool
result JSON plus the Marshal non-authority statement. Runtime handoff parsing
reads the MCP result's top-level `isError` and `structuredContent.ok` fields so
nested text or nested objects cannot accidentally masquerade as Runtime
acceptance.

The public D2 operation accepts explicit fresh Lattice Hero advice, refuses
unproven advice, calls the handoff helper, and includes the live Runtime Hero
dry-run result in the Marshal response.

`hero.marshal.prepare.*` is the operator-facing family for “pursue lattice
target X.” The split suffix chooses train/evaluate and one_step/budgeted. The
target wrapper calls the current Lattice target advice query, asks Marshal's
read-only Lattice callback to materialize symbolic
`latest_satisfying:<target_id>` model-state hints, validates the materialized
advice, inspects Runtime policy and active wave shape, and returns one compact
operator packet.

If that query identifies an artifact-readiness target, Marshal stops before the
handoff path and routes the operator to `hero.marshal.inspect.target` or
`hero.marshal.inspect.facts.summary`.
This keeps non-trainable Lattice evidence surfaces out of Runtime dispatch
semantics.

In `one_step` mode, `mode=plan` prepares without a Runtime call,
`mode=dry_run` calls Runtime dry-run, and
`mode=execute` requires accepted dry-run evidence before any Runtime
execution gate can pass. In `budgeted` mode, Runtime dry-run is part of the
driver loop for `dry_run` and `execute`, and is bounded by
`driver_policy.max_waves`. `budgeted` execution also requires
`driver_policy.max_wall_clock_seconds`, `driver_policy.allow_execute=true`, and
Runtime `allow_execute=true`. Train waves additionally require both
`driver_policy.allow_train_execute=true` and Runtime `allow_train_execute=true`.
The requested mode controls whether Runtime dry-run is requested.
Marshal reports desired wave differences but does not edit wave config, select
checkpoints, or claim target satisfaction.

Every target-driver run emits a ledger with the driver policy digest,
per-iteration Lattice target-deficit digest, suggested-wave digest, Runtime
handoff/request digests, terminal Runtime evidence digests, terminal state, and
non-authority statement. When `require_runtime_job_completion=true`, Marshal
requires durable `runtime.result.fact` plus `job.manifest`; a completed
`job.state` alone is not enough. If Runtime reports checkpoint writes or loaded
model-state checkpoints, `runtime.checkpoint_io.fact` must also be present and
is bound into the ledger. A compact `resume_ledger` in the prepare request file
may be supplied to continue a bounded run without forgetting spent
handoff/execution budget; stale target, mode, policy identity, missing
terminal evidence identity, missing manifest identity, or missing handoff
identity blocks resume.

Target-driver identity is split deliberately:

```text
target_driver_run_key
  Deterministic grouping key for the target, known active identity, drive mode,
  requested mode, and driver policy digest. It groups comparable driver ledgers
  but is not a terminal evidence identity.

target_driver_run_id
  Per-invocation ledger identity. It is derived from the run key plus
  ledger_created_at_utc and ledger_nonce, exists before the first Runtime
  handoff, is echoed into Runtime handoff/job evidence, and must remain stable
  across resume.

runtime_handoff_id / runtime_handoff_digest
  Per-handoff execution identity. Runtime terminal evidence must match these
  exactly; they are the hard reconciliation identity.
```

Marshal generates `ledger_created_at_utc` and `ledger_nonce` for a fresh
prepare call, or restores them from `resume_ledger` when continuing a bounded
run. Runtime handoff id/digest remain the strict terminal-evidence identity even
when several ledgers share the same target-driver run key.

Target-driver replay audit is also available internally for retention drills.
Full ledger replay recomputes iteration and ledger digests. Compact ledger
replay accepts removed iteration bodies only when `compacted_fields` says so and
the durable Runtime evidence identities remain. Replay rejects stale target,
protocol, graph order, target spec, split policy, driver policy, and Runtime
policy identities. It remains audit-only and never proves target satisfaction.

Runtime Hero owns the active wave. Marshal compares advice to Runtime's decoded
wave using the canonical fields `target_component_family_id`, `mode`,
`source_range`, range bounds, `job_kind`, `train_target`, and
`model_state_inputs`.

## D3 Execution Gate

D3 allows an execution handoff only after the dry-run path has produced
structured accepted dry-run evidence for the same dispatch. A non-empty string
or placeholder digest is not enough.

Prior dry-run evidence binds:

```text
accepted dry-run status
advice_digest
dispatch_request_identity_digest
runtime_preview_request_digest
suggested_wave_digest
plan_input_digest
Runtime Hero policy snapshot digest
Runtime Hero active-wave snapshot digest
Runtime response digest
dry-run response digest
canonical prior evidence digest
```

The confirmation token binds:

```text
target_id
runtime_root
protocol_contract_fingerprint
target_spec_fingerprint
split_policy_fingerprint
suggested_wave_digest
plan_input_digest
requested_mode
```

The execution gate refuses missing confirmation, confirmation mismatch, missing
or mismatched prior dry-run evidence, unproven/stale Lattice advice, Runtime
Hero `allow_execute=false`, and train execution when
`allow_train_execute=false`.

Current `budgeted` execution uses policy-confirmed automation. When
`driver_policy.allow_execute=true` and Runtime policy also allows execution,
Marshal creates the execution confirmation token internally from the accepted
dry-run evidence and exact request identity before it calls the execution gate.
This is not a second human confirmation step. A future interactive operator
surface may split this into a dry-run challenge followed by a caller-supplied
confirmation token.

The public execution handoff takes an accepted `marshal_execution_gate_result_t`.
It cannot be called from a plain dry-run decision. Immediately before calling
`hero.runtime.run mode=execute`, Marshal asks Runtime
Hero to decode the active wave and compares target, mode, source range, and
anchor bounds against the derived request, including concrete `PLAN_INPUT_*`
model-state inputs. The Runtime Hero run-wave schema accepts a canonical
`runtime_handoff` object plus an `execution_request_path` for launch-range
payload. The object carries
handoff schema/id/digest, creator/timestamp, target id, base config path/digest,
concrete wave fields, concrete checkpoint inputs, Runtime policy path/digest,
dry-run or execute intent, and an `unresolved_symbols` list. Runtime rejects
non-empty `unresolved_symbols`, symbolic `latest_satisfying:*` inputs,
mismatched intent, and mismatched active-wave fields before launching
`cuwacunu_exec`. Accepted handoffs are echoed back through the Runtime job
manifest, terminal result fact, and derived lattice exposure fact using
`runtime_handoff_id` and `runtime_handoff_digest`.

## D4 Dispatch Receipts

D4 receipts explain what Marshal requested and what Runtime Hero returned. They
record:

```text
target_id
lattice_proof_context
plan_basis_digest
suggested_wave_digest
Runtime request digest
Runtime preview request digest
Runtime execution request digest
Runtime handoff argument digest
Runtime response digest
policy decision
confirmation status
refusal reasons
post-run verification instruction
non-authority statement
```

`lattice_proof_context` is the active proof context copied from Lattice advice;
it is not a component spawn id, component spawn fingerprint, or target
satisfaction proof. Replay audit checks receipt schema, active identity, receipt
id/content consistency, required audit digests, retention class, and the
non-authority statement. A fresh receipt can explain a handoff; it still cannot
satisfy a lattice target.

Receipts may be compacted. Compact receipts remove bulky Runtime Hero output but
retain request/advice/decision/runtime-response digests, refusal reasons,
identity, compacted-field notes, and the non-authority statement. Compact and
tombstone receipts remain audit metadata only.

## S1 Status And Batch Preview

Marshal status is a compact read-only summary over recent receipts and Runtime
policy. It reports last accepted dry-run, last execution handoff, latest refusal
reason, and Lattice advice surface availability. It does not evaluate lattice
targets, prove targets, promote fact families, or make allocation, market, or
deployment decisions.

## Marshal Run Comparison

`hero.marshal.inspect.run.compare` compares two
Runtime jobs descriptively. The `baseline_job_id` and `candidate_job_id`
selectors are direct tool arguments. It reads the same Runtime job
state, terminal facts, checkpoint I/O facts, health facts, and raw report
fallback fields used by the operational report, then returns:

```text
operator_summary
stop_reason
runtime_panel
lattice_panel
audit_panel
run_pair
comparability
config_identity_deltas
wave_range_deltas
metric_deltas
checkpoint_deltas
warning_deltas
proof_context
```

`config_identity_deltas` and `wave_range_deltas` make config identity, protocol
identity, graph/source identity, wave action/mode, source-range policy, anchor
bounds, and accepted anchor count visible before metric differences. When
`include_machine_payload=true` is present in the inspect request file,
comparison also returns full baseline and candidate job rows, full performance
visibility panels, checkpoint lineage details, and per-run metric maps.

The comparison and run-report surfaces are read-only. They do not execute, edit
config, select a checkpoint, choose a winner, claim target satisfaction, promote
fact families into target kinds, or make allocation, market, or deployment
decisions. Lattice cleanliness is reported as a boundary: comparison can say
that Lattice replay is required, but it cannot prove cleanliness by itself.

Invariant:

```text
Compare does not choose. Compare explains.
```

Use `mode=compare` for run-to-run comparison, `mode=latest_chain` for the
latest training/eval chain, and `mode=single_job` for a bounded inspection of
one job.

## Auxiliary Surfaces

Codex-assisted Marshal wrappers call deterministic primitives and keep free-form
text as operator explanation or prompt text. They cannot bypass validation,
policy, confirmation, range, identity, or checkpoint checks.

Marshal tool schemas are cataloged for MCP/CLI compatibility, and
`hero_marshal.mcp` exposes the deterministic primitives through direct CLI and
JSON-RPC stdio modes. The handler layer rejects missing required fields and
unknown fields before materializing Marshal structs. Schema compatibility checks
remain harness safety only; they do not change proof semantics.

The performance budget separates library validation, preview derivation,
Runtime Hero policy read, long-lived MCP schema surface, direct CLI schema
surface, Runtime Hero handoff, and receipt IO. Every stage uses repeated samples
and reports sample count plus min/max/p50/p95 timing. Performance reports
explicitly state that speed is not target satisfaction evidence.
