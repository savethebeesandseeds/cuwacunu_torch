# Runtime Hero v2 policy
protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO
runtime_profile:enum = operator_default
runtime_exec_path:path = ../../.build/exec/cuwacunu_exec
default_config_path:path = .config
runtime_root:path = ../../.runtime/cuwacunu_exec
allowed_job_roots:path_list = ../../.runtime/cuwacunu_exec,/tmp
allow_force_rebuild_cache:bool = true
dev_nuke_backup_enabled:bool = false
dev_nuke_backup_root:path = ../../.backups/runtime_dev_nuke
allowed_dev_nuke_roots:path_list = ../../.runtime
max_capture_bytes:int = 65536
max_runtime_seconds:int = 0

RUNTIME_PROFILE operator_default {
  default_dry_run:bool = true
  allow_execute:bool = true
  allow_train_execute:bool = true
  allow_dev_nuke:bool = true
  max_runtime_seconds:int = 0
}

RUNTIME_PROFILE long_train_operator {
  default_dry_run:bool = true
  allow_execute:bool = true
  allow_train_execute:bool = true
  allow_dev_nuke:bool = true
  max_runtime_seconds:int = 0
}
