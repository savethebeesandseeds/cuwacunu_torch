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
rollout_policy_set:string = numeraire,current_weight,equal_weight,sdu
rollout_max_steps:int = 250
rollout_max_parallel_jobs:int = 4
rollout_runtime_exec_path:path = /cuwacunu/.build/exec/cuwacunu_exec
rollout_execution_backend_id:string = cajtucu.execution.paper.v1
rollout_cost_model_id:string = linear_transaction_cost_rate.v1
rollout_allow_synthetic_direct_edges:bool = false
rollout_synthetic_edge_research_reason:string =
rollout_linear_transaction_cost_rate:number = 0.001
rollout_allow_partial_fills:bool = false
rollout_equity_mismatch_tolerance:number = 0.000001
rollout_equity_mismatch_fail_tolerance:number = 0.01
rollout_live_execution_allowed:bool = false

ENVIRONMENT_PROFILE operator_default {
  allow_certify_issue:bool = true
  allow_rollout_replay:bool = true
  max_runtime_seconds:int = 600
  rollout_policy_set:string = numeraire,current_weight,equal_weight,sdu
  rollout_max_steps:int = 250
  rollout_max_parallel_jobs:int = 4
  rollout_runtime_exec_path:path = /cuwacunu/.build/exec/cuwacunu_exec
  rollout_execution_backend_id:string = cajtucu.execution.paper.v1
  rollout_cost_model_id:string = linear_transaction_cost_rate.v1
  rollout_allow_synthetic_direct_edges:bool = false
  rollout_synthetic_edge_research_reason:string =
  rollout_linear_transaction_cost_rate:number = 0.001
  rollout_allow_partial_fills:bool = false
  rollout_equity_mismatch_tolerance:number = 0.000001
  rollout_equity_mismatch_fail_tolerance:number = 0.01
  rollout_live_execution_allowed:bool = false
}
