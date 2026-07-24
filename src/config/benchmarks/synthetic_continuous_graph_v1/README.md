# Synthetic Continuous Graph Benchmark

This benchmark asks one bounded question:

```text
Can the cwu_02v protocol win on simple deterministic periodic continuous charts?
```

The generated charts are zero-drift periodic log-price functions with enough
cycles in train, evaluation, and holdout ranges. The generator writes
`artifacts/synthetic_periodic_chart_manifest.v1.report` so the cycle count is
explicit.

Benchmark-local generated summaries live under `artifacts/` so they do not sit
beside durable `.dsl` and `.config` inputs. Formal benchmark/gate outputs use
`.report`; investigative probe outputs use `.probe`. Full Runtime job outputs
and job-local `runtime.job_events.probe` streams still belong under the
repo `.runtime` tree, normally `.runtime/benchmarks/...` for benchmark runs or
`.runtime/cuwacunu_exec/...` for ordinary Runtime execution.

Synthetic learning diagnostics use the Runtime `job_events` probe stream rather
than a second probe kind. When a debug Runtime job has probes enabled, the
stream includes classified metric records such as `forecast.training.metric`,
`forecast.oracle.metric`, `representation.training.metric`, and
`representation.augmentation.metric`. Dry-runs at least expose job-state
progress counters; full graph-first representation and MDN runs append
report-snapshot diagnostics at `REPORT_EVERY`, including loss and forecast/oracle
metrics when available. These records carry `series_id`, `step`, component
identity, split role when known, and accepted-anchor bounds. Dense prediction
samples, forecast-vs-oracle tables, and policy action traces should remain
separate files referenced by artifact records.

The benchmark deliberately uses synthetic **kline-shaped** rows, not a special
source tensor path. The rows are registered as `record_type=kline` and flow
through the normal graph-first path:

```text
synthetic kline CSV
  -> Ujcamei graph-anchor retrieval
  -> NodeLift/SRL
  -> representation
  -> MDN forecast
  -> forecast/replay evidence
  -> policy training/evaluation
```

That choice keeps the benchmark close to the production protocol surface while
making the underlying price motion deterministic and inspectable.

Run:

```bash
bash src/scripts/benchmarks/synthetic_continuous_graph_v1/run_periodic_oracle_capability.sh
```

Optional policy comparison:

```bash
bash src/scripts/benchmarks/synthetic_continuous_graph_v1/run_periodic_oracle_capability.sh \
  --policy-report /path/to/policy_eval.report
```

The output lives under:

```text
.runtime/benchmarks/synthetic_continuous_graph_v1/periodic_oracle_capability_v1
```

The capability report returns one of:

```text
pass      policy result exists and meets the benchmark thresholds
fail      policy result exists but misses the thresholds
blocked   chart or protocol probe cannot support the test
unproven  chart/probe are available, but no policy performance report exists
```

The hindsight oracle is non-causal and is not training, readiness, deployment,
or policy-acceptance authority. It is only an upper-bound reference. The
capability answer must come from a real policy evaluation report produced by
the protocol path.

## Forecast Oracle Gate

The benchmark also includes a pre-policy forecast gate:

```text
synthetic_forecast_oracle_accuracy_gate.v1
```

This is not a separate benchmark. It is a stage inside the same simple-chart
benchmark, before policy execution. Its question is:

```text
Does the certified representation/MDN forecast stack understand the deterministic
periodic charts on the protected eval range?
```

The gate must use the existing protected eval range and no-lookahead-certified
forecast artifacts. It must not regenerate forecasts from checkpoints trained
on the eval range. The expected outputs are continuous metrics, not literal
`100% accuracy`:

```text
direction/sign accuracy
rank or best-asset agreement
MAE/RMSE or normalized error against the deterministic target
correlation or phase/lag agreement
MDN uncertainty sanity, where available
```

Lattice now exposes this as the non-dispatch target:

```text
synthetic_forecast_oracle_accuracy_ready
TARGET_CLASS = synthetic_forecast_oracle_gate
SUBJECT_FACT_FAMILY = forecast_eval
PROOF_KIND = synthetic_forecast_oracle_accuracy_bound
```

The gate first requires the usual clean forecast-eval artifact proof and a
passing upstream certified-replay eval target. It then applies benchmark
thresholds to feature-aware forecast-eval oracle metrics:

```text
MAX_ORACLE_EV_MAE = 0.05
MAX_ORACLE_EV_RMSE = 0.075
MIN_ORACLE_DIRECTIONAL_ACCURACY = 0.95
MAX_ORACLE_PRICE_EV_MAE = 0.02
MAX_ORACLE_PRICE_EV_RMSE = 0.025
MAX_ORACLE_ACTIVITY_EV_MAE = 0.07
MAX_ORACLE_ACTIVITY_EV_RMSE = 0.10
MIN_ORACLE_CLOSE_DIRECTIONAL_ACCURACY = 0.95
```

The aggregate fields are retained as compatibility context. The gate decision
uses price/activity magnitude and close-return direction so `log1p` activity
targets cannot hide weak price-phase recovery behind trivial sign accuracy.

The next diagnostic gate projects the close expected values into tradable
synthetic edges using quote node index `0` (`SYNUSD`) and base nodes
`SYNALPHA`, `SYNBETA`, and `SYNGAMMA`:

```text
synthetic_edge_return_projection_oracle_ready
TARGET_CLASS = synthetic_edge_return_projection_oracle_gate
SUBJECT_FACT_FAMILY = forecast_eval
PROOF_KIND = synthetic_edge_return_projection_oracle_bound
MAX_ORACLE_EDGE_RETURN_EV_MAE = 0.05
MAX_ORACLE_EDGE_RETURN_EV_RMSE = 0.075
MIN_ORACLE_EDGE_RETURN_DIRECTIONAL_ACCURACY = 0.95
MIN_ORACLE_EDGE_RETURN_PAIRWISE_RANK_ACCURACY = 0.95
MIN_ORACLE_EDGE_RETURN_BEST_ASSET_AGREEMENT = 0.60
MIN_ORACLE_EDGE_RETURN_CORRELATION = 0.25
```

This gate asks whether the forecast stack is useful in the tradable
base-minus-quote return space. It fails closed as `missing_report` for old
forecast facts without `edge_return_projection_*` fields, and reaches a real
metric decision once a fresh MDN eval emits them.

Current Lattice evidence already answers the admissibility part. For the fresh
synthetic run, `channel_mdn_certified_replay_expansion_eval_ready` is satisfied
with full eval coverage and a passing proof certificate. The underlying
`lattice.forecast_eval.fact` for `[760,1088)` now carries:

```text
support_count = 35424
valid_count = 27552
mean_nll = -2.13232
influence_end = 730
forecast_ev_valid_count = 27552
ev_mae = 0.0312739
ev_rmse = 0.0512565
signed_error = -0.0135866
directional_accuracy = 0.677555
```

So Lattice proves the forecast eval artifact is present, covered,
generation-backed, and no-lookahead-admissible, then reaches the benchmark
oracle metric gate. The first metric-emission result was a real
`metric_failed`, not `missing_report`: the clean forecast passed the aggregate
MAE/RMSE thresholds but failed the old aggregate
`MIN_ORACLE_DIRECTIONAL_ACCURACY = 0.95` with
`directional_accuracy = 0.677555`. The feature-aware gate now refines that
decision so the relevant failing signal is close-return direction, not the
mixed aggregate score.

The current feature-aware result is recorded in:

```text
artifacts/synthetic_feature_aware_forecast_oracle_gate.v1.report
artifacts/synthetic_edge_return_projection_oracle_gate.v1.report
```

The feature-aware result is still a real `metric_failed`, but now the deficit is
the specific close-return direction gate:

```text
metric:synthetic_forecast_oracle
close_directional_accuracy = 0.500254 < 0.95
proof_certificate_check_passed = true
```

The edge-return projection result now reaches a real `metric_failed` decision
against fresh evidence from
`cwu_02v_certified_replay_eval_mdn.run.channel_inference_mdn.attempt_000005`:

```text
edge_return_projection_valid_count = 2952
edge_return_projection_ev_mae = 0.0219482
edge_return_projection_ev_rmse = 0.0277461
edge_return_projection_directional_accuracy = 0.507453 < 0.95
edge_return_projection_pairwise_rank_accuracy = 0.491531 < 0.95
edge_return_projection_best_asset_agreement = 0.295732 < 0.60
edge_return_projection_correlation = 0.00355936 < 0.25
proof_certificate_check_passed = true
```

So the missing-evidence blocker is closed. The model is now failing in
policy-relevant edge-return space, not only in the indirect node-close
direction metric.

Interpretation:

```text
Evidence/provenance problem: closed for this forecast gate.
Forecast quality problem: open.
Policy capability question: still unproven until PPO execution and policy
  evaluation are compared against the synthetic oracle.
```

The low directional accuracy is not yet a policy result. It says the certified
representation/MDN forecast stack is only partially recovering the deterministic
periodic signal on the protected eval range. Before tuning the policy, the next
investigation should explain why this forecast gate is below the deterministic
simple-chart expectation.

The close-direction failure diagnostic is recorded in:

```text
artifacts/synthetic_close_direction_failure_diagnostics.v1.probe
```

It narrows the current failure as follows:

```text
not missing provenance
not insufficient deterministic cycles
not random raw close direction
not missing forecast metrics

open suspects:
  MDN/representation phase-learning failure
  target-space mismatch between NodeLift gauge-node close potential and
    policy-relevant edge close return
  direction-sign semantics in normalized NodeLift target space
```

The train-range probe is recorded in:

```text
artifacts/synthetic_train_range_forecast_fit_probe.v1.probe
```

It evaluated the trained representation/MDN checkpoints over `[0,730)` without
retraining:

```text
train_range.close_directional_accuracy = 0.491438
eval_range.close_directional_accuracy  = 0.500254
```

That rules out a simple "fit train, failed eval" explanation. The close-like
NodeLift target direction is already near random on the same range used to train
representation and MDN.

The direct supervised edge-return probe is recorded in:

```text
artifacts/synthetic_direct_edge_return_supervised_probe.v1.report
```

It showed that the raw target transform is sound and that useful edge signal is
available to simple supervised controls:

