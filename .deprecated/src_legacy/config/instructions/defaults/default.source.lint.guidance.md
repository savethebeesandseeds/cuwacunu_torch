Rules:
- Work from the current `source.lint` objective bundle, repository context, and
  the latest Marshal session memory.
- Read the objective markdown as the primary source for the target files,
  desired refactor, source-code authority limits, validation expectations, and
  human-facing deliverables.
- This guidance is for `source.lint` objectives. Do not launch Runtime campaigns
  for source-code work unless a future objective explicitly defines a separate
  post-change runtime smoke step.
- Current Marshal sessions do not have direct source-code mutation authority.
  When source writes are unavailable, produce the clearest actionable refactor
  plan, identify the required authority expansion, and stop rather than
  pretending the code was changed.
- If source-code mutation authority is granted in a later runtime, keep edits
  minimal and local to the requested scope. Preserve repository style, prefer
  C++20, use LLVM clang-format style, prefer Makefiles over CMake, and prefer
  `rg` for source discovery.
- Treat key files, secrets, and secret paths as sensitive material. Do not copy
  secret values into notes, objectives, generated files, or summaries.
- For large-file refactors, first identify stable ownership boundaries:
  public API declarations, private helpers, parser/validation routines,
  lifecycle orchestration, command/tool handlers, serialization, and tests.
- Split by behavior and ownership rather than by arbitrary line count. Keep
  observable behavior stable unless the objective explicitly asks for a
  behavior change.
- Preserve include order, namespace structure, static linkage intent, build
  target membership, and existing error messages unless changing them is part of
  the stated objective.
- Prefer a small validation path once source edits are authorized: the narrowest
  build target or test that covers the moved code, with broader validation only
  when the refactor touches shared contracts.

Intent contract:
- Use `intent = complete` when the current deliverable is a plan, decomposition,
  or authority handoff and no automatic Runtime launch is justified.
- Use `intent = pause_for_clarification` when the target behavior or acceptable
  source authority is ambiguous.
- Use `intent = request_governance` when the next useful step requires
  source-code write authority, broader filesystem authority, or validation
  budget.
- Do not use `intent = launch_campaign` for `source.lint` objectives unless the
  objective explicitly defines a Runtime validation campaign and the source-code
  changes it depends on have already been authorized and made.
