You are tasked with training and selecting the best BTCUSDT-only VICReg
jkimyei policy.

Objective identity:
- This objective is `runtime.operative.vicreg.solo.train`.

Operator continuation (`2026-04-18`):
- Reuse the existing `runtime.operative.vicreg.solo.train` Marshal ledger when
  practical instead of starting a parallel replacement session.
- The previous parked planner attempted to launch
  `bind_eval_vicreg_payload_no_swa_v1`, which is not a declared bind in the
  current campaign. Treat that as a stale planner typo, not as evidence against
  the candidate itself.
- The immediate next reusable-validation launch for the no-SWA comparison track
  is the already declared bind `bind_eval_vicreg_payload_btcusdt_no_swa_v1`.
- If that reusable validation run is healthy and competitive, then the next
  justified no-SWA follow-up is
  `bind_eval_vicreg_payload_btcusdt_untouched_test_no_swa_v1`.
- If `0x00FF` mount resolution or catalog visibility disagrees with the
  historical training/eval runs, treat that first as a lineage persistence or
  binding-selection problem before concluding the policy itself is unfit.
- Fresh verification on `2026-04-18` shows the revived `0x00FF` artifact is
  not dock-compatible with the current cleaned `vicreg.solo.train` contract
  surface. Keep `0x00FF` only as historical reference evidence, not as the
  active launch mount for this objective revision.
- The active bootstrap mount for new launches is the fresh unregistered token
  `0x0200` until this objective emits a dock-compatible candidate lineage
  under the current contract.
- The immediate launch to restart progress under the current compatible
  surface is `bind_train_vicreg_primary_btcusdt`, then reusable validation,
  then untouched final test before further comparison-track branching.

Current intent:
- This is a BTCUSDT-only training-policy search over objective-local
  `.jkimyei` settings, not an architecture search.
- The architecture-bearing truth sourced from `../../optim/` must stay frozen
  during the search. Treat ordinary tuning here as jkimyei and run-plan work,
  not as a dock or assembly redesign.
- Use the objective-local campaign, waves, wrapper, and jkimyei files as the
  ordinary tuning surface, with `tsi.wikimyei.representation.vicreg.jkimyei.dsl`
  as the primary mutation surface.
- The earlier "one last promotion-biased continuation on `0x00FF`" framing is
  stale for this objective revision. Do not treat the current bundle as a
  single unchanged final pass.
- Keep `0x00FF` as a historical incumbent reference candidate, but author new
  launches from the current compatible lineage and prefer fresh, clearly named
  comparison exact revision tokens and profile ids so evidence stays
  attributable.
- Objective satisfaction requires exploring multiple materially different
  jkimyei combinations, selecting a winner, and ending with a promotion-ready
  result for the optimized VICReg bundle.

Canonical run package:
- Keep one incumbent continuation candidate derived from the current promoted
  VICReg policy.
- Author additional named train/eval profiles when that is cleaner than
  mutating the incumbent in place.
- Prefer a small deliberate comparison set over many tiny nudges. Each added
  candidate should correspond to a clear hypothesis about why the current
  training policy is underperforming or too brittle.
- Bootstrap exception: if the objective-local campaign already declares one
  concrete incumbent train-plus-reusable-validation-plus-untouched-test run
  package, the first fresh launch may use that incumbent package unchanged.
- Expand the comparison surface only after that first incumbent evidence block
  unless the current objective files are too incomplete to launch at all.

Canonical dataset split:
- Train window:
  `01.01.2020` to `31.12.2023`.
- Reusable validation window:
  `01.01.2024` to `31.08.2024`.
- Untouched final test window:
  `01.09.2024` to `31.12.2024`.

Search stance:
- Search jkimyei settings, not network architecture. Keep
  `iitepi.contract.dsl` frozen unless a hard wiring fix is required for the
  search itself. In normal operation, do not change the active dock here.
- Explore materially different jkimyei combinations rather than rerunning the
  same profile once and stopping. Useful axes include optimizer pressure,
  scheduler shape, SWA usage/timing, gradient controls, VICReg loss weights,
  and augmentation intensity.
