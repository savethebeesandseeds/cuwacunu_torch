# Marshal Session Constitution

This document is the shortest stable statement of what the Marshal Hero session
is allowed to be in v4.

## Sovereignty

- Marshal Hero is the sovereign of session state, checkpoint orchestration,
  launch budget, pause/resume control, and Runtime launch authority.
- Runtime Hero is the sovereign of campaign execution.
- Config Hero is the only writer for autonomous truth-source mutation.
- Human Hero is the sovereign of human input capture, signed governance
  resolutions, and session-summary acknowledgments.
- Codex is the autonomous planner acting inside the authority envelope Marshal
  Hero provides.

## Core Session

- The session is `active -> running_campaign -> active -> ...` until it pauses,
  idles, or finishes.
- Launch-time Codex settings are resolved once when the session is created,
  persisted in the session manifest, and mirrored into a generated
  `hero.marshal.dsl` artifact inside the session ledger.
- A planning checkpoint may mutate objective-local truth sources, inspect
  evidence, and choose one intent.
- One planning checkpoint may launch at most one Runtime campaign.
- After a campaign reaches a terminal Runtime state, Marshal Hero stages a new
  input checkpoint and planning continues in the same Codex session when
  possible.
- If a planning checkpoint already produced an intent artifact and later fails
  during mutation bookkeeping or validation, Marshal Hero preserves that
  attempted checkpoint, parks the session as `idle` with `finish_reason=failed`,
  and allows a future `continue_session`.

## Intent Contract

- `intent = launch_campaign | pause_for_clarification | request_governance | complete | terminate`
- `complete` means the current objective is satisfied for now and the session
  should park as `idle` until a future `continue_session`.
- `launch_campaign` carries:
  - `mode = run_plan | binding`
  - `binding_id` only when `mode = binding`
  - `reset_runtime_state`
  - `requires_objective_mutation`
- `pause_for_clarification` carries:
  - `clarification_request.request`
- `request_governance` carries:
  - `kind = authority_expansion | launch_budget_expansion | policy_decision`
  - `request`
  - optional typed `delta`
- Every checkpoint also carries `reason` and optional `memory_note`.

## Human Boundary

- Human Hero is not part of ordinary runtime routing.
- Human interaction exists only for:
  - ordinary clarification via `pause_for_clarification`
  - governance expansion via `request_governance`
  - idle/finished session-summary acknowledgment
  - explicit operator pause/resume/terminate controls
- Human-facing operator surfaces may present a derived state vocabulary such as
  `Running`, `Waiting: Clarification`, `Waiting: Governance`, `Operator Paused`,
  `Review`, and `Done`, but those are projections of the persisted session
  ledger rather than replacements for `phase`, `pause_kind`, or
  `finish_reason`.
- Governance resolutions are typed:
  - `grant`
  - `deny`
  - `clarify`
  - `terminate`
- A granted governance resolution returns the session to `active`; it does not
  directly choose the next Runtime bind.
- Clarification answers are unsigned and auto-resume the paused session.

## Mutability

- Objective-local truth sources under `objective_root` are the default
  autonomous write surface.
- Shared defaults are outside ordinary autonomous authority.
- Shared-default mutation requires a human-granted `authority_expansion`.
- All truth-source writes happen through Config Hero whole-file
  create/read/replace/delete policy with decoder validation.
- Marshal Hero records per-checkpoint objective mutation summaries with
  before/after hashes when Codex changed truth-source files.

## Budgets

- `remaining_campaign_launches` bounds Runtime campaign launches.
- Exhausting launch budget ends the session as `exhausted` and `finished`.
- Review-turn budgeting no longer exists; the durable Codex session is the
  primary planning unit.

## Prohibitions

- Codex must not launch campaigns directly.
- Codex must not mutate repository truth-source files directly.
- Marshal Hero must not silently widen write authority beyond the current
  envelope.
- Runtime cold resets requested by Marshal must not remove `.marshal_hero` or
  `.human_hero` ledgers.
- Human Hero must not be used as an ordinary “choose the next bind” router.

## Persistence

- The session persists as artifacts, not as one immortal process.
- The minimum v4 ledger is:
  - `marshal.session.manifest.lls`
  - `marshal.objective.dsl`
  - `marshal.objective.md`
  - `marshal.guidance.md`
  - `hero.marshal.dsl`
  - `config.hero.policy.dsl`
  - `marshal.briefing.md`
  - `logs/codex.session.stdout.jsonl`
  - `logs/codex.session.stderr.jsonl`
  - `marshal.session.memory.md`
  - `marshal.session.events.jsonl`
  - `checkpoints/input.latest.json`
  - `checkpoints/intent.latest.json`
  - numbered `input.*.json` and `intent.*.json`
  - numbered `mutation.*.json` when truth-source files changed
  - `human/request.latest.md` when a pause is pending
  - `human/governance_resolution.latest.json` plus detached `.sig` when signed
    governance occurs
  - `human/clarification_answer.latest.json` when ordinary clarification occurs
  - `human/summary.latest.md` when a session reaches `idle` or `finished`

`codex.session.stdout.jsonl` stores the raw `codex exec --json` stream for the
latest checkpoint attempt. `codex.session.stderr.jsonl` mirrors stderr as
line-wrapped JSONL entries with `stream`, `line_index`, and `text`.

## Design Intention

The v4 session model should feel lawful, auditable, and productively
autonomous:

- evidence before launch
- bounded authority before mutation
- persistent session memory before repetition
- typed governance before unsafe expansion
- natural Codex conversation continuity inside the envelope, not outside it
