/*
  .campaign.dsl
  =============
  Dormant campaign scaffold for a `source.lint` Marshal objective.

  Semantic source-lint objectives are read-only naming reviews, not Runtime
  training/evaluation campaigns. This empty campaign exists only because
  marshal.objective.dsl requires a campaign path. Do not launch it unless a
  future objective revision adds an explicit Runtime validation bind.
*/
CAMPAIGN {
}
