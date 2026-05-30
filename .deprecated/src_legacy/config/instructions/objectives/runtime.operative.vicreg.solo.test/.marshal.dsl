/*
  .marshal.dsl
  ============
  Marshal Hero entrypoint for vicreg.solo.test.

  Fresh sessions should inherit the current source objective and
  local guidance from here rather than relying on older session-local
  snapshots.
  Optional:
    marshal_session_id:str = some.stable.marshal.id
*/
campaign_dsl_path:path = .campaign.dsl
objective_md_path:path = .objective.md
guidance_md_path:path = ../../defaults/default.runtime.operative.guidance.md
objective_name:str = runtime.operative.vicreg.solo.test
