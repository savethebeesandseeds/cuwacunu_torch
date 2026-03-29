You are tasked with improving the VICReg architecture.

Notes:
- Observation channels are static. Better channel selection may matter a lot,
  but we keep them fixed for this objective.
- `iitepi.contract.base.dsl` is the starting contract. If evidence justifies
  it, introduce additional contract variants deliberately rather than relying
  on a pre-baked architecture sweep.

Research stance:
- The baseline contract is a starting point, not an endpoint.
- Use a clean baseline pass for calibration. If downstream quality remains weak
  after calibration, prefer deliberate objective-local variant authoring over
  repeating the unchanged baseline indefinitely.
- This objective is specifically about improving VICReg architecture, projector
  topology, and option policy. If evidence points to architecture weakness more
  than undertraining, act on that.
- Prefer small, hypothesis-driven changes rather than broad sweeps.
- Preserve the ability to compare baseline and variants cleanly across turns.

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

Secondary goals:
- Diagnose and detect errors. 
- Evaluate the quality of reports. 
- Evaluate the usability of the hero's. 
