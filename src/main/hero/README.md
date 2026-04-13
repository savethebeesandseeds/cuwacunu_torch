# HERO MCP Runtime Guide

This directory builds six stdio MCP servers:

- `/cuwacunu/.build/hero/hero_config.mcp`
- `/cuwacunu/.build/hero/hero_hashimyei.mcp`
- `/cuwacunu/.build/hero/hero_lattice.mcp`
- `/cuwacunu/.build/hero/hero_runtime.mcp`
- `/cuwacunu/.build/hero/hero_marshal.mcp`
- `/cuwacunu/.build/hero/hero_human.mcp`

## Build

```bash
make -C /cuwacunu/src/main/hero -j12 all
```

## Install

HERO binaries live canonically under `/cuwacunu/.build/hero`, so their
post-build permission step is now exposed as `install-*-hero` targets:

```bash
make -C /cuwacunu/src/main/hero -j12 install-config-hero
make -C /cuwacunu/src/main/hero -j12 install-runtime-hero
```

## Defaults

All six HERO binaries default to:

- global config path: `/cuwacunu/src/config/.config`
- unencrypted catalog mode for Hashimyei/Lattice

Runtime paths are resolved from `[REAL_HERO]` pointers in that global config.

## Direct One-Shot Tool Calls

Each HERO MCP binary supports direct tool invocation:

```bash
/cuwacunu/.build/hero/hero_config.mcp \
  --tool hero.config.status \
  --args-json '{}'

/cuwacunu/.build/hero/hero_hashimyei.mcp \
  --tool hero.hashimyei.list \
  --args-json '{}'

/cuwacunu/.build/hero/hero_lattice.mcp \
  --tool hero.lattice.list_facts \
  --args-json '{}'

/cuwacunu/.build/hero/hero_runtime.mcp \
  --tool hero.runtime.list_campaigns \
  --args-json '{}'

/cuwacunu/.build/hero/hero_marshal.mcp \
  --tool hero.marshal.list_sessions \
  --args-json '{}'

/cuwacunu/.build/hero/hero_human.mcp \
  --tool hero.human.list_requests \
  --args-json '{}'
```

Human-friendly discovery:

```bash
hero_config.mcp --list-tools
hero_hashimyei.mcp --list-tools
hero_lattice.mcp --list-tools
hero_runtime.mcp --list-tools
hero_marshal.mcp --list-tools
hero_human.mcp --list-tools
```

## Stdio Session Lifetime

The stdio server mode now answers empty `resources/list` and
`resources/templates/list` requests, honors `shutdown` and `exit`, and applies
an idle session timeout by default. If no JSON-RPC message arrives on stdin for
`600` seconds, the server exits so leaked launcher pipes do not leave large idle
resident processes behind.

You can override that timeout with `HERO_MCP_IDLE_TIMEOUT_SEC`:

```bash
export HERO_MCP_IDLE_TIMEOUT_SEC=60   # exit after 1 minute of MCP idle time
export HERO_MCP_IDLE_TIMEOUT_SEC=0    # disable the idle timeout
```

For scripts and one-off diagnostics, prefer direct `--tool` execution because
it avoids keeping a resident MCP server process alive at all.

## Ownership Boundaries

- `Config Hero` owns config inspection, edits, validation, save/reload, and developer reset.
- `Hashimyei Hero` owns identity/catalog lineage surfaces.
- `Lattice Hero` owns run/report query surfaces.
- `Runtime Hero` owns detached campaign dispatch, campaign/job tracking, log tails, and stop/reconcile controls.
- `Marshal Hero` owns the persistent Codex session ledger, checkpoint orchestration, launch budgets, pauses, and bounded campaign continuation decisions.

`jkimyei` is not a separate Hero. It remains the training/runtime facet inside wikimyei components and their DSLs.

## Tool Surfaces

Config MCP:

