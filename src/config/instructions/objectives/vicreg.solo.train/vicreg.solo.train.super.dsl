/*
  vicreg.solo.train.super.dsl
  ===========================
  Super Hero entrypoint for training the frozen optimized VICReg bundle on
  BTCUSDT only.
  Optional:
    loop_id:str = some.stable.loop.id
*/
campaign_dsl_path:path = iitepi.campaign.dsl
objective_md_path:path = vicreg.solo.train.objective.md
guidance_md_path:path = ../../defaults/default.super.guidance.md
objective_name:str = vicreg.solo.train
