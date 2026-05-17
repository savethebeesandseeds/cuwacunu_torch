/*
  default.marshal.objective.dsl
  ===========================
  Machine anchor for a Marshal Hero loop. This file chooses the campaign bundle
  plus separate human-authored objective and guidance markdown for one
  objective.
  Optional:
    marshal_codex_model:str = gpt-5.4
    marshal_codex_reasoning_effort:str = xhigh
    marshal_session_id:str = some.stable.marshal.id
*/
campaign_dsl_path:path = default.iitepi.campaign.dsl
objective_md_path:path = default.marshal.objective.md
guidance_md_path:path = default.runtime.diagnostics.guidance.md
objective_name:str = runtime.diagnostics.marshal