- `hero.config.status`
- `hero.config.schema`
- `hero.config.show`
- `hero.config.get`
- `hero.config.set`
- `hero.config.default.list`
- `hero.config.default.read`
- `hero.config.default.create`
- `hero.config.default.replace`
- `hero.config.default.delete`
- `hero.config.objective.list`
- `hero.config.objective.read`
- `hero.config.objective.create`
- `hero.config.objective.replace`
- `hero.config.objective.delete`
- `hero.config.optim.list`
- `hero.config.optim.read`
- `hero.config.optim.create`
- `hero.config.optim.replace`
- `hero.config.optim.delete`
- `hero.config.optim.backups`
- `hero.config.optim.rollback`
- `hero.config.validate`
- `hero.config.diff`
- `hero.config.dry_run`
- `hero.config.backups`
- `hero.config.rollback`
- `hero.config.save`
- `hero.config.reload`
- `hero.config.dev_nuke_reset`

Config Hero write policy:
- `hero.config.set` is in-memory only until `hero.config.save`.
- `hero.config.default.create/replace/delete`, `hero.config.objective.create/replace/delete`, `hero.config.optim.create/replace/delete/rollback`, `hero.config.save`, and `hero.config.rollback` require `allow_local_write=true`.
- persisted config writes, default/objective file mutations, and plaintext optim mutation targets must stay inside `write_roots`.
- `hero.config.optim.*` resolves its hidden plaintext root and encrypted archive from `GENERAL.tsodao_dsl_filename`; its pre-mutation backups are encrypted TSODAO archive checkpoints written under `optim_backup_dir`.
- `hero.config.dev_nuke_reset` uses the saved global-config runtime root instead of `write_roots`; when `dev_nuke_reset_backup_enabled=true` it archives targets under `<runtime_root>/../.backups/hero.runtime_reset/<runtime_root-name>/<stamp>/` before cleanup and is intended to work even when `allow_local_write=false`.
- `hero.config.default.*` are constrained by `default_roots` plus `allowed_extensions`, and successful default mutations return a warning because they change shared truth.
- `hero.config.objective.*` are constrained by `objective_roots` plus `allowed_extensions`; this same surface covers the objective-local campaign file named by `campaign_dsl_path` when it lives under the objective root.
- `hero.config.optim.*` is constrained by the TSODAO hidden root plus `allowed_extensions`; `hero.config.optim.list/read` are read-only, while the mutating operations checkpoint the encrypted archive before they touch plaintext.
- `.md` files may be visible on those file surfaces when `allowed_extensions` includes markdown, but create/replace support still depends on the target file family's validation support.

Hashimyei MCP:

- `hero.hashimyei.list`
- `hero.hashimyei.get_component_manifest`
- `hero.hashimyei.get_founding_dsl_bundle`
- `hero.hashimyei.update_rank`
- `hero.hashimyei.reset_catalog`

Hashimyei is the component identity/discovery layer.
`hero.hashimyei.list` is intentionally snapshot-based: it reads the current
catalog state without forcing a store-wide ingest rebuild, so discovery probes
do not take the exclusive ingest lock. The component lookup tools still sync
the catalog to the store when they need fresh manifest resolution.
`hero.hashimyei.update_rank` persists a shared `hero.family.rank.v1` runtime
artifact scoped by `(family, contract_hash)`, returns before/after family
ordering, and synchronizes the lattice catalog so report views see the same
rank overlay. Rank exists only after an explicit overlay is written; it does
not bootstrap automatically and does not affect runtime component selection or
docking.
Component manifests are contract-scoped revisions and carry both
`founding_dsl_source_*` metadata and `docking_signature_sha256_hex`, so
Hashimyei can distinguish lineage origin from contract docking compatibility.
They also carry a component `lineage_state` such as `active`, `deprecated`,
`replaced`, or `tombstone`.
For the current VICReg+dataloader contract surface, observation docking remains
contract-owned even though loader shape is derived from the channel table:
`C = count(active == true)` and `T = max(seq_length)` over active rows. Runtime
validates those derived values against contract `__obs_channels`,
`__obs_seq_length`, and VICReg `INPUT.{C,T}` when contract-owned observation
DSL paths are present.
In the checked-in defaults, the intended contract-owned public docking widths
are `__obs_channels`, `__obs_seq_length`, `__obs_feature_dim`, and
`__embedding_dims`; private VICReg encoder/projector widths live in VICReg DSLs.
Runtime reuse now keys on public docking compatibility rather than an exact
founding contract match. The manifest still records its founding contract for
lineage/provenance, but private VICReg topology can vary across revisions as
long as the public docking widths remain compatible.
Each component revision also stores an immutable founding DSL bundle snapshot
under its canonical artifact leaf at
`.runtime/.hashimyei/tsi/.../_definition/<component_id>/...`; the
`hero.hashimyei.get_founding_dsl_bundle` tool reads that stored bundle as the
canonical founding-bundle surface.

