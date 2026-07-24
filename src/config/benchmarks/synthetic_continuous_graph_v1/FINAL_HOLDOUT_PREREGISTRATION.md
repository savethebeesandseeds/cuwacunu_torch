# Synthetic Conditioned-Head Final Holdout Preregistration

Status: **frozen scientific and executable protocol; ready but not executed**

This is the immutable pre-execution status. After the one-shot run, execution
status and results belong in the sealed runtime/installed reports; this
preregistration must not be edited around the observed final score.

Schema reserved for the eventual result:
`synthetic_mdn_frozen_conditioned_affine_final_holdout_v1`.

The final interval `[1088,1170)` is computationally unconsumed as of this
preregistration. It may already have been human-visible in the HTML chart, so
it is not described as human-blind. Reading any source anchor at or above 1088
for capture or metric evaluation consumes the interval even if execution later
fails.

This document freezes the candidate, scientific gates, controls, and
interpretation rules. The eventual executable closure must add exact helper,
runner, public-header-tree, executable, and transitive-config hashes without
changing any scientific term below.

## Candidate and Controls

The sole candidate is the run-only conditioned v2 artifact:

```text
path=.runtime/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_frozen_affine_deployment_bridge_v1/main/synthetic_mdn_frozen_affine_deployment_bridge_v1.pt
sha256=7c2e2008a9b8560c6db630da9e68a061b3a038eba26def6237dcc89a77731739
semantic_tensor_digest=84c92fb959c0b6b2f6e27fbfe8744f51a0d4e42ae0ad6172571e176308af0c38
artifact_family=wikimyei.inference.expected_value.mdn.per_edge_conditioned_affine_return_head.v2
conditioning_alpha=1e-10
run_only=true
policy_eligible=false
feature_contract=consume_[0,384)_of_live_[B,3,3,400]
fit=[0,554)
purge=[554,584)
selection=[584,730)
maximum_fit_or_selection_anchor=729
```

The control is the learned direct head from the same frozen production MDN
checkpoint. No other candidate, alpha, normalization, feature prefix, or
checkpoint may be evaluated on the final interval.

```text
representation_checkpoint_sha256=8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de
mdn_checkpoint_sha256=eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e
graph_order_fingerprint=d334e38b1887ae16
node_ids=SYNUSD,SYNALPHA,SYNBETA,SYNGAMMA
edge_ids=SYNALPHASYNUSD,SYNBETASYNUSD,SYNGAMMASYNUSD
channel_count=3
close_feature_coord=3
```

Qualification evidence is frozen at:

```text
historical_confirmation_report_sha256=134487c2c60285377abd0a14771204ddbb59a9d5ba5a8dc51129679405a5e411
historical_confirmation_master_sha256=237b3dcb4b599c835d78eeb7fc3989e4df71c3b19b35ffbc3ef772d98c2748e0
```

Historical `[760,1088)` was repeatedly inspected during development and is
adaptive validation evidence, not an independent test.

The required real-launcher validation A/B is complete. The production
checkpoint outputs and canonical report were byte-identical with the
conditioned shadow disabled versus enabled, while the live conditioned shadow
passed its preregistered absolute quality gates. Its comparison with the older
sealed metric report was operationally equivalent rather than exact because a
different `64+64+18` batch partition changed a few decisions at floating-point
boundaries; this is recorded explicitly rather than hidden.

```text
launcher_shadow_ab_fact_sha256=efee030e84a6868afef579086c88355951283b69023d55096a195f31f8fbf173
launcher_shadow_runtime_master_sha256=bf663d19ad76a8f403ee3ac982624819d51c758d800dfeb17a478bfebdb96bdd
launcher_shadow_canonical_report_sha256=856cd8bd354ec65120aecb9991885a13e2faf1df1eca6e1e4f2eae2fbd0210bd
launcher_shadow_conditioned_report_sha256=4689d70470682434d76a23d536ae8d11020ff03b55266a8180775d0a285c1f10
launcher_shadow_canonical_outputs_byte_exact=true
launcher_shadow_cached_metric_report_exact=false
launcher_shadow_cached_metric_operational_equivalence_pass=true
launcher_shadow_absolute_quality_gates_pass=true
launcher_shadow_maximum_metric_delta=0.00152207001522075558
```

## Frozen Source Identity

