/*
  hero.config.dsl
  ===============
  Purpose:
    Typed key-value runtime backend config for HERO MCP workflows.

  Format:
    <key>:<type> = <value>   # optional inline comment

  Critical policy:
    - mode:str = openai | selfhosted
    - selfhosted is intentionally disabled in this phase.
      Selecting selfhosted must fail fast with:
      "isufficient founds for self hosted model deployment, please change mode to openai."
    - protocol_layer:str = STDIO | HTTPS/SSE
    - HTTPS/SSE is intentionally disabled in this phase.
      Selecting HTTPS/SSE must fail fast and require STDIO.
*/

mode:str = openai # openai | selfhosted
protocol_layer:str = STDIO # STDIO | HTTPS/SSE (HTTPS/SSE not implemented yet)
transport:str = curl # curl only in current phase
endpoint:str = https://api.openai.com/v1/responses
auth_token_env:str = OPENAI_API_KEY
model:str = gpt-5-codex
reasoning_effort:str = medium # low | medium | high
temperature:float = 0.10
top_p:float = 1.00
max_output_tokens:int = 1400
timeout_ms:int = 30000
connect_timeout_ms:int = 10000
retry_max_attempts:int = 3
retry_backoff_ms:int = 700
verify_tls:bool = true
config_scope_root:str = /cuwacunu/src/config
allow_local_read:bool = true
allow_local_write:bool = false
write_roots:str = /tmp
backup_enabled:bool = true
backup_dir:str = /cuwacunu/src/config/backups/hero.config
backup_max_entries:int = 20
