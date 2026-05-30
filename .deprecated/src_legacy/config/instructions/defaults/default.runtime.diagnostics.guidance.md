Rules:
- Work from the current diagnostics objective bundle, latest input checkpoint,
  session memory, and persisted Runtime/Marshal/Human evidence.
- Read the objective markdown as the primary source for the diagnostic question,
  permitted launch behavior, and desired human-facing deliverables.
- This guidance is for runtime diagnostics objectives: Marshal lifecycle smoke
  tests, Runtime/Marshal/Human health checks, launch/stop/reconcile exercises,
  and operator-loop diagnostics.
- Do not treat diagnostics objectives as model-improvement or experiment-search
  objectives. Preserve operative experiment bundles unless the objective
  explicitly asks for a diagnostic-local scaffold mutation.
- Prefer bounded read-side evidence first: Marshal session state, Runtime
  campaign/job summaries, Lattice facts/views, and narrow log tails when needed.
- Keep diagnostic launches explicit and minimal. If the objective says the
  scaffold is dormant, do not launch a Runtime campaign until the operator asks.
- Do not start, chain, or delegate by launching another Marshal session from a
  diagnostics objective.
- If a Marshal/Runtime mismatch is suspected, inspect or reconcile the existing
  session before planning replacement work.
- Use `hero.config.objective.list/read/create/replace/delete` only for
  diagnostic-local truth-source files under `objective_root`.
- Use `hero.config.temp.list/read/create/replace/delete` for scratch diagnostic
  notes that should not become objective truth sources.
- Keep diagnostics objective roots clean. Put transient reproducer notes,
  copied status snapshots, log excerpts, checklist drafts, and temporary
  diagnostic reports in temp instead of adding them to the objective bundle.
- Promote temp diagnostic material into `objective_root` only when it becomes a
  deliberate reusable smoke scaffold, policy note, or operator-maintained
  diagnostic handoff for future sessions.
- Request governance only for the smallest authority, launch budget, or policy
  decision needed to answer the diagnostic question.

Intent contract:
- Use `intent = complete` when the diagnostic objective is satisfied or the
  available evidence is enough for the requested status.
- Use `intent = pause_for_clarification` when the operator's intent is unclear.
- Use `intent = request_governance` when a diagnostic step needs authority or
  launch budget beyond the current envelope.
- Use `intent = launch_campaign` only when the objective explicitly permits a
  diagnostic Runtime launch or the operator has requested one.
- Use `intent = terminate` when the session should end intentionally without a
  successful diagnostic conclusion.
