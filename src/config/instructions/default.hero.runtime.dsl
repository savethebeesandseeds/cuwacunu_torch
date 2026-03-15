/*
  default.hero.runtime.dsl
  ========================
  Purpose:
    Deterministic default settings for hero_runtime_mcp.
*/

campaigns_root:str = /cuwacunu/.runtime/campaigns
/* Child jobs persist under campaigns_root/<campaign_cursor>/jobs/<job_cursor>/ */
main_campaign_binary:str = /cuwacunu/.build/hero/main_campaign
config_folder:str = /cuwacunu/src/config
campaign_grammar_filename:str = /cuwacunu/src/config/bnf/iitepi.campaign.bnf
tail_default_lines:uint = 120
max_active_campaigns:uint = 1
