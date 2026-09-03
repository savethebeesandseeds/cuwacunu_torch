# Project Clear Signal — Frozen-Representation Affine Warm-Start Stability

## Protocol identity and finite stop

- Protocol and report schema:
  `synthetic_v2_frozen_representation_affine_warm_start_stability_development_v1`.
- Authority: development diagnostic only; benchmark acceptance is false.
- The protocol ends after at most one evaluator invocation and at most one
  seed-31, 3,500-step optimizer fit. A valid result records one of the three
  fixed scientific classifications below. Any step-zero integrity failure,
  timeout, signal, evaluator error, malformed report, incomplete schedule, or
  post-run verification failure terminalizes the sole attempt.
- There is no resume, replay, retry, recapture, seed sweep, parameter sweep,
  early stop, validation selection, refit, or follow-on rung under this
  identity.

The question is deliberately narrow: when the already proven direct-float32
affine map is embedded exactly in the Phase 2B two-GELU topology, does the fixed
Adam schedule preserve that known-good solution? This protocol neither trains
nor evaluates the representation, MDN, checkpoint, policy, certified set, or
final holdout.

## Frozen implementation and authority

The runner binds the following immutable implementation closure:

- evaluator
  `/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/frozen_representation_affine_warm_start_stability_probe.cpp`,
  SHA-256 `dcc6112c7920092cca1e36e24afe33fb4e9393325437a67942016052ad32296d`;
- compile-only wrapper
  `/cuwacunu/src/scripts/benchmarks/synthetic_continuous_graph_v2/build_frozen_representation_affine_warm_start_stability_probe.sh`,
  SHA-256 `43440ad66bf480e281354c80bdd536babe436e0ea14c2d54be99616bac92a89b`;
- predecessor optimizer-localization evaluator, SHA-256
  `7a55325e6f291e2355d1d5944c9fb00e94dadebe702d48dab3b9af349a0b871b`;
- canonical affine parser/source, SHA-256
  `45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939`;
- Phase 2A conditioned-affine source, SHA-256
  `5103e594a6096a325ac33b115594a739a0c3e3f0ad8d36b9fcf38d8ac8114570`.

Scientific authority is limited to these frozen development artifacts:

- train representation probe at
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/train/representation_edge_features.probe`,
  SHA-256 `d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75`;
- validation representation probe at
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/validation/representation_edge_features.probe`,
  SHA-256 `8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd`;
- Phase 2A development receipt, SHA-256
  `b611d3d3d9e2d1e198a2764b928886b647d5ee95211a89e584a49c4e05b7fbe5`,
  and its byte-identical main/replay reports, each SHA-256
  `2bb817bc5649c895e5fde2079fffb9505d2a42ac90b6f7ded55ef8b4946fe38a`;
- recovered Phase 2B development result, SHA-256
  `cdf50f2a827b0eeb99bb92dca4821dd255b4c0e8d102db7daee759c662d79418`,
  and nonlinear report, SHA-256
  `34d51b3226278f8fe43a79e09c1c9063fe1aab31db042bd6a224ef06445fbc36`;
- authoritative optimizer-localization V3 development receipt, SHA-256
  `ad9be3c76c69bafaa6e42ce63e1e10b05231878f87b465ffd8b56b8156237ad6`,
  its verification receipt, SHA-256
  `edcda0efcfe00fb6f53fa005040e4a17c8d8c5a50b4c97656dbe2b4516ee665d`,
  and its immutable predecessor scientific report, SHA-256
  `5b1ebcc7af65792074e653406a1a6f4120dc9ad4105adca5cbca90f6c5815f30`.

The V3 mechanical recovery closure is independently pinned as runner
`b951630c11299769c1cab41d8ba3eef53356845a83d542e73c706bfd778ea1bf`,
preregistration
`f39567b505757d75be814fe2392ee12ea0618f615c28ecddea518e7fd0ffedce`,
validator
`a93a4c6315c0c89f6b747490d3cb6112dbf506675ad188ed656d10693dbedbb2`,
validator self-test receipt
`c0d29f26643f3d6f34c08820eaac88da67355fd305eb80b17398679ea834bb53`,
verification attempt
`aac5242963fa6071a34385531b9de9192704e13f7423b441344337ba28ba3d7f`,
and verification log
`e8381a5a21e365aded459267fc7ea149bbb2a4f9bd778331b0f43efe713f7f77`.

The V3 receipt must remain complete, development-only, scientifically
available, and classified `float32_conditioning_failure`; it must also retain
paired-GELU/direct-float32 parity, direct-linear Adam failure, and all Phase
2A/2B/source hashes. Its frozen runner, preregistration, validator, validator
self-test receipt, verification attempt, verification log, and verification
receipt are independently rehashed by the warm-start runner.

Authorized anchors are train `[0,2496)` and validation `[2560,2816)`, with
maximum anchor 2815. Validation is evaluated only at step zero and after the
fully fixed 3,500-step fit. It cannot select a state, step, seed, threshold, or
classification rule.

## Fixed scientific procedure

All nine independent heads use `head = channel*3+edge`, flat row order
`anchor,edge,channel`, train-global float32 feature standardization, and
train-only per-edge/channel float32 target standardization. Metrics are
computed in float64.

1. Recompute the fixed Phase 2A float64 centered-Cholesky affine oracle with
   ridge `1e-12` from train only. There is no ridge selection. Its aggregate
   and channel metrics must reproduce the frozen Phase 2A report within
   absolute tolerance `1e-12`.
2. Cast and execute that exact map as the authoritative direct-float32 route.
   Its aggregate direction, rank, correlation, and RMSE/target-RMS values must
   reproduce the authoritative V3 receipt within `1e-12`.
