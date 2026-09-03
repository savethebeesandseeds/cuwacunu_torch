# Project Clear Signal — Affine-Injection Optimizer Localization

## Protocol identity and stop condition

- Protocol: `synthetic_v2_frozen_representation_affine_injection_optimizer_localization_development_v1`.
- Authority: development diagnostic only; benchmark acceptance is false.
- The protocol is complete after one immutable evaluator report mechanically
  classifies the gap between a fixed affine oracle, its float32 and paired-GELU
  executions, and one fixed-seed direct-linear Adam fit.
- There is one attempt, no resume, retry, seed sweep, or follow-on rung.

This is an optimizer-localization control, not a new representation benchmark.
It uses the already-sealed representation probes and does not run a data source,
raw capture, representation forward, checkpoint loader/writer, MDN, policy, or
certified/final evaluation.

## Frozen development authority

The runner binds these exact immutable inputs:

- train representation probe
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/train/representation_edge_features.probe`,
  SHA-256 `d07d3c30ab49fed9f80e76b60ec85da2895716c8bb235872272442e03c49df75`;
- validation representation probe
  `/cuwacunu/.runtime/benchmarks/synthetic_continuous_graph_v2/synthetic_v2_frozen_feature_capture_isolated_v2/jobs/validation/representation_edge_features.probe`,
  SHA-256 `8faa93e81e5c6014ee8c7d180b2184563821b73aa9da75e4c1fb27d6c9d329cd`;
- Phase 2A development receipt, SHA-256
  `b611d3d3d9e2d1e198a2764b928886b647d5ee95211a89e584a49c4e05b7fbe5`;
- byte-identical Phase 2A main and replay reports, each SHA-256
  `2bb817bc5649c895e5fde2079fffb9505d2a42ac90b6f7ded55ef8b4946fe38a`;
- recovered Phase 2B development result, SHA-256
  `cdf50f2a827b0eeb99bb92dca4821dd255b4c0e8d102db7daee759c662d79418`;
- its nonlinear report, SHA-256
  `34d51b3226278f8fe43a79e09c1c9063fe1aab31db042bd6a224ef06445fbc36`.

The authorized ranges remain train anchors `[0,2496)` and validation anchors
`[2560,2816)`, with maximum anchor 2815. The two probes must retain their exact
coordinate, target, row-order, 96-feature, and train/validation identities.
Validation may be evaluated only after every route is fully fixed; it may not
choose a ridge, seed, step, threshold, classification rule, or model state.

## Fixed scientific procedure

All routes use nine independent heads selected by
`channel_index * 3 + edge_index`, train-global feature standardization, and
train-only per-edge/channel target standardization where required.

1. **Float64 oracle.** Recompute nine independent ridge heads in float64 using
   train only and fixed ridge `1e-12`. There is no ridge search. The runner must
   compare every emitted aggregate and per-channel train/validation metric and
   count with the frozen Phase 2A fixed-ridge report: counts are exact and each
   floating metric has absolute tolerance `1e-12`. Failure is terminal-invalid.
2. **Direct float32.** Cast that same fixed affine map to float32 and execute it
   directly. There is no fit or optimizer.
3. **Injected paired-GELU.** Encode the same float32 affine heads analytically in
   the Phase 2B `96 -> 128 -> GELU -> 128 -> GELU -> 9` topology using paired
   `z,-z` neurons and `GELU(z)-GELU(-z)=z`; evaluate without optimization.
4. **Direct-linear Adam.** Initialize one direct `Linear(96,9)` model with seed
   31 and train only the gathered head on the standardized target. Reuse the
   exact Phase 2B seed-31 schedule: CPU float32 deterministic algorithms, Adam,
   3,500 steps, batch size 64, learning rate `1e-3`, betas `0.9,0.999`, epsilon
   `1e-8`, weight decay 0, gradient-norm clip 5, and
   `mt19937_64` uniform-with-replacement batches. There is no scheduler, early
   stopping, validation read by the trainer, checkpoint, or refit.

Fit accounting is exact: the oracle is one grouped closed-form fit containing
nine independent head solves; Adam is one optimizer fit. Therefore
`affine_oracle_grouped_fit_count=1`, `affine_oracle_head_solve_count=9`,
`optimizer_fits_completed=1`, and `total_train_fit_procedures=2`. Direct
float32 and paired-GELU execution are transformations of the frozen oracle and
are not additional fits.

The evaluator records train and validation direction, pairwise rank,
correlation, and RMSE/target-RMS for every route, plus maximum prediction deltas,
schedule identity, completed optimizer steps, and all protected-access flags.

## Fixed gates and mechanical classification

Parity thresholds are fixed before execution and use standardized-target
prediction units:

- direct-float32 versus float64 oracle: the maximum absolute prediction delta
  across train and validation is `<= 1e-3`;
- paired-GELU versus direct float32: the corresponding maximum delta is
  `<= 1e-5`.

All emitted original-unit deltas and route metrics must be finite and are
descriptive evidence; they do not independently determine these parity gates.

The float64 oracle's aggregate and nine per-head train standardized MSE values
must be positive and finite. The direct-linear Adam **recovery gate** is train
only and requires all of the following:

- aggregate train standardized MSE divided by the oracle value `<= 1.05`;
- each of the nine head MSE ratios `<= 1.10`;
- train direction, rank, and correlation no more than `0.01` below their oracle
  values; and
- train RMSE/target-RMS no more than `0.05` above its oracle value.

The **clear-failure gate** requires failure of that recovery gate plus either an
aggregate train standardized-MSE ratio `>= 1.25` or a maximum per-head ratio
`>= 1.50`. All ratios must be finite. Validation metrics are descriptive only
and never participate in optimizer recovery, clear-failure, or classification.

After mandatory oracle reproduction, classification is the first matching rule:

1. direct float32 parity fails: `float32_conditioning_failure`;
2. direct float32 passes but paired-GELU parity fails:
   `paired_gelu_execution_failure`;
3. both parity gates pass and direct-linear Adam meets the recovery gate:
   `deep_parameterization_or_optimization_failure`;
4. both parity gates pass and direct-linear Adam meets the clear-failure gate:
   `direct_linear_adam_optimizer_failure`;
5. otherwise: `optimizer_localization_inconclusive`.

These labels diagnose this fixed development procedure only. They do not prove
global information loss or grant authority to change/train the representation.

## Lifecycle and protected boundary

Preparation compiles only through the pinned build wrapper, under a 300-second
GNU timeout with 10-second TERM grace, and occurs before the attempt is consumed;
a preparation failure is not a scientific terminal. Development execution
invokes the evaluator exactly once through one captured GNU timeout child with
the same 300-second bound and TERM grace. On INT, TERM, HUP, or QUIT, the runner
uses a launch-in-progress latch. A signal arriving before PID assignment is
recorded and returned; if the async child already exists, its numeric `$!` is
adopted. Immediately after PID capture, any pending signal is dispatched. No
prior live background job is allowed. The runner signals the captured timeout,
waits and reaps it, and only then seals evidence and terminalizes. Normal
completion likewise reaps before report validation or publication. The private
runtime root is locked; attempt, log, report, terminal, and result artifacts use
atomic no-clobber publication and immutable metadata. Any post-attempt failure,
timeout, signal, malformed report, or incomplete optimizer count terminalizes
the attempt. `--verify-development` is read-only and never compiles or evaluates.

No path containing raw-source, certified, final-holdout, policy, MDN, or
checkpoint authority is permitted. Maximum scientific anchor access is 2815.
Successful output remains development-only and cannot authorize another run.
