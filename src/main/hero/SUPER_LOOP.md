# Super Loop Constitution

This document is the shortest stable statement of what the Super Hero loop is
allowed to be.

## Sovereignty

- Super Hero is the sovereign of loop state, review, and continuation.
- Runtime Hero is the sovereign of campaign execution.
- Config Hero is the only writer for autonomous truth-source objective/default
  file mutation.
- Human Hero is the sovereign of human adjudication and response attestation.
- Codex is the reviewer and tool-using planner inside a review boundary.

## Review Boundary

- A super review starts only after Runtime Hero reaches a terminal campaign
  state.
- Codex reviews evidence from the persisted loop ledger, current objective
  campaign, and latest review packet.
- Codex returns a bounded control decision.
- Super Hero validates that decision and is the only component allowed to
  request the next campaign launch or stop from Runtime Hero.
- When Codex returns `need_human`, Super Hero pauses and waits for a Human
  Hero signed response before resuming.

## Mutability

- Source objective bundles in the repository are the mutable truth source for
  Super Hero.
- Super Hero points `objective_root` at the selected source objective bundle
  under `src/config/instructions/objectives/...`.
- Autonomous mutation is limited to files inside the configured
  `objective_roots` and `default_roots`, filtered by `allowed_extensions`.
- Those writes happen through Config Hero whole-file
  read/create/replace/delete policy with decoder validation, not raw file
  edits.

## Decision Contract

- `control_kind = continue | stop | need_human`
- `continue` requires a bounded `next_action`
- `next_action.kind = default_plan | binding`
- `binding` requires `target_binding_id`
- `memory_note` should summarize what Codex actually learned and changed

## Prohibitions

- Codex must not launch campaigns directly.
- Codex must not mutate repository source files directly.
- Super Hero must not silently widen Config Hero write scope beyond the
  configured objective/default roots.

## Persistence

- The loop persists as artifacts, not as one immortal process.
- The minimum loop ledger is:
  - `loop.lls`
  - `super.objective.dsl`
  - `super.objective.md`
  - `super.guidance.md`
  - `config.hero.policy.dsl`
  - `super.briefing.md`
  - `logs/codex.session.log`
  - `memory.md`
  - `events.jsonl`
  - `reviews/latest.json` plus numbered `review_packet*.json`
  - `decisions/latest.json` plus numbered `decision*.json`
  - `human/request.latest.md` when needed
  - `human/response*.json` plus detached `.sig` when human adjudication occurs

## Design Intention

The super loop should feel lawful, auditable, and conservative:

- evidence before action
- bounded authority before mutation
- persistent memory before repetition
- human escalation before reckless autonomy
