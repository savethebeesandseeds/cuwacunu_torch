/*
  default.hero.marshal.dsl
  Marshal Hero owns the Codex loop, while Runtime Hero executes campaigns.
*/
runtime_hero_binary:path = /cuwacunu/.build/hero/hero_runtime.mcp
config_hero_binary:path = /cuwacunu/.build/hero/hero_config.mcp
lattice_hero_binary:path = /cuwacunu/.build/hero/hero_lattice.mcp
human_hero_binary:path = /cuwacunu/.build/hero/hero_human.mcp
human_operator_identities:path = ../../secrets/real/human_operator_identities
config_scope_root:path = ../..
marshal_codex_binary:path = codex
marshal_codex_model:str = gpt-5.3-codex-spark
marshal_codex_reasoning_effort:str = xhigh
tail_default_lines(1,+inf):int = 120
poll_interval_ms(100,+inf):int = 1000
marshal_codex_timeout_sec(1,+inf):int = 900
marshal_max_campaign_launches(1,+inf):int = 64
