/*
  vicreg.solo.settings_optimize.marshal.dsl
  =======================================
  Marshal Hero starts from this objective root, then launches the referenced
  campaign through Runtime Hero and applies the referenced objective and
  guidance markdown during review.
  Optional:
    marshal_session_id:str = some.stable.marshal.id
*/
campaign_dsl_path:path = iitepi.campaign.dsl
objective_md_path:path = vicreg.solo.settings_optimize.objective.md
guidance_md_path:path = ../../defaults/default.runtime.operative.guidance.md
objective_name:str = runtime.operative.vicreg.solo.settings_optimize
