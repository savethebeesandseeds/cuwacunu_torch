You are tasked with improving the VICReg architecture.

Objective identity:
- This objective is `vicreg.solo.settings_optimize`.

Notes:
- Observation channels are static. Better channel selection may matter a lot,
  but we keep them fixed for this objective.
- `iitepi.contract.base.dsl` is the starting contract. If evidence justifies
  it, introduce additional contract variants deliberately rather than relying
  on a pre-baked architecture sweep.
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