```text
target_transform.eval.projected_edge_max_abs_error = 6.9388939039072284e-18
eval.previous_return.directional_accuracy = 0.794715447154
eval.phase_lag_linear.directional_accuracy = 1.000000000000
frozen_representation.eval.directional_accuracy = 0.738482384824
frozen_representation.eval.pairwise_rank_accuracy = 0.707994579946
frozen_representation.eval.best_asset_agreement = 0.597560975610
frozen_representation.eval.correlation = 0.622333108679
```

The first MDN objective-alignment attempt is recorded in:

```text
artifacts/synthetic_mdn_edge_objective_alignment.v1.report
```

It added an explicit edge-return auxiliary term to MDN training, then ran a
fresh Runtime train/eval path using the same representation checkpoint:

```text
train job = train_core_channel_mdn.train.channel_inference_mdn.attempt_000001
train range = [0,730)
eval job = cwu_02v_certified_replay_eval_mdn.run.channel_inference_mdn.attempt_000001
eval range = [760,1088)
```

The auxiliary objective was active, but did not recover tradable edge forecasts:

```text
train.edge_return_projection_directional_accuracy = 0.487915
train.edge_return_projection_pairwise_rank_accuracy = 0.505747
train.edge_return_projection_correlation = -0.000884998

eval.edge_return_projection_directional_accuracy = 0.504065
eval.edge_return_projection_pairwise_rank_accuracy = 0.519309
eval.edge_return_projection_best_asset_agreement = 0.381098
eval.edge_return_projection_correlation = -0.000355394

eval.mean_edge_return_auxiliary_direction_loss = 0.693148
eval.mean_edge_return_auxiliary_rank_loss = 0.693147
```

This narrowed the failure further. The representation contains partial edge
signal under a simple supervised readout, but a light auxiliary projection loss
on the node-potential MDN expected value was not enough.

The dedicated MDN direct edge-return readout test is recorded in:

```text
artifacts/synthetic_mdn_direct_edge_readout.v1.report
```

It added a first-class direct edge-return output:

```text
direct_edge_return: [B, N - 1, C]
```

trained with Huber regression, logistic sign, and pairwise rank losses. The
fresh Runtime path used the same representation checkpoint, trained over
`[0,730)` for `3500` optimizer steps, and evaluated over `[760,1088)`.

The direct readout was active and reported by Runtime, but still did not recover
the deterministic edge signal:

```text
train.direct_edge_return_readout_directional_accuracy = 0.499872
train.direct_edge_return_readout_pairwise_rank_accuracy = 0.519334
train.direct_edge_return_readout_best_asset_agreement = 0.342071
train.direct_edge_return_readout_correlation = -0.00121821

eval.direct_edge_return_readout_directional_accuracy = 0.510840
eval.direct_edge_return_readout_pairwise_rank_accuracy = 0.557249
eval.direct_edge_return_readout_best_asset_agreement = 0.388211
eval.direct_edge_return_readout_correlation = 0.00546659
```

This closes another hypothesis: the failure is not only that edge return was a
weak auxiliary projection on the node-potential expected value. A dedicated
edge-return readout head also fails to learn useful train-range direction/rank
from the current MDN context. The next diagnostic should isolate the signal path
directly: raw target transform, phase/lag oracle, frozen representation probes,
and MDN direct readout on the same ranges before returning to PPO tuning.

The signal-path isolation result is recorded in:

```text
artifacts/synthetic_signal_path_isolation.v1.report
artifacts/synthetic_mdn_context_edge_probe.v1.report
```

It adds a post-MDN-trunk context probe. Runtime emits job-local
`mdn_edge_context_features.probe` files for non-training MDN debug runs, and the
benchmark combines the train/eval rows into:

```text
.runtime/cuwacunu_exec/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_edge_context_features_train_eval.probe
```

The current bounded readout uses the first `256` post-trunk context features,
which include the full base-node and quote-node context vectors. It does not
need the explicit `base - quote` block to form a linear edge readout.

Current comparison on protected eval `[760,1088)`:

```text
phase_lag_oracle.directional_accuracy          = 1.000000000000
previous_return_baseline.directional_accuracy  = 0.794715447154
frozen_representation.directional_accuracy     = 0.738482384824
mdn_post_trunk_context.directional_accuracy    = 0.775745257453
trained_mdn_direct_readout.directional_accuracy = 0.510840

phase_lag_oracle.correlation                   = 1.000000000000
frozen_representation.correlation              = 0.622333108679
mdn_post_trunk_context.correlation             = 0.664624524143
trained_mdn_direct_readout.correlation         = 0.00546659
```

Interpretation:

```text
target transform: passed
known periodic basis: solves the benchmark
frozen representation: partial signal is present
MDN post-trunk context: partial signal is still present
trained MDN direct readout: fails to extract that signal
```

So the next forecast-side work should focus on MDN training objective/readout
optimization dynamics and identity/context scaffolding, not PPO tuning. The
representation is not perfect, but this result argues against a total
representation collapse or a total MDN-context destruction of the signal.

The MDN edge-readout training-dynamics result is recorded in:

```text
artifacts/synthetic_mdn_edge_readout_training_dynamics.v1.report
```

It verifies three separate facts:

```text
target binding: passed
source wiring: passed
direct-head mechanical trainability: passed
```

The representation probe and MDN-context probe bind the same edge targets
exactly (`matched_rows=9522`, `max_abs_error=0`). The direct edge head is
registered under the MDN, included in `model_->parameters()`, contributes to
total loss, uses quote node index `0` and close feature index `3`, and emits
`direct_edge_return`. A targeted production-path canary also proves the head
receives nonzero gradients, updates parameters, and reduces direct regression
loss on a fixed deterministic batch.

The real Runtime training curve remains the problem:

```text
optimizer_steps_final = 3500
direct_readout_valid_count_final = 1916478
last_loss_first = 58.0055
last_loss_final = 6.50973

direct_direction_first = 0.496181
direct_direction_final = 0.499872
direct_direction_max = 0.502505
direct_rank_final = 0.519334
direct_correlation_final = -0.00121821
```

So the direct-readout failure is not a missing target, missing gradient, missing
optimizer update, total representation collapse, or total MDN-context signal
loss. The current leading suspect is full Runtime joint-training dynamics:
direct-readout loss scale/schedule, competition with the MDN NLL objective, or
insufficient identity/context scaffolding for the head during normal training.

`synthetic_mdn_runtime_training_probes.v1` adds visibility-only Runtime probe
fields for the next pass over that failure. MDN training reports now expose NLL
separately from total loss, direct-readout loss ratios, direct-head gradient
norms, direct-head parameter update norms, prediction/target spread and collapse
ratios, margin-conditioned direction/rank metrics, near-zero target share, and
per-edge/per-channel directional-accuracy spread summaries. These are
`runtime.job_events.probe` time-series for debugging. They are not Lattice
facts, readiness gates, or proof authority.

### Forecast Semantics Diagnostic

The fresh semantic diagnostic is recorded in:

```text
artifacts/synthetic_forecast_semantics_diagnostic.v1.probe
```

The diagnostic uses
`cwu_02v_certified_replay_eval_mdn.run.channel_inference_mdn.attempt_000007`,
which emits per-channel, per-target-feature, per-channel-target-feature, and
per-node expected-value metrics into the MDN report, `runtime.result.fact`, and
`lattice.forecast_eval.fact`.

The current direction metric is:

```text
expected.sign() == realized.sign()
```

over normalized target coordinates. That makes the aggregate
`directional_accuracy = 0.677555` hard to interpret as a single forecast-quality
answer:

```text
price features directional_accuracy    = 0.435721750
close-only directional_accuracy        = 0.500254000
activity features directional_accuracy = 1.000000000
```

The activity result is sign-trivial under `log1p` activity targets: it can be
directionally perfect while still having nontrivial magnitude error. The `open`
coordinate is also structurally suspect for sign scoring because the generator
sets open to the previous close; after log-return normalization it behaves like
a near-zero target and currently scores `0`.

So the forecast gate is no longer opaque. The problem is not missing Lattice
evidence. The current benchmark metric mixes weak price-direction recovery with
trivial activity signs. A train-range replay of the trained checkpoints shows
the same close-direction failure on `[0,730)`, so the issue is not just
protected-eval generalization. The next forecast benchmark step should inspect
target-space alignment: close-return sign, edge-return projection, price
correlation or phase/lag, activity magnitude error, and causal baseline
comparisons.

## Current Status

The benchmark uses a local standard `ujcamei.source.splits.dsl` catalog. For the
current 1170 accepted anchors, it materializes:

```text
train_core                      [0,730)
embargo/purge gap               [730,760)
certified_replay_expansion_eval [760,1088)
test_holdout                    [1088,1170)
```

The gap is required because the eval split `[760,1088)` is protected as
`[731,1089)` after the current Hx/Hf dilation. The previous adjacent split
trained upstream components through `[0,760)`, which correctly failed Lattice
with `leakage:protected_split`.

A fresh Marshal-aligned run on 2026-06-25 completed representation training
over `[0,730)`, MDN training over `[0,730)`, certified replay eval over
`[760,1088)`, and one PPO V0 policy execution over `[760,1088)`.

The forecast side is admissible but not good enough for the oracle gate. A fresh
direct Lattice check for `channel_mdn_certified_replay_expansion_eval_ready`
returned `satisfied=true`, `proof_certificate_check_passed=true`, coverage
fraction `1.0`, and protected split `overlap_found=false`. The
`synthetic_forecast_oracle_accuracy_ready` target reached a real metric decision:
the proof passed, but close-directional accuracy was `0.500254`, below the
`0.95` threshold.

The policy handoff no-lookahead certificate was clean:

```text
target_id = policy_execution_input_handoff_ready
proof_kind = policy_execution_input_handoff_bound
no_lookahead_provenance_complete = true
no_lookahead_provenance_admissible = true
proof_certificate_check_passed = true
primary_deficit_key = status:target
suggested_wave = wikimyei.policy.portfolio.graph_node_allocation train [760,1088)
runtime_wave_id = policy_training_ppo_v0
marshal_dispatch_state = ready_for_execution_gate
marshal_blocker_bucket = none
runtime_dry_run_accepted = true
next_safe_action = execution_gate
```

PPO execution then produced a policy checkpoint and quality reports:

