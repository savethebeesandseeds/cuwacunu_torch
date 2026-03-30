/*
  vicreg.solo.multi_symbol_eval.super.dsl
  =======================================
  Super Hero entrypoint for evaluating the frozen optimized VICReg bundle on
  discovered compatible symbols without changing the model settings.
  Optional:
    loop_id:str = some.stable.loop.id
*/
campaign_dsl_path:path = iitepi.campaign.dsl
objective_md_path:path = vicreg.solo.multi_symbol_eval.objective.md
guidance_md_path:path = ../../defaults/default.super.guidance.md
objective_name:str = vicreg.solo.multi_symbol_eval
