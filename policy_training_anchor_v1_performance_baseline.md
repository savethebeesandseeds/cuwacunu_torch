# policy_training_anchor_v1_performance_baseline.v2

Date: 2026-06-20

## Status

This note captures the current green anchor-v1 policy-training performance
profile after `increase_certified_replay_exposure.v2`.

The baseline is readiness-safe at the artifact proof layer:

- `policy_training_artifact_ready` is satisfied by direct Lattice evaluation on
  the v2 evidence root.
- `proof_certificate_check_passed=true`.
- `deficits=[]`.
- The proof certificate digest is `8f85723bd45b674f`.
- The selected policy-training fact digest is `517fb94fec140323`.

This is not a policy-acceptance, paper-online, deployment, market-readiness, or
live-trading claim. Runtime explicitly records:

- `policy_quality_claimed=false`
- `market_readiness_claimed=false`
- `deployment_readiness_claimed=false`
- `live_execution_allowed=false`

## Commands

The v2 proof used a narrow evidence root assembled from the matching upstream
train jobs, forecast/replay job, and policy job:

```sh
.build/hero/hero_lattice.mcp \
  --global-config /cuwacunu/src/config/.config \
  --tool hero.lattice.evaluate.target \
  --args-json '{"target_id":"policy_training_artifact_ready","runtime_root":"/tmp/hero_v2_evidence_root"}'
```

Runtime policy dry-run derives the activation contract from the active wave and
Runtime replay state:

```sh
.build/hero/hero_runtime.mcp \
  --global-config /cuwacunu/src/config/.config \
  --tool hero.runtime.run \
  --args-json '{"mode":"dry_run"}'
```

The current Runtime dry-run contract digest is `39b0fc42e14278d6`. The
historical policy-training job below still records the earlier executed
contract digest `fd91f7620c46632a`.

## Evidence Paths

Policy-training job:

```text
.runtime/cuwacunu_exec/components/wikimyei.policy.portfolio.graph_node_allocation/spawns/wikimyei.policy.portfolio.graph_node_allocation.ppo_v0_4b5ba29b8ad254d7/jobs/train/policy_training_ppo_v0_fd91f7620c46632a.attempt_fd91f7620c46632a_1781988984173151_44435
```

Source forecast/replay job:

```text
.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/lch_902062add6d314cd/jobs/run/cwu_02v_channel_validation_eval_mdn_1200_2247.run.channel_inference_mdn.attempt_000000
```

Primary performance profile:

```text
.runtime/cuwacunu_exec/components/wikimyei.policy.portfolio.graph_node_allocation/spawns/wikimyei.policy.portfolio.graph_node_allocation.ppo_v0_4b5ba29b8ad254d7/jobs/train/policy_training_ppo_v0_fd91f7620c46632a.attempt_fd91f7620c46632a_1781988984173151_44435/artifacts/wikimyei.policy.portfolio.graph_node_allocation/reports/policy_training_anchor_v1_performance_profile.report
```

## Exposure Profile

Proof markers from the selected policy-training fact:

| Proof field | Value |
| --- | --- |
| No-lookahead schema | `no_lookahead_artifact_provenance.v1` |
| No-lookahead influence end | `1170` |
| Label/reward availability end | `1170` |
| Embargo policy | `embargo_policy_anchor_v1` |
| Embargo window | `[1200,2247)` |
| Provenance closure digest | `4732ecc99540c1ac` |
| Snapshot bundle id | `bundle_policy_training_anchor_v1_1200_2247_policy_generation_50be38004d0880cc` |
| Snapshot bundle generation vector | `1bb080abb21193ce` |
| Snapshot bundle valid from | `2247` |
| Snapshot bundle compatibility closure | `86b4bcc4554bc1c2` |
| Policy actor checkpoint | `50be38004d0880cc333c72a7d18a21f4705bc2baa7bf46b7dd9f834227e468eb` |