```text
job_id = policy_training_ppo_v0_953fb0a6d4a17505.attempt_953fb0a6d4a17505_1782364458930185_43292
checkpoint_digest = eb19364248d36e5946ea822f0b7291f2e5fc66e0d69811126236afb391b643c8
sample_count = 328
optimizer_steps = 1
validation_total_log_growth = 0.00935234
validation_final_equity_numeraire = 10096.1
validation_invalid_action_count = 318
policy_quality_ppo_minus_equal_weight_total_log_growth = 0.00094415
```

The benchmark capability script now normalizes the policy's numeraire equity by
the report's numeraire-only baseline before comparing it with the oracle and
baseline growth multiples. The current result is:

```text
protocol_capable_on_synthetic_periodic_charts = fail
policy_final_equity_growth_multiple = 1.009610000000
oracle_final_equity_numeraire = 2192.881807257396
best_causal_baseline = one_step_momentum
best_causal_baseline_final_equity = 186.906576675422
policy_vs_oracle_ratio = 0.000460403291
policy_vs_best_causal_baseline_ratio = 0.005401682584
```

This is a real performance failure for the current benchmark run, not a missing
policy-execution failure. Separately, `policy_training_artifact_ready` remains
readiness-blocked because Lattice rejects the fresh policy-training fact's
bundle/causal closure:

```text
primary_deficit_key = artifact:policy_training_lineage
issues = bundle_component_missing,
         policy_execution_evidence_snapshot_mismatch,
         bundle_generation_vector_mismatch,
         causal_artifact_production_closure_mismatch,
         causal_provenance_closure_mismatch,
         causal_provenance_subproof_incomplete
```

No-lookahead is clean; readiness is blocked by bundle/causal provenance closure.

## Failure Root Cause

The consolidated root-cause report is:

```text
artifacts/synthetic_benchmark_failure_root_cause.v1.report
```

Current conclusion:

```text
The first decisive failure is pre-policy. The representation/MDN forecast stack
is no-lookahead-clean and finite, but it does not recover the deterministic
close-like phase/direction signal.
```

The most important evidence is that close-direction accuracy is coin-flip both
on protected eval and on the same range used to train representation and MDN:

```text
train_range.close_directional_accuracy = 0.491438
eval_range.close_directional_accuracy  = 0.500254
```

That makes broad PPO tuning the wrong first move. PPO also has real issues
(`328` samples, `1` optimizer step, and `318` invalid validation actions), but
the forecast gate already fails before policy quality should be treated as the
primary explanation.

That edge-return projection gate has now been run:

```text
artifacts/synthetic_edge_return_projection_oracle_gate.v1.report
```

It is weak too: directional accuracy is `0.507453`, pairwise rank accuracy is
`0.491531`, best-asset agreement is `0.295732`, and correlation is
`0.00355936`. This makes the current finding stronger: the representation/MDN
path is not recovering the deterministic synthetic signal in policy-relevant
edge-return space.

The probe-enabled learning diagnostic is recorded in:

```text
artifacts/synthetic_benchmark_learning_diagnostics.v1.probe
```

It ran a bounded fresh diagnostic path with Runtime `job_events` probes enabled:

```text
representation train-core: 320 steps over [0,730)
MDN train-core:            400 steps over [0,730)
```

The representation objective moved:

```text
representation.last_loss: 3.2351 -> 0.224892
representation_effective_rank_fraction: 0.796531 -> 0.719553
```

The MDN objective also moved:

```text
mdn.last_loss: 3.83776 -> 0.655708
aggregate_directional_accuracy_final = 0.64528
```

But the policy-relevant edge-return oracle metrics stayed near random:

```text
edge_return_projection_directional_accuracy: 0.51441 -> 0.513742
edge_return_projection_pairwise_rank_accuracy: 0.529687 -> 0.501575
edge_return_projection_best_asset_agreement_final = 0.327761
edge_return_projection_correlation_final = -0.00116026
```

This is the key learning-curve finding: optimization is not stalled globally,
but loss improvement is not aligned with the deterministic tradable direction
signal. The next useful work is therefore MDN/target/objective alignment and
representation phase readout, not PPO tuning.

The direct edge-return supervised probe is recorded in:

```text
artifacts/synthetic_direct_edge_return_supervised_probe.v1.report
```

It adds a small target-space and supervised-learnability control before touching
PPO:

```text
target_transform.projection_identity_passed = true
target_transform.eval.projected_edge_max_abs_error = 6.9388939039072284e-18

eval.previous_return.directional_accuracy = 0.794715447154
eval.previous_return.pairwise_rank_accuracy = 0.739837398374
eval.previous_return.best_asset_agreement = 0.628048780488

eval.phase_lag_linear.directional_accuracy = 1.000000000000
eval.phase_lag_linear.pairwise_rank_accuracy = 1.000000000000
eval.phase_lag_linear.best_asset_agreement = 1.000000000000
eval.phase_lag_linear.correlation = 1.000000000000
```

The current report includes a frozen-representation edge probe generated by the
real Runtime MDN path. Runtime wrote train and eval
`representation_edge_features.probe` artifacts, the benchmark script combined
them, and a small supervised linear readout was fit on the frozen representation
features. The result is materially better than random but not oracle-grade:

```text
frozen_representation.eval.directional_accuracy = 0.738482384824
frozen_representation.eval.pairwise_rank_accuracy = 0.707994579946
frozen_representation.eval.best_asset_agreement = 0.597560975610
frozen_representation.eval.correlation = 0.622333108679
```

This means the representation is not collapsed and does carry partial
policy-relevant edge signal. It is still weaker than the simple previous-return
baseline and far below the phase/lag oracle, while the MDN edge projection and
the dedicated direct edge-return readout remain near random. The strongest
current suspect is therefore the representation-to-MDN signal path: the
representation exposes some tradable signal under a simple supervised probe, but
the current MDN context/readout/training path does not preserve or use it.

The full probe-guided MDN diagnostic is recorded in:

```text
artifacts/synthetic_mdn_probe_guided_diagnostic_run.v1.probe
```

It ran the durable synthetic benchmark path after a fresh dev-nuke:

```text
representation train-core: 3000 steps over [0,730)
MDN train-core:            3500 steps over [0,730)
certified eval:            [760,1088)
```

The new Runtime `job_events` probe fields make the MDN direct-readout failure
more concrete:

```text
direct_head_receives_gradients = true
direct_head_grad_norm_max = 4.99276
direct_head_parameters_move = true
direct_head_parameter_update_norm_max = 0.0947658
direct_loss_ratio_to_nll_last = 4.02319
direct_loss_ratio_to_total_last = 1.33078

train.direct_edge_return_readout_directional_accuracy = 0.499872
train.direct_edge_return_readout_margin_directional_accuracy = 0.499579
train.direct_edge_return_readout_pairwise_rank_accuracy = 0.519334
train.direct_edge_return_readout_margin_pairwise_rank_accuracy = 0.514689
train.direct_edge_return_readout_correlation = -0.00121821
train.direct_edge_return_readout_pred_to_realized_std_ratio = 0.641866
```

The protected eval artifact from the same fresh checkpoints is also weak:

```text
eval.edge_return_projection_directional_accuracy = 0.499322
eval.edge_return_projection_pairwise_rank_accuracy = 0.467818
eval.edge_return_projection_best_asset_agreement = 0.29878
eval.edge_return_projection_correlation = 0.00583662

eval.direct_edge_return_readout_directional_accuracy = 0.51084
eval.direct_edge_return_readout_margin_directional_accuracy = 0.507517
eval.direct_edge_return_readout_pairwise_rank_accuracy = 0.557249
eval.direct_edge_return_readout_margin_pairwise_rank_accuracy = 0.55418
eval.direct_edge_return_readout_correlation = 0.00546659
eval.direct_edge_return_readout_pred_to_realized_std_ratio = 0.00296166
```

This rules out a dead head, missing optimizer registration, missing gradients,
and a too-small direct-readout loss as primary explanations. The failure is not
obviously localized to one edge or channel. The current best next milestone is
`synthetic_mdn_objective_readout_alignment_ablation.v1`: compare objective
schedules and readout/identity scaffolding while keeping the same durable
synthetic benchmark path and 0.95 oracle thresholds.

The first concrete intervention is
`synthetic_mdn_direct_readout_training_intervention.v1`. It changes code and
durable config before the next run instead of repeating the same failed path:
the MDN `.jkimyei` owns an explicit direct edge-return target scale, an NLL
warmup weight, and an optional direct-head-only warmup. Metrics remain reported
in raw return space. The purpose is to test whether the MDN context already
contains enough edge signal but the raw-scale direct readout was being optimized
through a poorly conditioned objective schedule. This is still a forecast-side
diagnostic; it must not lower the 0.95 deterministic oracle gates.

The full intervention evaluation is recorded in:

```text
artifacts/synthetic_mdn_direct_readout_intervention_eval.v1.probe
```

It ran after a fresh Runtime reset through the durable synthetic benchmark
config and Marshal/Runtime path:

```text
representation train-core: 3000 steps over [0,730)
MDN train-core:            3500 steps over [0,730)
certified eval:            [760,1088)
```

The intervention was active and observable:

```text
direct_readout_target_scale = 36
direct_readout_warmup_steps_config = 800
direct_readout_warmup_nll_weight_config = 0
direct_readout_warmup_direct_head_only_config = true
direct_readout_warmup_steps_observed = 800
direct_readout_direct_head_only_warmup_steps_observed = 800
direct_readout_scheduled_nll_weight_min = 0
direct_readout_scheduled_nll_weight_last = 1
direct_head_receives_gradients = true
direct_head_parameters_move = true
```

The train-range result stayed near random:

```text
train.direct_edge_return_readout_directional_accuracy = 0.498849
train.direct_edge_return_readout_margin_directional_accuracy = 0.499075
train.direct_edge_return_readout_pairwise_rank_accuracy = 0.505641
train.direct_edge_return_readout_margin_pairwise_rank_accuracy = 0.503318
train.direct_edge_return_readout_correlation = 0.00053337
train.direct_edge_return_readout_pred_to_realized_std_ratio = 0.588126
train.edge_return_projection_directional_accuracy = 0.513593
train.edge_return_projection_pairwise_rank_accuracy = 0.504674
train.edge_return_projection_correlation = 0.000500944
```

The protected eval result also stayed weak:

```text
eval.edge_return_projection_directional_accuracy = 0.51084
eval.edge_return_projection_pairwise_rank_accuracy = 0.476626
eval.edge_return_projection_best_asset_agreement = 0.333333
eval.edge_return_projection_correlation = -0.0175664

eval.direct_edge_return_readout_directional_accuracy = 0.51084
eval.direct_edge_return_readout_margin_directional_accuracy = 0.507517
eval.direct_edge_return_readout_pairwise_rank_accuracy = 0.498645
eval.direct_edge_return_readout_margin_pairwise_rank_accuracy = 0.496732
eval.direct_edge_return_readout_correlation = 0.00461366
eval.direct_edge_return_readout_pred_to_realized_std_ratio = 0.0117962
```

This closes the narrow hypothesis that the original failure was caused only by
raw target scale, NLL competition during early training, or missing direct-head
gradient flow. The direct head trained, moved, and had an explicit direct-only
warmup, but it still did not learn deterministic edge-return direction or rank
from the current MDN context path. The next forecast-side branch should change
the information pathway or target/readout structure, not merely repeat the same
schedule. Current candidate:
`synthetic_mdn_objective_readout_alignment_ablation.v1`.

The direct-head feature-surface comparator is recorded in:

```text
artifacts/synthetic_mdn_edge_identity_readout_comparator.v1.probe
```

It uses the fresh `mdn_edge_context_features.probe` from the completed
intervention eval and fits small offline readouts over the same rows:

```text
comparator fit range:     [760,989)
comparator holdout range: [989,1088)

shared_no_edge_id.holdout.directional_accuracy = 0.581369
shared_no_edge_id.holdout.pairwise_rank_accuracy = 0.590348
shared_no_edge_id.holdout.correlation = 0.254592

shared_edge_id.holdout.directional_accuracy = 0.582492
shared_edge_id.holdout.pairwise_rank_accuracy = 0.581369
shared_edge_id.holdout.correlation = 0.254598

per_edge.holdout.directional_accuracy = 0.685746
per_edge.holdout.pairwise_rank_accuracy = 0.672278
per_edge.holdout.best_asset_agreement = 0.572391
per_edge.holdout.correlation = 0.163083
```

Interpretation:

```text
per-edge specialization helps
simple one-hot edge id does not materially help the shared linear readout
the MDN direct-head feature surface still does not expose oracle-grade signal
```

The result narrows the next code branch. Do not merely append a silent edge id
to the final projection and expect the benchmark to pass. The next intervention
should give the direct readout stronger identity/context scaffolding and a
trainable information pathway, for example an identity-conditioned or per-edge
direct readout plus learned node/edge embeddings or a direct-readout adapter
whose upstream features are allowed to move during the direct objective.

Implementation status:

```text
synthetic_mdn_identity_conditioned_direct_readout_ablation.v1
```

is now implemented and evaluated through the durable synthetic train/eval path.
The MDN direct edge-return readout is configured from durable `.jkimyei`
settings, with explicit modes:

```text
shared
edge_embedding
per_edge
edge_embedding_per_edge
```

The active synthetic MDN profile selects:

```text
MDN_DIRECT_EDGE_RETURN_READOUT_IDENTITY_MODE = edge_embedding_per_edge
MDN_DIRECT_EDGE_RETURN_READOUT_BASE_EDGE_COUNT = 3
MDN_DIRECT_EDGE_RETURN_READOUT_IDENTITY_EMBEDDING_DIM = 16
MDN_DIRECT_EDGE_RETURN_READOUT_ADAPTER_HIDDEN_DIM = 128
```

A focused launcher test proves the non-default mode is parsed from `.jkimyei`,
materialized into the MDN model, recorded in the training report, and bound into
checkpoint identity. The fresh result is recorded at:

```text
src/config/benchmarks/synthetic_continuous_graph_v1/artifacts/synthetic_mdn_identity_conditioned_direct_readout_ablation.v1.probe
```

The intervention was active and healthy in the mechanical sense:

```text
direct_head_receives_gradients = true
parameters_are_moving = true
direct_readout_loss_scale = comparable_to_nll
predictions_collapsed_vs_targets = variance_present
```

It did not solve the deterministic benchmark:

```text
directional_accuracy = 0.514161
margin_directional_accuracy = 0.516307
pairwise_rank_accuracy = 0.508168
margin_pairwise_rank_accuracy = 0.508807
correlation = -0.00177698
```

So explicit edge identity, per-edge projection, and the residual direct-readout
adapter are not sufficient by themselves. The next branch should inspect
objective/readout alignment and target weighting, while keeping the strict
deterministic oracle thresholds.

```text
synthetic_mdn_objective_readout_alignment_ablation.v1
```

is the next bounded objective-schedule diagnostic. The important gap in the
prior identity-conditioned run is that the direct-readout warmup could suppress
NLL only during the warmup window; after warmup the scheduled NLL weight always
returned to `1.0`. That means the run did not truly answer whether the ordinary
MDN NLL objective is overpowering or redirecting the edge-return readout after
the initial direct-head-only phase.

The MDN `.jkimyei` now declares:

```text
MDN_DIRECT_EDGE_RETURN_READOUT_POST_WARMUP_NLL_WEIGHT
```

with neutral value `1.0`. The benchmark wrapper
`run_mdn_objective_readout_alignment_ablation.sh` can run a durable synthetic
diagnostic variant such as `edge_dominant_post_nll_0`, which sets the
post-warmup NLL weight to `0.0` for the run and restores the `.jkimyei` on exit.
The deterministic oracle gates remain unchanged at `0.95`; this ablation only
tests objective/readout alignment, not a relaxed success criterion.

## Cached Runtime-Head Parity and Augmentation Characterization

The 2026-07-13 cached-feature parity diagnostic changes the next-experiment
ordering. It reconstructs the exact post-adapter node context from the latest
surviving 400-feature MDN probe and trains the unchanged production
`DirectEdgeReturnHead` without the MDN trunk, NLL, representation, or policy.

Structural reconstruction is exact, but the active production head remains
random on its own fit split:

```text
production edge_embedding_per_edge fit:
  direction = 0.508976
  rank      = 0.508491
  corr      = 0.002458

fit-only standardized per-edge closed-form control:
  direction = 0.859292
  rank      = 0.842310
  corr      = 0.850094
```

Per-sample LayerNorm reduces that linear control to
`0.629306/0.603105/0.377413`, proving a major conditioning loss. The current
Adam recipe also fails to reach the closed-form solution when given a convex,
fit-only standardized per-edge linear head, even after no-clip, quadratic-loss,
20,000-step, full-batch, higher-rate, and zero-initialization controls.

The production readout architecture and solver conditioning are confirmed
failures. An initial CPU `float64` ridge split inside the small protected-eval
cache was unstable and correctly retained as diagnostic-only:

```text
selection validation:       RMSE=0.027788 direction=0.500000 rank=0.485507
diagnostic confirmation:    RMSE=0.028203 direction=0.529742 rank=0.565657
```

The frozen checkpoints were then replayed over `[0,730)` with zero optimizer
steps, producing `6,570` training rows. The real gate selects alpha wholly
inside training using `[0,554)`, purge `[554,584)`, and validation `[584,730)`.
No candidate clears the unchanged `0.95` direction and rank thresholds. The
lowest-RMSE diagnostic fallback (`alpha=1e-10`) records:

```text
inner train validation: direction=0.805936 rank=0.793760 corr=0.687220
protected eval:         direction=0.810976 rank=0.785908 corr=0.740333
```

Thus the frozen post-adapter features contain stable, material signal, and the
near-random production head is a genuine readout failure. The remaining gap to
`0.95/0.95` is also genuine upstream representation/context quality loss. A
calibrated readout can recover much of the present signal but cannot solve the
benchmark alone. The controlled outer-input augmentation ablation has now
completed; its result is recorded below. The unconsumed `[1088,1170)` holdout
remains sealed.

The new deterministic augmentation test separately records:

```text
light_phase_safe_v2 retention: H4=0.5, H10=0.8, H30=0.9
period-8 oracle direction:     H4=0.96875, H10=1.0, H30=1.0
```

Temporal augmentation is disproportionately destructive to short histories,
but it does not by itself explain a random forecast head.

Evidence:

```text
artifacts/synthetic_mdn_cached_feature_runtime_head_parity.v1.report
artifacts/synthetic_mdn_frozen_affine_calibration_sidecar_v1.report
artifacts/synthetic_mdn_frozen_affine_objective_ladder_v1.report
artifacts/synthetic_mdn_frozen_affine_optimizer_parity_v1.report
artifacts/synthetic_mdn_frozen_feature_ridge_diagnostic.v1.report
artifacts/synthetic_mdn_frozen_train_eval_ridge_gate.v1.report
artifacts/synthetic_mtf_augmentation_phase_diagnostic.v1.report
artifacts/synthetic_mtf_outer_input_augmentation_off_v1.report
artifacts/synthetic_mtf_vicreg_weak_view_augmentation_off_v1.report
artifacts/synthetic_mtf_vicreg_gradient_off_v1.report
```

Reproduction:

```text
src/scripts/benchmarks/synthetic_continuous_graph_v1/
  run_mdn_cached_feature_runtime_head_parity.sh
  run_mdn_frozen_affine_calibration_sidecar.sh
  run_mdn_frozen_affine_objective_ladder.sh
  run_mdn_frozen_affine_optimizer_parity.sh
  run_mdn_frozen_feature_capture.sh
  run_mtf_outer_input_augmentation_off_ablation.sh
  run_mtf_vicreg_weak_view_augmentation_off_ablation.sh
  run_mtf_vicreg_gradient_off_ablation.sh

src/config/benchmarks/synthetic_continuous_graph_v1/
  wikimyei.representation.mtf_jepa_mae_vicreg.vicreg_weak_view_augmentation_off_v1.jkimyei
  wikimyei.representation.mtf_jepa_mae_vicreg.vicreg_gradient_off_v1.jkimyei

SYNTHETIC_MDN_RUNTIME_HEAD_PARITY_RIDGE_ONLY=true \
  bash \
  src/scripts/benchmarks/synthetic_continuous_graph_v1/run_mdn_cached_feature_runtime_head_parity.sh

make -C src/tests/bench/jkimyei/training/channel_graph_first_launchers \
  run-test_jkimyei_mtf_jepa_mae_vicreg_augmentations
```

