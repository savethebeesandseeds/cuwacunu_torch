# HERO MCP Runtime Guide

This directory builds five stdio MCP servers:

- `/cuwacunu/.build/hero/hero_config_mcp`
- `/cuwacunu/.build/hero/hero_hashimyei_mcp`
- `/cuwacunu/.build/hero/hero_lattice_mcp`
- `/cuwacunu/.build/hero/hero_runtime_mcp`
- `/cuwacunu/.build/hero/hero_super_mcp`

## Build

```bash
make -C /cuwacunu/src/main/hero -j12 all
```

## Defaults

All five HERO binaries default to:

- global config path: `/cuwacunu/src/config/.config`
- unencrypted catalog mode for Hashimyei/Lattice

Runtime paths are resolved from `[REAL_HERO]` pointers in that global config.

## Direct One-Shot Tool Calls

Each HERO MCP binary supports direct tool invocation:

```bash
/cuwacunu/.build/hero/hero_config_mcp \
  --tool hero.config.status \
  --args-json '{}'

/cuwacunu/.build/hero/hero_hashimyei_mcp \
  --tool hero.hashimyei.list \
  --args-json '{}'

/cuwacunu/.build/hero/hero_lattice_mcp \
  --tool hero.lattice.list_facts \
  --args-json '{}'

/cuwacunu/.build/hero/hero_runtime_mcp \
  --tool hero.runtime.list_campaigns \
  --args-json '{}'

/cuwacunu/.build/hero/hero_super_mcp \
  --tool hero.super.list_loops \
  --args-json '{}'
```

Human-friendly discovery:

```bash
hero_config_mcp --list-tools
hero_hashimyei_mcp --list-tools
hero_lattice_mcp --list-tools
hero_runtime_mcp --list-tools
hero_super_mcp --list-tools
```

## Ownership Boundaries

- `Config Hero` owns config inspection, edits, validation, save/reload, and developer reset.
- `Hashimyei Hero` owns identity/catalog lineage surfaces.
- `Lattice Hero` owns run/report query surfaces.
- `Runtime Hero` owns detached campaign dispatch, campaign/job tracking, log tails, and stop/reconcile controls.
- `Super Hero` owns the Codex loop, loop ledger, review packet/decision cycle, and bounded campaign continuation decisions.

`jkimyei` is not a separate Hero. It remains the training/runtime facet inside wikimyei components and their DSLs.

## Tool Surfaces

Config MCP:

- `hero.config.status`
- `hero.config.schema`
- `hero.config.show`
- `hero.config.get`
- `hero.config.set`
- `hero.config.default.read`
- `hero.config.default.replace`
- `hero.config.objective_dsl.read`
- `hero.config.objective_dsl.replace`
- `hero.config.objective_campaign.read`
- `hero.config.objective_campaign.replace`
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
- `hero.config.default.replace`, `hero.config.objective_dsl.replace`, `hero.config.objective_campaign.replace`, `hero.config.save`, `hero.config.rollback`, and `hero.config.dev_nuke_reset` require `allow_local_write=true`.
- persisted config writes, `instructions/defaults/*.dsl` writes, mutable objective-local `.dsl` writes, objective-local `campaign.dsl` writes, and `hero.config.dev_nuke_reset` targets must stay inside `write_roots`.
- `hero.config.objective_dsl.read/replace` exclude source `campaign.dsl` and source `super.objective.dsl`.
- `hero.config.objective_campaign.read/replace` are the dedicated path for source objective-local `campaign.dsl` files.

Hashimyei MCP:

- `hero.hashimyei.list`
- `hero.hashimyei.get_component_manifest`
- `hero.hashimyei.get_founding_dsl_bundle`
- `hero.hashimyei.update_rank`
- `hero.hashimyei.reset_catalog`

Hashimyei is the component identity/discovery layer.
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
and `super_loop_id`.

- `binding_id` narrows execution to one declared bind instead of the default `RUN` plan
- `campaign_dsl_path` overrides the configured source campaign for that launch
- `super_loop_id` links the launched campaign to an existing Super Hero loop ledger; this is the linkage field Super Hero uses when it asks Runtime Hero to launch the next campaign

Super MCP:

- `hero.super.start_loop`
- `hero.super.list_loops`
- `hero.super.get_loop`
- `hero.super.resume_loop`
- `hero.super.stop_loop`

Human MCP:

- `hero.human.list_requests`
- `hero.human.get_request`
- `hero.human.respond`

When `hero_human_mcp` is invoked with no tool/list flags on a tty, it opens a simple ncurses operator console for pending requests instead of waiting for JSON-RPC on stdin.

`hero.super.start_loop` is now the primary way to run a Codex supervision loop. It starts from `super.objective.dsl`, either the selected `super_objective_dsl_path` argument or the default sample `default.super.objective.dsl` beside the configured default campaign. Super Hero owns the loop ledger under `<runtime_root>/.super_hero/<loop_id>/`, runs an initial Codex review before any Runtime campaign exists, lets that review decide the first launch, then waits for terminal campaign states, invokes Codex again with bounded Config/Runtime/Lattice MCP tools attached, validates the returned control decision, and decides whether to stop or launch the next campaign. Its loop-local Config Hero policy now takes `config_scope_root` from Super Hero defaults directly instead of inferring it from Config Hero defaults, while enabling Config Hero backups for truth-source mutations under `.backups/hero.super/<objective_name>/`.

## Runtime Model

