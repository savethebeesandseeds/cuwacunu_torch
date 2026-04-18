/*
  marshal.test.smoke.marshal.dsl
  ==============================
  Marshal Hero entrypoint for marshal.test.smoke.

  This objective exists to exercise Marshal session behavior itself rather than
  a specific model family. Fresh sessions should inherit the current objective
  markdown, default guidance, and local campaign scaffold from here.
*/
campaign_dsl_path:path = iitepi.campaign.dsl
objective_md_path:path = marshal.test.smoke.objective.md
guidance_md_path:path = ../../defaults/default.marshal.guidance.md
objective_name:str = marshal.test.smoke
