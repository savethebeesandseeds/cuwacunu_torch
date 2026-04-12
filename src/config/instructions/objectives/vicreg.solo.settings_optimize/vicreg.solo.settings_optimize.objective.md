You are tasked with improving the VICReg architecture.

Objective identity:
- This objective is `vicreg.solo.settings_optimize`.

Notes:
- Observation channels currently point at the objective-local medium dock
  `1d + 4h + 1h`. We changed the dock because the old coarse profile starved
  effective sample count while giving little horizon diversity.
- The earlier sub-daily launch blocker has been addressed. Medium-channel
  launches are no longer blocked by malformed raw kline close timestamps: the
  dataloader now canonicalizes the affected bars well enough to build caches,
  insert masked gaps, and train on this dock.
- A direct Runtime launch already established that the hardened dataloader can
  get through source initialization and complete training on a sub-daily dock.
  Do not treat the old raw timestamp failure as the current limiting factor.
- A mistaken Marshal launch used `3d + 1d + 1h` and was stopped promptly. Do not
  treat that paused run as evidence for or against the intended `1d + 4h + 1h`
  dock.
- The most recent paired eval failure on this objective was operational, not
  scientific: Hashimyei catalog refresh hit a live ingest lock before payload
  evaluation ran. Treat a repeat of that failure as infrastructure triage, not
  as evidence against the architecture or dock.
- `optim/` remains the frozen best-known bundle from the previous channel
  regime. Preserve that record. This objective is where we re-search network
  settings against the new medium-channel dock.
- `iitepi.contract.base.dsl` remains the calibration baseline, but the default
  run plan is seeded from the previously winning `encoder_capacity_v1`
  architecture so we carry forward the strongest pre-channel-change design we
  found.
- Read `resume.notes.md` before planning if it exists. Treat it as the
  operator-maintained handoff summarizing the latest useful evidence and the
  intended continuation point for fresh sessions.
- This objective is about VICReg architecture quality, projector topology, and
  option policy. It is not a cross-symbol evaluation objective.

Research stance:
- The baseline contract is a starting point, not an endpoint.
- Use a clean baseline pass for calibration. If downstream quality remains weak
  after calibration, prefer deliberate objective-local variant authoring over
  repeating the unchanged baseline indefinitely.
- If evidence points to architecture weakness more than undertraining, act on
  that. If evidence instead shows undertraining or evaluation noise, avoid
  pretending that another architecture edit is justified.
- Prefer small, hypothesis-driven changes rather than broad sweeps.
- Preserve the ability to compare baseline and variants cleanly across turns.
- Prefer named objective-local variants when they make the hypothesis easier to
  compare and discuss than mutating the baseline in place.
- Keep the campaign bundle readable: each added variant should have a clear
  reason to exist and a clear comparison target.

Primary objective-local mutation surfaces:
- `iitepi.contract.base.dsl`
- `tsi.wikimyei.representation.vicreg.dsl`
- `tsi.wikimyei.representation.vicreg.network_design.dsl`
- `iitepi.campaign.dsl`
- Additional objective-local variant DSL files when a named variant is cleaner
  than mutating the baseline file in place

Primary goal:
- Improve VICReg network design by running campaigns and analysing the results.
- Improve VICReg projector topology and option policy by running campaigns and
  analysing the results.
- Produce a best-known candidate or a principled stop backed by comparative
  evidence across baseline and objective-local variants.

Desired result shape:
- Identify the current best-known candidate explicitly.
- Summarize how it compares against the baseline and any tested variants.
- Highlight the main evidence used for judgment, such as payload quality,
  linear or MDN skill, stability proxies, or entropic-capacity diagnostics.
- If no further automatic launch is justified, say whether the loop should stop
  because the objective is satisfied or because the remaining hypotheses are
  too weak.

Secondary goals:
- Diagnose and detect errors.
- Evaluate the quality of reports.
- Evaluate the usability of the hero's.
