/*
  .marshal.dsl
  ============
  Marshal Hero entrypoint for evaluating the frozen optimized VICReg bundle on
  discovered compatible symbols without changing the model settings.
  Optional:
    marshal_session_id:str = some.stable.marshal.id
*/
campaign_dsl_path:path = .campaign.dsl
objective_md_path:path = .objective.md
guidance_md_path:path = ../../defaults/default.runtime.operative.guidance.md
objective_name:str = runtime.operative.vicreg.solo.multi_symbol_eval
