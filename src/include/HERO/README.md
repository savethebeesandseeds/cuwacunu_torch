# HERO

`HERO` is a standalone include module.

Canonical config surface:
- `src/include/HERO/hero_config/hero.config.h`
- `src/include/HERO/hero_config/hero.config.def`
- Runtime DSL: `src/config/instructions/hero.config.dsl`
- Runtime MAN: `src/config/man/hero.config.man`

Current goal is structure-only:
- `hero.config.ask`
- `hero.config.fix`

Local config MCP executable:
- `src/main/hero/config_hero/hero_config_mcp.cpp`
- build output: `.build/hero/hero_config_mcp`

Hashimyei catalog surface:
- `src/include/HERO/hashimyei/hashimyei_catalog.h`
- `src/main/hero/hashimyei_hero/hero_hashimyei_ingest.cpp`
- `src/main/hero/hashimyei_hero/hero_hashimyei_mcp.cpp`
- build outputs:
  - `.build/hero/hero_hashimyei_ingest`
  - `.build/hero/hero_hashimyei_mcp`

Hashimyei MCP tool highlights:
- run traceability: `scan`, `latest`, `history`, `performance`, `provenance`
- identity-first component queries: `component`, `founding_dsl`
- merged report export: `report` (joins multiple `.kv` payloads)

Future learning targets (declared, not wired yet):
- `tsiemene/tsi.probe`
- `piaabo/torch_compat/network_analytics`
- `piaabo/math_compat/statistics_space`