Runtime Hero dispatches one immutable campaign snapshot at a time.

- `campaign_cursor` is the public campaign identity.
- `job_cursor` is the public child-job identity and is derived from its parent
  `campaign_cursor` (for example `<campaign_cursor>.job.0000`).
- campaign execution is sequential only in v1: one `RUN` after another unless
  `hero.runtime.start_campaign(binding_id=...)` narrows the plan to one bind.
- super-loop review boundaries are prelaunch plus post-campaign in v1. Super Hero chooses the first launch before any campaign exists and later reviews after terminal campaign states, not between individual `RUN` steps.
- jobs remain separate Linux worker processes supervised by detached runners.

Runtime campaign state persists under:

- `<runtime_root>/.campaigns/<campaign_cursor>/campaign.lls`
- `<runtime_root>/.campaigns/<campaign_cursor>/campaign.dsl`
- `<runtime_root>/.campaigns/<campaign_cursor>/jobs/`
- `<runtime_root>/.campaigns/<campaign_cursor>/stdout.log`
- `<runtime_root>/.campaigns/<campaign_cursor>/stderr.log`

Super-loop state persists under:

- `<runtime_root>/.super_hero/<loop_id>/loop.lls`
- `<runtime_root>/.super_hero/<loop_id>/super.objective.dsl`
- `<runtime_root>/.super_hero/<loop_id>/super.objective.md`
- `<runtime_root>/.super_hero/<loop_id>/super.guidance.md`
- `<runtime_root>/.super_hero/<loop_id>/config.hero.policy.dsl`
- `<runtime_root>/.super_hero/<loop_id>/memory.md`
- `<runtime_root>/.super_hero/<loop_id>/super.briefing.md`
- `<runtime_root>/.super_hero/<loop_id>/logs/codex.session.log`
- `<runtime_root>/.super_hero/<loop_id>/human/request.latest.md` when needed
- `<runtime_root>/.super_hero/<loop_id>/human/response.latest.json` after Human Hero response
- `<runtime_root>/.super_hero/<loop_id>/human/response.latest.sig` after Human Hero response
- `<runtime_root>/.super_hero/<loop_id>/events.jsonl`
- `<runtime_root>/.super_hero/<loop_id>/reviews/latest.json`
- `<runtime_root>/.super_hero/<loop_id>/decisions/latest.json`
- `<runtime_root>/.super_hero/<loop_id>/human/responses/human_response.0001.json`
- `<runtime_root>/.super_hero/<loop_id>/human/responses/human_response.0001.sig`
- `<runtime_root>/.super_hero/<loop_id>/reviews/review_packet.0001.json`
- `<runtime_root>/.super_hero/<loop_id>/decisions/decision.0001.json`

Runtime job state persists under:

- `<runtime_root>/.campaigns/<campaign_cursor>/jobs/<job_cursor>/job.lls`
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
phases without scraping stdout/stderr.

Runtime manifests:

- campaign schema: `hero.runtime.campaign.v2`
- job schema: `hero.runtime.job.v3`
- super-loop schema: `hero.super.loop.v1`
- super review packet schema: `hero.super.review_packet.v1`
- super decision schema: `hero.super.decision.v1`

The primary v1 super loop keeps Codex shell access read-only while attaching bounded MCP tools. Super Hero starts from `super.objective.dsl`, resolves its `campaign_dsl_path`, `objective_md_path`, and `guidance_md_path`, copies those authored files into the loop ledger, points `objective_root` at the truth-source objective bundle under `src/config/instructions/objectives/...`, writes the loop-local Config Hero policy `config.hero.policy.dsl`, builds `super.briefing.md`, invokes `codex exec -C [GENERAL].repo_root`, validates `control_kind = continue | stop | need_human` plus a bounded `next_action` object with `kind = none | default_plan | binding`, and remains the only component that can launch the next campaign. `stop` and `need_human` use `next_action.kind = none`; `continue` uses `default_plan` or `binding`, and always carries `next_action.reset_runtime_state`. During that review session Codex may use loop-scoped `hero.config.objective_dsl.read/replace`, `hero.config.objective_campaign.read/replace`, `hero.runtime.get_*`/tail tools, and `hero.lattice.*` read tools directly against that truth-source objective bundle and persisted runtime evidence. Runtime Hero still snapshots campaign inputs on every launch, so execution remains deterministic even though Super Hero mutations survive runtime resets. Those truth-source mutations are now backed by Config Hero backups under `.backups/hero.super/<objective_name>/`. The source `super.objective.dsl` remains immutable loop constitution, while campaign edits are now explicit through the dedicated objective_campaign tools. Only one non-terminal Super loop may own a given `super.objective.dsl` at a time. When Codex returns `need_human`, Super Hero writes `human/request.latest.md` and pauses. Human Hero is the operator-facing responder: it can be used via `hero.human.*` MCP tools or run with no arguments on a tty, writes a signed `human_response*.json` plus detached `.sig`, and asks `hero.super.resume_loop` to verify the attestation before the loop continues.

Super-loop constitution:

- See [SUPER_LOOP.md](/cuwacunu/src/main/hero/SUPER_LOOP.md) for the short statement of sovereignty, mutability, decision boundaries, and prohibitions.

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
- Super Hero `<runtime_root>/.super_hero`
- Hashimyei/Lattice catalog files

It uses the saved global config on disk and fails fast while active campaigns or
jobs still exist under `<runtime_root>/.campaigns`.
