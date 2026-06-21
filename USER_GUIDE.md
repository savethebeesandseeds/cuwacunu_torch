# Cuwacunu User Guide

This guide describes the clean Hero operator path for the current repository
state. Use the checked-in config files under `src/config/`; do not create
temporary config copies for real training.

## Ground Rules

- `src/config/.config` is the only checked-in `.config` file.
- Train-core long training policies are the canonical root `.jkimyei` files in
  `src/config/`; do not keep separate operator copies.
- Marshal is the preferred operator surface for target preparation, Runtime
  handoff materialization, and replay rollout. PPO policy-training execution
  uses the durable Runtime contract path because `policy_training_artifact_ready`
  is a non-dispatchable Lattice artifact-readiness target.
- Runtime executes accepted work and writes evidence. Lattice proves target
  satisfaction. Environment writes rollout and replay evidence.
- Live trading and live capital remain disabled. Training, dry-run execution,
  replay execution, and paper/replay evaluation are allowed.

All examples use the canonical config explicitly:

```sh
GLOBAL_CONFIG=/cuwacunu/src/config/.config
```

## Anchor-V1 Closeout Before Performance Work

Before performance testing or tuning, verify the current protection state. The
latest PPO execution is inspectable performance evidence, not readiness-grade
proof. Lattice must stay the proof authority and Marshal must not dispatch
artifact-readiness proofs as Runtime waves.

Check the policy-training artifact proof:

```sh
.build/hero/hero_lattice.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.lattice.evaluate.target \
  --args-json '{"target_id":"policy_training_artifact_ready"}'
```

Current anchor-v1 closeout state is green for the artifact proof:

- `target_status=satisfied`
- `proof_certificate_check_passed=true`
- `no_lookahead_provenance_checked=true`
- `no_lookahead_provenance_complete=true`
- `no_lookahead_provenance_admissible=true`
- snapshot-bundle publishability is complete/admissible for the selected
  policy-training fact
- target deficits are empty

Then ask Marshal to prepare the same target:

```sh
.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.prepare.train \
  --args-json '{"mode":"dry_run","profile":"single_wave_operator","target_id":"policy_training_artifact_ready"}'
```

The expected response is not a Runtime handoff. It should report:

- `target_status=satisfied`
- `dispatch_state=blocked`
- `blocker_bucket=non_dispatchable_artifact_readiness`
- `next_safe_action=inspect`

That is correct. `policy_training_artifact_ready` is a Lattice
artifact-readiness proof, not a train command. Performance experiments can be
run from durable Runtime contracts, but they do not imply policy acceptance,
complete training, paper-online readiness, deployment readiness, or live
authority.

For the current clean chain, the dispatchable train/evaluate targets should
already be reached:

```sh
.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.prepare.train \
  --args-json '{"mode":"dry_run","profile":"single_wave_operator","target_id":"cwu_02v_representation_train_core_ready"}'

.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.prepare.train \
  --args-json '{"mode":"dry_run","profile":"single_wave_operator","target_id":"cwu_02v_mdn_train_core_ready"}'

.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.prepare.evaluate \
  --args-json '{"mode":"dry_run","profile":"single_wave_operator","target_id":"channel_mdn_validation_eval_ready"}'
```

Closeout-ready responses report `dispatch_state=already_satisfied` and
`driver_terminal_state=reached`. If one of these becomes unsatisfied after a
fresh runtime reset, follow the corresponding training section below and use
Marshal dry-run before execution.

Policy acceptance, paper-online readiness, deployment readiness, and live
capital are not implied by this closeout. They require their own Lattice facts
and Environment/Marshal sidecars.

## Status And Fresh Runtime

Inspect the active Hero policy/config state:

```sh
.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.status \
  --args-json '{}'

.build/hero/hero_runtime.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.runtime.status \
  --args-json '{}'

.build/hero/hero_environment.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.environment.status \
  --args-json '{}'
```

If a clean runtime is required, preview the reset first:

```sh
.build/hero/hero_runtime.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.runtime.reset \
  --args-json '{"mode":"plan","runtime_root":"/cuwacunu/.runtime/cuwacunu_exec","backup":false}'
```

Execute the reset only after the preview is acceptable:

```sh
.build/hero/hero_runtime.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.runtime.reset \
  --args-json '{"mode":"execute","runtime_root":"/cuwacunu/.runtime/cuwacunu_exec","backup":false}'
```

## Representation Train-Core

Set `src/config/.config` to the long-train profile and representation wave:

```ini
[HERO]
runtime_hero_profile = long_train_operator
runtime_wave_id = train_core_mtf_jepa_mae_vicreg
```

Ask Marshal for the representation plan and dry run:

```sh
.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.prepare.train \
  --args-json '{"mode":"plan","profile":"single_wave_operator","target_id":"cwu_02v_representation_train_core_ready"}'

.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.prepare.train \
  --args-json '{"mode":"dry_run","profile":"single_wave_operator","target_id":"cwu_02v_representation_train_core_ready"}'
```

When the dry run reaches the execution gate, execute through Marshal:

```sh
.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.prepare.train \
  --args-json '{"mode":"execute","profile":"single_wave_operator","target_id":"cwu_02v_representation_train_core_ready"}'
```

Verify the target before starting MDN:

```sh
.build/hero/hero_lattice.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.lattice.evaluate.deficit \
  --args-json '{"target_id":"cwu_02v_representation_train_core_ready"}'
```

Current clean-run evidence:

- job id:
  `train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001`
- certificate: `cert_5684ae4025d7`

## MDN Train-Core

After representation evidence is proven, change only `src/config/.config` to:

```ini
[HERO]
runtime_wave_id = train_core_channel_mdn
```

Run the same Marshal sequence for the MDN target:

```sh
.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.prepare.train \
  --args-json '{"mode":"plan","profile":"single_wave_operator","target_id":"cwu_02v_mdn_train_core_no_test_leakage"}'

.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.prepare.train \
  --args-json '{"mode":"dry_run","profile":"single_wave_operator","target_id":"cwu_02v_mdn_train_core_no_test_leakage"}'

.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.prepare.train \
  --args-json '{"mode":"execute","profile":"single_wave_operator","target_id":"cwu_02v_mdn_train_core_no_test_leakage"}'
```

Verify the target:

```sh
.build/hero/hero_lattice.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.lattice.evaluate.deficit \
  --args-json '{"target_id":"cwu_02v_mdn_train_core_no_test_leakage"}'
```

Current clean-run evidence:

- job id:
  `train_core_channel_mdn.train.channel_inference_mdn.attempt_000004`
- certificate: `cert_646a03c950b2`

## Forecast Evaluation

After the MDN train-core target is satisfied, select the validation/evaluation
wave:

```ini
[HERO]
runtime_wave_id = cwu_02v_channel_validation_eval_mdn_1800_2050
```

Prepare and execute the evaluation run through Marshal:

```sh
.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.prepare.evaluate \
  --args-json '{"mode":"plan","profile":"single_wave_operator","target_id":"channel_mdn_validation_eval_ready"}'

.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.prepare.evaluate \
  --args-json '{"mode":"dry_run","profile":"single_wave_operator","target_id":"channel_mdn_validation_eval_ready"}'

.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.prepare.evaluate \
  --args-json '{"mode":"execute","profile":"single_wave_operator","target_id":"channel_mdn_validation_eval_ready"}'
```

The current completed evaluation job is:

```text
/cuwacunu/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/bjb_6c38dff31d4c275d/jobs/run/cwu_02v_channel_validation_eval_mdn_1800_2050.run.channel_inference_mdn.attempt_000004
```

It loaded the trained representation and MDN checkpoints and wrote replay
artifacts for the Environment step.

## Environment Replay

Use Marshal to run the bounded replay rollout against the completed evaluation
job:

```sh
EVAL_JOB_DIR=/cuwacunu/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/bjb_6c38dff31d4c275d/jobs/run/cwu_02v_channel_validation_eval_mdn_1800_2050.run.channel_inference_mdn.attempt_000004

.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.rollout \
  --args-json '{"mode":"execute","runtime_job_dir":"/cuwacunu/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/bjb_6c38dff31d4c275d/jobs/run/cwu_02v_channel_validation_eval_mdn_1800_2050.run.channel_inference_mdn.attempt_000004","rollout_id":"full_training_environment_eval_v1","rollout_attempt_id":"attempt_000001","target_node_ids":["BTC","USDT","ETH"],"profile":"replay_validation_default","include_machine_payload":false}'
```

Environment writes the replay report and the allocation sidecar from replay
evidence. Verify both Lattice targets:

```sh
.build/hero/hero_lattice.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.lattice.evaluate.deficit \
  --args-json '{"target_id":"replay_environment_artifact_ready"}'

.build/hero/hero_lattice.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.lattice.evaluate.deficit \
  --args-json '{"target_id":"allocation_artifact_ready"}'
```

Current clean-run evidence:

- rollout id: `full_training_environment_eval_v1`
- replay certificate: `cert_78681b42d97c`
- replay fact digest: `dd07522c6fa7ed80`
- replay report digest: `e924e34ec5f44dce`
- replay report:
  `/cuwacunu/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/bjb_6c38dff31d4c275d/jobs/run/cwu_02v_channel_validation_eval_mdn_1800_2050.run.channel_inference_mdn.attempt_000004/artifacts/full_training_environment_eval_v1.replay.report`
- allocation fact digest: `221f2adb2d25bcef`
- mean total reward: `0.0126205`
- mean total log growth: `0.0136851`
- mean final equity numeraire: `1.01828`
- episode count: `16`

## PPO Policy Training

The current learned-policy path is the bounded PPO V0 Runtime wave.

Select the PPO policy wave:

```ini
[HERO]
runtime_wave_id = policy_training_ppo_v0
```

Dry-run the active policy wave. Runtime derives the sparse activation contract
from the selected wave, configured `.jkimyei`, split policy, and completed
compatible MDN replay artifacts under the Runtime root:

```sh
.build/hero/hero_runtime.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.runtime.run \
  --args-json '{"mode":"dry_run"}'
```

Execute still requires a materialized proof contract or handoff carrying the
policy-execution no-lookahead locks:

```sh
.build/hero/hero_runtime.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.runtime.run \
  --args-json '{"mode":"execute","contract_path":"<MATERIALIZED_POLICY_TRAINING_CONTRACT>","contract_digest":"<DRY_RUN_CONTRACT_DIGEST>"}'
```

Verify policy readiness. For the v2 run, the clean proof used a narrow evidence
root containing the representation train job, MDN train job, forecast/replay
job, and policy job from the same run family; broad full-root scans can be slow
when older runtime history is present.

```sh
rm -rf /tmp/hero_v2_evidence_root
mkdir -p /tmp/hero_v2_evidence_root

ln -s /cuwacunu/.runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/gzt_587be17eeef4e2f2/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001 \
  /tmp/hero_v2_evidence_root/representation_train_0_1170

ln -s /cuwacunu/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/lch_902062add6d314cd/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001 \
  /tmp/hero_v2_evidence_root/mdn_train_0_1170

ln -s /cuwacunu/.runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/lch_902062add6d314cd/jobs/run/cwu_02v_channel_validation_eval_mdn_1200_2247.run.channel_inference_mdn.attempt_000000 \
  /tmp/hero_v2_evidence_root/eval_1200_2247

ln -s /cuwacunu/.runtime/cuwacunu_exec/components/wikimyei.policy.portfolio.graph_node_allocation/spawns/wikimyei.policy.portfolio.graph_node_allocation.ppo_v0_4b5ba29b8ad254d7/jobs/train/policy_training_ppo_v0_fd91f7620c46632a.attempt_fd91f7620c46632a_1781988984173151_44435 \
  /tmp/hero_v2_evidence_root/policy_1200_2247

.build/hero/hero_lattice.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.lattice.evaluate.target \
  --args-json '{"target_id":"policy_training_artifact_ready","runtime_root":"/tmp/hero_v2_evidence_root"}'
```