## Outer-Input Augmentation-Off Result

The 2026-07-14 control retrained a fresh representation and matched MDN while
neutralizing only the six launcher-owned outer mechanisms. The hidden VICReg
weak-view time drop (`0.01`) and Gaussian jitter (`0.005`) remained active, so
this is an outer-stack control rather than a complete no-augmentation run.

Values are `direction / rank / correlation / RMSE`:

| Surface and split | Baseline | Outer off | Delta |
|---|---:|---:|---:|
| Raw representation, inner validation | `0.730594 / 0.700913 / 0.578101 / 0.023612` | `0.696347 / 0.694064 / 0.546996 / 0.023653` | `-0.034247 / -0.006849 / -0.031106 / +0.000041` |
| Raw representation, historical eval | `0.743225 / 0.722900 / 0.627061 / 0.021694` | `0.707656 / 0.693428 / 0.584996 / 0.022502` | `-0.035569 / -0.029472 / -0.042065 / +0.000809` |
| Post-MDN, inner validation | `0.805936 / 0.793760 / 0.687220 / 0.020837` | `0.765601 / 0.763318 / 0.662225 / 0.021203` | `-0.040335 / -0.030441 / -0.024995 / +0.000366` |
| Post-MDN, historical eval | `0.810976 / 0.785908 / 0.740333 / 0.018693` | `0.775068 / 0.758130 / 0.706236 / 0.019694` | `-0.035908 / -0.027778 / -0.034097 / +0.001001` |

The raw selected alpha moved from `1e-10` to `1e-8`; a fixed-`1e-10` control
also worsened historical direction/rank to `0.718157/0.701220`. Post-MDN alpha
stayed at `1e-10`. Both selected outputs replay byte-identically, both gates
remain below `0.95/0.95`, and `[1088,1170)` was never opened.

The outer stack is therefore not the primary bottleneck and appears modestly
helpful in this one-seed diagnostic. Effective rank and conditioning improved
when it was disabled, but task-aligned decodability worsened, so geometry-only
health metrics are not sufficient. The follow-up restored
`light_phase_safe_v2` and disabled only the model-internal VICReg weak views;
its result is below.

For the raw probe, the comparator's emitted post-adapter boundary label is
misleading: the actual surface is the pre-MDN-adapter `H=32` representation
encoding with `[base, quote, base - quote]`. The post-MDN `H=128`/400-feature
probe is the true post-adapter surface.

## Hidden VICReg Weak-View Augmentation-Off Result

The follow-up changed only the internal weak-view Gaussian jitter (`0.005 ->
0`) and time-dropout scale (`0.10 -> 0`, effective probability `0.01 -> 0`).
The zero-effect implementation still consumed the legacy RNG draws. Outer
augmentation, masks, losses, optimizer, seed, 3,000/3,500-step budgets, and
all ranges stayed fixed.

Values are `direction / rank / correlation / RMSE`:

| Surface and split | Baseline | Hidden weak views off | Delta |
|---|---:|---:|---:|
| Raw representation, inner validation | `0.730594 / 0.700913 / 0.578101 / 0.023612` | `0.688737 / 0.694064 / 0.550651 / 0.024272` | `-0.041857 / -0.006849 / -0.027450 / +0.000661` |
| Raw representation, historical eval | `0.743225 / 0.722900 / 0.627061 / 0.021694` | `0.724255 / 0.708333 / 0.610111 / 0.022026` | `-0.018970 / -0.014566 / -0.016950 / +0.000332` |
| Post-MDN, inner validation | `0.805936 / 0.793760 / 0.687220 / 0.020837` | `0.805936 / 0.785388 / 0.682145 / 0.020944` | `+0.000000 / -0.008371 / -0.005075 / +0.000108` |
| Post-MDN, historical eval | `0.810976 / 0.785908 / 0.740333 / 0.018693` | `0.798780 / 0.778794 / 0.753269 / 0.018278` | `-0.012195 / -0.007114 / +0.012936 / -0.000415` |

Both surfaces kept `alpha=1e-10`, both selected outputs replay byte-identically,
and neither clears `0.95/0.95`. Historical post-MDN correlation and RMSE move
slightly in the favorable direction, but the direction/rank gate does not:
the weak views are not the primary bottleneck and may be modestly useful at
this seed. `[1088,1170)` remains unopened.

The representation objective is still dominated by global VICReg pressure:
its weighted contribution is about `0.197399`, 82% of mean loss `0.240950`.
The next clean upstream test keeps both augmentation layers and the VICReg
branch active while changing only `LAMBDA_VICREG=0.05 -> 0`.

## Aggregate VICReg Gradient-Off Result

The follow-up made only that one-line coefficient change. The VICReg branch,
canonical weak views, RNG draws, outer augmentation, masks, optimizer, model,
seeds, 3,000/3,500-step budgets, and all ranges stayed fixed. The control still
computed the global VICReg aggregate, but its contribution to the representation
gradient was zero.

This sharply improved intrinsic latent geometry:

```text
effective-rank fraction: 0.665276 -> 0.715283
condition number:        94.1924  -> 31.1743
isotropy score:          0.014639 -> 0.0373614
```

It did not improve the forecasting task. Values are
`direction / rank / correlation / RMSE`:

| Surface and split | Baseline | VICReg gradient off | Delta |
|---|---:|---:|---:|
| Raw representation, inner validation | `0.730594 / 0.700913 / 0.578101 / 0.023612` | `0.729072 / 0.701674 / 0.556664 / 0.023451` | `-0.001522 / +0.000761 / -0.021438 / -0.000161` |
| Raw representation, historical eval | `0.743225 / 0.722900 / 0.627061 / 0.021694` | `0.741531 / 0.695799 / 0.602476 / 0.022208` | `-0.001694 / -0.027100 / -0.024585 / +0.000514` |
| Post-MDN, inner validation | `0.805936 / 0.793760 / 0.687220 / 0.020837` | `0.799087 / 0.755708 / 0.658574 / 0.021759` | `-0.006849 / -0.038052 / -0.028646 / +0.000922` |
| Post-MDN, historical eval | `0.810976 / 0.785908 / 0.740333 / 0.018693` | `0.786924 / 0.762195 / 0.722576 / 0.019257` | `-0.024051 / -0.023713 / -0.017756 / +0.000564` |

Raw alpha selection moved from `1e-10` to `1e-9`. At fixed canonical
`alpha=1e-10`, validation direction/rank is `0.728311/0.702435` and historical
direction/rank is `0.742886/0.697493`; the historical rank regression remains.
Raw selected, raw fixed-alpha, and post-MDN probes each replay byte-identically.
Both selected gates remain below `0.95/0.95`, and `[1088,1170)` remains
unopened.

The matched production direct head also remains near random at
`0.515033/0.509114` direction/rank with correlation `-0.001655`. Aggregate
VICReg removal therefore does not solve either failure. At seed 17 it appears
useful for task decodability, especially after MDN context, despite making the
latent geometry look less ideal. Keep canonical augmentation and
`LAMBDA_VICREG=0.05`; do not continue broad augmentation/VICReg removal.

The specialized MTF report does not accumulate the already-computed VICReg
similarity, variance, and covariance subterms. This run cannot identify a
component cause. If component work is revisited, add no-math-change telemetry
and measure weighted shared-encoder gradients before choosing one component
arm.

The sealed result is recorded in
`artifacts/synthetic_mtf_vicreg_gradient_off_v1.report` and reproduced by
`src/scripts/benchmarks/synthetic_continuous_graph_v1/run_mtf_vicreg_gradient_off_ablation.sh`.

## Actual-Train-Range Affine Optimizer Parity Result

The frozen post-adapter surface was next tested on the real development range
`[0,730)`, with the already-consumed historical diagnostic range
`[760,1088)` used only for confirmation. Adam settings were fixed in advance.
The analytic ridge retained its purged train-only selection split and refit on
all development anchors.

| Readout and split | Direction | Rank | RMSE |
|---|---:|---:|---:|
| Production head, development | `0.483257` | `0.512938` | `0.028359` |
| Production head, historical | `0.481030` | `0.508130` | `0.028395` |
| Trainable affine Adam, development | `0.543836` | `0.553425` | `0.035386` |
| Trainable affine Adam, historical | `0.537940` | `0.550813` | `0.035622` |
| Analytic affine, development refit | `0.841705` | `0.824962` | `0.016640` |
| Analytic affine, historical | `0.810976` | `0.785908` | `0.018693` |

This reproduces the optimizer/readout failure on the actual training range.
The decisive copy control also proves that the registered affine module can
represent the closed-form solution: copying the analytic mean, scale, weights,
and biases into its per-edge `nn::Linear` modules changed predictions by at
most `4.55e-13` on development and `4.66e-10` historically, with zero
direction/rank change and sub-`1e-13` RMSE drift. The parameterization and
feature reconstruction are sound; the current Adam plus production-objective
path fails to find the representable solution.

This does not yet separate optimizer from objective, wire the affine sidecar
into the live MDN checkpoint path, or prove that no honest nonlinear readout
can exceed the affine result. Affine decodability remains below the
`0.95/0.95` gate. The two GPU baselines and two copy controls each replay
byte-identically. The
legacy captures are protected by a fresh full-content evidence manifest rather
than claimed as originally stage-sealed. Maximum inspected anchor is 1087;
`[1088,1170)` remains unopened.

The durable record is
`artifacts/synthetic_mdn_frozen_affine_optimizer_parity_v1.report`; reproduce
it with
`src/scripts/benchmarks/synthetic_continuous_graph_v1/run_mdn_frozen_affine_optimizer_parity.sh`.

## Exportable Diagnostic Affine Calibration Sidecar Result

The development-only analytic affine refit is now a real public diagnostic
module, `PerEdgeAffineReturnHead`, with strict save/load metadata and no live
MDN or policy wiring. It accepts post-direct-adapter context or the exact
400-value cached readout feature tensor. The cached first `3H=384` values are
required to equal `[base, quote, base - quote]` reconstructed from context; the
remaining 16 cached identity values are explicitly ignored. The canonical run
found bit-exact feature prefixes across all inspected rows.

The artifact records only fit/selection provenance: fit `[0,554)`, purge
`[554,584)`, validation `[584,730)`, refit `[0,730)`, and `valid_from=730`.
Historical confirmation is deliberately absent from its metadata. Its loaded
metrics reproduce the analytic decoder:

| Split | Direction | Rank | RMSE | Maximum analytic prediction delta |
|---|---:|---:|---:|---:|
| Development `[0,730)` | `0.841705` | `0.824962` | `0.016640` | `4.55e-13` |
| Historical `[760,1088)` | `0.810976` | `0.785908` | `0.018693` | `4.66e-10` |

Source and loaded tensors are exactly equal, all parameters remain frozen,
canonical metadata and semantic state digest round-trip, and context,
cached-readout, in-memory, and reloaded predictions are bit-exact with one
another. Main/replay probes and metadata are byte-identical. The physical
Torch archives differ because their basenames differ; archive byte identity is
observational only, while semantic state and loaded predictions are the
acceptance authority.

The sidecar therefore closes the export/reload and feature-contract question,
not the `0.95/0.95` quality gap. It remains diagnostic-only. Its durable record
is
`artifacts/synthetic_mdn_frozen_affine_calibration_sidecar_v1.report`; reproduce
or verify it with
`src/scripts/benchmarks/synthetic_continuous_graph_v1/run_mdn_frozen_affine_calibration_sidecar.sh`.
Maximum inspected anchor is 1087; `[1088,1170)` remains unopened.

The next readout control should hold Adam, initialization, batching, seed, and
frozen features fixed while adding objective ingredients in order: raw-target
MSE; target scaling plus Huber regression; direction; then rank. Judge every
arm against the reloaded sidecar using the existing closeness tolerances and do
not tune on historical confirmation. Failure of raw-target MSE would redirect
the diagnosis to optimizer, batching, and conditioning.

## Frozen-Affine Objective Ladder Result

That predeclared ladder is now complete on the exact frozen post-adapter
development surface. Selection used fit `[0,554)`, purge `[554,584)`, and
validation `[584,730)`. No arm passed, so refit was inapplicable, the
previously consumed historical confirmation `[760,1088)` was not opened, and
the final `[1088,1170)` holdout remains sealed. Maximum loaded anchor is 729.

The float64 analytic ridge oracle reached
`0.805936 / 0.793760 / 0.020837` direction/rank/RMSE. Its separately remapped,
frozen CUDA-float32 affine copy reached
`0.806697 / 0.794521 / 0.020834` and passed every parity gate, proving that the
registered candidate surface can express an oracle-level solution.

| Arm | Direction | Rank | RMSE | Result |
|---|---:|---:|---:|---|
| L0 raw MSE, mini-batch Adam | `0.574581` | `0.588280` | `0.028125` | fail |
| L1 scaled Huber | `0.513699` | `0.528919` | `0.033824` | fail |
| L2 plus direction | `0.514460` | `0.515982` | `0.031274` | fail |
| L3 plus rank | `0.508371` | `0.528919` | `0.034331` | fail |
| B2 raw MSE, full-batch Adam | `0.559361` | `0.570015` | `0.028043` | fail |
| B3 PCA-whitened mini-batch Adam | `0.771689` | `0.767884` | `0.022291` | fail |
| B4 raw MSE, full-batch LBFGS | `0.700913` | `0.694064` | `0.024414` | fail |

L0 and B2 clipped zero updates, so clipping and mini-batch noise are not the
primary explanation. In contrast, every L1-L3 update clipped, with final
full-gradient norms above 12,900; the scaled composite objective aggravates
the failure but cannot be its sole cause because raw MSE also fails.

The strongest boundary is conditioning. Each raw 384-coordinate edge surface
retains only 26 eigen-directions at the declared threshold, has nullity 358,
effective rank near 1.41, and retained condition number above `6.6e9`.
Standardization raises retained ranks to only `121/119/120`, while condition
numbers remain near `1e10`. PCA whitening recovers most of the available
signal, but B3 still misses the oracle gates by `0.034247` direction,
`0.025875` rank, and `0.001454` RMSE. Its raw-weight collapse also misses the
separate float32 deployability delta by `6.50e-6`.

The diagnosis is therefore two-layered: the current training path fails to
recover signal the same CUDA affine module can represent, and the analytic
affine ceiling itself remains below the benchmark's `0.95/0.95` requirement.
Optimizer/conditioning repair comes first; upstream feature quality remains a
real second problem.

The canonical runner sealed the development capture and every actual input.
It carried the historical probe only as a conditional CLI pathname and did not
require, read, validate, hash, or seal its raw content. Since selection was
empty, the helper never opened it. Main and replay probes are byte-identical at
`1b43e31eae93a8a88ebe07b7ac9c7ab23b4881bf4b101f60566694286f8b4c7f`,
both success logs are empty, and the exact validator pins 1,109 result keys.

The durable record is
`artifacts/synthetic_mdn_frozen_affine_objective_ladder_v1.report`; reproduce
or verify it with
`src/scripts/benchmarks/synthetic_continuous_graph_v1/run_mdn_frozen_affine_objective_ladder.sh`.

## Frozen-Affine Conditioning-Parity Result

The ordered follow-up is complete on the same sealed post-adapter development
surface, with fit `[0,554)`, purge `[554,584)`, validation `[584,730)`, and no
anchor above 729 loaded. It compares retained PCA coordinates, ordinary
full-rank coordinates, and a full-rank damped eigenspace under the same
`alpha=1e-10` mapped-weight ridge objective. It remains diagnostic-only,
contains no policy path, and leaves `[1088,1170)` sealed.

Analytic rows in this experiment are fit-objective noninferiority references:
they are optimized on the fit range and then measured on validation. The
separate affine sidecar was fitted on all `[0,730)` development anchors, so
its metrics on `[584,730)` are descriptive and in-sample only; it is not a
gate reference.

| Method | Direction | Rank | RMSE | Scientific result | Raw-f32 collapse |
|---|---:|---:|---:|---|---|
| Full analytic, all 384 modes | `0.805936` | `0.793760` | `0.02083634` | reference | fail, `1.814e-4` |
| R0 retained PCA analytic, ranks `121/119/120` | `0.765601` | `0.756469` | `0.02251255` | operational fail | pass, `1.701e-5` |
| R1 retained PCA, full-batch Adam | `0.762557` | `0.754947` | `0.02255043` | pass | pass, `1.756e-5` |
| R1 retained PCA, full-batch LBFGS | `0.765601` | `0.756469` | `0.02251255` | pass in 3 iterations | pass, `1.901e-5` |
| R2 ordinary full-rank LBFGS | `0.694064` | `0.691781` | `0.02452816` | fail after 500 iterations | pass for failed state |
| R2 damped full-rank LBFGS | `0.805936` | `0.793760` | `0.02083634` | pass in 3 iterations | fail, `1.654e-4` |

The retained analytic reference itself misses the full-reference gate by
`0.040335` direction, `0.037291` rank, and `0.001676` RMSE. PCA truncation
therefore discards material signal; optimizer repair inside that reduced basis
cannot recover the all-mode result. Explicit retained-coordinate LBFGS does,
however, recover its own fit objective to a `5.43e-12` relative gap, proving
that the reduced surface is straightforward to optimize when left
uncollapsed.

The full-rank comparison isolates the conditioning failure on this affine
diagnostic. Ordinary LBFGS exhausts 500 iterations with an `0.8746` relative
objective gap. The damped coordinate system keeps every one of the 384 modes
and changes no objective, data, initialization, or solver, yet reaches a
`3.23e-14` relative gap in three iterations and matches the full reference.
Coordinate conditioning therefore causally closes this measured optimizer
gap. It does not explain why the full affine reference itself remains below
the benchmark's `0.95/0.95` requirement, and it makes no new causal claim
about augmentation.

Deployment remains a separate issue. Direct transformed-coordinate float32
evaluation has only `3.64e-9` maximum quantization error, but collapsing the
successful full-rank damped state into raw float32 affine weights changes
development predictions by `1.654e-4`, above the `2e-5` tolerance. The
ordinary arm's collapse passes only because it stopped far from the desired
high-weight solution. A production repair should therefore retain centered
damped coordinates explicitly or establish a higher-precision collapse
contract; scientific parity alone is insufficient.

Main and replay probes are byte-identical at
`1f218f938d965cdb2ee6ef59dc4152a35e4c41ec3189b95dd47dc16252485fd1`,
both success logs are empty, and all 608 keys are pinned. The canonical
master-manifest file is sealed at
`414242616aa71e74aaa5812a05d620073099b26cb7c014a0bc3b75eeb480329e`.
The durable record is
`artifacts/synthetic_mdn_frozen_affine_conditioning_parity_v1.report`;
reproduce or verify it with
`src/scripts/benchmarks/synthetic_continuous_graph_v1/run_mdn_frozen_affine_conditioning_parity.sh`.

## Frozen-Affine Deployment-Bridge Result

The conditioned full-rank affine solution now crosses an actual diagnostic
runtime boundary. The canonical experiment is development-only: it uses
anchors `[0,730)`, with fit `[0,554)`, purge `[554,584)`, validation
`[584,730)`, and maximum loaded anchor 729. The final `[1088,1170)` holdout
remains sealed. No policy path is present; the v2 artifact is run-only and
policy-ineligible.

The bridge reconstructs the conditioning diagnostic's damped analytic state,
prediction digest, and fit objective exactly. The trained damped arm differs
from that analytic reference by at most `1.8626451e-8`, so this is a deployment
test of the successful conditioned solution rather than a new fit.

| Runtime path | Maximum development delta | `2e-5` gate |
|---|---:|---|
| Uncentered mapped float32 | `1.5605241e-4` | fail |
| Centered mapped float32 | `1.5287660e-4` | fail |
| Explicit damped transform in float32 | `2.1270663e-4` | fail |
| Float32 normalize, then centered mapped float64 | `1.6942620e-5` | pass |

The passing path preserves the capture's exact float32 normalization order,
promotes on the same device, then subtracts the fit-only edge center and
performs the mapped-weight dot product plus bias in float64 before casting the
output to float32. Centering in float32 is not enough, and evaluating the
explicit damped transform wholly in float32 is worse. The matched centered
comparison therefore identifies higher-precision accumulation as the repair
for this measured deployment-arithmetic failure.