Lattice MCP:

- `hero.lattice.list_facts`
- `hero.lattice.get_fact`
- `hero.lattice.list_views`
- `hero.lattice.get_view`
- `hero.lattice.refresh`

Lattice is the fact index plus query-time view layer.
Use `hero.lattice.get_fact` for assembled component fact bundles.
Use `hero.lattice.get_view` for derived comparisons.
Normal read tools synchronize the catalog to the current runtime store before
querying. Use `hero.lattice.refresh(reingest=true)` when you want to force a
rebuild explicitly.

Runtime MCP:

- `hero.runtime.start_campaign`
- `hero.runtime.list_campaigns`
- `hero.runtime.get_campaign`
- `hero.runtime.stop_campaign`
- `hero.runtime.list_jobs`
- `hero.runtime.get_job`
- `hero.runtime.stop_job`
- `hero.runtime.tail_log`
- `hero.runtime.tail_trace`
- `hero.runtime.reconcile`

`hero.runtime.start_campaign` accepts optional `binding_id`, `campaign_dsl_path`,
`marshal_session_id`, and `reset_runtime_state`.

- `binding_id` narrows execution to one declared bind instead of the default `RUN` plan
- `campaign_dsl_path` overrides the configured source campaign for that launch
- `marshal_session_id` links the launched campaign to an existing Marshal Hero session ledger; this is the linkage field Marshal Hero uses when it asks Runtime Hero to launch the next campaign
- `reset_runtime_state=true` requests a cold runtime-owned reset before the first worker job begins; it clears `.campaigns` and `.hashimyei`/runtime catalogs as needed, but preserves `.marshal_hero` and `.human_hero` so Marshal/Human ledgers survive launch-time resets

Use `hero.config.dev_nuke_reset` when you intentionally want the broader developer wipe that also removes `.marshal_hero` and `.human_hero`.

Marshal MCP:

- `hero.marshal.start_session`
- `hero.marshal.list_sessions`
- `hero.marshal.get_session`
- `hero.marshal.pause_session`
- `hero.marshal.resume_session`
- `hero.marshal.continue_session`
- `hero.marshal.terminate_session`

Human MCP:

- `hero.human.list_requests`
- `hero.human.get_request`
- `hero.human.answer_request`
- `hero.human.resolve_governance`
- `hero.human.list_summaries`
- `hero.human.get_summary`
- `hero.human.ack_summary`

