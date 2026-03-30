# Super Loop Constitution

This document is the shortest stable statement of what the Super Hero loop is
allowed to be in v2.

## Sovereignty

- Super Hero is the sovereign of loop state, turn orchestration, budgets,
  escalation, and Runtime launch authority.
- Runtime Hero is the sovereign of campaign execution.
- Config Hero is the only writer for autonomous truth-source mutation.
- Human Hero is the sovereign of operator attestation and typed governance
  resolutions.
- Codex is the autonomous planner acting inside the authority envelope Super
  Hero provides.

## Core Loop

- The loop is `bootstrap -> planning -> running -> planning -> ...`.
- A planning turn may mutate objective-local truth sources, inspect evidence,
  and choose one outcome.
- One planning turn may launch at most one Runtime campaign.
- After a campaign reaches a terminal Runtime state, Super Hero stages a new
  turn context and planning continues.

## Outcome Contract

- `outcome = launch | escalate | success | stop | fail`
- `launch` carries:
  - `mode = run_plan | binding`
  - `binding_id` only when `mode = binding`
  - `reset_runtime_state`
  - `requires_objective_mutation`
- `escalate` carries:
  - `kind = authority_expansion | budget_expansion | objective_clarification`
  - `request`
  - optional typed `delta`
- Every turn also carries `reason` and optional `memory_note`.

## Human Boundary

- Human Hero is not part of ordinary runtime routing.
- Human intervention exists only for:
  - authority expansion
  - budget expansion
  - objective clarification
  - explicit operator stop
- Human resolutions are typed:
  - `grant`
  - `deny`
  - `clarify`
  - `stop`
- A granted resolution returns the loop to `planning`; it does not directly
  choose the next Runtime bind.

## Mutability

- Objective-local truth sources under `objective_root` are the default
  autonomous write surface.
- Shared defaults are outside ordinary autonomous authority.
- Shared-default mutation requires a human-granted `authority_expansion`.
- All truth-source writes happen through Config Hero whole-file
  create/read/replace/delete policy with decoder validation.
- Super Hero records per-turn objective mutation summaries with before/after
  hashes when Codex changed truth-source files during a planning turn.

## Budgets

- `remaining_review_turns` bounds Codex planning turns.
- `remaining_campaign_launches` bounds Runtime campaign launches.
- Exhausting either budget ends the loop as `exhausted`.

## Prohibitions

- Codex must not launch campaigns directly.
- Codex must not mutate repository truth-source files directly.
- Super Hero must not silently widen write authority beyond the current
  envelope.
- Runtime cold resets requested by Super must not remove `.super_hero` or
  `.human_hero` ledgers.
- Human Hero must not be used as an ordinary “choose the next bind” router.

## Persistence

- The loop persists as artifacts, not as one immortal process.
- The minimum v2 ledger is:
  - `super.loop.manifest.lls`
  - `super.objective.dsl`
  - `super.objective.md`
  - `super.guidance.md`
  - `config.hero.policy.dsl`
  - `super.briefing.md`
  - `logs/codex.session.log`
  - `super.loop.memory.md`
  - `super.loop.events.jsonl`
  - `turns/turn_context.latest.json`
  - `turns/turn_outcome.latest.json`
  - numbered `turn_context.*.json` and `turn_outcome.*.json`
  - numbered `turn_mutation.*.json` when truth-source files changed
  - `human/escalation.latest.md` when a typed escalation is pending
  - `human/resolution.latest.json` plus detached `.sig` when human governance
    occurs
  - `human/report.latest.md` when a loop reaches `success`, `stopped`,
    `failed`, or `exhausted`

## Design Intention

The v2 loop should feel lawful, auditable, and productively autonomous:

- evidence before launch
- bounded authority before mutation
- persistent memory before repetition
- typed human governance before unsafe expansion
- Codex autonomy inside the envelope, not outside it
