/*
  default.hero.config.dsl
  ===============
  Purpose:
    Typed deterministic configuration for Config HERO and its MCP workflows.
    NOTE:
      This file configures Config HERO, the configuration-facing HERO
      instrument. Config HERO exists to inspect, validate, diff, back up,
      roll back, and mutate configuration safely through tools instead of raw
      edits.

  Format:
    <key>(domain):<type> = <value>   # optional inline comment

  Critical policy:
    - protocol_layer:str = STDIO | HTTPS/SSE
    - HTTPS/SSE is intentionally unsupported in this phase.
      Selecting HTTPS/SSE must fail fast; STDIO is required.
    - allow_local_write gates config/default/objective/optim filesystem mutation.
    - default_roots lists where hero.config.default.* may operate.
    - objective_roots lists where hero.config.objective.* may operate.
    - GENERAL.tsodao_dsl_filename resolves the TSODAO hidden_root used by
      hero.config.optim.*.
    - allowed_extensions constrains which file extensions those tools may touch.
      Listing and reading follow that policy directly; create/replace still
      depend on file-family validation support.
    - write_roots constrains saved config writes, default/objective mutation
      targets, and plaintext optim mutation targets when allow_local_write=true.
    - optim_backup_* controls encrypted TSODAO checkpoints for
      hero.config.optim.create/replace/delete/rollback.
    - dev_nuke_reset_backup_enabled snapshots targeted runtime state into
      <runtime_root>/../.backups/hero.runtime_reset/<runtime_root-name>/<stamp>
      before hero.config.dev_nuke_reset clears it.
    - dev_nuke_reset uses runtime roots resolved from the saved global config
      and is not gated by allow_local_write or write_roots.
*/

protocol_layer[STDIO|HTTPS/SSE]:str = STDIO # STDIO | HTTPS/SSE (HTTPS/SSE not implemented yet)
config_scope_root:str = /cuwacunu/src/config
allow_local_read:bool = true
allow_local_write:bool = false
default_roots:str = /cuwacunu/src/config/instructions/defaults
objective_roots:str = /cuwacunu/src/config/instructions/objectives
allowed_extensions:str = .dsl,.md
write_roots:str = /tmp
backup_enabled:bool = true
backup_dir:str = /cuwacunu/.backups/hero.config
backup_max_entries(1,+inf):int = 20
optim_backup_enabled:bool = true
optim_backup_dir:str = /cuwacunu/.backups/hero.config.optim
optim_backup_max_entries(1,+inf):int = 20
dev_nuke_reset_backup_enabled:bool = true
