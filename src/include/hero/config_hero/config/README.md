# Hero Config Contracts

This folder contains Config Hero internal contract helpers.

The public MCP-facing entry headers remain at:

```text
src/include/hero/config_hero/hero_config.h
src/include/hero/config_hero/hero_config_tools.h
```

The store contract lives here so Config Hero follows the same root-entry plus
domain-subfolder shape as Runtime, Lattice, and Marshal.
