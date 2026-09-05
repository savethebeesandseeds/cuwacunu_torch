# LWM-0B — Reference-relative temporal spread floor

Registered 2026-09-05 before measurement. Finite scope: implement and evaluate
one replacement anti-collapse constraint, with no encoder optimizer or EMA
updates. Stop after one valid run; no coefficient, threshold or seed search.

Retain LWM-0 temporal data, online full-target prediction, fit groups 0..255,
seeds 17/31/47, predictor reconstruction (512 updates per seed), sparse encoder
readout, and fixed 1024 directions seeded with seed+42000. Authenticate the
retained source, LWM-0A log, encoder header and certified archives. Confirmation
stays unopened. Calibrate from detached fit256 encoder states only, without labels.

For raw states and separately states centered over the true time axis W,
compute population batch variance in each projected direction, at each time and
channel. Freeze reference variances v_ref; require finite v_ref > 1e-10.
For each branch, use mean(relu(0.5 - sqrt(v/v_ref + 1e-6))^2)/0.25;
average the two branch losses. Keep weight 0.09. Computation is float32 without
autocast. This imposes a spread floor relative to the certified state, not a
Gaussian distribution target. Calibration and directions receive no gradients.

Mandatory representation fixtures, using the first64 fit states:

- Healthy: loss and gradient norm <= 1e-8.
- All-zero: finite positive loss and exactly zero gradient; explicitly no escape
  guarantee at exact collapse.
- Uniform contraction 0.1*z: loss > 0.1, nonzero finite gradient, negative gradient
  increases centered batch energy and one normalized step lowers the loss.
- Static over W: temporal branch loss > 0.9.
- Near-static mean_W(z)+0.1*(z-mean_W(z)): temporal loss > 0.1, negative gradient
  increases temporal energy and one normalized step lowers total loss.
- Rank-one and near-rank-one (rank-one plus 0.01*z), each rescaled to match the
  reference raw centered energy per time/channel: loss > 1e-5. Positive penalty
  does not establish rank restoration or universal collapse prevention.
- Batch reversal invariance within 1e-5. Fixture steps operate on representation
  copies only and have relative norm 0.001.

On encoder copies, evaluate three gradient directions at relative trunk
displacement 0.001: healthy prediction+floor; floor alone under uniform
contraction; floor alone under near-static intervention. The latter two are
mandatory active-direction stress checks, not actual encoder training states.
Keep their intervention in the loss graph; evaluate the original uncontracted
readout on the displaced encoder with the retained full RMC family, aggregate,
order, shuffle and geometry guards and the five label-free structural checks
(maximum relative decline 2%). Never normalize a zero direction.

Replay baseline and healthy combined evaluations against LWM-0A baseline and
prediction-only results within 1e-8; prediction loss/norm must replay LWM-0
within 1e-8 + 1e-6 relative. This checks reconstruction but is not independent
evidence for the inactive regularizer. Verify displacement, original/copy state,
fixed predictor, gradient slots and CPU/CUDA RNG preservation.

All fixture and all three direction guards must pass in all three seeds for
`passes_local_checks`; otherwise `not_admitted`. Custody/replay/mechanics failure
is `invalid`. Neither result admits training: finite projection tests and
synthetic activation do not establish behavior along an optimizer trajectory.
