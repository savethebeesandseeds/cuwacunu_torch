/*
  default.hero.config.dsl
  ===============
  Purpose:
    Typed key-value deterministic config for HERO MCP workflows.
    Current runtime is deterministic-only: ask/fix model actions are disabled.
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
*/

protocol_layer[STDIO|HTTPS/SSE]:str = STDIO # STDIO | HTTPS/SSE (HTTPS/SSE not implemented yet)
config_scope_root:str = /cuwacunu/src/config
allow_local_read:bool = true
allow_local_write:bool = false
write_roots:str = /tmp
backup_enabled:bool = true
backup_dir:str = /cuwacunu/.backups/hero.config
backup_max_entries(1,+inf):int = 20
