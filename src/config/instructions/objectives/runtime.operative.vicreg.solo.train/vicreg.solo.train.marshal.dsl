/*
  vicreg.solo.train.marshal.dsl
  ===========================
  Marshal Hero entrypoint for the cleaned BTCUSDT-only VICReg train objective.

  Fresh sessions should inherit the current source objective and shared
  guidance from here rather than relying on older session-local snapshots.
  Optional:
    marshal_session_id:str = some.stable.marshal.id
*/
campaign_dsl_path:path = iitepi.campaign.dsl
objective_md_path:path = vicreg.solo.train.objective.md
guidance_md_path:path = ../../defaults/default.runtime.operative.guidance.md
objective_name:str = runtime.operative.vicreg.solo.train
