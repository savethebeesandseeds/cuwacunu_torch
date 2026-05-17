/*
  .marshal.dsl
  ============
  Marshal Hero entrypoint for `source.lint` refactoring planning.

  This objective is intentionally dormant with respect to Runtime launch. It
  segregates source-code work from runtime-operative and runtime-diagnostics
  objectives until Marshal source mutation authority is decided separately.
*/
campaign_dsl_path:path = .campaign.dsl
objective_md_path:path = .objective.md
guidance_md_path:path = ../../defaults/default.source.lint.guidance.md
objective_name:str = source.lint.refactoring
marshal_codex_model:str = gpt-5.4
marshal_codex_reasoning_effort:str = xhigh
