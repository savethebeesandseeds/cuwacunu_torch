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
- Deleting .o objects before rebuild is allowed, don't need to ask for confirmation.

# Test Commands
- Run only the smallest relevant tests for touched behavior.
- If full test execution is too expensive, state clearly what was not run.

# Code Style
- Keep changes minimal and local to the requested scope.
- Leaving legacy ghosts in the code is strongly discouraged.

# HERO MCP Servers
- Codex MCP registrations (stdio):
  - `hero-config` -> `/cuwacunu/.build/hero/hero_config_mcp`
  - `hero-hashimyei` -> `/cuwacunu/.build/hero/hero_hashimyei_mcp`
  - `hero-lattice` -> `/cuwacunu/.build/hero/hero_lattice_mcp`
- MCP usage preference:
  - For Hero runtime/config questions (Hashimyei, Lattice, or Config behavior), use MCP calls first before reading local source files.
  - Prefer direct tool invocations (`hero.config.*`, `hero.hashimyei.*`, `hero.lattice.*`) and return MCP output as the source of truth.
  - Only inspect source files when MCP is unavailable or fails (hard error), and only then.
- Preferred MCP args for all three servers:
  - `--global-config /cuwacunu/src/config/.config`
- Default runtime policy for Hashimyei/Lattice MCP comes from `[REAL_HERO]` DSL path
  pointers in `src/config/.config`:
  - `config_hero_dsl_filename`
  - `hashimyei_hero_dsl_filename`
  - `lattice_hero_dsl_filename`
- Runtime contract:
  - Config tools: `hero.config.*`
  - Hashimyei tools: `hero.hashimyei.*`
  - Lattice tools: `hero.lattice.*`
- Hashimyei/Lattice MCP catalogs run unencrypted by policy; no passphrase preflight is required.
- Secret-independent smoke tests for Hashimyei/Lattice should use temporary
  catalogs under `/tmp/hero_mcp_smoke/...`.

# Security Constraints
- Do not commit secrets.
- Treat key files and secret paths as sensitive material.

# PR Rules
- Update `.man` files if you add or change behavior of a configuration setting.
- Keep config docs in `src/config/README.md` and `src/main/hero/README.md`
  aligned with actual runtime behavior.

# Known Pitfalls
- Ensure `default.hero.config.dsl` keeps `protocol_layer:str = STDIO` for current MCP runtime.
- `HTTPS/SSE` is intentionally not implemented yet and must fail fast.
