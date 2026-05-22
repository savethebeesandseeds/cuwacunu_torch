# Config Hero

The fresh Hero migration currently has Config Hero, Runtime Hero, and Lattice
Hero.

Each Hero binary has two entry modes:

- MCP stdio server mode for Codex/tool clients.
- Direct one-shot mode with `--tool <name> --args-json <json>` for local smoke
  tests and scripts.

Build:

```bash
make -C /cuwacunu/src/main/hero build-config-hero
make -C /cuwacunu/src/main/hero build-runtime-hero
make -C /cuwacunu/src/main/hero build-lattice-hero
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
```

The default policy paths are `[HERO].config_hero_dsl_path` and
`[HERO].runtime_hero_dsl_path` and `[HERO].lattice_hero_dsl_path` in
`src/config/.config`, falling back to
`/cuwacunu/src/config/hero.config.dsl` and
`/cuwacunu/src/config/hero.runtime.dsl` and
`/cuwacunu/src/config/hero.lattice.dsl`.

This v2 surface intentionally removes legacy Config Hero responsibilities that
belonged to retired migration layers:

- no Hashimyei identity receipts
- no TSODAO optim file surface
- no Config-owned runtime reset tool
- no default/temp/objective split

Config Hero is now a small policy-controlled config file surface. It can list,
read, write, and delete files under configured roots, while domain-specific DSL
validation remains owned by Ujcamei, Kikijyeba, Wikimyei, and Jkimyei.

Agent workflow:

1. `hero.config.status` checks policy health.
2. `hero.config.map` finds global config path references and missing files.
3. `hero.config.resolve` preflights a target path before read or mutation.
4. `hero.config.read` returns file content plus `sha256`.
5. `hero.config.write` uses `dry_run=true`, then writes with
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
2. `hero.runtime.wave` decodes `kikijyeba.settings.wave.dsl`.
3. `hero.runtime.dry_run` builds and validates the protocol contract through
   `cuwacunu_exec --dry-run` and returns job artifacts.
4. `hero.runtime.list_jobs` and `hero.runtime.get_job` inspect prior job
   directories.
5. `hero.runtime.dev_nuke` previews and, when explicitly enabled, clears the
   runtime artifact root for developer reset workflows.

`hero.runtime.execute` defaults to dry-run. Non-dry-run execution is denied
unless `allow_execute=true`; MODE=train has the additional
`allow_train_execute=true` guard.

`hero.runtime.dev_nuke` also defaults to dry-run. Non-dry-run reset is denied
unless `allow_dev_nuke=true` and, by default, `confirm_dev_nuke=true` is passed.
When `dev_nuke_backup_enabled=true`, it moves active runtime-root entries into
`dev_nuke_backup_root` before they disappear from the active runtime root. It
refuses roots outside `allowed_dev_nuke_roots` and refuses execution while
nonterminal or unknown-status `job.state` files are present.

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
evidence files.

Lattice Hero agent runbook:

1. Call `hero.lattice.status` first.
   Report the active target/split DSL paths, split-policy fingerprint, target
   count, runtime root, fact counts, warnings, and any active identity inferred
   from runtime manifests.
2. Call `hero.lattice.list_targets` to discover target ids. V0 has one active
   target file per global config, but that file may contain many
   `LATTICE_TARGET` blocks.
3. Call `hero.lattice.explain_target` before editing or evaluating a target. It
   does not scan runtime evidence; it explains the compiled proof obligation,
   profile, dependency, cursor-epoch exposure requirements, split protection,
   checkpoint source, and resolved split range.
4. Call `hero.lattice.scan_exposure` before evaluating unfamiliar runtime
   roots. Runtime roots contain job directories with `job.manifest`,
   `job.state`, component reports, checkpoints, `lattice.exposure.fact`, and
   `lattice.checkpoint.fact`. Report scan warnings verbatim. Fact previews
   include `anchor_domain_health`: candidate/accepted graph anchors, skip
   reasons, common/reference key bounds, and source-domain warning level. MDN
   jobs also produce derived `node_exposure` previews from per-node report
   fields, including routed/active/trained/evaluated row counts, valid targets,
   and mean NLL, so agents can inspect node-level target support without
   assuming per-node VICReg support.
5. Call `hero.lattice.evaluate_target` to ask whether a target is satisfied by
   evidence. Call `hero.lattice.plan_target` when the user asks what should run
   next; the returned wave is a recommendation only. Exposure-backed target
   results include `exposure_summaries`: unique cursor coverage, repeated
   cursor-epoch load, and optimizer-step density. These are visibility metrics,
   not automatic failures. Validation evaluation targets use
   `evaluation_metric` coverage from run-mode jobs, so they can be satisfied
   without optimizer mutation or a new checkpoint. Targets may also return
   `warning_results` from `LATTICE_WARN`; triggered warning messages are
   mirrored in the target-level `warnings` list.
   `EVALUATED_CHECKPOINT_SOURCE = latest_satisfying:<target_id>` means the
   evaluation report must prove it loaded that exact MDN checkpoint, with the
   matching representation checkpoint lineage. Suggested waves may include
   model-state input hints such as `INPUT_MDN_CHECKPOINT`, but Runtime Hero
   remains responsible for execution and the runtime report remains the proof.
6. Call `hero.lattice.checkpoint_closure` for checkpoint lineage. A complete
   MDN closure should include the MDN exposure fact and the exact upstream
   VICReg checkpoint producer fact. Missing input lineage must fail closed and
   should be treated as a blocker.

Read-only guard targets may use
`CHECKPOINT_SOURCE = latest_satisfying:<target_id>`. Treat those as checkpoint
inspection over another target's proven artifact, not as separate training
requests. If the source target is unsatisfied, the guard target blocks and
forwards the source target's suggested wave.

When the active runtime root is empty, graph-anchor targets cannot infer active
identity. Pass `protocol_contract_fingerprint`, `graph_order_fingerprint`,
`source_cursor_token`, and component assembly fingerprints explicitly to
`evaluate_target` or `plan_target`, or run a Runtime Hero dry-run first to
produce identity-bearing manifests. Lattice Hero may suggest a wave, but Runtime
Hero remains the only executor.

Agent invariants:

- Lattice Hero is read-only: never use it as an executor or scheduler.
- Runtime Hero is the execution surface for any suggested wave.
- `explain_target` is the source of truth for target language meaning.
- `scan_exposure` is the source of truth for runtime evidence warnings,
  including Ujcamei graph-anchor domain health warnings and derived MDN
  node-exposure previews.
- `evaluate_target` is the source of truth for exposure-load summaries and
  non-blocking target warnings. Repeated exposure is suspicious only when an
  explicit `LATTICE_WARN` policy says it is suspicious.
- Empty runtime roots require explicit active identity for graph-anchor targets.
