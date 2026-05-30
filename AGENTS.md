# Project Overview
This repository targets deterministic C/C++ runtime components for market data,
training/inference pipelines, and DSL-driven system configuration.

# Codebase Instructions
- Prefer C++20.
- Use clang-format with LLVM style.
- Avoid Python unless explicitly requested.
- Use Makefiles instead of CMake when possible.
- Prefer `rg` for search.

# Goal Usage
- Keep `/goals` finite and scoped to a concrete, achievable objective.
- Avoid forever goals, broad umbrella goals, or indefinite monitoring goals; split
  broad work into bounded milestones with clear completion criteria.

# Build Commands
- Do not run/build when confidence in changes is high.
- When building, use `make -j12` (builds are long in this environment).
- Deleting .o objects before rebuild is allowed, don't need to ask for confirmation.
- First building individual targets might trigget the build of all the libs, so is better to first make lib and then make the final target. 
- iinuji_cmd fast path (for interface edits): the `make` dependency graph in `src/main` forces an `autoprep` pass before many targets, which can trigger full lib sync/revalidation.
  - Use this for rapid edit/build cycles:
    - `make -C src/main/interface AUTO_PREP=0 build-cuwacunu-cmd`
    - `make -C src/main/interface AUTO_PREP=0 run-cuwacunu-cmd`
  - For quick install of just the interface binary: `make -C src/main/interface AUTO_PREP=0 install-cuwacunu-cmd`
  - Avoid `make -C src/main AUTO_PREP=0 install` when you only need `iinuji_cmd`; `install` in `src/main` still routes through `link -> autoprep`.
  - Return to default `AUTO_PREP` before broader release/CI-facing validation.

# Test Commands
- Run only the smallest relevant tests for touched behavior.
- If full test execution is too expensive, state clearly what was not run.
- Build and test only when mayor milestones have been reached.

# Code Style
- Keep changes minimal and local to the requested scope.
- Leaving legacy ghosts in the code is strongly discouraged.

# HERO MCP Servers
- Codex MCP registrations (stdio):
  - `hero-config` -> `/cuwacunu/.build/hero/hero_config.mcp`
  - `hero-runtime` -> `/cuwacunu/.build/hero/hero_runtime.mcp`
  - `hero-lattice` -> `/cuwacunu/.build/hero/hero_lattice.mcp`
  - `hero-marshal` -> `/cuwacunu/.build/hero/hero_marshal.mcp`
- MCP usage preference:
  - For Hero runtime/config questions (Runtime, Lattice, Marshal, or Config behavior), use MCP calls first before reading local source files.
  - Prefer direct tool invocations (`hero.runtime.*`, `hero.config.*`, `hero.lattice.*`, `hero.marshal.*`) and return MCP output as the source of truth.
  - Only inspect source files when MCP is unavailable or fails (hard error), and only then.
- Preferred MCP args for all four servers:
  - `--global-config /cuwacunu/src/config/.config`
- Fresh Config/Runtime/Lattice/Marshal Hero policy comes from `[HERO]` DSL path pointers in
  `src/config/.config`:
  - `config_hero_dsl_path`
  - `runtime_hero_dsl_path`
  - `lattice_hero_dsl_path`
  - `marshal_hero_dsl_path`
- Marshal Hero has a minimal symmetry policy at `src/config/hero.marshal.dsl`;
  its public tool registry lives under `src/include/hero/marshal_hero`, and
  execution authority still belongs to Runtime Hero policy.
- Runtime contract:
  - Runtime tools: `hero.runtime.*`
  - Config tools: `hero.config.*`
  - Lattice tools: `hero.lattice.*`
  - Marshal tools: `hero.marshal.*`
- Lattice MCP catalogs run unencrypted by policy; no passphrase preflight is required.
- Secret-independent smoke tests for Lattice should use temporary
  catalogs under `/tmp/hero_mcp_smoke/...`.

# Security Constraints
- Do not commit secrets.
- Treat key files and secret paths as sensitive material.

# PR Rules
- Update `.man` files if you add or change behavior of a configuration setting.
- Keep config docs in `src/config/README.md` and `src/main/hero/README.md`
  aligned with actual runtime behavior.

# Known Pitfalls
- Ensure `hero.config.dsl`, `hero.runtime.dsl`, `hero.lattice.dsl`, and
  `hero.marshal.dsl` keep
  `protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO` for current MCP runtime.
- `HTTPS/SSE` is intentionally not implemented yet and must fail fast.