| Component/artifact | Intent | Fit/influence | Coverage/use | Resulting availability |
| --- | --- | --- | --- | --- |
| Representation checkpoint `80e3b2da522b56a6` | Prior upstream representation | fit `[0,1170)`, influence end `1170` | consumed by forecast/replay for `[1200,2247)` | admissible at policy target begin |
| MDN checkpoint `d245555d1c3d373e` | Prior upstream forecast model | fit `[0,1170)`, influence end `1170` | consumed by forecast/replay for `[1200,2247)` | admissible at policy target begin |
| Forecast eval fact `b69480bb2d007dbe` | Forecast inputs for replay | influence end `1170` | coverage `[1200,2247)` | no-lookahead clean |
| Replay environment fact `9aa33797035e4565` | Replay lineage boundary | inherited influence end `1170` | coverage `[1200,2247)` | no-lookahead clean |
| Certified replay bank `412f9341e7b6e577fc1873e064b4a49a56ff0d0af1899eaa72fcacb0b17d817c` | Policy experience source | inherited influence end `1170` | 17 chunks, 1047 samples, coverage `[1200,2247)` | no-lookahead clean |
| Policy training output `50be38004d0880cc...` | PPO policy update | fit `[1200,2247)` | trained from certified replay | valid from anchor `2247` |

The key rule is preserved: the policy can train on `[1200,2247)` because its
forecast/replay inputs were generated from upstream influence ending at `1170`.
The `[1170,1200)` gap is the current anchor-v1 context/purge gap. The policy
output itself includes `[1200,2247)` and is therefore only valid from `2247`
onward.

## Training Profile

| Metric | Value |
| --- | ---: |
| Runtime status | `completed` |
| Trainer | `ppo_policy_adapter.v1` |
| Device | `cuda` |
| CUDA verification | `true` |
| Resume mode | `fresh_spawn` |
| Sample count | `1047` |
| Certified replay chunks | `17` |
| Certified replay utilization | `1.0` |
| Optimizer steps | `6` |
| Policy loss final | `0.0052719` |
| Value loss | `0.00499815` |
| Approx KL | `0.0141982` |
| KL target ratio | `0.473273` |
| Clip fraction | `0.0706781` |
| Entropy | `-3.15632` |
| Parameter delta L2 | `0.0296424` |

The PPO update is not obviously unstable: KL is below target and clip fraction
is not high. The certified replay source-capacity warning is cleared for the
1024-sample diagnostic threshold.

## Replay And Evaluation Profile

| Metric | Value |
| --- | ---: |
| Source replay bundles | `17` |
| Validation sample count | `1047` |
| Validation final equity numeraire | `10411` |
| Validation total log growth | `0.0228491` |
| Validation max drawdown | `0.13409` |
| Validation turnover | `95.0718` |
| Validation transaction cost numeraire | `486.914` |
| Validation invalid action count | `0` |
| Validation invalid trace count | `0` |
| Validation reward total | `0.340899` |

Policy quality comparison is present but remains diagnostic:

| Comparison | Value |
| --- | ---: |
| PPO mean total log growth | `0.0228491` |
| PPO mean final equity numeraire | `10411` |
| Equal-weight mean total log growth | `0.0254198` |
| Equal-weight mean final equity numeraire | `10434.3` |
| SDU mean total log growth | `-0.00118081` |
| SDU mean final equity numeraire | `9988.62` |
| Numeraire-only final equity | `10000` |

## Performance Interpretation

The current green path is clean enough to continue performance testing, but not
strong enough to claim policy quality. The current performance tag is now:

```text
policy_quality_comparison
```

The profile marks:

- `sample_count_low=false`
- `certified_replay_sample_count_low=false`
- `certified_replay_sample_deficit_to_threshold=0`
- `certified_replay_source_capacity_limited=false`
- `certified_replay_scaling_bottleneck_kind=none`
- `certified_replay_scaling_recommendation=compare_policy_quality`
- `profile_warning_count=0`

`increase_certified_replay_exposure.v2` expanded the clean replay window from
647 samples over `[1600,2247)` to 1047 samples over `[1200,2247)`. The next
useful performance work is no longer first-order replay-capacity expansion. It
is comparing policy quality against baselines and tuning PPO behavior under the
now-larger certified replay profile.

## Notes

- Lattice proof authority is direct `hero_lattice.mcp`.
- The target remains artifact readiness only. It is intentionally
  non-dispatchable and does not select a model or decide deployment.
- Anchor-v1 proves accepted-anchor influence admissibility. It is not the final
  word on label/reward availability, embargo correctness, or future
  percent/range-stable split authoring.
- The v2 run used the durable policy activation contract. The dry-run/execute
  contract digest path still needs a small canonicalization cleanup before the
  user guide should require passing an explicit dry-run digest into execute.
