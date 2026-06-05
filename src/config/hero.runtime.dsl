# Runtime Hero v2 policy
protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO
runtime_profile:enum = operator_default
runtime_exec_path:path = /cuwacunu/.build/exec/cuwacunu_exec
default_config_path:path = /cuwacunu/src/config/.config
runtime_root:path = /cuwacunu/.runtime/cuwacunu_exec
allowed_job_roots:path_list = /cuwacunu/.runtime/cuwacunu_exec,/tmp
allow_force_rebuild_cache:bool = false
require_confirm_execute:bool = true
require_confirm_dev_nuke:bool = true
dev_nuke_backup_enabled:bool = false
dev_nuke_backup_root:path = /tmp/cuwacunu_runtime_dev_nuke_backups/hero.runtime_dev_nuke
allowed_dev_nuke_roots:path_list = /cuwacunu/.runtime
max_capture_bytes:int = 65536
max_runtime_seconds:int = 600

RUNTIME_PROFILE operator_default {
  default_dry_run:bool = true
  allow_execute:bool = true
  allow_train_execute:bool = true
  allow_dev_nuke:bool = true
  max_runtime_seconds:int = 600
}

RUNTIME_PROFILE long_train_operator {
  default_dry_run:bool = true
  allow_execute:bool = true
  allow_train_execute:bool = true
  allow_dev_nuke:bool = false
  max_runtime_seconds:int = 21600
}