3. Inject the direct-float32 map analytically into
   `Linear(96,128)+GELU+Linear(128,128)+GELU+Linear(128,9)` using paired
   `z,-z` neurons and `GELU(z)-GELU(-z)=z`. Only 18 units per hidden layer are
   needed; the other 110 are zero. All six parameter tensors, totaling 30,089
   trainable parameters, participate in the subsequent fit.
4. Evaluate the injected topology on both frozen splits before constructing
   Adam. The maximum absolute standardized-target prediction delta against
   direct float32 must be `<=1e-5` independently on train and validation. If
   either comparison fails, the evaluator throws before constructing Adam,
   performs zero optimizer steps, emits no complete report, and the runner
   terminalizes the attempt. This is an integrity/precondition failure, not an
   inconclusive scientific result.
5. Only after step-zero parity passes, instantiate a new Adam optimizer with no
   prior state and train all parameters. The one fixed fit is CPU float32 with
   deterministic algorithms, seed 31, 3,500 steps, batch size 64, learning
   rate `1e-3`, betas `0.9,0.999`, epsilon `1e-8`, weight decay 0, global
   gradient-norm clip 5, and `mt19937_64` uniform-with-replacement batches.
   The fixed schedule fingerprint is `f2fa41d284a42d60`. There is no scheduler,
   early stop, seed selection, hyperparameter search, retry, refit, checkpoint,
   or validation read by the trainer.
6. After all 3,500 steps, emit the fixed full-train summary MSE, then evaluate
   the final route metrics on train and validation and apply the gates below
   mechanically. This entails two deterministic post-fit train forwards (the
   summary MSE and route metrics) and one post-fit validation forward; none can
   select or alter model state. The summary MSE is Torch float32 while route
   MSE is recomputed after float64 conversion; their two duplicate comparisons
   use the V3-recovered mixed tolerance
   `abs(a-b) <= 1.25e-7 + 1.25e-7*max(abs(a),abs(b))`, never `1e-12`.

Fit accounting is one grouped analytic oracle reconstruction with nine head
solves plus one optimizer fit: `optimizer_fits_completed=1`,
`optimizer_steps_completed=3500`, and `total_train_fit_procedures=2`.
The complete report schema contains exactly 1,153 unique, nonempty
`key=value` lines. Duplicate, malformed, missing-required, or extra keys are
terminal-invalid.

## Fixed preservation and clear-stop gates

After the mandatory step-zero precondition, the train preservation gate passes
only if every condition holds:

- final aggregate train standardized MSE / float64-oracle train standardized
  MSE is `<=1.05`;
- each of nine final train head standardized MSE / corresponding float64-oracle
  head standardized MSE ratios is `<=1.10`;
- final aggregate train direction, pairwise rank, and correlation are each no
  more than `0.01` below the float64 oracle; and
- final aggregate train RMSE/target-RMS is no more than `0.05` above the
  float64 oracle.

The validation preservation guard passes only if final aggregate validation
direction, pairwise rank, and correlation are each no more than `0.01` below
the authoritative direct-float32 route and final validation RMSE/target-RMS is
no more than `0.05` above direct float32. It is a post-fit preservation guard,
never a training or selection input.

The warm-start stability gate passes iff step-zero parity plus both train and
validation preservation pass. The clear-stop gate passes iff step-zero parity
passes, warm-start stability fails, and either the final/oracle aggregate train
standardized-MSE ratio is `>=1.25` or the maximum of the nine final/oracle train
head standardized-MSE ratios is `>=1.50`.

After a valid complete fit, classification is the first matching rule:

1. stability gate passes: `warm_start_stability_established`;
2. clear-stop gate passes: `optimizer_destabilization_clear_stop`;
3. otherwise: `warm_start_stability_inconclusive`.

If stability is established, the controlled readout repair is deterministic
affine warm start and any nonlinear addition begins as a zero-initialized
residual. If optimizer destabilization reaches the clear stop, the affine
prediction becomes a frozen base excluded from the optimizer and only a
separate residual or uncertainty branch may be trained. An inconclusive result
authorizes neither architecture change. No result authorizes representation
retraining or access beyond development.

## Lifecycle and protected boundary

`--prepare` is compile-only, runs before the attempt, and is bounded by GNU
timeout at 300 seconds with 10-second TERM grace. It may not read either probe.
`--run-development` requires the frozen build receipt; it cannot compile. The
sole evaluator invocation is foreground within one private worker bounded by
one 300-second `setsid` GNU-timeout supervisor with the same grace period. The
worker is authorized by an unlinked, inherited, single-record capability FD and
inherits the identity-checked exclusive protocol lock. Shared locks hold the
frozen capture, Phase 2B, original optimizer-localization, and V3 authorities.

The launch-in-progress latch records signals arriving before timeout PID
assignment. After PID capture, any pending signal is dispatched. On INT, TERM,
HUP, or QUIT the runner signals the captured process group, waits, reaps it,
then seals terminal evidence. Normal completion also reaps before process
scanning, report validation, or result publication. Attempt, lifecycle, log,
report, science-complete, result, rejected, and terminal artifacts use private
directories, immutable metadata, and atomic no-clobber publication. Any
post-attempt failure consumes the identity permanently. `--verify-development`
is read-only and never compiles or evaluates.

The evaluator CLI is exactly:

`EVAL --development-only --train-input ABS --validation-input ABS --output ABS`

All paths are required, absolute, distinct, and fixed by the runner. Protected
or model/checkpoint options are rejected. No raw source, certified input, final
holdout, policy data, MDN, model checkpoint, capture, or representation forward
may be opened. Successful output remains development-only.
