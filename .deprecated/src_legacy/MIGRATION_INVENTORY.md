# Legacy Migration Inventory

This archive is not active source. It is kept so we can recover useful ideas
without silently carrying old behavior back into the live tree.

## Keep In Mind

The current active system is already stronger in several places:

- Runtime owns execution and durable evidence.
- Lattice owns read-only proof over Runtime evidence.
- Marshal owns bounded coordination and operator explanation.
- Wikimyei now has graph-first, channel-preserving representation and MDN
  surfaces.

Anything migrated from `src_legacy` should preserve those boundaries. Do not
restore old tools by copying them wholesale.

## Worth Rebuilding Later

### TSODAO Hidden-Surface Guard

Legacy files:

- `main/tools/tsodao.cpp`
- `main/tools/tsodao_private_*.cpp`
- `scripts/tsodao_sync_mark.sh`
- `scripts/setup_tsodao_key.sh`
- `config/man/tsodao.man`
- `config/instructions/defaults/tsodao.dsl`

What is useful:

- A safety-first hidden/public authored-surface workflow.
- GPG recipient or symmetric archive sealing.
- Refusal to guess sync direction when plaintext and archive diverge.
- Local sync-state memory under `.runtime/.tsodao/`.
- Git hook integration for pre-commit/pre-push checks.
- A shell prompt mark for hidden-surface attention.

What to change before reintroducing:

- Reframe it as an active tool only if we still need encrypted local authored
  surfaces. It should not be tied to removed Hashimyei concepts.
- Keep secrets and key material out of public docs and review packets.
- Add modern Config/Runtime policy docs and tests before rebuilding.
- Use the current Hero naming and config path conventions.

Likely future home:

- `src/main/tools/tsodao.cpp`
- `src/config/hero.tsodao.dsl` or a non-Hero tool policy DSL
- `src/config/man/tsodao.man`

### Marshal Agentic Session Harness

Legacy files:

- `main/hero/MARSHAL_SESSION.md`
- `include/hero/marshal_hero/marshal_session.h`
- `impl/hero/marshal_hero/marshal_session_workspace.cpp`
- `impl/hero/marshal_hero/hero_marshal_tools_private_*.cpp`
- `include/hero/marshal_hero/hero_marshal_tools.def`
- `config/man/marshal.objective.man`
- `config/bnf/objective.marshal.bnf`
- `config/instructions/objectives/*`

What is useful:

- Durable Marshal sessions with manifest, memory, event log, turn log, and
  checkpoint artifacts.
- Finite launch budgets.
- Pause/resume/reconcile/archive/terminate lifecycle operations.
- Operator message delivery into a live session.
- Codex runner integration as an advisory planning loop.
- Objective-local truth-source bundles with objective markdown, guidance
  markdown, and session-local policy snapshots.

What to change before reintroducing:

- Do not restore the old public tool set as-is. The current public Marshal
  surface should stay small:
  `hero.marshal.status`, `hero.marshal.reach_lattice_target`, and
  `hero.marshal.evaluate`.
- Treat session harnessing as an internal or explicitly experimental layer until
  the deterministic target driver is fully hardened.
- Runtime execution must still go through Runtime Hero. Lattice proof must still
  come from Lattice Hero.
- Codex output must never become proof authority, checkpoint selection, or an
  unbounded scheduler.
- Any future autonomous Marshal must require finite budgets and durable resume
  ledgers.

Likely future shape:

- Keep deterministic Marshal tools public.
- Add an optional internal `marshal_session` engine later.
- If exposed, expose it through one bounded tool rather than the old many-tool
  session surface.

### Objective Bundles And Campaign/Wave Examples

Legacy files:

- `config/instructions/objectives/runtime.operative.vicreg.solo.train/`
- `config/instructions/objectives/runtime.operative.vicreg.solo.test/`
- `config/instructions/objectives/runtime.operative.vicreg.solo.settings_optimize/`
- `config/instructions/objectives/runtime.operative.vicreg.solo.multi_symbol_eval/`
- `config/instructions/objectives/source.lint.*`

What is useful:

- Concrete examples of objective-scoped operator intent.
- Campaign and wave profiles for train/eval loops.
- Resume notes and multi-variant training experiments.
- Source-lint objectives as examples of non-training Marshal work.

What to change before reintroducing:

- Convert examples into current Runtime wave profiles, Marshal driver fixtures,
  or documentation. Do not revive old campaign authority.
- Replace index-only or legacy source assumptions with current split, runtime,
  and lattice evidence conventions.
- Keep examples small and testable.

### VICReg Diagnostics And Training Policy Ideas

Legacy files:

- `include/wikimyei/representation/VICReg/*`
- `impl/wikimyei/representation/VICReg/*`
- `include/tsiemene/tsi.wikimyei.representation.encoding.vicreg_private_*.h`
- `config/man/tsi.wikimyei.representation.encoding.vicreg*.man`
- `config/instructions/defaults/default.tsi.wikimyei.representation.encoding.vicreg*.dsl`

What is useful:

- Network analytics and representation health reporting ideas.
- Epoch summaries with invariance, variance, covariance, optimizer-step, and
  training-policy facts.
- Buffer/SWA policy experiments.
- Transfer-matrix or embedding-sequence diagnostics.

What to change before reintroducing:

- The old fused/node VICReg implementation is not active and should not come
  back.
- Migrate only diagnostics/policy ideas that fit the channel-preserving
  representation path.
- Runtime should emit terminal result facts; Lattice can then warn over them.

### Rendering And Operator UX Helpers

Legacy files:

- `include/iinuji/render/*`
- `include/iinuji/iinuji_cmd/*`

What is useful:

- Terminal rendering primitives for compact operator reports.

What to change before reintroducing:

- Keep the current interface surface minimal.
- Reuse only if Marshal/Runtime reports need stable terminal rendering.

## Probably Do Not Migrate

- Legacy Hashimyei catalogs and private surfaces.
- Legacy Human Hero public session-routing tools.
- Old fused/node-only VICReg front doors.
- Old campaign authority that lets Marshal become a scheduler by default.
- Legacy MDN paths that do not satisfy the active `[B,N,C,De]` contract.

## Next Review Questions

- Do we still need an encrypted authored-surface workflow, or can TSODAO stay
  archived?
- Should future Codex-assisted Marshal work be an internal engine under
  `hero.marshal.evaluate`, or a separate experimental tool?
- Which VICReg diagnostics should become Runtime terminal facts first?
- Which legacy objective examples are still useful as modern target-driver
  fixtures?
