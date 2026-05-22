# Runtime Hero v2 policy
protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO
runtime_exec_path:path = /cuwacunu/.build/exec/cuwacunu_exec
default_config_path:path = /cuwacunu/src/config/.config
runtime_root:path = /cuwacunu/.runtime/cuwacunu_exec
allowed_job_roots:path_list = /cuwacunu/.runtime/cuwacunu_exec,/tmp
default_dry_run:bool = true
allow_execute:bool = false
allow_train_execute:bool = false
allow_force_rebuild_cache:bool = false
require_confirm_execute:bool = true
allow_dev_nuke:bool = false
require_confirm_dev_nuke:bool = true
dev_nuke_backup_enabled:bool = true
dev_nuke_backup_root:path = /cuwacunu/.runtime/.backups/hero.runtime_dev_nuke
allowed_dev_nuke_roots:path_list = /cuwacunu/.runtime/cuwacunu_exec
max_capture_bytes:int = 65536
max_runtime_seconds:int = 600
