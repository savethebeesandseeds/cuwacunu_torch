# Project Overview
This repository targets deterministic C/C++ runtime components for market data,
training/inference pipelines, and DSL-driven system configuration.

# Codebase Instructions
- Prefer C++20.
- Use clang-format with LLVM style.
- Avoid Python unless explicitly requested.
- Use Makefiles instead of CMake when possible.
- Prefer `rg` for search.

# Build Commands
- Do not run/build when confidence in changes is high.
- When building, use `make -j12` (builds are long in this environment).

# Test Commands
- Run only the smallest relevant tests for touched behavior.
- If full test execution is too expensive, state clearly what was not run.

# Code Style
- Keep changes minimal and local to the requested scope.
- Leaving legacy ghosts in the code is strongly discouraged.

# Config HERO Policy
- Config HERO is the deterministic configuration instrument.
- Runtime config file: `src/config/instructions/hero.config.dsl`.
- MCP binaries:
  - `.build/hero/hero_config_mcp`
  - `src/config/hero_config_mcp` (installed helper)
- Prefer Config HERO tools for runtime configuration changes:
  - `hero.get`, `hero.set`, `hero.validate`, `hero.diff`/`hero.dry_run`,
    `hero.save`, `hero.backups`, `hero.rollback`.
- This policy applies to configuration work under `src/config`, including:
  - board-level `.config` and `.dsl`,
  - contract `.dsl`,
  - wave `.dsl`,
  - instruction/manifests where values are being tuned.
- Direct raw file edits are allowed when changing schema/grammar/spec itself
  (new keys, parser behavior, docs, templates), not routine value tuning.
- Before persisting config changes, prefer `validate` + `diff` and keep rollback available.

# Security Constraints
- Do not commit secrets.
- Treat key files and secret paths as sensitive material.

# PR Rules
- Update `.man` files if you add or change behavior of a configuration setting.
- Keep config docs in `src/config/README.md` and `src/main/hero/README.md`
  aligned with actual runtime behavior.

# Known Pitfalls
- Ensure `hero.config.dsl` keeps `protocol_layer:str = STDIO` for current MCP runtime.
- `HTTPS/SSE` is intentionally not implemented yet and must fail fast.
