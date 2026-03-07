# Cuwacunu Runtime Entrypoints

This tree is reserved for production and operator-facing executables
(`main()` entrypoints), distinct from `src/tests/`.

Current layout:
- `hero/`: HERO configuration assistants (`ask`, `fix`, future MCP bridges).
- `tools/`: operational utilities (for example secure key setup tooling).

Build outputs:
- runtime utility binaries are emitted under `src/cuwacunu/build/`.
- current binaries include:
  - `cuwacunu/build/hero_config_mcp`
  - `cuwacunu/build/secure_key_setup`

Build from `src/`:

```bash
make cuwacunu
```
