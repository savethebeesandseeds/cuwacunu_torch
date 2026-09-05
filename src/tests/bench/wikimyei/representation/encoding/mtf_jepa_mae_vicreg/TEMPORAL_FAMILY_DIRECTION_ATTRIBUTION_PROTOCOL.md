# LWM-0A — Temporal Family Direction Attribution

Registered 2026-09-05 before new measurements. Scope: explain the four retained
LWM-0 family-floor failures with a single zero-encoder-update extension.

Freeze all LWM-0 data, normalization, seeds, predictor, warm-up schedule,
directions, coefficient, probes and thresholds. Reconstruct each predictor with
the identical 512 predictor-only updates because LWM-0 saved no predictor state.
This is deterministic state reconstruction, not a new predictor search.

Reuse the authenticated LWM-0 source and SIGReg header. Generate an include copy
changing only its entry-point name; preserve the original source and executable.
Authenticate the LWM-0 log, protocol, encoder header, seam audit and FSPA archives.
Require reconstructed losses/norms within 1e-6 relative + 1e-8 absolute of LWM-0.
Before interpretation, require baseline and both combined RMC evaluations to
replay all logged numeric fields within 1e-8 absolute, plus the original protected
virtual metrics within 1e-8. Mismatch is `invalid`, never scientific evidence.

At the existing relative trunk displacement 0.001, evaluate the full prediction,
raw SIGReg-only, and centered SIGReg-only directions with the existing full RMC
probes. Replay the two combined directions as controls. Use disposable copies;
verify the realized displacement, unchanged nontrunk state, original encoder,
predictor, gradient slots and CPU/CUDA RNG. No encoder optimizer or EMA updates,
checkpoint writes, confirmation access, coefficient changes, or new seed search.

Report all family deltas and existing quality safeguards. For each of the four
previously failed seed/candidate/family cases, apply the unchanged -0.005 floor:

- prediction passes, corresponding regularizer fails: `regularizer_direction`;
- prediction fails, regularizer passes: `prediction_direction`;
- both fail: `shared_direction_failure`;
- both pass but the replayed combination fails: `combination_only_threshold`.

These labels associate local directions with the observed failures. Equal-norm
directions are not additive loss contributions; threshold patterns do not prove
training causality or a nonlinear interaction. Do not promote an objective from
this audit. Stop after one valid run and retain concise findings and measurements.
