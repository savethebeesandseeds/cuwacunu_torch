# Tool Inspection Temp

Temporary inspection snapshot of the current Hero MCP public tool surfaces,
direct arguments, and file-backed request arguments where present.

Last refreshed: 2026-06-12 from the built Hero MCP catalogs under
`.build/hero` using `--global-config /cuwacunu/src/config/.config`.

Notes:

- The public catalog check currently covers 103 tools and matches the table's
  public tool names exactly.
- This table is normalized to one row per accepted argument occurrence.
- `args_digest` is the digest string for the file named by `args_path`; it is
  not another file path.
- `args_path` files are `.kv` request packets.
- Duplicate argument names appear more than once when accepted by different
  tools or different nested request packet shapes.
- This table lists path/digest arguments for domain artifacts such as
  `contract_path` and `runtime_handoff_path`; it does not expand the internal
  schemas of those domain artifacts.
- Public catalog sources refreshed:
  `.build/hero/hero_config.mcp`,
  `.build/hero/hero_environment.mcp`,
  `.build/hero/hero_runtime.mcp`,
  `.build/hero/hero_lattice.mcp`, and
  `.build/hero/hero_marshal.mcp`.
- Expanded `.kv` request rows remain implementation-derived because the public
  MCP schemas intentionally expose only `args_path`/`args_digest` for those
  packets.

Remaining high-count disposition:

| Tool family | Disposition |
| --- | --- |
| `hero.environment.certify.*` | Keep file-backed. Certification evidence is per-request evidence, not stable hero configuration. |
| rollout pair | Keep file-backed. Rollout packets are execution/audit requests that need digest binding. |
| `hero.marshal.prepare.*` | Derouted by intent and driver shape. Prepare handoff fields stay file-backed, but `intent` and `drive_mode` are encoded in the concrete tool name. |
| `hero.marshal.inspect.protocol.strict` | Keep direct. The five expected identity values are one coherent strict assertion, not separate subjects. |
| `hero.marshal.inspect.component` | Keep direct. The count is common runtime/config/job context plus one component selector. |
| `hero.marshal.inspect.facts.*` | Keep direct after preview derouting. The count is common target/fact/runtime/config context plus optional lineage/machine payload controls and, for preview suffixes, one explicit row selector. |
| `hero.runtime.run` | Reviewed in the final pass. Remaining file-backed fields are wave/replay locators, policy-training evidence, lineage, schedules, bounds, hyperparameters, or artifact digests; fixed policy-training contract constants are derived by Runtime. |

