/*
  vicreg.solo.settings_optimize.super.dsl
  =======================================
  Super Hero starts from this objective root, then launches the referenced
  campaign through Runtime Hero and applies the referenced objective and
  guidance markdown during review.
  Optional:
    loop_id:str = some.stable.loop.id
*/
campaign_dsl_path:path = iitepi.campaign.dsl
objective_md_path:path = vicreg.solo.settings_optimize.objective.md
guidance_md_path:path = ../../defaults/default.super.guidance.md
objective_name:str = vicreg.solo.settings_optimize
