You are tasked with selecting which VICReg component revision should be
promoted.

Objective identity:
- This objective is `runtime.operative.vicreg.solo.promote_optim`.

Primary intent:
- Compare every discoverable VICReg component revision on the untouched benchmark
  window and produce an auditable winner.
- Run no training in this objective. This is an evaluation-and-promotion gate.
- Keep the frozen optimized artifact surface untouched until explicit human
  approval is granted.
- Promote only by operator decision; do not auto-write into `../../optim/*`.
- Treat [iitepi.contract.dsl](/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.promote_optim/iitepi.contract.dsl)
  as the benchmark truth source for this objective's `DOCK` and `ASSEMBLY`.
  Do not change it unless the benchmark interface itself must change.

Discovery and run plan:
- Before the first real launch, discover candidate component revisions with
  `hero.hashimyei.list` where canonical path starts with
  `tsi.wikimyei.representation.vicreg.` (canonical prefix only).
- Before authoring a benchmark bind for a discovered revision, confirm it is
  dock-compatible with the selected benchmark contract. Prefer
  `hero.hashimyei.evaluate_contract_compatibility` when that compatibility is
  unclear.
- Convert each discovered revision token to an explicit
  `MOUNT { w_rep = EXACT 0x...; }` entry and emit one `BIND` per candidate in
  this objective campaign. The campaign may run multiple binds or one at a time.
- Find all dock-compatible VICReg component revisions and evaluate them, then compare
  their results.
- Keep all bindings in eval-only mode; do not add training binds here.

Benchmark definition:
- Use BTCUSDT only for this promotion benchmark.
- Canonical benchmark window: `01.09.2024` to `31.12.2024`.
- Canonical benchmark profile: `eval_payload_only` unless a strong reason is found
  to justify a different eval profile and documented in planning notes.
- Reuse (modify if you need):
  - [iitepi.contract.dsl](/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.promote_optim/iitepi.contract.dsl)
  (objective-local contract surface),
  - [iitepi.waves.dsl](/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.promote_optim/iitepi.waves.dsl),
  - [iitepi.campaign.dsl](/cuwacunu/src/config/instructions/objectives/runtime.operative.vicreg.solo.promote_optim/iitepi.campaign.dsl).

Required comparison frame:
- For each candidate, collect transfer-matrix outputs from the benchmark run.
- Track at minimum:
  - `matrix.forecast.linear.error_skill(-inf,+inf)`
  - `matrix.forecast.mdn.error_skill(-inf,+inf)`
  - `matrix.forecast.raw_linear.error_skill(-inf,+inf)`
  - `matrix.forecast.stats_only.error_skill(-inf,+inf)`
  - `runtime.train.optimizer_step_count` / `runtime.samples.seen_count`
  - `runtime.anomaly.*` counts
- And consider others. Your task is to compare the full reports in the lattice
  for these.
- Treat higher-skill values as better; if ties or near ties persist, use
  sample throughput and anomaly profile as tie-breakers.

Promotion gate:
- Do not finalize promotion automatically.
- Before making any `optim/` mutation request, produce a compact evidence table:
  `mounted_revision_token | canonical_path | linear_skill | mdn_skill | raw_linear_skill | stats_skill | support | winner? | health_note`.
- Identify:
  - the best candidate on this benchmark, and
  - any disqualifier (failed run, missing reports, malformed inputs, poor health).
- End the session with an explicit human-approval request for promotion:
  - candidate exact revision token,
  - expected target files under `../../optim/`,
  - key evidence summary,
  - and a one-line rationale.

Scope and constraints:
- If discovery yields more than a practical number of candidates, prefer a
  first-pass pruning step on obvious non-healthy runs, then compare only healthy
  candidates.
- Prefer campaign and wave edits over contract edits for ordinary benchmarking
  work in this objective.
- For changes stay within
  `src/config/instructions/objectives/runtime.operative.vicreg.solo.promote_optim/*` or use
  `src/config/instructions/temp`.
