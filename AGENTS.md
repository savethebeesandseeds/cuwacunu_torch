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
- The first action for any requested goal is a goal-scope preflight: decide
  whether the objective is finite, attainable, and has a clear stop condition.
- Reject pursuit of broad, indefinite, or umbrella goals as stated. Ask the user
  to narrow them into a bounded milestone before creating or entering the goal.
- Keep `/goals` finite and scoped to a concrete, achievable objective.
- Avoid forever goals, broad umbrella goals, or indefinite monitoring goals; split
  broad work into bounded milestones with clear completion criteria.
- Do not create a goal unless the user explicitly asks for one.
- Before starting a goal, define a practical completion condition and stop when it
  is satisfied. Do not keep extending the goal with adjacent cleanup or new scope.
- Prefer milestone-sized plans inside goals. Each plan item should cover a broad,
  meaningful phase of work, not a tiny edit or one file read.
- Keep plan updates sparse and useful: update plans when a milestone starts,
  completes, or materially changes. Avoid token-heavy micro-planning.
- For long-running goals, favor the smallest path that achieves the stated
  objective. Defer optional refactors, extra validation, and exploratory work
  unless they are required for correctness.

# Build Commands
- Do not run/build when confidence in changes is high.
- When building, use `make -j12` (builds are long in this environment).
- Deleting .o objects before rebuild is allowed, don't need to ask for confirmation.
- In goals, do not build after every small edit. Build only at major milestones,
  before final handoff when risk justifies it, or when the user explicitly asks.
- Prefer targeted builds over broad builds. If a target may trigger all libraries,
  first make the relevant library target, then make the final target.
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
- Build and test only when major milestones have been reached.
- In goals, avoid repeated validation loops unless a failure requires another
  check. Prefer one focused validation pass near the end of the bounded objective.

# Code Style
- Leaving legacy in the code is strongly discouraged.

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
- MCP registration maintenance:
  - Check registrations with `codex mcp list` when a Hero tool namespace is
    missing from Codex.
  - The active Codex config is normally `/root/.codex/config.toml`.
  - Remove stale Hero MCP registrations whose binaries are not installed.
  - Keep the four active Hero MCPs registered: config, runtime, lattice, and
    marshal.
  - After changing Codex MCP registrations, start a fresh Codex session before
    expecting new `mcp__hero_*` tool namespaces to appear.
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

# Runtime and Training Dispatch
- Marshal is the preferred operator surface for readiness-target dispatch.
  For trainable Lattice targets, use `hero.marshal.reach_lattice_target`
  before Runtime execution. Treat Marshal advice and receipts as the dispatch
  audit trail; Lattice remains the proof authority and Runtime remains the
  executor.
- Do not launch non-dry-run model training with ad hoc `/tmp` configs. Temporary
  configs are acceptable for dry-run/smoke checks only. Training that writes
  checkpoints must use durable repo config, a reviewed durable overlay, or a
  Marshal-materialized handoff that can be inspected later.
- Do not patch `*.jkimyei` checkpoint inputs in `/tmp` for real training. Use
  Marshal/Lattice plan-input materialization or an explicitly reviewed durable
  config change so downstream runs can find the same model-state inputs.
- If Marshal reports `runtime_wave_not_aligned`, stop and align the durable
  Runtime wave/config first. Do not bypass the blocker with direct
  `cuwacunu_exec` execution.
- Direct `cuwacunu_exec` non-dry-run training is an emergency bypass only. It
  requires explicit user authorization and should include Marshal handoff fields
  when available:
  - `--runtime-handoff-id`
  - `--runtime-handoff-digest`
  - `--marshal-target-driver-run-id`
- If a direct run already happened, do not retrain immediately. First check the
  Runtime manifest, state, report, and Lattice checkpoint facts for compatible
  `protocol_contract_fingerprint`, `graph_order_fingerprint`,
  `component_spawn_fingerprint`, source cursor identity, checkpoint path, and
  loaded checkpoint inputs. Then use Marshal/Lattice read-only tools to decide
  whether the artifacts can be reconciled into the official target path.
- A safe readiness-target training preflight is:
  - `hero.marshal.status`
  - `hero.marshal.reach_lattice_target` with `requested_mode=dry_run`,
    `materialize_plan_inputs=true`, and `include_runtime_dry_run=true`
  - align durable Runtime config/wave if Marshal blocks
  - rerun the Marshal dry-run until it accepts the handoff
  - only then execute through the approved Marshal/Runtime path

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
