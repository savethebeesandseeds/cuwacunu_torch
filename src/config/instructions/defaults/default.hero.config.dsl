/*
  default.hero.config.dsl
  ===============
  Purpose:
    Typed key-value deterministic config for HERO MCP workflows.
    NOTE:
      This file configures Config HERO, one instrument inside the broader HERO
      category. Config HERO exists to inspect, validate, diff, backup, and
      rollback configuration changes safely through tools instead of raw edits.

  Format:
    <key>(domain):<type> = <value>   # optional inline comment

  Critical policy:
    - protocol_layer:str = STDIO | HTTPS/SSE
    - HTTPS/SSE is intentionally disabled in this phase.
      Selecting HTTPS/SSE must fail fast and require STDIO.
    - allow_local_write gates config/default/objective filesystem mutation.
    - default_roots lists where hero.config.default.* may operate.
    - objective_roots lists where hero.config.objective.* may operate.
    - allowed_extensions constrains which file extensions those tools may touch.
    - write_roots constrains persisted config writes, objective/default file
      mutation targets, and backups when allow_local_write=true.
    - dev_nuke_reset uses runtime roots resolved from the saved global config
      and is not gated by allow_local_write/write_roots.
*/

protocol_layer[STDIO|HTTPS/SSE]:str = STDIO # STDIO | HTTPS/SSE (HTTPS/SSE not implemented yet)
config_scope_root:str = /cuwacunu/src/config
allow_local_read:bool = true
allow_local_write:bool = false
default_roots:str = /cuwacunu/src/config/instructions/defaults
objective_roots:str = /cuwacunu/src/config/instructions/objectives
allowed_extensions:str = .dsl
write_roots:str = /tmp
backup_enabled:bool = true
backup_dir:str = /cuwacunu/.backups/hero.config
backup_max_entries(1,+inf):int = 20
