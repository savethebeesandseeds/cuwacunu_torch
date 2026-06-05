# Lattice Hero v0 policy
#
# Lattice Hero is read-only. It evaluates lattice target readiness, scans
# runtime artifacts into an in-memory exposure ledger, inspects checkpoint
# exposure closures, batches multi-target evaluation through one scan, reports
# rebuildable index/cache status, and queries audit index rows with live-scan
# parity proof or an explicit unproven cache fast path. It does not execute
# waves, and cache rows are read models over immutable runtime/fact files rather
# than primary evidence.
protocol_layer[STDIO|HTTPS/SSE]:enum = STDIO

# Default global config that contains Hero-owned Lattice target/split DSL paths.
default_config_path:path = /cuwacunu/src/config/.config

# Runtime artifact root scanned for job.manifest/job.state/report/checkpoint
# evidence.
runtime_root:path = /cuwacunu/.runtime/cuwacunu_exec

# Explicit runtime_root and checkpoint_path arguments must live below one of
# these roots.
allowed_runtime_roots:path_list = /cuwacunu/.runtime/cuwacunu_exec,/tmp

# Build an in-memory exposure ledger from normal runtime artifacts during
# target evaluation. This should stay true unless a future DB index is used as a
# cache and can prove it is current.
auto_build_exposure_ledger:bool = true

# Tool output limits.
max_fact_preview:int = 64
max_closure_facts:int = 256
