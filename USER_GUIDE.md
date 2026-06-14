# Cuwacunu User Guide

This guide describes the clean Hero operator path for the current repository
state. Use the checked-in config files under `src/config/`; do not create
temporary config copies for real training.

## Ground Rules

- `src/config/.config` is the only checked-in `.config` file.
- Train-core long training policies live in
  `src/config/operator/cwu_02v_train_core/` as `.jkimyei` files only.
- Marshal is the preferred operator surface for target preparation and Runtime
  handoff materialization.
- Runtime executes accepted work and writes evidence. Lattice proves target
  satisfaction. Environment writes rollout and replay evidence.
- Live trading and live capital remain disabled. Training, dry-run execution,
  replay execution, and paper/replay evaluation are allowed.

All examples use the canonical config explicitly:

```sh
GLOBAL_CONFIG=/cuwacunu/src/config/.config
```

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
  --args-json '{"target_id":"cwu_02v_representation_train_core_ready","include_machine_payload":false}'
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
  --args-json '{"target_id":"cwu_02v_mdn_train_core_no_test_leakage","include_machine_payload":false}'
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
  --args-json '{"target_id":"replay_environment_artifact_ready","include_machine_payload":false}'

.build/hero/hero_lattice.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.lattice.evaluate.deficit \
  --args-json '{"target_id":"allocation_artifact_ready","include_machine_payload":false}'
```

Current clean-run evidence:

- rollout id: `full_training_environment_eval_v1`
- replay certificate: `cert_d7b0fdb43e80`
- replay fact digest: `e164fe9c6aca6c44`
- replay report digest: `e924e34ec5f44dce`
- allocation fact digest: `221f2adb2d25bcef`
- mean total reward: `0.0126205`
- mean total log growth: `0.0136851`
- mean final equity numeraire: `1.01828`
- episode count: `16`

## Paper-Online Admission And Session

After Lattice proves `paper_online_readiness_contract_ready`, prepare the
session through Marshal before asking Environment to issue admission or run the
paper session. Keep request files under the runtime root so Environment policy
can read them:

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

## PPO Policy Training

The current learned-policy path is the bounded PPO V0 contract:

```text
/cuwacunu/src/config/policy_training_ppo_v0.contract
```

Select the PPO policy wave:

```ini
[HERO]
runtime_wave_id = policy_training_ppo_v0
```

Dry-run the durable contract:

```sh
.build/hero/hero_runtime.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.runtime.run \
  --args-json '{"mode":"dry_run","contract_path":"/cuwacunu/src/config/policy_training_ppo_v0.contract"}'
```

Execute with the contract digest returned by the dry run:

```sh
.build/hero/hero_runtime.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.runtime.run \
  --args-json '{"mode":"execute","contract_path":"/cuwacunu/src/config/policy_training_ppo_v0.contract","contract_digest":"8002b464b7472d02"}'
```

Verify policy readiness:

```sh
.build/hero/hero_lattice.mcp \
  --global-config "$GLOBAL_CONFIG" \
  --tool hero.lattice.evaluate.deficit \
  --args-json '{"target_id":"policy_training_artifact_ready"}'
```

Current clean-run evidence:

- job id:
  `policy_training_ppo_v0_8002b464b7472d02.attempt_8002b464b7472d02_1781416475224990_69960`
- policy fact digest: `1095cf66c1a0c48b`
- certificate: `cert_070bdb168a73`
- certificate digest: `070bdb168a737d0b`
- actor checkpoint digest:
  `e1e37a6076e221ecbcd83f8e59f6e74e3f3fa57b686d84da9eef8581c67587be`
- critic checkpoint digest:
  `2b54bda7168d6ce1f1a7de416dc5a2b856864b76b8940692eeabba6f3c0cd7ef`
- optimizer state digest:
  `46375e6db71e83c3011d6a6af7172f5e87cf96f414e54407e2829547d1b3c78b`
- optimizer Torch state digest:
  `0b37050815365576ebe6c0f4cab36eb7f4216e6f427f09c0a272bfe55da71f7f`
- rollout collection digest:
  `bb619f56d077efe8fee173be41ae52fe2248ca0cacb4208848cb4d257001fa52`
- PPO update report digest:
  `2f2df3a0506d29b11e00b76fbcbe6b434254ba8b7b41e7f4b9cf1de3d293248e`
- validation rollout report digest:
  `e3cc6de6c7b30853eda8024cdc2c4c2dce2a84caa310592a88565080d945546f`
- policy quality report digest:
  `2dec5d77285d9e39cbc95158c1489af1ffcdb9beaa16801302aa46c1774b1dc8`
- CUDA verification: `true`
- replay-backed sample count: `250`
- optimizer steps: `1`
