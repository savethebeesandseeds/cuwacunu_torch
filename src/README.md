# Source Tree Overview

This `src/` directory contains the core C++ implementation of **Cuwacunu**, an AI-driven trading and portfolio runtime.

At a high level, it combines:
- market/exchange integration
- configuration and DSL-driven runtime wiring
- model training and inference components
- terminal UI and diagnostics

## Main Layout

- `include/`: public headers and module APIs.
- `impl/`: module implementations.
- `tests/`: benchmark-style and functional test targets.
- `config/`: runtime configs, DSL grammars, and instruction files.
- `Makefile`, `Makefile.config`: top-level build orchestration for `src/`.

## Core Modules

- `piaabo`: shared utilities (config parsing, encryption, JSON/files, compat helpers).
- `iitepi`: runtime registry for board/contract/wave configuration and locking.
- `camahjucunu`: data/exchange connectivity and typed trading/domain models.
- `jkimyei`: training setup/orchestration (optimizers, schedulers, losses, schema).
- `wikimyei`: model inference and representation-learning components (e.g. VICReg/MDN).
- `iinuji`: terminal/UI rendering and command views for interactive inspection.
- `tsiemene`: DSL and runtime directive infrastructure used across board/contract flows.

## Build and Test (from `src/`)

```bash
make modules   # build core modules
make all       # build modules + bundles
make tests     # run test dispatcher
```