Latest PPO execution evidence with anchor-v1 performance profile:

- job id:
  `policy_training_ppo_v0_fd91f7620c46632a.attempt_fd91f7620c46632a_1781988984173151_44435`
- Runtime contract digest:
  `fd91f7620c46632a`
- Lattice proof certificate digest:
  `8f85723bd45b674f`
- selected policy-training fact digest:
  `517fb94fec140323`
- actor checkpoint digest:
  `50be38004d0880cc333c72a7d18a21f4705bc2baa7bf46b7dd9f834227e468eb`
- critic checkpoint digest:
  `b3a20621ac63c379274a8d249aa03765bab87027dad43685a78b7aad8d51b61e`
- policy output generation id:
  `policy_generation_50be38004d0880cc`
- policy output generation vector digest:
  `6e94c6eda6b9e63254323f7561754dfbc63163b95e805e588f17ec84656252ae`
- certified replay bank digest:
  `412f9341e7b6e577fc1873e064b4a49a56ff0d0af1899eaa72fcacb0b17d817c`
- policy experience set digest:
  `36bd6289049d45819a45840d479117aa3910c3bb014627cc0e68e8fccd31b513`
- performance profile digest:
  `8fc2a48f953c49629a4d966c7f3f6dfb1f07882a922ecea739d819f4d5a7ada5`
- policy output fit range: `[1200, 2247)`
- policy output valid-from anchor: `2247`
- CUDA verification: `true`
- replay-backed sample count: `1047`
- certified replay chunks: `17`
- certified replay coverage: `[1200, 2247)`
- policy experience mode: `on_policy_current_rollout`
- PPO learning rate: `0.00002`
- optimizer steps: `6`
- policy quality claimed: `false`
- market readiness claimed: `false`
- readiness note: diagnostic performance evidence only; this is not policy
  selection, market readiness, deployment readiness, or live authority.
- baseline note:
  `policy_training_anchor_v1_performance_baseline.md`

Inspect the policy-environment performance reports:

```sh
POLICY_JOB_DIR=/cuwacunu/.runtime/cuwacunu_exec/components/wikimyei.policy.portfolio.graph_node_allocation/spawns/wikimyei.policy.portfolio.graph_node_allocation.ppo_v0_4b5ba29b8ad254d7/jobs/train/policy_training_ppo_v0_fd91f7620c46632a.attempt_fd91f7620c46632a_1781988984173151_44435
POLICY_REPORT_DIR="$POLICY_JOB_DIR/artifacts/wikimyei.policy.portfolio.graph_node_allocation/reports"

rg -n '^(sample_count|sample_count_low|ppo_target_kl|ppo_approx_kl|ppo_kl_target_ratio|ppo_kl_target_exceeded|ppo_clip_fraction|ppo_clip_fraction_high|certified_replay_bank_bound|certified_replay_bank_manifest_digest|certified_replay_chunk_count|certified_replay_sample_count|certified_replay_sample_count_low|certified_replay_sample_deficit_to_threshold|policy_training_uses_all_certified_replay|certified_replay_sample_utilization_ratio|ppo_rollout_sample_cap_limited|certified_replay_source_capacity_limited|certified_replay_scaling_bottleneck_kind|certified_replay_scaling_recommendation|certified_replay_coverage_anchor_begin|certified_replay_coverage_anchor_end_exclusive|policy_experience_set_digest|policy_experience_mode|training_replay_backing_complete|validation_rollout_ready|policy_quality_comparison_complete|profile_warning_count|profile_warning_|suggested_next_performance_tag|performance_profile_claimed|policy_quality_claimed|report_integrity_only)' \
  "$POLICY_REPORT_DIR/policy_training_anchor_v1_performance_profile.report"

rg -n '^(schema|bank_id|chunk_count|sample_count|coverage_anchor_begin|coverage_anchor_end_exclusive|chunk_[0-9]+_(digest|coverage_anchor_begin|coverage_anchor_end_exclusive|anchor_count|artifact_path_index_digest))' \
  "$POLICY_REPORT_DIR/certified_replay_bank_manifest.report"

rg -n '^(validation_rollout_ready|validation_runtime_replay_complete|sample_count|replay_backed_step_count|final_equity_numeraire|total_log_growth|max_drawdown|turnover|transaction_cost_numeraire|invalid_action_count|invalid_trace_count|reward_total)' \
  "$POLICY_REPORT_DIR/ppo_v0_validation_rollout.report"

rg -n '^(quality_report_ready|baseline_set_complete|comparison_policy_set_complete|policy_[0-9]+_(id|kind|completed_count|failed_count|mean_total_reward|mean_total_log_growth|mean_final_equity_numeraire|mean_max_drawdown|mean_total_turnover|cajtucu_valid_trace_count|cajtucu_invalid_trace_count))' \
  "$POLICY_REPORT_DIR/policy_quality_report.report"
```