When `hero_human.mcp` is invoked with no tool/list flags on a tty, it opens the Human Hero operator console instead of waiting for JSON-RPC on stdin. On ncurses-capable terminals this is now an all-session cockpit: the default sessions view shows active, running, paused, idle, and finished Marshal sessions with state-aware colors, state-filter cycling, and scrollable detail panes, while `Tab` cycles into focused requests and summaries views. Human-facing language is derived from the persisted ledger into operator states such as `Running`, `Waiting: Clarification`, `Waiting: Governance`, `Operator Paused`, `Review`, and `Done`, plus next-action hints. Request rows correspond to sessions paused for ordinary clarification or signed governance. Idle session summaries are actionable from that console and can be continued with a fresh operator instruction when you want to launch more work, while finished-session summaries remain informational. Summary actions are intentionally distinct: Continue reopens the session, while Acknowledge records a required review message and clears the report from the Human inbox after review. Session summaries begin with an effort summary that foregrounds elapsed wall time, checkpoint count, and campaign-launch usage. The same shared Human inbox/detail/action layer now also powers the `iinuji_cmd` Human screen. On unsupported terminals such as `TERM=dumb` it falls back to the line-prompt responder automatically.
Human Hero defaults are loaded from `[REAL_HERO].human_hero_dsl_filename`. If `operator_id` is still `CHANGE_ME_OPERATOR` or empty, the first Human Hero use auto-initializes it to `<user>@<hostname>` and persists that value back into `default.hero.human.dsl`. `hero.human.answer_request` is unsigned and auto-resumes a session paused for ordinary clarification. `hero.human.resolve_governance` is the signed governance path: operators provide `marshal_session_id` carrying the marshal cursor, `resolution_kind = grant | deny | clarify | terminate`, and `reason`, while Marshal Hero retains responsibility for choosing the next Runtime action. Session summaries are acknowledged with a required message to clear them from the operator inbox, and those acknowledgments produce signed summary-ack artifacts for auditability. Session inspection and lifecycle control now live under `hero.marshal.*`.

Human operator setup:

1. Run:
   ```bash
   bash /cuwacunu/setup.sh
   bash /cuwacunu/src/scripts/setup_human_operator.sh
   ```
2. The script bootstraps `operator_id` if needed, creates or validates the unencrypted OpenSSH `ssh-ed25519` identity at the configured `operator_signing_ssh_identity` path, and updates `human_operator_identities`.
3. Re-check later with:
   ```bash
   bash /cuwacunu/src/scripts/setup_human_operator.sh --validate
   ```
4. If an operator identity already exists, the script asks whether to replace it with a fresh keypair; use `--yes` to auto-accept that replacement prompt.
5. After that, `hero.human.resolve_governance` and `hero.human.ack_summary` can sign operator artifacts that Marshal Hero can verify or that Human Hero can use to clear session-summary inbox items. Use `hero.marshal.continue_session` when you want the idle session to keep running; acknowledgment does not continue it.

`hero.marshal.start_session` is the primary way to run the v4 Marshal Hero autonomy session. It starts from `marshal.objective.dsl`, either the selected `marshal_objective_dsl_path` argument or the default sample `default.marshal.objective.dsl` beside the configured default campaign, and it also accepts optional `marshal_codex_model` plus `marshal_codex_reasoning_effort` overrides for that new session only. Marshal Hero owns the session ledger under `<runtime_root>/.marshal_hero/<marshal_cursor>/`, resolves the Codex launch settings as `start_session override > default.hero.marshal.dsl`, writes a generated session-local `hero.marshal.dsl`, stages an initial bootstrap input checkpoint before any Runtime campaign exists, returns once the detached session runner is launched, and then lets that runner continue autonomously inside the configured authority envelope. The runner cycles through `active -> running_campaign -> active` until it pauses, idles, or finishes, persists one durable `codex_session_id`, records checkpoint artifacts, tracks campaign-launch budget, and reuses the manifest-pinned `resolved_marshal_codex_*` values for fresh and resumed Codex checkpoints instead of rereading mutable defaults. `complete` now parks the session in `idle`, and `hero.marshal.continue_session` stages a new operator checkpoint so the same session can keep going later. Its session-local Config Hero policy takes `config_scope_root` from Marshal Hero defaults directly and enables Config Hero backups for truth-source mutations under `.backups/hero.marshal/<objective_name>/`.

## Runtime Model

Runtime Hero dispatches one immutable campaign snapshot at a time.