```text
anchor_range=[1088,1170)
anchor_count=82
expected_rows=738
expected_feature_shape=[82,3,3,400]
expected_batches=64,18
source_cursor_token=version=ujcamei.graph_anchor_cursor_report.v1|graph=d334e38b1887ae16|reference_edge=SYNALPHASYNUSD|Hx=30|Hf=1|edges=3|accepted=1170|candidates=1170|skipped=0|first=1580428799999|last=1681430399999
base_config_sha256=7c84cee94ecf839336c0383878298981b9ab362e80a570cefef20e9fed272fd6
capture_runner_sha256=115042fce12b569f3bac5429949898e65aae755e3673527b67a269d1a27dd82b
objective_helper_sha256=157b5f71f905cef670d0034b0ca04795bb8ac1561148493c58cb37a63f637289
```

Raw CSV value bytes are pinned independently of the cursor token:

```text
8d266d72ba9b2a419a82078d60a1f3a3d5984d3bbe444ce377246aafe68fbb96  data/raw/SYNALPHASYNUSD/1d/SYNALPHASYNUSD-1d-all-years.csv
d2cf91591bb554a77e6e9c3af4e50a587793a4f24f599d26c93b3ba7b0a9f020  data/raw/SYNALPHASYNUSD/1w/SYNALPHASYNUSD-1w-all-years.csv
2691ad279d841229eb5980918d7837d6e0c24bb9067781bff2e4da428f9d13ad  data/raw/SYNALPHASYNUSD/3d/SYNALPHASYNUSD-3d-all-years.csv
5d0a0c5581810cfaba7e783731398c0d80dbaa13672bf4868cb2519ca501055b  data/raw/SYNBETASYNUSD/1d/SYNBETASYNUSD-1d-all-years.csv
8fbab5da910748df8eaec534e1877a2295a43e3f4415d08ba97bd4986cf6b9b2  data/raw/SYNBETASYNUSD/1w/SYNBETASYNUSD-1w-all-years.csv
6e4e691cc95d3572d2d8ff59e642acaeaf6992b3c73cf243ba85d8f8496d3d4b  data/raw/SYNBETASYNUSD/3d/SYNBETASYNUSD-3d-all-years.csv
81f7f7d5ef074360faf8514b0df5c204a3698f7cfaf0abf2a071a4526ca8f698  data/raw/SYNGAMMASYNUSD/1d/SYNGAMMASYNUSD-1d-all-years.csv
2ed806271d1a88a56e3ded4051b81ea9f385db0ae46b4c215137af98f50f0872  data/raw/SYNGAMMASYNUSD/1w/SYNGAMMASYNUSD-1w-all-years.csv
267a29cfba20027d219a61968d0439f08c66c1284cc6d00a56f0d9d224d6a6f0  data/raw/SYNGAMMASYNUSD/3d/SYNGAMMASYNUSD-3d-all-years.csv
```

## Frozen Executable Closure

The build/hash-only preflight completed without creating a consumption ledger,
capture vault, final runtime package, or installed final report. These are the
exact seals that `--run` must reproduce before it can create the irreversible
ledger or read anchor 1088:

```text
final_helper_sha256=1bd51f5f5fece2f75297b9a1c36d3365974096e9af4c9c3e801d28e76a8b93f0
final_runner_sha256=dc05760a62d04ed28e44a5e0b9ab02e65e108e999c266999e323c43fd0c9eea0
runtime_executable_sha256=c3bfb3df0ebfe23a0b2859bef2a6d4a4f607c60974dc2a96abe57a1927f92a48
public_header_tree_manifest_sha256=343fc6d4cd16a0e6ce075e3ac42ad2c52111f6ccccc5e858f2415452cf96606e
transitive_config_tree_manifest_sha256=af3eb77d88ef1debdf88e8b85c38a68185212b4a8d6b3bb8474fa48b1a73d840
makefile_tree_manifest_sha256=f35288c830ccb1b2165a2227f71daeefe373946c4bd7924fa1a6f98aab36a961
final_helper_binary_sha256=1c7f231aac31be3093cfc0cc6fe559067fd33e9743fd5b72183b05bb6c2b441a
runtime_shared_libraries_manifest_sha256=e599517ff314dde4ff09d6c60dbf98474bc7bf15d557ea242ee24dc933df2e51
helper_shared_libraries_manifest_sha256=e599517ff314dde4ff09d6c60dbf98474bc7bf15d557ea242ee24dc933df2e51
```