Current policy-environment performance evidence:

- validation rollout ready: `true`
- validation runtime replay complete: `true`
- validation replay-backed steps: `1047`
- validation final equity numeraire: `10411`
- validation total log growth: `0.0228491`
- validation max drawdown: `0.13409`
- validation invalid actions: `0`
- PPO approximate KL: `0.0141982`
- PPO KL target exceeded: `false`
- PPO clip fraction: `0.0706781`
- PPO clip fraction high: `false`
- performance profile warnings: `0`
- current tuning tag: `policy_quality_comparison`
- certified replay bank bound: `true`
- certified replay bank digest:
  `412f9341e7b6e577fc1873e064b4a49a56ff0d0af1899eaa72fcacb0b17d817c`
- certified replay chunks: `17`
- certified replay samples: `1047`
- certified replay sample deficit to threshold: `0`
- certified replay source capacity limited: `false`
- policy training used all certified replay: `true`
- certified replay scaling recommendation:
  `compare_policy_quality`
- certified replay coverage: `[1200, 2247)`
- policy experience mode: `on_policy_current_rollout`
- policy experience set digest:
  `36bd6289049d45819a45840d479117aa3910c3bb014627cc0e68e8fccd31b513`
- learned policy comparison id:
  `wikimyei.policy.portfolio.graph_node_allocation.v1`
- learned policy completed episodes: `17`
- learned policy failed episodes: `0`
- learned policy mean total log growth: `0.0228491`
- learned policy mean final equity numeraire: `10411`
- learned policy Cajtucu valid traces: `1047`
- learned policy Cajtucu invalid traces: `0`

These numbers are visibility for performance analysis only. They are not a
policy gate, deployment gate, or paper-online admission signal until the Lattice
readiness proof passes and Marshal observes the structured passing certificate
state for the exact evidence snapshot.

## Future Paper-Online Admission And Session

This section starts after `paper_online_readiness_contract_ready` is proven.
That target is not part of the completed training and replay-evaluation run.
Prepare the session through Marshal before asking Environment to issue admission
or run the paper session. Keep request files under the runtime root so
Environment policy can read them:

```sh
READINESS_JOB_DIR=/cuwacunu/.runtime/cuwacunu_exec/path/to/paper_online_readiness_job
REQUEST_DIR=/cuwacunu/.runtime/cuwacunu_exec/operator_requests
SESSION_ROOT=/cuwacunu/.runtime/cuwacunu_exec/jobs/paper_online_session/session_001
mkdir -p "$REQUEST_DIR"

cat > "$REQUEST_DIR/paper_online_session_admission.request.json" <<JSON
{
  "schema": "kikijyeba.environment.paper_online_session_admission_request.v1",
  "readiness": {
    "job_dir": "$READINESS_JOB_DIR",
    "target_id": "paper_online_readiness_contract_ready",
    "proof_certificate_digest": "<paper_online_readiness_certificate_digest>",
    "proof_checked_at_ms": 111000,
    "max_proof_age_ms": 60000
  },
  "session": {
    "admission_id": "paper_online_session_admission_001",
    "requested_at_ms": 111000
  }
}
JSON

cat > "$REQUEST_DIR/paper_online_session_handoff.request.json" <<JSON
{
  "schema": "kikijyeba.marshal.paper_online_session_handoff_request.v1",
  "config_path": "$GLOBAL_CONFIG",
  "runtime_root": "/cuwacunu/.runtime/cuwacunu_exec",
  "readiness": {
    "job_dir": "$READINESS_JOB_DIR",
    "target_id": "paper_online_readiness_contract_ready",
    "proof_certificate_digest": "<paper_online_readiness_certificate_digest>",
    "proof_checked_at_ms": 111000,
    "max_proof_age_ms": 60000
  },
  "session": {
    "admission_id": "paper_online_session_admission_001",
    "admission_requested_at_ms": 111000,
    "session_id": "paper_online_session_001",
    "session_root": "$SESSION_ROOT",
    "session_requested_at_ms": 112000,
    "max_steps": 64,
    "step_interval_ms": 1000,
    "market_data_receive_lag_ms": 100,
    "target_node_ids": ["USDT", "BTC", "ETH"],
    "recover_persistent_ledger": true
  },
  "receipt_path": "$READINESS_JOB_DIR/marshal.paper_online_session_handoff.paper_online_session_001.receipt.json",
  "handoff_id": "paper_online_session_001.paper_online_session_handoff",
  "timeout_seconds": 600
}
JSON
```

Ask Marshal for the handoff plan and safe dry-run first. Before admission is
issued, dry-run should stop at `dispatch_state=admission_ready`:

```sh
.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.paper_online.session_handoff \
  --args-json "{\"mode\":\"plan\",\"handoff_request_path\":\"$REQUEST_DIR/paper_online_session_handoff.request.json\"}"

.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.paper_online.session_handoff \
  --args-json "{\"mode\":\"dry_run\",\"handoff_request_path\":\"$REQUEST_DIR/paper_online_session_handoff.request.json\"}"
```

Then check and issue Environment admission with the returned `preview_digest`:

```sh
.build/hero/hero_environment.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.environment.certify.paper_online_session_admission \
  --args-json "{\"mode\":\"check\",\"admission_request_path\":\"$REQUEST_DIR/paper_online_session_admission.request.json\"}"

.build/hero/hero_environment.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.environment.certify.paper_online_session_admission \
  --args-json "{\"mode\":\"issue\",\"admission_request_path\":\"$REQUEST_DIR/paper_online_session_admission.request.json\",\"expected_preview_digest\":\"<preview_digest>\"}"
```

Rerun the Marshal dry-run after admission is issued. It should reach
`dispatch_state=prepared` and write the handoff receipt:

```sh
.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.paper_online.session_handoff \
  --args-json "{\"mode\":\"dry_run\",\"handoff_request_path\":\"$REQUEST_DIR/paper_online_session_handoff.request.json\"}"
```

Run through the same Marshal handoff when the prepared receipt is accepted. The
run mode repeats the admission check and session validation, then delegates the
bounded paper session to Environment:

```sh
.build/hero/hero_marshal.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.marshal.paper_online.session_handoff \
  --args-json "{\"mode\":\"run\",\"handoff_request_path\":\"$REQUEST_DIR/paper_online_session_handoff.request.json\"}"
```

The successful Marshal run returns `dispatch_state=executed`; if the delegated
Environment run is refused, it returns `dispatch_state=run_blocked` with the
Environment run digest and error. Environment writes the session artifacts and
`lattice.paper_online_session.fact` under `SESSION_ROOT`. Inspect the Lattice
family after execution:

```sh
.build/hero/hero_lattice.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.lattice.inspect.facts.summary \
  --args-json '{"family":"paper_online_session"}'

.build/hero/hero_lattice.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.lattice.inspect.facts.preview \
  --args-json '{"family":"paper_online_session","limit":5}'
```