- `campaign_cursor` is the public campaign identity and defaults to a compact
  base-36 millisecond cursor like `campaign.mnr38ktb`.
- `job_cursor` is the public child-job identity and is derived from its parent
  `campaign_cursor` (for example `<campaign_cursor>.j.0`).
- campaign execution is sequential only in v1: one `RUN` after another unless
  `hero.runtime.start_campaign(binding_id=...)` narrows the plan to one bind.
- session planning checkpoints are prelaunch plus post-campaign in v1. Marshal Hero chooses the first launch before any campaign exists and later plans again after terminal campaign states, not between individual `RUN` steps.
- jobs remain separate Linux worker processes supervised by detached runners.

Runtime campaign state persists under:

- `<runtime_root>/.campaigns/<campaign_cursor>/runtime.campaign.manifest.lls`
- `<runtime_root>/.campaigns/<campaign_cursor>/campaign.dsl`
- `<runtime_root>/.campaigns/<campaign_cursor>/jobs/`
- `<runtime_root>/.campaigns/<campaign_cursor>/stdout.log`
- `<runtime_root>/.campaigns/<campaign_cursor>/stderr.log`

Marshal-session state persists under:

- `<runtime_root>/.marshal_hero/<marshal_cursor>/marshal.session.manifest.lls`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/marshal.objective.dsl`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/marshal.objective.md`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/marshal.guidance.md`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/hero.marshal.dsl`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/config.hero.policy.dsl`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/marshal.session.memory.md`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/marshal.briefing.md`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/logs/codex.session.stdout.jsonl`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/logs/codex.session.stderr.jsonl`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/human/request.latest.md` when a pause is pending
- `<runtime_root>/.marshal_hero/<marshal_cursor>/human/summary.latest.md` when a session reaches `idle` or `finished`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/human/governance_resolution.latest.json` after signed governance
- `<runtime_root>/.marshal_hero/<marshal_cursor>/human/governance_resolution.latest.sig` after signed governance
- `<runtime_root>/.marshal_hero/<marshal_cursor>/human/clarification_answer.latest.json` after ordinary clarification
- `<runtime_root>/.marshal_hero/<marshal_cursor>/human/summary_ack.latest.json` after Human Hero acknowledges a session summary
- `<runtime_root>/.marshal_hero/<marshal_cursor>/human/summary_ack.latest.sig` after Human Hero acknowledges a session summary
- `<runtime_root>/.marshal_hero/<marshal_cursor>/marshal.session.events.jsonl`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/checkpoints/input.latest.json`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/checkpoints/intent.latest.json`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/checkpoints/mutation.latest.json` when the latest planning checkpoint changed truth-source files
- `<runtime_root>/.marshal_hero/<marshal_cursor>/human/governance_resolutions/governance_resolution.0001.json`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/human/governance_resolutions/governance_resolution.0001.sig`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/checkpoints/input.0001.json`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/checkpoints/intent.0001.json`
- `<runtime_root>/.marshal_hero/<marshal_cursor>/checkpoints/mutation.0001.json` when that planning checkpoint changed truth-source files

`codex.session.stdout.jsonl` stores the raw `codex exec --json` stream for the
latest checkpoint attempt. `codex.session.stderr.jsonl` mirrors stderr as
line-wrapped JSONL entries with `stream`, `line_index`, and `text`.

Runtime job state persists under:

- `<runtime_root>/.campaigns/<campaign_cursor>/jobs/<job_cursor>/runtime.job.manifest.lls`
- `<runtime_root>/.campaigns/<campaign_cursor>/jobs/<job_cursor>/instructions/campaign.dsl`
- `<runtime_root>/.campaigns/<campaign_cursor>/jobs/<job_cursor>/instructions/binding.contract.dsl`
- `<runtime_root>/.campaigns/<campaign_cursor>/jobs/<job_cursor>/instructions/binding.wave.dsl`
- `<runtime_root>/.campaigns/<campaign_cursor>/jobs/<job_cursor>/instructions/*.dsl`
- `<runtime_root>/.campaigns/<campaign_cursor>/jobs/<job_cursor>/stdout.log`
- `<runtime_root>/.campaigns/<campaign_cursor>/jobs/<job_cursor>/stderr.log`
- `<runtime_root>/.campaigns/<campaign_cursor>/jobs/<job_cursor>/job.trace.jsonl`

The staged contract and wave snapshots resolve `% __var ? default %` placeholders before launch, rewrite known downstream DSL-file references to local staged files, and normalize copied instruction filenames by dropping a leading `default.` prefix.
`job.trace.jsonl` is an append-only structured execution trace emitted by the
worker and runtime-binding loop; use `hero.runtime.tail_trace` for the latest
phases without scraping stdout/stderr. Component-owned summaries may append
phase-specific scalar telemetry, such as VICReg epoch loss terms.

STDIO MCP server diagnostics persist separately from Runtime Hero job logs:

- Config Hero `<runtime_root>/.config_hero/mcp/hero_config_mcp/stderr.log`
- Config Hero `<runtime_root>/.config_hero/mcp/hero_config_mcp/events.jsonl`
- Hashimyei Hero `<runtime_root>/.hashimyei/_meta/mcp/hero_hashimyei_mcp/stderr.log`
- Hashimyei Hero `<runtime_root>/.hashimyei/_meta/mcp/hero_hashimyei_mcp/events.jsonl`
- Lattice Hero `<runtime_root>/.hashimyei/_meta/mcp/hero_lattice_mcp/stderr.log`
- Lattice Hero `<runtime_root>/.hashimyei/_meta/mcp/hero_lattice_mcp/events.jsonl`

`stderr.log` mirrors operator-facing diagnostics emitted by the long-lived STDIO
server process. `events.jsonl` is a small append-only lifecycle/direct-tool log
that records startup and one-shot tool execution metadata. `stdout` remains the
STDIO transport and is intentionally not mirrored into a dump file.

Runtime manifests:

- campaign schema: `hero.runtime.campaign.v2`
- job schema: `hero.runtime.job.v3`
- marshal-session schema: `hero.marshal.session.v4`
- marshal input-checkpoint schema: `hero.marshal.input_checkpoint.v3`
- marshal intent-checkpoint schema: `hero.marshal.intent_checkpoint.v3`
- marshal mutation-checkpoint schema: `hero.marshal.mutation_checkpoint.v3`
- human governance-resolution schema: `hero.human.governance_resolution.v3`
- human clarification-answer schema: `hero.human.clarification_answer.v3`
- human summary-ack schema: `hero.human.summary_ack.v3`

The v4 Marshal session keeps Codex shell access read-only while attaching bounded MCP tools. Marshal Hero starts from `marshal.objective.dsl`, resolves its `campaign_dsl_path`, `objective_md_path`, and `guidance_md_path`, resolves the session's Codex model/effort once at launch, copies the authored files into the session ledger, points `objective_root` at the truth-source objective bundle under `src/config/instructions/objectives/...`, writes the session-local `hero.marshal.dsl` plus `config.hero.policy.dsl`, builds `marshal.briefing.md`, and stages a Codex planning checkpoint onto the detached session runner. Codex returns an intent object with `launch_campaign | pause_for_clarification | request_governance | complete | terminate`. `launch_campaign` carries `mode = run_plan | binding`, optional `binding_id`, `reset_runtime_state`, and `requires_objective_mutation`; `pause_for_clarification` carries `clarification_request.request`; `request_governance` carries `kind = authority_expansion | launch_budget_expansion | policy_decision`, an operator-facing request, and an optional typed delta. `complete` means the current objective is satisfied for now, so Marshal Hero records a summary and parks the session as `idle` until `hero.marshal.continue_session` supplies a new operator instruction. If `launch.requires_objective_mutation=true` but the planning checkpoint produced no objective-local mutation summary, Marshal Hero rejects the launch instead of silently rerunning unchanged truth sources, preserves the attempted checkpoint metadata, and parks the session as `idle` with `finish_reason=failed` so the operator can review the summary and continue after fixing the cause. Marshal Hero remains the only component that can launch the next campaign. During a planning checkpoint Codex may use session-scoped `hero.config.objective.*`, optionally `hero.config.default.*` when authority scope permits, `hero.runtime.get_*`/tail tools, and `hero.lattice.*` read tools directly against truth-source config roots and persisted runtime evidence. Objective-local truth-source mutations are audited with per-checkpoint mutation summaries under `checkpoints/mutation.*.json`. Human Hero answers ordinary clarification with `hero.human.answer_request` and resolves governance with signed `grant | deny | clarify | terminate`, while `hero.marshal.resume_session` keeps the same `codex_session_id` moving across checkpoints and `hero.marshal.continue_session` reopens idle sessions.

Marshal-session constitution:

- See [MARSHAL_SESSION.md](/cuwacunu/src/main/hero/MARSHAL_SESSION.md) for the short statement of sovereignty, mutability, decision boundaries, and prohibitions.

## Lattice Notes

Lattice is healthiest as a fact index plus query-time view engine.

- `hero.lattice.get_view` returns a query-time derived transport, not a persisted runtime report fragment.
- `hero.lattice.get_fact` returns an assembled complete fact bundle for one component canonical path and selector context.
- persisted fragments remain strict runtime `.lls` facts emitted by their owning components.
- persisted reports should be read as latest semantic facts by default; historical retrieval is an explicit `wave_cursor` query, not the primary report model.
- fact tool outputs now foreground `canonical_path`, semantic taxa, and context summaries.
- fact retrieval is component/canonical-path centric; correlation keys such as
  `wave_cursor` remain view selectors, while persisted report meaning now lives
  in the flat header `schema`, `semantic_taxon`, `canonical_path`, `binding_id`,
  `wave_cursor`, with optional `source_runtime_cursor` on source-selected
  reports.
- persisted runtime reports are now documented around the flat header
  `schema`, `semantic_taxon`, `canonical_path`, `binding_id`, `wave_cursor`, optional
  `source_runtime_cursor`, plus payload keys.
- normal read tools synchronize the lattice catalog to the current runtime
  store before querying; `hero.lattice.refresh(reingest=true)` remains the
  explicit force-rebuild tool.

For runtime report query surfaces:

- `binding_id` is the primary runtime selection field carried inside report context
- public `wave_cursor` uses readable `<run>.<epoch>.<batch>` form when a historical context is explicitly requested
- campaign and job lineage stay in Runtime Hero campaign/job state plus run manifests; they are adjacent to report facts, not part of the report header

Current derived view kinds:

- `entropic_capacity_comparison`: compares `source.data` facts against `embedding.network` facts for one `wave_cursor`, with optional `canonical_path` narrowing and optional `contract_hash` filtering
- `family_evaluation_report`: serializes runtime reports for one tsiemene family and one `contract_hash`, defaulting to the latest coherent bundle per family member. Optional `wave_cursor` requests a historical context instead. Ranking decisions stay client-owned; this view only exposes the evidence transport.

## Dev Reset

`hero.config.dev_nuke_reset` removes:

- runtime dump roots
- Runtime Hero `<runtime_root>/.campaigns`
- Marshal Hero `<runtime_root>/.marshal_hero`
- Human Hero `<runtime_root>/.human_hero`
- Hashimyei/Lattice catalog files

It uses the saved global config on disk and fails fast while active campaigns or
jobs still exist under `<runtime_root>/.campaigns`.

When `dev_nuke_reset_backup_enabled=true`, it first moves those targeted runtime
roots and catalog files into
`<runtime_root>/../.backups/hero.runtime_reset/<runtime_root-name>/<stamp>/`.
