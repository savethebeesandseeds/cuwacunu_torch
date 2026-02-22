# iinuji_cmd

`iinuji_cmd` is the command-first terminal shell for cuwacunu.

Bench bootstrap:
- `src/tests/bench/iinuji/torch_include/test_iinuji_cmd.cpp`

## Current layout

Core:
- `src/include/iinuji/iinuji_cmd/state.h`
- `src/include/iinuji/iinuji_cmd/commands.h`
- `src/include/iinuji/iinuji_cmd/app.h`
- `src/include/iinuji/iinuji_cmd/views.h`

Shared view helpers:
- `src/include/iinuji/iinuji_cmd/views/common.h`
- `src/include/iinuji/iinuji_cmd/views/ui.h`

Per-screen modules (standard):
- `src/include/iinuji/iinuji_cmd/views/home/{state.h,view.h,app.h}`
- `src/include/iinuji/iinuji_cmd/views/board/{state.h,commands.h,view.h,app.h}`
- `src/include/iinuji/iinuji_cmd/views/logs/{state.h,view.h,app.h}`
- `src/include/iinuji/iinuji_cmd/views/tsiemene/{state.h,commands.h,view.h,app.h}`
- `src/include/iinuji/iinuji_cmd/views/data/{state.h,commands.h,view.h,app.h}`
- `src/include/iinuji/iinuji_cmd/views/config/{state.h,commands.h,view.h,app.h}`

`view.h` (singular at root) was removed; `views.h` is the single include entrypoint.

## State model

`CmdState` holds:
- global state: active screen, running flag, command line
- per-screen state blocks:
  - `HomeState`
  - `BoardState` (board decode/validation + selected circuit)
  - `LogsState`
  - `TsiemeneState` (selected tab)
  - `DataState` (observation summary + runtime tensor selection)
  - `ConfigState` (tabs + selected tab)

## Command model

`commands.h` is now canonical-first dispatcher.

Canonical path registry + dispatch lives in:
- `src/include/iinuji/iinuji_cmd/commands/iinuji.paths.def`
- `src/include/iinuji/iinuji_cmd/commands/iinuji.paths.h`
- `src/include/iinuji/iinuji_cmd/commands/iinuji.path.handlers.h`
- `src/include/iinuji/iinuji_cmd/commands/iinuji.command.aliases.h`

Selection/utility helpers live in:
- `views/board/commands.h`
- `views/tsiemene/commands.h`
- `views/data/commands.h`
- `views/config/commands.h`

User command model:
- canonical path form: `iinuji.*()`
- direct aliases are intentionally defined in `iinuji.paths.def` and resolved in `iinuji.command.aliases.h`
- preferred path forms are static and suffix-based where practical:
  - `...index.n1()`
  - `...id.<token>()`

## App model

`app.h` now focuses on:
- ncurses boot and layout tree
- global event loop
- global mouse scroll behavior
- delegating key handling to screen app modules

Screen app logic lives in:
- `views/board/app.h`
- `views/tsiemene/app.h`
- `views/config/app.h`
- `views/data/app.h`

`views/data/app.h` owns:
- dataset/tensor runtime cache for F5
- sampled tensor navigation
- full-screen plot overlay rendering
- F5 arrow navigation model:
  - `Up/Down` selects focused control
  - `Left/Right` changes focused control
  - printable keys are reserved for `cmd>`
- F5 x-axis support (`index` vs `key_value` / `key_type_t`)

## Interaction model

Primary interaction stays command-first (`cmd> ...`).

Shortcuts:
- `F1` home
- `F2` board
- `F8` logs
- `F4` tsiemene
- `F5` data
- `F9` config

Mouse:
- wheel: vertical scroll (both active workspace panels)
- `Shift`/`Ctrl`/`Alt` + wheel: horizontal scroll

## Design direction

This split keeps global concerns small and makes each screen self-contained:
- state local to the screen
- command parsing local to the screen
- render logic local to the screen
- key/app behavior local to the screen

That gives room to grow each screen without expanding monolithic `app.h` and `commands.h` again.
