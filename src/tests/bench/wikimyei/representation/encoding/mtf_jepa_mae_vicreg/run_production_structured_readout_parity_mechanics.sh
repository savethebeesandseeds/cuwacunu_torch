#!/usr/bin/env bash

set -u

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../../../../" && pwd -P)"
cd "$repo_root" || exit 2

receipt_tmp="$(mktemp -d)"
case "$receipt_tmp" in
  /tmp/*) ;;
  *) exit 2 ;;
esac
trap 'rm -rf -- "$receipt_tmp"' EXIT

printf '%s\n' 'schema=wikimyei.mtf_jepa_mae_vicreg.srr2_mechanics.v1'
printf '%s\n' 'experiment=production-structured-readout-mechanics'

all_pass=true

run_case() {
  local label="$1"
  local command_text="$2"
  shift 2
  local output_path="$receipt_tmp/${label}.log"
  local exit_code=0
  local pass_marker=true

  /bin/bash -lc "$command_text" >"$output_path" 2>&1 || exit_code=$?
  if [[ "$exit_code" -ne 0 ]]; then
    pass_marker=false
  fi
  local required_marker
  for required_marker in "$@"; do
    if ! grep -Fq -- "$required_marker" "$output_path"; then
      pass_marker=false
    fi
  done

  printf 'srr2.mechanics.%s.command=%s\n' "$label" "$command_text"
  printf 'srr2.mechanics.%s.exit_code=%s\n' "$label" "$exit_code"
  printf 'srr2.mechanics.%s.pass_marker=%s\n' "$label" "$pass_marker"

  if [[ "$pass_marker" != true ]]; then
    all_pass=false
  fi
  printf -v "${label}_pass" '%s' "$pass_marker"
}

run_case production \
  'CUBLAS_WORKSPACE_CONFIG=:4096:8 ./.build/tests/test_production_structured_readout' \
  'SRR-2 production structured readout mechanics passed' \
  'cuda_cases=passed' \
  'production_shadow_cpu64_bytes_exact=true' \
  'production_shadow_cpu32_bytes_exact=true' \
  'legacy_policy_goldens_exact=true' \
  'training_or_augmentation_used=false'

run_case shadow \
  './.build/tests/test_structured_readout_shadow' \
  'SRR-1 structured readout shadow tests passed' \
  'cells=16 tokens_per_channel=24 output_shape=[B,3,32]' \
  'training_or_augmentation_used=false'

run_case gate \
  './.build/tests/test_production_structured_readout_parity_gate' \
  'production_structured_readout_parity_gate=PASS' \
  'terminal_precedence=invalid,parent,compatibility,sealed,parity,device,quality,success' \
  'coverage=18_retained+18_repeats_exact'

run_case auditor \
  './.build/tests/test_production_structured_readout_parity_log_auditor --self-test' \
  'srr2.audit.self_test=PASS'

run_case config \
  './.build/tests/test_wikimyei_graph_first_specs' \
  '[test_wikimyei_graph_first_specs] all checks passed'

run_case adapter \
  './.build/tests/test_jkimyei_channel_graph_first_launchers' \
  '[Jkimyei ChannelGraphFirstLaunchers test] all checks passed'

printf 'srr2.compatibility.legacy_enum_ordinals_exact=%s\n' "$config_pass"
printf 'srr2.compatibility.legacy_policy_names_exact=%s\n' "$config_pass"
printf 'srr2.compatibility.structured_policy_appended=%s\n' "$config_pass"
printf 'srr2.compatibility.structured_policy_name_exact=%s\n' "$config_pass"
printf 'srr2.compatibility.parser_round_trip_exact=%s\n' "$config_pass"
printf 'srr2.compatibility.unknown_policy_rejected=%s\n' "$config_pass"
printf 'srr2.compatibility.cpp_default_all_tokens=%s\n' "$config_pass"
printf 'srr2.compatibility.omitted_dsl_all_tokens=%s\n' "$config_pass"
printf 'srr2.compatibility.active_dsl_all_tokens=%s\n' "$config_pass"
printf 'srr2.compatibility.protocol_fingerprint_distinct=%s\n' "$config_pass"
printf 'srr2.compatibility.structured_checkpoint_round_trip_exact=%s\n' "$adapter_pass"
printf 'srr2.compatibility.legacy_checkpoint_all_tokens=%s\n' "$adapter_pass"
printf 'srr2.compatibility.legacy_checkpoint_does_not_inherit_structured=%s\n' "$adapter_pass"
printf 'srr2.compatibility.checkpoint_mismatch_rejected=%s\n' "$adapter_pass"
printf 'srr2.compatibility.malformed_checkpoint_rejected=%s\n' "$adapter_pass"
printf 'srr2.compatibility.legacy_policy_bytes_exact=%s\n' "$production_pass"
printf 'srr2.compatibility.public_selector_contract_exact=%s\n' "$production_pass"
printf 'srr2.compatibility.adapter_reaches_structured_selector=%s\n' "$adapter_pass"
printf '%s\n' 'srr2.mechanics.training_or_augmentation_used=false'
printf 'srr2.mechanics.local_contracts_exact=%s\n' "$all_pass"
printf 'srr2.mechanics.pass=%s\n' "$all_pass"

if [[ "$all_pass" != true ]]; then
  exit 1
fi
