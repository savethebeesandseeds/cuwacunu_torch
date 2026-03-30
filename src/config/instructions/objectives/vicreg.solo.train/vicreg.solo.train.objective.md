You are tasked with training the frozen optimized VICReg bundle on BTCUSDT
only.

Objective identity:
- This objective is `vicreg.solo.train`.

Notes:
- The optimized VICReg truth source lives in `../../optim/` and is frozen for
  this objective.
- This objective is not a settings-optimization objective and not a
  cross-symbol evaluation objective.
- Use the frozen optimized contract, circuit, representation design, and
  channels as they exist under `../../optim/`.
- Objective-local waves own training effort and evaluation cadence for this
  objective; they are not part of the frozen optimized artifact bundle.
- Materialize or refresh the trained optimized checkpoint in the promoted
  Hashimyei slot `0x00FF`.
- The target source symbol for this objective is `BTCUSDT` only.

Research stance:
- This is a training objective, not a search objective.
- Do not author new variants or mutate the imported `optim` bundle.
- Do not expand the symbol roster beyond `BTCUSDT`.
- Treat each launch as a train-plus-eval effort block, not as a training-only
  block.
- The imported optim train wave is intentionally short, so meaningful training
  effort may require multiple launches against the same promoted slot.
- Use post-launch evaluation evidence to decide whether another train-plus-eval
  launch is justified.
- Prefer disciplined iterative effort over one-shot undertraining or blind
  relaunch churn.

Primary objective-local mutation surfaces:
- `iitepi.campaign.dsl` only if a concrete BTCUSDT training/eval adjustment is
  justified

Primary goals:
- Train the frozen optimized VICReg bundle on `BTCUSDT`.
- Persist the trained result in promoted slot `0x00FF`.
- Evaluate the trained slot after each effort block and use the evidence to
  decide whether more training waves are warranted.
- Produce a report that makes it clear whether the optimized training run
  converged usefully, what artifact or checkpoint provenance backs the promoted
  slot, and whether more training effort is still justified.

Training policy:
- A normal launch for this objective should include both BTCUSDT training and a
  held-out BTCUSDT payload evaluation pass.
- Do not stop after a training-only launch unless a hard operational blocker
  prevents evaluation.
- Use Lattice and Runtime evidence after each launch to assess whether the
  latest eval improved enough to justify another pass.
- If evaluation stagnates, regresses, or the promoted slot already looks good
  enough for this objective, prefer `stop` or `success` over reflexive relaunch.

Debug-evidence policy:
- When the active waves include `MODE |= debug`, inspect persisted
  Hashimyei/Lattice evidence for missing or incomplete training-relevant debug
  outputs.
- Treat missing debug evidence as a first-class reporting surface, not as an
  invisible footnote.
- Distinguish between:
  - missing debug evidence that blocks confidence in the training conclusion
  - missing debug evidence that is merely nice-to-have
- If important debug evidence is missing, call that out explicitly and state
  whether it changes the relaunch decision.

Desired result shape:
- State whether the BTCUSDT train-plus-eval objective completed successfully.
- Identify the promoted slot used for the trained artifact.
- Summarize the latest training evidence, latest eval evidence, and whether
  another effort block still appears justified.
- Summarize any important missing debug/Lattice evidence relevant to judging
  the training outcome.
- Do not broaden into cross-symbol comparison here.

Secondary goals:
- Diagnose and detect errors.
- Evaluate the quality of reports.
- Evaluate the usability of the hero's.
