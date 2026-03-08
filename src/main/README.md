# Cuwacunu Runtime Entrypoints

This tree is reserved for production and operator-facing executables
(`main()` entrypoints) and local test dispatchers under `src/tests/`.

Current layout:
- `hero/`: HERO configuration assistants (`ask`, `fix`, future MCP bridges).
- `tools/`: operational utilities (for example secure key setup tooling).
- `tests/`: benchmark/functional test dispatch tree.

Build outputs:
- runtime utility binaries are emitted under `.build/hero/`.
- current binaries include:
  - `.build/hero/hero_config_mcp`
  - `.build/hero/secure_key_setup`

Build from `src/`:

```bash
make main
```
