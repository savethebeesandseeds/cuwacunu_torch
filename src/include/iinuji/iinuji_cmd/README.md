# cuwacunu.cmd

`cuwacunu.cmd` is the Hero-first terminal shell for cuwacunu.

Primary interface entrypoint:
- `src/main/interface/cuwacunu_cmd.cpp`
- output binary: `.build/interface/cuwacunu.cmd`

Focused smoke harness:
- `src/tests/bench/iinuji/test_iinuji_cmd.cpp`

## Current layout

Core:
- `src/include/iinuji/iinuji_cmd/state.h`
- `src/include/iinuji/iinuji_cmd/commands.h`
- `src/include/iinuji/iinuji_cmd/app.h`
- `src/include/iinuji/iinuji_cmd/views.h`

Shared helpers:
- `src/include/iinuji/iinuji_cmd/views/common.h`
- `src/include/iinuji/iinuji_cmd/views/ui.h`
- `src/include/hero/human_hero/hero_human_tools.h`

First-class shell screens:
- `src/include/iinuji/iinuji_cmd/views/workbench/{state.h,commands.h,view.h,app.h}`
- `src/include/iinuji/iinuji_cmd/views/runtime/{state.h,commands.h,view.h,app.h}`
- `src/include/iinuji/iinuji_cmd/views/logs/{state.h,view.h,app.h}`
- `src/include/iinuji/iinuji_cmd/views/config/{state.h,commands.h,view.h,app.h}`

The removed specialist-screen helpers are no longer part of the `iinuji_cmd` shell surface or its header tree.

## State model

`CmdState` holds:
- global shell state: active screen, running flag, command line, help overlay
- `WorkbenchState`: Human Hero operator workbench feed, focus, selection, and status
- `RuntimeState`: session-linked runtime inventory, detail mode, selection, status
- `ShellLogsState`
- `ConfigState`

The default startup screen is `Home`.

## Command model

`commands.h` is canonical-first and Hero-first.

Canonical path registry + dispatch lives in:
- `src/include/iinuji/iinuji_cmd/commands/iinuji.paths.def`
- `src/include/iinuji/iinuji_cmd/commands/iinuji.paths.h`
- `src/include/iinuji/iinuji_cmd/commands/iinuji.path.handlers.h`
- `src/include/iinuji/iinuji_cmd/commands/iinuji.command.aliases.h`

Primary shell calls:
- `iinuji.screen.home()`
- `iinuji.screen.workbench()`
- `iinuji.screen.runtime()`
- `iinuji.screen.lattice()`
- `iinuji.screen.logs()`
- `iinuji.screen.config()`
- `iinuji.refresh()`
- `iinuji.show.workbench()`
- `iinuji.show.runtime()`
- `iinuji.show.logs()`
- `iinuji.show.config()`

Direct aliases accept `home` for the F1 Home screen and `workbench` for the F2
Workbench screen.

Workbench-specific calls:
- `iinuji.workbench.refresh()`

Runtime-specific calls:
- `iinuji.runtime.refresh()`
- `iinuji.runtime.detail.next()`
- `iinuji.runtime.row.prev()`
- `iinuji.runtime.row.next()`
- `iinuji.runtime.item.prev()`
- `iinuji.runtime.item.next()`

Config calls are file-centered:
- `iinuji.config.files()`
- `iinuji.config.file.index.n1()`
- `iinuji.config.file.id.<token>()`

Removed specialist-screen shell calls now fail as unknown commands by design.

## Workbench screen

The `Workbench` screen is the main operator cockpit for `.runtime/.marshal_hero`.

It reuses the shared Human Hero workbench/action layer from `hero_human_tools` for:
- session collection, request/review detection, and selection recovery
- detail text builders
- operator dialogs
- action routing for clarification answers, governance resolutions, message, pause, resume, terminate, acknowledge, and archive/release

This keeps `iinuji_cmd` aligned with the standalone `hero_human_mcp` tty console instead of maintaining a second independent Human workflow.

