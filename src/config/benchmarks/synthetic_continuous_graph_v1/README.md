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

## Next Investigation

Useful questions to answer next:

```text
1. Why does MDN loss improvement fail to improve edge-return sign/rank despite
   frozen representation features containing partial edge signal?
2. Why does a dedicated MDN direct edge-return readout fail on the train range
   when the frozen-representation supervised probe is materially above random?
3. Are target scaling and target-feature weights causing close/edge information
   to be dominated by easier magnitude/activity coordinates?
4. Are node/channel identity and cross-node/cross-channel context too weak for
   this synthetic graph?
5. Would representation training recover oracle-grade phase if exposed to a
   stronger phase/lag diagnostic or a no-augmentation control?
6. Why does PPO produce many invalid actions despite positive final equity over
   the short validation replay?
7. Why is the policy-training fact's snapshot bundle/causal closure mismatched
   after an otherwise clean no-lookahead handoff?
```