The v2 artifact's semantic digest is
`84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af0c38`.
CPU/CUDA maximum disagreement is only `1.4551915e-11`; full/chunked,
in-memory/reloaded, and suffix-perturbation comparisons are exact. The
main/replay probe is byte-identical at
`00a382d060d20d7a8277d9f120fdc6578c572072264847157424c8a09207d3fb`,
with 504 pinned keys and key-set digest
`275b86d4ece31ec00bbb9591724d439335d9de5f9a72a99fe84d0fd1abc82f6b`.
The archives are byte-identical at
`7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739`,
and the master-manifest file is sealed at
`467c69c9a05035c9b58f26d90e29df127fdc11dec36f4e7deed8c7b905b23a91`.

The deployment pass is narrow: `1.6942620e-5` consumes `84.7131%` of the
tolerance and leaves about `3.06e-6` headroom. It proves this arithmetic and
archive contract on the frozen affine development surface; it does not prove
upstream representation quality, production nonlinear forecasting recovery,
augmentation causality, or policy performance.

The durable record is
`artifacts/synthetic_mdn_frozen_affine_deployment_bridge_v1.report`;
reproduce or verify it with
`src/scripts/benchmarks/synthetic_continuous_graph_v1/run_mdn_frozen_affine_deployment_bridge.sh`.

## Raw-History MDN Isolation Result

The production `ChannelContextMdn` and canonical NLL trainer have now been
tested without the representation engine or policy. The diagnostic projects
30 causal 1d close returns into the exact four-node uniform gauge and predicts
the next node return. It uses effective fit anchors `[1,554)`, purge
`[554,584)`, and validation `[584,730)`; the helper receives only frozen
760-row CSV prefixes, loads no anchor above 729, and never opens historical
`[760,1088)` or final holdout `[1088,1170)` data.

| Arm | Validation direction | Margin direction | Rank | RMSE | NLL | Diagnostic result |
|---|---:|---:|---:|---:|---:|---|
| K=1 | `0.965753` | `1.000000` | `1.000000` | `0.00023336` | `-4.604219` | pass |
| K=3 | `0.986301` | `1.000000` | `1.000000` | `0.00028118` | `-4.258433` | main validation passes; canary fails |

This establishes that the raw synthetic charts are sequentially learnable and
that the MDN backbone, distribution path, and NLL objective can learn them
without a representation checkpoint. K=1 passes every gate. K=3 also learns
the main fit/validation surface, but its 16-anchor repeated-batch density
canary fails because NLL worsens from `-0.745463` to `1.868101` even while
margin direction and rank reach 1.0. In that canary, mixture entropy contracts
from `1.048788` to `0.028079`, component 2 usage reaches `0.996120`, and mean
sigma falls from `0.108147` to `0.006464`.

The K=3 observation supports a mixture/density-dynamics hypothesis, not a
single-seed causal attribution. NLL scores absolute four-node densities while
the forecast metrics score base-minus-quote differences of mixture
expectations, so common-mode node error, component cancellation, over-tight
sigma, or a few density outliers can preserve excellent edge forecasts while
damaging likelihood. The result rules out "these simple charts are not
forecastable" and "the MDN cannot learn raw causal history" as explanations
for the near-random integrated result. It does not by itself distinguish the
representation, representation-to-MDN interface, or integrated optimization
path.

Main and replay probes are byte-identical at
`f5b14f48081b84100f153635cd2663a76a08b5598ad6d8ac4bbf38f700918ecb`;
the master-manifest file is sealed at
`7745ec13ba9d914bfab289980d4f55b4cc79c57399703fe98c43ef8310964eac`.
The durable record is
`artifacts/synthetic_mdn_raw_history_isolation_v1.report`; reproduce or verify
it with
`src/scripts/benchmarks/synthetic_continuous_graph_v1/run_mdn_raw_history_isolation.sh`.

## Frozen-Representation K=1 Isolation Result

The same K=1 `ChannelContextMdn` and NLL schedule were then applied to the
frozen production representation, still without policy, checkpoint writes,
historical confirmation, or final holdout access. The probe reconstructs the
channel-2 node context exactly from the canonical representation capture and
uses fit `[1,554)`, purge `[554,584)`, and readout validation `[584,730)`.

| Input arm | Margin direction | Margin rank | Correlation | Prediction/target std | NLL |
|---|---:|---:|---:|---:|---:|
| Native frozen representation | `0.501235` | `0.503480` | `-0.027769` | `0.000088` | `-2.463191` |
| Fit-only featurewise z-score | `0.525926` | `0.522042` | `0.096803` | `0.023072` | `-2.518800` |
| Previous-return control | `0.814815` | `0.740139` | `0.653617` | `0.998690` | n/a |
| Authored seasonal-lag oracle | `1.000000` | `1.000000` | `1.000000` | `1.000000` | n/a |

Both representation arms fail. The standardized arm ends slightly worse than
the unconditional per-node Gaussian validation NLL of `-2.524585`, and its
mean prediction collapses to about `2.3%` of target standard deviation.
Featurewise scaling is therefore not a sufficient repair. In contrast, the
raw-history K=1 arm using the identical batch schedule reached `1.0/1.0`
margin direction/rank, so this is not a generic MDN-capacity or synthetic-data
failure.

This experiment is a frozen-feature readout diagnostic, not a representation
generalization result. The representation checkpoint itself trained on all
anchors `[0,730)`, so `[584,730)` is held out only from the fresh K=1
optimizer. Overlapping upstream windows expose 145 of its 146 target rows as
later representation contexts. The aggregate capture count nevertheless
proves all `8,760 = 730 * 4 * 3` context-mask slots were valid; the helper's
all-true mask reconstruction is not an untested assumption.

Main and replay reports are byte-identical at
`8d1bc98a9ef766871781262eb63a87f2cb2c7c6d0eaec9906d7a31fcfa98de38`;
the master-manifest file is sealed at
`1bc73921d059c71c1a8c17a56fd3d73b580445ff3ecab750e6057235aa1e3994`.
The durable record is
`artifacts/synthetic_mdn_frozen_representation_k1_isolation_v1.report`;
reproduce or verify it with
`src/scripts/benchmarks/synthetic_continuous_graph_v1/run_mdn_frozen_representation_k1_isolation.sh`.

## Cached Direct-Head Input-Normalization A/B

The production direct head was next isolated at its exact cached post-adapter
400-coordinate input. The two arms have identical initial parameter bytes,
edge specialization, batches, Adam state, `100/5/5` objective, target scale
36, and 3,500-step schedule. The only arithmetic difference is whether the
registered per-sample `input_norm` forward call executes.

| Arm | Canary margin direction/rank | Validation margin direction/rank | Correlation |
|---|---:|---:|---:|
| Current sample LayerNorm | `0.758065 / 0.702899` | `0.506984 / 0.517401` | `-0.018226` |
| Exact LayerNorm bypass | `0.741935 / 0.702899` | `0.480690 / 0.470998` | `-0.000991` |

Neither arm passes the predeclared `0.95/0.95` 16-anchor capacity canary. The
bypass also fails to improve the full validation result, so the report's
causal-evidence gate is false and it makes no claim that removing LayerNorm
repairs the trainable head. This does not erase the separate closed-form
finding that sample LayerNorm loses substantial linear signal; it proves that
bypassing that operation alone is insufficient because the unconditioned
head/optimizer/objective path still collapses.

Main and replay reports are byte-identical at
`70b9e5bbf51645642239fde69cd4b7c24e53ae06b1e04bc4d44309b68122c670`;
the master-manifest file is sealed at
`6ab981c63852e01100ee3ac3b0995d109278ed7f6b0e0c948784faf7a51d1760`.
The durable record is
`artifacts/synthetic_mdn_cached_feature_input_norm_ablation_v1.report`;
reproduce or verify it with
`src/scripts/benchmarks/synthetic_continuous_graph_v1/run_mdn_cached_feature_input_norm_ablation.sh`.

## Frozen-Representation K=1 Objective A/B

The final isolation holds the fit-zscored representation, K=1 model,
initialization, batches, optimizer, and schedule fixed. Arm A is byte-exact to
one canonical `ChannelContextMdnNllTrainer` step. Arm B optimizes the same K=1
mixture mean directly in edge-return space with production target scale 36
and Smooth-L1 beta `0.5`; its logit/sigma slices and the separate direct head
remain unchanged.

| Decoder/objective | Canary margin direction/rank | Validation margin direction/rank | Correlation |
|---|---:|---:|---:|
| Trainable-sigma node NLL | `0.568182 / 0.744681` | `0.525926 / 0.522042` | `0.096803` |
| Target-scaled edge Huber | `1.000000 / 0.978723` | `0.651852 / 0.545244` | `0.305033` |
| Fit-only standardized per-edge linear control | n/a | `0.908642 / 0.907193` | `0.911848` |

Direct edge supervision causally rescues mechanical fitting: its canary reaches
near-perfect signal recovery while NLL does not. On the full development
slice it adds `0.125926` margin-direction accuracy but only `0.023202` rank,
and it remains far below the exact linear control. The report therefore calls
this a mechanical-fit rescue without a material validation rescue. Arm B
changes both loss family and target geometry, so this result implicates the
combined trainable-density/node-gauge/edge-objective alignment bundle; it does
not isolate sigma alone.

The linear control is the strongest current boundary. The frozen
representation contains much more deterministic edge signal than either MDN
objective extracts, but even that transductive development decoder remains
below the `0.95/0.95` oracle gate. The diagnosis is therefore layered rather
than binary: readout objective and optimization are the immediate failure,
while upstream representation quality still sets a lower ceiling than the
authored chart permits.

Main and replay reports are byte-identical at
`c687a00d3b3e2f17b79f4127d1d5521fa88f25e4bb0ca361dd54478a463988a8`;
the master-manifest file is sealed at
`27ca3446f60a8155be28080d66e9690ab6e34dbfc7965591c2657505c366daaa`.
The durable record is
`artifacts/synthetic_mdn_frozen_representation_k1_objective_ab_v1.report`;
reproduce or verify it with
`src/scripts/benchmarks/synthetic_continuous_graph_v1/run_mdn_frozen_representation_k1_objective_ab.sh`.

## Production-Loader Conditioned Direct-Feature Bridge