| Tool | Surface | Argument | Scope / requirement |
| --- | --- | --- | --- |
| `hero.config.status` | public | `(none)` | Tool accepts no arguments. |
| `hero.config.inspect.schema` | public | `(none)` | Tool accepts no arguments. |
| `hero.config.inspect.show` | public | `(none)` | Tool accepts no arguments. |
| `hero.config.inspect.value` | public | `key` | Required Config policy key. |
| `hero.config.inspect.validate_global_config` | public | `(none)` | Tool accepts no arguments. |
| `hero.config.inspect.map` | public | `include_sha256` | Optional boolean. |
| `hero.config.inspect.bundle` | public | `config_path` | Optional managed config path override. |
| `hero.config.inspect.bundle` | public | `include_content` | Optional boolean. |
| `hero.config.inspect.resolve_path.read` | public | `path` | Required managed path to resolve for read. |
| `hero.config.inspect.resolve_path.read` | public | `include_sha256` | Optional boolean. |
| `hero.config.inspect.resolve_path.write` | public | `path` | Required managed path to resolve for write. |
| `hero.config.inspect.resolve_path.write` | public | `include_sha256` | Optional boolean. |
| `hero.config.inspect.diff` | public | `include_content` | Optional boolean. |
| `hero.config.inspect.backups` | public | `(none)` | Tool accepts no arguments. |
| `hero.config.inspect.file_list` | public | `path` | Optional managed root/file path. |
| `hero.config.inspect.file_list` | public | `recursive` | Optional boolean. |
| `hero.config.inspect.file_list` | public | `include_sha256` | Optional boolean. |
| `hero.config.inspect.file_list` | public | `limit` | Optional non-negative integer. |
| `hero.config.inspect.file_read` | public | `path` | Required managed file path. |
| `hero.config.apply.set` | public | `key` | Required Config policy key. |
| `hero.config.apply.set` | public | `value` | Required Config policy value. |
| `hero.config.apply.set` | public | `reason` | Optional audit note echoed in the result. |
| `hero.config.apply.save` | public | `reason` | Optional audit note echoed in the result. |
| `hero.config.apply.save` | public | `include_content` | Optional boolean for the internal save preflight preview. |
| `hero.config.apply.reload` | public | `reason` | Optional audit note echoed in the result. |
| `hero.config.apply.rollback` | public | `backup_id` | Optional backup selector; latest eligible backup is selected when omitted. |
| `hero.config.apply.rollback` | public | `reason` | Optional audit note echoed in the result. |
| `hero.config.apply.write` | public | `path` | Required managed config path. |
| `hero.config.apply.write` | public | `content` | Required replacement file content. |
| `hero.config.apply.write` | public | `expected_sha256` | Optional expected current file digest. |
| `hero.config.apply.write` | public | `reason` | Optional audit note echoed in the result. |
| `hero.config.apply.delete` | public | `path` | Required managed config path. |
| `hero.config.apply.delete` | public | `expected_sha256` | Optional expected current file digest. |
| `hero.config.apply.delete` | public | `reason` | Optional audit note echoed in the result. |
| `hero.environment.status` | public | `(none)` | Tool accepts no arguments. |
| `hero.environment.inspect.schema` | public | `(none)` | Tool accepts no arguments. |
| `hero.environment.inspect.job` | public | `job_dir` | Required Environment job directory. |
| `hero.environment.inspect.job` | public | `include_text` | Optional boolean. |
| `hero.environment.inspect.job` | public | `max_bytes` | Optional non-negative text preview byte limit. |
| `hero.environment.certify.policy_acceptance` | public | `mode` | Required; enum: `check`, `issue`. |
| `hero.environment.certify.policy_acceptance` | public | `args_path` | Required `.kv` certification request packet path. |
| `hero.environment.certify.policy_acceptance` | public | `args_digest` | Optional by schema; required for `mode=issue`. |
| `hero.environment.certify.paper_online_readiness` | public | `mode` | Required; enum: `check`, `issue`. |
| `hero.environment.certify.paper_online_readiness` | public | `args_path` | Required `.kv` certification request packet path. |
| `hero.environment.certify.paper_online_readiness` | public | `args_digest` | Optional by schema; required for `mode=issue`. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `policy_training_job_dir` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `tsodao_settings_protection_job_dir` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `acceptance_id` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `accepted_policy_training_proof_certificate_digest` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `tsodao_settings_protection_proof_certificate_digest` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `primary_metric_value` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `accepted_policy_training_ready` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `tsodao_settings_protection_ready` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `protected_settings_bound` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `mandatory_baselines_bound` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `mandatory_baselines_passed` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `after_cost_metrics_bound` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `primary_metric_passed` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `uncertainty_policy_bound` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `uncertainty_passed` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `selector_split_bound` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `validation_test_disjoint` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `test_sealed_until_acceptance` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `negative_tests_bound` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `negative_tests_passed` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `leakage_negative_tests_passed` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `threshold_selection_audit_bound` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `threshold_selected_before_test` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `tie_policy_bound` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `tie_policy_passed` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `cost_slippage_assumptions_bound` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `promotion_criteria_bound` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `promotion_criteria_satisfied` | Policy-acceptance request field. |
| `hero.environment.certify.policy_acceptance` | `args_path` `.kv` | `policy_acceptance_decision` | Policy-acceptance request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `policy_acceptance_job_dir` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `readiness_id` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `policy_acceptance_proof_certificate_digest` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `paper_online_profile_digest` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `direct_edge_universe_digest` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `max_market_data_staleness_ms` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `clock_skew_tolerance_ms` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `policy_acceptance_ready` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `tsodao_settings_protection_ready` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `accepted_policy_bound` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `protected_settings_bound` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `direct_edge_universe_validated` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `locked_execution_profile_bound` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `persistent_paper_ledger_recovery_bound` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `idempotency_bound` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `duplicate_action_protection_bound` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `session_lifecycle_bound` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `clock_timestamp_policy_bound` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `market_data_staleness_bound` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `reward_report_artifact_path_bound` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `operator_abort_bound` | Paper-online-readiness request field. |
| `hero.environment.certify.paper_online_readiness` | `args_path` `.kv` | `kill_switch_bound` | Paper-online-readiness request field. |
| `hero.environment.rollout` | public | `mode` | Required; enum: `validate`, `replay`. |
| `hero.environment.rollout` | public | `args_path` | Required `.kv` rollout request packet path. |
| `hero.environment.rollout` | public | `args_digest` | Optional by schema; required for `mode=replay`. |
| `hero.environment.rollout` | public | `include_machine_payload` | Optional response verbosity flag. |
| `hero.environment.rollout` | `args_path` `.kv` | `rollout_id` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `rollout_attempt_id` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `idempotency_key` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `experiment_id` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `config_path` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `runtime_job_dir` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `replay_batch_index_path` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `runtime_exec_path` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `report_path` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `graph_order_fingerprint` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `asset_universe_digest` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `accounting_numeraire_node_id` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `target_node_ids` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `policy_set` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `max_steps` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `max_parallel_jobs` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `execution_profile` | Rollout request field. |
| `hero.environment.rollout` | `args_path` `.kv` | `timeout_seconds` | Rollout request field. |
| `hero.runtime.status` | public | `(none)` | Tool accepts no arguments. |
| `hero.runtime.inspect.schema` | public | `(none)` | Tool accepts no arguments. |
| `hero.runtime.inspect.wave` | public | `config_path` | Optional global config path override. |
| `hero.runtime.inspect.jobs` | public | `root` | Optional runtime job root. |
| `hero.runtime.inspect.jobs` | public | `limit` | Optional non-negative job row limit. |
| `hero.runtime.inspect.jobs` | public | `include_artifacts` | Optional boolean. |
| `hero.runtime.inspect.job` | public | `job_id` | Runtime job id; mutually exclusive with `job_dir`. |
| `hero.runtime.inspect.job` | public | `job_dir` | Runtime job directory; mutually exclusive with `job_id`. |
| `hero.runtime.inspect.job` | public | `include_text` | Optional boolean. |
| `hero.runtime.inspect.job` | public | `max_bytes` | Optional positive text preview byte limit. |
| `hero.runtime.inspect.artifact.job` | public | `job_id` | Runtime job id; mutually exclusive with `job_dir`. |
| `hero.runtime.inspect.artifact.job` | public | `job_dir` | Runtime job directory; mutually exclusive with `job_id`. |
| `hero.runtime.inspect.artifact.job` | public | `artifact` | Optional named artifact. |
| `hero.runtime.inspect.artifact.job` | public | `max_bytes` | Optional text preview byte limit. |
| `hero.runtime.inspect.artifact.path` | public | `path` | Required explicit artifact path. |
| `hero.runtime.inspect.artifact.path` | public | `max_bytes` | Optional text preview byte limit. |
| `hero.runtime.run` | public | `mode` | Required; enum: `dry_run`, `execute`. |
| `hero.runtime.run` | public | `args_path` | Optional top-level Runtime run request packet path. |
| `hero.runtime.run` | public | `args_digest` | Optional by schema; required for `mode=execute` when `args_path` is supplied. |
| `hero.runtime.replay` | internal non-catalog | `mode` | Required; enum: `dry_run`, `execute`. |
| `hero.runtime.replay` | internal non-catalog | `args_path` | Optional top-level Runtime replay request packet path. |
| `hero.runtime.replay` | internal non-catalog | `args_digest` | Optional by schema; required for `mode=execute` when `args_path` is supplied. |
| `hero.runtime.run` | top-level `args_path` `.kv` | `config_path` | Runtime run request field. |
| `hero.runtime.run` | top-level `args_path` `.kv` | `contract_path` | Runtime run request field. |
| `hero.runtime.run` | top-level `args_path` `.kv` | `contract_digest` | Runtime run request field. |
| `hero.runtime.run` | top-level `args_path` `.kv` | `execution_request_path` | Runtime run request field pointing to a nested execution `.kv`. |
| `hero.runtime.run` | top-level `args_path` `.kv` | `execution_request_digest` | Digest for `execution_request_path`. |
| `hero.runtime.run` | top-level `args_path` `.kv` | `runtime_handoff_path` | Runtime run request field pointing to a handoff artifact. |
| `hero.runtime.run` | top-level `args_path` `.kv` | `runtime_handoff_digest` | Digest for `runtime_handoff_path`. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `config_path` | Wave execution request field. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `job_dir` | Wave execution request field. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `force_rebuild_cache` | Wave execution request field. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `source_range` | Wave execution request field. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `anchor_index_begin` | Wave execution request field. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `anchor_index_end` | Wave execution request field. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `source_key_begin` | Wave execution request field. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `source_key_end` | Wave execution request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `job_id` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `job_dir` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `config_path` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `accounting_numeraire_node_id` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `target_node_ids` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `experiment_id` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `report_path` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `initial_equity_numeraire` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `max_node_weight` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `max_turnover_l1` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `max_steps` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `max_parallel_jobs` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `include_equal_weight` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `include_current_weight` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `include_graph_node_allocation_policy` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `on_policy_sample` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `include_numeraire_only_policy` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `include_spot_distributional_utility_policy` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `allow_synthetic_direct_edges` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `validation_rollout` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `linear_transaction_cost_rate` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `execution_profile_digest` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `policy_set_digest` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `policy_artifact_digest` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `action_distribution_id` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `causal_schedule_digest` | Internal delegated replay request field. |
| `hero.runtime.replay` | nested `execution_request_path` `.kv` | `snapshot_family_digest` | Internal delegated replay request field. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `job_id` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `job_dir` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `report_path` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `replay_job_dir` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `config_path` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `accounting_numeraire_node_id` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `target_node_ids` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `experiment_id` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `policy_set_digest` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `linear_transaction_cost_rate` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `initial_equity_numeraire` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `max_node_weight` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `max_turnover_l1` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `max_steps` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `max_parallel_jobs` | Contract-backed policy-training execution. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `job_id` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `job_dir` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `report_path` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `replay_job_dir` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `accounting_numeraire_node_id` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `target_node_ids` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `experiment_id` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `protocol_id` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `protocol_contract_fingerprint` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `graph_order_fingerprint` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `source_cursor_token` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `split_policy_fingerprint` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `component_assembly_fingerprint` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `policy_id` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `policy_kind` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `policy_architecture_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `training_config_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `training_range_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `validation_range_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `test_range_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `observation_schema_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `reward_contract_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `action_distribution_id` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `execution_profile_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `policy_set_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `policy_dsl_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `policy_net_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `policy_input_feature_manifest_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `policy_jkimyei_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `target_node_universe_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `action_distribution_config_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `snapshot_family_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `actor_architecture_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `critic_architecture_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `actor_checkpoint_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `critic_checkpoint_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `optimizer_state_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `ppo_config_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `resume_mode` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `resume_actor_checkpoint_path` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `resume_actor_checkpoint_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `resume_optimizer_state_path` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `resume_optimizer_state_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `advantage_normalization_policy` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `rollout_collection_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `ppo_update_report_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `validation_rollout_report_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `policy_quality_report_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `training_schedule_mode` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `causal_schedule_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `causal_schedule_cursor_key_kind` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `normalization_fit_range_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `replay_buffer_source_range_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `early_stopping_policy_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `hyperparameter_selection_policy_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `selector_split` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `selector_policy_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `parent_checkpoint_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `parent_forecast_eval_fact_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `parent_observer_belief_fact_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `parent_allocation_engine_fact_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `parent_replay_environment_fact_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `parent_replay_environment_report_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `final_refit_parent_selected_checkpoint_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `policy_artifact_digest` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `ppo_gamma` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `ppo_gae_lambda` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `ppo_clip_epsilon` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `ppo_target_kl` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `ppo_entropy_coeff` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `ppo_value_loss_coeff` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `ppo_max_grad_norm` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `linear_transaction_cost_rate` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `initial_equity_numeraire` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `max_node_weight` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `max_turnover_l1` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `ppo_minibatch_size` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `ppo_epochs_per_rollout` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `random_seed` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `max_episodes` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `max_steps` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `max_parallel_jobs` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `max_wall_clock_seconds` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `causal_schedule_readiness_eligible` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `causal_schedule_no_future_snapshot_use` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `offline_full_window_research_allowed` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `final_refit_uses_validation` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `validation_no_longer_proof` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `sealed_test_required` | Policy-training wave contract materialization. |
| `hero.runtime.run` | nested `execution_request_path` `.kv` | `live_execution_allowed` | Policy-training wave contract materialization. |
| `hero.runtime.reset` | public | `mode` | Required; enum: `plan`, `execute`. |
| `hero.runtime.reset` | public | `args_path` | Optional by schema; execute requires digest-pinned request. |
| `hero.runtime.reset` | public | `args_digest` | Optional by schema; execute requires digest-pinned request. |
| `hero.runtime.reset` | `args_path` `.kv` | `runtime_root` | Runtime reset request field. |
| `hero.runtime.reset` | `args_path` `.kv` | `backup` | Runtime reset request field. |
| `hero.lattice.status` | public | `(none)` | Tool accepts no arguments. |
| `hero.lattice.inspect.schema` | public | `(none)` | Tool accepts no arguments. |
| `hero.lattice.inspect.targets` | public | `config_path` | Optional global config path override. |
| `hero.lattice.inspect.target` | public | `target_id` | Required Lattice target id. |
| `hero.lattice.inspect.target` | public | `config_path` | Optional global config path override. |
| `hero.lattice.inspect.exposure` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.exposure` | public | `limit` | Optional fact preview row limit. |
| `hero.lattice.inspect.fact_families` | public | `runtime_root` | Optional runtime evidence root for catalog summary. |
| `hero.lattice.inspect.facts.summary` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.facts.summary` | public | `family` | Optional fact family selector. |
| `hero.lattice.inspect.facts.scan` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.facts.scan` | public | `family` | Optional fact family selector. |
| `hero.lattice.inspect.facts.scan` | public | `limit` | Optional row limit. |
| `hero.lattice.inspect.facts.scan` | public | `include_facts` | Optional boolean. |
| `hero.lattice.inspect.facts.lineage` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.facts.lineage` | public | `family` | Optional fact family selector. |
| `hero.lattice.inspect.facts.lineage` | public | `limit` | Optional row limit. |
| `hero.lattice.inspect.facts.preview` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.facts.preview` | public | `family` | Required fact family selector. |
| `hero.lattice.inspect.facts.preview` | public | `limit` | Optional fact preview limit. |
| `hero.lattice.inspect.facts.preview.by_digest` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.facts.preview.by_digest` | public | `family` | Required fact family selector. |
| `hero.lattice.inspect.facts.preview.by_digest` | public | `limit` | Optional fact preview limit. |
| `hero.lattice.inspect.facts.preview.by_digest` | public | `fact_digest` | Required exact fact digest selector. |
| `hero.lattice.inspect.facts.preview.by_digest_prefix` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.facts.preview.by_digest_prefix` | public | `family` | Required fact family selector. |
| `hero.lattice.inspect.facts.preview.by_digest_prefix` | public | `limit` | Optional fact preview limit. |
| `hero.lattice.inspect.facts.preview.by_digest_prefix` | public | `fact_digest_prefix` | Required unique fact digest prefix selector. |
| `hero.lattice.inspect.facts.preview.by_index` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.facts.preview.by_index` | public | `family` | Required fact family selector. |
| `hero.lattice.inspect.facts.preview.by_index` | public | `limit` | Optional fact preview limit. |
| `hero.lattice.inspect.facts.preview.by_index` | public | `fact_index` | Required fact row index selector. |
| `hero.lattice.inspect.index.status` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.index.status` | public | `index_path` | Optional runtime index cache path. |
| `hero.lattice.inspect.index.status` | public | `limit` | Optional index row/result limit. |
| `hero.lattice.inspect.index.status` | public | `validation_strength` | Optional cache validation strength. |
| `hero.lattice.inspect.index.query` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.index.query` | public | `index_path` | Optional runtime index cache path. |
| `hero.lattice.inspect.index.query` | public | `limit` | Optional index row/result limit. |
| `hero.lattice.inspect.index.query` | public | `validation_strength` | Optional cache validation strength. |
| `hero.lattice.inspect.index.query.by_relation` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.index.query.by_relation` | public | `index_path` | Optional runtime index cache path. |
| `hero.lattice.inspect.index.query.by_relation` | public | `limit` | Optional index row/result limit. |
| `hero.lattice.inspect.index.query.by_relation` | public | `validation_strength` | Optional cache validation strength. |
| `hero.lattice.inspect.index.query.by_relation` | public | `relation` | Required query relation. |
| `hero.lattice.inspect.index.query.by_key` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.index.query.by_key` | public | `index_path` | Optional runtime index cache path. |
| `hero.lattice.inspect.index.query.by_key` | public | `limit` | Optional index row/result limit. |
| `hero.lattice.inspect.index.query.by_key` | public | `validation_strength` | Optional cache validation strength. |
| `hero.lattice.inspect.index.query.by_key` | public | `key` | Required exact query key. |
| `hero.lattice.inspect.index.query.by_key_contains` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.index.query.by_key_contains` | public | `index_path` | Optional runtime index cache path. |
| `hero.lattice.inspect.index.query.by_key_contains` | public | `limit` | Optional index row/result limit. |
| `hero.lattice.inspect.index.query.by_key_contains` | public | `validation_strength` | Optional cache validation strength. |
| `hero.lattice.inspect.index.query.by_key_contains` | public | `key_contains` | Required key substring query. |
| `hero.lattice.inspect.index.query.by_digest` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.index.query.by_digest` | public | `index_path` | Optional runtime index cache path. |
| `hero.lattice.inspect.index.query.by_digest` | public | `limit` | Optional index row/result limit. |
| `hero.lattice.inspect.index.query.by_digest` | public | `validation_strength` | Optional cache validation strength. |
| `hero.lattice.inspect.index.query.by_digest` | public | `digest` | Required exact digest query. |
| `hero.lattice.inspect.index.query.by_digest_prefix` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.index.query.by_digest_prefix` | public | `index_path` | Optional runtime index cache path. |
| `hero.lattice.inspect.index.query.by_digest_prefix` | public | `limit` | Optional index row/result limit. |
| `hero.lattice.inspect.index.query.by_digest_prefix` | public | `validation_strength` | Optional cache validation strength. |
| `hero.lattice.inspect.index.query.by_digest_prefix` | public | `digest_prefix` | Required digest prefix query. |
| `hero.lattice.inspect.index.query.unproven_cache` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.index.query.unproven_cache` | public | `index_path` | Optional runtime index cache path. |
| `hero.lattice.inspect.index.query.unproven_cache` | public | `limit` | Optional index row/result limit. |
| `hero.lattice.inspect.index.query.unproven_cache` | public | `validation_strength` | Optional cache validation strength. |
| `hero.lattice.inspect.index.query.unproven_cache.by_relation` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.index.query.unproven_cache.by_relation` | public | `index_path` | Optional runtime index cache path. |
| `hero.lattice.inspect.index.query.unproven_cache.by_relation` | public | `limit` | Optional index row/result limit. |
| `hero.lattice.inspect.index.query.unproven_cache.by_relation` | public | `validation_strength` | Optional cache validation strength. |
| `hero.lattice.inspect.index.query.unproven_cache.by_relation` | public | `relation` | Required query relation. |
| `hero.lattice.inspect.index.query.unproven_cache.by_key` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.index.query.unproven_cache.by_key` | public | `index_path` | Optional runtime index cache path. |
| `hero.lattice.inspect.index.query.unproven_cache.by_key` | public | `limit` | Optional index row/result limit. |
| `hero.lattice.inspect.index.query.unproven_cache.by_key` | public | `validation_strength` | Optional cache validation strength. |
| `hero.lattice.inspect.index.query.unproven_cache.by_key` | public | `key` | Required exact query key. |
| `hero.lattice.inspect.index.query.unproven_cache.by_key_contains` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.index.query.unproven_cache.by_key_contains` | public | `index_path` | Optional runtime index cache path. |
| `hero.lattice.inspect.index.query.unproven_cache.by_key_contains` | public | `limit` | Optional index row/result limit. |
| `hero.lattice.inspect.index.query.unproven_cache.by_key_contains` | public | `validation_strength` | Optional cache validation strength. |
| `hero.lattice.inspect.index.query.unproven_cache.by_key_contains` | public | `key_contains` | Required key substring query. |
| `hero.lattice.inspect.index.query.unproven_cache.by_digest` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.index.query.unproven_cache.by_digest` | public | `index_path` | Optional runtime index cache path. |
| `hero.lattice.inspect.index.query.unproven_cache.by_digest` | public | `limit` | Optional index row/result limit. |
| `hero.lattice.inspect.index.query.unproven_cache.by_digest` | public | `validation_strength` | Optional cache validation strength. |
| `hero.lattice.inspect.index.query.unproven_cache.by_digest` | public | `digest` | Required exact digest query. |
| `hero.lattice.inspect.index.query.unproven_cache.by_digest_prefix` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.index.query.unproven_cache.by_digest_prefix` | public | `index_path` | Optional runtime index cache path. |
| `hero.lattice.inspect.index.query.unproven_cache.by_digest_prefix` | public | `limit` | Optional index row/result limit. |
| `hero.lattice.inspect.index.query.unproven_cache.by_digest_prefix` | public | `validation_strength` | Optional cache validation strength. |
| `hero.lattice.inspect.index.query.unproven_cache.by_digest_prefix` | public | `digest_prefix` | Required digest prefix query. |
| `hero.lattice.inspect.derived.target_satisfied` | public | `target_id` | Required target id. |
| `hero.lattice.inspect.derived.target_satisfied` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.derived.target_satisfied` | public | `config_path` | Optional global config path override. |
| `hero.lattice.inspect.derived.target_satisfied` | public | `limit` | Optional witness limit. |
| `hero.lattice.inspect.derived.forbidden_overlap` | public | `target_id` | Required target id. |
| `hero.lattice.inspect.derived.forbidden_overlap` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.derived.forbidden_overlap` | public | `config_path` | Optional global config path override. |
| `hero.lattice.inspect.derived.forbidden_overlap` | public | `limit` | Optional witness limit. |
| `hero.lattice.inspect.derived.unresolved_lineage.target` | public | `target_id` | Required target id. |
| `hero.lattice.inspect.derived.unresolved_lineage.target` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.derived.unresolved_lineage.target` | public | `config_path` | Optional global config path override. |
| `hero.lattice.inspect.derived.unresolved_lineage.target` | public | `limit` | Optional witness limit. |
| `hero.lattice.inspect.derived.stale_cache` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.derived.stale_cache` | public | `index_path` | Optional runtime index cache path. |
| `hero.lattice.inspect.derived.stale_cache` | public | `validation_strength` | Optional cache validation strength. |
| `hero.lattice.inspect.derived.stale_cache` | public | `limit` | Optional witness limit. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_path` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_path` | public | `limit` | Optional witness limit. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_path` | public | `checkpoint_path` | Required checkpoint path selector. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_path.with_ancestor_path` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_path.with_ancestor_path` | public | `limit` | Optional witness limit. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_path.with_ancestor_path` | public | `checkpoint_path` | Required checkpoint path selector. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_path.with_ancestor_path` | public | `ancestor_checkpoint_path` | Required ancestor checkpoint path selector. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_path.with_ancestor_id` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_path.with_ancestor_id` | public | `limit` | Optional witness limit. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_path.with_ancestor_id` | public | `checkpoint_path` | Required checkpoint path selector. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_path.with_ancestor_id` | public | `ancestor_checkpoint_id` | Required ancestor checkpoint id selector. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_identity` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_identity` | public | `limit` | Optional witness limit. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_identity` | public | `checkpoint_id` | Required checkpoint id selector. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_identity` | public | `checkpoint_file_digest` | Required checkpoint file digest selector. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_identity.with_ancestor_path` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_identity.with_ancestor_path` | public | `limit` | Optional witness limit. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_identity.with_ancestor_path` | public | `checkpoint_id` | Required checkpoint id selector. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_identity.with_ancestor_path` | public | `checkpoint_file_digest` | Required checkpoint file digest selector. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_identity.with_ancestor_path` | public | `ancestor_checkpoint_path` | Required ancestor checkpoint path selector. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_identity.with_ancestor_id` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_identity.with_ancestor_id` | public | `limit` | Optional witness limit. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_identity.with_ancestor_id` | public | `checkpoint_id` | Required checkpoint id selector. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_identity.with_ancestor_id` | public | `checkpoint_file_digest` | Required checkpoint file digest selector. |
| `hero.lattice.inspect.derived.checkpoint_ancestor.by_checkpoint_identity.with_ancestor_id` | public | `ancestor_checkpoint_id` | Required ancestor checkpoint id selector. |
| `hero.lattice.inspect.derived.unresolved_lineage.checkpoint` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.derived.unresolved_lineage.checkpoint` | public | `limit` | Optional witness limit. |
| `hero.lattice.inspect.derived.unresolved_lineage.checkpoint` | public | `checkpoint_path` | Optional checkpoint path selector. |
| `hero.lattice.inspect.derived.unresolved_lineage.checkpoint` | public | `checkpoint_id` | Optional checkpoint id selector. |
| `hero.lattice.inspect.derived.unresolved_lineage.checkpoint` | public | `checkpoint_file_digest` | Optional checkpoint file digest selector. |
| `hero.lattice.inspect.checkpoint` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.inspect.checkpoint` | public | `checkpoint_path` | Optional checkpoint path selector. |
| `hero.lattice.inspect.checkpoint` | public | `checkpoint_id` | Optional checkpoint id selector. |
| `hero.lattice.inspect.checkpoint` | public | `checkpoint_file_digest` | Optional checkpoint file digest selector. |
| `hero.lattice.inspect.checkpoint` | public | `limit` | Optional closure fact limit. |
| `hero.lattice.evaluate.target` | public | `target_id` | Required Lattice target id. |
| `hero.lattice.evaluate.target` | public | `config_path` | Optional global config path override. |
| `hero.lattice.evaluate.target` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.evaluate.targets` | public | `target_ids` | Optional target id array; omitted means configured targets up to limit. |
| `hero.lattice.evaluate.targets` | public | `config_path` | Optional global config path override. |
| `hero.lattice.evaluate.targets` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.evaluate.targets` | public | `limit` | Optional target evaluation limit. |
| `hero.lattice.evaluate.deficit` | public | `target_id` | Required Lattice target id. |
| `hero.lattice.evaluate.deficit` | public | `config_path` | Optional global config path override. |
| `hero.lattice.evaluate.deficit` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.evaluate.latest_satisfying_checkpoint.target` | public | `target_id` | Required source target id. |
| `hero.lattice.evaluate.latest_satisfying_checkpoint.target` | public | `config_path` | Optional global config path override. |
| `hero.lattice.evaluate.latest_satisfying_checkpoint.target` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.evaluate.latest_satisfying_checkpoint.hint` | public | `symbolic_hint` | Required `latest_satisfying:<target_id>` selector. |
| `hero.lattice.evaluate.latest_satisfying_checkpoint.hint` | public | `config_path` | Optional global config path override. |
| `hero.lattice.evaluate.latest_satisfying_checkpoint.hint` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.lattice.compare` | public | `left_target_id` | Required left target id. |
| `hero.lattice.compare` | public | `right_target_id` | Required right target id. |
| `hero.lattice.compare` | public | `config_path` | Optional global config path override. |
| `hero.lattice.compare` | public | `runtime_root` | Optional runtime evidence root. |
| `hero.marshal.status` | public | `(none)` | Tool accepts no arguments. |
| `hero.marshal.prepare.train.one_step` | public | `target_id` | Required target identifier. |
| `hero.marshal.prepare.train.one_step` | public | `mode` | Required; enum: `plan`, `dry_run`, `execute`. |
| `hero.marshal.prepare.train.one_step` | public | `args_path` | Optional `.kv` request packet path. |
| `hero.marshal.prepare.train.one_step` | public | `args_digest` | Optional by schema; non-plan modes require digest binding. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `config_path` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `runtime_root` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `driver_policy` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `resume_ledger` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `source_lattice_timestamp` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `recommendation_attempt_count` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `context` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `protocol_contract_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `graph_order_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `source_cursor_token` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `vicreg_assembly_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `mdn_assembly_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `materialize_plan_inputs` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `include_machine_payload` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `runtime_policy` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `runtime_wave` | Marshal prepare request field. |
| `hero.marshal.prepare.train.one_step` | `args_path` `.kv` | `timeout_seconds` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | public | `target_id` | Required target identifier. |
| `hero.marshal.prepare.train.budgeted` | public | `mode` | Required; enum: `plan`, `dry_run`, `execute`. |
| `hero.marshal.prepare.train.budgeted` | public | `args_path` | Optional `.kv` request packet path. |
| `hero.marshal.prepare.train.budgeted` | public | `args_digest` | Optional by schema; non-plan modes require digest binding. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `config_path` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `runtime_root` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `driver_policy` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `resume_ledger` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `source_lattice_timestamp` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `recommendation_attempt_count` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `context` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `protocol_contract_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `graph_order_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `source_cursor_token` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `vicreg_assembly_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `mdn_assembly_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `materialize_plan_inputs` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `include_machine_payload` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `runtime_policy` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `runtime_wave` | Marshal prepare request field. |
| `hero.marshal.prepare.train.budgeted` | `args_path` `.kv` | `timeout_seconds` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | public | `target_id` | Required target identifier. |
| `hero.marshal.prepare.evaluate.one_step` | public | `mode` | Required; enum: `plan`, `dry_run`, `execute`. |
| `hero.marshal.prepare.evaluate.one_step` | public | `args_path` | Optional `.kv` request packet path. |
| `hero.marshal.prepare.evaluate.one_step` | public | `args_digest` | Optional by schema; non-plan modes require digest binding. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `config_path` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `runtime_root` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `driver_policy` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `resume_ledger` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `source_lattice_timestamp` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `recommendation_attempt_count` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `context` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `protocol_contract_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `graph_order_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `source_cursor_token` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `vicreg_assembly_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `mdn_assembly_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `materialize_plan_inputs` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `include_machine_payload` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `runtime_policy` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `runtime_wave` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.one_step` | `args_path` `.kv` | `timeout_seconds` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | public | `target_id` | Required target identifier. |
| `hero.marshal.prepare.evaluate.budgeted` | public | `mode` | Required; enum: `plan`, `dry_run`, `execute`. |
| `hero.marshal.prepare.evaluate.budgeted` | public | `args_path` | Optional `.kv` request packet path. |
| `hero.marshal.prepare.evaluate.budgeted` | public | `args_digest` | Optional by schema; non-plan modes require digest binding. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `config_path` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `runtime_root` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `driver_policy` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `resume_ledger` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `source_lattice_timestamp` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `recommendation_attempt_count` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `context` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `protocol_contract_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `graph_order_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `source_cursor_token` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `vicreg_assembly_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `mdn_assembly_fingerprint` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `materialize_plan_inputs` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `include_machine_payload` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `runtime_policy` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `runtime_wave` | Marshal prepare request field. |
| `hero.marshal.prepare.evaluate.budgeted` | `args_path` `.kv` | `timeout_seconds` | Marshal prepare request field. |
| `hero.marshal.rollout` | public | `mode` | Required; enum: `plan`, `execute`. |
| `hero.marshal.rollout` | public | `args_path` | Required `.kv` rollout request packet path. |
| `hero.marshal.rollout` | public | `args_digest` | Optional by schema; execute requires digest binding. |
| `hero.marshal.rollout` | public | `include_machine_payload` | Optional response verbosity flag. |
| `hero.marshal.rollout` | `args_path` `.kv` | `rollout_id` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `rollout_attempt_id` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `idempotency_key` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `experiment_id` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `config_path` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `runtime_job_dir` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `replay_batch_index_path` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `runtime_exec_path` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `report_path` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `graph_order_fingerprint` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `asset_universe_digest` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `accounting_numeraire_node_id` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `target_node_ids` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `policy_set` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `max_steps` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `max_parallel_jobs` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `execution_profile` | Rollout request field. |
| `hero.marshal.rollout` | `args_path` `.kv` | `timeout_seconds` | Rollout request field. |
| `hero.marshal.inspect.run.latest_chain` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.run.latest_chain` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.run.latest_chain` | public | `job_ids` | Optional job selector list. |
| `hero.marshal.inspect.run.latest_chain` | public | `target_ids` | Optional target filter list. |
| `hero.marshal.inspect.run.latest_chain` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.run.training_state` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.run.training_state` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.run.training_state` | public | `job_ids` | Optional job selector list. |
| `hero.marshal.inspect.run.training_state` | public | `target_ids` | Optional target filter list. |
| `hero.marshal.inspect.run.training_state` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.run.single_job` | public | `job_id` | Required job selector. |
| `hero.marshal.inspect.run.single_job` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.run.single_job` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.run.single_job` | public | `target_ids` | Optional target filter list. |
| `hero.marshal.inspect.run.single_job` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.run.compare` | public | `baseline_job_id` | Required compare baseline job id. |
| `hero.marshal.inspect.run.compare` | public | `candidate_job_id` | Required compare candidate job id. |
| `hero.marshal.inspect.run.compare` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.run.compare` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.run.compare` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.target` | public | `target_id` | Required Lattice target id. |
| `hero.marshal.inspect.target` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.target` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.target` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.protocol.report` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.protocol.report` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.protocol.report` | public | `job_id` | Optional job selector. |
| `hero.marshal.inspect.protocol.report` | public | `job_ids` | Optional job selector list. |
| `hero.marshal.inspect.protocol.report` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.protocol.strict` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.protocol.strict` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.protocol.strict` | public | `job_id` | Optional job selector. |
| `hero.marshal.inspect.protocol.strict` | public | `job_ids` | Optional job selector list. |
| `hero.marshal.inspect.protocol.strict` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.protocol.strict` | public | `protocol_contract_fingerprint` | Required expected identity field. |
| `hero.marshal.inspect.protocol.strict` | public | `graph_order_fingerprint` | Required expected identity field. |
| `hero.marshal.inspect.protocol.strict` | public | `source_cursor_token` | Required expected identity field. |
| `hero.marshal.inspect.protocol.strict` | public | `vicreg_assembly_fingerprint` | Required expected identity field. |
| `hero.marshal.inspect.protocol.strict` | public | `mdn_assembly_fingerprint` | Required expected identity field. |
| `hero.marshal.inspect.spawn.by_id` | public | `component_spawn_id` | Required component-spawn selector. |
| `hero.marshal.inspect.spawn.by_id` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.spawn.by_id` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.spawn.by_id` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.spawn.by_label` | public | `component_spawn_label` | Required component-spawn label selector. |
| `hero.marshal.inspect.spawn.by_label` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.spawn.by_label` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.spawn.by_label` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.spawn.by_registry` | public | `component_spawn_registry_id` | Required component-spawn registry selector. |
| `hero.marshal.inspect.spawn.by_registry` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.spawn.by_registry` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.spawn.by_registry` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.spawn.by_fingerprint` | public | `component_spawn_fingerprint` | Required expected spawn identity field. |
| `hero.marshal.inspect.spawn.by_fingerprint` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.spawn.by_fingerprint` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.spawn.by_fingerprint` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.spawn.by_job` | public | `job_id` | Required Runtime job selector. |
| `hero.marshal.inspect.spawn.by_job` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.spawn.by_job` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.spawn.by_job` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.component` | public | `component_family_id` | Required component family id. |
| `hero.marshal.inspect.component` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.component` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.component` | public | `job_id` | Optional job selector. |
| `hero.marshal.inspect.component` | public | `job_ids` | Optional job selector list. |
| `hero.marshal.inspect.component` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.facts.summary` | public | `target_id` | Optional target selector. |
| `hero.marshal.inspect.facts.summary` | public | `fact_family_id` | Optional fact-family selector. |
| `hero.marshal.inspect.facts.summary` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.facts.summary` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.facts.summary` | public | `limit` | Optional non-negative row limit. |
| `hero.marshal.inspect.facts.summary` | public | `include_facts` | Optional boolean. |
| `hero.marshal.inspect.facts.summary` | public | `include_lineage` | Optional boolean. |
| `hero.marshal.inspect.facts.summary` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.facts.lineage` | public | `target_id` | Optional target selector. |
| `hero.marshal.inspect.facts.lineage` | public | `fact_family_id` | Optional fact-family selector. |
| `hero.marshal.inspect.facts.lineage` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.facts.lineage` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.facts.lineage` | public | `limit` | Optional non-negative row limit. |
| `hero.marshal.inspect.facts.lineage` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.facts.preview` | public | `target_id` | Optional target selector. |
| `hero.marshal.inspect.facts.preview` | public | `fact_family_id` | Optional fact-family selector. |
| `hero.marshal.inspect.facts.preview` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.facts.preview` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.facts.preview` | public | `limit` | Optional non-negative row limit. |
| `hero.marshal.inspect.facts.preview` | public | `include_lineage` | Optional boolean to include the compact lineage audit panel. |
| `hero.marshal.inspect.facts.preview` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.facts.preview.by_digest` | public | `target_id` | Optional target selector. |
| `hero.marshal.inspect.facts.preview.by_digest` | public | `fact_family_id` | Optional fact-family selector. |
| `hero.marshal.inspect.facts.preview.by_digest` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.facts.preview.by_digest` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.facts.preview.by_digest` | public | `limit` | Optional non-negative row limit. |
| `hero.marshal.inspect.facts.preview.by_digest` | public | `include_lineage` | Optional boolean to include the compact lineage audit panel. |
| `hero.marshal.inspect.facts.preview.by_digest` | public | `fact_digest` | Required exact fact preview selector. |
| `hero.marshal.inspect.facts.preview.by_digest` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.facts.preview.by_digest_prefix` | public | `target_id` | Optional target selector. |
| `hero.marshal.inspect.facts.preview.by_digest_prefix` | public | `fact_family_id` | Optional fact-family selector. |
| `hero.marshal.inspect.facts.preview.by_digest_prefix` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.facts.preview.by_digest_prefix` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.facts.preview.by_digest_prefix` | public | `limit` | Optional non-negative row limit. |
| `hero.marshal.inspect.facts.preview.by_digest_prefix` | public | `include_lineage` | Optional boolean to include the compact lineage audit panel. |
| `hero.marshal.inspect.facts.preview.by_digest_prefix` | public | `fact_digest_prefix` | Required unique fact preview selector. |
| `hero.marshal.inspect.facts.preview.by_digest_prefix` | public | `include_machine_payload` | Optional boolean. |
| `hero.marshal.inspect.facts.preview.by_index` | public | `target_id` | Optional target selector. |
| `hero.marshal.inspect.facts.preview.by_index` | public | `fact_family_id` | Optional fact-family selector. |
| `hero.marshal.inspect.facts.preview.by_index` | public | `config_path` | Optional config path selector. |
| `hero.marshal.inspect.facts.preview.by_index` | public | `runtime_root` | Optional Runtime root selector. |
| `hero.marshal.inspect.facts.preview.by_index` | public | `limit` | Optional non-negative row limit. |
| `hero.marshal.inspect.facts.preview.by_index` | public | `include_lineage` | Optional boolean to include the compact lineage audit panel. |
| `hero.marshal.inspect.facts.preview.by_index` | public | `fact_index` | Required fact row index selector. |
| `hero.marshal.inspect.facts.preview.by_index` | public | `include_machine_payload` | Optional boolean. |
