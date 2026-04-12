You are tasked with training the frozen optimized VICReg bundle on BTCUSDT
only.

Objective identity:
- This objective is `vicreg.solo.train`.

Current intent:
- This is a BTCUSDT-only training objective, not an architecture search.
- The frozen truth source lives under `../../optim/` and must stay frozen.
- You can create/modify dsl files inside the objective folder you need, modify these .dsl as you need.
- Use the objective-local campaign, waves, wrapper, and jkimyei files as the
  only ordinary tuning surface.
- Treat the current `0x00FF` slot as the working continuation candidate for one
  last promotion-biased pass rather than restarting from scratch.
- The practical acceptance gate is downstream engine usefulness after
  promotion; representation-side reports are supporting evidence only for this
  final round.

Canonical run package:
- Treat this objective bundle as intentionally cleaned and reduced.
- Keep one primary train profile (you can modify at will):
  `stable_pretrain_linear_only`.
- Keep one evaluation profile (you can modify at will):
  `eval_payload_only`.

Canonical dataset split:
- Train window:
  `01.01.2020` to `31.12.2023`.
- Reusable validation window:
  `01.01.2024` to `31.08.2024`.
- Untouched final test window:
  `01.09.2024` to `31.12.2024`.

Canonical success-oriented training path:
- Prefer materially long-horizon runs over short exploratory nudges.
- Too rought asugmentations might be the reason the model is not learning. 
- BTCUSDT is a complex series, bare this in mind don't expect good forecasting metrics. 
- Feel free to modify the training policy.

Evaluation policy:
- Judge downstream value against the simple same-slice baselines when
  available:
  - `matrix.forecast.stats_only`
  - `matrix.forecast.raw_linear`
  - `matrix.forecast.linear`
  - `matrix.forecast.raw_mdn`
  - `matrix.forecast.mdn`
  - any others... feel free to make your own evaluations.
- Prefer relative value over raw future forecastability alone.
- If the learned embedding cannot beat simple stats or raw-surface baselines on
  reusable validation after a materially long-horizon run, count that against
  the frozen VICReg design.
- For this final continuation block, it is acceptable to spend the untouched
  final test after reusable validation in the same campaign so the objective
  ends with a promotion-ready evidence package.

Operational stance:
- Treat each launch as a train-plus-eval effort block, not as a training-only
  block, unless an operational blocker prevents eval.
- When training observability is coarse, use Runtime stdout/trace and persisted
  reports to determine whether training still appears to be progressing.
- If debug visibility is needed, keep dedicated debug eval waves rather than
  slowing the main exhaustive train wave unnecessarily.
- Bias toward conservative continuation edits over fresh ablations. This round
  should try to leave behind the most stable working candidate we can promote
  into `../../optim/` right now.

Primary objective-local mutation surfaces:
- `iitepi.campaign.dsl`
- `iitepi.waves.dsl`
- `tsi.wikimyei.representation.vicreg.jkimyei.dsl`
- `tsi.wikimyei.representation.vicreg.dsl` only if policy wiring itself must
  change
- Do not change the iitepi.contract.dsl

Desired result shape:
- State whether the latest BTCUSDT train-plus-validation effort completed
  successfully.
- Identify the promoted slot and the exact train/validation/test windows used.
- State whether the reported numbers come from reusable validation or untouched
  final test.
- Summarize training health, downstream utility, and whether another effort
  block is justified.
- State explicitly whether the objective-local jkimyei policy is good enough to
  promote into `../../optim/`.
