# Config Hero v2 policy
protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO
config_root:path = .
managed_roots:path_list = .
allow_read:bool = true
allow_write:bool = false
write_roots:path_list = /tmp
allowed_extensions:string_list = .config,.dsl,.bnf,.man,.md,.net,.jkimyei
backup_enabled:bool = true
backup_dir:path = ../../.backups/hero.config
backup_max_entries:int = 20
require_expected_sha256_for_replace:bool = true
require_expected_sha256_for_delete:bool = true
