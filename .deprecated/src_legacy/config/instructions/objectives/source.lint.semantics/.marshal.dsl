/*
  .marshal.dsl
  ============
  Marshal Hero entrypoint for `source.lint` semantic naming review.

  This objective is intentionally dormant with respect to Runtime launch. It
  asks Marshal to inspect repository source naming and return improvement
  proposals only, without changing source code.
*/
campaign_dsl_path:path = .campaign.dsl
objective_md_path:path = .objective.md
guidance_md_path:path = ../../defaults/default.source.lint.guidance.md
objective_name:str = source.lint.semantics
marshal_codex_model:str = gpt-5.4
marshal_codex_reasoning_effort:str = high