## Runtime screen

The `Runtime` screen is a read-only runtime ledger linked to the selected Marshal session when possible.

It shows:
- lane-based runtime browsing for sessions, campaigns, and jobs on the left
- the currently selected runtime chain anchored to the active lane item
- a styled operator overview, lineage, historic child lists, log-tail, or trace-summary detail on the right
- explicit detached/orphan lineage for campaigns without sessions and jobs without campaign/session parents
- wave-contract-binding identity for the selected job

The runtime screen is intentionally observational in this refactor. Runtime mutation remains outside `iinuji_cmd`.

## Interaction model

Primary interaction stays command-first (`cmd> ...`) with fixed shell navigation:
- `F1` Home
- `F2` Workbench
- `F3` Runtime
- `F4` Lattice
- `F8` Shell Logs
- `F9` Config

Workbench screen shortcuts:
- `Up/Down` choose `Inbox` or `Booster` while navigation is focused
- `Enter` or `Right` focuses the selected section from navigation
- `Up/Down` move the selected row in the focused worklist
- `Enter` opens the selected inbox row's action list while `Inbox` is focused
- `Enter` runs the selected guided utility while `Booster` is focused
- `Esc` returns from the worklist to navigation
- `PgUp/PgDn/Home/End` and wheel scroll the focused panel

Workbench inbox:
- `Workbench` is a single operator inbox that merges requests, operator-paused sessions, and pending reviews

Workbench Booster:
- `Booster` is the guided operator utility section inside `Workbench`
- initial actions:
  - `Launch Marshal`
  - `Launch Campaign`
  - `Dev Nuke Reset`
- `Launch Marshal` chooses an existing objective bundle under objective roots or scaffolds a new objective bundle from defaults and opens it in the editor
- existing marshal objectives are shown by name; objectives already owned by an active or resumable session are disabled in the picker
- Marshal launch offers guided Codex model and reasoning-effort choices with a custom override escape hatch
- `Launch Campaign` can use the discovered default DSL, choose an existing DSL, or create a temp copy from the default before launch
- newly created temp campaigns open in the editor before you return to Booster to launch them

Runtime screen shortcuts:
- `Tab` cycle detail mode
- `r` refresh
- `Left/Right` move between `Sessions`, `Campaigns`, and `Jobs`
- `Up/Down` move the selected lane while navigation is focused
- `Enter` focuses the active runtime lane worklist
- `Esc` returns from the worklist to lane navigation
- `Up/Down` move through the selected lane while the worklist is focused
- `h/l` or `[/]` browse backward/forward inside the active lane's linked runtime chain
- `PgUp/PgDn/Home/End` and wheel scroll the focused panel; open detail viewers keep scroll ownership while active

Config screen shortcuts:
- `Enter` focuses file browser from family focus
- `Esc` returns from file browser to family focus
- `Up/Down` move through the selected file browser entry
- `Home/End` jump to first or last visible file
- `Enter` or `e` focuses the embedded file editor
- `Ctrl+S` saves editable `.dsl` buffers through Config Hero replace
- `Esc` or `Ctrl+Q` leaves the embedded editor; dirty buffers require a second press to discard
- `r` reloads the family browser from the active policy

The config screen catalogs files from the global Config Hero policy so every objective folder remains visible. When a Marshal session is selected, saves still use that session's narrower write scope when applicable.

Mouse:
- wheel: vertical scroll in the focused panel
- `Shift`/`Ctrl`/`Alt` + wheel: horizontal scroll in the focused panel where supported

## Design direction

`iinuji_cmd` is now centered on Hero runtime operations instead of legacy specialist navigation. The shell is intentionally small:
- Home for the loading/landing canvas
- Workbench for operator work
- Runtime for runtime evidence and historical lineage
- Lattice for lineage and fact browsing
- Shell Logs for shell diagnostics
- Config for file-centered Hero/runtime config browsing

That keeps the top-level shell deterministic and focused without carrying obsolete screen-specific helper stacks.