- Prefer clean, named candidates that isolate one hypothesis or one tightly
  coupled pair of hypotheses at a time.
- Use separate exact revision tokens in `BIND.MOUNT` for distinct candidates
  whenever that
  keeps the evidence easier to compare and discuss. Do not silently overwrite
  unrelated candidate weights into the same lineage.
- Launch at most one candidate per checkpoint, then compare and prune before
  expanding the search further.
- Prefer `launch.mode = binding` while the comparison set is evolving, instead
  of blindly rerunning an unchanged full run plan.
- Do not require same-checkpoint candidate authoring merely because the
  existing bootstrap run plan contains only the incumbent candidate. Launch the
  incumbent first when it is already a valid concrete package, then decide what
  new candidates to author from evidence.
- Do not stop after the first launched candidate unless a hard operational
  blocker or overwhelmingly decisive evidence makes further automatic search
  unjustified.

Training and evaluation policy:
- Prefer materially long-horizon runs over tiny exploratory nudges once a
  candidate is worthy of a real comparison.
- BTCUSDT is a difficult series. Do not expect strong absolute forecasting
  numbers; judge the learned representation primarily on relative downstream
  usefulness against the simple same-slice baselines.
- Judge downstream value against the simple same-slice baselines when
  available:
  - `matrix.forecast.stats_only`
  - `matrix.forecast.raw_linear`
  - `matrix.forecast.linear`
  - `matrix.forecast.raw_mdn`
  - `matrix.forecast.mdn`
  - any other clearly justified objective-local evaluation additions
- Prefer reusable validation for pruning and comparison.
- Spend the untouched final test only on the finalist or the clearly preferred
  winner, not on every weak early candidate.
- If the learned embedding cannot beat simple stats or raw-surface baselines on
  reusable validation after a materially long-horizon run, count that against
  that jkimyei combination.

Operational stance:
- Treat each launch as a train-plus-eval effort block unless an operational
  blocker prevents eval.
- When training observability is coarse, use Runtime stdout/trace and persisted
  reports to determine whether training still appears to be progressing.
- If a launch fails before training begins, treat it as an authored campaign,
  wave, or mount-resolution bug first. Verify `BIND.MOUNT`, inspect
  `hero.runtime.explain_binding_selection`, and confirm the intended bind/wave
  pair before spending another launch on the same candidate.
- If debug visibility is needed, keep dedicated debug eval waves rather than
  slowing the main exhaustive train wave unnecessarily.
- Bias toward conservative, attributable candidate authoring over chaotic
  mutation. The objective should leave behind a small understandable search
  record, not an opaque pile of near-duplicate profiles.

Primary objective-local mutation surfaces:
- `tsi.wikimyei.representation.vicreg.jkimyei.dsl`
- `iitepi.campaign.dsl`
- `iitepi.waves.dsl`
- `tsi.wikimyei.representation.vicreg.dsl` only if jkimyei wiring itself must
  change
- Do not change `iitepi.contract.dsl` for ordinary tuning

Promotion end-state:
- The objective is not satisfied merely because one fresh train run completed.
- The objective is satisfied when a best-known jkimyei candidate is selected
  from comparative evidence, its exact profile id and exact revision token are
  named, and the session leaves behind a promotion-ready recommendation for replacing
  the current optimized VICReg settings.
- If current Marshal authority/tooling cannot directly mutate `../../optim/`
  or promote the winner component revision, do not treat that limitation as silent
  success. Instead, end with the smallest justified operator-facing governance
  or policy request that names the exact winner, the exact files/settings to
  replace, and why no more automatic search is needed.

Desired result shape:
- List every serious candidate that was actually launched, including its
  profile id, exact revision token, and the key jkimyei differences it introduced.
- Identify the current winner explicitly and state whether it beats the
  incumbent `0x00FF` continuation on reusable validation.
- State whether the reported deciding evidence comes from reusable validation
  or untouched final test.
- Summarize training health, downstream utility, and why the winning candidate
  is preferred over the rejected combinations.
- State explicitly whether the objective is now ready to replace the optimized
  VICReg settings, and if not, what exact blocker remains.
