/*
  default.hero.super.dsl
  Super Hero owns the Codex loop, while Runtime Hero executes campaigns.
*/
runtime_hero_binary:path = /cuwacunu/.build/hero/hero_runtime_mcp
config_hero_binary:path = /cuwacunu/.build/hero/hero_config_mcp
lattice_hero_binary:path = /cuwacunu/.build/hero/hero_lattice_mcp
human_hero_binary:path = /cuwacunu/.build/hero/hero_human_mcp
human_operator_identities:path = ../../secrets/real/human_operator_identities
config_scope_root:path = ../..
super_codex_binary:path = codex
tail_default_lines(1,+inf):int = 120
poll_interval_ms(100,+inf):int = 1000
super_codex_timeout_sec(1,+inf):int = 900
super_max_review_turns(1,+inf):int = 8
super_max_campaign_launches(1,+inf):int = 8