## Required Execution Contract

Before unlocking the interval:

1. Complete the real-launcher conditioned-shadow A/B on already-consumed
   validation `[584,730)` and prove canonical `log_pi`, `mu`, `sigma`, direct
   return, and canonical report equality with the sidecar absent versus present.
2. Build and review the final helper and no-clobber runner without reading a
   holdout row. Freeze the entire executable/config/header/input closure and
   make it read-only.
3. Acquire a unique canonical lock and prove no final capture, result, or
   consumption ledger already exists.

The sealed runner must then:

1. Create the no-clobber consumption ledger immediately before permitting the
   production launcher to read anchor 1088.
2. Capture `[1088,1170)` exactly once into private scratch using the production
   representation and MDN forward path. Do not print or inspect its data or
   metrics during capture.
3. Freeze the resulting 738-row feature probe and evaluate the checkpoint
   control and sole conditioned candidate twice from that same immutable probe.
   Replay must not recapture the source.
4. Publish the result whether the scientific gates pass or fail. A failed gate
   is a valid result, not an infrastructure error and not permission to tune.

Hard validity gates:

- anchor range exactly `[1088,1170)`, maximum anchor 1169;
- 82 anchors, two batches `64+18`, zero skipped batches;
- exactly 738 valid targets and 738 rank pairs;
- per-edge and per-channel valid counts exactly 246;
- reconstructed dynamic prefix `[0,384)` bit-exact with maximum delta zero;
- full-input and suffix-perturbed conditioned predictions bit-identical;
- production checkpoint loader called with the complete non-null identity;
- both heads in `eval`, `NoGrad`, CUDA with float32 live input;
- `training=false`, `refit=false`, `optimizer_steps=0`;
- `checkpoint_written=false`, `policy_path_used=false`;
- input hashes identical before and after execution;
- all predictions and required metrics finite;
- primary and replay reports byte-identical with empty success logs;
- frozen closure and published output manifests verify.

A validity-gate failure permits no scientific conclusion. If any holdout row was
read, the interval remains consumed.

## Predeclared Scientific Gates

These are the same seven gates declared before historical confirmation. They
must not be tightened around the historical score.

| Gate | Threshold |
|---|---:|
| Conditioned directional accuracy | `>= 0.65` |
| Conditioned pairwise rank accuracy | `>= 0.65` |
| Conditioned correlation | `>= 0.50` |
| Conditioned RMSE | `<= 0.025` |
| Direction gain over checkpoint head | `>= +0.10` |
| Rank gain over checkpoint head | `>= +0.10` |
| Conditioned/checkpoint RMSE ratio | `<= 0.90` |

MAE, signed error, prediction/target standard-deviation ratio, best-asset
agreement, margin metrics, per-edge/per-channel direction, and the production
`100/5/5` direct objective are descriptive only. The 738 edge/channel rows
share only 82 serial anchors and must not be treated as independent samples.

## Permitted Conclusions

| Outcome | Permitted conclusion |
|---|---|
| All validity and seven scientific gates pass | The exact fixed conditioned head generalizes through the frozen production direct-feature surface on the final segment and materially beats the learned checkpoint head. This strongly confirms the immediate failure is learned readout/training. |
| Absolute gates pass; improvement gates fail | The candidate forecasts adequately, but the final segment does not independently localize failure because the checkpoint control also performed well. |
| Improvement gates pass; absolute gates fail | The checkpoint head is inferior, but the conditioned feature path is still inadequate under final temporal shift. |
| Direction/rank pass; RMSE/correlation fail | Sign/order transfers but calibration or magnitude does not; no full acceptance. |
| RMSE/correlation pass; direction/rank fail | Magnitudes co-move but decision-relevant sign/order fails; no full acceptance. |
| Candidate equals or loses to checkpoint | The claimed robust conditioned-head advantage is falsified on the final segment. |
| Both absolute and relative gates fail | Historical success was insufficiently general or adaptively overfit. |
| Provenance, coverage, or replay fails | No scientific conclusion; the interval is still consumed if any row was read. |

Even a complete pass does not validate the MDN distribution output, policy,
the forecast-to-action path, other synthetic seeds, or real-market data.

After any holdout execution, the candidate and interval may never be reused for
tuning. Further development requires a newly generated process/seed with a
holdout sealed before model work begins.