The conditioned v2 per-edge readout has now crossed the real production MDN
boundary. The sealed helper reconstructs the captured representation tensor as
`[730,4,3,32]`, creates the exact production `ChannelContextMdn`, and loads the
real checkpoint through
`channel_graph_first_inference_launcher_detail::load_channel_mdn_checkpoint_file`
with the complete non-null checkpoint identity. It then runs the backbone,
channel adapters, and direct readout-feature constructor in the original
64-anchor CUDA batches and applies the archived conditioned head.

| Checked surface | Maximum absolute delta | Exact tensor equality |
|---|---:|---:|
| Recomputed versus cached `[730,3,3,400]` direct features | `0.0` | true |
| Recomputed versus cached conditioned predictions | `0.0` | true |
| Recomputed validation metrics versus the sealed shadow report | `0.0` | n/a |
| Recomputed validation production objective versus the sealed shadow report | `0.0` | n/a |

The live recomputed validation result is direction `0.805936`, rank `0.793760`,
correlation `0.687225`, RMSE `0.02083614`, and production direct-edge objective
`42.651226`. Primary and replay reports are byte-identical at
`b1468a46a054f7c0099fea42da119f5bfda04014c90f4b8148e9d2c934c9a876`;
the master-manifest file is sealed at
`bca6b33872bd64bac1319ab208c8789b24b5442e9032b7e5205e5e398350e119`.

This removes the cache/export boundary as an explanation: the signal recovered
by the conditioned affine head exists exactly at the real checkpoint's direct
feature seam. The immediate integrated failure is therefore in the learned
readout/training path, not an accidental probe reconstruction or deployment
arithmetic mismatch. It still does not prove unseen-future generalization.
The upstream representation and MDN both trained on development `[0,730)`, and
this bridge deliberately did not run representation forward, the MDN
distribution head, policy, or the end-to-end forecasting launcher. Historical
confirmation and final holdout `[1088,1170)` remain unopened.

The durable record is
`artifacts/synthetic_mdn_frozen_conditioned_affine_mdn_forward_bridge_v1.report`;
reproduce or verify it with
`src/scripts/benchmarks/synthetic_continuous_graph_v1/run_mdn_frozen_conditioned_affine_mdn_forward_bridge.sh`.

## No-Refit Historical Conditioned-Head Confirmation

The conditioned v2 head also passes the predeclared no-refit confirmation on
historical anchors `[760,1088)`. The frozen representation and MDN checkpoints
were trained only on development `[0,730)`, and the conditioned artifact was
fit on `[0,554)`, purged on `[554,584)`, and selected on `[584,730)`. The
confirmation loads the exact production MDN checkpoint head and the exact
conditioned artifact against the same captured `[328,3,3,400]` feature surface;
it performs zero optimizer steps and writes no checkpoint.

| Readout on `[760,1088)` | Direction | Rank | Correlation | RMSE | Production objective |
|---|---:|---:|---:|---:|---:|
| Frozen checkpoint direct head | `0.481030` | `0.508130` | `-0.007598` | `0.028335` | `67.259834` |
| Frozen conditioned v2 head | `0.802507` | `0.780488` | `0.729269` | `0.019188` | `39.528431` |
| Conditioned improvement | `+0.321477` | `+0.272358` | n/a | `0.677177x` | n/a |

All seven gates declared before this evaluation pass: direction and rank at
least `0.65`, correlation at least `0.50`, RMSE at most `0.025`, direction and
rank improvements of at least `0.10`, and RMSE at most `0.90x` the checkpoint
head. This is the cleanest localization so far. A fixed readout fit entirely on
development transfers strong signal through the frozen representation and MDN
direct-feature surface, while the learned checkpoint head remains random. The
immediate production failure is therefore the learned direct-readout/training
path, not a cache boundary and not an absence of generalizing information in
the frozen feature surface.

This is an honest model-unseen and no-refit test, but not a pristine project
holdout: `[760,1088)` had already been inspected by earlier diagnostic
experiments. It does not validate the MDN distribution output, policy, or the
complete forecast-to-action path. The only computationally unconsumed one-shot
interval remains `[1088,1170)`, and it was not opened by this evaluation. The
HTML chart may already have made that segment human-visible, so it must not be
described as a human-blind holdout.

Primary and replay reports are byte-identical at
`134487c2c60285377abd0a14771204ddbb59a9d5ba5a8dc51129679405a5e411`;
the 378-file master-manifest file is sealed at
`237b3dcb4b599c835d78eeb7fc3989e4df71c3b19b35ffbc3ef772d98c2748e0`.
The durable record is
`artifacts/synthetic_mdn_frozen_conditioned_affine_historical_confirmation_v1.report`;
reproduce or verify it with
`src/scripts/benchmarks/synthetic_continuous_graph_v1/run_mdn_frozen_conditioned_affine_historical_confirmation.sh`.

## Real-Launcher Conditioned-Shadow Non-Mutation A/B

The opt-in conditioned v2 shadow has now run through the complete production
Channel Graph-First representation and MDN launcher on validation
`[584,730)`. Control and treatment each loaded the same frozen representation
and MDN checkpoints, processed exactly `146` anchors in `64+64+18`, performed
zero optimizer steps, wrote no checkpoint or feature probe, and did not enter
replay or policy.

The shadow is non-mutating at the actual launcher boundary. Across all three
batches, canonical `log_pi`, `mu`, `sigma`, and `direct_edge_return` are both
`torch::equal` and raw-byte equal with maximum delta zero. The returned
canonical launcher reports are also byte-identical. The live conditioned
readout remains strong: direction `0.806697`, rank `0.792237`, correlation
`0.686404`, and RMSE `0.0208663`, with all `1,314` targets and rank pairs
present. It clears the four preregistered absolute quality gates.

The initial smoke run deliberately failed its stronger demand for exact metric
equality with the older cached `[0,730)` capture. A direct probe comparison
localized the discrepancy. Targets and the 16-coordinate identity suffix are
exact; only anchors `704..711` change in the 384-coordinate dynamic prefix when
validation is launched alone. Of `525,600` feature values, `14,877` differ,
with maximum delta `1.90735e-6`, mean absolute delta `2.14866e-9`, and delta
RMSE `2.61192e-8`. The old full-development capture used a final 26-anchor
batch beginning at 704, while the validation-only launcher places 704..711 in
a full 64-anchor batch. The highly conditioned affine mapping amplifies this
small batch-shape floating-point difference into one directional decision and
two rank decisions out of 1,314. The checkpoint head's canonical outputs are
unchanged by the shadow.

The sealed fact records this honestly as `exact=false` plus count-aware
operational equivalence: continuous cached-metric drift is at most
`8.20722e-4`, directional disagreement is one decision, and rank disagreement
is two decisions. No candidate, artifact, fit, or scientific gate was changed.
The validation-only batch-partition probe is sealed by content at
`07bcc23d8848f1abebcb725151a08ee724127ab3aeab02ed2046aa7ff1c0e755`.

Primary and replay facts are byte-identical at
`efee030e84a6868afef579086c88355951283b69023d55096a195f31f8fbf173`;
the canonical report is sealed at
`856cd8bd354ec65120aecb9991885a13e2faf1df1eca6e1e4f2eae2fbd0210bd`,
the live shadow report at
`4689d70470682434d76a23d536ae8d11020ff03b55266a8180775d0a285c1f10`,
and the master manifest at
`bf663d19ad76a8f403ee3ac982624819d51c758d800dfeb17a478bfebdb96bdd`.
The durable record is
`artifacts/synthetic_mdn_channel_graph_first_conditioned_affine_shadow_eval_v1.report`;
reproduce or verify it with
`src/scripts/benchmarks/synthetic_continuous_graph_v1/run_channel_graph_first_conditioned_affine_shadow_eval.sh`.
The maximum opened anchor remains 1087; final `[1088,1170)` was not read.

## Current Failure Model

The accumulated evidence no longer supports a single-cause explanation:

1. The raw charts are deterministic and forecastable; raw-history K=1 reaches
   the oracle gates.
2. The frozen representation/MDN direct-feature surface is not random or
   collapsed. A development-fitted conditioned head transfers to model-unseen
   history at `0.8025/0.7805` direction/rank. This still leaves a real upstream
   quality gap to the authored chart's `0.95/0.95` oracle.
3. Trainable-sigma node NLL is badly aligned with the desired edge forecast.
   Edge Huber fixes tiny-set fitting and some direction accuracy, not rank.
4. The learned checkpoint readout fails to recover signal available to the
   conditioned affine solution: on the same historical feature tensor it is
   `0.4810/0.5081` while the frozen conditioned head is `0.8025/0.7805`.
   Full-rank conditioning recovers the affine optimum and transfers it; the
   ordinary training coordinates do not. The real-launcher shadow proves this
   rescue can be attached without changing a byte of canonical MDN output.
5. Per-sample LayerNorm loses useful closed-form signal, but its isolated
   bypass is not a repair. Conditioning must be changed as a complete contract,
   including stored fit statistics and deployment arithmetic.
6. The production 800-step direct-head-only warmup trains a head above a frozen
   random trunk, and the all-nine-target NLL is dominated by several activity
   coordinates. These remain high-confidence aggravating mechanisms requiring
   exact production-path A/Bs, not explanations assumed from configuration.

## Next Investigation

Useful questions to answer next:

```text
1. Decompose the objective bundle with fixed-sigma node MSE, raw edge MSE, and
   target-scaled edge Huber under identical initialization and coordinates.
   This separates sigma optimization, node-gauge geometry, and loss saturation.
2. Add a causal raw-close-history residual/bypass beside the learned
   representation. The raw K=1 oracle proves this information is sufficient;
   the bypass tests whether a representation bottleneck can be made nonfatal.
3. Replace the direct-head-only frozen-random-trunk warmup with a same-seed A/B
   that either updates the trunk/adapters or prefits the conditioned readout.
4. Run close-only before all-nine-target training, then add targets one group at
   a time with fit-derived normalization so activity coordinates cannot dominate
   price/return gradients.
5. Build and review the already preregistered one-shot `[1088,1170)` executable
   closure before opening it. Preserve the exact representation, MDN, artifact,
   no-refit rule, seven gates, checkpoint control, and publish-on-failure rule.
6. If the conditioned edge path still plateaus below `0.95/0.95`, change the
   representation input itself: preserve phase/raw temporal residuals and
   normalize price versus activity scales before MTF tokenization.
7. Only after the forecast path passes should PPO invalid-action and snapshot
   closure issues return to the critical path.
```
