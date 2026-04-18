/*
  vicreg.solo.test.marshal.dsl
  ===========================
  Marshal Hero entrypoint for vicreg.solo.test.

  Fresh sessions should inherit the current source objective and
  local guidance from here rather than relying on older session-local
  snapshots.
  Optional:
    marshal_session_id:str = some.stable.marshal.id
*/
campaign_dsl_path:path = iitepi.campaign.dsl
objective_md_path:path = vicreg.solo.test.objective.md
guidance_md_path:path = vicreg.solo.test.guidance.md
objective_name:str = vicreg.solo.test
