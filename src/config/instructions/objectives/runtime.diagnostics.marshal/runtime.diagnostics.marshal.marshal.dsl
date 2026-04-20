/*
  runtime.diagnostics.marshal.marshal.dsl
  ======================================
  Marshal Hero entrypoint for runtime.diagnostics.marshal.

  This objective exists to exercise and diagnose Marshal session behavior
  itself rather than a specific model family. Fresh sessions should inherit the
  current objective markdown, diagnostics guidance, and local campaign scaffold
  from here.
*/
campaign_dsl_path:path = iitepi.campaign.dsl
objective_md_path:path = runtime.diagnostics.marshal.objective.md
guidance_md_path:path = ../../defaults/default.runtime.diagnostics.guidance.md
objective_name:str = runtime.diagnostics.marshal
