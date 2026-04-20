You are supervising a `source.lint` refactoring objective.

Objective identity:
- This objective is `source.lint.refactoring`.

Primary objective:
- Refactor the largest source file from the current over-350-line source list:
  `src/impl/hero/marshal_hero/hero_marshal_tools.cpp` at 9404 lines.
- Split that file into multiple cohesive implementation files while preserving
  current Marshal Hero behavior.

Current authority constraint:
- Marshal Hero currently cannot modify repository source code directly.
- Do not launch a Runtime campaign for this objective.
- Until source-code mutation authority exists, produce an actionable refactor
  plan and request only the minimum governance/authority needed for code-write
  execution.

Marshal model policy:
- This objective requests `marshal_codex_model:str = gpt-5.4` and
  `marshal_codex_reasoning_effort:str = xhigh` in its Marshal objective DSL
  because large source-file decomposition benefits from the higher-capability
  planning model.

Preferred refactor shape:
- Identify natural ownership boundaries inside `hero_marshal_tools.cpp`, such
  as session lifecycle, checkpoint handling, intent decoding, objective-file
  mutation bookkeeping, Runtime launch/stop integration, operator messaging,
  and summary/archive flows.
- Propose a small set of new `.cpp` files under
  `src/impl/hero/marshal_hero/` with clear responsibility names.
- Preserve public headers and external behavior unless a later operator message
  explicitly authorizes API changes.
- Preserve existing logging, ledger schema, event names, and failure semantics.
- Keep the build-system change explicit: identify the Makefile or source list
  that must include the new translation units.

Desired human-facing deliverables:
- A concise decomposition plan naming each proposed new file and the code it
  should own.
- A risk list covering linkage/static helpers, initialization order, schema
  compatibility, and the narrowest useful validation target.
- If source mutation remains unavailable, finish with the exact governance or
  authority request needed to perform the split later.
