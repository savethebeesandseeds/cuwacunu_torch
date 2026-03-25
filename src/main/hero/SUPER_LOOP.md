# Super Loop Constitution

This document is the shortest stable statement of what the Super Hero loop is
allowed to be.

## Sovereignty

- Super Hero is the sovereign of loop state, review, and continuation.
- Runtime Hero is the sovereign of campaign execution.
- Config Hero is the only writer for autonomous loop-local `.dsl` mutation.
- Human Hero is the sovereign of human adjudication and response attestation.
- Codex is the reviewer and tool-using planner inside a review boundary.

## Review Boundary

- A super review starts only after Runtime Hero reaches a terminal campaign
  state.
- Codex reviews evidence from the persisted loop ledger, copied campaign
  bundle, and latest review packet.
- Codex returns a bounded control decision.
- Super Hero validates that decision and is the only component allowed to
  request the next campaign launch or stop from Runtime Hero.
- When Codex returns `need_human`, Super Hero pauses and waits for a Human
  Hero signed response before resuming.

## Mutability

- Source objective bundles in the repository are not mutated by the loop.
- Super Hero copies the selected objective bundle into
  `<runtime_root>/.super_hero/<loop_id>/instructions/`.
- Autonomous mutation is limited to mutable loop-local objective `.dsl` files
  inside that copied bundle.
- The copied `campaign.dsl` and copied `super.objective.dsl` are loop
  constitution and are not mutable through Config Hero.
- Those writes happen through Config Hero whole-file read/replace policy with
  decoder validation, not raw file edits.

## Decision Contract

- `control_kind = continue | stop | need_human`
- `continue` requires a bounded `next_action`
- `next_action.kind = default_plan | binding`
- `binding` requires `target_binding_id`
- `memory_note` should summarize what Codex actually learned and changed

## Prohibitions

- Codex must not launch campaigns directly.
- Codex must not mutate repository source files directly.
- Codex must not mutate the copied `campaign.dsl`.
- Codex must not mutate the copied `super.objective.dsl`.
- Super Hero must not silently widen Config Hero write scope beyond the loop
  objective root.

## Persistence

- The loop persists as artifacts, not as one immortal process.
- The minimum loop ledger is:
  - `loop.lls`
  - `super.objective.dsl`
  - `super.objective.md`
  - `super.hero.config.dsl`
  - `super.briefing.md`
  - `memory.md`
  - `events.jsonl`
  - `review_packet*.json`
  - `decision*.json`
  - `human_request.latest.md` when needed
  - `human_response*.json` plus detached `.sig` when human adjudication occurs

## Design Intention

The super loop should feel lawful, auditable, and conservative:

- evidence before action
- bounded authority before mutation
- persistent memory before repetition
- human escalation before reckless autonomy
