# Environment Hero v1 policy
#
# Environment Hero owns execution-environment admission and environment-run
# evidence. Runtime Hero remains the low-level executor, Lattice Hero remains
# read-only proof authority, and Marshal Hero remains the high-level operator.
protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO
environment_profile:enum = operator_default
default_config_path:path = /cuwacunu/src/config/.config
runtime_root:path = /cuwacunu/.runtime/cuwacunu_exec
allowed_job_roots:path_list = /cuwacunu/.runtime/cuwacunu_exec,/tmp
allow_certify_issue:bool = true
allow_rollout_replay:bool = true
max_capture_bytes:int = 65536
max_runtime_seconds:int = 600

ENVIRONMENT_PROFILE operator_default {
  allow_certify_issue:bool = true
  allow_rollout_replay:bool = true
  max_runtime_seconds:int = 600
}
